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

class LL_COMMON_API LLVCRControlsMixinCommon
{
public:
	virtual ~LLVCRControlsMixinCommon() {}

	enum EPlayState
	{
		STOPPED,
		PAUSED,
		STARTED
	};

	void start();
	void stop();
	void pause();
	void resume();
	void restart();
	void reset();

	bool isStarted() { return mPlayState == STARTED; }
	bool isPaused()  { return mPlayState == PAUSED; }
	bool isStopped() { return mPlayState == STOPPED; }
	EPlayState getPlayState() { return mPlayState; }

protected:
	LLVCRControlsMixinCommon()
	:	mPlayState(STOPPED)
	{}

private:
	// trigger data accumulation (without reset)
	virtual void handleStart() = 0;
	// stop data accumulation, should put object in queryable state
	virtual void handleStop() = 0;
	// clear accumulated values, can be called while started
	virtual void handleReset() = 0;

	EPlayState mPlayState;
};

template<typename DERIVED>
class LLVCRControlsMixin
:	public LLVCRControlsMixinCommon
{
public:
	void splitTo(DERIVED& other)
	{
		onSplitTo(other);
	}

	void splitFrom(DERIVED& other)
	{
		other.onSplitTo(*this);
	}
private:
	// atomically stop this object while starting the other
	// no data can be missed in between stop and start
	virtual void handleSplitTo(DERIVED& other) = 0;

};

namespace LLTrace
{
	class LL_COMMON_API Recording : public LLVCRControlsMixin<Recording>
	{
	public:
		Recording();

		~Recording();

		void makePrimary();
		bool isPrimary() const;

		void mergeRecording(const Recording& other);

		void update();

		// Count accessors
		F64 getSum(const TraceType<CountAccumulator<F64> >& stat) const;
		S64 getSum(const TraceType<CountAccumulator<S64> >& stat) const;
		template <typename T>
		T getSum(const Count<T, typename T::is_unit_tag_t>& stat) const
		{
			return (T)getSum(static_cast<const TraceType<CountAccumulator<StorageType<T>::type_t> >&> (stat));
		}

		F64 getPerSec(const TraceType<CountAccumulator<F64> >& stat) const;
		F64 getPerSec(const TraceType<CountAccumulator<S64> >& stat) const;
		template <typename T>
		T getPerSec(const Count<T, typename T::is_unit_tag_t>& stat) const
		{
			return (T)getPerSec(static_cast<const TraceType<CountAccumulator<StorageType<T>::type_t> >&> (stat));
		}

		// Measurement accessors
		F64 getSum(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		S64 getSum(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getSum(const Measurement<T, typename T::is_unit_tag_t>& stat) const
		{
			return (T)getSum(static_cast<const TraceType<MeasurementAccumulator<StorageType<T>::type_t> >&> (stat));
		}

		F64 getPerSec(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		F64 getPerSec(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getPerSec(const Measurement<T, typename T::is_unit_tag_t>& stat) const
		{
			return (T)getPerSec(static_cast<const TraceType<MeasurementAccumulator<StorageType<T>::type_t> >&> (stat));
		}

		F64 getMin(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		S64 getMin(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getMin(const Measurement<T, typename T::is_unit_tag_t>& stat) const
		{
			return (T)getMin(static_cast<const TraceType<MeasurementAccumulator<StorageType<T>::type_t> >&> (stat));
		}

		F64 getMax(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		S64 getMax(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getMax(const Measurement<T, typename T::is_unit_tag_t>& stat) const
		{
			return (T)getMax(static_cast<const TraceType<MeasurementAccumulator<StorageType<T>::type_t> >&> (stat));
		}

		F64 getMean(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		F64 getMean(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getMean(Measurement<T, typename T::is_unit_tag_t>& stat) const
		{
			return (T)getMean(static_cast<const TraceType<MeasurementAccumulator<StorageType<T>::type_t> >&> (stat));
		}

		F64 getStandardDeviation(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		F64 getStandardDeviation(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getStandardDeviation(const Measurement<T, typename T::is_unit_tag_t>& stat) const
		{
			return (T)getMean(static_cast<const TraceType<MeasurementAccumulator<StorageType<T>::type_t> >&> (stat));
		}

		F64 getLastValue(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		S64 getLastValue(const TraceType<MeasurementAccumulator<S64> >& stat) const;
		template <typename T>
		T getLastValue(const Measurement<T, typename T::is_unit_tag_t>& stat) const
		{
			return (T)getLastValue(static_cast<const TraceType<MeasurementAccumulator<StorageType<T>::type_t> >&> (stat));
		}

		U32 getSampleCount(const TraceType<MeasurementAccumulator<F64> >& stat) const;
		U32 getSampleCount(const TraceType<MeasurementAccumulator<S64> >& stat) const;

		LLUnit::Seconds<F64> getDuration() const { return mElapsedSeconds; }

		// implementation for LLVCRControlsMixin
		/*virtual*/ void handleStart();
		/*virtual*/ void handleStop();
		/*virtual*/ void handleReset();
		/*virtual*/ void handleSplitTo(Recording& other);

	private:
		friend class ThreadRecorder;
		// returns data for current thread
		class ThreadRecorder* getThreadRecorder(); 

		LLCopyOnWritePointer<AccumulatorBuffer<CountAccumulator<F64> > >		mCountsFloat;
		LLCopyOnWritePointer<AccumulatorBuffer<MeasurementAccumulator<F64> > >	mMeasurementsFloat;
		LLCopyOnWritePointer<AccumulatorBuffer<CountAccumulator<S64> > >		mCounts;
		LLCopyOnWritePointer<AccumulatorBuffer<MeasurementAccumulator<S64> > >	mMeasurements;
		LLCopyOnWritePointer<AccumulatorBuffer<TimerAccumulator> >				mStackTimers;

		LLTimer			mSamplingTimer;
		F64				mElapsedSeconds;
	};

	class LL_COMMON_API PeriodicRecording
	:	public LLVCRControlsMixin<PeriodicRecording>
	{
	public:
		PeriodicRecording(S32 num_periods);
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
			return mRecordingPeriods[(mCurPeriod + mNumPeriods - offset) % mNumPeriods];
		}

		const Recording& getPrevRecordingPeriod(S32 offset) const
		{
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
		typename T getPeriodMin(const TraceType<CountAccumulator<T> >& stat) const
		{
			T min_val = std::numeric_limits<T>::max();
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				min_val = llmin(min_val, mRecordingPeriods[i].getSum(stat));
			}
			return (T)min_val;
		}

		template <typename T>
		F64 getPeriodMinPerSec(const TraceType<CountAccumulator<T> >& stat) const
		{
			F64 min_val = std::numeric_limits<F64>::max();
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				min_val = llmin(min_val, mRecordingPeriods[i].getPerSec(stat));
			}
			return min_val;
		}

		template <typename T>
		T getPeriodMax(const TraceType<CountAccumulator<T> >& stat) const
		{
			T max_val = std::numeric_limits<T>::min();
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				max_val = llmax(max_val, mRecordingPeriods[i].getSum(stat));
			}
			return max_val;
		}

		template <typename T>
		F64 getPeriodMaxPerSec(const TraceType<CountAccumulator<T> >& stat) const
		{
			F64 max_val = std::numeric_limits<F64>::min();
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				max_val = llmax(max_val, mRecordingPeriods[i].getPerSec(stat));
			}
			return max_val;
		}

		template <typename T>
		F64 getPeriodMean(const TraceType<CountAccumulator<T> >& stat) const
		{
			F64 mean = 0.0;
			F64 count = 0;
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				if (mRecordingPeriods[i].getDuration() > 0.f)
				{
					count++;
					mean += mRecordingPeriods[i].getSum(stat);
				}
			}
			mean /= (F64)mNumPeriods;
			return mean;
		}

		template <typename T>
		F64 getPeriodMeanPerSec(const TraceType<CountAccumulator<T> >& stat) const
		{
			F64 mean = 0.0;
			F64 count = 0;
			for (S32 i = 0; i < mNumPeriods; i++)
			{
				if (mRecordingPeriods[i].getDuration() > 0.f)
				{
					count++;
					mean += mRecordingPeriods[i].getPerSec(stat);
				}
			}
			mean /= count;
			return mean;
		}

	private:

		// implementation for LLVCRControlsMixin
		/*virtual*/ void handleStart();
		/*virtual*/ void handleStop();
		/*virtual*/ void handleReset();

		/*virtual*/ void handleSplitTo(PeriodicRecording& other);

		Recording*	mRecordingPeriods;
		Recording	mTotalRecording;
		bool		mTotalValid;
		S32			mNumPeriods,
					mCurPeriod;
	};

	PeriodicRecording& get_frame_recording();

	class ExtendableRecording
	:	public LLVCRControlsMixin<ExtendableRecording>
	{
		void extend();

	private:
		// implementation for LLVCRControlsMixin
		/*virtual*/ void handleStart();
		/*virtual*/ void handleStop();
		/*virtual*/ void handleReset();
		/*virtual*/ void handleSplitTo(ExtendableRecording& other);

		Recording mAcceptedRecording;
		Recording mPotentialRecording;
	};
}

#endif // LL_LLTRACERECORDING_H
