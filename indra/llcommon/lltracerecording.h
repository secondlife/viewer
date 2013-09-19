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
#include "lltraceaccumulators.h"

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

	void start();
	void stop();
	void pause();
	void resume();
	void restart();
	void reset();

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
	virtual void handleStart() = 0;
	// stop active behavior
	virtual void handleStop() = 0;
	// clear accumulated state, can be called while started
	virtual void handleReset() = 0;

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
		EPlayState play_state = getPlayState();
		stop();
		other.reset();

		handleSplitTo(other);

		other.setPlayState(play_state);
	}

	virtual void splitFrom(DERIVED& other)
	{
		static_cast<self_t&>(other).handleSplitTo(*static_cast<DERIVED*>(this));
	}
private:
	self_t& operator = (const self_t& other)
	{
		// don't do anything, derived class must implement logic
	}

	// atomically stop this object while starting the other
	// no data can be missed in between stop and start
	virtual void handleSplitTo(DERIVED& other) {};

};

namespace LLTrace
{
	template<typename T>
	class TraceType;

	template<typename T>
	class CountStatHandle;

	template<typename T>
	class SampleStatHandle;

	template<typename T>
	class EventStatHandle;

	template<typename T>
	struct RelatedTypes
	{
		typedef F64 fractional_t;
		typedef T	sum_t;
	};

	template<typename T, typename UNIT_T>
	struct RelatedTypes<LLUnit<T, UNIT_T> >
	{
		typedef LLUnit<typename RelatedTypes<T>::fractional_t, UNIT_T> fractional_t;
		typedef LLUnit<typename RelatedTypes<T>::sum_t, UNIT_T> sum_t;
	};

	template<>
	struct RelatedTypes<bool>
	{
		typedef F64 fractional_t;
		typedef U32 sum_t;
	};

	class Recording 
	:	public LLStopWatchControlsMixin<Recording>
	{
	public:
		Recording(EPlayState state = LLStopWatchControlsMixinCommon::STOPPED);

		Recording(const Recording& other);
		~Recording();

		Recording& operator = (const Recording& other);

		// accumulate data from subsequent, non-overlapping recording
		void appendRecording(Recording& other);

		// grab latest recorded data
		void update();

		// ensure that buffers are exclusively owned by this recording
		void makeUnique() { mBuffers.makeUnique(); }

		// Timer accessors
		F64Seconds getSum(const TraceType<TimeBlockAccumulator>& stat);
		F64Seconds getSum(const TraceType<TimeBlockAccumulator::SelfTimeFacet>& stat);
		U32 getSum(const TraceType<TimeBlockAccumulator::CallCountFacet>& stat);

		F64Seconds getPerSec(const TraceType<TimeBlockAccumulator>& stat);
		F64Seconds getPerSec(const TraceType<TimeBlockAccumulator::SelfTimeFacet>& stat);
		F32 getPerSec(const TraceType<TimeBlockAccumulator::CallCountFacet>& stat);

		// Memory accessors
		bool hasValue(const TraceType<MemStatAccumulator>& stat);
		bool hasValue(const TraceType<MemStatAccumulator::ShadowMemFacet>& stat);

		F64Bytes getMin(const TraceType<MemStatAccumulator>& stat);
		F64Bytes getMean(const TraceType<MemStatAccumulator>& stat);
		F64Bytes getMax(const TraceType<MemStatAccumulator>& stat);
		F64Bytes getStandardDeviation(const TraceType<MemStatAccumulator>& stat);
		F64Bytes getLastValue(const TraceType<MemStatAccumulator>& stat);

		F64Bytes getMin(const TraceType<MemStatAccumulator::ShadowMemFacet>& stat);
		F64Bytes getMean(const TraceType<MemStatAccumulator::ShadowMemFacet>& stat);
		F64Bytes getMax(const TraceType<MemStatAccumulator::ShadowMemFacet>& stat);
		F64Bytes getStandardDeviation(const TraceType<MemStatAccumulator::ShadowMemFacet>& stat);
		F64Bytes getLastValue(const TraceType<MemStatAccumulator::ShadowMemFacet>& stat);

		U32 getSum(const TraceType<MemStatAccumulator::AllocationCountFacet>& stat);
		U32 getSum(const TraceType<MemStatAccumulator::DeallocationCountFacet>& stat);

		// CountStatHandle accessors
		F64 getSum(const TraceType<CountAccumulator>& stat);
		template <typename T>
		typename RelatedTypes<T>::sum_t getSum(const CountStatHandle<T>& stat)
		{
			return (typename RelatedTypes<T>::sum_t)getSum(static_cast<const TraceType<CountAccumulator>&> (stat));
		}

		F64 getPerSec(const TraceType<CountAccumulator>& stat);
		template <typename T>
		typename RelatedTypes<T>::fractional_t getPerSec(const CountStatHandle<T>& stat)
		{
			return (typename RelatedTypes<T>::fractional_t)getPerSec(static_cast<const TraceType<CountAccumulator>&> (stat));
		}

		U32 getSampleCount(const TraceType<CountAccumulator>& stat);


		// SampleStatHandle accessors
		bool hasValue(const TraceType<SampleAccumulator>& stat);

		F64 getMin(const TraceType<SampleAccumulator>& stat);
		template <typename T>
		T getMin(const SampleStatHandle<T>& stat)
		{
			return (T)getMin(static_cast<const TraceType<SampleAccumulator>&> (stat));
		}

		F64 getMax(const TraceType<SampleAccumulator>& stat);
		template <typename T>
		T getMax(const SampleStatHandle<T>& stat)
		{
			return (T)getMax(static_cast<const TraceType<SampleAccumulator>&> (stat));
		}

		F64 getMean(const TraceType<SampleAccumulator>& stat);
		template <typename T>
		typename RelatedTypes<T>::fractional_t getMean(SampleStatHandle<T>& stat)
		{
			return (typename RelatedTypes<T>::fractional_t)getMean(static_cast<const TraceType<SampleAccumulator>&> (stat));
		}

		F64 getStandardDeviation(const TraceType<SampleAccumulator>& stat);
		template <typename T>
		typename RelatedTypes<T>::fractional_t getStandardDeviation(const SampleStatHandle<T>& stat)
		{
			return (typename RelatedTypes<T>::fractional_t)getStandardDeviation(static_cast<const TraceType<SampleAccumulator>&> (stat));
		}

		F64 getLastValue(const TraceType<SampleAccumulator>& stat);
		template <typename T>
		T getLastValue(const SampleStatHandle<T>& stat)
		{
			return (T)getLastValue(static_cast<const TraceType<SampleAccumulator>&> (stat));
		}

		U32 getSampleCount(const TraceType<SampleAccumulator>& stat);

		// EventStatHandle accessors
		bool hasValue(const TraceType<EventAccumulator>& stat);

		F64 getSum(const TraceType<EventAccumulator>& stat);
		template <typename T>
		typename RelatedTypes<T>::sum_t getSum(const EventStatHandle<T>& stat)
		{
			return (typename RelatedTypes<T>::sum_t)getSum(static_cast<const TraceType<EventAccumulator>&> (stat));
		}

		F64 getMin(const TraceType<EventAccumulator>& stat);
		template <typename T>
		T getMin(const EventStatHandle<T>& stat)
		{
			return (T)getMin(static_cast<const TraceType<EventAccumulator>&> (stat));
		}

		F64 getMax(const TraceType<EventAccumulator>& stat);
		template <typename T>
		T getMax(const EventStatHandle<T>& stat)
		{
			return (T)getMax(static_cast<const TraceType<EventAccumulator>&> (stat));
		}

		F64 getMean(const TraceType<EventAccumulator>& stat);
		template <typename T>
		typename RelatedTypes<T>::fractional_t getMean(EventStatHandle<T>& stat)
		{
			return (typename RelatedTypes<T>::fractional_t)getMean(static_cast<const TraceType<EventAccumulator>&> (stat));
		}

		F64 getStandardDeviation(const TraceType<EventAccumulator>& stat);
		template <typename T>
		typename RelatedTypes<T>::fractional_t getStandardDeviation(const EventStatHandle<T>& stat)
		{
			return (typename RelatedTypes<T>::fractional_t)getStandardDeviation(static_cast<const TraceType<EventAccumulator>&> (stat));
		}

		F64 getLastValue(const TraceType<EventAccumulator>& stat);
		template <typename T>
		T getLastValue(const EventStatHandle<T>& stat)
		{
			return (T)getLastValue(static_cast<const TraceType<EventAccumulator>&> (stat));
		}

		U32 getSampleCount(const TraceType<EventAccumulator>& stat);

		F64Seconds getDuration() const { return mElapsedSeconds; }

	protected:
		friend class ThreadRecorder;

		// implementation for LLStopWatchControlsMixin
		/*virtual*/ void handleStart();
		/*virtual*/ void handleStop();
		/*virtual*/ void handleReset();
		/*virtual*/ void handleSplitTo(Recording& other);

		// returns data for current thread
		class ThreadRecorder* getThreadRecorder(); 

		LLTimer											mSamplingTimer;
		F64Seconds					mElapsedSeconds;
		LLCopyOnWritePointer<AccumulatorBufferGroup>	mBuffers;
		bool											mInHandOff;

	};

	class LL_COMMON_API PeriodicRecording
	:	public LLStopWatchControlsMixin<PeriodicRecording>
	{
	public:
		PeriodicRecording(U32 num_periods, EPlayState state = STOPPED);

		void nextPeriod();
		size_t getNumRecordedPeriods() { return mNumPeriods; }

		F64Seconds getDuration() const;

		void appendPeriodicRecording(PeriodicRecording& other);
		void appendRecording(Recording& recording);
		Recording& getLastRecording();
		const Recording& getLastRecording() const;
		Recording& getCurRecording();
		const Recording& getCurRecording() const;
		Recording& getPrevRecording(U32 offset);
		const Recording& getPrevRecording(U32 offset) const;
		Recording snapshotCurRecording() const;

		template <typename T>
		size_t getSampleCount(const TraceType<T>& stat, size_t num_periods = U32_MAX)
        {
			size_t total_periods = mNumPeriods;
			num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

            size_t num_samples = 0;
			for (S32 i = 1; i <= num_periods; i++)
			{
				Recording& recording = getPrevRecording(i);
				num_samples += recording.getSampleCount(stat);
			}
			return num_samples;
        }
        
		//
		// PERIODIC MIN
		//

		// catch all for stats that have a defined sum
		template <typename T>
		typename T::value_t getPeriodMin(const TraceType<T>& stat, size_t num_periods = U32_MAX)
		{
			size_t total_periods = mNumPeriods;
			num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

			typename T::value_t min_val = std::numeric_limits<typename T::value_t>::max();
			for (S32 i = 1; i <= num_periods; i++)
			{
				Recording& recording = getPrevRecording(i);
				min_val = llmin(min_val, recording.getSum(stat));
			}
			return min_val;
		}

		template<typename T>
		T getPeriodMin(const CountStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return T(getPeriodMin(static_cast<const TraceType<CountAccumulator>&>(stat), num_periods));
		}

		F64 getPeriodMin(const TraceType<SampleAccumulator>& stat, size_t num_periods = U32_MAX);
		template<typename T>
		T getPeriodMin(const SampleStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return T(getPeriodMin(static_cast<const TraceType<SampleAccumulator>&>(stat), num_periods));
		}

		F64 getPeriodMin(const TraceType<EventAccumulator>& stat, size_t num_periods = U32_MAX);
		template<typename T>
		T getPeriodMin(const EventStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return T(getPeriodMin(static_cast<const TraceType<EventAccumulator>&>(stat), num_periods));
		}

		F64Bytes getPeriodMin(const TraceType<MemStatAccumulator>& stat, size_t num_periods = U32_MAX);
		F64Bytes getPeriodMin(const MemStatHandle& stat, size_t num_periods = U32_MAX);

		template <typename T>
		typename RelatedTypes<typename T::value_t>::fractional_t getPeriodMinPerSec(const TraceType<T>& stat, size_t num_periods = U32_MAX)
		{
			size_t total_periods = mNumPeriods;
			num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

			typename RelatedTypes<typename T::value_t>::fractional_t min_val = std::numeric_limits<F64>::max();
			for (S32 i = 1; i <= num_periods; i++)
			{
				Recording& recording = getPrevRecording(i);
				min_val = llmin(min_val, recording.getPerSec(stat));
			}
			return (typename RelatedTypes<typename T::value_t>::fractional_t) min_val;
		}

		template<typename T>
		typename RelatedTypes<T>::fractional_t getPeriodMinPerSec(const CountStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return typename RelatedTypes<T>::fractional_t(getPeriodMinPerSec(static_cast<const TraceType<CountAccumulator>&>(stat), num_periods));
		}

		//
		// PERIODIC MAX
		//

		// catch all for stats that have a defined sum
		template <typename T>
		typename T::value_t getPeriodMax(const TraceType<T>& stat, size_t num_periods = U32_MAX)
		{
			size_t total_periods = mNumPeriods;
			num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

			typename T::value_t max_val = std::numeric_limits<typename T::value_t>::min();
			for (S32 i = 1; i <= num_periods; i++)
			{
				Recording& recording = getPrevRecording(i);
				max_val = llmax(max_val, recording.getSum(stat));
			}
			return max_val;
		}

		template<typename T>
		T getPeriodMax(const CountStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return T(getPeriodMax(static_cast<const TraceType<CountAccumulator>&>(stat), num_periods));
		}

		F64 getPeriodMax(const TraceType<SampleAccumulator>& stat, size_t num_periods = U32_MAX);
		template<typename T>
		T getPeriodMax(const SampleStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return T(getPeriodMax(static_cast<const TraceType<SampleAccumulator>&>(stat), num_periods));
		}

		F64 getPeriodMax(const TraceType<EventAccumulator>& stat, size_t num_periods = U32_MAX);
		template<typename T>
		T getPeriodMax(const EventStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return T(getPeriodMax(static_cast<const TraceType<EventAccumulator>&>(stat), num_periods));
		}

		F64Bytes getPeriodMax(const TraceType<MemStatAccumulator>& stat, size_t num_periods = U32_MAX);
		F64Bytes getPeriodMax(const MemStatHandle& stat, size_t num_periods = U32_MAX);

		template <typename T>
		typename RelatedTypes<typename T::value_t>::fractional_t getPeriodMaxPerSec(const TraceType<T>& stat, size_t num_periods = U32_MAX)
		{
			size_t total_periods = mNumPeriods;
			num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

			F64 max_val = std::numeric_limits<F64>::min();
			for (S32 i = 1; i <= num_periods; i++)
			{
				Recording& recording = getPrevRecording(i);
				max_val = llmax(max_val, recording.getPerSec(stat));
			}
			return (typename RelatedTypes<typename T::value_t>::fractional_t)max_val;
		}

		template<typename T>
		typename RelatedTypes<T>::fractional_t getPeriodMaxPerSec(const CountStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return typename RelatedTypes<T>::fractional_t(getPeriodMaxPerSec(static_cast<const TraceType<CountAccumulator>&>(stat), num_periods));
		}

		//
		// PERIODIC MEAN
		//

		// catch all for stats that have a defined sum
		template <typename T>
		typename RelatedTypes<typename T::value_t>::fractional_t getPeriodMean(const TraceType<T >& stat, size_t num_periods = U32_MAX)
		{
			size_t total_periods = mNumPeriods;
			num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

			typename RelatedTypes<typename T::value_t>::fractional_t mean(0);

			for (S32 i = 1; i <= num_periods; i++)
			{
				Recording& recording = getPrevRecording(i);
				if (recording.getDuration() > (F32Seconds)0.f)
				{
					mean += recording.getSum(stat);
				}
			}
			return (num_periods
				? typename RelatedTypes<typename T::value_t>::fractional_t(mean / num_periods)
				: typename RelatedTypes<typename T::value_t>::fractional_t(NaN));
		}

		template<typename T>
		typename RelatedTypes<T>::fractional_t getPeriodMean(const CountStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return typename RelatedTypes<T>::fractional_t(getPeriodMean(static_cast<const TraceType<CountAccumulator>&>(stat), num_periods));
		}
		F64 getPeriodMean(const TraceType<SampleAccumulator>& stat, size_t num_periods = U32_MAX);
		template<typename T> 
		typename RelatedTypes<T>::fractional_t getPeriodMean(const SampleStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return typename RelatedTypes<T>::fractional_t(getPeriodMean(static_cast<const TraceType<SampleAccumulator>&>(stat), num_periods));
		}

		F64 getPeriodMean(const TraceType<EventAccumulator>& stat, size_t num_periods = U32_MAX);
		template<typename T>
		typename RelatedTypes<T>::fractional_t getPeriodMean(const EventStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return typename RelatedTypes<T>::fractional_t(getPeriodMean(static_cast<const TraceType<EventAccumulator>&>(stat), num_periods));
		}

		F64Bytes getPeriodMean(const TraceType<MemStatAccumulator>& stat, size_t num_periods = U32_MAX);
		F64Bytes getPeriodMean(const MemStatHandle& stat, size_t num_periods = U32_MAX);
		
		template <typename T>
		typename RelatedTypes<typename T::value_t>::fractional_t getPeriodMeanPerSec(const TraceType<T>& stat, size_t num_periods = U32_MAX)
		{
			size_t total_periods = mNumPeriods;
			num_periods = llmin(num_periods, isStarted() ? total_periods - 1 : total_periods);

			typename RelatedTypes<typename T::value_t>::fractional_t mean = 0;

			for (S32 i = 1; i <= num_periods; i++)
			{
				Recording& recording = getPrevRecording(i);
				if (recording.getDuration() > (F32Seconds)0.f)
				{
					mean += recording.getPerSec(stat);
				}
			}

			return (num_periods
				? typename RelatedTypes<typename T::value_t>::fractional_t(mean / num_periods)
				: typename RelatedTypes<typename T::value_t>::fractional_t(NaN));
		}

		template<typename T>
		typename RelatedTypes<T>::fractional_t getPeriodMeanPerSec(const CountStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return typename RelatedTypes<T>::fractional_t(getPeriodMeanPerSec(static_cast<const TraceType<CountAccumulator>&>(stat), num_periods));
		}

		//
		// PERIODIC STANDARD DEVIATION
		//

		F64 getPeriodStandardDeviation(const TraceType<SampleAccumulator>& stat, size_t num_periods = U32_MAX);

		template<typename T> 
		typename RelatedTypes<T>::fractional_t getPeriodStandardDeviation(const SampleStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return typename RelatedTypes<T>::fractional_t(getPeriodStandardDeviation(static_cast<const TraceType<SampleAccumulator>&>(stat), num_periods));
		}

		F64 getPeriodStandardDeviation(const TraceType<EventAccumulator>& stat, size_t num_periods = U32_MAX);
		template<typename T>
		typename RelatedTypes<T>::fractional_t getPeriodStandardDeviation(const EventStatHandle<T>& stat, size_t num_periods = U32_MAX)
		{
			return typename RelatedTypes<T>::fractional_t(getPeriodStandardDeviation(static_cast<const TraceType<EventAccumulator>&>(stat), num_periods));
		}

		F64Bytes getPeriodStandardDeviation(const TraceType<MemStatAccumulator>& stat, size_t num_periods = U32_MAX);
		F64Bytes getPeriodStandardDeviation(const MemStatHandle& stat, size_t num_periods = U32_MAX);

	private:
		// implementation for LLStopWatchControlsMixin
		/*virtual*/ void handleStart();
		/*virtual*/ void handleStop();
		/*virtual*/ void handleReset();
		/*virtual*/ void handleSplitTo(PeriodicRecording& other);

	private:
		std::vector<Recording>	mRecordingPeriods;
		const bool				mAutoResize;
		size_t					mCurPeriod;
		size_t					mNumPeriods;
	};

	PeriodicRecording& get_frame_recording();

	class ExtendableRecording
	:	public LLStopWatchControlsMixin<ExtendableRecording>
	{
	public:
		void extend();

		Recording& getAcceptedRecording() { return mAcceptedRecording; }
		const Recording& getAcceptedRecording() const {return mAcceptedRecording;}

		Recording& getPotentialRecording()				{ return mPotentialRecording; }
		const Recording& getPotentialRecording() const	{ return mPotentialRecording;}

	private:
		// implementation for LLStopWatchControlsMixin
		/*virtual*/ void handleStart();
		/*virtual*/ void handleStop();
		/*virtual*/ void handleReset();
		/*virtual*/ void handleSplitTo(ExtendableRecording& other);

	private:
		Recording mAcceptedRecording;
		Recording mPotentialRecording;
	};

	class ExtendablePeriodicRecording
	:	public LLStopWatchControlsMixin<ExtendablePeriodicRecording>
	{
	public:
		ExtendablePeriodicRecording();
		void extend();

		PeriodicRecording& getResults()				{ return mAcceptedRecording; }
		const PeriodicRecording& getResults() const	{return mAcceptedRecording;}
		
		void nextPeriod() { mPotentialRecording.nextPeriod(); }

	private:
		// implementation for LLStopWatchControlsMixin
		/*virtual*/ void handleStart();
		/*virtual*/ void handleStop();
		/*virtual*/ void handleReset();
		/*virtual*/ void handleSplitTo(ExtendablePeriodicRecording& other);

	private:
		PeriodicRecording mAcceptedRecording;
		PeriodicRecording mPotentialRecording;
	};
}

#endif // LL_LLTRACERECORDING_H
