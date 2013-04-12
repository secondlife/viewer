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
		PeriodicRecording(U32 num_periods, EPlayState state = STOPPED);

		void nextPeriod();
		U32 getNumPeriods() { return mRecordingPeriods.size(); }

		Recording& getLastRecording()
		{
			U32 num_periods = mRecordingPeriods.size();
			return mRecordingPeriods[(mCurPeriod + num_periods - 1) % num_periods];
		}

		const Recording& getLastRecording() const
		{
			return getPrevRecording(1);
		}

		Recording& getCurRecording()
		{
			return mRecordingPeriods[mCurPeriod];
		}

		const Recording& getCurRecording() const
		{
			return mRecordingPeriods[mCurPeriod];
		}

		Recording& getPrevRecording(U32 offset)
		{
			U32 num_periods = mRecordingPeriods.size();
			offset = llclamp(offset, 0u, num_periods - 1);
			return mRecordingPeriods[(mCurPeriod + num_periods - offset) % num_periods];
		}

		const Recording& getPrevRecording(U32 offset) const
		{
			U32 num_periods = mRecordingPeriods.size();
			offset = llclamp(offset, 0u, num_periods - 1);
			return mRecordingPeriods[(mCurPeriod + num_periods - offset) % num_periods];
		}

		Recording snapshotCurRecording() const
		{
			Recording recording_copy(getCurRecording());
			recording_copy.stop();
			return recording_copy;
		}

		template <typename T>
		typename T::value_t getPeriodMin(const TraceType<T>& stat, U32 num_periods = S32_MAX) const
		{
			size_t total_periods = mRecordingPeriods.size();
			num_periods = llmin(num_periods, total_periods);

			typename T::value_t min_val = (std::numeric_limits<typename T::value_t>::max)();
			for (S32 i = 1; i <= num_periods; i++)
			{
				S32 index = (mCurPeriod + total_periods - i) % total_periods;
				min_val = llmin(min_val, mRecordingPeriods[index].getSum(stat));
			}
			return min_val;
		}

		template <typename T>
		F64 getPeriodMinPerSec(const TraceType<T>& stat, U32 num_periods = S32_MAX) const
		{
			size_t total_periods = mRecordingPeriods.size();
			num_periods = llmin(num_periods, total_periods);

			F64 min_val = (std::numeric_limits<F64>::max)();
			for (S32 i = 1; i <= num_periods; i++)
			{
				S32 index = (mCurPeriod + total_periods - i) % total_periods;
				min_val = llmin(min_val, mRecordingPeriods[index].getPerSec(stat));
			}
			return min_val;
		}

		template <typename T>
		typename T::value_t getPeriodMax(const TraceType<T>& stat, U32 num_periods = S32_MAX) const
		{
			size_t total_periods = mRecordingPeriods.size();
			num_periods = llmin(num_periods, total_periods);

			typename T::value_t max_val = (std::numeric_limits<typename T::value_t>::min)();
			for (S32 i = 1; i <= num_periods; i++)
			{
				S32 index = (mCurPeriod + total_periods - i) % total_periods;
				max_val = llmax(max_val, mRecordingPeriods[index].getSum(stat));
			}
			return max_val;
		}

		template <typename T>
		F64 getPeriodMaxPerSec(const TraceType<T>& stat, U32 num_periods = S32_MAX) const
		{
			size_t total_periods = mRecordingPeriods.size();
			num_periods = llmin(num_periods, total_periods);

			F64 max_val = (std::numeric_limits<F64>::min)();
			for (S32 i = 1; i <= num_periods; i++)
			{
				S32 index = (mCurPeriod + total_periods - i) % total_periods;
				max_val = llmax(max_val, mRecordingPeriods[index].getPerSec(stat));
			}
			return max_val;
		}

		template <typename T>
		typename T::value_t getPeriodMean(const TraceType<T>& stat, U32 num_periods = S32_MAX) const
		{
			size_t total_periods = mRecordingPeriods.size();
			num_periods = llmin(num_periods, total_periods);

			typename T::value_t mean = 0.0;
			if (num_periods <= 0) { return mean; }

			for (S32 i = 1; i <= num_periods; i++)
			{
				S32 index = (mCurPeriod + total_periods - i) % total_periods;
				if (mRecordingPeriods[index].getDuration() > 0.f)
				{
					mean += mRecordingPeriods[index].getSum(stat);
				}
			}
			mean /= num_periods;
			return mean;
		}

		template <typename T>
		typename T::value_t getPeriodMeanPerSec(const TraceType<T>& stat, U32 num_periods = S32_MAX) const
		{
			size_t total_periods = mRecordingPeriods.size();
			num_periods = llmin(num_periods, total_periods);

			typename T::value_t mean = 0.0;
			if (num_periods <= 0) { return mean; }

			for (S32 i = 1; i <= num_periods; i++)
			{
				S32 index = (mCurPeriod + total_periods - i) % total_periods;
				if (mRecordingPeriods[index].getDuration() > 0.f)
				{
					mean += mRecordingPeriods[index].getPerSec(stat);
				}
			}
			mean /= num_periods;
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
		std::vector<Recording>	mRecordingPeriods;
		Recording	mTotalRecording;
		const bool	mAutoResize;
		S32			mCurPeriod;
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
