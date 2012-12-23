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
#include "llthreadlocalpointer.h"

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
		typedef AccumulatorBuffer<ACCUMULATOR> self_t;
		static const U32 DEFAULT_ACCUMULATOR_BUFFER_SIZE = 64;
	private:
		struct StaticAllocationMarker { };

		AccumulatorBuffer(StaticAllocationMarker m)
		:	mStorageSize(0),
			mStorage(NULL),
			mNextStorageSlot(0)
		{
		}

	public:

		AccumulatorBuffer(const AccumulatorBuffer& other = *getDefaultBuffer())
		:	mStorageSize(0),
			mStorage(NULL),
			mNextStorageSlot(other.mNextStorageSlot)
		{
			resize(other.mStorageSize);
			for (S32 i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i] = other.mStorage[i];
			}
		}

		~AccumulatorBuffer()
		{
			if (sPrimaryStorage == mStorage)
			{
				sPrimaryStorage = getDefaultBuffer()->mStorage;
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
			if (new_size <= mStorageSize) return;

			ACCUMULATOR* old_storage = mStorage;
			mStorage = new ACCUMULATOR[new_size];
			if (old_storage)
			{
				for (S32 i = 0; i < mStorageSize; i++)
				{
					mStorage[i] = old_storage[i];
				}
			}
			mStorageSize = new_size;
			delete[] old_storage;

			self_t* default_buffer = getDefaultBuffer();
			if (this != default_buffer
				&& new_size > default_buffer->size())
			{
				//NB: this is not thread safe, but we assume that all resizing occurs during static initialization
				default_buffer->resize(new_size);
			}
		}

		size_t size() const
		{
			return mNextStorageSlot;
		}

		static self_t* getDefaultBuffer()
		{
			// this buffer is allowed to leak so that trace calls from global destructors have somewhere to put their data
			// so as not to trigger an access violation
			//TODO: make this thread local but need to either demand-init apr or remove apr dependency
			static self_t* sBuffer = new AccumulatorBuffer(StaticAllocationMarker());
			static bool sInitialized = false;
			if (!sInitialized)
			{
				sBuffer->resize(DEFAULT_ACCUMULATOR_BUFFER_SIZE);
				sInitialized = true;
			}
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
		:	LLInstanceTracker<TraceType<ACCUMULATOR>, std::string>(name),
			mName(name),
			mDescription(description ? description : "")	
		{
			mAccumulatorIndex = AccumulatorBuffer<ACCUMULATOR>::getDefaultBuffer()->reserveSlot();
		}

		LL_FORCE_INLINE ACCUMULATOR* getPrimaryAccumulator() const
		{
			ACCUMULATOR* accumulator_storage = AccumulatorBuffer<ACCUMULATOR>::getPrimaryStorage();
			return &accumulator_storage[mAccumulatorIndex];
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

				// combine variance (and hence standard deviation) of 2 different sized sample groups using
				// the following formula: http://www.mrc-bsu.cam.ac.uk/cochrane/handbook/chapter_7/7_7_3_8_combining_groups.htm
				F64 n_1 = (F64)mNumSamples,
					n_2 = (F64)other.mNumSamples;
				F64 m_1 = mMean,
					m_2 = other.mMean;
				F64 sd_1 = getStandardDeviation(),
					sd_2 = other.getStandardDeviation();
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
		typedef TimeBlockAccumulator self_t;

		// fake class that allows us to view call count aspect of timeblock accumulator
		struct CallCountAspect 
		{
			typedef U32 value_t;
		};

		struct SelfTimeAspect
		{
			typedef LLUnit<LLUnits::Seconds, F64> value_t;
		};

		TimeBlockAccumulator();
		void addSamples(const self_t& other);
		void reset(const self_t* other);

		//
		// members
		//
		U64							mSelfTimeCounter,
									mTotalTimeCounter;
		U32							mCalls;
		class TimeBlock*			mParent;		// last acknowledged parent of this time block
		class TimeBlock*			mLastCaller;	// used to bootstrap tree construction
		U16							mActiveCount;	// number of timers with this ID active on stack
		bool						mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame

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

	template<>
	class TraceType<TimeBlockAccumulator::SelfTimeAspect>
		:	public TraceType<TimeBlockAccumulator>
	{
	public:
		typedef F32 mean_t;

		TraceType(const char* name, const char* description = "")
			:	TraceType<TimeBlockAccumulator>(name, description)
		{}
	};

	class TimeBlock;
	class TimeBlockTreeNode
	{
	public:
		TimeBlockTreeNode();

		void setParent(TimeBlock* parent);
		TimeBlock* getParent() { return mParent; }

		TimeBlock*					mBlock;
		TimeBlock*					mParent;	
		std::vector<TimeBlock*>		mChildren;
		bool						mNeedsSorting;
	};


	template <typename T = F64>
	class Measurement
	:	public TraceType<MeasurementAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >
	{
	public:
		typedef typename LLUnits::HighestPrecisionType<T>::type_t storage_t;
		typedef TraceType<MeasurementAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> > trace_t;

		Measurement(const char* name, const char* description = NULL) 
		:	trace_t(name, description)
		{}

		template<typename UNIT_T>
		void sample(UNIT_T value)
		{
			T converted_value(value);
			trace_t::getPrimaryAccumulator()->sample(LLUnits::rawValue(converted_value));
		}
	};

	template <typename T = F64>
	class Count 
	:	public TraceType<CountAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> >
	{
	public:
		typedef typename LLUnits::HighestPrecisionType<T>::type_t storage_t;
		typedef TraceType<CountAccumulator<typename LLUnits::HighestPrecisionType<T>::type_t> > trace_t;

		Count(const char* name, const char* description = NULL) 
		:	trace_t(name)
		{}

		template<typename UNIT_T>
		void add(UNIT_T value)
		{
			T converted_value(value);
			trace_t::getPrimaryAccumulator()->add(LLUnits::rawValue(converted_value));
		}
	};

struct MemStatAccumulator
{
	MemStatAccumulator()
	:	mSize(0),
		mChildSize(0),
		mAllocatedCount(0),
		mDeallocatedCount(0)
	{}

	void addSamples(const MemStatAccumulator& other)
	{
		mSize += other.mSize;
		mChildSize += other.mChildSize;
		mAllocatedCount += other.mAllocatedCount;
		mDeallocatedCount += other.mDeallocatedCount;
	}

	void reset(const MemStatAccumulator* other)
	{
		mSize = 0;
		mChildSize = 0;
		mAllocatedCount = 0;
		mDeallocatedCount = 0;
	}

	size_t		mSize,
				mChildSize;
	int			mAllocatedCount,
				mDeallocatedCount;
};

class MemStat : public TraceType<MemStatAccumulator>
{
public:
	typedef TraceType<MemStatAccumulator> trace_t;
	MemStat(const char* name)
	:	trace_t(name)
	{}
};

// measures effective memory footprint of specified type
// specialize to cover different types

template<typename T>
struct MemFootprint
{
	static size_t measure(const T& value)
	{
		return sizeof(T);
	}

	static size_t measure()
	{
		return sizeof(T);
	}
};

template<typename T>
struct MemFootprint<T*>
{
	static size_t measure(const T* value)
	{
		if (!value)
		{
			return 0;
		}
		return MemFootprint<T>::measure(*value);
	}

	static size_t measure()
	{
		return MemFootPrint<T>::measure();
	}
};

template<typename T>
struct MemFootprint<std::basic_string<T> >
{
	static size_t measure(const std::basic_string<T>& value)
	{
		return value.capacity() * sizeof(T);
	}

	static size_t measure()
	{
		return sizeof(std::basic_string<T>);
	}
};

template<typename T>
struct MemFootprint<std::vector<T> >
{
	static size_t measure(const std::vector<T>& value)
	{
		return value.capacity() * MemFootprint<T>::measure();
	}

	static size_t measure()
	{
		return sizeof(std::vector<T>);
	}
};

template<typename T>
struct MemFootprint<std::list<T> >
{
	static size_t measure(const std::list<T>& value)
	{
		return value.size() * (MemFootprint<T>::measure() + sizeof(void*) * 2);
	}

	static size_t measure()
	{
		return sizeof(std::list<T>);
	}
};

template<typename T>
class MemTrackable
{
	template<typename TRACKED, typename TRACKED_IS_TRACKER>
	struct TrackMemImpl;

	typedef MemTrackable<T> mem_trackable_t;

public:
	typedef void mem_trackable_tag_t;

	~MemTrackable()
	{
		memDisclaim(mMemFootprint);
	}

	void* operator new(size_t allocation_size) 
	{
		// reserve 8 bytes for allocation size (and preserving 8 byte alignment of structs)
		void* allocation = ::operator new(allocation_size + 8);
		*(size_t*)allocation = allocation_size;
		MemStatAccumulator* accumulator = sStat.getPrimaryAccumulator();
		if (accumulator)
		{
			accumulator->mSize += allocation_size;
			accumulator->mAllocatedCount++;
		}
		return (void*)((char*)allocation + 8);
	}

	void operator delete(void* ptr)
	{
		size_t* allocation_size = (size_t*)((char*)ptr - 8);
		MemStatAccumulator* accumulator = sStat.getPrimaryAccumulator();
		if (accumulator)
		{
			accumulator->mSize -= *allocation_size;
			accumulator->mAllocatedCount--;
			accumulator->mDeallocatedCount++;
		}
		::delete((char*)ptr - 8);
	}

	void *operator new [](size_t size)
	{
		size_t* result = (size_t*)malloc(size + 8);
		*result = size;
		MemStatAccumulator* accumulator = sStat.getPrimaryAccumulator();
		if (accumulator)
		{
			accumulator->mSize += size;
			accumulator->mAllocatedCount++;
		}
		return (void*)((char*)result + 8);
	}

	void operator delete[](void* ptr)
	{
		size_t* allocation_size = (size_t*)((char*)ptr - 8);
		MemStatAccumulator* accumulator = sStat.getPrimaryAccumulator();
		if (accumulator)
		{
			accumulator->mSize -= *allocation_size;
			accumulator->mAllocatedCount--;
			accumulator->mDeallocatedCount++;
		}
		::delete[]((char*)ptr - 8);
	}

	// claim memory associated with other objects/data as our own, adding to our calculated footprint
	template<typename T>
	T& memClaim(T& value)
	{
		TrackMemImpl<T>::claim(*this, value);
		return value;
	}

	template<typename T>
	const T& memClaim(const T& value)
	{
		TrackMemImpl<T>::claim(*this, value);
		return value;
	}


	void memClaim(size_t size)
	{
		MemStatAccumulator* accumulator = sStat.getPrimaryAccumulator();
		mMemFootprint += size;
		if (accumulator)
		{
			accumulator->mSize += size;
		}
	}

	// remove memory we had claimed from our calculated footprint
	template<typename T>
	T& memDisclaim(T& value)
	{
		TrackMemImpl<T>::disclaim(*this, value);
		return value;
	}

	template<typename T>
	const T& memDisclaim(const T& value)
	{
		TrackMemImpl<T>::disclaim(*this, value);
		return value;
	}

	void memDisclaim(size_t size)
	{
		MemStatAccumulator* accumulator = sStat.getPrimaryAccumulator();
		if (accumulator)
		{
			accumulator->mSize -= size;
		}
		mMemFootprint -= size;
	}

private:
	size_t mMemFootprint;

	template<typename TRACKED, typename TRACKED_IS_TRACKER = void>
	struct TrackMemImpl
	{
		static void claim(mem_trackable_t& tracker, const TRACKED& tracked)
		{
			MemStatAccumulator* accumulator = sStat.getPrimaryAccumulator();
			if (accumulator)
			{
				size_t footprint = MemFootprint<TRACKED>::measure(tracked);
				accumulator->mSize += footprint;
				tracker.mMemFootprint += footprint;
			}
		}

		static void disclaim(mem_trackable_t& tracker, const TRACKED& tracked)
		{
			MemStatAccumulator* accumulator = sStat.getPrimaryAccumulator();
			if (accumulator)
			{
				size_t footprint = MemFootprint<TRACKED>::measure(tracked);
				accumulator->mSize -= footprint;
				tracker.mMemFootprint -= footprint;
			}
		}
	};

	template<typename TRACKED>
	struct TrackMemImpl<TRACKED, typename TRACKED::mem_trackable_tag_t>
	{
		static void claim(mem_trackable_t& tracker, TRACKED& tracked)
		{
			MemStatAccumulator* accumulator = sStat.getPrimaryAccumulator();
			if (accumulator)
			{
				accumulator->mChildSize += MemFootprint<TRACKED>::measure(tracked);
			}
		}

		static void disclaim(mem_trackable_t& tracker, TRACKED& tracked)
		{
			MemStatAccumulator* accumulator = sStat.getPrimaryAccumulator();
			if (accumulator)
			{
				accumulator->mChildSize -= MemFootprint<TRACKED>::measure(tracked);
			}
		}
	};
	static MemStat sStat;
};

template<typename T> MemStat MemTrackable<T>::sStat(typeid(T).name());

}
#endif // LL_LLTRACE_H
