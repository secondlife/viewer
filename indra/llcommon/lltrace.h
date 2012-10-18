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
//#include "lltracethreadrecorder.h"
#include "llunit.h"
#include "llapr.h"

#include <list>

#define TOKEN_PASTE_ACTUAL(x, y) x##y
#define TOKEN_PASTE(x, y) TOKEN_PASTE_ACTUAL(x, y)
#define RECORD_BLOCK_TIME(block_timer) LLTrace::BlockTimer::Recorder TOKEN_PASTE(block_time_recorder, __COUNTER__)(block_timer);

namespace LLTrace
{
	class Recording;

	typedef LLUnit::Bytes<F64>			Bytes;
	typedef LLUnit::Kilobytes<F64>		Kilobytes;
	typedef LLUnit::Megabytes<F64>		Megabytes;
	typedef LLUnit::Gigabytes<F64>		Gigabytes;
	typedef LLUnit::Bits<F64>			Bits;
	typedef LLUnit::Kilobits<F64>		Kilobits;
	typedef LLUnit::Megabits<F64>		Megabits;
	typedef LLUnit::Gigabits<F64>		Gigabits;

	typedef LLUnit::Seconds<F64>		Seconds;
	typedef LLUnit::Milliseconds<F64>	Milliseconds;
	typedef LLUnit::Minutes<F64>		Minutes;
	typedef LLUnit::Hours<F64>			Hours;
	typedef LLUnit::Days<F64>			Days;
	typedef LLUnit::Weeks<F64>			Weeks;
	typedef LLUnit::Milliseconds<F64>	Milliseconds;
	typedef LLUnit::Microseconds<F64>	Microseconds;
	typedef LLUnit::Nanoseconds<F64>	Nanoseconds;

	typedef LLUnit::Meters<F64>			Meters;
	typedef LLUnit::Kilometers<F64>		Kilometers;
	typedef LLUnit::Centimeters<F64>	Centimeters;
	typedef LLUnit::Millimeters<F64>	Millimeters;

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

		LL_FORCE_INLINE const ACCUMULATOR& operator[](size_t index) const
		{ 
			return mStorage[index]; 
		}

		void addSamples(const AccumulatorBuffer<ACCUMULATOR>& other)
		{
			llassert(mNextStorageSlot == other.mNextStorageSlot);

			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i].addSamples(other.mStorage[i]);
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
	:	 public LLInstanceTracker<TraceType<ACCUMULATOR>, std::string>
	{
	public:
		TraceType(const char* name, const char* description = NULL)
		:	LLInstanceTracker(name),
			mName(name),
			mDescription(description ? description : "")
		{
			mAccumulatorIndex = AccumulatorBuffer<ACCUMULATOR>::getDefaultBuffer().reserveSlot();
		}

		LL_FORCE_INLINE ACCUMULATOR& getPrimaryAccumulator()
		{
			return AccumulatorBuffer<ACCUMULATOR>::getPrimaryStorage()[mAccumulatorIndex];
		}

		ACCUMULATOR& getAccumulator(AccumulatorBuffer<ACCUMULATOR>* buffer) { return (*buffer)[mAccumulatorIndex]; }
		const ACCUMULATOR& getAccumulator(const AccumulatorBuffer<ACCUMULATOR>* buffer) const { return (*buffer)[mAccumulatorIndex]; }

	protected:
		std::string	mName;
		std::string mDescription;
		size_t		mAccumulatorIndex;
	};


	template<typename T>
	class LL_COMMON_API MeasurementAccumulator
	{
	public:
		MeasurementAccumulator()
		:	mSum(0),
			mMin(std::numeric_limits<T>::max()),
			mMax(std::numeric_limits<T>::min()),
			mMean(0),
			mVarianceSum(0),
			mNumSamples(0),
			mLastValue(0)
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
			F64 old_mean = mMean;
			mMean += ((F64)value - old_mean) / (F64)mNumSamples;
			mVarianceSum += ((F64)value - old_mean) * ((F64)value - mMean);
			mLastValue = value;
		}

		void addSamples(const MeasurementAccumulator<T>& other)
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
			F64 weight = (F64)mNumSamples / (F64)(mNumSamples + other.mNumSamples);
			mMean = mMean * weight + other.mMean * (1.f - weight);

			F64 n_1 = (F64)mNumSamples,
				n_2 = (F64)other.mNumSamples;
			F64 m_1 = mMean,
				m_2 = other.mMean;
			F64 sd_1 = getStandardDeviation(),
				sd_2 = other.getStandardDeviation();
			// combine variance (and hence standard deviation) of 2 different sized sample groups using
			// the following formula: http://www.mrc-bsu.cam.ac.uk/cochrane/handbook/chapter_7/7_7_3_8_combining_groups.htm
			if (n_1 == 0)
			{
				mVarianceSum = other.mVarianceSum;
			}
			else if (n_2 == 0)
			{
				// don't touch variance
				// mVarianceSum = mVarianceSum;
			}
			else
			{
				mVarianceSum =  (F32)mNumSamples
							* ((((n_1 - 1.f) * sd_1 * sd_1)
								+ ((n_2 - 1.f) * sd_2 * sd_2)
								+ (((n_1 * n_2) / (n_1 + n_2))
									* ((m_1 * m_1) + (m_2 * m_2) - (2.f * m_1 * m_2))))
							/ (n_1 + n_2 - 1.f));
			}
			mLastValue = other.mLastValue;
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
		T	getLastValue() const { return mLastValue; }
		F64	getMean() const { return mMean; }
		F64 getStandardDeviation() const { return sqrtf(mVarianceSum / mNumSamples); }
		U32 getSampleCount() const { return mNumSamples; }

	private:
		T	mSum,
			mMin,
			mMax,
			mLastValue;

		F64 mMean,
			mVarianceSum;

		U32	mNumSamples;
	};

	template<typename T>
	class LL_COMMON_API CountAccumulator
	{
	public:
		CountAccumulator()
		:	mSum(0),
			mNumSamples(0)
		{}

		LL_FORCE_INLINE void add(T value)
		{
			mNumSamples++;
			mSum += value;
		}

		void addSamples(const CountAccumulator<T>& other)
		{
			mSum += other.mSum;
			mNumSamples += other.mNumSamples;
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

	typedef TraceType<MeasurementAccumulator<F64> > measurement_common_t;

	template <typename T = F64, typename IS_UNIT = void>
	class LL_COMMON_API Measurement
	:	public TraceType<MeasurementAccumulator<T> >
	{
	public:
		typedef T storage_t;

		Measurement(const char* name, const char* description = NULL) 
		:	TraceType(name, description)
		{}

		void sample(T value)
		{
			getPrimaryAccumulator().sample(value);
		}
	};

	template <typename T>
	class LL_COMMON_API Measurement <T, typename T::is_unit_tag_t>
	:	public TraceType<MeasurementAccumulator<typename T::storage_t> >
	{
	public:
		typedef typename T::storage_t storage_t;
		typedef Measurement<typename T::storage_t> base_measurement_t;

		Measurement(const char* name, const char* description = NULL) 
		:	TraceType(name, description)
		{}

		template<typename UNIT_T>
		void sample(UNIT_T value)
		{
			T converted_value;
			converted_value.assignFrom(value);
			getPrimaryAccumulator().sample(converted_value.value());
		}
	};

	typedef TraceType<CountAccumulator<F64> > count_common_t;

	template <typename T = F64, typename IS_UNIT = void>
	class LL_COMMON_API Count 
	:	public TraceType<CountAccumulator<T> >
	{
	public:
		typedef T storage_t;

		Count(const char* name, const char* description = NULL) 
		:	TraceType(name)
		{}

		void add(T value)
		{
			getPrimaryAccumulator().add(value);
		}
	};

	template <typename T>
	class LL_COMMON_API Count <T, typename T::is_unit_tag_t>
	:	public TraceType<CountAccumulator<typename T::storage_t> >
	{
	public:
		typedef typename T::storage_t storage_t;
		typedef Count<typename T::storage_t> base_count_t;

		Count(const char* name, const char* description = NULL) 
		:	TraceType(name)
		{}

		template<typename UNIT_T>
		void add(UNIT_T value)
		{
			T converted_value;
			converted_value.assignFrom(value);
			getPrimaryAccumulator().add(converted_value.value());
		}
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

		void addSamples(const TimerAccumulator& other)
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
