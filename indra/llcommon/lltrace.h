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

#include "llthread.h"

#include <list>

namespace LLTrace
{

	// one per thread per type
	template<typename ACCUMULATOR>
	struct AccumulatorBuffer : public AccumulatorBufferBase
	{
		ACCUMULATOR*				mStorage;
		size_t						mStorageSize;
		size_t						mNextStorageSlot;
		static S32					sStorageKey; // key used to access thread local storage pointer to accumulator values
		
		AccumulatorBuffer()
		:	mStorageSize(64),
			mStorage(new ACCUMULATOR[64]),
			mNextStorageSlot(0)
		{}

		AccumulatorBuffer(const AccumulatorBuffer& other)
		:	mStorageSize(other.mStorageSize),
			mStorage(new ACCUMULATOR[other.mStorageSize]),
			mNextStorageSlot(other.mNextStorageSlot)
		{

		}

		LL_FORCE_INLINE ACCUMULATOR& operator[](size_t index) { return (*mStorage)[index]; }

		void mergeFrom(const AccumulatorBuffer<ACCUMULATOR>& other)
		{
			llassert(mNextStorageSlot == other.mNextStorageSlot);

			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i].mergeFrom(other.mStorage[i]);
			}
		}

		void copyFrom(const AccumulatorBuffer<Accumulator>& other)
		{
			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i] = other.mStorage[i];
			}
		}

		void reset()
		{
			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i].reset();
			}
		}

		void makePrimary()
		{
			//TODO: use sStorageKey to set mStorage as active buffer
		}

		// NOTE: this is not thread-safe.  We assume that slots are reserved in the main thread before any child threads are spawned
		size_t reserveSlot()
		{
			size_t next_slot = mNextStorageSlot++;
			if (next_slot >= mStorageSize)
			{
				size_t new_size = mStorageSize + (mStorageSize >> 2);
				delete [] mStorage;
				mStorage = new mStorage(new_size);
				mStorageSize = new_size;
			}
			llassert(next_slot < mStorageSize);
			return next_slot;
		}
	};
	template<typename ACCUMULATOR> S32 AccumulatorBuffer<ACCUMULATOR>::sStorageKey;

	template<typename ACCUMULATOR>
	class Trace
	{
	public:
		Trace(const std::string& name)
		:	mName(name)
		{
			mAccumulatorIndex = sAccumulatorBuffer.reserveSlot();
		}

		LL_FORCE_INLINE ACCUMULATOR& getAccumulator()
		{
			return sAccumulatorBuffer[mAccumulatorIndex];
		}

	private:
		std::string	mName;
		size_t		mAccumulatorIndex;

		// this needs to be thread local
		static AccumulatorBuffer<ACCUMULATOR>	sAccumulatorBuffer;
	};

	template<typename ACCUMULATOR> std::vector<ACCUMULATOR> Trace<ACCUMULATOR>::sAccumulatorBuffer;

	template<typename T>
	class StatAccumulator
	{
	public:
		StatAccumulator()
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

		void mergeFrom(const Stat<T>& other)
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

		void reset()
		{
			mNumSamples = 0;
			mSum = 0;
			mMin = 0;
			mMax = 0;
		}

	private:
		T	mSum,
			mMin,
			mMax;

		U32	mNumSamples;
	};

	template <typename T>
	class Stat : public Trace<StatAccumulator<T> >
	{
	public:
		Stat(const std::string& name) 
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
		const class BlockTimer*			mTimer;			// points to block timer associated with this storage
		U8								mActiveCount;	// number of timers with this ID active on stack
		bool							mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame
		std::vector<TimerAccumulator*>	mChildren;		// currently assumed child timers

		void mergeFrom(const TimerAccumulator& other)
		{
			mTotalTimeCounter += other.mTotalTimeCounter;
			mChildTimeCounter += other.mChildTimeCounter;
			mCalls += other.mCalls;
		}

		void reset()
		{
			mTotalTimeCounter = 0;
			mChildTimeCounter = 0;
			mCalls = 0;
		}

	};

	class BlockTimer : public Trace<TimerAccumulator>
	{
	public:
		BlockTimer(const char* name)
		:	Trace(name)
		{}

		struct Recorder
		{
			struct StackEntry
			{
				Recorder*			mRecorder;
				TimerAccumulator*	mAccumulator;
				U32					mChildTime;
			};

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

			StackEntry mLastRecorder;
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

		static Recorder::StackEntry sCurRecorder;
	};

	BlockTimer::Recorder::StackEntry BlockTimer::sCurRecorder;

	class Sampler
	{
	public:
		Sampler(const Sampler& other)
		:	mF32Stats(other.mF32Stats),
			mS32Stats(other.mS32Stats),
			mTimers(other.mTimers)
		{}

		~Sampler()
		{
			stop();
		}

		void makePrimary()
		{
			mF32Stats.makePrimary();
			mS32Stats.makePrimary();
			mTimers.makePrimary();
		}

		void start()
		{
			reset();
			resume();
		}

		void stop()
		{
			getThreadTracer()->deactivate(this);
		}

		void resume()
		{
			ThreadTracer* thread_data = getThreadTracer();
			thread_data->flushData();
			thread_data->activate(this);
		}

		void mergeFrom(const Sampler& other)
		{
			mF32Stats.mergeFrom(other.mF32Stats);
			mS32Stats.mergeFrom(other.mS32Stats);
			mTimers.mergeFrom(other.mTimers);
		}

		void reset()
		{
			mF32Stats.reset();
			mS32Stats.reset();
			mTimers.reset();
		}

	private:
		// returns data for current thread
		struct ThreadTracer* getThreadTracer() { return NULL; } 

		AccumulatorBuffer<StatAccumulator<F32> >	mF32Stats;
		AccumulatorBuffer<StatAccumulator<S32> >	mS32Stats;

		AccumulatorBuffer<TimerAccumulator>			mTimers;
	};

	struct ThreadTracer
	{
		ThreadTracer(LLThread& this_thread, ThreadTracer& parent_data)
		:	mPrimarySampler(parent_data.mPrimarySampler),
			mSharedSampler(parent_data.mSharedSampler),
			mSharedSamplerMutex(this_thread.getAPRPool()),
			mParent(parent_data)
		{
			mPrimarySampler.makePrimary();
			parent_data.addChildThread(this);
		}

		~ThreadTracer()
		{
			mParent.removeChildThread(this);
		}

		void addChildThread(ThreadTracer* child)
		{
			mChildThreadTracers.push_back(child);
		}

		void removeChildThread(ThreadTracer* child)
		{
			// TODO: replace with intrusive list
			std::list<ThreadTracer*>::iterator found_it = std::find(mChildThreadTracers.begin(), mChildThreadTracers.end(), child);
			if (found_it != mChildThreadTracers.end())
			{
				mChildThreadTracers.erase(found_it);
			}
		}

		void flushData()
		{
			for (std::list<Sampler*>::iterator it = mActiveSamplers.begin(), end_it = mActiveSamplers.end();
				it != end_it;
				++it)
			{
				(*it)->mergeFrom(mPrimarySampler);
			}
			mPrimarySampler.reset();
		}

		void activate(Sampler* sampler)
		{
			mActiveSamplers.push_back(sampler);
		}

		void deactivate(Sampler* sampler)
		{
			// TODO: replace with intrusive list
			std::list<Sampler*>::iterator found_it = std::find(mActiveSamplers.begin(), mActiveSamplers.end(), sampler);
			if (found_it != mActiveSamplers.end())
			{
				mActiveSamplers.erase(found_it);
			}
		}
		
		// call this periodically to gather stats data in parent thread
		void publishToParent()
		{
			mSharedSamplerMutex.lock();
			{	
				mSharedSampler.mergeFrom(mPrimarySampler);
			}
			mSharedSamplerMutex.unlock();
		}

		// call this periodically to gather stats data from children
		void gatherChildData()
		{
			for (std::list<ThreadTracer*>::iterator child_it = mChildThreadTracers.begin(), end_it = mChildThreadTracers.end();
				child_it != end_it;
				++child_it)
			{
				(*child_it)->mSharedSamplerMutex.lock();
				{
					//TODO for now, just aggregate, later keep track of thread data independently
					mPrimarySampler.mergeFrom((*child_it)->mSharedSampler);
				}
				(*child_it)->mSharedSamplerMutex.unlock();
			}
		}

		Sampler	mPrimarySampler;

		ThreadTracer&				mParent;
		std::list<Sampler*>			mActiveSamplers;
		std::list<ThreadTracer*>	mChildThreadTracers;

		// TODO:  add unused space here to avoid false sharing?
		LLMutex	mSharedSamplerMutex;
		Sampler mSharedSampler;
	};


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
