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

namespace LLTrace
{
	//template<typename T, typename IS_UNIT> class Rate;
	//template<typename T, typename IS_UNIT> class Measurement;
	//template<typename T> class Count;
	//template<typename T> class AccumulatorBuffer;
	//template<typename T> class RateAccumulator;
	//template<typename T> class MeasurementAccumulator;
	//class TimerAccumulator;

	class LL_COMMON_API Recording
	{
	public:
		Recording();

		~Recording();

		void makePrimary();
		bool isPrimary() const;

		void start();
		void stop();
		void resume();
		void reset();
		void update();
		bool isStarted() const { return mIsStarted; }

		void addSamples(const Recording& other);
		void addDeltas(const Recording& baseline, const Recording& target);

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
		friend class ThreadRecorder;
		// returns data for current thread
		class ThreadRecorder* getThreadRecorder(); 

		LLCopyOnWritePointer<AccumulatorBuffer<RateAccumulator<F32> > >			mRates;
		LLCopyOnWritePointer<AccumulatorBuffer<MeasurementAccumulator<F32> > >	mMeasurements;
		LLCopyOnWritePointer<AccumulatorBuffer<TimerAccumulator> >				mStackTimers;

		bool			mIsStarted;
		LLTimer			mSamplingTimer;
		F64				mElapsedSeconds;
	};

	class LL_COMMON_API PeriodicRecording
	{

	};
}

#endif // LL_LLTRACERECORDING_H
