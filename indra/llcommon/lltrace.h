/** 
 * @file lltrace.h
 * @brief Runtime statistics accumulation.
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

#ifndef LL_LLTRACE_H
#define LL_LLTRACE_H

#include "stdtypes.h"
#include "llpreprocessor.h"

#include <vector>
#include <boost/type_traits/alignment_of.hpp>

namespace LLTrace
{
	//TODO figure out best way to do this and proper naming convention
	
	static void init()
	{

	}

	template<typename ACCUMULATOR>
	class Trace
	{
	public:
		Trace(const std::string& name)
		:	mName(name)
		{
			mStorageIndex = sNextIndex++;
			sStorage.reserve(sNextIndex);
		}

		LL_FORCE_INLINE ACCUMULATOR& getAccumulator()
		{
			return sStorage[mStorageIndex];
		}

		void mergeFrom(const Trace<ACCUMULATOR>& other)
		{
			getAccumulator().mergeFrom(other.getAccumulator());
		}


	private:
		std::string				mName;
		ptrdiff_t				mStorageIndex;

		// this needs to be thread local
		static std::vector<ACCUMULATOR>	sStorage;
		static size_t					sNextIndex;
	};

	template<typename ACCUMULATOR> std::vector<ACCUMULATOR> Trace<ACCUMULATOR>::sStorage;
	template<typename ACCUMULATOR> size_t Trace<ACCUMULATOR>::sNextIndex = 0;

	template<typename T>
	class Accumulator
	{
	public:
		Accumulator()
			:	mSum(),
			mMin(),
			mMax(),
			mNumSamples(0)
		{}

		LL_FORCE_INLINE void sample(T value)
		{
			mNumSamples++;
			mSum += value;
			if (value < mMin)
			{
				mMin = value;
			}
			else if (value > mMax)
			{
				mMax = value;
			}
		}

		void mergeFrom(const Accumulator<T>& other)
		{
			mSum += other.mSum;
			if (other.mMin < mMin)
			{
				mMin = other.mMin;
			}
			if (other.mMax > mMax)
			{
				mMax = other.mMax;
			}
			mNumSamples += other.mNumSamples;
		}

	private:
		T	mSum,
			mMin,
			mMax;

		U32	mNumSamples;
	};


	template<typename T>
	class Stat : public Trace<Accumulator<T> >
	{
	public:
		Stat(const char* name)
		:	Trace(name)
		{}

		void sample(T value)
		{
			getAccumulator().sample(value);
		}

	};

	struct TimerAccumulator
	{
		U32 							mTotalTimeCounter,
										mChildTimeCounter,
										mCalls;
		TimerAccumulator*				mParent;		// info for caller timer
		TimerAccumulator*				mLastCaller;	// used to bootstrap tree construction
		const BlockTimer*				mTimer;			// points to block timer associated with this storage
		U8								mActiveCount;	// number of timers with this ID active on stack
		bool							mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame
		std::vector<TimerAccumulator*>	mChildren;		// currently assumed child timers

		void mergeFrom(const TimerAccumulator& other)
		{
			mTotalTimeCounter += other.mTotalTimeCounter;
			mChildTimeCounter += other.mChildTimeCounter;
			mCalls += other.mCalls;
		}
	};

	class BlockTimer : public Trace<TimerAccumulator>
	{
	public:
		BlockTimer(const char* name)
		:	Trace(name)
		{}

		struct Recorder;

		struct RecorderStackEntry
		{
			Recorder*	mRecorder;
			TimerAccumulator*	mAccumulator;
			U32					mChildTime;
		};

		struct Recorder
		{
			LL_FORCE_INLINE Recorder(BlockTimer& block_timer)
			:	mLastRecorder(sCurRecorder)
			{
				mStartTime = getCPUClockCount32();
				TimerAccumulator* accumulator = &block_timer.getAccumulator(); // get per-thread accumulator
				accumulator->mActiveCount++;
				accumulator->mCalls++;
				accumulator->mMoveUpTree |= (accumulator->mParent->mActiveCount == 0);

				// push new timer on stack
				sCurRecorder.mRecorder = this;
				sCurRecorder.mAccumulator = accumulator;
				sCurRecorder.mChildTime = 0;
			}

			LL_FORCE_INLINE ~Recorder()
			{
				U32 total_time = getCPUClockCount32() - mStartTime;

				TimerAccumulator* accumulator = sCurRecorder.mAccumulator;
				accumulator->mTotalTimeCounter += total_time;
				accumulator->mChildTimeCounter += sCurRecorder.mChildTime;
				accumulator->mActiveCount--;

				accumulator->mLastCaller = mLastRecorder.mAccumulator;
				mLastRecorder.mChildTime += total_time;

				// pop stack
				sCurRecorder = mLastRecorder;
			}

			RecorderStackEntry mLastRecorder;
			U32 mStartTime;
		};

	private:
		static U32 getCPUClockCount32()
		{
			U32 ret_val;
			__asm
			{
				_emit   0x0f
				_emit   0x31
				shr eax,8
				shl edx,24
				or eax, edx
				mov dword ptr [ret_val], eax
			}
			return ret_val;
		}

		// return full timer value, *not* shifted by 8 bits
		static U64 getCPUClockCount64()
		{
			U64 ret_val;
			__asm
			{
				_emit   0x0f
				_emit   0x31
				mov eax,eax
				mov edx,edx
				mov dword ptr [ret_val+4], edx
				mov dword ptr [ret_val], eax
			}
			return ret_val;
		}

		static RecorderStackEntry sCurRecorder;
	};

	BlockTimer::RecorderStackEntry BlockTimer::sCurRecorder;

	class TimeInterval 
	{
	public:
		void start() {}
		void stop() {}
		void resume() {}
	};

	class Sampler
	{
	public:
		void start() {}
		void stop() {}
		void resume() {}
	};

}

#define TOKEN_PASTE_ACTUAL(x, y) x##y
#define TOKEN_PASTE(x, y) TOKEN_PASTE_ACTUAL(x, y)
#define RECORD_BLOCK_TIME(block_timer) LLTrace::BlockTimer::Recorder TOKEN_PASTE(block_time_recorder, __COUNTER__)(block_timer);

#endif // LL_LLTRACE_H
