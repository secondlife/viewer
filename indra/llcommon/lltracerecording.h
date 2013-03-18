/** 
 * @file lltracerecording.h
 * @brief Sampling object for collecting runtime statistics originating from lltrace.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLTRACERECORDING_H
#define LL_LLTRACERECORDING_H

#include "stdtypes.h"
#include "llpreprocessor.h"

#include "llpointer.h"
#include "lltimer.h"
#include "lltrace.h"

class LLStopWatchControlsMixinCommon
{
public:
	virtual ~LLStopWatchControlsMixinCommon() {}

	enum EPlayState
	{
		STOPPED,
		PAUSED,
		STARTED
	};

	virtual void start();
	virtual void stop();
	virtual void pause();
	virtual void resume();
	virtual void restart();
	virtual void reset();

	bool isStarted() const { return mPlayState == STARTED; }
	bool isPaused() const  { return mPlayState == PAUSED; }
	bool isStopped() const { return mPlayState == STOPPED; }
	EPlayState getPlayState() const { return mPlayState; }
	// force play state to specific value by calling appropriate handle* methods
	void setPlayState(EPlayState state);

protected:
	LLStopWatchControlsMixinCommon()
	:	mPlayState(STOPPED)
	{}

private:
	// trigger active behavior (without reset)
	virtual void handleStart(){};
	// stop active behavior
	virtual void handleStop(){};
	// clear accumulated state, can be called while started
	virtual void handleReset(){};

	EPlayState mPlayState;
};

template<typename DERIVED>
class LLStopWatchControlsMixin
:	public LLStopWatchControlsMixinCommon
{
public:
	typedef LLStopWatchControlsMixin<DERIVED> self_t;
	virtual void splitTo(DERIVED& other)
	{
		handleSplitTo(other);
	}

	virtual void splitFrom(DERIVED& other)
	{
		static_cast<self_t&>(other).handleSplitTo(*static_cast<DERIVED*>(this));
	}
private:
	// atomically stop this object while starting the other
	// no data can be missed in between stop and start
	virtual void handleSplitTo(DERIVED& other) {};

};

namespace LLTrace
{
	class RecordingBuffers
	{
	public:
		RecordingBuffers();

		void handOffTo(RecordingBuffers& other);
		void makePrimary();
		bool isPrimary() const;

		void makeUnique();

		void appendBuffers(const RecordingBuffers& other);
		void mergeBuffers(const RecordingBuffers& other);
		void resetBuffers(RecordingBuffers* other = NULL);

	protected:
		LLCopyOnWritePointer<AccumulatorBuffer<CountAccumulator<F64> > >		mCountsFloat;
		LLCopyOnWritePointer<AccumulatorBuffer<MeasurementAccumulator<F64> > >	mMeasurementsFloat;
		LLCopyOnWritePointer<AccumulatorBuffer<CountAccumulator<S64> > >		mCounts;
		LLCopyOnWritePointer<AccumulatorBuffer<MeasurementAccumulator<S64> > >	mMeasurements;
		LLCopyOnWritePointer<AccumulatorBuffer<TimeBlockAccumulator> >			mStackTimers;
		LLCopyOnWritePointer<AccumulatorBuffer<MemStatAccumulator> >			mMemStats;
	};

	class Recording : public LLStopWatchControlsMixin<Recording>, public RecordingBuffers
	{
	public:
		Recording();

		Recording(const Recording& other);
		~Recording();

		// accumulate data from subsequent, non-overlapping recording
		void appendRecording(const Recording& other);

		// gather data from recording, ignoring time relationship (for example, pulling data from slave threads)
		void mergeRecording(const Recording& other);

		// grab latest recorded data
		void update();

		// Timer accessors
		LLUnit<LLUnits::Seconds, F64> getSum(const TraceType<TimeBlockAccumulator>& stat) const;
		LLUnit<LLUnits::Seconds, F64> getSum(const TraceType<TimeBlockAccumulator::SelfTimeAspect>& stat) const;
		U32 getSum(const TraceType<TimeBlockAccumulator::CallCountAspect>& stat) const;

		LLUnit<LLUnits::Seconds, F64> getPerSec(const TraceType<TimeBlockAccumulator>& stat) const;
		LLUnit<LLUnits::Seconds, F64> getPerSec(const TraceType<TimeBlockAccumulator::SelfTimeAspect>& stat) const;
		F32 getPerSec(const TraceType<TimeBlockAccumulator::CallCountAspect>& stat) const;

		// Memory accessors
		LLUnit<LLUnits::Bytes, U32> getSum(const TraceType<MemStatAccumulator>& stat) const;
		LLUnit<LLUnits::Bytes, F32> getPerSec(const TraceType<MemStatAccumulator>& stat) const;

		// CountStatHandle accessors
		F64 getSum(const TraceType<CountAccumulator<F64> >& stat) const;
		S64 getSum(const TraceType<CountAccumulator<S64> >& stat) const;
		template <typename T>
		T getSum(const CountStatHandle<T>& stat) const
		{
			return (T)getSum(static_cast<const TraceType<CountAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >&> (stat));
		}

		F64 getPerSec(const TraceType<CountAccumulator<F64> >& stat) const;
		F64 getPerSec(const TraceType<CountAccumulator<S64> >& stat) const;
		template <typename T>
		T getPerSec(const CountStatHandle<T>& stat) const
		{
			return (T)getPerSec(static_cast<const TraceType<CountAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >&> (stat));
		}

		U32 getSampleCount(const TraceType<CountAccumulator<F64> >& stat) const;
		U32 getSampleCount(const TraceType<CountAccumulator<S64> >& stat) const;


		// MeasurementStatHandle accessors
		F64 getSum(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		S64 getSum(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getSum(const MeasurementStatHandle<T>& stat) const
		{
			return (T)getSum(static_cast<const TraceType<MeasurementAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >&> (stat));
		}

		F64 getMin(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		S64 getMin(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getMin(const MeasurementStatHandle<T>& stat) const
		{
			return (T)getMin(static_cast<const TraceType<MeasurementAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >&> (stat));
		}

		F64 getMax(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		S64 getMax(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getMax(const MeasurementStatHandle<T>& stat) const
		{
			return (T)getMax(static_cast<const TraceType<MeasurementAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >&> (stat));
		}

		F64 getMean(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		F64 getMean(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getMean(MeasurementStatHandle<T>& stat) const
		{
			return (T)getMean(static_cast<const TraceType<MeasurementAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >&> (stat));
		}

		F64 getStandardDeviation(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		F64 getStandardDeviation(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getStandardDeviation(const MeasurementStatHandle<T>& stat) const
		{
			return (T)getMean(static_cast<const TraceType<MeasurementAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >&> (stat));
		}

		F64 getLastValue(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		S64 getLastValue(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getLastValue(const MeasurementStatHandle<T>& stat) const
		{
			return (T)getLastValue(static_cast<const TraceType<MeasurementAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >&> (stat));
		}

		U32 getSampleCount(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		U32 getSampleCount(const TraceType<MeasurementAccumulator<S64> >& stat) const;

		LLUnit<LLUnits::Seconds, F64> getDuration() const { return LLUnit<LLUnits::Seconds, F64>(mElapsedSeconds); }

	private:
		friend class ThreadRecorder;

		// implementation for LLStopWatchControlsMixin
		/*virtual*/ void handleStart();
		/*virtual*/ void handleStop();
		/*virtual*/ void handleReset();
		/*virtual*/ void handleSplitTo(Recording& other);

		// returns data for current thread
		class ThreadRecorder* getThreadRecorder(); 

		LLTimer			mSamplingTimer;
		F64				mElapsedSeconds;
	};

	class LL_COMMON_API PeriodicRecording
	:	public LLStopWatchControlsMixin<PeriodicRecording>
	{
	public:
		PeriodicRecording(S32 num_periods, EPlayState state = STOPPED);
		PeriodicRecording(PeriodicRecording& recording);
		~PeriodicRecording();

		void nextPeriod();
		S32 getNumPeriods() { return mNumPeriods; }

		Recording& getLastRecordingPeriod()
		{
			return mRecordingPeriods[(mCurPeriod + mNumPeriods - 1) % mNumPeriods];
		}

		const Recording& getLastRecordingPeriod() const
		{
			return getPrevRecordingPeriod(1);
		}

		Recording& getCurRecordingPeriod()
		{
			return mRecordingPeriods[mCurPeriod];
		}

		const Recording& getCurRecordingPeriod() const
		{
			return mRecordingPeriods[mCurPeriod];
		}

		Recording& getPrevRecordingPeriod(S32 offset)
		{
			offset = llclamp(offset, 0, mNumPeriods - 1);
			return mRecordingPeriods[(mCurPeriod + mNumPeriods - offset) % mNumPeriods];
		}

		const Recording& getPrevRecordingPeriod(S32 offset) const
		{
			offset = llclamp(offset, 0, mNumPeriods - 1);
			return mRecordingPeriods[(mCurPeriod + mNumPeriods - offset) % mNumPeriods];
		}

		Recording snapshotCurRecordingPeriod() const
		{
			Recording recording_copy(getCurRecordingPeriod());
			recording_copy.stop();
			return recording_copy;
		}

		Recording& getTotalRecording();

		template <typename T>
		typename T::value_t getPeriodMin(const TraceType<T>& stat) const
		{
			typename T::value_t min_val = (std::numeric_limits<typename T::value_t>::max)();
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				min_val = llmin(min_val, mRecordingPeriods[i].getSum(stat));
			}
			return min_val;
		}

		template <typename T>
		F64 getPeriodMinPerSec(const TraceType<T>& stat) const
		{
			F64 min_val = (std::numeric_limits<F64>::max)();
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				min_val = llmin(min_val, mRecordingPeriods[i].getPerSec(stat));
			}
			return min_val;
		}

		template <typename T>
		typename T::value_t getPeriodMax(const TraceType<T>& stat) const
		{
			typename T::value_t max_val = (std::numeric_limits<typename T::value_t>::min)();
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				max_val = llmax(max_val, mRecordingPeriods[i].getSum(stat));
			}
			return max_val;
		}

		template <typename T>
		F64 getPeriodMaxPerSec(const TraceType<T>& stat) const
		{
			F64 max_val = (std::numeric_limits<F64>::min)();
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				max_val = llmax(max_val, mRecordingPeriods[i].getPerSec(stat));
			}
			return max_val;
		}

		template <typename T>
		typename MeanValueType<TraceType<T> >::type getPeriodMean(const TraceType<T>& stat) const
		{
			typename MeanValueType<TraceType<T> >::type mean = 0.0;
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				if (mRecordingPeriods[i].getDuration() > 0.f)
				{
					mean += mRecordingPeriods[i].getSum(stat);
				}
			}
			mean /= mNumPeriods;
			return mean;
		}

		template <typename T>
		typename MeanValueType<TraceType<T> >::type getPeriodMeanPerSec(const TraceType<T>& stat) const
		{
			typename MeanValueType<TraceType<T> >::type mean = 0.0;
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				if (mRecordingPeriods[i].getDuration() > 0.f)
				{
					mean += mRecordingPeriods[i].getPerSec(stat);
				}
			}
			mean /= mNumPeriods;
			return mean;
		}

		// implementation for LLStopWatchControlsMixin
		/*virtual*/ void start();
		/*virtual*/ void stop();
		/*virtual*/ void pause();
		/*virtual*/ void resume();
		/*virtual*/ void restart();
		/*virtual*/ void reset();
		/*virtual*/ void splitTo(PeriodicRecording& other);
		/*virtual*/ void splitFrom(PeriodicRecording& other);

	private:
		Recording*	mRecordingPeriods;
		Recording	mTotalRecording;
		bool		mTotalValid;
		S32			mNumPeriods,
					mCurPeriod;
	};

	PeriodicRecording& get_frame_recording();

	class ExtendableRecording
	:	public LLStopWatchControlsMixin<ExtendableRecording>
	{
	public:
		void extend();

		Recording& getAcceptedRecording() { return mAcceptedRecording; }

		// implementation for LLStopWatchControlsMixin
		/*virtual*/ void start();
		/*virtual*/ void stop();
		/*virtual*/ void pause();
		/*virtual*/ void resume();
		/*virtual*/ void restart();
		/*virtual*/ void reset();
		/*virtual*/ void splitTo(ExtendableRecording& other);
		/*virtual*/ void splitFrom(ExtendableRecording& other);
	private:
		Recording mAcceptedRecording;
		Recording mPotentialRecording;
	};
}

#endif // LL_LLTRACERECORDING_H
