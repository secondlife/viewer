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

template<typename DERIVED>
class LL_COMMON_API LLVCRControlsMixinInterface
{
public:
	virtual ~LLVCRControlsMixinInterface() {}
	// trigger data accumulation (without reset)
	virtual void handleStart() = 0;
	// stop data accumulation, should put object in queryable state
	virtual void handleStop() = 0;
	// clear accumulated values, can be called while started
	virtual void handleReset() = 0;
	// atomically stop this object while starting the other
	// no data can be missed in between stop and start
	virtual void handleSplitTo(DERIVED& other) = 0;
};

template<typename DERIVED>
class LL_COMMON_API LLVCRControlsMixin
:	private LLVCRControlsMixinInterface<DERIVED>
{
public:
	enum EPlayState
	{
		STOPPED,
		PAUSED,
		STARTED
	};

	void start()
	{
		switch (mPlayState)
		{
		case STOPPED:
			handleReset();
			handleStart();
			break;
		case PAUSED:
			handleStart();
			break;
		case STARTED:
			handleReset();
			break;
		}
		mPlayState = STARTED;
	}

	void stop()
	{
		switch (mPlayState)
		{
		case STOPPED:
			break;
		case PAUSED:
			handleStop();
			break;
		case STARTED:
			break;
		}
		mPlayState = STOPPED;
	}

	void pause()
	{
		switch (mPlayState)
		{
		case STOPPED:
			break;
		case PAUSED:
			break;
		case STARTED:
			handleStop();
			break;
		}
		mPlayState = PAUSED;
	}

	void resume()
	{
		switch (mPlayState)
		{
		case STOPPED:
			handleStart();
			break;
		case PAUSED:
			handleStart();
			break;
		case STARTED:
			break;
		}
		mPlayState = STARTED;
	}

	void restart()
	{
		switch (mPlayState)
		{
		case STOPPED:
			handleReset();
			handleStart();
			break;
		case PAUSED:
			handleReset();
			handleStart();
			break;
		case STARTED:
			handleReset();
			break;
		}
		mPlayState = STARTED;
	}

	void reset()
	{
		handleReset();
	}

	void splitTo(DERIVED& other)
	{
		onSplitTo(other);
	}

	void splitFrom(DERIVED& other)
	{
		other.onSplitTo(*this);
	}

	bool isStarted() { return mPlayState == STARTED; }
	bool isPaused()  { return mPlayState == PAUSED; }
	bool isStopped() { return mPlayState == STOPPED; }
	EPlayState getPlayState() { return mPlayState; }

protected:

	LLVCRControlsMixin()
	:	mPlayState(STOPPED)
	{}

private:
	EPlayState mPlayState;
};

namespace LLTrace
{
	//template<typename T, typename IS_UNIT> class Rate;
	//template<typename T, typename IS_UNIT> class Measurement;
	//template<typename T> class Count;
	//template<typename T> class AccumulatorBuffer;
	//template<typename T> class RateAccumulator;
	//template<typename T> class MeasurementAccumulator;
	//class TimerAccumulator;

	class LL_COMMON_API Recording : public LLVCRControlsMixin<Recording>
	{
	public:
		Recording();

		~Recording();

		void makePrimary();
		bool isPrimary() const;

		void mergeRecording(const Recording& other);
		void mergeRecordingDelta(const Recording& baseline, const Recording& target);

		void update();

		// Rate accessors
		template <typename T, typename IS_UNIT>
		typename Rate<T, IS_UNIT>::base_unit_t getSum(const Rate<T, IS_UNIT>& stat) const
		{
			return (typename Rate<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mRates).getSum();
		}

		template <typename T, typename IS_UNIT>
		typename Rate<T, IS_UNIT>::base_unit_t getPerSec(const Rate<T, IS_UNIT>& stat) const
		{
			return (typename Rate<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mRates).getSum() / mElapsedSeconds;
		}

		// Measurement accessors
		template <typename T, typename IS_UNIT>
		typename Measurement<T, IS_UNIT>::base_unit_t getSum(const Measurement<T, IS_UNIT>& stat) const
		{
			return (typename Measurement<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mMeasurements).getSum();

		}

		template <typename T, typename IS_UNIT>
		typename Measurement<T, IS_UNIT>::base_unit_t getPerSec(const Measurement<T, IS_UNIT>& stat) const
		{
			return (typename Rate<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mMeasurements).getSum() / mElapsedSeconds;
		}

		template <typename T, typename IS_UNIT>
		typename Measurement<T, IS_UNIT>::base_unit_t getMin(const Measurement<T, IS_UNIT>& stat) const
		{
			return (typename Measurement<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mMeasurements).getMin();
		}

		template <typename T, typename IS_UNIT>
		typename Measurement<T, IS_UNIT>::base_unit_t getMax(const Measurement<T, IS_UNIT>& stat) const
		{
			return (typename Measurement<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mMeasurements).getMax();
		}

		template <typename T, typename IS_UNIT>
		typename Measurement<T, IS_UNIT>::base_unit_t getMean(const Measurement<T, IS_UNIT>& stat) const
		{
			return (typename Measurement<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mMeasurements).getMean();
		}

		template <typename T, typename IS_UNIT>
		typename Measurement<T, IS_UNIT>::base_unit_t getStandardDeviation(const Measurement<T, IS_UNIT>& stat) const
		{
			return (typename Measurement<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mMeasurements).getStandardDeviation();
		}

		template <typename T, typename IS_UNIT>
		typename Measurement<T, IS_UNIT>::base_unit_t getLastValue(const Measurement<T, IS_UNIT>& stat) const
		{
			return (typename Measurement<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mMeasurements).getLastValue();
		}

		// Count accessors
		template <typename T>
		typename Count<T>::base_unit_t getSum(const Count<T>& stat) const
		{
			return getSum(stat.mTotal);
		}

		template <typename T>
		typename Count<T>::base_unit_t getPerSec(const Count<T>& stat) const
		{
			return getPerSec(stat.mTotal);
		}

		template <typename T>
		typename Count<T>::base_unit_t getIncrease(const Count<T>& stat) const
		{
			return getPerSec(stat.mTotal);
		}

		template <typename T>
		typename Count<T>::base_unit_t getIncreasePerSec(const Count<T>& stat) const
		{
			return getPerSec(stat.mIncrease);
		}

		template <typename T>
		typename Count<T>::base_unit_t getDecrease(const Count<T>& stat) const
		{
			return getPerSec(stat.mDecrease);
		}

		template <typename T>
		typename Count<T>::base_unit_t getDecreasePerSec(const Count<T>& stat) const
		{
			return getPerSec(stat.mDecrease);
		}

		template <typename T>
		typename Count<T>::base_unit_t getChurn(const Count<T>& stat) const
		{
			return getIncrease(stat) + getDecrease(stat);
		}

		template <typename T>
		typename Count<T>::base_unit_t getChurnPerSec(const Count<T>& stat) const
		{
			return getIncreasePerSec(stat) + getDecreasePerSec(stat);
		}

		F64 getSampleTime() const { return mElapsedSeconds; }

	private:
		friend class PeriodicRecording;
		// implementation for LLVCRControlsMixin
		/*virtual*/ void handleStart();
		/*virtual*/ void handleStop();
		/*virtual*/ void handleReset();
		/*virtual*/ void handleSplitTo(Recording& other);


		friend class ThreadRecorder;
		// returns data for current thread
		class ThreadRecorder* getThreadRecorder(); 

		LLCopyOnWritePointer<AccumulatorBuffer<RateAccumulator<F32> > >			mRates;
		LLCopyOnWritePointer<AccumulatorBuffer<MeasurementAccumulator<F32> > >	mMeasurements;
		LLCopyOnWritePointer<AccumulatorBuffer<TimerAccumulator> >				mStackTimers;

		LLTimer			mSamplingTimer;
		F64				mElapsedSeconds;
	};

	class LL_COMMON_API PeriodicRecording
	:	public LLVCRControlsMixin<PeriodicRecording>
	{
	public:
		PeriodicRecording(S32 num_periods)
		:	mNumPeriods(num_periods),
			mCurPeriod(0),
			mTotalValid(false),
			mRecordingPeriods(new Recording[num_periods])
	{
			llassert(mNumPeriods > 0);
		}

		~PeriodicRecording()
		{
			delete[] mRecordingPeriods;
		}

		void nextPeriod()
		{
			EPlayState play_state = getPlayState();
			getCurRecordingPeriod().stop();
			mCurPeriod = (mCurPeriod + 1) % mNumPeriods;
			switch(play_state)
			{
			case STOPPED:
				break;
			case PAUSED:
				getCurRecordingPeriod().pause();
				break;
			case STARTED:
				getCurRecordingPeriod().start();
				break;
			}
			// new period, need to recalculate total
			mTotalValid = false;
		}

		Recording& getCurRecordingPeriod()
		{
			return mRecordingPeriods[mCurPeriod];
		}

		const Recording& getCurRecordingPeriod() const
		{
			return mRecordingPeriods[mCurPeriod];
		}

		Recording& getTotalRecording()
		{
			if (!mTotalValid)
			{
				mTotalRecording.reset();
				for (S32 i = (mCurPeriod + 1) % mNumPeriods; i < mCurPeriod; i++)
				{
					mTotalRecording.mergeRecording(mRecordingPeriods[i]);
				}
			}
			mTotalValid = true;
			return mTotalRecording;
		}

	private:
		// implementation for LLVCRControlsMixin
		/*virtual*/ void handleStart()
		{
			getCurRecordingPeriod().handleStart();
		}

		/*virtual*/ void handleStop()
		{
			getCurRecordingPeriod().handleStop();
		}

		/*virtual*/ void handleReset()
		{
			getCurRecordingPeriod().handleReset();
		}

		/*virtual*/ void handleSplitTo(PeriodicRecording& other)
		{
			getCurRecordingPeriod().handleSplitTo(other.getCurRecordingPeriod());
		}

		Recording*	mRecordingPeriods;
		Recording	mTotalRecording;
		bool		mTotalValid;
		S32			mNumPeriods,
					mCurPeriod;
	};
}

#endif // LL_LLTRACERECORDING_H
