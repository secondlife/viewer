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

#include "llmemory.h"
#include "llrefcount.h"
#include "lltracerecording.h"
#include "lltracethreadrecorder.h"
#include "llunit.h"

#include <list>

#define TOKEN_PASTE_ACTUAL(x, y) x##y
#define TOKEN_PASTE(x, y) TOKEN_PASTE_ACTUAL(x, y)
#define RECORD_BLOCK_TIME(block_timer) LLTrace::BlockTimer::Recorder TOKEN_PASTE(block_time_recorder, __COUNTER__)(block_timer);

namespace LLTrace
{
	void init();
	void cleanup();

	LLThreadLocalPointer<class ThreadRecorder>& get_thread_recorder();

	class LL_COMMON_API MasterThreadRecorder& getMasterThreadRecorder();

	// one per thread per type
	template<typename ACCUMULATOR>
	class LL_COMMON_API AccumulatorBuffer : public LLRefCount
	{
		static const U32 DEFAULT_ACCUMULATOR_BUFFER_SIZE = 64;
	private:
		enum StaticAllocationMarker { STATIC_ALLOC };

		AccumulatorBuffer(StaticAllocationMarker m)
		:	mStorageSize(64),
			mNextStorageSlot(0),
			mStorage(new ACCUMULATOR[DEFAULT_ACCUMULATOR_BUFFER_SIZE])
		{}

	public:

		// copying an accumulator buffer does not copy the actual contents, but simply initializes the buffer size
		// to be identical to the other buffer
		AccumulatorBuffer(const AccumulatorBuffer& other = getDefaultBuffer())
		:	mStorageSize(other.mStorageSize),
			mStorage(new ACCUMULATOR[other.mStorageSize]),
			mNextStorageSlot(other.mNextStorageSlot)
		{}

		~AccumulatorBuffer()
		{
			if (sPrimaryStorage == mStorage)
			{
				//TODO pick another primary?
				sPrimaryStorage = NULL;
			}
		}

		LL_FORCE_INLINE ACCUMULATOR& operator[](size_t index) 
		{ 
			return mStorage[index]; 
		}

		void mergeSamples(const AccumulatorBuffer<ACCUMULATOR>& other)
		{
			llassert(mNextStorageSlot == other.mNextStorageSlot);

			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i].mergeSamples(other.mStorage[i]);
			}
		}

		void mergeDeltas(const AccumulatorBuffer<ACCUMULATOR>& start, const AccumulatorBuffer<ACCUMULATOR>& finish)
		{
			llassert(mNextStorageSlot == start.mNextStorageSlot && mNextStorageSlot == finish.mNextStorageSlot);

			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i].mergeDeltas(start.mStorage[i], finish.mStorage[i]);
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

		bool isPrimary() const
		{
			return sPrimaryStorage == mStorage;
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
			static AccumulatorBuffer sBuffer(STATIC_ALLOC);
			return sBuffer;
		}

	private:
		ACCUMULATOR*							mStorage;
		size_t									mStorageSize;
		size_t									mNextStorageSlot;
		static LLThreadLocalPointer<ACCUMULATOR>	sPrimaryStorage;
	};
	template<typename ACCUMULATOR> LLThreadLocalPointer<ACCUMULATOR> AccumulatorBuffer<ACCUMULATOR>::sPrimaryStorage;

	template<typename ACCUMULATOR>
	class LL_COMMON_API TraceType 
	{
	public:
		TraceType(const std::string& name)
		:	mName(name)
		{
			mAccumulatorIndex = AccumulatorBuffer<ACCUMULATOR>::getDefaultBuffer().reserveSlot();
		}

		LL_FORCE_INLINE ACCUMULATOR& getPrimaryAccumulator()
		{
			return AccumulatorBuffer<ACCUMULATOR>::getPrimaryStorage()[mAccumulatorIndex];
		}

		ACCUMULATOR& getAccumulator(AccumulatorBuffer<ACCUMULATOR>* buffer) { return (*buffer)[mAccumulatorIndex]; }
		const ACCUMULATOR& getAccumulator(AccumulatorBuffer<ACCUMULATOR>* buffer) const { return (*buffer)[mAccumulatorIndex]; }

	protected:
		std::string	mName;
		size_t		mAccumulatorIndex;
	};


	template<typename T>
	class LL_COMMON_API MeasurementAccumulator
	{
	public:
		MeasurementAccumulator()
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
			F32 old_mean = mMean;
			mMean += ((F32)value - old_mean) / (F32)mNumSamples;
			mStandardDeviation += ((F32)value - old_mean) * ((F32)value - mMean);
		}

		void mergeSamples(const MeasurementAccumulator<T>& other)
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
			F32 weight = (F32)mNumSamples / (F32)(mNumSamples + other.mNumSamples);
			mMean = mMean * weight + other.mMean * (1.f - weight);

			F32 n_1 = (F32)mNumSamples,
				n_2 = (F32)other.mNumSamples;
			F32 m_1 = mMean,
				m_2 = other.mMean;
			F32 sd_1 = mStandardDeviation,
				sd_2 = other.mStandardDeviation;
			// combine variance (and hence standard deviation) of 2 different sized sample groups using
			// the following formula: http://www.mrc-bsu.cam.ac.uk/cochrane/handbook/chapter_7/7_7_3_8_combining_groups.htm
			F32 variance = ((((n_1 - 1.f) * sd_1 * sd_1)
								+ ((n_2 - 1.f) * sd_2 * sd_2)
								+ (((n_1 * n_2) / (n_1 + n_2))
									* ((m_1 * m_1) + (m_2 * m_2) - (2.f * m_1 * m_2))))
							/ (n_1 + n_2 - 1.f));
			mStandardDeviation = sqrtf(variance);
		}

		void mergeDeltas(const MeasurementAccumulator<T>& start, const MeasurementAccumulator<T>& finish)
		{
			llerrs << "Delta merge invalid for measurement accumulators" << llendl;
		}

		void reset()
		{
			mNumSamples = 0;
			mSum = 0;
			mMin = 0;
			mMax = 0;
		}

		T	getSum() const { return mSum; }
		T	getMin() const { return mMin; }
		T	getMax() const { return mMax; }
		F32	getMean() const { return mMean; }
		F32 getStandardDeviation() const { return mStandardDeviation; }

	private:
		T	mSum,
			mMin,
			mMax;

		F32 mMean,
			mStandardDeviation;

		U32	mNumSamples;
	};

	template<typename T>
	class LL_COMMON_API RateAccumulator
	{
	public:
		RateAccumulator()
		:	mSum(0),
			mNumSamples(0)
		{}

		LL_FORCE_INLINE void add(T value)
		{
			mNumSamples++;
			mSum += value;
		}

		void mergeSamples(const RateAccumulator<T>& other)
		{
			mSum += other.mSum;
			mNumSamples += other.mNumSamples;
		}

		void mergeDeltas(const RateAccumulator<T>& start, const RateAccumulator<T>& finish)
		{
			mSum += finish.mSum - start.mSum;
			mNumSamples += finish.mNumSamples - start.mNumSamples;
		}

		void reset()
		{
			mNumSamples = 0;
			mSum = 0;
		}

		T	getSum() const { return mSum; }

	private:
		T	mSum;

		U32	mNumSamples;
	};

	template <typename T, typename IS_UNIT = void>
	class LL_COMMON_API Measurement
	:	public TraceType<MeasurementAccumulator<T> >, 
		public LLInstanceTracker<Measurement<T, IS_UNIT>, std::string>
	{
	public:
		Measurement(const std::string& name) 
		:	TraceType(name),
			LLInstanceTracker(name)
		{}

		void sample(T value)
		{
			getPrimaryAccumulator().sample(value);
		}
	};

	template <typename T>
	class LL_COMMON_API Measurement <T, typename T::is_unit_t>
	:	public Measurement<typename T::value_t>
	{
	public:
		typedef Measurement<typename T::value_t> base_measurement_t;
		Measurement(const std::string& name) 
		:	Measurement<typename T::value_t>(name)
		{}

		template<typename UNIT_T>
		void sample(UNIT_T value)
		{
			base_measurement_t::sample(value.value());
		}
	};

	template <typename T, typename IS_UNIT = void>
	class LL_COMMON_API Rate 
	:	public TraceType<RateAccumulator<T> >, 
		public LLInstanceTracker<Rate<T>, std::string>
	{
	public:
		Rate(const std::string& name) 
		:	TraceType(name),
			LLInstanceTracker(name)
		{}

		void add(T value)
		{
			getPrimaryAccumulator().add(value);
		}
	};

	template <typename T>
	class LL_COMMON_API Rate <T, typename T::is_unit_t>
	:	public Rate<typename T::value_t>
	{
	public:
		Rate(const std::string& name) 
		:	Rate<typename T::value_t>(name)
		{}

		template<typename UNIT_T>
		void add(UNIT_T value)
		{
			getPrimaryAccumulator().add(value.value());
		}
	};

	template <typename T>
	class LL_COMMON_API Count 
	{
	public:
		Count(const std::string& name) 
		:	mIncrease(name + "_increase"),
			mDecrease(name + "_decrease"),
			mTotal(name)
		{}

		void count(T value)
		{
			if (value < 0)
			{
				mDecrease.add(value * -1);
			}
			else
			{
				mIncrease.add(value);
			}
			mTotal.add(value);
		}
	private:
		friend class LLTrace::Recording;
		Rate<T> mIncrease;
		Rate<T> mDecrease;
		Rate<T> mTotal;
	};

	class LL_COMMON_API TimerAccumulator
	{
	public:
		U32 							mTotalTimeCounter,
										mChildTimeCounter,
										mCalls;

		TimerAccumulator*				mParent;		// info for caller timer
		TimerAccumulator*				mLastCaller;	// used to bootstrap tree construction
		const class BlockTimer*			mTimer;			// points to block timer associated with this storage
		U8								mActiveCount;	// number of timers with this ID active on stack
		bool							mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame
		std::vector<TimerAccumulator*>	mChildren;		// currently assumed child timers

		void mergeSamples(const TimerAccumulator& other)
		{
			mTotalTimeCounter += other.mTotalTimeCounter;
			mChildTimeCounter += other.mChildTimeCounter;
			mCalls += other.mCalls;
		}

		void mergeDeltas(const TimerAccumulator& start, const TimerAccumulator& finish)
		{
			mTotalTimeCounter += finish.mTotalTimeCounter - start.mTotalTimeCounter;
			mChildTimeCounter += finish.mChildTimeCounter - start.mChildTimeCounter;
			mCalls += finish.mCalls - start.mCalls;
		}

		void reset()
		{
			mTotalTimeCounter = 0;
			mChildTimeCounter = 0;
			mCalls = 0;
		}

	};

	class LL_COMMON_API BlockTimer : public TraceType<TimerAccumulator>
	{
	public:
		BlockTimer(const char* name)
		:	TraceType(name)
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
				TimerAccumulator* accumulator = &block_timer.getPrimaryAccumulator(); // get per-thread accumulator
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
}

#endif // LL_LLTRACE_H
