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

#include "llmutex.h"
#include "llmemory.h"

#include <list>

#define TOKEN_PASTE_ACTUAL(x, y) x##y
#define TOKEN_PASTE(x, y) TOKEN_PASTE_ACTUAL(x, y)
#define RECORD_BLOCK_TIME(block_timer) LLTrace::BlockTimer::Recorder TOKEN_PASTE(block_time_recorder, __COUNTER__)(block_timer);

namespace LLTrace
{
	void init();
	void cleanup();

	extern class MasterThreadTrace *gMasterThreadTrace;
	extern LLThreadLocalPtr<class ThreadTraceData> gCurThreadTrace;

	// one per thread per type
	template<typename ACCUMULATOR>
	class AccumulatorBuffer
	{
		static const U32 DEFAULT_ACCUMULATOR_BUFFER_SIZE = 64;
	private:
		enum StaticAllocationMarker { STATIC_ALLOC };

		AccumulatorBuffer(StaticAllocationMarker m)
		:	mStorageSize(64),
			mNextStorageSlot(0),
			mStorage(new ACCUMULATOR[DEFAULT_ACCUMULATOR_BUFFER_SIZE])
		{
		}
	public:

		AccumulatorBuffer(const AccumulatorBuffer& other = getDefaultBuffer())
		:	mStorageSize(other.mStorageSize),
			mStorage(new ACCUMULATOR[other.mStorageSize]),
			mNextStorageSlot(other.mNextStorageSlot)
		{}

		LL_FORCE_INLINE ACCUMULATOR& operator[](size_t index) 
		{ 
			return mStorage[index]; 
		}

		void mergeFrom(const AccumulatorBuffer<ACCUMULATOR>& other)
		{
			llassert(mNextStorageSlot == other.mNextStorageSlot);

			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i].mergeFrom(other.mStorage[i]);
			}
		}

		void copyFrom(const AccumulatorBuffer<ACCUMULATOR>& other)
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
			sPrimaryStorage = mStorage;
		}

		LL_FORCE_INLINE static ACCUMULATOR* getPrimaryStorage() 
		{ 
			return sPrimaryStorage.get(); 
		}

		// NOTE: this is not thread-safe.  We assume that slots are reserved in the main thread before any child threads are spawned
		size_t reserveSlot()
		{
			size_t next_slot = mNextStorageSlot++;
			if (next_slot >= mStorageSize)
			{
				size_t new_size = mStorageSize + (mStorageSize >> 2);
				delete [] mStorage;
				mStorage = new ACCUMULATOR[new_size];
				mStorageSize = new_size;
			}
			llassert(next_slot < mStorageSize);
			return next_slot;
		}

		static AccumulatorBuffer<ACCUMULATOR>& getDefaultBuffer()
		{
			static AccumulatorBuffer sBuffer;
			return sBuffer;
		}

	private:
		ACCUMULATOR*							mStorage;
		size_t									mStorageSize;
		size_t									mNextStorageSlot;
		static LLThreadLocalPtr<ACCUMULATOR>	sPrimaryStorage;
	};
	template<typename ACCUMULATOR> LLThreadLocalPtr<ACCUMULATOR> AccumulatorBuffer<ACCUMULATOR>::sPrimaryStorage;

	template<typename ACCUMULATOR>
	class Trace
	{
	public:
		Trace(const std::string& name)
		:	mName(name)
		{
			mAccumulatorIndex = AccumulatorBuffer<ACCUMULATOR>::getDefaultBuffer().reserveSlot();
		}

		LL_FORCE_INLINE ACCUMULATOR& getAccumulator()
		{
			return AccumulatorBuffer<ACCUMULATOR>::getPrimaryStorage()[mAccumulatorIndex];
		}

	private:
		std::string	mName;
		size_t		mAccumulatorIndex;
	};


	template<typename T>
	class StatAccumulator
	{
	public:
		StatAccumulator()
		:	mSum(0),
			mMin(0),
			mMax(0),
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

		void mergeFrom(const StatAccumulator<T>& other)
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

	class Sampler
	{
	public:
		Sampler() {}
		Sampler(const Sampler& other)
		:	mF32Stats(other.mF32Stats),
			mS32Stats(other.mS32Stats),
			mTimers(other.mTimers)
		{}

		~Sampler() {}

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

		void stop();
		void resume();

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
		class ThreadTraceData* getThreadTrace(); 

		AccumulatorBuffer<StatAccumulator<F32> >	mF32Stats;
		AccumulatorBuffer<StatAccumulator<S32> >	mS32Stats;

		AccumulatorBuffer<TimerAccumulator>			mTimers;
	};

	class ThreadTraceData
	{
	public:
		ThreadTraceData()
		{
			mPrimarySampler.makePrimary();
		}

		ThreadTraceData(const ThreadTraceData& other)
		:	mPrimarySampler(other.mPrimarySampler)
		{
			mPrimarySampler.makePrimary();
		}

		void activate(Sampler* sampler)
		{
			flushPrimary();
			mActiveSamplers.push_back(sampler);
		}

		void deactivate(Sampler* sampler)
		{
			sampler->mergeFrom(mPrimarySampler);

			// TODO: replace with intrusive list
			std::list<Sampler*>::iterator found_it = std::find(mActiveSamplers.begin(), mActiveSamplers.end(), sampler);
			if (found_it != mActiveSamplers.end())
			{
				mActiveSamplers.erase(found_it);
			}
		}

		void flushPrimary()
		{
			for (std::list<Sampler*>::iterator it = mActiveSamplers.begin(), end_it = mActiveSamplers.end();
				it != end_it;
				++it)
			{
				(*it)->mergeFrom(mPrimarySampler);
			}
			mPrimarySampler.reset();
		}

		Sampler* getPrimarySampler() { return &mPrimarySampler; }
	protected:
		Sampler					mPrimarySampler;
		std::list<Sampler*>		mActiveSamplers;
	};

	class MasterThreadTrace : public ThreadTraceData
	{
	public:
		MasterThreadTrace()
		{}

		void addSlaveThread(class SlaveThreadTrace* child);
		void removeSlaveThread(class SlaveThreadTrace* child);

		// call this periodically to gather stats data from worker threads
		void pullFromWorkerThreads();

	private:
		struct WorkerThreadTraceProxy
		{
			WorkerThreadTraceProxy(class SlaveThreadTrace* trace)
				:	mWorkerTrace(trace)
			{}
			class SlaveThreadTrace*	mWorkerTrace;
			Sampler				mSamplerStorage;
		};
		typedef std::list<WorkerThreadTraceProxy> worker_thread_trace_list_t;

		worker_thread_trace_list_t		mSlaveThreadTraces;
		LLMutex							mSlaveListMutex;
	};

	class SlaveThreadTrace : public ThreadTraceData
	{
	public:
		explicit 
		SlaveThreadTrace(MasterThreadTrace& master_trace = *gMasterThreadTrace)
		:	mMaster(master_trace),
			ThreadTraceData(master_trace),
			mSharedData(mPrimarySampler)
		{
			master_trace.addSlaveThread(this);
		}

		~SlaveThreadTrace()
		{
			mMaster.removeSlaveThread(this);
		}

		// call this periodically to gather stats data for master thread to consume
		void pushToParent()
		{
			mSharedData.copyFrom(mPrimarySampler);
		}

		MasterThreadTrace& 	mMaster;

		// this data is accessed by other threads, so give it a 64 byte alignment
		// to avoid false sharing on most x86 processors
		LL_ALIGNED(64) class SharedData
		{
		public:
			explicit 
			SharedData(const Sampler& other_sampler) 
			:	mSampler(other_sampler)
			{}

			void copyFrom(Sampler& source)
			{
				LLMutexLock lock(&mSamplerMutex);
				{	
					mSampler.mergeFrom(source);
				}
			}

			void copyTo(Sampler& sink)
			{
				LLMutexLock lock(&mSamplerMutex);
				{
					sink.mergeFrom(mSampler);
				}
			}
		private:
			// add a cache line's worth of unused space to avoid any potential of false sharing
			LLMutex					mSamplerMutex;
			Sampler					mSampler;
		};
		SharedData					mSharedData;
	};


	class TimeInterval 
	{
	public:
		void start() {}
		void stop() {}
		void resume() {}
	};
}

#endif // LL_LLTRACE_H
