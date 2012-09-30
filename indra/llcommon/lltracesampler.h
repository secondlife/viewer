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

#include "lltrace.h"

namespace LLTrace
{
	class LL_COMMON_API Sampler
	{
	public:
		~Sampler();

		void makePrimary();

		void start();
		void stop();
		void resume();

		void mergeFrom(const Sampler* other);

		void reset();

		bool isStarted() { return mIsStarted; }

		F32 getSum(Stat<F32>& stat) { return stat.getAccumulator(mF32Stats).getSum(); }
		F32 getMin(Stat<F32>& stat) { return stat.getAccumulator(mF32Stats).getMin(); }
		F32 getMax(Stat<F32>& stat) { return stat.getAccumulator(mF32Stats).getMax(); }
		F32 getMean(Stat<F32>& stat) { return stat.getAccumulator(mF32Stats).getMean(); }

		S32 getSum(Stat<S32>& stat) { return stat.getAccumulator(mS32Stats).getSum(); }
		S32 getMin(Stat<S32>& stat) { return stat.getAccumulator(mS32Stats).getMin(); }
		S32 getMax(Stat<S32>& stat) { return stat.getAccumulator(mS32Stats).getMax(); }
		S32 getMean(Stat<S32>& stat) { return stat.getAccumulator(mS32Stats).getMean(); }

		F64 getSampleTime() { return mElapsedSeconds; }

	private:
		friend class ThreadTrace;
		Sampler(class ThreadTrace* thread_trace);

		// no copy
		Sampler(const Sampler& other) {}
		// returns data for current thread
		class ThreadTrace* getThreadTrace(); 

		//TODO: take snapshot at sampler start so we can simplify updates
		//AccumulatorBuffer<StatAccumulator<F32> >	mF32StatsStart;
		//AccumulatorBuffer<StatAccumulator<S32> >	mS32StatsStart;
		//AccumulatorBuffer<TimerAccumulator>			mStackTimersStart;

		AccumulatorBuffer<StatAccumulator<F32> >	mF32Stats;
		AccumulatorBuffer<StatAccumulator<S32> >	mS32Stats;
		AccumulatorBuffer<TimerAccumulator>			mStackTimers;

		bool										mIsStarted;
		LLTimer										mSamplingTimer;
		F64											mElapsedSeconds;
		ThreadTrace*								mThreadTrace;
	};
}

#endif // LL_LLTRACESAMPLER_H
