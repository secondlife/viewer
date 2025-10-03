/**
* @file llperfstats.h
* @brief Statistics collection to support autotune and perf flaoter.
*
* $LicenseInfo:firstyear=2022&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2022, Linden Research, Inc.
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
#pragma once
#ifndef LL_PERFSTATS_H_INCLUDED
#define LL_PERFSTATS_H_INCLUDED

#include <atomic>
#include <chrono>
#include <array>
#include <unordered_map>
#include <mutex>
#include "lluuid.h"
#include "llfasttimer.h"
#include "llapp.h"
#include "llprofiler.h"
#include "pipeline.h"

extern U32 gFrameCount;
extern LLUUID gAgentID;
namespace LLPerfStats
{

    // called once per main loop iteration
    void updateClass();

// Note if changing these, they should correspond with the log range of the correpsonding sliders
    static constexpr U64 ART_UNLIMITED_NANOS{50000000};
    static constexpr U64 ART_MINIMUM_NANOS{100000};
    static constexpr U64 ART_MIN_ADJUST_UP_NANOS{5000};
    static constexpr U64 ART_MIN_ADJUST_DOWN_NANOS{10000};

    static constexpr F32 PREFERRED_DD{180};
    static constexpr U32 SMOOTHING_PERIODS{50};
    static constexpr U32 DD_STEP{10};

    static constexpr U32 TUNE_AVATARS_ONLY{0};
    static constexpr U32 TUNE_SCENE_AND_AVATARS{1};
    static constexpr U32 TUNE_SCENE_ONLY{2};

    extern F64 cpu_hertz;

    extern std::atomic<int64_t> tunedAvatars;
    extern std::atomic<U64> renderAvatarMaxART_ns;
    extern bool belowTargetFPS;
    extern U32 lastGlobalPrefChange;
    extern U32 lastSleepedFrame;
    extern U64 meanFrameTime;
    extern std::mutex bufferToggleLock;

    enum class ObjType_t{
        OT_GENERAL=0, // Also Unknown. Used for n/a type stats such as scenery
        OT_COUNT
    };
    enum class StatType_t{
        RENDER_GEOMETRY=0,
        RENDER_SHADOWS,
        RENDER_HUDS,
        RENDER_UI,
        RENDER_COMBINED,
        RENDER_SWAP,
        RENDER_FRAME,
        RENDER_DISPLAY,
        RENDER_SLEEP,
        RENDER_LFS,
        RENDER_MESHREPO,
        //RENDER_FPSLIMIT,
        RENDER_FPS,
        RENDER_IDLE,
        RENDER_DONE, // toggle buffer & clearbuffer (see processUpdate for hackery)
        STATS_COUNT
    };

    struct StatsRecord
    {
        StatType_t  statType;
        ObjType_t   objType;
        LLUUID      avID;
        LLUUID      objID;
        uint64_t    time;
        bool        isRigged;
        bool        isHUD;
    };

    struct Tunables
    {
        static constexpr U32 Nothing{0};
        static constexpr U32 NonImpostors{1};
        static constexpr U32 ReflectionDetail{2};
        static constexpr U32 FarClip{4};
        static constexpr U32 UserMinDrawDistance{8};
        static constexpr U32 UserTargetDrawDistance{16};
        static constexpr U32 UserImpostorDistance{32};
        static constexpr U32 UserImpostorDistanceTuningEnabled{64};
        static constexpr U32 UserFPSTuningStrategy{128};
        static constexpr U32 UserAutoTuneEnabled{256};
        static constexpr U32 UserTargetFPS{512};
        static constexpr U32 UserARTCutoff{1024};
        static constexpr U32 UserAutoTuneLock{4096};

        U32 tuningFlag{0}; // bit mask for changed settings

        // proxy variables, used to pas the new value to be set via the mainthread
        U32 nonImpostors{0};
        S32 reflectionDetail{0};
        F32 farClip{0.0};
        F32 userMinDrawDistance{0.0};
        F32 userTargetDrawDistance{0.0};
        F32 userImpostorDistance{0.0};
        bool userImpostorDistanceTuningEnabled{false};
        U32 userFPSTuningStrategy{0};
        bool userAutoTuneEnabled{false};
        bool userAutoTuneLock{true};
        U32 userTargetFPS{0};
        F32 userARTCutoffSliderValue{0};
        S32 userTargetReflections{0};
        bool autoTuneTimeout{true};
        bool vsyncEnabled{true};

        void updateNonImposters(U32 nv){nonImpostors=nv; tuningFlag |= NonImpostors;};
        void updateReflectionDetail(S32 nv){reflectionDetail=nv; tuningFlag |= ReflectionDetail;};
        void updateFarClip(F32 nv){farClip=nv; tuningFlag |= FarClip;};
        void updateUserMinDrawDistance(F32 nv){userMinDrawDistance=nv; tuningFlag |= UserMinDrawDistance;};
        void updateUserTargetDrawDistance(F32 nv){userTargetDrawDistance=nv; tuningFlag |= UserTargetDrawDistance;};
        void updateImposterDistance(F32 nv){userImpostorDistance=nv; tuningFlag |= UserImpostorDistance;};
        void updateImposterDistanceTuningEnabled(bool nv){userImpostorDistanceTuningEnabled=nv; tuningFlag |= UserImpostorDistanceTuningEnabled;};
        void updateUserFPSTuningStrategy(U32 nv){userFPSTuningStrategy=nv; tuningFlag |= UserFPSTuningStrategy;};
        void updateTargetFps(U32 nv){userTargetFPS=nv; tuningFlag |= UserTargetFPS;};
        void updateUserARTCutoffSlider(F32 nv){userARTCutoffSliderValue=nv; tuningFlag |= UserARTCutoff;};
        void updateUserAutoTuneEnabled(bool nv){userAutoTuneEnabled=nv; tuningFlag |= UserAutoTuneEnabled;};
        void updateUserAutoTuneLock(bool nv){userAutoTuneLock=nv; tuningFlag |= UserAutoTuneLock;};

        void resetChanges(){tuningFlag=Nothing;};
        void initialiseFromSettings();
        void updateRenderCostLimitFromSettings();
        void updateSettingsFromRenderCostLimit();
        void applyUpdates();
    };

    extern Tunables tunables;

    class StatsRecorder{
    public:
        static inline StatsRecorder& getInstance()
        {
            static StatsRecorder instance;
            return instance;
        }
        static inline void setFocusAv(const LLUUID& avID){focusAv = avID;};
        static inline const LLUUID& getFocusAv(){return focusAv;};
        static inline void setAutotuneInit(){autotuneInit = true;};

        static inline void send(StatsRecord && upd)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            StatsRecorder::getInstance().processUpdate(upd);
        }

        static void endFrame()
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            StatsRecorder::getInstance().processUpdate(StatsRecord{StatType_t::RENDER_DONE, ObjType_t::OT_GENERAL, LLUUID::null, LLUUID::null, 0});
        }

        static void clearStats()
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            StatsRecorder::getInstance().processUpdate(StatsRecord{StatType_t::RENDER_DONE, ObjType_t::OT_GENERAL, LLUUID::null, LLUUID::null, 1});
        }

        static inline void setEnabled(bool on_or_off){collectionEnabled=on_or_off;};
        static inline void enable()     { collectionEnabled=true; };
        static inline void disable()    { collectionEnabled=false; };
        static inline bool enabled()    { return collectionEnabled; };

        static inline int getReadBufferIndex() { return (writeBuffer ^ 1); };
        // static inline const StatsTypeMatrix& getCurrentStatsMatrix(){ return statsDoubleBuffer[getReadBufferIndex()];}
        static inline uint64_t get(ObjType_t otype, LLUUID id, StatType_t type)
        {
            return statsDoubleBuffer[getReadBufferIndex()][static_cast<size_t>(otype)][id][static_cast<size_t>(type)];
        }
        static inline uint64_t getSceneStat(StatType_t type)
        {
            return statsDoubleBuffer[getReadBufferIndex()][static_cast<size_t>(ObjType_t::OT_GENERAL)][LLUUID::null][static_cast<size_t>(type)];
        }

        static inline uint64_t getSum(ObjType_t otype, StatType_t type)
        {
            return sum[getReadBufferIndex()][static_cast<size_t>(otype)][static_cast<size_t>(type)];
        }
        static inline uint64_t getMax(ObjType_t otype, StatType_t type)
        {
            return max[getReadBufferIndex()][static_cast<size_t>(otype)][static_cast<size_t>(type)];
        }
        static void updateAvatarParams();
    private:
        StatsRecorder();

        static int countNearbyAvatars(S32 distance);
        static U64 getMeanTotalFrameTime();
        static void updateMeanFrameTime(U64 tot_frame_time_raw);
// StatsArray is a uint64_t for each possible statistic type.
        using StatsArray    = std::array<uint64_t, static_cast<size_t>(LLPerfStats::StatType_t::STATS_COUNT)>;
        using StatsMap      = std::unordered_map<LLUUID, StatsArray, boost::hash<LLUUID>>;
        using StatsTypeMatrix = std::array<StatsMap, static_cast<size_t>(LLPerfStats::ObjType_t::OT_COUNT)>;
        using StatsSummaryArray = std::array<StatsArray, static_cast<size_t>(LLPerfStats::ObjType_t::OT_COUNT)>;

        static std::atomic<int> writeBuffer;
        static LLUUID focusAv;
        static bool autotuneInit;
        static std::array<StatsTypeMatrix,2> statsDoubleBuffer;
        static std::array<StatsSummaryArray,2> max;
        static std::array<StatsSummaryArray,2> sum;
        static bool collectionEnabled;


        void processUpdate(const StatsRecord& upd) const
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            // LL_INFOS("perfstats") << "processing update:" << LL_ENDL;
            // Note: nullptr is used as the key for global stats

            if (upd.statType == StatType_t::RENDER_DONE && upd.objType == ObjType_t::OT_GENERAL && upd.time == 0)
            {
                // LL_INFOS("perfstats") << "End of Frame Toggle Buffer:" << gFrameCount << LL_ENDL;
                toggleBuffer();
                return;
            }
            if (upd.statType == StatType_t::RENDER_DONE && upd.objType == ObjType_t::OT_GENERAL && upd.time == 1)
            {
                // LL_INFOS("perfstats") << "New region - clear buffers:" << gFrameCount << LL_ENDL;
                clearStatsBuffers();
                return;
            }

            auto ot{upd.objType};
            auto& key{upd.objID};
            auto type {upd.statType};
            auto val {upd.time};

            if (ot == ObjType_t::OT_GENERAL)
            {
                // LL_INFOS("perfstats") << "General update:" << LL_ENDL;
                doUpd(key, ot, type,val);
                return;
            }
        }

        static inline void doUpd(const LLUUID& key, ObjType_t ot, StatType_t type, uint64_t val)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
            using ST = StatType_t;
            StatsMap& stm {statsDoubleBuffer[writeBuffer][static_cast<size_t>(ot)]};
            auto& thisAsset = stm[key];

            thisAsset[static_cast<size_t>(type)] += val;
            thisAsset[static_cast<size_t>(ST::RENDER_COMBINED)] += val;

            sum[writeBuffer][static_cast<size_t>(ot)][static_cast<size_t>(type)] += val;
            sum[writeBuffer][static_cast<size_t>(ot)][static_cast<size_t>(ST::RENDER_COMBINED)] += val;

            if(max[writeBuffer][static_cast<size_t>(ot)][static_cast<size_t>(type)] < thisAsset[static_cast<size_t>(type)])
            {
                max[writeBuffer][static_cast<size_t>(ot)][static_cast<size_t>(type)] = thisAsset[static_cast<size_t>(type)];
            }
            if(max[writeBuffer][static_cast<size_t>(ot)][static_cast<size_t>(ST::RENDER_COMBINED)] < thisAsset[static_cast<size_t>(ST::RENDER_COMBINED)])
            {
                max[writeBuffer][static_cast<size_t>(ot)][static_cast<size_t>(ST::RENDER_COMBINED)] = thisAsset[static_cast<size_t>(ST::RENDER_COMBINED)];
            }
        }

        static void toggleBuffer();
        static void clearStatsBuffers();

        ~StatsRecorder() = default;
        StatsRecorder(const StatsRecorder&) = delete;
        StatsRecorder& operator=(const StatsRecorder&) = delete;

    };

    template <enum ObjType_t ObjTypeDiscriminator>
    class RecordTime
    {

    private:
        RecordTime(const RecordTime&) = delete;
        RecordTime() = delete;
        U64 start;
    public:
        StatsRecord stat;

        RecordTime( const LLUUID& av, const LLUUID& id, StatType_t type, bool isRiggedAtt=false, bool isHUDAtt=false):
                    start{LLTrace::BlockTimer::getCPUClockCount64()},
                    stat{type, ObjTypeDiscriminator, std::move(av), std::move(id), 0, isRiggedAtt, isHUDAtt}
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
        };

        template < ObjType_t OD = ObjTypeDiscriminator,
                   std::enable_if_t<OD == ObjType_t::OT_GENERAL> * = nullptr>
        explicit RecordTime( StatType_t type ):RecordTime<ObjTypeDiscriminator>(LLUUID::null, LLUUID::null, type )
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
        };

        ~RecordTime()
        {
            if(!LLPerfStats::StatsRecorder::enabled())
            {
                return;
            }

            stat.time = LLTrace::BlockTimer::getCPUClockCount64() - start;
            StatsRecorder::send(std::move(stat));
        };
    };


    inline double raw_to_ns(U64 raw)    { return (static_cast<double>(raw) * 1000000000.0) / LLPerfStats::cpu_hertz; };
    inline double raw_to_us(U64 raw)    { return (static_cast<double>(raw) *    1000000.0) / LLPerfStats::cpu_hertz; };
    inline double raw_to_ms(U64 raw)    { return (static_cast<double>(raw) *       1000.0) / LLPerfStats::cpu_hertz; };

    inline U64 ns_to_raw(double ns)     { return (U64)(LLPerfStats::cpu_hertz * (ns / 1000000000.0)); }
    inline U64 us_to_raw(double us)     { return (U64)(LLPerfStats::cpu_hertz * (us / 1000000.0)); }
    inline U64 ms_to_raw(double ms)     { return (U64)(LLPerfStats::cpu_hertz * (ms / 1000.0));

    }


    using RecordSceneTime = RecordTime<ObjType_t::OT_GENERAL>;

};// namespace LLPerfStats

#endif
