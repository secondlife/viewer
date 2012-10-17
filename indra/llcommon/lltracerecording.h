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
		template <typename T, typename IS_UNIT>
		typename Count<T, IS_UNIT>::base_unit_t getSum(const Count<T, IS_UNIT>& stat) const
		{
			return (typename Count<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mCounts).getSum();
		}

		template <typename T, typename IS_UNIT>
		typename Count<T, IS_UNIT>::base_unit_t getPerSec(const Count<T, IS_UNIT>& stat) const
		{
			return (typename Count<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mCounts).getSum() / mElapsedSeconds;
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
			return (typename Count<T, IS_UNIT>::base_unit_t)stat.getAccumulator(mMeasurements).getSum() / mElapsedSeconds;
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
		typename Measurement<T, IS_UNIT>::base_unit_t getMean(Measurement<T, IS_UNIT>& stat) const
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

		template <typename T, typename IS_UNIT>
		U32 getSampleCount(const Measurement<T, IS_UNIT>& stat) const
		{
			return stat.getAccumulator(mMeasurements).getSampleCount();
		}

		F64 getDuration() const { return mElapsedSeconds; }

		// implementation for LLVCRControlsMixin
		/*virtual*/ void handleStart();
		/*virtual*/ void handleStop();
		/*virtual*/ void handleReset();
		/*virtual*/ void handleSplitTo(Recording& other);

	private:
		friend class ThreadRecorder;
		// returns data for current thread
		class ThreadRecorder* getThreadRecorder(); 

		LLCopyOnWritePointer<AccumulatorBuffer<CountAccumulator<F64> > >		mCounts;
		LLCopyOnWritePointer<AccumulatorBuffer<MeasurementAccumulator<F64> > >	mMeasurements;
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

		Recording& getLastRecordingPeriod()
		{
			return mRecordingPeriods[(mCurPeriod + mNumPeriods - 1) % mNumPeriods];
		}

		const Recording& getLastRecordingPeriod() const
		{
			return mRecordingPeriods[(mCurPeriod + mNumPeriods - 1) % mNumPeriods];
		}

		Recording& getCurRecordingPeriod()
		{
			return mRecordingPeriods[mCurPeriod];
		}

		const Recording& getCurRecordingPeriod() const
		{
			return mRecordingPeriods[mCurPeriod];
		}

		Recording snapshotCurRecordingPeriod() const
		{
			Recording recording_copy(getCurRecordingPeriod());
			recording_copy.stop();
			return recording_copy;
		}

		Recording& getTotalRecording();

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
