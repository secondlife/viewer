/** 
 * @file lltracesampler.h
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

#ifndef LL_LLTRACESAMPLER_H
#define LL_LLTRACESAMPLER_H

#include "stdtypes.h"
#include "llpreprocessor.h"

#include "llpointer.h"
#include "lltimer.h"

namespace LLTrace
{
	template<typename T> class Rate;
	template<typename T> class Measurement;
	template<typename T> class AccumulatorBuffer;
	template<typename T> class RateAccumulator;
	template<typename T> class MeasurementAccumulator;
	class TimerAccumulator;

	class LL_COMMON_API Sampler
	{
	public:
		Sampler();

		~Sampler();

		void makePrimary();
		bool isPrimary();

		void start();
		void stop();
		void resume();

		void mergeSamples(const Sampler& other);
		void initDeltas(const Sampler& other);
		void mergeDeltas(const Sampler& other);

		void reset();

		bool isStarted() { return mIsStarted; }

		F32 getSum(Rate<F32>& stat);
		F32 getPerSec(Rate<F32>& stat);

		F32 getSum(Measurement<F32>& stat);
		F32 getMin(Measurement<F32>& stat);
		F32 getMax(Measurement<F32>& stat);
		F32 getMean(Measurement<F32>& stat);
		F32 getStandardDeviation(Measurement<F32>& stat);

		F64 getSampleTime() { return mElapsedSeconds; }

	private:
		friend class ThreadTrace;
		// returns data for current thread
		class ThreadTrace* getThreadTrace(); 

		LLCopyOnWritePointer<AccumulatorBuffer<RateAccumulator<F32> > >			mRatesStart;
		LLCopyOnWritePointer<AccumulatorBuffer<RateAccumulator<F32> > >			mRates;
		LLCopyOnWritePointer<AccumulatorBuffer<MeasurementAccumulator<F32> > >	mMeasurements;
		LLCopyOnWritePointer<AccumulatorBuffer<TimerAccumulator> >				mStackTimersStart;
		LLCopyOnWritePointer<AccumulatorBuffer<TimerAccumulator> >				mStackTimers;

		bool			mIsStarted;
		LLTimer			mSamplingTimer;
		F64				mElapsedSeconds;
	};

	class LL_COMMON_API PeriodicSampler
	{

	};
}

#endif // LL_LLTRACESAMPLER_H
