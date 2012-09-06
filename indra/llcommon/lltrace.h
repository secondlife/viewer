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

namespace LLTrace
{
	class Stat
	{
	public:
		Stat(const char* name)
		:	mName(name)
		{}
		void record() {}
		void record(int value) {}
		void record(float value) {}
	private:
		const std::string mName;
	};

	class BlockTimer
	{
	public:
		BlockTimer(const char* name)
		:	mName(name)
		{}

		struct Accumulator
		{
			U32 						mSelfTimeCounter;
			U32 						mTotalTimeCounter;
			U32 						mCalls;
			Accumulator*				mParent;		// info for caller timer
			Accumulator*				mLastCaller;	// used to bootstrap tree construction
			const BlockTimer*			mTimer;			// points to block timer associated with this storage
			U16							mActiveCount;	// number of timers with this ID active on stack
			bool						mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame
			std::vector<Accumulator*>	mChildren;		// currently assumed child timers
		};

		struct RecorderHead
		{
			struct Recorder*	mRecorder;
			Accumulator*		mAccumulator;
			U32					mChildTime;
		};

		struct Recorder
		{
			LL_FORCE_INLINE Recorder(BlockTimer& block_timer)
			:	mLastHead(sRecorderHead)
			{
				mStartTime = getCPUClockCount32();
				Accumulator* accumulator = ???; // get per-thread accumulator
				accumulator->mActiveCount++;
				accumulator->mCalls++;
				accumulator->mMoveUpTree |= (accumulator->mParent->mActiveCount == 0);

				// push new timer on stack
				sRecorderHead.mRecorder = this;
				sRecorderHead.mAccumulator = accumulator;
				sRecorderHead.mChildTime = 0;
			}

			LL_FORCE_INLINE ~Recorder()
			{
				U32 total_time = getCPUClockCount32() - mStartTime;

				Accumulator* accumulator = sRecorderHead.mAccumulator;
				accumulator->mSelfTimeCounter += total_time- sRecorderHead.mChildTime;
				accumulator->mTotalTimeCounter += total_time;
				accumulator->mActiveCount--;

				accumulator->mLastCaller = mLastHead->mAccumulator;
				mLastHead->mChildTime += total_time;

				// pop stack
				sRecorderHead = mLastHead;
			}

			AccumulatorHead mLastHead;
			U32 mStartTime;
		};

	private:
		U32 getCPUClockCount32()
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

		const std::string mName;
		static RecorderHead* sRecorderHead;
	};

	BlockTimer::RecorderHead BlockTimer::sRecorderHead;

	class TimeInterval 
	{
	public:
		void start() {}
		void stop() {}
		void resume() {}
	};

	class SamplingTimeInterval
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
