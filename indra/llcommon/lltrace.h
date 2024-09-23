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
#include "lltraceaccumulators.h"
#include "llthreadlocalstorage.h"
#include "lltimer.h"
#include "llpointer.h"
#include "llunits.h"

#define LL_TRACE_ENABLED 1

namespace LLTrace
{
class Recording;

template<typename T>
T storage_value(T val) { return val; }

template<typename UNIT_TYPE, typename STORAGE_TYPE>
STORAGE_TYPE storage_value(LLUnit<STORAGE_TYPE, UNIT_TYPE> val) { return val.value(); }

template<typename UNIT_TYPE, typename STORAGE_TYPE>
STORAGE_TYPE storage_value(LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> val) { return val.value(); }

class StatBase
{
public:
    StatBase(const char* name, const char* description);
    virtual ~StatBase() {}
    virtual const char* getUnitLabel() const;

    const std::string& getName() const { return mName; }
    const std::string& getDescription() const { return mDescription; }

protected:
    std::string mName;
    std::string mDescription;
};

template<typename ACCUMULATOR>
class StatType
:   public StatBase,
    public LLInstanceTracker<StatType<ACCUMULATOR>, std::string>
{
public:
    typedef LLInstanceTracker<StatType<ACCUMULATOR>, std::string> instance_tracker_t;
    StatType(const char* name, const char* description)
    :   instance_tracker_t(name),
        StatBase(name, description),
        mAccumulatorIndex(AccumulatorBuffer<ACCUMULATOR>::getDefaultBuffer()->reserveSlot())
    {}

    LL_FORCE_INLINE ACCUMULATOR& getCurrentAccumulator() const
    {
        ACCUMULATOR* accumulator_storage = LLThreadLocalSingletonPointer<ACCUMULATOR>::getInstance();
        return accumulator_storage ? accumulator_storage[mAccumulatorIndex] : (*AccumulatorBuffer<ACCUMULATOR>::getDefaultBuffer())[mAccumulatorIndex];
    }

    size_t getIndex() const { return mAccumulatorIndex; }
    static size_t getNumIndices() { return AccumulatorBuffer<ACCUMULATOR>::getNumIndices(); }

protected:
    const size_t        mAccumulatorIndex;
};


template<>
class StatType<TimeBlockAccumulator::CallCountFacet>
:   public StatType<TimeBlockAccumulator>
{
public:

    StatType(const char* name, const char* description = "")
    :   StatType<TimeBlockAccumulator>(name, description)
    {}
};

template<>
class StatType<TimeBlockAccumulator::SelfTimeFacet>
    :   public StatType<TimeBlockAccumulator>
{
public:

    StatType(const char* name, const char* description = "")
        :   StatType<TimeBlockAccumulator>(name, description)
    {}
};

template <typename T = F64>
class EventStatHandle
:   public StatType<EventAccumulator>
{
public:
    typedef F64 storage_t;
    typedef StatType<EventAccumulator> stat_t;
    typedef EventStatHandle<T> self_t;

    EventStatHandle(const char* name, const char* description = NULL)
    :   stat_t(name, description)
    {}

    /*virtual*/ const char* getUnitLabel() const { return LLGetUnitLabel<T>::getUnitLabel(); }

};

template<typename T, typename VALUE_T>
void record(EventStatHandle<T>& measurement, VALUE_T value)
{
#if LL_TRACE_ENABLED
    T converted_value(value);
    measurement.getCurrentAccumulator().record(storage_value(converted_value));
#endif
}

template <typename T = F64>
class SampleStatHandle
:   public StatType<SampleAccumulator>
{
public:
    typedef F64 storage_t;
    typedef StatType<SampleAccumulator> stat_t;
    typedef SampleStatHandle<T> self_t;

    SampleStatHandle(const char* name, const char* description = NULL)
    :   stat_t(name, description)
    {}

    /*virtual*/ const char* getUnitLabel() const { return LLGetUnitLabel<T>::getUnitLabel(); }
};

template<typename T, typename VALUE_T>
void sample(SampleStatHandle<T>& measurement, VALUE_T value)
{
#if LL_TRACE_ENABLED
    T converted_value(value);
    measurement.getCurrentAccumulator().sample(storage_value(converted_value));
#endif
}

template <typename T = F64>
class CountStatHandle
:   public StatType<CountAccumulator>
{
public:
    typedef F64 storage_t;
    typedef StatType<CountAccumulator> stat_t;
    typedef CountStatHandle<T> self_t;

    CountStatHandle(const char* name, const char* description = NULL)
    :   stat_t(name, description)
    {}

    /*virtual*/ const char* getUnitLabel() const { return LLGetUnitLabel<T>::getUnitLabel(); }
};

template<typename T, typename VALUE_T>
void add(CountStatHandle<T>& count, VALUE_T value)
{
#if LL_TRACE_ENABLED
    T converted_value(value);
    count.getCurrentAccumulator().add(storage_value(converted_value));
#endif
}

// measures effective memory footprint of specified type
// specialize to cover different types
template<typename T, typename IS_MEM_TRACKABLE = void, typename IS_UNITS = void>
struct MeasureMem
{
    static size_t measureFootprint(const T& value)
    {
        return sizeof(T);
    }
};

template<typename T, typename IS_BYTES>
struct MeasureMem<T, typename T::mem_trackable_tag_t, IS_BYTES>
{
    static size_t measureFootprint(const T& value)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
        return sizeof(T) + value.getMemFootprint();
    }
};

template<typename T, typename IS_MEM_TRACKABLE>
struct MeasureMem<T, IS_MEM_TRACKABLE, typename T::is_unit_t>
{
    static size_t measureFootprint(const T& value)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
        return U32Bytes(value).value();
    }
};

template<typename T, typename IS_MEM_TRACKABLE, typename IS_BYTES>
struct MeasureMem<T*, IS_MEM_TRACKABLE, IS_BYTES>
{
    static size_t measureFootprint(const T* value)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
        if (!value)
        {
            return 0;
        }
        return MeasureMem<T>::measureFootprint(*value);
    }
};

template<typename T, typename IS_MEM_TRACKABLE, typename IS_BYTES>
struct MeasureMem<LLPointer<T>, IS_MEM_TRACKABLE, IS_BYTES>
{
    static size_t measureFootprint(const LLPointer<T> value)
    {
        if (value.isNull())
        {
            return 0;
        }
        return MeasureMem<T>::measureFootprint(*value);
    }
};

template<typename IS_MEM_TRACKABLE, typename IS_BYTES>
struct MeasureMem<S32, IS_MEM_TRACKABLE, IS_BYTES>
{
    static size_t measureFootprint(S32 value)
    {
        return value;
    }
};

template<typename IS_MEM_TRACKABLE, typename IS_BYTES>
struct MeasureMem<U32, IS_MEM_TRACKABLE, IS_BYTES>
{
    static size_t measureFootprint(U32 value)
    {
        return value;
    }
};

template<typename T, typename IS_MEM_TRACKABLE, typename IS_BYTES>
struct MeasureMem<std::basic_string<T>, IS_MEM_TRACKABLE, IS_BYTES>
{
    static size_t measureFootprint(const std::basic_string<T>& value)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
        return value.capacity() * sizeof(T);
    }
};

}

#endif // LL_LLTRACE_H
