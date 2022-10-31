/** 
 * @file lltracerecording.h
 * @brief Sampling object for collecting runtime statistics originating from lltrace.
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

#ifndef LL_LLTRACERECORDING_H
#define LL_LLTRACERECORDING_H

#include "stdtypes.h"
#include "llpreprocessor.h"

#include "lltimer.h"
#include "lltraceaccumulators.h"
#include "llpointer.h"

class LLStopWatchControlsMixinCommon
{
public:
    virtual ~LLStopWatchControlsMixinCommon() {}

    enum EPlayState
    {
        STOPPED,
        PAUSED,
        STARTED
    };

    void start();   // moves to started state, resetting if stopped
    void stop();    // moves to stopped state
    void pause();   // moves to paused state, unless stopped
    void unpause(); // moves to started state if paused
    void resume();  // moves to started state, without resetting
    void restart(); // moves to started state, always resetting
    void reset();   // resets

    bool isStarted() const { return mPlayState == STARTED; }
    bool isPaused() const  { return mPlayState == PAUSED; }
    bool isStopped() const { return mPlayState == STOPPED; }

    EPlayState getPlayState() const { return mPlayState; }
    // force play state to specific value by calling appropriate handle* methods
    void setPlayState(EPlayState state);

protected:
    LLStopWatchControlsMixinCommon()
    :   mPlayState(STOPPED)
    {}

private:
    // override these methods to provide started/stopped semantics

    // activate behavior (without reset)
    virtual void handleStart() = 0;
    // deactivate behavior
    virtual void handleStop() = 0;
    // clear accumulated state, may be called while started
    virtual void handleReset() = 0;

    EPlayState mPlayState;
};

template<typename DERIVED>
class LLStopWatchControlsMixin
:   public LLStopWatchControlsMixinCommon
{
public:

    typedef LLStopWatchControlsMixin<DERIVED> self_t;
    virtual void splitTo(DERIVED& other)
    {
        EPlayState play_state = getPlayState();
        stop();
        other.reset();

        handleSplitTo(other);

        other.setPlayState(play_state);
    }

    virtual void splitFrom(DERIVED& other)
    {
        static_cast<self_t&>(other).handleSplitTo(*static_cast<DERIVED*>(this));
    }
private:
    self_t& operator = (const self_t& other)
    {
        // don't do anything, derived class must implement logic
    }

    // atomically stop this object while starting the other
    // no data can be missed in between stop and start
    virtual void handleSplitTo(DERIVED& other) {};

};

namespace LLTrace
{
    template<typename T>
    class StatType;

    template<typename T>
    class CountStatHandle;

    template<typename T>
    class SampleStatHandle;

    template<typename T>
    class EventStatHandle;

    class MemStatHandle;

    template<typename T>
    struct RelatedTypes
    {
        typedef F64 fractional_t;
        typedef T   sum_t;
    };

    template<typename T, typename UNIT_T>
    struct RelatedTypes<LLUnit<T, UNIT_T> >
    {
        typedef LLUnit<typename RelatedTypes<T>::fractional_t, UNIT_T> fractional_t;
        typedef LLUnit<typename RelatedTypes<T>::sum_t, UNIT_T> sum_t;
    };

    template<>
    struct RelatedTypes<bool>
    {
        typedef F64 fractional_t;
        typedef S32 sum_t;
    };

    class Recording 
    :   public LLStopWatchControlsMixin<Recording>
    {
    public:
        Recording(EPlayState state = LLStopWatchControlsMixinCommon::STOPPED);

        Recording(const Recording& other);
        ~Recording();

        Recording& operator = (const Recording& other);

        // accumulate data from subsequent, non-overlapping recording
        void appendRecording(Recording& other);

        // grab latest recorded data
        void update();

        // ensure that buffers are exclusively owned by this recording
        void makeUnique() { mBuffers.makeUnique(); }

        // Timer accessors
        bool hasValue(const StatType<TimeBlockAccumulator>& stat);
        F64Seconds getSum(const StatType<TimeBlockAccumulator>& stat);
        F64Seconds getSum(const StatType<TimeBlockAccumulator::SelfTimeFacet>& stat);
        S32 getSum(const StatType<TimeBlockAccumulator::CallCountFacet>& stat);

        F64Seconds getPerSec(const StatType<TimeBlockAccumulator>& stat);
        F64Seconds getPerSec(const StatType<TimeBlockAccumulator::SelfTimeFacet>& stat);
        F32 getPerSec(const StatType<TimeBlockAccumulator::CallCountFacet>& stat);

        // Memory accessors
        bool hasValue(const StatType<MemAccumulator>& stat);
        F64Kilobytes getMin(const StatType<MemAccumulator>& stat);
        F64Kilobytes getMean(const StatType<MemAccumulator>& stat);
        F64Kilobytes getMax(const StatType<MemAccumulator>& stat);
        F64Kilobytes getStandardDeviation(const StatType<MemAccumulator>& stat);
        F64Kilobytes getLastValue(const StatType<MemAccumulator>& stat);

        bool hasValue(const StatType<MemAccumulator::AllocationFacet>& stat);
        F64Kilobytes getSum(const StatType<MemAccumulator::AllocationFacet>& stat);
        F64Kilobytes getPerSec(const StatType<MemAccumulator::AllocationFacet>& stat);
        S32 getSampleCount(const StatType<MemAccumulator::AllocationFacet>& stat);

        bool hasValue(const StatType<MemAccumulator::DeallocationFacet>& stat);
        F64Kilobytes getSum(const StatType<MemAccumulator::DeallocationFacet>& stat);
        F64Kilobytes getPerSec(const StatType<MemAccumulator::DeallocationFacet>& stat);
        S32 getSampleCount(const StatType<MemAccumulator::DeallocationFacet>& stat);

        // CountStatHandle accessors
        bool hasValue(const StatType<CountAccumulator>& stat);
        F64 getSum(const StatType<CountAccumulator>& stat);
        template <typename T>
        typename RelatedTypes<T>::sum_t getSum(const CountStatHandle<T>& stat)
        {
            return (typename RelatedTypes<T>::sum_t)getSum(static_cast<const StatType<CountAccumulator>&> (stat));
        }

        F64 getPerSec(const StatType<CountAccumulator>& stat);
        template <typename T>
        typename RelatedTypes<T>::fractional_t getPerSec(const CountStatHandle<T>& stat)
        {
            return (typename RelatedTypes<T>::fractional_t)getPerSec(static_cast<const StatType<CountAccumulator>&> (stat));
        }

        S32 getSampleCount(const StatType<CountAccumulator>& stat);


        // SampleStatHandle accessors
        bool hasValue(const StatType<SampleAccumulator>& stat);

        F64 getMin(const StatType<SampleAccumulator>& stat);
        template <typename T>
        T getMin(const SampleStatHandle<T>& stat)
        {
            return (T)getMin(static_cast<const StatType<SampleAccumulator>&> (stat));
        }

        F64 getMax(const StatType<SampleAccumulator>& stat);
        template <typename T>
        T getMax(const SampleStatHandle<T>& stat)
        {
            return (T)getMax(static_cast<const StatType<SampleAccumulator>&> (stat));
        }

        F64 getMean(const StatType<SampleAccumulator>& stat);
        template <typename T>
        typename RelatedTypes<T>::fractional_t getMean(SampleStatHandle<T>& stat)
        {
            return (typename RelatedTypes<T>::fractional_t)getMean(static_cast<const StatType<SampleAccumulator>&> (stat));
        }

        F64 getStandardDeviation(const StatType<SampleAccumulator>& stat);
        template <typename T>
        typename RelatedTypes<T>::fractional_t getStandardDeviation(const SampleStatHandle<T>& stat)
        {
            return (typename RelatedTypes<T>::fractional_t)getStandardDeviation(static_cast<const StatType<SampleAccumulator>&> (stat));
        }

        F64 getLastValue(const StatType<SampleAccumulator>& stat);
        template <typename T>
        T getLastValue(const SampleStatHandle<T>& stat)
        {
            return (T)getLastValue(static_cast<const StatType<SampleAccumulator>&> (stat));
        }

        S32 getSampleCount(const StatType<SampleAccumulator>& stat);

        // EventStatHandle accessors
        bool hasValue(const StatType<EventAccumulator>& stat);

        F64 getSum(const StatType<EventAccumulator>& stat);
        template <typename T>
        typename RelatedTypes<T>::sum_t getSum(const EventStatHandle<T>& stat)
        {
            return (typename RelatedTypes<T>::sum_t)getSum(static_cast<const StatType<EventAccumulator>&> (stat));
        }

        F64 getMin(const StatType<EventAccumulator>& stat);
        template <typename T>
        T getMin(const EventStatHandle<T>& stat)
        {
            return (T)getMin(static_cast<const StatType<EventAccumulator>&> (stat));
        }

        F64 getMax(const StatType<EventAccumulator>& stat);
        template <typename T>
        T getMax(const EventStatHandle<T>& stat)
        {
            return (T)getMax(static_cast<const StatType<EventAccumulator>&> (stat));
        }

        F64 getMean(const StatType<EventAccumulator>& stat);
        template <typename T>
        typename RelatedTypes<T>::fractional_t getMean(EventStatHandle<T>& stat)
        {
            return (typename RelatedTypes<T>::fractional_t)getMean(static_cast<const StatType<EventAccumulator>&> (stat));
        }

        F64 getStandardDeviation(const StatType<EventAccumulator>& stat);
        template <typename T>
        typename RelatedTypes<T>::fractional_t getStandardDeviation(const EventStatHandle<T>& stat)
        {
            return (typename RelatedTypes<T>::fractional_t)getStandardDeviation(static_cast<const StatType<EventAccumulator>&> (stat));
        }

        F64 getLastValue(const StatType<EventAccumulator>& stat);
        template <typename T>
        T getLastValue(const EventStatHandle<T>& stat)
        {
            return (T)getLastValue(static_cast<const StatType<EventAccumulator>&> (stat));
        }

        S32 getSampleCount(const StatType<EventAccumulator>& stat);

        F64Seconds getDuration() const { return mElapsedSeconds; }

    protected:
        friend class ThreadRecorder;

        // implementation for LLStopWatchControlsMixin
        /*virtual*/ void handleStart();
        /*virtual*/ void handleStop();
        /*virtual*/ void handleReset();
        /*virtual*/ void handleSplitTo(Recording& other);

        // returns data for current thread
        class ThreadRecorder* getThreadRecorder(); 

        LLTimer                                         mSamplingTimer;
        F64Seconds                                      mElapsedSeconds;
        LLCopyOnWritePointer<AccumulatorBufferGroup>    mBuffers;
        AccumulatorBufferGroup*                         mActiveBuffers;

    };

    class LL_COMMON_API PeriodicRecording
    :   public LLStopWatchControlsMixin<PeriodicRecording>
    {
    public:
        PeriodicRecording(S32 num_periods, EPlayState state = STOPPED);
        ~PeriodicRecording();

        void nextPeriod();
        S32 getNumRecordedPeriods() 
        { 
            // current period counts if not active
            return mNumRecordedPeriods + (isStarted() ? 0 : 1); 
        }

        F64Seconds getDuration() const;

        void appendPeriodicRecording(PeriodicRecording& other);
        void appendRecording(Recording& recording);
        Recording& getLastRecording();
        const Recording& getLastRecording() const;
        Recording& getCurRecording();
        const Recording& getCurRecording() const;
        Recording& getPrevRecording(S32 offset);
        const Recording& getPrevRecording(S32 offset) const;
        Recording snapshotCurRecording() const;

        template <typename T>
        S32 getSampleCount(const StatType<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            num_periods = llmin(num_periods, getNumRecordedPeriods());

            S32 num_samples = 0;
            for (S32 i = 1; i <= num_periods; i++)
            {
                Recording& recording = getPrevRecording(i);
                num_samples += recording.getSampleCount(stat);
            }
            return num_samples;
        }
        
        //
        // PERIODIC MIN
        //

        // catch all for stats that have a defined sum
        template <typename T>
        typename T::value_t getPeriodMin(const StatType<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            num_periods = llmin(num_periods, getNumRecordedPeriods());

            bool has_value = false;
            typename T::value_t min_val(std::numeric_limits<typename T::value_t>::max());
            for (S32 i = 1; i <= num_periods; i++)
            {
                Recording& recording = getPrevRecording(i);
                if (recording.hasValue(stat))
                {
                    min_val = llmin(min_val, recording.getSum(stat));
                    has_value = true;
                }
            }

            return has_value 
                ? min_val 
                : T::getDefaultValue();
        }

        template<typename T>
        T getPeriodMin(const CountStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return T(getPeriodMin(static_cast<const StatType<CountAccumulator>&>(stat), num_periods));
        }

        F64 getPeriodMin(const StatType<SampleAccumulator>& stat, S32 num_periods = S32_MAX);
        template<typename T>
        T getPeriodMin(const SampleStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return T(getPeriodMin(static_cast<const StatType<SampleAccumulator>&>(stat), num_periods));
        }

        F64 getPeriodMin(const StatType<EventAccumulator>& stat, S32 num_periods = S32_MAX);
        template<typename T>
        T getPeriodMin(const EventStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return T(getPeriodMin(static_cast<const StatType<EventAccumulator>&>(stat), num_periods));
        }

        F64Kilobytes getPeriodMin(const StatType<MemAccumulator>& stat, S32 num_periods = S32_MAX);
        F64Kilobytes getPeriodMin(const MemStatHandle& stat, S32 num_periods = S32_MAX);

        template <typename T>
        typename RelatedTypes<typename T::value_t>::fractional_t getPeriodMinPerSec(const StatType<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            num_periods = llmin(num_periods, getNumRecordedPeriods());

            typename RelatedTypes<typename T::value_t>::fractional_t min_val(std::numeric_limits<F64>::max());
            for (S32 i = 1; i <= num_periods; i++)
            {
                Recording& recording = getPrevRecording(i);
                min_val = llmin(min_val, recording.getPerSec(stat));
            }
            return (typename RelatedTypes<typename T::value_t>::fractional_t) min_val;
        }

        template<typename T>
        typename RelatedTypes<T>::fractional_t getPeriodMinPerSec(const CountStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return typename RelatedTypes<T>::fractional_t(getPeriodMinPerSec(static_cast<const StatType<CountAccumulator>&>(stat), num_periods));
        }

        //
        // PERIODIC MAX
        //

        // catch all for stats that have a defined sum
        template <typename T>
        typename T::value_t getPeriodMax(const StatType<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            num_periods = llmin(num_periods, getNumRecordedPeriods());

            bool has_value = false;
            typename T::value_t max_val(std::numeric_limits<typename T::value_t>::min());
            for (S32 i = 1; i <= num_periods; i++)
            {
                Recording& recording = getPrevRecording(i);
                if (recording.hasValue(stat))
                {
                    max_val = llmax(max_val, recording.getSum(stat));
                    has_value = true;
                }
            }

            return has_value 
                ? max_val 
                : T::getDefaultValue();
        }

        template<typename T>
        T getPeriodMax(const CountStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return T(getPeriodMax(static_cast<const StatType<CountAccumulator>&>(stat), num_periods));
        }

        F64 getPeriodMax(const StatType<SampleAccumulator>& stat, S32 num_periods = S32_MAX);
        template<typename T>
        T getPeriodMax(const SampleStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return T(getPeriodMax(static_cast<const StatType<SampleAccumulator>&>(stat), num_periods));
        }

        F64 getPeriodMax(const StatType<EventAccumulator>& stat, S32 num_periods = S32_MAX);
        template<typename T>
        T getPeriodMax(const EventStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return T(getPeriodMax(static_cast<const StatType<EventAccumulator>&>(stat), num_periods));
        }

        F64Kilobytes getPeriodMax(const StatType<MemAccumulator>& stat, S32 num_periods = S32_MAX);
        F64Kilobytes getPeriodMax(const MemStatHandle& stat, S32 num_periods = S32_MAX);

        template <typename T>
        typename RelatedTypes<typename T::value_t>::fractional_t getPeriodMaxPerSec(const StatType<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            num_periods = llmin(num_periods, getNumRecordedPeriods());

            F64 max_val = std::numeric_limits<F64>::min();
            for (S32 i = 1; i <= num_periods; i++)
            {
                Recording& recording = getPrevRecording(i);
                max_val = llmax(max_val, recording.getPerSec(stat));
            }
            return (typename RelatedTypes<typename T::value_t>::fractional_t)max_val;
        }

        template<typename T>
        typename RelatedTypes<T>::fractional_t getPeriodMaxPerSec(const CountStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return typename RelatedTypes<T>::fractional_t(getPeriodMaxPerSec(static_cast<const StatType<CountAccumulator>&>(stat), num_periods));
        }

        //
        // PERIODIC MEAN
        //

        // catch all for stats that have a defined sum
        template <typename T>
        typename RelatedTypes<typename T::value_t>::fractional_t getPeriodMean(const StatType<T >& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            num_periods = llmin(num_periods, getNumRecordedPeriods());

            typename RelatedTypes<typename T::value_t>::fractional_t mean(0);

            for (S32 i = 1; i <= num_periods; i++)
            {
                Recording& recording = getPrevRecording(i);
                if (recording.getDuration() > (F32Seconds)0.f)
                {
                    mean += recording.getSum(stat);
                }
            }
            return (num_periods
                ? typename RelatedTypes<typename T::value_t>::fractional_t(mean / num_periods)
                : typename RelatedTypes<typename T::value_t>::fractional_t(NaN));
        }

        template<typename T>
        typename RelatedTypes<T>::fractional_t getPeriodMean(const CountStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return typename RelatedTypes<T>::fractional_t(getPeriodMean(static_cast<const StatType<CountAccumulator>&>(stat), num_periods));
        }
        F64 getPeriodMean(const StatType<SampleAccumulator>& stat, S32 num_periods = S32_MAX);
        template<typename T> 
        typename RelatedTypes<T>::fractional_t getPeriodMean(const SampleStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return typename RelatedTypes<T>::fractional_t(getPeriodMean(static_cast<const StatType<SampleAccumulator>&>(stat), num_periods));
        }

        F64 getPeriodMean(const StatType<EventAccumulator>& stat, S32 num_periods = S32_MAX);
        template<typename T>
        typename RelatedTypes<T>::fractional_t getPeriodMean(const EventStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return typename RelatedTypes<T>::fractional_t(getPeriodMean(static_cast<const StatType<EventAccumulator>&>(stat), num_periods));
        }

        F64Kilobytes getPeriodMean(const StatType<MemAccumulator>& stat, S32 num_periods = S32_MAX);
        F64Kilobytes getPeriodMean(const MemStatHandle& stat, S32 num_periods = S32_MAX);
        
        template <typename T>
        typename RelatedTypes<typename T::value_t>::fractional_t getPeriodMeanPerSec(const StatType<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            num_periods = llmin(num_periods, getNumRecordedPeriods());

            typename RelatedTypes<typename T::value_t>::fractional_t mean = 0;

            for (S32 i = 1; i <= num_periods; i++)
            {
                Recording& recording = getPrevRecording(i);
                if (recording.getDuration() > (F32Seconds)0.f)
                {
                    mean += recording.getPerSec(stat);
                }
            }

            return (num_periods
                ? typename RelatedTypes<typename T::value_t>::fractional_t(mean / num_periods)
                : typename RelatedTypes<typename T::value_t>::fractional_t(NaN));
        }

        template<typename T>
        typename RelatedTypes<T>::fractional_t getPeriodMeanPerSec(const CountStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return typename RelatedTypes<T>::fractional_t(getPeriodMeanPerSec(static_cast<const StatType<CountAccumulator>&>(stat), num_periods));
        }

        F64 getPeriodMedian( const StatType<SampleAccumulator>& stat, S32 num_periods = S32_MAX);

        template <typename T>
        typename RelatedTypes<typename T::value_t>::fractional_t getPeriodMedianPerSec(const StatType<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            num_periods = llmin(num_periods, getNumRecordedPeriods());

            std::vector <typename RelatedTypes<typename T::value_t>::fractional_t> buf;
            for (S32 i = 1; i <= num_periods; i++)
            {
                Recording& recording = getPrevRecording(i);
                if (recording.getDuration() > (F32Seconds)0.f)
                {
                    buf.push_back(recording.getPerSec(stat));
                }
            }
            std::sort(buf.begin(), buf.end());

            return typename RelatedTypes<T>::fractional_t((buf.size() % 2 == 0) ? (buf[buf.size() / 2 - 1] + buf[buf.size() / 2]) / 2 : buf[buf.size() / 2]);
        }

        template<typename T>
        typename RelatedTypes<T>::fractional_t getPeriodMedianPerSec(const CountStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return typename RelatedTypes<T>::fractional_t(getPeriodMedianPerSec(static_cast<const StatType<CountAccumulator>&>(stat), num_periods));
        }

        //
        // PERIODIC STANDARD DEVIATION
        //

        F64 getPeriodStandardDeviation(const StatType<SampleAccumulator>& stat, S32 num_periods = S32_MAX);

        template<typename T> 
        typename RelatedTypes<T>::fractional_t getPeriodStandardDeviation(const SampleStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return typename RelatedTypes<T>::fractional_t(getPeriodStandardDeviation(static_cast<const StatType<SampleAccumulator>&>(stat), num_periods));
        }

        F64 getPeriodStandardDeviation(const StatType<EventAccumulator>& stat, S32 num_periods = S32_MAX);
        template<typename T>
        typename RelatedTypes<T>::fractional_t getPeriodStandardDeviation(const EventStatHandle<T>& stat, S32 num_periods = S32_MAX)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            return typename RelatedTypes<T>::fractional_t(getPeriodStandardDeviation(static_cast<const StatType<EventAccumulator>&>(stat), num_periods));
        }

        F64Kilobytes getPeriodStandardDeviation(const StatType<MemAccumulator>& stat, S32 num_periods = S32_MAX);
        F64Kilobytes getPeriodStandardDeviation(const MemStatHandle& stat, S32 num_periods = S32_MAX);

    private:
        // implementation for LLStopWatchControlsMixin
        /*virtual*/ void handleStart();
        /*virtual*/ void handleStop();
        /*virtual*/ void handleReset();
        /*virtual*/ void handleSplitTo(PeriodicRecording& other);

    private:
        std::vector<Recording>  mRecordingPeriods;
        const bool              mAutoResize;
        S32                     mCurPeriod;
        S32                     mNumRecordedPeriods;
    };

    PeriodicRecording& get_frame_recording();

    class ExtendableRecording
    :   public LLStopWatchControlsMixin<ExtendableRecording>
    {
    public:
        void extend();

        Recording& getAcceptedRecording() { return mAcceptedRecording; }
        const Recording& getAcceptedRecording() const {return mAcceptedRecording;}

        Recording& getPotentialRecording()              { return mPotentialRecording; }
        const Recording& getPotentialRecording() const  { return mPotentialRecording;}

    private:
        // implementation for LLStopWatchControlsMixin
        /*virtual*/ void handleStart();
        /*virtual*/ void handleStop();
        /*virtual*/ void handleReset();
        /*virtual*/ void handleSplitTo(ExtendableRecording& other);

    private:
        Recording mAcceptedRecording;
        Recording mPotentialRecording;
    };

    class ExtendablePeriodicRecording
    :   public LLStopWatchControlsMixin<ExtendablePeriodicRecording>
    {
    public:
        ExtendablePeriodicRecording();
        void extend();

        PeriodicRecording& getResults()             { return mAcceptedRecording; }
        const PeriodicRecording& getResults() const {return mAcceptedRecording;}
        
        void nextPeriod() { mPotentialRecording.nextPeriod(); }

    private:
        // implementation for LLStopWatchControlsMixin
        /*virtual*/ void handleStart();
        /*virtual*/ void handleStop();
        /*virtual*/ void handleReset();
        /*virtual*/ void handleSplitTo(ExtendablePeriodicRecording& other);

    private:
        PeriodicRecording mAcceptedRecording;
        PeriodicRecording mPotentialRecording;
    };
}

#endif // LL_LLTRACERECORDING_H
