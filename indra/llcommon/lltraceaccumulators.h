
/** 
 * @file lltraceaccumulators.h
 * @brief Storage for accumulating statistics
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

#ifndef LL_LLTRACEACCUMULATORS_H
#define LL_LLTRACEACCUMULATORS_H


#include "stdtypes.h"
#include "llpreprocessor.h"
#include "llunits.h"
#include "lltimer.h"
#include "llrefcount.h"
#include "llthreadlocalstorage.h"
#include "llmemory.h"
#include <limits>

namespace LLTrace
{
    const F64 NaN   = std::numeric_limits<double>::quiet_NaN();

    enum EBufferAppendType
    {
        SEQUENTIAL,
        NON_SEQUENTIAL
    };

    template<typename ACCUMULATOR>
    class AccumulatorBuffer : public LLRefCount
    {
        typedef AccumulatorBuffer<ACCUMULATOR> self_t;
        static const S32 DEFAULT_ACCUMULATOR_BUFFER_SIZE = 32;
    private:
        struct StaticAllocationMarker { };

        AccumulatorBuffer(StaticAllocationMarker m)
        :   mStorageSize(0),
            mStorage(NULL)
        {}

    public:
        AccumulatorBuffer()
            : mStorageSize(0),
            mStorage(NULL)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            const AccumulatorBuffer& other = *getDefaultBuffer();
            resize(sNextStorageSlot);
            for (S32 i = 0; i < sNextStorageSlot; i++)
            {
                mStorage[i] = other.mStorage[i];
            }
        }

        ~AccumulatorBuffer()
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            if (isCurrent())
            {
                LLThreadLocalSingletonPointer<ACCUMULATOR>::setInstance(NULL);
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


        AccumulatorBuffer(const AccumulatorBuffer& other)
            : mStorageSize(0),
            mStorage(NULL)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            resize(sNextStorageSlot);
            for (S32 i = 0; i < sNextStorageSlot; i++)
            {
                mStorage[i] = other.mStorage[i];
            }
        }

        void addSamples(const AccumulatorBuffer<ACCUMULATOR>& other, EBufferAppendType append_type)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            llassert(mStorageSize >= sNextStorageSlot && other.mStorageSize >= sNextStorageSlot);
            for (size_t i = 0; i < sNextStorageSlot; i++)
            {
                mStorage[i].addSamples(other.mStorage[i], append_type);
            }
        }

        void copyFrom(const AccumulatorBuffer<ACCUMULATOR>& other)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            llassert(mStorageSize >= sNextStorageSlot && other.mStorageSize >= sNextStorageSlot);
            for (size_t i = 0; i < sNextStorageSlot; i++)
            {
                mStorage[i] = other.mStorage[i];
            }
        }

        void reset(const AccumulatorBuffer<ACCUMULATOR>* other = NULL)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            llassert(mStorageSize >= sNextStorageSlot);
            for (size_t i = 0; i < sNextStorageSlot; i++)
            {
                mStorage[i].reset(other ? &other->mStorage[i] : NULL);
            }
        }

        void sync(F64SecondsImplicit time_stamp)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            llassert(mStorageSize >= sNextStorageSlot);
            for (size_t i = 0; i < sNextStorageSlot; i++)
            {
                mStorage[i].sync(time_stamp);
            }
        }

        void makeCurrent()
        {
            LLThreadLocalSingletonPointer<ACCUMULATOR>::setInstance(mStorage);
        }

        bool isCurrent() const
        {
            return LLThreadLocalSingletonPointer<ACCUMULATOR>::getInstance() == mStorage;
        }

        static void clearCurrent()
        {
            LLThreadLocalSingletonPointer<ACCUMULATOR>::setInstance(NULL);
        }

        // NOTE: this is not thread-safe.  We assume that slots are reserved in the main thread before any child threads are spawned
        size_t reserveSlot()
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            size_t next_slot = sNextStorageSlot++;
            if (next_slot >= mStorageSize)
            {
                // don't perform doubling, as this should only happen during startup
                // want to keep a tight bounds as we will have a lot of these buffers
                resize(mStorageSize + mStorageSize / 2);
            }
            llassert(mStorage && next_slot < mStorageSize);
            return next_slot;
        }

        void resize(size_t new_size)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
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
            return getNumIndices();
        }

        size_t capacity() const
        {
            return mStorageSize;
        }

        static size_t getNumIndices() 
        {
            return sNextStorageSlot;
        }

        static self_t* getDefaultBuffer()
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            static bool sInitialized = false;
            if (!sInitialized)
            {
                // this buffer is allowed to leak so that trace calls from global destructors have somewhere to put their data
                // so as not to trigger an access violation
                sDefaultBuffer = new AccumulatorBuffer(StaticAllocationMarker());
                sInitialized = true;
                sDefaultBuffer->resize(DEFAULT_ACCUMULATOR_BUFFER_SIZE);
            }
            return sDefaultBuffer;
        }

    private:
        ACCUMULATOR*    mStorage;
        size_t          mStorageSize;
        static size_t   sNextStorageSlot;
        static self_t*  sDefaultBuffer;
    };

    template<typename ACCUMULATOR> size_t AccumulatorBuffer<ACCUMULATOR>::sNextStorageSlot = 0;
    template<typename ACCUMULATOR> AccumulatorBuffer<ACCUMULATOR>* AccumulatorBuffer<ACCUMULATOR>::sDefaultBuffer = NULL;

    class EventAccumulator
    {
    public:
        typedef F64 value_t;
        static F64 getDefaultValue() { return NaN; }

        EventAccumulator()
        :   mSum(0),
            mMin(F32(NaN)),
            mMax(F32(NaN)),
            mMean(NaN),
            mSumOfSquares(0),
            mNumSamples(0),
            mLastValue(NaN)
        {}

        void record(F64 value)
        {
            if (mNumSamples == 0)
            {
                mSum = value;
                mMean = value;
                mMin = value;
                mMax = value;
            }
            else
            {
                mSum += value;
                F64 old_mean = mMean;
                mMean += (value - old_mean) / (F64)mNumSamples;
                mSumOfSquares += (value - old_mean) * (value - mMean);

                if (value < mMin) { mMin = value; }
                else if (value > mMax) { mMax = value; }
            }

            mNumSamples++;
            mLastValue = value;
        }

        void addSamples(const EventAccumulator& other, EBufferAppendType append_type);
        void reset(const EventAccumulator* other);
        void sync(F64SecondsImplicit) {}

        F64 getSum() const               { return mSum; }
        F32 getMin() const               { return mMin; }
        F32 getMax() const               { return mMax; }
        F64 getLastValue() const         { return mLastValue; }
        F64 getMean() const              { return mMean; }
        F64 getStandardDeviation() const { return sqrtf(mSumOfSquares / mNumSamples); }
        F64 getSumOfSquares() const      { return mSumOfSquares; }
        S32 getSampleCount() const       { return mNumSamples; }
        bool hasValue() const            { return mNumSamples > 0; }

        // helper utility to calculate combined sumofsquares total
        static F64 mergeSumsOfSquares(const EventAccumulator& a, const EventAccumulator& b);

    private:
        F64 mSum,
            mLastValue;

        F64 mMean,
            mSumOfSquares;

        F32 mMin,
            mMax;

        S32 mNumSamples;
    };


    class SampleAccumulator
    {
    public:
        typedef F64 value_t;
        static F64 getDefaultValue() { return NaN; }

        SampleAccumulator()
        :   mSum(0),
            mMin(F32(NaN)),
            mMax(F32(NaN)),
            mMean(NaN),
            mSumOfSquares(0),
            mLastSampleTimeStamp(0),
            mTotalSamplingTime(0),
            mNumSamples(0),
            mLastValue(NaN),
            mHasValue(false)
        {}

        void sample(F64 value)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            F64SecondsImplicit time_stamp = LLTimer::getTotalSeconds();

            // store effect of last value
            sync(time_stamp);

            if (!mHasValue)
            {
                mHasValue = true;

                mMin = value;
                mMax = value;
                mMean = value;
                mLastSampleTimeStamp = time_stamp;
            }
            else
            {
                if (value < mMin) { mMin = value; }
                else if (value > mMax) { mMax = value; }
            }

            mLastValue = value;
            mNumSamples++;
        }

        void addSamples(const SampleAccumulator& other, EBufferAppendType append_type);
        void reset(const SampleAccumulator* other);

        void sync(F64SecondsImplicit time_stamp)
        {
            if (mHasValue && time_stamp != mLastSampleTimeStamp)
            {
                F64SecondsImplicit delta_time = time_stamp - mLastSampleTimeStamp;
                mSum += mLastValue * delta_time;
                mTotalSamplingTime += delta_time;
                F64 old_mean = mMean;
                mMean += (delta_time / mTotalSamplingTime) * (mLastValue - old_mean);
                mSumOfSquares += delta_time * (mLastValue - old_mean) * (mLastValue - mMean);
            }
            mLastSampleTimeStamp = time_stamp;
        }

        F64 getSum() const               { return mSum; }
        F32 getMin() const               { return mMin; }
        F32 getMax() const               { return mMax; }
        F64 getLastValue() const         { return mLastValue; }
        F64 getMean() const              { return mMean; }
        F64 getStandardDeviation() const { return sqrtf(mSumOfSquares / mTotalSamplingTime); }
        F64 getSumOfSquares() const      { return mSumOfSquares; }
        F64SecondsImplicit getSamplingTime() const { return mTotalSamplingTime; }
        S32 getSampleCount() const       { return mNumSamples; }
        bool hasValue() const            { return mHasValue; }

        // helper utility to calculate combined sumofsquares total
        static F64 mergeSumsOfSquares(const SampleAccumulator& a, const SampleAccumulator& b);

    private:
        F64     mSum,
                mLastValue;

        F64     mMean,
                mSumOfSquares;

        F64SecondsImplicit  
                mLastSampleTimeStamp,
                mTotalSamplingTime;

        F32     mMin,
                mMax;

        S32     mNumSamples;
        // distinct from mNumSamples, since we might have inherited a last value from
        // a previous sampling period
        bool    mHasValue;      
    };

    class CountAccumulator
    {
    public:
        typedef F64 value_t;
        static F64 getDefaultValue() { return 0; }

        CountAccumulator()
        :   mSum(0),
            mNumSamples(0)
        {}

        void add(F64 value)
        {
            mNumSamples++;
            mSum += value;
        }

        void addSamples(const CountAccumulator& other, EBufferAppendType /*type*/)
        {
            mSum += other.mSum;
            mNumSamples += other.mNumSamples;
        }

        void reset(const CountAccumulator* other)
        {
            mNumSamples = 0;
            mSum = 0;
        }

        void sync(F64SecondsImplicit) {}

        F64 getSum() const { return mSum; }

        S32 getSampleCount() const { return mNumSamples; }

        bool hasValue() const            { return true; }

    private:
        F64 mSum;

        S32 mNumSamples;
    };

    class alignas(32) TimeBlockAccumulator
    {
    public:
        typedef F64Seconds value_t;
        static F64Seconds getDefaultValue() { return F64Seconds(0); }

        typedef TimeBlockAccumulator self_t;

        // fake classes that allows us to view different facets of underlying statistic
        struct CallCountFacet 
        {
            typedef S32 value_t;
        };

        struct SelfTimeFacet
        {
            typedef F64Seconds value_t;
        };

        // arrays are allocated with 32 byte alignment
        void *operator new [](size_t size)
        {
            return ll_aligned_malloc<32>(size);
        }

        void operator delete[](void* ptr, size_t size)
        {
            ll_aligned_free<32>(ptr);
        }

        TimeBlockAccumulator();
        void addSamples(const self_t& other, EBufferAppendType append_type);
        void reset(const self_t* other);
        void sync(F64SecondsImplicit) {}
        bool hasValue() const { return true; }

        //
        // members
        //
        U64                         mTotalTimeCounter,
                                    mSelfTimeCounter;
        S32                         mCalls;
        class BlockTimerStatHandle* mParent;        // last acknowledged parent of this time block
        class BlockTimerStatHandle* mLastCaller;    // used to bootstrap tree construction
        U16                         mActiveCount;   // number of timers with this ID active on stack
        bool                        mMoveUpTree;    // needs to be moved up the tree of timers at the end of frame

    };

    class BlockTimerStatHandle;

    class TimeBlockTreeNode
    {
    public:
        TimeBlockTreeNode();

        void setParent(BlockTimerStatHandle* parent);
        BlockTimerStatHandle* getParent() { return mParent; }

        BlockTimerStatHandle*                   mBlock;
        BlockTimerStatHandle*                   mParent;    
        std::vector<BlockTimerStatHandle*>      mChildren;
        bool                        mCollapsed;
        bool                        mNeedsSorting;
    };
    
    struct BlockTimerStackRecord
    {
        class BlockTimer*   mActiveTimer;
        class BlockTimerStatHandle* mTimeBlock;
        U64                 mChildTime;
    };

    struct MemAccumulator
    {
        typedef F64Bytes value_t;
        static F64Bytes getDefaultValue() { return F64Bytes(0); }

        typedef MemAccumulator self_t;

        // fake classes that allows us to view different facets of underlying statistic
        struct AllocationFacet 
        {
            typedef F64Bytes value_t;
            static F64Bytes getDefaultValue() { return F64Bytes(0); }
        };

        struct DeallocationFacet 
        {
            typedef F64Bytes value_t;
            static F64Bytes getDefaultValue() { return F64Bytes(0); }
        };

        void addSamples(const MemAccumulator& other, EBufferAppendType append_type)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            mAllocations.addSamples(other.mAllocations, append_type);
            mDeallocations.addSamples(other.mDeallocations, append_type);

            if (append_type == SEQUENTIAL)
            {
                mSize.addSamples(other.mSize, SEQUENTIAL);
            }
            else
            {
                F64 allocation_delta(other.mAllocations.getSum() - other.mDeallocations.getSum());
                mSize.sample(mSize.hasValue() 
                    ? mSize.getLastValue() + allocation_delta 
                    : allocation_delta);
            }
        }

        void reset(const MemAccumulator* other)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            mSize.reset(other ? &other->mSize : NULL);
            mAllocations.reset(other ? &other->mAllocations : NULL);
            mDeallocations.reset(other ? &other->mDeallocations : NULL);
        }

        void sync(F64SecondsImplicit time_stamp) 
        {
            mSize.sync(time_stamp);
        }

        bool hasValue() const            { return mSize.hasValue(); }

        SampleAccumulator   mSize;
        EventAccumulator    mAllocations;
        CountAccumulator    mDeallocations;
    };

    struct AccumulatorBufferGroup : public LLRefCount
    {
        AccumulatorBufferGroup();
        AccumulatorBufferGroup(const AccumulatorBufferGroup&);
        ~AccumulatorBufferGroup();

        void handOffTo(AccumulatorBufferGroup& other);
        void makeCurrent();
        bool isCurrent() const;
        static void clearCurrent();

        void append(const AccumulatorBufferGroup& other);
        void merge(const AccumulatorBufferGroup& other);
        void reset(AccumulatorBufferGroup* other = NULL);
        void sync();

        AccumulatorBuffer<CountAccumulator>     mCounts;
        AccumulatorBuffer<SampleAccumulator>    mSamples;
        AccumulatorBuffer<EventAccumulator>     mEvents;
        AccumulatorBuffer<TimeBlockAccumulator> mStackTimers;
        AccumulatorBuffer<MemAccumulator>   mMemStats;
    };
}

#endif // LL_LLTRACEACCUMULATORS_H

