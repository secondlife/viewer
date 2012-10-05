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

namespace LLTrace
{
	template<typename T, typename IS_UNIT> class Rate;
	template<typename T, typename IS_UNIT> class Measurement;
	template<typename T> class Count;
	template<typename T> class AccumulatorBuffer;
	template<typename T> class RateAccumulator;
	template<typename T> class MeasurementAccumulator;
	class TimerAccumulator;

	class LL_COMMON_API Recording
	{
	public:
		Recording();

		~Recording();

		void makePrimary();
		bool isPrimary();

		void start();
		void stop();
		void resume();

		void mergeSamples(const Recording& other);
		void mergeDeltas(const Recording& baseline, const Recording& target);

		void reset();
		void update();

		bool isStarted() { return mIsStarted; }

		// Rate accessors
		F32 getSum(const Rate<F32, void>& stat);
		F32 getPerSec(const Rate<F32, void>& stat);

		// Measurement accessors
		F32 getSum(const Measurement<F32, void>& stat);
		F32 getPerSec(const Measurement<F32, void>& stat);
		F32 getMin(const Measurement<F32, void>& stat);
		F32 getMax(const Measurement<F32, void>& stat);
		F32 getMean(const Measurement<F32, void>& stat);
		F32 getStandardDeviation(const Measurement<F32, void>& stat);

		// Count accessors
		F32 getSum(const Count<F32>& stat);
		F32 getPerSec(const Count<F32>& stat);
		F32 getIncrease(const Count<F32>& stat);
		F32 getIncreasePerSec(const Count<F32>& stat);
		F32 getDecrease(const Count<F32>& stat);
		F32 getDecreasePerSec(const Count<F32>& stat);
		F32 getChurn(const Count<F32>& stat);
		F32 getChurnPerSec(const Count<F32>& stat);

		F64 getSampleTime() { return mElapsedSeconds; }

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
