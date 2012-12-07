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
#include "llunit.h"
#include "llapr.h"

#include <list>

#define LL_RECORD_BLOCK_TIME(block_timer) LLTrace::TimeBlock::Recorder LL_GLUE_TOKENS(block_time_recorder, __COUNTER__)(block_timer);

namespace LLTrace
{
	class Recording;

	typedef LLUnit<LLUnits::Bytes, F64>			Bytes;
	typedef LLUnit<LLUnits::Kilobytes, F64>		Kilobytes;
	typedef LLUnit<LLUnits::Megabytes, F64>		Megabytes;
	typedef LLUnit<LLUnits::Gigabytes, F64>		Gigabytes;
	typedef LLUnit<LLUnits::Bits, F64>			Bits;
	typedef LLUnit<LLUnits::Kilobits, F64>		Kilobits;
	typedef LLUnit<LLUnits::Megabits, F64>		Megabits;
	typedef LLUnit<LLUnits::Gigabits, F64>		Gigabits;

	typedef LLUnit<LLUnits::Seconds, F64>		Seconds;
	typedef LLUnit<LLUnits::Milliseconds, F64>	Milliseconds;
	typedef LLUnit<LLUnits::Minutes, F64>		Minutes;
	typedef LLUnit<LLUnits::Hours, F64>			Hours;
	typedef LLUnit<LLUnits::Milliseconds, F64>	Milliseconds;
	typedef LLUnit<LLUnits::Microseconds, F64>	Microseconds;
	typedef LLUnit<LLUnits::Nanoseconds, F64>	Nanoseconds;

	typedef LLUnit<LLUnits::Meters, F64>		Meters;
	typedef LLUnit<LLUnits::Kilometers, F64>	Kilometers;
	typedef LLUnit<LLUnits::Centimeters, F64>	Centimeters;
	typedef LLUnit<LLUnits::Millimeters, F64>	Millimeters;

	void init();
	void cleanup();
	bool isInitialized();

	LLThreadLocalPointer<class ThreadRecorder>& get_thread_recorder();

	class MasterThreadRecorder& getMasterThreadRecorder();

	// one per thread per type
	template<typename ACCUMULATOR>
	class AccumulatorBuffer : public LLRefCount
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

		AccumulatorBuffer(const AccumulatorBuffer& other = getDefaultBuffer())
		:	mStorageSize(other.mStorageSize),
			mStorage(new ACCUMULATOR[other.mStorageSize]),
			mNextStorageSlot(other.mNextStorageSlot)
		{
			for (S32 i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i] = other.mStorage[i];
			}
		}

		~AccumulatorBuffer()
		{
			if (sPrimaryStorage == mStorage)
			{
				//TODO pick another primary?
				sPrimaryStorage = NULL;
			}
			delete[] mStorage;
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

		void reset(const AccumulatorBuffer<ACCUMULATOR>* other = NULL)
		{
			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i].reset(other ? &other->mStorage[i] : NULL);
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
			if (LLTrace::isInitialized())
			{
				llerrs << "Attempting to declare trace object after program initialization.  Trace objects should be statically initialized." << llendl;
			}
			size_t next_slot = mNextStorageSlot++;
			if (next_slot >= mStorageSize)
			{
				resize(mStorageSize + (mStorageSize >> 2));
			}
			llassert(mStorage && next_slot < mStorageSize);
			return next_slot;
		}

		void resize(size_t new_size)
		{
			ACCUMULATOR* old_storage = mStorage;
			mStorage = new ACCUMULATOR[new_size];
			for (S32 i = 0; i < mStorageSize; i++)
			{
				mStorage[i] = old_storage[i];
			}
			mStorageSize = new_size;
			delete[] old_storage;
		}

		size_t size()
		{
			return mNextStorageSlot;
		}

		static AccumulatorBuffer<ACCUMULATOR>& getDefaultBuffer()
		{
			static AccumulatorBuffer sBuffer(STATIC_ALLOC);
			return sBuffer;
		}

	private:
		ACCUMULATOR*								mStorage;
		size_t										mStorageSize;
		size_t										mNextStorageSlot;
		static LLThreadLocalPointer<ACCUMULATOR>	sPrimaryStorage;
	};
	template<typename ACCUMULATOR> LLThreadLocalPointer<ACCUMULATOR> AccumulatorBuffer<ACCUMULATOR>::sPrimaryStorage;

	//TODO: replace with decltype when C++11 is enabled
	template<typename T>
	struct MeanValueType
	{
		typedef F64 type;
	};

	template<typename ACCUMULATOR>
	class TraceType 
	:	 public LLInstanceTracker<TraceType<ACCUMULATOR>, std::string>
	{
	public:
		typedef typename MeanValueType<TraceType<ACCUMULATOR> >::type  mean_t;

		TraceType(const char* name, const char* description = NULL)
		:	LLInstanceTracker(name),
			mName(name),
			mDescription(description ? description : "")	
		{
			mAccumulatorIndex = AccumulatorBuffer<ACCUMULATOR>::getDefaultBuffer().reserveSlot();
		}

		LL_FORCE_INLINE ACCUMULATOR& getPrimaryAccumulator() const
		{
			ACCUMULATOR* accumulator_storage = AccumulatorBuffer<ACCUMULATOR>::getPrimaryStorage();
			return accumulator_storage[mAccumulatorIndex];
		}

		size_t getIndex() const { return mAccumulatorIndex; }

		std::string& getName() { return mName; }
		const std::string& getName() const { return mName; }

	protected:
		std::string	mName;
		std::string mDescription;
		size_t		mAccumulatorIndex;
	};

	template<typename T>
	class MeasurementAccumulator
	{
	public:
		typedef T value_t;
		typedef MeasurementAccumulator<T> self_t;

		MeasurementAccumulator()
		:	mSum(0),
			mMin((std::numeric_limits<T>::max)()),
			mMax((std::numeric_limits<T>::min)()),
			mMean(0),
			mVarianceSum(0),
			mNumSamples(0),
			mLastValue(0)
		{}

		LL_FORCE_INLINE void sample(T value)
		{
			T storage_value(value);
			mNumSamples++;
			mSum += storage_value;
			if (storage_value < mMin)
			{
				mMin = storage_value;
			}
			if (storage_value > mMax)
			{
				mMax = storage_value;
			}
			F64 old_mean = mMean;
			mMean += ((F64)storage_value - old_mean) / (F64)mNumSamples;
			mVarianceSum += ((F64)storage_value - old_mean) * ((F64)storage_value - mMean);
			mLastValue = storage_value;
		}

		void addSamples(const self_t& other)
		{
			if (other.mNumSamples)
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
				F64 weight = (F64)mNumSamples / (F64)(mNumSamples + other.mNumSamples);
				mNumSamples += other.mNumSamples;
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
					mVarianceSum =  (F64)mNumSamples
								* ((((n_1 - 1.f) * sd_1 * sd_1)
									+ ((n_2 - 1.f) * sd_2 * sd_2)
									+ (((n_1 * n_2) / (n_1 + n_2))
										* ((m_1 * m_1) + (m_2 * m_2) - (2.f * m_1 * m_2))))
								/ (n_1 + n_2 - 1.f));
				}
				mLastValue = other.mLastValue;
			}
		}

		void reset(const self_t* other)
		{
			mNumSamples = 0;
			mSum = 0;
			mMin = 0;
			mMax = 0;
			mMean = 0;
			mVarianceSum = 0;
			mLastValue = other ? other->mLastValue : 0;
		}

		T	getSum() const { return (T)mSum; }
		T	getMin() const { return (T)mMin; }
		T	getMax() const { return (T)mMax; }
		T	getLastValue() const { return (T)mLastValue; }
		F64	getMean() const { return mMean; }
		F64 getStandardDeviation() const { return sqrtf(mVarianceSum / mNumSamples); }
		U32 getSampleCount() const { return mNumSamples; }

	private:
		T	mSum,
			mMin,
			mMax,
			mLastValue;

		F64	mMean,
			mVarianceSum;

		U32	mNumSamples;
	};

	template<typename T>
	class CountAccumulator
	{
	public:
		typedef CountAccumulator<T> self_t;
		typedef T value_t;

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

		void reset(const self_t* other)
		{
			mNumSamples = 0;
			mSum = 0;
		}

		T	getSum() const { return (T)mSum; }

		U32 getSampleCount() const { return mNumSamples; }

	private:
		T	mSum;

		U32	mNumSamples;
	};

	class TimeBlockAccumulator
	{
	public:
		typedef LLUnit<LLUnits::Seconds, F64> value_t;

		// fake class that allows us to view call count aspect of timeblock accumulator
		struct CallCountAspect 
		{
			typedef U32 value_t;
		};

		TimeBlockAccumulator();
		void addSamples(const TimeBlockAccumulator& other);
		void reset(const TimeBlockAccumulator* other);

		//
		// members
		//
		U64 						mSelfTimeCounter,
									mTotalTimeCounter;
		U32 mCalls;
	};


	template<>
	struct MeanValueType<TraceType<TimeBlockAccumulator> >
	{
		typedef LLUnit<LLUnits::Seconds, F64> type;
	};

	template<>
	class TraceType<TimeBlockAccumulator::CallCountAspect>
	:	public TraceType<TimeBlockAccumulator>
	{
	public:
		typedef F32 mean_t;

		TraceType(const char* name, const char* description = "")
		:	TraceType<TimeBlockAccumulator>(name, description)
		{}
	};

	class TimeBlockTreeNode
	{
	public:
		TimeBlockTreeNode();
		class TimeBlock*			mLastCaller;	// used to bootstrap tree construction
		U16							mActiveCount;	// number of timers with this ID active on stack
		bool						mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame
	};


	template <typename T = F64>
	class Measurement
	:	public TraceType<MeasurementAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >
	{
	public:
		typedef typename LLUnits::HighestPrecisionType<T>::type_t storage_t;

		Measurement(const char* name, const char* description = NULL) 
		:	TraceType(name, description)
		{}

		template<typename UNIT_T>
		void sample(UNIT_T value)
		{
			T converted_value(value);
			getPrimaryAccumulator().sample(LLUnits::rawValue(converted_value));
		}
	};

	template <typename T = F64>
	class Count 
	:	public TraceType<CountAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >
	{
	public:
		typedef typename LLUnits::HighestPrecisionType<T>::type_t storage_t;

		Count(const char* name, const char* description = NULL) 
		:	TraceType(name)
		{}

		template<typename UNIT_T>
		void add(UNIT_T value)
		{
			T converted_value(value);
			getPrimaryAccumulator().add(LLUnits::rawValue(converted_value));
		}
	};
}

#endif // LL_LLTRACE_H
