/**
 * @file llenvmanager.cpp
 * @brief Implementation of classes managing WindLight and water settings.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llenvironment.h"

#include <algorithm>

#include "llagent.h"
#include "llviewercontrol.h" // for gSavedSettings
#include "llviewerregion.h"
#include "llwlhandlers.h"
#include "lltrans.h"
#include "lltrace.h"
#include "llfasttimer.h"
#include "llviewercamera.h"
#include "pipeline.h"
#include "llsky.h"

#include "llviewershadermgr.h"

#include "llparcel.h"
#include "llviewerparcelmgr.h"

#include "llsdserialize.h"
#include "lldiriterator.h"

#include "llsettingsvo.h"
#include "llnotificationsutil.h"

#include "llregioninfomodel.h"

#include <boost/make_shared.hpp>

#include "llatmosphere.h"
#include "llagent.h"
#include "roles_constants.h"
#include "llestateinfomodel.h"

#include "lldispatcher.h"
#include "llviewergenericmessage.h"
#include "llexperiencelog.h"

//=========================================================================
namespace
{
    const std::string KEY_ENVIRONMENT("environment");
    const std::string KEY_DAYASSET("day_asset");
    const std::string KEY_DAYCYCLE("day_cycle");
    const std::string KEY_DAYHASH("day_hash");
    const std::string KEY_DAYLENGTH("day_length");
    const std::string KEY_DAYNAME("day_name");
    const std::string KEY_DAYNAMES("day_names");
    const std::string KEY_DAYOFFSET("day_offset");
    const std::string KEY_ENVVERSION("env_version");
    const std::string KEY_ISDEFAULT("is_default");
    const std::string KEY_PARCELID("parcel_id");
    const std::string KEY_REGIONID("region_id");
    const std::string KEY_TRACKALTS("track_altitudes");
    const std::string KEY_FLAGS("flags");

    const std::string MESSAGE_PUSHENVIRONMENT("PushExpEnvironment");

    const std::string ACTION_CLEARENVIRONMENT("ClearEnvironment");
    const std::string ACTION_PUSHFULLENVIRONMENT("PushFullEnvironment");
    const std::string ACTION_PUSHPARTIALENVIRONMENT("PushPartialEnvironment");

    const std::string KEY_ASSETID("asset_id");
    const std::string KEY_TRANSITIONTIME("transition_time");
    const std::string KEY_ACTION("action");
    const std::string KEY_ACTIONDATA("action_data");
    const std::string KEY_EXPERIENCEID("public_id");
    const std::string KEY_OBJECTNAME("ObjectName");     // some of these do not conform to the '_' format.  
    const std::string KEY_PARCELNAME("ParcelName");     // But changing these would also alter the Experience Log requirements.
    const std::string KEY_COUNT("Count");

    const std::string LISTENER_NAME("LLEnvironmentSingleton");
    const std::string PUMP_EXPERIENCE("experience_permission");

    const std::string LOCAL_ENV_STORAGE_FILE("local_environment_data.bin");

    //---------------------------------------------------------------------
    LLTrace::BlockTimerStatHandle   FTM_ENVIRONMENT_UPDATE("Update Environment Tick");
    LLTrace::BlockTimerStatHandle   FTM_SHADER_PARAM_UPDATE("Update Shader Parameters");

    LLSettingsBase::Seconds         DEFAULT_UPDATE_THRESHOLD(10.0);
    const LLSettingsBase::Seconds   MINIMUM_SPANLENGTH(0.01f);

    //---------------------------------------------------------------------
    inline LLSettingsBase::TrackPosition get_wrapping_distance(LLSettingsBase::TrackPosition begin, LLSettingsBase::TrackPosition end)
    {
        if (begin < end)
        {
            return end - begin;
        }
        else if (begin > end)
        {
            return LLSettingsBase::TrackPosition(1.0) - (begin - end);
        }

        return 1.0f;
    }

    LLSettingsDay::CycleTrack_t::iterator get_wrapping_atafter(LLSettingsDay::CycleTrack_t &collection, const LLSettingsBase::TrackPosition& key)
    {
        if (collection.empty())
            return collection.end();

        LLSettingsDay::CycleTrack_t::iterator it = collection.upper_bound(key);

        if (it == collection.end())
        {   // wrap around
            it = collection.begin();
        }

        return it;
    }

    LLSettingsDay::CycleTrack_t::iterator get_wrapping_atbefore(LLSettingsDay::CycleTrack_t &collection, const LLSettingsBase::TrackPosition& key)
    {
        if (collection.empty())
            return collection.end();

        LLSettingsDay::CycleTrack_t::iterator it = collection.lower_bound(key);

        if (it == collection.end())
        {   // all keyframes are lower, take the last one.
            --it; // we know the range is not empty
        }
        else if ((*it).first > key)
        {   // the keyframe we are interested in is smaller than the found.
            if (it == collection.begin())
                it = collection.end();
            --it;
        }

        return it;
    }

    LLSettingsDay::TrackBound_t get_bounding_entries(LLSettingsDay::CycleTrack_t &track, const LLSettingsBase::TrackPosition& keyframe)
    {
        return LLSettingsDay::TrackBound_t(get_wrapping_atbefore(track, keyframe), get_wrapping_atafter(track, keyframe));
    }

    // Find normalized track position of given time along full length of cycle
    inline LLSettingsBase::TrackPosition convert_time_to_position(const LLSettingsBase::Seconds& time, const LLSettingsBase::Seconds& len)
    {
        LLSettingsBase::TrackPosition position = LLSettingsBase::TrackPosition(fmod((F64)time, (F64)len) / (F64)len);
        return llclamp(position, 0.0f, 1.0f);
    }

    inline LLSettingsBase::BlendFactor convert_time_to_blend_factor(const LLSettingsBase::Seconds& time, const LLSettingsBase::Seconds& len, LLSettingsDay::CycleTrack_t &track)
    {
        LLSettingsBase::TrackPosition position = convert_time_to_position(time, len);
        LLSettingsDay::TrackBound_t bounds(get_bounding_entries(track, position));

        LLSettingsBase::TrackPosition spanlength(get_wrapping_distance((*bounds.first).first, (*bounds.second).first));
        if (position < (*bounds.first).first)
            position += 1.0;

        LLSettingsBase::TrackPosition start = position - (*bounds.first).first;

        return static_cast<LLSettingsBase::BlendFactor>(start / spanlength);
    }

    //---------------------------------------------------------------------
    class LLTrackBlenderLoopingTime : public LLSettingsBlenderTimeDelta
    {
    public:
        LLTrackBlenderLoopingTime(const LLSettingsBase::ptr_t &target, const LLSettingsDay::ptr_t &day, S32 trackno,
            LLSettingsBase::Seconds cyclelength, LLSettingsBase::Seconds cycleoffset, LLSettingsBase::Seconds updateThreshold) :
            LLSettingsBlenderTimeDelta(target, LLSettingsBase::ptr_t(), LLSettingsBase::ptr_t(), LLSettingsBase::Seconds(1.0)),
            mDay(day),
            mTrackNo(0),
            mCycleLength(cyclelength),
            mCycleOffset(cycleoffset)
        {
            // must happen prior to getBoundingEntries call...
            mTrackNo = selectTrackNumber(trackno);

            LLSettingsBase::Seconds now(getAdjustedNow());
            LLSettingsDay::TrackBound_t initial = getBoundingEntries(now);

            mInitial = (*initial.first).second;
            mFinal = (*initial.second).second;
            mBlendSpan = getSpanTime(initial);

            initializeTarget(now);
            setOnFinished([this](const LLSettingsBlender::ptr_t &){ onFinishedSpan(); });
        }

        void switchTrack(S32 trackno, const LLSettingsBase::TrackPosition&) override
        {
            S32 use_trackno = selectTrackNumber(trackno);

            if (use_trackno == mTrackNo)
            {   // results in no change
                return;
            }

            LLSettingsBase::ptr_t pstartsetting = mTarget->buildDerivedClone();
            mTrackNo = use_trackno;

            LLSettingsBase::Seconds now = getAdjustedNow() + LLEnvironment::TRANSITION_ALTITUDE;
            LLSettingsDay::TrackBound_t bounds = getBoundingEntries(now);

            LLSettingsBase::ptr_t pendsetting = (*bounds.first).second->buildDerivedClone();
            LLSettingsBase::TrackPosition targetpos = convert_time_to_position(now, mCycleLength) - (*bounds.first).first;
            LLSettingsBase::TrackPosition targetspan = get_wrapping_distance((*bounds.first).first, (*bounds.second).first);

            LLSettingsBase::BlendFactor blendf = calculateBlend(targetpos, targetspan);
            pendsetting->blend((*bounds.second).second, blendf);

            reset(pstartsetting, pendsetting, LLEnvironment::TRANSITION_ALTITUDE);
        }

    protected:
        S32 selectTrackNumber(S32 trackno)
        {
            if (trackno == 0)
            {   // We are dealing with the water track.  There is only ever one.
                return trackno;
            }

            for (S32 test = trackno; test != 0; --test)
            {   // Find the track below the requested one with data.
                LLSettingsDay::CycleTrack_t &track = mDay->getCycleTrack(test);

                if (!track.empty())
                    return test;
            }

            return 1;
        }

        LLSettingsDay::TrackBound_t getBoundingEntries(LLSettingsBase::Seconds time)
        {
            LLSettingsDay::CycleTrack_t &wtrack = mDay->getCycleTrack(mTrackNo);
            LLSettingsBase::TrackPosition position = convert_time_to_position(time, mCycleLength);
            LLSettingsDay::TrackBound_t bounds = get_bounding_entries(wtrack, position);
            return bounds;
        }

        void initializeTarget(LLSettingsBase::Seconds time)
        {
            LLSettingsBase::BlendFactor blendf(convert_time_to_blend_factor(time, mCycleLength, mDay->getCycleTrack(mTrackNo)));

            blendf = llclamp(blendf, 0.0, 0.999);
            setTimeSpent(LLSettingsBase::Seconds(blendf * mBlendSpan));

            setBlendFactor(blendf);
        }

        LLSettingsBase::Seconds getAdjustedNow() const
        {
            LLSettingsBase::Seconds now(LLDate::now().secondsSinceEpoch());

            return (now + mCycleOffset);
        }

        LLSettingsBase::Seconds getSpanTime(const LLSettingsDay::TrackBound_t &bounds) const
        {
            LLSettingsBase::Seconds span = mCycleLength * get_wrapping_distance((*bounds.first).first, (*bounds.second).first);
            if (span < MINIMUM_SPANLENGTH) // for very short spans set a minimum length.
                span = MINIMUM_SPANLENGTH;
            return span;
        }

    private:
        LLSettingsDay::ptr_t        mDay;
        S32                         mTrackNo;
        LLSettingsBase::Seconds     mCycleLength;
        LLSettingsBase::Seconds     mCycleOffset;

        void onFinishedSpan()
        {
            LLSettingsBase::Seconds adjusted_now = getAdjustedNow();
            LLSettingsDay::TrackBound_t next = getBoundingEntries(adjusted_now);
            LLSettingsBase::Seconds nextspan = getSpanTime(next);

            reset((*next.first).second, (*next.second).second, nextspan);

            // Recalculate (reinitialize) position. Because:
            // - 'delta' from applyTimeDelta accumulates errors (probably should be fixed/changed to absolute time)
            // - freezes and lag can result in reset being called too late, so we need to add missed time
            // - occasional time corrections can happen
            // - some transition switches can happen outside applyTimeDelta thus causing 'desync' from 'delta' (can be fixed by getting rid of delta)
            initializeTarget(adjusted_now);
        }
    };

    class LLEnvironmentPushDispatchHandler : public LLDispatchHandler
    {
    public:
        virtual bool operator()(const LLDispatcher *, const std::string& key, const LLUUID& invoice, const sparam_t& strings) override
        {
            LLSD message;
            sparam_t::const_iterator it = strings.begin();

            if (it != strings.end())
            {
                const std::string& llsdRaw = *it++;
                std::istringstream llsdData(llsdRaw);
                if (!LLSDSerialize::deserialize(message, llsdData, llsdRaw.length()))
                {
                    LL_WARNS() << "LLExperienceLogDispatchHandler: Attempted to read parameter data into LLSD but failed:" << llsdRaw << LL_ENDL;
                }
            }

            message[KEY_EXPERIENCEID] = invoice;
            // Object Name
            if (it != strings.end())
            {
                message[KEY_OBJECTNAME] = *it++;
            }

            // parcel Name
            if (it != strings.end())
            {
                message[KEY_PARCELNAME] = *it++;
            }
            message[KEY_COUNT] = 1;

            LLEnvironment::instance().handleEnvironmentPush(message);
            return true;
        }
    };

    LLEnvironmentPushDispatchHandler environment_push_dispatch_handler;

    template<class SETTINGT>
    class LLSettingsInjected : public SETTINGT
    {
    public:
        typedef std::shared_ptr<LLSettingsInjected<SETTINGT> >  ptr_t;

        LLSettingsInjected(typename SETTINGT::ptr_t source) :
            SETTINGT(),
            mSource(source),
            mLastSourceHash(0),
            mLastHash(0)
        {}

        virtual ~LLSettingsInjected() {};

        typename SETTINGT::ptr_t getSource() const                    { return this->mSource; }
        void setSource(const typename SETTINGT::ptr_t &source)        
        {
            if (source.get() == this) // do not set a source to itself.
                return;
            this->mSource = source; 
            this->setDirtyFlag(true); 
            this->mLastSourceHash = 0; 
        }

        virtual bool isDirty() const override { return SETTINGT::isDirty() || (this->mSource->isDirty()); }
        virtual bool isVeryDirty() const override { return SETTINGT::isVeryDirty() || (this->mSource->isVeryDirty()); }

        void injectSetting(const std::string keyname, LLSD value, LLUUID experience_id, F32Seconds transition)
        {
            if (transition > 0.1)
            {
                typename Injection::ptr_t injection = std::make_shared<Injection>(transition, keyname, value, true, experience_id);

                mInjections.push_back(injection);
                std::stable_sort(mInjections.begin(), mInjections.end(), [](const typename Injection::ptr_t &a, const typename Injection::ptr_t &b) { return a->mTimeRemaining < b->mTimeRemaining; });
            }
            else
            {
                mOverrideValues[keyname] = value;
                mOverrideExps[keyname] = experience_id;
                this->setDirtyFlag(true);
            }
        }

        void removeInjection(const std::string keyname, LLUUID experience, LLSettingsBase::Seconds transition)
        {
            injections_t injections_buf;
            for (auto it = mInjections.begin(); it != mInjections.end(); it++)
            {
                if ((keyname.empty() || ((*it)->mKeyName == keyname)) &&
                    (experience.isNull() || (experience == (*it)->mExperience)))
                {
                    if (transition != LLEnvironment::TRANSITION_INSTANT)
                    {
                        typename Injection::ptr_t injection = std::make_shared<Injection>(transition, keyname, (*it)->mLastValue, false, LLUUID::null);
                        injections_buf.push_front(injection);
                    }
                }
                else
                {
                    injections_buf.push_front(*it);
                }
            }
            mInjections.clear();
            mInjections = injections_buf;

            for (auto itexp = mOverrideExps.begin(); itexp != mOverrideExps.end();)
            {
                if (experience.isNull() || ((*itexp).second == experience))
                {
                    if (transition != LLEnvironment::TRANSITION_INSTANT)
                    {
                        typename Injection::ptr_t injection = std::make_shared<Injection>(transition, (*itexp).first, mOverrideValues[(*itexp).first], false, LLUUID::null);
                        mInjections.push_front(injection);
                    }
                    mOverrideValues.erase((*itexp).first);
                    mOverrideExps.erase(itexp++);
                }
                else
                    ++itexp;
            }
            std::stable_sort(mInjections.begin(), mInjections.end(), [](const typename Injection::ptr_t &a, const typename Injection::ptr_t &b) { return a->mTimeRemaining < b->mTimeRemaining; });
        }

        void removeInjections(LLUUID experience_id, LLSettingsBase::Seconds transition)
        {
            removeInjection(std::string(), experience_id, transition);
        }

        void injectExperienceValues(LLSD values, LLUUID experience_id, typename LLSettingsBase::Seconds transition)
        {
            for (auto it = values.beginMap(); it != values.endMap(); ++it)
            {
                injectSetting((*it).first, (*it).second, experience_id, transition);
            }
            this->setDirtyFlag(true);
        }

        void applyInjections(LLSettingsBase::Seconds delta)
        {
            this->mSettings = this->mSource->getSettings();

            for (auto ito = mOverrideValues.beginMap(); ito != mOverrideValues.endMap(); ++ito)
            {
                this->mSettings[(*ito).first] = (*ito).second;
            }

            const LLSettingsBase::stringset_t &slerps = this->getSlerpKeys();
            const LLSettingsBase::stringset_t &skips = this->getSkipInterpolateKeys();
            const LLSettingsBase::stringset_t &specials = this->getSpecialKeys();

            typename injections_t::iterator it;
            for (it = mInjections.begin(); it != mInjections.end(); ++it)
            {
                std::string key_name = (*it)->mKeyName;

                LLSD value = this->mSettings[key_name];
                LLSD target = (*it)->mValue;

                if ((*it)->mFirstTime)
                    (*it)->mFirstTime = false;
                else
                    (*it)->mTimeRemaining -= delta;

                typename LLSettingsBase::BlendFactor mix = 1.0f - ((*it)->mTimeRemaining.value() / (*it)->mTransition.value());

                if (mix >= 1.0)
                {
                    if ((*it)->mBlendIn)
                    {
                        mOverrideValues[key_name] = target;
                        mOverrideExps[key_name] = (*it)->mExperience;
                        this->mSettings[key_name] = target;
                    }
                    else
                    {
                        this->mSettings.erase(key_name);
                    }
                }
                else if (specials.find(key_name) != specials.end())
                {
                    updateSpecial(*it, mix);
                }
                else if (skips.find(key_name) == skips.end())
                {
                    if (!(*it)->mBlendIn)
                        mix = 1.0 - mix;
                    (*it)->mLastValue = this->interpolateSDValue(key_name, value, target, this->getParameterMap(), mix, slerps);
                    this->mSettings[key_name] = (*it)->mLastValue;
                }
            }

            size_t hash = this->getHash();

            if (hash != mLastHash)
            {
                this->setDirtyFlag(true);
                mLastHash = hash;
            }

            it = mInjections.begin();
            it = std::find_if(mInjections.begin(), mInjections.end(), [](const typename Injection::ptr_t &a) { return a->mTimeRemaining > 0.0f; });

            if (it != mInjections.begin())
            {
                mInjections.erase(mInjections.begin(), mInjections.end());
            }

        }

        bool hasInjections() const
        {
            return (!mInjections.empty() || (mOverrideValues.size() > 0));
        }

    protected:
        struct Injection
        {
            Injection(typename LLSettingsBase::Seconds transition, const std::string &keyname, LLSD value, bool blendin, LLUUID experince, S32 index = -1) :
                mTransition(transition),
                mTimeRemaining(transition),
                mKeyName(keyname),
                mValue(value),
                mExperience(experince),
                mIndex(index),
                mBlendIn(blendin),
                mFirstTime(true)
            {}

            typename LLSettingsBase::Seconds    mTransition;
            typename LLSettingsBase::Seconds    mTimeRemaining;
            std::string                         mKeyName;
            LLSD                                mValue;
            LLSD                                mLastValue;
            LLUUID                              mExperience;
            S32                                 mIndex;
            bool                                mBlendIn;
            bool                                mFirstTime;

            typedef std::shared_ptr<Injection>  ptr_t;
        };


        virtual void updateSettings() override
        {
            static LLFrameTimer timer;

            if (!this->mSource)
                return;

            // clears the dirty flag on this object.  Need to prevent triggering 
            // more calls into this updateSettings
            LLSettingsBase::updateSettings();

            resetSpecial();

            if (this->mSource->isDirty())
            {
                this->mSource->updateSettings();
            }

            typename LLSettingsBase::Seconds delta(timer.getElapsedTimeAndResetF32());


            SETTINGT::updateSettings();

            if (!mInjections.empty())
                this->setDirtyFlag(true);
        }

        LLSettingsBase::stringset_t getSpecialKeys() const;
        void                        resetSpecial();
        void                        updateSpecial(const typename Injection::ptr_t &injection, typename LLSettingsBase::BlendFactor mix);

    private:
        typedef std::map<std::string, LLUUID>   key_to_expid_t;
        typedef std::deque<typename Injection::ptr_t>    injections_t;

        size_t                      mLastSourceHash;
        size_t                      mLastHash;
        typename SETTINGT::ptr_t    mSource;
        injections_t                mInjections;
        LLSD                        mOverrideValues;
        key_to_expid_t              mOverrideExps;
    };

    template<>
    LLSettingsBase::stringset_t LLSettingsInjected<LLSettingsVOSky>::getSpecialKeys() const
    {
        static LLSettingsBase::stringset_t specialSet;

        if (specialSet.empty())
        {
            specialSet.insert(SETTING_BLOOM_TEXTUREID);
            specialSet.insert(SETTING_RAINBOW_TEXTUREID);
            specialSet.insert(SETTING_HALO_TEXTUREID);
            specialSet.insert(SETTING_CLOUD_TEXTUREID);
            specialSet.insert(SETTING_MOON_TEXTUREID);
            specialSet.insert(SETTING_SUN_TEXTUREID);
        }
        return specialSet;
    }

    template<>
    LLSettingsBase::stringset_t LLSettingsInjected<LLSettingsVOWater>::getSpecialKeys() const
    {
        static stringset_t specialSet;

        if (specialSet.empty())
        {
            specialSet.insert(SETTING_TRANSPARENT_TEXTURE);
            specialSet.insert(SETTING_NORMAL_MAP);
        }
        return specialSet;
    }

    template<>
    void LLSettingsInjected<LLSettingsVOSky>::resetSpecial()
    {
        mNextSunTextureId.setNull();
        mNextMoonTextureId.setNull();
        mNextCloudTextureId.setNull();
        mNextBloomTextureId.setNull();
        mNextRainbowTextureId.setNull();
        mNextHaloTextureId.setNull();
        setBlendFactor(0.0f);
    }

    template<>
    void LLSettingsInjected<LLSettingsVOWater>::resetSpecial()
    {
        mNextNormalMapID.setNull();
        mNextTransparentTextureID.setNull();
        setBlendFactor(0.0f);
    }

    template<>
    void LLSettingsInjected<LLSettingsVOSky>::updateSpecial(const typename LLSettingsInjected<LLSettingsVOSky>::Injection::ptr_t &injection, typename LLSettingsBase::BlendFactor mix)
    {
        if (injection->mKeyName == SETTING_SUN_TEXTUREID)
        {
            mNextSunTextureId = injection->mValue.asUUID();
        }
        else if (injection->mKeyName == SETTING_MOON_TEXTUREID)
        {
            mNextMoonTextureId = injection->mValue.asUUID();
        }
        else if (injection->mKeyName == SETTING_CLOUD_TEXTUREID)
        {
            mNextCloudTextureId = injection->mValue.asUUID();
        }
        else if (injection->mKeyName == SETTING_BLOOM_TEXTUREID)
        {
            mNextBloomTextureId = injection->mValue.asUUID();
        }
        else if (injection->mKeyName == SETTING_RAINBOW_TEXTUREID)
        {
            mNextRainbowTextureId = injection->mValue.asUUID();
        }
        else if (injection->mKeyName == SETTING_HALO_TEXTUREID)
        {
            mNextHaloTextureId = injection->mValue.asUUID();
        }

        // Unfortunately I don't have a per texture blend factor.  We'll just pick the one that is furthest along.
        if (getBlendFactor() < mix)
        {
            setBlendFactor(mix);
        }
    }

    template<>
    void LLSettingsInjected<LLSettingsVOWater>::updateSpecial(const typename LLSettingsInjected<LLSettingsVOWater>::Injection::ptr_t &injection, typename LLSettingsBase::BlendFactor mix)
    {
        if (injection->mKeyName == SETTING_NORMAL_MAP)
        {
            mNextNormalMapID = injection->mValue.asUUID();
        }
        else if (injection->mKeyName == SETTING_TRANSPARENT_TEXTURE)
        {
            mNextTransparentTextureID = injection->mValue.asUUID();
        }

        // Unfortunately I don't have a per texture blend factor.  We'll just pick the one that is furthest along.
        if (getBlendFactor() < mix)
        {
            setBlendFactor(mix);
        }
    }

    typedef LLSettingsInjected<LLSettingsVOSky>   LLSettingsInjectedSky;
    typedef LLSettingsInjected<LLSettingsVOWater> LLSettingsInjectedWater;

    //=====================================================================
    class DayInjection : public LLEnvironment::DayInstance
    {
        friend class InjectedTransition;

    public:
        typedef std::shared_ptr<DayInjection> ptr_t;
        typedef std::weak_ptr<DayInjection> wptr_t;

                                            DayInjection(LLEnvironment::EnvSelection_t env);
        virtual                             ~DayInjection();

        virtual bool                        applyTimeDelta(const LLSettingsBase::Seconds& delta) override;

        void                                setInjectedDay(const LLSettingsDay::ptr_t &pday, LLUUID experience_id, LLSettingsBase::Seconds transition);
        void                                setInjectedSky(const LLSettingsSky::ptr_t &psky, LLUUID experience_id, LLSettingsBase::Seconds transition);
        void                                setInjectedWater(const LLSettingsWater::ptr_t &pwater, LLUUID experience_id, LLSettingsBase::Seconds transition);

        void                                injectSkySettings(LLSD settings, LLUUID experience_id, LLSettingsBase::Seconds transition);
        void                                injectWaterSettings(LLSD settings, LLUUID experience_id, LLSettingsBase::Seconds transition);

        void                                clearInjections(LLUUID experience_id, LLSettingsBase::Seconds transition_time);

        virtual void                        animate() override;

        LLEnvironment::DayInstance::ptr_t   getBaseDayInstance() const  { return mBaseDayInstance; }
        void                                setBaseDayInstance(const LLEnvironment::DayInstance::ptr_t &baseday);

        S32                                 countExperiencesActive() const { return mActiveExperiences.size(); }

        bool                                isOverriddenSky() const { return !mSkyExperience.isNull(); }
        bool                                isOverriddenWater() const { return !mWaterExperience.isNull(); }

        bool                                hasInjections() const;

        void                                testExperiencesOnParcel(S32 parcel_id);
    private:
        static void                         testExperiencesOnParcelCoro(wptr_t that, S32 parcel_id);


        void                                animateSkyChange(LLSettingsSky::ptr_t psky, LLSettingsBase::Seconds transition);
        void                                animateWaterChange(LLSettingsWater::ptr_t pwater, LLSettingsBase::Seconds transition);

        void                                onEnvironmentChanged(LLEnvironment::EnvSelection_t env);
        void                                onParcelChange();

        void                                checkExperience();


        LLEnvironment::DayInstance::ptr_t   mBaseDayInstance;

        LLSettingsInjectedSky::ptr_t        mInjectedSky;
        LLSettingsInjectedWater::ptr_t      mInjectedWater;
        std::set<LLUUID>                    mActiveExperiences;
        LLUUID                              mDayExperience;
        LLUUID                              mSkyExperience;
        LLUUID                              mWaterExperience;
        LLEnvironment::connection_t         mEnvChangeConnection;
        boost::signals2::connection         mParcelChangeConnection;
    };

    class InjectedTransition : public LLEnvironment::DayTransition
    {
    public:
                                    InjectedTransition(const DayInjection::ptr_t &injection, const LLSettingsSky::ptr_t &skystart, const LLSettingsWater::ptr_t &waterstart, DayInstance::ptr_t &end, LLSettingsDay::Seconds time):
                                        LLEnvironment::DayTransition(skystart, waterstart, end, time),
                                        mInjection(injection)
                                    { }
        virtual                     ~InjectedTransition() { };

        virtual void                animate() override;

    protected:
        DayInjection::ptr_t         mInjection;
    };

}

//=========================================================================
const F32Seconds LLEnvironment::TRANSITION_INSTANT(0.0f);
const F32Seconds LLEnvironment::TRANSITION_FAST(1.0f);
const F32Seconds LLEnvironment::TRANSITION_DEFAULT(5.0f);
const F32Seconds LLEnvironment::TRANSITION_SLOW(10.0f);
const F32Seconds LLEnvironment::TRANSITION_ALTITUDE(5.0f);

const LLUUID LLEnvironment::KNOWN_SKY_SUNRISE("01e41537-ff51-2f1f-8ef7-17e4df760bfb");
const LLUUID LLEnvironment::KNOWN_SKY_MIDDAY("6c83e853-e7f8-cad7-8ee6-5f31c453721c");
const LLUUID LLEnvironment::KNOWN_SKY_SUNSET("084e26cd-a900-28e8-08d0-64a9de5c15e2");
const LLUUID LLEnvironment::KNOWN_SKY_MIDNIGHT("8a01b97a-cb20-c1ea-ac63-f7ea84ad0090");

const S32 LLEnvironment::NO_TRACK(-1);
const S32 LLEnvironment::NO_VERSION(-3); // For viewer sided change, like ENV_LOCAL. -3 since -1 and -2 are taken by parcel initial server/viewer version
const S32 LLEnvironment::VERSION_CLEANUP(-4); // for cleanups

const F32 LLEnvironment::SUN_DELTA_YAW(F_PI);   // 180deg 


const U32 LLEnvironment::DayInstance::NO_ANIMATE_SKY(0x01);
const U32 LLEnvironment::DayInstance::NO_ANIMATE_WATER(0x02);

std::string env_selection_to_string(LLEnvironment::EnvSelection_t sel)
{
#define RTNENUM(E) case LLEnvironment::E: return #E
    switch (sel){
        RTNENUM(ENV_EDIT);
        RTNENUM(ENV_LOCAL);
        RTNENUM(ENV_PUSH);
        RTNENUM(ENV_PARCEL);
        RTNENUM(ENV_REGION);
        RTNENUM(ENV_DEFAULT);
        RTNENUM(ENV_END);
        RTNENUM(ENV_CURRENT);
        RTNENUM(ENV_NONE);
    default:
        return llformat("Unknown(%d)", sel);
    }
#undef RTNENUM
}


//-------------------------------------------------------------------------
LLEnvironment::LLEnvironment():
    mCloudScrollDelta(),
    mCloudScrollPaused(false),
    mSelectedSky(),
    mSelectedWater(),
    mSelectedDay(),
    mSelectedEnvironment(LLEnvironment::ENV_LOCAL),
    mCurrentTrack(1),
    mEditorCounter(0),
    mShowSunBeacon(false),
    mShowMoonBeacon(false)
{
}

void LLEnvironment::initSingleton()
{
    LLSettingsSky::ptr_t p_default_sky = LLSettingsVOSky::buildDefaultSky();
    LLSettingsWater::ptr_t p_default_water = LLSettingsVOWater::buildDefaultWater();

    mCurrentEnvironment = std::make_shared<DayInstance>(ENV_DEFAULT);
    mCurrentEnvironment->setSky(p_default_sky);
    mCurrentEnvironment->setWater(p_default_water);

    mEnvironments[ENV_DEFAULT] = mCurrentEnvironment;

    requestRegion();

    gAgent.addParcelChangedCallback([this]() { onParcelChange(); });

    //TODO: This frequently results in one more request than we need.  It isn't breaking, but should be nicer.
    // We need to know new env version to fix this, without it we can only do full re-request
    // Happens: on updates, on opening LLFloaterRegionInfo, on region crossing if info floater is open
    LLRegionInfoModel::instance().setUpdateCallback([this]() { requestRegion(); });
    gAgent.addRegionChangedCallback([this]() { onRegionChange(); });

    gAgent.whenPositionChanged([this](const LLVector3 &localpos, const LLVector3d &) { onAgentPositionHasChanged(localpos); });

    if (!gGenericDispatcher.isHandlerPresent(MESSAGE_PUSHENVIRONMENT))
    {
        gGenericDispatcher.addHandler(MESSAGE_PUSHENVIRONMENT, &environment_push_dispatch_handler);
    }

    LLEventPumps::instance().obtain(PUMP_EXPERIENCE).listen(LISTENER_NAME, [this](LLSD message) { listenExperiencePump(message); return false; });
}

void LLEnvironment::cleanupSingleton()
{
    LLEventPumps::instance().obtain(PUMP_EXPERIENCE).stopListening(LISTENER_NAME);
}

LLEnvironment::~LLEnvironment()
{
}

bool LLEnvironment::canEdit() const
{
    return true;
}

LLSettingsSky::ptr_t LLEnvironment::getCurrentSky() const 
{ 
    LLSettingsSky::ptr_t psky = mCurrentEnvironment->getSky(); 

    if (!psky && mCurrentEnvironment->getEnvironmentSelection() >= ENV_EDIT)
    {
        for (int idx = 0; idx < ENV_END; ++idx)
        {
            if (mEnvironments[idx]->getSky())
            {
                psky = mEnvironments[idx]->getSky();
                break;
            }
        }
    }
    return psky;
}

LLSettingsWater::ptr_t LLEnvironment::getCurrentWater() const 
{
    LLSettingsWater::ptr_t pwater = mCurrentEnvironment->getWater(); 

    if (!pwater && mCurrentEnvironment->getEnvironmentSelection() >= ENV_EDIT)
    {
        for (int idx = 0; idx < ENV_END; ++idx)
        {
            if (mEnvironments[idx]->getWater())
            {
                pwater = mEnvironments[idx]->getWater();
                break;
            }
        }
    }
    return pwater;
}

void LayerConfigToDensityLayer(const LLSD& layerConfig, DensityLayer& layerOut)
{
    layerOut.constant_term  = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM].asReal();
    layerOut.exp_scale      = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR].asReal();
    layerOut.exp_term       = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
    layerOut.linear_term    = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM].asReal();
    layerOut.width          = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH].asReal();
}

void LLEnvironment::getAtmosphericModelSettings(AtmosphericModelSettings& settingsOut, const LLSettingsSky::ptr_t &psky)
{
    settingsOut.m_skyBottomRadius   = psky->getSkyBottomRadius();
    settingsOut.m_skyTopRadius      = psky->getSkyTopRadius();
    settingsOut.m_sunArcRadians     = psky->getSunArcRadians();
    settingsOut.m_mieAnisotropy     = psky->getMieAnisotropy();

    LLSD rayleigh = psky->getRayleighConfigs();
    settingsOut.m_rayleighProfile.clear();
    for (LLSD::array_iterator itf = rayleigh.beginArray(); itf != rayleigh.endArray(); ++itf)
    {
        DensityLayer layer;
        LLSD& layerConfig = (*itf);
        LayerConfigToDensityLayer(layerConfig, layer);
        settingsOut.m_rayleighProfile.push_back(layer);
    }

    LLSD mie = psky->getMieConfigs();
    settingsOut.m_mieProfile.clear();
    for (LLSD::array_iterator itf = mie.beginArray(); itf != mie.endArray(); ++itf)
    {
        DensityLayer layer;
        LLSD& layerConfig = (*itf);
        LayerConfigToDensityLayer(layerConfig, layer);
        settingsOut.m_mieProfile.push_back(layer);
    }
    settingsOut.m_mieAnisotropy = psky->getMieAnisotropy();

    LLSD absorption = psky->getAbsorptionConfigs();
    settingsOut.m_absorptionProfile.clear();
    for (LLSD::array_iterator itf = absorption.beginArray(); itf != absorption.endArray(); ++itf)
    {
        DensityLayer layer;
        LLSD& layerConfig = (*itf);
        LayerConfigToDensityLayer(layerConfig, layer);
        settingsOut.m_absorptionProfile.push_back(layer);
    }
}

bool LLEnvironment::canAgentUpdateParcelEnvironment() const
{
    LLParcel *parcel(LLViewerParcelMgr::instance().getAgentOrSelectedParcel());

    return canAgentUpdateParcelEnvironment(parcel);
}


bool LLEnvironment::canAgentUpdateParcelEnvironment(LLParcel *parcel) const
{
    if (!parcel)
        return false;

    if (!LLEnvironment::instance().isExtendedEnvironmentEnabled())
        return false;

    if (gAgent.isGodlike())
        return true;

    if (!parcel->getRegionAllowEnvironmentOverride())
        return false;

    return LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_ALLOW_ENVIRONMENT);
}

bool LLEnvironment::canAgentUpdateRegionEnvironment() const
{
    if (gAgent.isGodlike())
        return true;

    return gAgent.canManageEstate();
}

bool LLEnvironment::isExtendedEnvironmentEnabled() const
{
    return !gAgent.getRegionCapability("ExtEnvironment").empty();
}

bool LLEnvironment::isInventoryEnabled() const
{
    return (!gAgent.getRegionCapability("UpdateSettingsAgentInventory").empty() &&
        !gAgent.getRegionCapability("UpdateSettingsTaskInventory").empty());
}

void LLEnvironment::onRegionChange()
{
//     if (gAgent.getRegionCapability("ExperienceQuery").empty())
//     {
//     // for now environmental experiences do not survive region crossings
    clearExperienceEnvironment(LLUUID::null, TRANSITION_DEFAULT);
//     }

    LLViewerRegion* cur_region = gAgent.getRegion();
    if (!cur_region)
    {
        return;
    }
    if (!cur_region->capabilitiesReceived())
    {
        cur_region->setCapabilitiesReceivedCallback([](LLUUID region_id) {  LLEnvironment::instance().requestRegion(); });
        return;
    }
    requestRegion();
}

void LLEnvironment::onParcelChange()
{
    S32 parcel_id(INVALID_PARCEL_ID);
    LLParcel* parcel = LLViewerParcelMgr::instance().getAgentParcel();

    if (parcel)
    {
        parcel_id = parcel->getLocalID();
    }

    requestParcel(parcel_id);
}

//-------------------------------------------------------------------------
F32 LLEnvironment::getCamHeight() const
{
    return (mCurrentEnvironment->getSky()->getDomeOffset() * mCurrentEnvironment->getSky()->getDomeRadius());
}

F32 LLEnvironment::getWaterHeight() const
{
    LLViewerRegion* cur_region = gAgent.getRegion();
    return cur_region ? cur_region->getWaterHeight() : DEFAULT_WATER_HEIGHT;
}

bool LLEnvironment::getIsSunUp() const
{
    if (!mCurrentEnvironment || !mCurrentEnvironment->getSky())
        return false;
    return mCurrentEnvironment->getSky()->getIsSunUp();
}

bool LLEnvironment::getIsMoonUp() const
{
    if (!mCurrentEnvironment || !mCurrentEnvironment->getSky())
        return false;
    return mCurrentEnvironment->getSky()->getIsMoonUp();
}

//-------------------------------------------------------------------------
void LLEnvironment::setSelectedEnvironment(LLEnvironment::EnvSelection_t env, LLSettingsBase::Seconds transition, bool forced)
{
    mSelectedEnvironment = env;
    updateEnvironment(transition, forced);
    LL_DEBUGS("ENVIRONMENT") << "Setting environment " << env_selection_to_string(env) << " with transition: " << transition << LL_ENDL;
}

bool LLEnvironment::hasEnvironment(LLEnvironment::EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env >= ENV_DEFAULT) || (!mEnvironments[env]))
    {
        return false;
    }

    return true;
}

LLEnvironment::DayInstance::ptr_t LLEnvironment::getEnvironmentInstance(LLEnvironment::EnvSelection_t env, bool create /*= false*/)
{
    DayInstance::ptr_t environment = mEnvironments[env];
    if (create)
    {
        if (environment)
            environment = environment->clone();
        else
        {
            if (env == ENV_PUSH)
                environment = std::make_shared<DayInjection>(env);
            else
                environment = std::make_shared<DayInstance>(env);
        }
        mEnvironments[env] = environment;
    }

    return environment;
}


void LLEnvironment::setEnvironment(LLEnvironment::EnvSelection_t env, const LLSettingsDay::ptr_t &pday, LLSettingsDay::Seconds daylength, LLSettingsDay::Seconds dayoffset, S32 env_version)
{
    if ((env < ENV_EDIT) || (env >= ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to change invalid environment selection (" << env_selection_to_string(env) << ")." << LL_ENDL;
        return;
    }

    logEnvironment(env, pday, env_version);

    DayInstance::ptr_t environment = getEnvironmentInstance(env, true);

    environment->clear();
    environment->setDay(pday, daylength, dayoffset);
    environment->setSkyTrack(mCurrentTrack);
    environment->animate();

    if (!mSignalEnvChanged.empty())
        mSignalEnvChanged(env, env_version);
}


void LLEnvironment::setEnvironment(LLEnvironment::EnvSelection_t env, LLEnvironment::fixedEnvironment_t fixed, S32 env_version)
{
    if ((env < ENV_EDIT) || (env >= ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to change invalid environment selection (" << env_selection_to_string(env) << ")." << LL_ENDL;
        return;
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env, true);


    if (fixed.first)
    {
        logEnvironment(env, fixed.first, env_version);
        environment->setSky(fixed.first);
        environment->setFlags(DayInstance::NO_ANIMATE_SKY);
    }
    else if (!environment->getSky())
    {
        LL_DEBUGS("ENVIRONMENT") << "Blank sky for " << env_selection_to_string(env) << ". Reusing environment for sky." << LL_ENDL;
        environment->setSky(mCurrentEnvironment->getSky());
        environment->setFlags(DayInstance::NO_ANIMATE_SKY);
    }
        
    if (fixed.second)
    {
        logEnvironment(env, fixed.second, env_version);
        environment->setWater(fixed.second);
        environment->setFlags(DayInstance::NO_ANIMATE_WATER);
    }
    else if (!environment->getWater())
    {
        LL_DEBUGS("ENVIRONMENT") << "Blank water for " << env_selection_to_string(env) << ". Reusing environment for water." << LL_ENDL;
        environment->setWater(mCurrentEnvironment->getWater());
        environment->setFlags(DayInstance::NO_ANIMATE_WATER);
    }

    if (!mSignalEnvChanged.empty())
        mSignalEnvChanged(env, env_version);
}

void LLEnvironment::setEnvironment(LLEnvironment::EnvSelection_t env, const LLSettingsBase::ptr_t &settings, S32 env_version)
{
    DayInstance::ptr_t environment = getEnvironmentInstance(env);

    if (env == ENV_DEFAULT)
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to set default environment. Not allowed." << LL_ENDL;
        return;
    }

    if (!settings)
    {
        clearEnvironment(env);
        return;
    }

    if (settings->getSettingsType() == "daycycle")
    {
        LLSettingsDay::Seconds daylength(LLSettingsDay::DEFAULT_DAYLENGTH);
        LLSettingsDay::Seconds dayoffset(LLSettingsDay::DEFAULT_DAYOFFSET);
        if (environment)
        {
            daylength = environment->getDayLength();
            dayoffset = environment->getDayOffset();
        }
        setEnvironment(env, std::static_pointer_cast<LLSettingsDay>(settings), daylength, dayoffset);
    }
    else if (settings->getSettingsType() == "sky")
    {
        fixedEnvironment_t fixedenv(std::static_pointer_cast<LLSettingsSky>(settings), LLSettingsWater::ptr_t());
        setEnvironment(env, fixedenv);
    }
    else if (settings->getSettingsType() == "water")
    {
        fixedEnvironment_t fixedenv(LLSettingsSky::ptr_t(), std::static_pointer_cast<LLSettingsWater>(settings));
        setEnvironment(env, fixedenv);
    }
}

void LLEnvironment::setEnvironment(EnvSelection_t env, const LLUUID &assetId, S32 env_version)
{
    setEnvironment(env, assetId, LLSettingsDay::DEFAULT_DAYLENGTH, LLSettingsDay::DEFAULT_DAYOFFSET);
}


void LLEnvironment::setEnvironment(EnvSelection_t env,
                                   const LLUUID &assetId,
                                   LLSettingsDay::Seconds daylength,
                                   LLSettingsDay::Seconds dayoffset,
                                   S32 env_version)
{
    LLSettingsVOBase::getSettingsAsset(assetId,
        [this, env, daylength, dayoffset, env_version](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat)
        {
            onSetEnvAssetLoaded(env, asset_id, settings, daylength, dayoffset, TRANSITION_DEFAULT, status, env_version);
        });
}

void LLEnvironment::onSetEnvAssetLoaded(EnvSelection_t env,
                                        LLUUID asset_id,
                                        LLSettingsBase::ptr_t settings,
                                        LLSettingsDay::Seconds daylength,
                                        LLSettingsDay::Seconds dayoffset,
                                        LLSettingsBase::Seconds transition,
                                        S32 status,
                                        S32 env_version)
{
    if (!settings || status)
    {
        LLSD args;
        args["NAME"] = asset_id.asString();
        LLNotificationsUtil::add("FailedToFindSettings", args);
        LL_DEBUGS("ENVIRONMENT") << "Failed to find settings for " << env_selection_to_string(env) << ", asset_id: " << asset_id << LL_ENDL;
        return;
    }
    LL_DEBUGS("ENVIRONMENT") << "Loaded asset: " << asset_id << LL_ENDL;

    setEnvironment(env, settings);
    updateEnvironment(transition);
}

void LLEnvironment::clearEnvironment(LLEnvironment::EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env >= ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to change invalid environment selection." << LL_ENDL;
        return;
    }

    LL_DEBUGS("ENVIRONMENT") << "Cleaning environment " << env_selection_to_string(env) << LL_ENDL;

    mEnvironments[env].reset();

    if (!mSignalEnvChanged.empty())
        mSignalEnvChanged(env, VERSION_CLEANUP);
}

void LLEnvironment::logEnvironment(EnvSelection_t env, const LLSettingsBase::ptr_t &settings, S32 env_version)
{
    LL_DEBUGS("ENVIRONMENT") << "Setting Day environment " << env_selection_to_string(env) << " with version(update type): " << env_version << LL_NEWLINE;
    // code between LL_DEBUGS and LL_ENDL won't execute unless log is enabled
    if (settings)
    {
        LLUUID asset_id = settings->getAssetId();
        if (asset_id.notNull())
        {
            LL_CONT << "Asset id: " << asset_id << LL_NEWLINE;
        }

        LLUUID id = settings->getId(); // Not in use?
        if (id.notNull())
        {
            LL_CONT << "Settings id: " << id << LL_NEWLINE;
        }

        LL_CONT << "Name: " << settings->getName() << LL_NEWLINE
            << "Type: " << settings->getSettingsType() << LL_NEWLINE
            << "Flags: " << settings->getFlags(); // Not in use?
    }
    else
    {
        LL_CONT << "Empty settings!";
    }
    LL_CONT << LL_ENDL;
}

LLSettingsDay::ptr_t LLEnvironment::getEnvironmentDay(LLEnvironment::EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env > ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to retrieve invalid environment selection (" << env_selection_to_string(env) << ")." << LL_ENDL;
        return LLSettingsDay::ptr_t();
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env);

    if (environment)
        return environment->getDayCycle();

    return LLSettingsDay::ptr_t();
}

LLSettingsDay::Seconds LLEnvironment::getEnvironmentDayLength(EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env > ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to retrieve invalid environment selection (" << env_selection_to_string(env) << ")." << LL_ENDL;
        return LLSettingsDay::Seconds(0);
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env);

    if (environment)
        return environment->getDayLength();

    return LLSettingsDay::Seconds(0);
}

LLSettingsDay::Seconds LLEnvironment::getEnvironmentDayOffset(EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env > ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to retrieve invalid environment selection (" << env_selection_to_string(env) << ")." << LL_ENDL;
        return LLSettingsDay::Seconds(0);
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env);
    if (environment)
        return environment->getDayOffset();

    return LLSettingsDay::Seconds(0);
}


LLEnvironment::fixedEnvironment_t LLEnvironment::getEnvironmentFixed(LLEnvironment::EnvSelection_t env, bool resolve)
{
    if ((env == ENV_CURRENT) || resolve)
    {
        fixedEnvironment_t fixed;
        for (S32 idx = ((resolve) ? env : mSelectedEnvironment); idx < ENV_END; ++idx)
        {
            if (fixed.first && fixed.second)
                break;

            if (idx == ENV_EDIT)
                continue;   // skip the edit environment.

            DayInstance::ptr_t environment = getEnvironmentInstance(static_cast<EnvSelection_t>(idx));
            if (environment)
            {
                if (!fixed.first)
                    fixed.first = environment->getSky();
                if (!fixed.second)
                    fixed.second = environment->getWater();
            }
        }

        if (!fixed.first || !fixed.second)
            LL_WARNS("ENVIRONMENT") << "Can not construct complete fixed environment.  Missing Sky and/or Water." << LL_ENDL;

        return fixed;
    }

    if ((env < ENV_EDIT) || (env > ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to retrieve invalid environment selection (" << env_selection_to_string(env) << ")." << LL_ENDL;
        return fixedEnvironment_t();
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env);

    if (environment)
        return fixedEnvironment_t(environment->getSky(), environment->getWater());

    return fixedEnvironment_t();
}

LLEnvironment::DayInstance::ptr_t LLEnvironment::getSelectedEnvironmentInstance()
{
    for (S32 idx = mSelectedEnvironment; idx < ENV_DEFAULT; ++idx)
    {
        if (mEnvironments[idx])
            return mEnvironments[idx];
    }

    return mEnvironments[ENV_DEFAULT];
}

LLEnvironment::DayInstance::ptr_t LLEnvironment::getSharedEnvironmentInstance()
{
    for (S32 idx = ENV_PARCEL; idx < ENV_DEFAULT; ++idx)
    {
        if (mEnvironments[idx])
            return mEnvironments[idx];
    }

    return mEnvironments[ENV_DEFAULT];
}

void LLEnvironment::updateEnvironment(LLSettingsBase::Seconds transition, bool forced)
{
    DayInstance::ptr_t pinstance = getSelectedEnvironmentInstance();

    if ((mCurrentEnvironment != pinstance) || forced)
    {
        if (transition != TRANSITION_INSTANT)
        {
	        DayInstance::ptr_t trans = std::make_shared<DayTransition>(
	            mCurrentEnvironment->getSky(), mCurrentEnvironment->getWater(), pinstance, transition);
	
	        trans->animate();
	
	        mCurrentEnvironment = trans;
        }
        else
        {
            mCurrentEnvironment = pinstance;
        }
    }
}

LLVector4 LLEnvironment::toCFR(const LLVector3 vec) const
{
    LLVector4 vec_cfr(vec.mV[1], vec.mV[0], vec.mV[2], 0.0f);
    return vec_cfr;
}

LLVector4 LLEnvironment::toLightNorm(const LLVector3 vec) const
{
    LLVector4 vec_ogl(vec.mV[1], vec.mV[2], vec.mV[0], 0.0f);
    return vec_ogl;
}

LLVector3 LLEnvironment::getLightDirection() const
{
    LLSettingsSky::ptr_t psky = mCurrentEnvironment->getSky();
    if (!psky)
    {
        return LLVector3(0, 0, 1);
    }
    return psky->getLightDirection();
}

LLVector3 LLEnvironment::getSunDirection() const
{
    LLSettingsSky::ptr_t psky = mCurrentEnvironment->getSky();
    if (!psky)
    {
        return LLVector3(0, 0, 1);
    }
    return psky->getSunDirection();
}

LLVector3 LLEnvironment::getMoonDirection() const
{
    LLSettingsSky::ptr_t psky = mCurrentEnvironment->getSky();
    if (!psky)
    {
        return LLVector3(0, 0, -1);
    }
    return psky->getMoonDirection();
}

LLVector4 LLEnvironment::getLightDirectionCFR() const
{
    LLVector3 light_direction = getLightDirection();
    LLVector4 light_direction_cfr = toCFR(light_direction);
    return light_direction_cfr;
}

LLVector4 LLEnvironment::getSunDirectionCFR() const
{
    LLVector3 light_direction = getSunDirection();
    LLVector4 light_direction_cfr = toCFR(light_direction);
    return light_direction_cfr;
}

LLVector4 LLEnvironment::getMoonDirectionCFR() const
{
    LLVector3 light_direction = getMoonDirection();
    LLVector4 light_direction_cfr = toCFR(light_direction);
    return light_direction_cfr;
}

LLVector4 LLEnvironment::getClampedLightNorm() const
{
    LLVector3 light_direction = getLightDirection();
    if (light_direction.mV[2] < -0.1f)
    {
        light_direction.mV[2] = -0.1f;
    }
    return toLightNorm(light_direction);
}

LLVector4 LLEnvironment::getClampedSunNorm() const
{
    LLVector3 light_direction = getSunDirection();
    if (light_direction.mV[2] < -0.1f)
    {
        light_direction.mV[2] = -0.1f;
    }
    return toLightNorm(light_direction);
}

LLVector4 LLEnvironment::getClampedMoonNorm() const
{
    LLVector3 light_direction = getMoonDirection();
    if (light_direction.mV[2] < -0.1f)
    {
        light_direction.mV[2] = -0.1f;
    }
    return toLightNorm(light_direction);
}

LLVector4 LLEnvironment::getRotatedLightNorm() const
{
    LLVector3 light_direction = getLightDirection();
    light_direction *= LLQuaternion(-mLastCamYaw, LLVector3(0.f, 1.f, 0.f));
    return toLightNorm(light_direction);
}

//-------------------------------------------------------------------------
void LLEnvironment::update(const LLViewerCamera * cam)
{
    LL_RECORD_BLOCK_TIME(FTM_ENVIRONMENT_UPDATE);
    //F32Seconds now(LLDate::now().secondsSinceEpoch());
    static LLFrameTimer timer;

    F32Seconds delta(timer.getElapsedTimeAndResetF32());

    {
        DayInstance::ptr_t keeper = mCurrentEnvironment;    
        // make sure the current environment does not go away until applyTimeDelta is done.
        mCurrentEnvironment->applyTimeDelta(delta);

    }
    // update clouds, sun, and general
    updateCloudScroll();

    // cache this for use in rotating the rotated light vec for shader param updates later...
    mLastCamYaw = cam->getYaw() + SUN_DELTA_YAW;

    stop_glerror();

    // *TODO: potential optimization - this block may only need to be
    // executed some of the time.  For example for water shaders only.
    {
        LLViewerShaderMgr::shader_iter shaders_iter, end_shaders;
        end_shaders = LLViewerShaderMgr::instance()->endShaders();
        for (shaders_iter = LLViewerShaderMgr::instance()->beginShaders(); shaders_iter != end_shaders; ++shaders_iter)
        {
            if ((shaders_iter->mProgramObject != 0)
                && (gPipeline.canUseWindLightShaders()
                || shaders_iter->mShaderGroup == LLGLSLShader::SG_WATER))
            {
                shaders_iter->mUniformsDirty = TRUE;
            }
        }
    }
}

void LLEnvironment::updateCloudScroll()
{
    // This is a function of the environment rather than the sky, since it should 
    // persist through sky transitions.
    static LLTimer s_cloud_timer;

    F64 delta_t = s_cloud_timer.getElapsedTimeAndResetF64();
    
    if (mCurrentEnvironment->getSky() && !mCloudScrollPaused)
    {
        LLVector2 cloud_delta = static_cast<F32>(delta_t)* (mCurrentEnvironment->getSky()->getCloudScrollRate()) / 100.0;
        mCloudScrollDelta += cloud_delta;
    }

}

// static
void LLEnvironment::updateGLVariablesForSettings(LLGLSLShader *shader, const LLSettingsBase::ptr_t &psetting)
{
    LL_RECORD_BLOCK_TIME(FTM_SHADER_PARAM_UPDATE);

    //_WARNS("RIDER") << "----------------------------------------------------------------" << LL_ENDL;
    LLSettingsBase::parammapping_t params = psetting->getParameterMap();
    for (auto &it: params)
    {
        LLSD value;
        // legacy first since it contains ambient color and we prioritize value from legacy, see getAmbientColor()
        if (psetting->mSettings.has(LLSettingsSky::SETTING_LEGACY_HAZE) && psetting->mSettings[LLSettingsSky::SETTING_LEGACY_HAZE].has(it.first))
        {
            value = psetting->mSettings[LLSettingsSky::SETTING_LEGACY_HAZE][it.first];
        }
        else if (psetting->mSettings.has(it.first))
        {
            value = psetting->mSettings[it.first];
        }
        else
        {
            // We need to reset shaders, use defaults
            value = it.second.getDefaultValue();
        }

        LLSD::Type setting_type = value.type();
        stop_glerror();
        switch (setting_type)
        {
        case LLSD::TypeInteger:
            shader->uniform1i(it.second.getShaderKey(), value.asInteger());
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << value << LL_ENDL;
            break;
        case LLSD::TypeReal:
            shader->uniform1f(it.second.getShaderKey(), value.asReal());
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << value << LL_ENDL;
            break;

        case LLSD::TypeBoolean:
            shader->uniform1i(it.second.getShaderKey(), value.asBoolean() ? 1 : 0);
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << value << LL_ENDL;
            break;

        case LLSD::TypeArray:
        {
            LLVector4 vect4(value);
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << vect4 << LL_ENDL;
            shader->uniform4fv(it.second.getShaderKey(), 1, vect4.mV);
            break;
        }

        //  case LLSD::TypeMap:
        //  case LLSD::TypeString:
        //  case LLSD::TypeUUID:
        //  case LLSD::TypeURI:
        //  case LLSD::TypeBinary:
        //  case LLSD::TypeDate:
        default:
            break;
        }
        stop_glerror();
    }
    //_WARNS("RIDER") << "----------------------------------------------------------------" << LL_ENDL;

    psetting->applySpecial(shader);
}

void LLEnvironment::updateShaderUniforms(LLGLSLShader *shader)
{
    updateGLVariablesForSettings(shader, mCurrentEnvironment->getWater());
    updateGLVariablesForSettings(shader, mCurrentEnvironment->getSky());
}

void LLEnvironment::recordEnvironment(S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envinfo, LLSettingsBase::Seconds transition)
{
    if (envinfo->mParcelId == INVALID_PARCEL_ID)
    {
        // the returned info applies to an entire region.
        if (!envinfo->mDayCycle)
        {
            clearEnvironment(ENV_PARCEL);
            setEnvironment(ENV_REGION, LLSettingsDay::GetDefaultAssetId(), LLSettingsDay::DEFAULT_DAYLENGTH, LLSettingsDay::DEFAULT_DAYOFFSET, envinfo->mEnvVersion);
            updateEnvironment();
        }
        else if (envinfo->mDayCycle->isTrackEmpty(LLSettingsDay::TRACK_WATER)
                 || envinfo->mDayCycle->isTrackEmpty(LLSettingsDay::TRACK_GROUND_LEVEL))
        {
            LL_WARNS("ENVIRONMENT") << "Invalid day cycle for region" << LL_ENDL;
            clearEnvironment(ENV_PARCEL);
            setEnvironment(ENV_REGION, LLSettingsDay::GetDefaultAssetId(), LLSettingsDay::DEFAULT_DAYLENGTH, LLSettingsDay::DEFAULT_DAYOFFSET, envinfo->mEnvVersion);
            updateEnvironment();
        }
        else
        {
            setEnvironment(ENV_REGION, envinfo->mDayCycle, envinfo->mDayLength, envinfo->mDayOffset, envinfo->mEnvVersion);
            mTrackAltitudes = envinfo->mAltitudes;
        }

        LL_DEBUGS("ENVIRONMENT") << "Altitudes set to {" << mTrackAltitudes[0] << ", "<< mTrackAltitudes[1] << ", " << mTrackAltitudes[2] << ", " << mTrackAltitudes[3] << LL_ENDL;
    }
    else
    {
        LLParcel *parcel = LLViewerParcelMgr::instance().getAgentParcel();
        LL_DEBUGS("ENVIRONMENT") << "Have parcel environment #" << envinfo->mParcelId << LL_ENDL;
        if (parcel && (parcel->getLocalID() != parcel_id))
        {
            LL_DEBUGS("ENVIRONMENT") << "Requested parcel #" << parcel_id << " agent is on " << parcel->getLocalID() << LL_ENDL;
            return;
        }

        if (!envinfo->mDayCycle)
        {
            LL_DEBUGS("ENVIRONMENT") << "Clearing environment on parcel #" << parcel_id << LL_ENDL;
            clearEnvironment(ENV_PARCEL);
        }
        else if (envinfo->mDayCycle->isTrackEmpty(LLSettingsDay::TRACK_WATER)
                 || envinfo->mDayCycle->isTrackEmpty(LLSettingsDay::TRACK_GROUND_LEVEL))
        {
            LL_WARNS("ENVIRONMENT") << "Invalid day cycle for parcel #" << parcel_id << LL_ENDL;
            clearEnvironment(ENV_PARCEL);
        }
        else
        {
            setEnvironment(ENV_PARCEL, envinfo->mDayCycle, envinfo->mDayLength, envinfo->mDayOffset, envinfo->mEnvVersion);
        }
    }

    updateEnvironment(transition);
}

void LLEnvironment::adjustRegionOffset(F32 adjust)
{
    if (isExtendedEnvironmentEnabled())
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to adjust region offset on EEP region.  Legacy regions only." << LL_ENDL;
    }

    if (mEnvironments[ENV_REGION])
    {
        F32 day_length = mEnvironments[ENV_REGION]->getDayLength();
        F32 day_offset = mEnvironments[ENV_REGION]->getDayOffset();

        F32 day_adjustment = adjust * day_length;

        day_offset += day_adjustment;
        if (day_offset < 0.0f)
            day_offset = day_length + day_offset;
        mEnvironments[ENV_REGION]->setDayOffset(LLSettingsBase::Seconds(day_offset));
    }
}

//=========================================================================
void LLEnvironment::requestRegion(environment_apply_fn cb)
{
    requestParcel(INVALID_PARCEL_ID, cb);
}

void LLEnvironment::updateRegion(const LLSettingsDay::ptr_t &pday, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    updateParcel(INVALID_PARCEL_ID, pday, day_length, day_offset, altitudes, cb);
}

void LLEnvironment::updateRegion(const LLUUID &asset_id, std::string display_name, S32 track_num, S32 day_length, S32 day_offset, U32 flags, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    if (!isExtendedEnvironmentEnabled())
    {
        LL_WARNS("ENVIRONMENT") << "attempt to apply asset id to region not supporting it." << LL_ENDL;
        LLNotificationsUtil::add("NoEnvironmentSettings");
        return;
    }

    updateParcel(INVALID_PARCEL_ID, asset_id, display_name, track_num, day_length, day_offset, flags, altitudes, cb);
}

void LLEnvironment::updateRegion(const LLSettingsSky::ptr_t &psky, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    updateParcel(INVALID_PARCEL_ID, psky, day_length, day_offset, altitudes, cb);
}

void LLEnvironment::updateRegion(const LLSettingsWater::ptr_t &pwater, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    updateParcel(INVALID_PARCEL_ID, pwater, day_length, day_offset, altitudes, cb);
}


void LLEnvironment::resetRegion(environment_apply_fn cb)
{
    resetParcel(INVALID_PARCEL_ID, cb);
}

void LLEnvironment::requestParcel(S32 parcel_id, environment_apply_fn cb)
{
    if (!isExtendedEnvironmentEnabled())
    {   /*TODO: When EEP is live on the entire grid, this can go away. */
        if (parcel_id == INVALID_PARCEL_ID)
        {
            if (!cb)
            {
                LLSettingsBase::Seconds transition = LLViewerParcelMgr::getInstance()->getTeleportInProgress() ? TRANSITION_FAST : TRANSITION_DEFAULT;
                cb = [this, transition](S32 pid, EnvironmentInfo::ptr_t envinfo)
                {
                    clearEnvironment(ENV_PARCEL);
                    recordEnvironment(pid, envinfo, transition);
                };
            }

            LLEnvironmentRequest::initiate(cb);
        }
        else if (cb)
            cb(parcel_id, EnvironmentInfo::ptr_t());
        return;
    }

    if (!cb)
    {
        LLSettingsBase::Seconds transition = LLViewerParcelMgr::getInstance()->getTeleportInProgress() ? TRANSITION_FAST : TRANSITION_DEFAULT;
        cb = [this, transition](S32 pid, EnvironmentInfo::ptr_t envinfo) { recordEnvironment(pid, envinfo, transition); };
    }

    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroRequestEnvironment",
        [this, parcel_id, cb]() { coroRequestEnvironment(parcel_id, cb); });
}

void LLEnvironment::updateParcel(S32 parcel_id, const LLUUID &asset_id, std::string display_name, S32 track_num, S32 day_length, S32 day_offset, U32 flags, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    UpdateInfo::ptr_t updates(std::make_shared<UpdateInfo>(asset_id, display_name, day_length, day_offset, altitudes, flags));
    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroUpdateEnvironment",
        [this, parcel_id, track_num, updates, cb]() { coroUpdateEnvironment(parcel_id, track_num, updates, cb); });
}

void LLEnvironment::onUpdateParcelAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, S32 parcel_id, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes)
{
    if (status)
    {
        LL_WARNS("ENVIRONMENT") << "Unable to get settings asset with id " << asset_id << "!" << LL_ENDL;
        LLNotificationsUtil::add("FailedToLoadSettingsApply");
        return;
    }

    LLSettingsDay::ptr_t pday;

    if (settings->getSettingsType() == "daycycle")
        pday = std::static_pointer_cast<LLSettingsDay>(settings);
    else
    {
        pday = createDayCycleFromEnvironment( (parcel_id == INVALID_PARCEL_ID) ? ENV_REGION : ENV_PARCEL, settings);
    }

    if (!pday)
    {
        LL_WARNS("ENVIRONMENT") << "Unable to construct day around " << asset_id << "!" << LL_ENDL;
        LLNotificationsUtil::add("FailedToBuildSettingsDay");
        return;
    }

    updateParcel(parcel_id, pday, day_length, day_offset, altitudes);
}

void LLEnvironment::updateParcel(S32 parcel_id, const LLSettingsSky::ptr_t &psky, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    LLSettingsDay::ptr_t pday = createDayCycleFromEnvironment((parcel_id == INVALID_PARCEL_ID) ? ENV_REGION : ENV_PARCEL, psky);
    pday->setFlag(psky->getFlags());
    updateParcel(parcel_id, pday, day_length, day_offset, altitudes, cb);
}

void LLEnvironment::updateParcel(S32 parcel_id, const LLSettingsWater::ptr_t &pwater, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    LLSettingsDay::ptr_t pday = createDayCycleFromEnvironment((parcel_id == INVALID_PARCEL_ID) ? ENV_REGION : ENV_PARCEL, pwater);
    pday->setFlag(pwater->getFlags());
    updateParcel(parcel_id, pday, day_length, day_offset, altitudes, cb);
}

void LLEnvironment::updateParcel(S32 parcel_id, const LLSettingsDay::ptr_t &pday, S32 track_num, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    UpdateInfo::ptr_t updates(std::make_shared<UpdateInfo>(pday, day_length, day_offset, altitudes));

    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroUpdateEnvironment",
        [this, parcel_id, track_num, updates, cb]() { coroUpdateEnvironment(parcel_id, track_num, updates, cb); });
}

void LLEnvironment::updateParcel(S32 parcel_id, const LLSettingsDay::ptr_t &pday, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    updateParcel(parcel_id, pday, NO_TRACK, day_length, day_offset, altitudes, cb);
}



void LLEnvironment::resetParcel(S32 parcel_id, environment_apply_fn cb)
{
    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroResetEnvironment",
        [this, parcel_id, cb]() { coroResetEnvironment(parcel_id, NO_TRACK, cb); });
}

void LLEnvironment::coroRequestEnvironment(S32 parcel_id, LLEnvironment::environment_apply_fn apply)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ResetEnvironment", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    std::string url = gAgent.getRegionCapability("ExtEnvironment");
    if (url.empty())
        return;

    LL_DEBUGS("ENVIRONMENT") << "Requesting for parcel_id=" << parcel_id << LL_ENDL;

    if (parcel_id != INVALID_PARCEL_ID)
    {
        std::stringstream query;

        query << "?parcelid=" << parcel_id;
        url += query.str();
    }

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);
    // results that come back may contain the new settings

    LLSD httpResults = result["http_result"];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if (!status)
    {
        LL_WARNS("ENVIRONMENT") << "Couldn't retrieve environment settings for " << ((parcel_id == INVALID_PARCEL_ID) ? ("region!") : ("parcel!")) << LL_ENDL;
    }
    else
    {
        LLSD environment = result[KEY_ENVIRONMENT];
        if (environment.isDefined() && apply)
        {
            EnvironmentInfo::ptr_t envinfo = LLEnvironment::EnvironmentInfo::extract(environment);
            apply(parcel_id, envinfo);
        }
    }

}

void LLEnvironment::coroUpdateEnvironment(S32 parcel_id, S32 track_no, UpdateInfo::ptr_t updates, environment_apply_fn apply)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ResetEnvironment", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    std::string url = gAgent.getRegionCapability("ExtEnvironment");
    if (url.empty())
        return;

    LLSD body(LLSD::emptyMap());
    body[KEY_ENVIRONMENT] = LLSD::emptyMap();

    if (track_no == NO_TRACK)
    {   // day length and offset are only applicable if we are addressing the entire day cycle.
        if (updates->mDayLength > 0)
            body[KEY_ENVIRONMENT][KEY_DAYLENGTH] = updates->mDayLength;
        if (updates->mDayOffset > 0)
            body[KEY_ENVIRONMENT][KEY_DAYOFFSET] = updates->mDayOffset;

        if ((parcel_id == INVALID_PARCEL_ID) && (updates->mAltitudes.size() == 3))
        {   // only test for altitude changes if we are changing the region.
            body[KEY_ENVIRONMENT][KEY_TRACKALTS] = LLSD::emptyArray();
            for (S32 i = 0; i < 3; ++i)
            {
                body[KEY_ENVIRONMENT][KEY_TRACKALTS][i] = updates->mAltitudes[i];
            }
        }
    }

    if (updates->mDayp)
        body[KEY_ENVIRONMENT][KEY_DAYCYCLE] = updates->mDayp->getSettings();
    else if (!updates->mSettingsAsset.isNull())
    {
        body[KEY_ENVIRONMENT][KEY_DAYASSET] = updates->mSettingsAsset;
        if (!updates->mDayName.empty())
            body[KEY_ENVIRONMENT][KEY_DAYNAME] = updates->mDayName;
    }

    body[KEY_ENVIRONMENT][KEY_FLAGS] = LLSD::Integer(updates->mFlags);
    //_WARNS("ENVIRONMENT") << "Body = " << body << LL_ENDL;

    if ((parcel_id != INVALID_PARCEL_ID) || (track_no != NO_TRACK))
    {
        std::stringstream query;
        query << "?";

        if (parcel_id != INVALID_PARCEL_ID)
        {
            query << "parcelid=" << parcel_id;

            if (track_no != NO_TRACK)
                query << "&";
        }
        if (track_no != NO_TRACK)
        { 
            query << "trackno=" << track_no;
        }
        url += query.str();
    }

    LLSD result = httpAdapter->putAndSuspend(httpRequest, url, body);
    // results that come back may contain the new settings

    LLSD notify;

    LLSD httpResults = result["http_result"];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if ((!status) || !result["success"].asBoolean())
    {
        LL_WARNS("ENVIRONMENT") << "Couldn't update Windlight settings for " << ((parcel_id == INVALID_PARCEL_ID) ? ("region!") : ("parcel!")) << LL_ENDL;

        notify = LLSD::emptyMap();
        notify["FAIL_REASON"] = result["message"].asString();
    }
    else
    {
        LLSD environment = result[KEY_ENVIRONMENT];
        if (environment.isDefined() && apply)
        {
            EnvironmentInfo::ptr_t envinfo = LLEnvironment::EnvironmentInfo::extract(environment);
            apply(parcel_id, envinfo);
        }
    }

    if (!notify.isUndefined())
    {
        LLNotificationsUtil::add("WLRegionApplyFail", notify);
        //LLEnvManagerNew::instance().onRegionSettingsApplyResponse(false);
    }
}

void LLEnvironment::coroResetEnvironment(S32 parcel_id, S32 track_no, environment_apply_fn apply)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ResetEnvironment", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    std::string url = gAgent.getRegionCapability("ExtEnvironment");
    if (url.empty())
        return;

    if ((parcel_id != INVALID_PARCEL_ID) || (track_no != NO_TRACK))
    {
        std::stringstream query;
        query << "?";

        if (parcel_id != INVALID_PARCEL_ID)
        {
            query << "parcelid=" << parcel_id;

            if (track_no != NO_TRACK)
                query << "&";
        }
        if (track_no != NO_TRACK)
        {
            query << "trackno=" << track_no;
        }
        url += query.str();
    }

    LLSD result = httpAdapter->deleteAndSuspend(httpRequest, url);
    // results that come back may contain the new settings

    LLSD notify; 

    LLSD httpResults = result["http_result"];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if ((!status) || !result["success"].asBoolean())
    {
        LL_WARNS("ENVIRONMENT") << "Couldn't reset Windlight settings in " << ((parcel_id == INVALID_PARCEL_ID) ? ("region!") : ("parcel!")) << LL_ENDL;

        notify = LLSD::emptyMap();
        notify["FAIL_REASON"] = result["message"].asString();
    }
    else
    {
       LLSD environment = result[KEY_ENVIRONMENT];
        if (environment.isDefined() && apply)
        {
            EnvironmentInfo::ptr_t envinfo = LLEnvironment::EnvironmentInfo::extract(environment);
            apply(parcel_id, envinfo);
        }
    }

    if (!notify.isUndefined())
    {
        LLNotificationsUtil::add("WLRegionApplyFail", notify);
        //LLEnvManagerNew::instance().onRegionSettingsApplyResponse(false);
    }

}


//=========================================================================

LLEnvironment::EnvironmentInfo::EnvironmentInfo():
    mParcelId(INVALID_PARCEL_ID),
    mRegionId(),
    mDayLength(0),
    mDayOffset(0),
    mDayHash(0),
    mDayCycle(),
    mAltitudes({ { 0.0, 0.0, 0.0, 0.0 } }),
    mIsDefault(false),
    mIsLegacy(false),
    mDayCycleName(),
    mNameList(),
    mEnvVersion(INVALID_PARCEL_ENVIRONMENT_VERSION)
{
}

LLEnvironment::EnvironmentInfo::ptr_t LLEnvironment::EnvironmentInfo::extract(LLSD environment)
{
    ptr_t pinfo = std::make_shared<EnvironmentInfo>();

    pinfo->mIsDefault = environment.has(KEY_ISDEFAULT) ? environment[KEY_ISDEFAULT].asBoolean() : true;
    pinfo->mParcelId = environment.has(KEY_PARCELID) ? environment[KEY_PARCELID].asInteger() : INVALID_PARCEL_ID;
    pinfo->mRegionId = environment.has(KEY_REGIONID) ? environment[KEY_REGIONID].asUUID() : LLUUID::null;
    pinfo->mIsLegacy = false;

    if (environment.has(KEY_TRACKALTS))
    {
        for (int idx = 0; idx < 3; idx++)
        {
            pinfo->mAltitudes[idx+1] = environment[KEY_TRACKALTS][idx].asReal();
        }
        pinfo->mAltitudes[0] = 0;
    }

    if (environment.has(KEY_DAYCYCLE))
    {
        pinfo->mDayCycle = LLSettingsVODay::buildFromEnvironmentMessage(environment[KEY_DAYCYCLE]);
        pinfo->mDayLength = LLSettingsDay::Seconds(environment.has(KEY_DAYLENGTH) ? environment[KEY_DAYLENGTH].asInteger() : -1);
        pinfo->mDayOffset = LLSettingsDay::Seconds(environment.has(KEY_DAYOFFSET) ? environment[KEY_DAYOFFSET].asInteger() : -1);
        pinfo->mDayHash = environment.has(KEY_DAYHASH) ? environment[KEY_DAYHASH].asInteger() : 0;
    }
    else
    {
        pinfo->mDayLength = LLEnvironment::instance().getEnvironmentDayLength(ENV_REGION);
        pinfo->mDayOffset = LLEnvironment::instance().getEnvironmentDayOffset(ENV_REGION);
    }

    if (environment.has(KEY_DAYASSET))
    {
        pinfo->mAssetId = environment[KEY_DAYASSET].asUUID();
    }

    if (environment.has(KEY_DAYNAMES))
    {
        LLSD daynames = environment[KEY_DAYNAMES];
        if (daynames.isArray())
        {
            pinfo->mDayCycleName.clear();
            for (S32 index = 0; index < pinfo->mNameList.size(); ++index)
            {
                pinfo->mNameList[index] = daynames[index].asString();
            }
        }
        else if (daynames.isString())
        {
            for (std::string &name: pinfo->mNameList)
            {
                name.clear();
            }

            pinfo->mDayCycleName = daynames.asString();
        }
    }
    else if (pinfo->mDayCycle)
    {
        pinfo->mDayCycleName = pinfo->mDayCycle->getName();
    }


    if (environment.has(KEY_ENVVERSION))
    {
        LLSD version = environment[KEY_ENVVERSION];
        pinfo->mEnvVersion = version.asInteger();
    }
    else
    {
        // can be used for region, but versions should be same
        pinfo->mEnvVersion = pinfo->mIsDefault ? UNSET_PARCEL_ENVIRONMENT_VERSION : INVALID_PARCEL_ENVIRONMENT_VERSION;
    }

    return pinfo;
}


LLEnvironment::EnvironmentInfo::ptr_t LLEnvironment::EnvironmentInfo::extractLegacy(LLSD legacy)
{
    if (!legacy.isArray() || !legacy[0].has("regionID"))
    {
        LL_WARNS("ENVIRONMENT") << "Invalid legacy settings for environment: " << legacy << LL_ENDL;
        return ptr_t();
    }

    ptr_t pinfo = std::make_shared<EnvironmentInfo>();

    pinfo->mIsDefault = false;
    pinfo->mParcelId = INVALID_PARCEL_ID;
    pinfo->mRegionId = legacy[0]["regionID"].asUUID();
    pinfo->mIsLegacy = true;

    pinfo->mDayLength = LLSettingsDay::DEFAULT_DAYLENGTH;
    pinfo->mDayOffset = LLSettingsDay::DEFAULT_DAYOFFSET;
    pinfo->mDayCycle = LLSettingsVODay::buildFromLegacyMessage(pinfo->mRegionId, legacy[1], legacy[2], legacy[3]);
    if (pinfo->mDayCycle)
        pinfo->mDayHash = pinfo->mDayCycle->getHash();

    pinfo->mAltitudes[0] = 0;
    pinfo->mAltitudes[2] = 10001;
    pinfo->mAltitudes[3] = 10002;
    pinfo->mAltitudes[4] = 10003;

    return pinfo;
}

//=========================================================================
LLSettingsWater::ptr_t LLEnvironment::createWaterFromLegacyPreset(const std::string filename, LLSD &messages)
{
    std::string name(gDirUtilp->getBaseFileName(filename, true));
    std::string path(gDirUtilp->getDirName(filename));

    LLSettingsWater::ptr_t water = LLSettingsVOWater::buildFromLegacyPresetFile(name, path, messages);

    if (!water)
    {
        messages["NAME"] = name;
        messages["FILE"] = filename;
    }
    return water;
}

LLSettingsSky::ptr_t LLEnvironment::createSkyFromLegacyPreset(const std::string filename, LLSD &messages)
{
    std::string name(gDirUtilp->getBaseFileName(filename, true));
    std::string path(gDirUtilp->getDirName(filename));

    LLSettingsSky::ptr_t sky = LLSettingsVOSky::buildFromLegacyPresetFile(name, path, messages);
    if (!sky)
    {
        messages["NAME"] = name;
        messages["FILE"] = filename;
    }
    return sky;
}

LLSettingsDay::ptr_t LLEnvironment::createDayCycleFromLegacyPreset(const std::string filename, LLSD &messages)
{
    std::string name(gDirUtilp->getBaseFileName(filename, true));
    std::string path(gDirUtilp->getDirName(filename));

    LLSettingsDay::ptr_t day = LLSettingsVODay::buildFromLegacyPresetFile(name, path, messages);
    if (!day)
    {
        messages["NAME"] = name;
        messages["FILE"] = filename;
    }
    return day;
}

LLSettingsDay::ptr_t LLEnvironment::createDayCycleFromEnvironment(EnvSelection_t env, LLSettingsBase::ptr_t settings)
{
    std::string type(settings->getSettingsType());

    if (type == "daycycle")
        return std::static_pointer_cast<LLSettingsDay>(settings);

    if ((env != ENV_PARCEL) && (env != ENV_REGION))
    {
        LL_WARNS("ENVIRONMENT") << "May only create from parcel or region environment." << LL_ENDL;
        return LLSettingsDay::ptr_t();
    }

    LLSettingsDay::ptr_t day = this->getEnvironmentDay(env);
    if (!day && (env == ENV_PARCEL))
    {
        day = this->getEnvironmentDay(ENV_REGION);
    }

    if (!day)
    {
        LL_WARNS("ENVIRONMENT") << "Could not retrieve existing day settings." << LL_ENDL;
        return LLSettingsDay::ptr_t();
    }

    day = day->buildClone();

    if (type == "sky")
    {
        for (S32 idx = 1; idx < LLSettingsDay::TRACK_MAX; ++idx)
            day->clearCycleTrack(idx);
        day->setSettingsAtKeyframe(settings, 0.0f, 1);
    }
    else if (type == "water")
    {
        day->clearCycleTrack(LLSettingsDay::TRACK_WATER);
        day->setSettingsAtKeyframe(settings, 0.0f, LLSettingsDay::TRACK_WATER);
    }

    return day;
}

void LLEnvironment::onAgentPositionHasChanged(const LLVector3 &localpos)
{
    S32 trackno = calculateSkyTrackForAltitude(localpos.mV[VZ]);
    if (trackno == mCurrentTrack)
        return;

    mCurrentTrack = trackno;
    for (S32 env = ENV_LOCAL; env < ENV_DEFAULT; ++env)
    {
        if (mEnvironments[env])
            mEnvironments[env]->setSkyTrack(mCurrentTrack);
    }
}

S32 LLEnvironment::calculateSkyTrackForAltitude(F64 altitude)
{
    auto it = std::find_if_not(mTrackAltitudes.begin(), mTrackAltitudes.end(), [altitude](F32 test) { return altitude > test; });
    
    if (it == mTrackAltitudes.begin())
        return 1;
    else if (it == mTrackAltitudes.end())
        return 4;

    return std::min(static_cast<S32>(std::distance(mTrackAltitudes.begin(), it)), 4);
}

//-------------------------------------------------------------------------
void LLEnvironment::handleEnvironmentPush(LLSD &message)
{
    // Log the experience message
    LLExperienceLog::instance().handleExperienceMessage(message);

    std::string action = message[KEY_ACTION].asString();
    LLUUID experience_id = message[KEY_EXPERIENCEID].asUUID();
    LLSD action_data = message[KEY_ACTIONDATA];
    F32 transition_time = action_data[KEY_TRANSITIONTIME].asReal();

    //TODO: Check here that the viewer thinks the experience is still valid.


    if (action == ACTION_CLEARENVIRONMENT)
    { 
        handleEnvironmentPushClear(experience_id, action_data, transition_time);
    }
    else if (action == ACTION_PUSHFULLENVIRONMENT)
    { 
        handleEnvironmentPushFull(experience_id, action_data, transition_time);
    }
    else if (action == ACTION_PUSHPARTIALENVIRONMENT)
    { 
        handleEnvironmentPushPartial(experience_id, action_data, transition_time);
    }
    else
    { 
        LL_WARNS("ENVIRONMENT", "GENERICMESSAGES") << "Unknown environment push action '" << action << "'" << LL_ENDL;
    }
}

void LLEnvironment::handleEnvironmentPushClear(LLUUID experience_id, LLSD &message, F32 transition)
{
    clearExperienceEnvironment(experience_id, LLSettingsBase::Seconds(transition));
}

void LLEnvironment::handleEnvironmentPushFull(LLUUID experience_id, LLSD &message, F32 transition)
{
    LLUUID asset_id(message[KEY_ASSETID].asUUID());

    setExperienceEnvironment(experience_id, asset_id, LLSettingsBase::Seconds(transition));
}

void LLEnvironment::handleEnvironmentPushPartial(LLUUID experience_id, LLSD &message, F32 transition)
{
    LLSD settings(message["settings"]);

    if (settings.isUndefined())
        return;

    setExperienceEnvironment(experience_id, settings, LLSettingsBase::Seconds(transition));
}

void LLEnvironment::clearExperienceEnvironment(LLUUID experience_id, LLSettingsBase::Seconds transition_time)
{
    DayInjection::ptr_t injection = std::dynamic_pointer_cast<DayInjection>(getEnvironmentInstance(ENV_PUSH));
    if (injection)
    {
        injection->clearInjections(experience_id, transition_time);
    }

}

void LLEnvironment::setSharedEnvironment()
{
    clearEnvironment(LLEnvironment::ENV_LOCAL);
    setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
    updateEnvironment();
}

void LLEnvironment::setExperienceEnvironment(LLUUID experience_id, LLUUID asset_id, F32 transition_time)
{
    LLSettingsVOBase::getSettingsAsset(asset_id,
        [this, experience_id, transition_time](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat)
    {
        onSetExperienceEnvAssetLoaded(experience_id, settings, transition_time, status);
    });


}

void LLEnvironment::onSetExperienceEnvAssetLoaded(LLUUID experience_id, LLSettingsBase::ptr_t settings, F32 transition_time, S32 status)
{
    DayInjection::ptr_t environment = std::dynamic_pointer_cast<DayInjection>(getEnvironmentInstance(ENV_PUSH));
    bool updateenvironment(false);

    if (!settings || status)
    {
        LLSD args;
        args["NAME"] = experience_id.asString();
        LLNotificationsUtil::add("FailedToFindSettings", args);
        return;
    }

    if (!environment)
    {
        environment = std::dynamic_pointer_cast<DayInjection>(getEnvironmentInstance(ENV_PUSH, true));
        updateenvironment = true;
    }

    if (settings->getSettingsType() == "daycycle")
    {
        environment->setInjectedDay(std::static_pointer_cast<LLSettingsDay>(settings), experience_id, LLSettingsBase::Seconds(transition_time));
    }
    else if (settings->getSettingsType() == "sky")
    {
        environment->setInjectedSky(std::static_pointer_cast<LLSettingsSky>(settings), experience_id, LLSettingsBase::Seconds(transition_time));
    }
    else if (settings->getSettingsType() == "water")
    {
        environment->setInjectedWater(std::static_pointer_cast<LLSettingsWater>(settings), experience_id, LLSettingsBase::Seconds(transition_time));
    }

    if (updateenvironment)
        updateEnvironment(TRANSITION_INSTANT, true);
}


void LLEnvironment::setExperienceEnvironment(LLUUID experience_id, LLSD data, F32 transition_time)
{
    LLSD sky(data["sky"]);
    LLSD water(data["water"]);

    if (sky.isUndefined() && water.isUndefined())
    {
        clearExperienceEnvironment(experience_id, LLSettingsBase::Seconds(transition_time));
        return;
    }

    DayInjection::ptr_t environment = std::dynamic_pointer_cast<DayInjection>(getEnvironmentInstance(ENV_PUSH));
    bool updateenvironment(false);

    if (!environment)
    {
        environment = std::dynamic_pointer_cast<DayInjection>(getEnvironmentInstance(ENV_PUSH, true));
        updateenvironment = true;
    }

    if (!sky.isUndefined())
    {
        environment->injectSkySettings(sky, experience_id, LLSettingsBase::Seconds(transition_time));
    }

    if (!water.isUndefined())
    {
        environment->injectWaterSettings(sky, experience_id, LLSettingsBase::Seconds(transition_time));
    }

    if (updateenvironment)
        updateEnvironment(TRANSITION_INSTANT, true);

}

void LLEnvironment::listenExperiencePump(const LLSD &message)
{
    LLUUID experience_id = message["experience"];
    LLSD data = message[experience_id.asString()];
    std::string permission(data["permission"].asString());

    if ((permission == "Forget") || (permission == "Block"))
    {
        clearExperienceEnvironment(experience_id, (permission == "Block") ? TRANSITION_INSTANT : TRANSITION_FAST);
    }
}

//=========================================================================
LLEnvironment::DayInstance::DayInstance(EnvSelection_t env) :
    mDayCycle(),
    mSky(),
    mWater(),
    mDayLength(LLSettingsDay::DEFAULT_DAYLENGTH),
    mDayOffset(LLSettingsDay::DEFAULT_DAYOFFSET),
    mBlenderSky(),
    mBlenderWater(),
    mInitialized(false),
    mSkyTrack(1),
    mEnv(env),
    mAnimateFlags(0)
{ }


LLEnvironment::DayInstance::ptr_t LLEnvironment::DayInstance::clone() const
{
    ptr_t environment = std::make_shared<DayInstance>(mEnv);

    environment->mDayCycle = mDayCycle;
    environment->mSky = mSky;
    environment->mWater = mWater;
    environment->mDayLength = mDayLength;
    environment->mDayOffset = mDayOffset;
    environment->mBlenderSky = mBlenderSky;
    environment->mBlenderWater = mBlenderWater;
    environment->mInitialized = mInitialized;
    environment->mSkyTrack = mSkyTrack;
    environment->mAnimateFlags = mAnimateFlags;

    return environment;
}

bool LLEnvironment::DayInstance::applyTimeDelta(const LLSettingsBase::Seconds& delta)
{
    ptr_t keeper(shared_from_this());   // makes sure that this does not go away while it is being worked on.

    bool changed(false);
    if (!mInitialized)
        initialize();

    if (mBlenderSky)
        changed |= mBlenderSky->applyTimeDelta(delta);
    if (mBlenderWater)
        changed |= mBlenderWater->applyTimeDelta(delta);
    return changed;
}

void LLEnvironment::DayInstance::setDay(const LLSettingsDay::ptr_t &pday, LLSettingsDay::Seconds daylength, LLSettingsDay::Seconds dayoffset)
{
    mInitialized = false;

    mAnimateFlags = 0;

    mDayCycle = pday;
    mDayLength = daylength;
    mDayOffset = dayoffset;

    mBlenderSky.reset();
    mBlenderWater.reset();

    mSky = LLSettingsVOSky::buildDefaultSky();
    mWater = LLSettingsVOWater::buildDefaultWater();

    animate();
}


void LLEnvironment::DayInstance::setSky(const LLSettingsSky::ptr_t &psky)
{
    mInitialized = false;

    bool different_sky = mSky != psky;
    
    mSky = psky;
    mSky->mReplaced |= different_sky;
    mSky->update();
    mBlenderSky.reset();

    if (gAtmosphere)
    {
        AtmosphericModelSettings settings;
        LLEnvironment::getAtmosphericModelSettings(settings, psky);
        gAtmosphere->configureAtmosphericModel(settings);
    }
}

void LLEnvironment::DayInstance::setWater(const LLSettingsWater::ptr_t &pwater)
{
    mInitialized = false;

    bool different_water = mWater != pwater;
    mWater = pwater;
    mWater->mReplaced |= different_water;
    mWater->update();
    mBlenderWater.reset();
}

void LLEnvironment::DayInstance::initialize()
{
    mInitialized = true;

    if (!mWater)
        mWater = LLSettingsVOWater::buildDefaultWater();
    if (!mSky)
        mSky = LLSettingsVOSky::buildDefaultSky();
}

void LLEnvironment::DayInstance::clear()
{
    mDayCycle.reset();
    mSky.reset();
    mWater.reset();
    mDayLength = LLSettingsDay::DEFAULT_DAYLENGTH;
    mDayOffset = LLSettingsDay::DEFAULT_DAYOFFSET;
    mBlenderSky.reset();
    mBlenderWater.reset();
    mSkyTrack = 1;
}

void LLEnvironment::DayInstance::setSkyTrack(S32 trackno)
{
    mSkyTrack = trackno;
    if (mBlenderSky)
    {
        mBlenderSky->switchTrack(trackno, 0.0);
    }
}

void LLEnvironment::DayInstance::setBlenders(const LLSettingsBlender::ptr_t &skyblend, const LLSettingsBlender::ptr_t &waterblend)
{
    mBlenderSky = skyblend;
    mBlenderWater = waterblend;
}

LLSettingsBase::TrackPosition LLEnvironment::DayInstance::getProgress() const
{
    LLSettingsBase::Seconds now(LLDate::now().secondsSinceEpoch());
    now += mDayOffset;

    if ((mDayLength <= 0) || !mDayCycle)
        return -1.0f;   // no actual day cycle.

    return convert_time_to_position(now, mDayLength);
}

LLSettingsBase::TrackPosition LLEnvironment::DayInstance::secondsToKeyframe(LLSettingsDay::Seconds seconds)
{
    return convert_time_to_position(seconds, mDayLength);
}

void LLEnvironment::DayInstance::animate()
{
    LLSettingsBase::Seconds now(LLDate::now().secondsSinceEpoch());

    now += mDayOffset;

    if (!mDayCycle)
        return;

    if (!(mAnimateFlags & NO_ANIMATE_WATER))
    {
        LLSettingsDay::CycleTrack_t &wtrack = mDayCycle->getCycleTrack(0);

        if (wtrack.empty())
        {
            mWater.reset();
            mBlenderWater.reset();
        }
        else
        {
            mWater = LLSettingsVOWater::buildDefaultWater();
            mBlenderWater = std::make_shared<LLTrackBlenderLoopingTime>(mWater, mDayCycle, 0,
                mDayLength, mDayOffset, DEFAULT_UPDATE_THRESHOLD);
        }
    }

    if (!(mAnimateFlags & NO_ANIMATE_SKY))
    {
        // sky, initialize to track 1
        LLSettingsDay::CycleTrack_t &track = mDayCycle->getCycleTrack(1);

        if (track.empty())
        {
            mSky.reset();
            mBlenderSky.reset();
        }
        else
        {
            mSky = LLSettingsVOSky::buildDefaultSky();
            mBlenderSky = std::make_shared<LLTrackBlenderLoopingTime>(mSky, mDayCycle, 1,
                mDayLength, mDayOffset, DEFAULT_UPDATE_THRESHOLD);
            mBlenderSky->switchTrack(mSkyTrack, 0.0);
        }
    }
}

//-------------------------------------------------------------------------
LLEnvironment::DayTransition::DayTransition(const LLSettingsSky::ptr_t &skystart,
    const LLSettingsWater::ptr_t &waterstart, LLEnvironment::DayInstance::ptr_t &end, LLSettingsDay::Seconds time) :
    DayInstance(ENV_NONE),
    mStartSky(skystart),
    mStartWater(waterstart),
    mNextInstance(end),
    mTransitionTime(time)
{
    
}

bool LLEnvironment::DayTransition::applyTimeDelta(const LLSettingsBase::Seconds& delta)
{
    bool changed(false);

    changed = mNextInstance->applyTimeDelta(delta);
    changed |= DayInstance::applyTimeDelta(delta);
    return changed;
}

void LLEnvironment::DayTransition::animate() 
{
    mNextInstance->animate();

    mWater = mStartWater->buildClone();
    mBlenderWater = std::make_shared<LLSettingsBlenderTimeDelta>(mWater, mStartWater, mNextInstance->getWater(), mTransitionTime);
    mBlenderWater->setOnFinished(
        [this](LLSettingsBlender::ptr_t blender) { 
            mBlenderWater.reset();

            if (!mBlenderSky && !mBlenderWater)
                LLEnvironment::instance().mCurrentEnvironment = mNextInstance;
            else
                setWater(mNextInstance->getWater());
    });

    mSky = mStartSky->buildClone();
    mBlenderSky = std::make_shared<LLSettingsBlenderTimeDelta>(mSky, mStartSky, mNextInstance->getSky(), mTransitionTime);
    mBlenderSky->setOnFinished(
        [this](LLSettingsBlender::ptr_t blender) {
        mBlenderSky.reset();

        if (!mBlenderSky && !mBlenderWater)
            LLEnvironment::instance().mCurrentEnvironment = mNextInstance;
        else
            setSky(mNextInstance->getSky());
    });
}

void LLEnvironment::saveToSettings()
{
    std::string user_dir = gDirUtilp->getLindenUserDir();
    if (user_dir.empty())
    {
        // not logged in
        return;
    }
    bool has_data = false;

    if (gSavedSettings.getBOOL("EnvironmentPersistAcrossLogin"))
    {
        DayInstance::ptr_t environment = getEnvironmentInstance(ENV_LOCAL);
        if (environment)
        {
            // Environment is 'layered'. No data in ENV_LOCAL means we are using parcel/region
            // Store local environment for next session
            LLSD env_data;

            LLSettingsDay::ptr_t day = environment->getDayCycle();
            if (day)
            {
                const std::string name = day->getName();
                const LLUUID asset_id = day->getAssetId();
                if (asset_id.notNull())
                {
                    // just save the id
                    env_data["day_id"] = asset_id;
                    env_data["day_length"] = LLSD::Integer(environment->getDayLength());
                    env_data["day_offset"] = LLSD::Integer(environment->getDayOffset());
                    has_data = true;
                }
                else if (!name.empty() && name != LLSettingsBase::DEFAULT_SETTINGS_NAME)
                {
                    // This setting was created locally and was not saved
                    // The only option is to save the whole thing
                    env_data["day_llsd"] = day->getSettings();
                    env_data["day_length"] = LLSD::Integer(environment->getDayLength());
                    env_data["day_offset"] = LLSD::Integer(environment->getDayOffset());
                    has_data = true;
                }
            }

            LLSettingsSky::ptr_t sky = environment->getSky();
            if ((environment->getFlags() & DayInstance::NO_ANIMATE_SKY) && sky)
            {
                const std::string name = sky->getName();
                const LLUUID asset_id = sky->getAssetId();
                if (asset_id.notNull())
                {
                    // just save the id
                    env_data["sky_id"] = asset_id;
                    has_data = true;
                }
                else if (!name.empty() && name != LLSettingsBase::DEFAULT_SETTINGS_NAME)
                {
                    // This setting was created locally and was not saved
                    // The only option is to save the whole thing
                    env_data["sky_llsd"] = sky->getSettings();
                    has_data = true;
                }
                has_data = true;
            }

            LLSettingsWater::ptr_t water = environment->getWater();
            if ((environment->getFlags() & DayInstance::NO_ANIMATE_WATER) && water)
            {
                const std::string name = water->getName();
                const LLUUID asset_id = water->getAssetId();
                if (asset_id.notNull())
                {
                    // just save the id
                    env_data["water_id"] = asset_id;
                    has_data = true;
                }
                else if (!name.empty() && name != LLSettingsBase::DEFAULT_SETTINGS_NAME)
                {
                    // This setting was created locally and was not saved
                    // The only option is to save the whole thing
                    env_data["water_llsd"] = water->getSettings();
                    has_data = true;
                }
            }

            std::string user_filepath = user_dir + gDirUtilp->getDirDelimiter() + LOCAL_ENV_STORAGE_FILE;
            llofstream out(user_filepath.c_str(), std::ios_base::out | std::ios_base::binary);
            if (out.good())
            {
                LLSDSerialize::toBinary(env_data, out);
                out.close();
            }
            else
            {
                LL_WARNS("ENVIRONMENT") << "Unable to open " << user_filepath << " for output." << LL_ENDL;
            }
        }
    }

    if (!has_data)
    {
        LLFile::remove(user_dir + gDirUtilp->getDirDelimiter() + LOCAL_ENV_STORAGE_FILE, ENOENT);
    }
}

void LLEnvironment::loadSkyWaterFromSettings(const LLSD &env_data, bool &valid, bool &assets_present)
{
    if (env_data.has("sky_id"))
    {
        // causes asset loaded callback and an update
        setEnvironment(ENV_LOCAL, env_data["sky_id"].asUUID());
        valid = true;
        assets_present = true;
    }
    else if (env_data.has("sky_llsd"))
    {
        LLSettingsSky::ptr_t sky = LLSettingsVOSky::buildSky(env_data["sky_llsd"]);
        setEnvironment(ENV_LOCAL, sky);
        valid = true;
    }

    if (env_data.has("water_id"))
    {
        // causes asset loaded callback and an update
        setEnvironment(ENV_LOCAL, env_data["water_id"].asUUID());
        valid = true;
        assets_present = true;
    }
    else if (env_data.has("water_llsd"))
    {
        LLSettingsWater::ptr_t sky = LLSettingsVOWater::buildWater(env_data["water_llsd"]);
        setEnvironment(ENV_LOCAL, sky);
        valid = true;
    }
}

bool LLEnvironment::loadFromSettings()
{
    if (!gSavedSettings.getBOOL("EnvironmentPersistAcrossLogin"))
    {
        return false;
    }

    std::string user_path = gDirUtilp->getLindenUserDir();
    if (user_path.empty())
    {
        LL_WARNS("ENVIRONMENT") << "Can't load previous environment, Environment was initialized before user logged in" << LL_ENDL;
        return false;
    }
    std::string user_filepath(user_path + gDirUtilp->getDirDelimiter() + LOCAL_ENV_STORAGE_FILE);
    if (!gDirUtilp->fileExists(user_filepath))
    {
        // No previous environment
        return false;
    }

    LLSD env_data;
    llifstream file(user_filepath.c_str(), std::ios_base::in | std::ios_base::binary);
    if (file.is_open())
    {
        LLSDSerialize::fromBinary(env_data, file, LLSDSerialize::SIZE_UNLIMITED);
        if (env_data.isUndefined())
        {
            LL_WARNS("ENVIRONMENT") << "error loading " << user_filepath << LL_ENDL;
            return false;
        }
        else
        {
            LL_INFOS("ENVIRONMENT") << "Loaded previous session environment from: " << user_filepath << LL_ENDL;
        }
        file.close();
    }
    else
    {
        LL_INFOS("ENVIRONMENT") << "Unable to open previous session environment file " << user_filepath << LL_ENDL;
    }

    if (!env_data.isMap() || env_data.emptyMap())
    {
        LL_DEBUGS("ENVIRONMENT") << "Empty map loaded from: " << user_filepath << LL_ENDL;
        return false;
    }

    bool valid = false;
    bool has_assets = false;

    if (env_data.has("day_id"))
    {
        LLSettingsDay::Seconds length = LLSettingsDay::Seconds(env_data["day_length"].asInteger());
        LLSettingsDay::Seconds offset = LLSettingsDay::Seconds(env_data["day_offset"].asInteger());
        LLUUID assetId = env_data["day_id"].asUUID();

        LLSettingsVOBase::getSettingsAsset(assetId,
            [this, length, offset, env_data](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat)
        {
            // Day should be always applied first,
            // otherwise it will override sky or water that was set earlier
            // so wait for asset to load before applying sky/water
            onSetEnvAssetLoaded(ENV_LOCAL, asset_id, settings, length, offset, TRANSITION_DEFAULT, status, NO_VERSION);
            bool valid = false, has_assets = false;
            loadSkyWaterFromSettings(env_data, valid, has_assets);
            if (!has_assets && valid)
            {
                // Settings were loaded from file without having an asset, needs update
                // otherwise update will be done by asset callback
                updateEnvironment(TRANSITION_DEFAULT, true);
            }
        });
        // bail early, everything have to be done at callback
        return true;
    }
    else if (env_data.has("day_llsd"))
    {
        S32 length = env_data["day_length"].asInteger();
        S32 offset = env_data["day_offset"].asInteger();
        LLSettingsDay::ptr_t pday = LLSettingsVODay::buildDay(env_data["day_llsd"]);
        setEnvironment(ENV_LOCAL, pday, LLSettingsDay::Seconds(length), LLSettingsDay::Seconds(offset));
        valid = true;
    }

    loadSkyWaterFromSettings(env_data, valid, has_assets);

    if (valid && !has_assets)
    {
        // Settings were loaded from file without having an asset, needs update
        // otherwise update will be done by asset callback
        updateEnvironment(TRANSITION_DEFAULT, true);
    }
    return valid;
}

void LLEnvironment::saveBeaconsState()
{
    if (mEditorCounter == 0)
    {
        mShowSunBeacon = gSavedSettings.getBOOL("sunbeacon");
        mShowMoonBeacon = gSavedSettings.getBOOL("moonbeacon");
    }
    ++mEditorCounter;
}
void LLEnvironment::revertBeaconsState()
{
    --mEditorCounter;
    if (mEditorCounter == 0)
    {
        gSavedSettings.setBOOL("sunbeacon", mShowSunBeacon && gSavedSettings.getBOOL("sunbeacon"));
        gSavedSettings.setBOOL("moonbeacon", mShowMoonBeacon && gSavedSettings.getBOOL("moonbeacon"));
    }
}

//=========================================================================
LLTrackBlenderLoopingManual::LLTrackBlenderLoopingManual(const LLSettingsBase::ptr_t &target, const LLSettingsDay::ptr_t &day, S32 trackno) :
        LLSettingsBlender(target, LLSettingsBase::ptr_t(), LLSettingsBase::ptr_t()),
        mDay(day),
        mTrackNo(trackno),
        mPosition(0.0)
{
    LLSettingsDay::TrackBound_t initial = getBoundingEntries(mPosition);

    if (initial.first != mEndMarker)
    {   // No frames in track
        mInitial = (*initial.first).second;
        mFinal = (*initial.second).second;

        LLSD initSettings = mInitial->getSettings();
        mTarget->replaceSettings(initSettings);
    }
}

LLSettingsBase::BlendFactor LLTrackBlenderLoopingManual::setPosition(const LLSettingsBase::TrackPosition& position)
{
    mPosition = llclamp(position, 0.0f, 1.0f);

    LLSettingsDay::TrackBound_t bounds = getBoundingEntries(mPosition);

    if (bounds.first == mEndMarker)
    {   // No frames in track.
        return 0.0;
    }

    mInitial = (*bounds.first).second;
    mFinal = (*bounds.second).second;

    F64 spanLength = getSpanLength(bounds);

    F64 spanPos = ((mPosition < (*bounds.first).first) ? (mPosition + 1.0) : mPosition) - (*bounds.first).first;

    if (spanPos > spanLength)
    {
        // we are clamping position to 0-1 and spanLength is 1
        // so don't account for case of spanPos == spanLength
        spanPos = fmod(spanPos, spanLength);
    }

    F64 blendf = spanPos / spanLength;
    return LLSettingsBlender::setBlendFactor(blendf);
}

void LLTrackBlenderLoopingManual::switchTrack(S32 trackno, const LLSettingsBase::TrackPosition& position)
{
    mTrackNo = trackno;

    LLSettingsBase::TrackPosition useposition = (position < 0.0) ? mPosition : position;

    setPosition(useposition);
}

LLSettingsDay::TrackBound_t LLTrackBlenderLoopingManual::getBoundingEntries(F64 position)
{
    LLSettingsDay::CycleTrack_t &wtrack = mDay->getCycleTrack(mTrackNo);

    mEndMarker = wtrack.end();

    LLSettingsDay::TrackBound_t bounds = get_bounding_entries(wtrack, position);
    return bounds;
}

F64 LLTrackBlenderLoopingManual::getSpanLength(const LLSettingsDay::TrackBound_t &bounds) const
{
    return get_wrapping_distance((*bounds.first).first, (*bounds.second).first);
}

//=========================================================================
namespace
{
    DayInjection::DayInjection(LLEnvironment::EnvSelection_t env):
        LLEnvironment::DayInstance(env),
        mBaseDayInstance(),
        mInjectedSky(),
        mInjectedWater(),
        mActiveExperiences(),
        mDayExperience(),
        mSkyExperience(),
        mWaterExperience(),
        mEnvChangeConnection(),
        mParcelChangeConnection()
    {
        mInjectedSky = std::make_shared<LLSettingsInjectedSky>(LLEnvironment::instance().getCurrentSky());
        mInjectedWater = std::make_shared<LLSettingsInjectedWater>(LLEnvironment::instance().getCurrentWater());
        mBaseDayInstance = LLEnvironment::instance().getSharedEnvironmentInstance();
        mSky = mInjectedSky;
        mWater = mInjectedWater;

        mEnvChangeConnection = LLEnvironment::instance().setEnvironmentChanged([this](LLEnvironment::EnvSelection_t env, S32) { onEnvironmentChanged(env); });
        mParcelChangeConnection = gAgent.addParcelChangedCallback([this]() { onParcelChange(); });
    }

    DayInjection::~DayInjection()
    {
        if (mEnvChangeConnection.connected())
            mEnvChangeConnection.disconnect();
        if (mParcelChangeConnection.connected())
            mParcelChangeConnection.disconnect();
    }


    bool DayInjection::applyTimeDelta(const LLSettingsBase::Seconds& delta)
    {
        bool changed(false);

        if (mBaseDayInstance)
            changed |= mBaseDayInstance->applyTimeDelta(delta);
        mInjectedSky->applyInjections(delta);
        mInjectedWater->applyInjections(delta);
        changed |= LLEnvironment::DayInstance::applyTimeDelta(delta);
        if (changed)
        {
            mInjectedSky->setDirtyFlag(true);
            mInjectedWater->setDirtyFlag(true);
        }
        mInjectedSky->update();
        mInjectedWater->update();

        if (!hasInjections())
        {   // There are no injections being managed.  This should really go away.
            LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_PUSH);
            LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        }

        return changed;
    }

    void DayInjection::setBaseDayInstance(const LLEnvironment::DayInstance::ptr_t &baseday) 
    { 
        mBaseDayInstance = baseday;

        if (mSkyExperience.isNull())
            mInjectedSky->setSource(mBaseDayInstance->getSky());
        if (mWaterExperience.isNull())
            mInjectedWater->setSource(mBaseDayInstance->getWater());
    }


    bool DayInjection::hasInjections() const
    {
        return (!mSkyExperience.isNull() || !mWaterExperience.isNull() || !mDayExperience.isNull() ||
            mBlenderSky || mBlenderWater || mInjectedSky->hasInjections() || mInjectedWater->hasInjections());
    }


    void DayInjection::testExperiencesOnParcel(S32 parcel_id)
    {
        LLCoros::instance().launch("DayInjection::testExperiencesOnParcel",
            [this, parcel_id]() { DayInjection::testExperiencesOnParcelCoro(std::static_pointer_cast<DayInjection>(this->shared_from_this()), parcel_id); });

    }

    void DayInjection::setInjectedDay(const LLSettingsDay::ptr_t &pday, LLUUID experience_id, LLSettingsBase::Seconds transition)
    {
        mSkyExperience = experience_id;
        mWaterExperience = experience_id;
        mDayExperience = experience_id;

        mBaseDayInstance = mBaseDayInstance->clone();
        mBaseDayInstance->setEnvironmentSelection(LLEnvironment::ENV_NONE);
        mBaseDayInstance->setDay(pday, mBaseDayInstance->getDayLength(), mBaseDayInstance->getDayOffset());
        animateSkyChange(mBaseDayInstance->getSky(), transition);
        animateWaterChange(mBaseDayInstance->getWater(), transition);

        mActiveExperiences.insert(experience_id);
    }

    void DayInjection::setInjectedSky(const LLSettingsSky::ptr_t &psky, LLUUID experience_id, LLSettingsBase::Seconds transition)
    {
        mSkyExperience = experience_id;
        mActiveExperiences.insert(experience_id);
        checkExperience();
        animateSkyChange(psky, transition);
    }

    void DayInjection::setInjectedWater(const LLSettingsWater::ptr_t &pwater, LLUUID experience_id, LLSettingsBase::Seconds transition)
    {
        mWaterExperience = experience_id;
        mActiveExperiences.insert(experience_id);
        checkExperience();
        animateWaterChange(pwater, transition);
    }

    void DayInjection::injectSkySettings(LLSD settings, LLUUID experience_id, LLSettingsBase::Seconds transition)
    {
        mInjectedSky->injectExperienceValues(settings, experience_id, transition);
        mActiveExperiences.insert(experience_id);
    }

    void DayInjection::injectWaterSettings(LLSD settings, LLUUID experience_id, LLSettingsBase::Seconds transition)
    {
        mInjectedWater->injectExperienceValues(settings, experience_id, transition);
        mActiveExperiences.insert(experience_id);
    }

    void DayInjection::clearInjections(LLUUID experience_id, LLSettingsBase::Seconds transition_time)
    {
        if ((experience_id.isNull() && !mDayExperience.isNull()) || (experience_id == mDayExperience))
        {
            mDayExperience.setNull();
            if (mSkyExperience == experience_id)
                mSkyExperience.setNull();
            if (mWaterExperience == experience_id)
                mWaterExperience.setNull();

            mBaseDayInstance = LLEnvironment::instance().getSharedEnvironmentInstance();

            if (mSkyExperience.isNull())
                animateSkyChange(mBaseDayInstance->getSky(), transition_time);
            if (mWaterExperience.isNull())
                animateWaterChange(mBaseDayInstance->getWater(), transition_time);
        }

        if ((experience_id.isNull() && !mSkyExperience.isNull()) || (experience_id == mSkyExperience))
        {
            mSkyExperience.setNull();
            animateSkyChange(mBaseDayInstance->getSky(), transition_time);
        }
        if ((experience_id.isNull() && !mWaterExperience.isNull()) || (experience_id == mWaterExperience))
        {
            mWaterExperience.setNull();
            animateWaterChange(mBaseDayInstance->getWater(), transition_time);
        }

        mInjectedSky->removeInjections(experience_id, transition_time);
        mInjectedWater->removeInjections(experience_id, transition_time);

        if (experience_id.isNull())
            mActiveExperiences.clear();
        else
            mActiveExperiences.erase(experience_id);

        if ((transition_time == LLEnvironment::TRANSITION_INSTANT) && (countExperiencesActive() == 0))
        {   // Only do this if instant and there are no other experiences injecting values.
            // (otherwise will be handled after transition)
            LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_PUSH);
            LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
        }
    }


    void DayInjection::testExperiencesOnParcelCoro(wptr_t that, S32 parcel_id)
    {
        LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
            httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("testExperiencesOnParcelCoro", httpPolicy));
        LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
        std::string url = gAgent.getRegionCapability("ExperienceQuery");

        if (url.empty())
        {
            LL_WARNS("ENVIRONMENT") << "No experience query cap." << LL_ENDL;
            return; // no checking in this region.
        }

        {
            ptr_t thatlock(that);
            std::stringstream fullurl;

            if (!thatlock)
                return;

            fullurl << url << "?";
            fullurl << "parcelid=" << parcel_id;

            for (auto it = thatlock->mActiveExperiences.begin(); it != thatlock->mActiveExperiences.end(); ++it)
            {
                if (it != thatlock->mActiveExperiences.begin())
                    fullurl << ",";
                else
                    fullurl << "&experiences=";
                fullurl << (*it).asString();
            }
            url = fullurl.str();
        }

        LLSD result = httpAdapter->getAndSuspend(httpRequest, url);
        LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        if (!status)
        {
            LL_WARNS() << "Unable to retrieve experience status for parcel." << LL_ENDL;
            return;
        }

        {
            LLParcel* parcel = LLViewerParcelMgr::instance().getAgentParcel();
            if (!parcel)
                return;

            if (parcel_id != parcel->getLocalID())
            {
                // Agent no longer on queried parcel.
                return;
            }
        }


        LLSD experiences = result["experiences"];
        {
            ptr_t thatlock(that);
            if (!thatlock)
                return;

            for (LLSD::map_iterator itr = experiences.beginMap(); itr != experiences.endMap(); ++itr)
            {
                if (!((*itr).second.asBoolean()))
                    thatlock->clearInjections(LLUUID((*itr).first), LLEnvironment::TRANSITION_FAST);

            }
        }
    }

    void DayInjection::animateSkyChange(LLSettingsSky::ptr_t psky, LLSettingsBase::Seconds transition)
    {
        if (mInjectedSky.get() == psky.get())
        {   // An attempt to animate to itself... don't do it.
            return;
        }
        if (transition == LLEnvironment::TRANSITION_INSTANT)
        {
            mBlenderSky.reset();
            mInjectedSky->setSource(psky);
        }
        else
        {
            LLSettingsSky::ptr_t start_sky(mInjectedSky->getSource()->buildClone());
            LLSettingsSky::ptr_t target_sky(start_sky->buildClone());
            mInjectedSky->setSource(target_sky);

            mBlenderSky = std::make_shared<LLSettingsBlenderTimeDelta>(target_sky, start_sky, psky, transition);
            mBlenderSky->setOnFinished(
                [this, psky](LLSettingsBlender::ptr_t blender) 
                {
                    mBlenderSky.reset();
                    mInjectedSky->setSource(psky);
                    setSky(mInjectedSky);
                    if (!mBlenderWater && (countExperiencesActive() == 0))
                    {
                        LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_PUSH);
                        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
                    }
                });
        }
    }

    void DayInjection::animateWaterChange(LLSettingsWater::ptr_t pwater, LLSettingsBase::Seconds transition)
    {
        if (mInjectedWater.get() == pwater.get())
        {   // An attempt to animate to itself. Bad idea.
            return;
        }
        if (transition == LLEnvironment::TRANSITION_INSTANT)
        { 
            mBlenderWater.reset();
            mInjectedWater->setSource(pwater);
        }
        else
        {
            LLSettingsWater::ptr_t start_Water(mInjectedWater->getSource()->buildClone());
            LLSettingsWater::ptr_t scratch_Water(start_Water->buildClone());
            mInjectedWater->setSource(scratch_Water);

            mBlenderWater = std::make_shared<LLSettingsBlenderTimeDelta>(scratch_Water, start_Water, pwater, transition);
            mBlenderWater->setOnFinished(
                [this, pwater](LLSettingsBlender::ptr_t blender) 
                {
                    mBlenderWater.reset();
                    mInjectedWater->setSource(pwater);
                    setWater(mInjectedWater);
                    if (!mBlenderSky && (countExperiencesActive() == 0))
                    {
                        LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_PUSH);
                        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
                    }
                });
        }
    }

    void DayInjection::onEnvironmentChanged(LLEnvironment::EnvSelection_t env)
    {
        if (env >= LLEnvironment::ENV_PARCEL)
        {
            LLEnvironment::EnvSelection_t base_env(mBaseDayInstance->getEnvironmentSelection());
            LLEnvironment::DayInstance::ptr_t nextbase = LLEnvironment::instance().getSharedEnvironmentInstance();

            if ((base_env == LLEnvironment::ENV_NONE) || (nextbase == mBaseDayInstance) || 
                    (!mSkyExperience.isNull() && !mWaterExperience.isNull()))
            {   // base instance completely overridden, or not changed no transition will happen
                return;
            }

            LL_WARNS("PUSHENV", "ENVIRONMENT") << "Underlying environment has changed (" << env << ")! Base env is type " << base_env << LL_ENDL;

            LLEnvironment::DayInstance::ptr_t trans = std::make_shared<InjectedTransition>(std::static_pointer_cast<DayInjection>(shared_from_this()),
                mBaseDayInstance->getSky(), mBaseDayInstance->getWater(), nextbase, LLEnvironment::TRANSITION_DEFAULT);

            trans->animate();
            setBaseDayInstance(trans);
        }
    }

    void DayInjection::onParcelChange()
    {
        S32 parcel_id(INVALID_PARCEL_ID);
        LLParcel* parcel = LLViewerParcelMgr::instance().getAgentParcel();

        if (!parcel)
            return;

        parcel_id = parcel->getLocalID();

        testExperiencesOnParcel(parcel_id);
    }

    void DayInjection::checkExperience()
    {
        if ((!mDayExperience.isNull()) && (mSkyExperience != mDayExperience) && (mWaterExperience != mDayExperience))
        {   // There was a day experience but we've replaced it with a water and a sky experience. 
            mDayExperience.setNull();
            mBaseDayInstance = LLEnvironment::instance().getSharedEnvironmentInstance();
        }
    }

    void DayInjection::animate()
    {

    }

    void InjectedTransition::animate()
    {
        mNextInstance->animate();

        if (!mInjection->isOverriddenSky())
        {
            mSky = mStartSky->buildClone();
            mBlenderSky = std::make_shared<LLSettingsBlenderTimeDelta>(mSky, mStartSky, mNextInstance->getSky(), mTransitionTime);
            mBlenderSky->setOnFinished(
                [this](LLSettingsBlender::ptr_t blender) {
                mBlenderSky.reset();

                if (!mBlenderSky && !mBlenderSky)
                    mInjection->setBaseDayInstance(mNextInstance);
                else
                    mInjection->mInjectedSky->setSource(mNextInstance->getSky());
            });
        }
        else
        {
            mSky = mInjection->getSky();
            mBlenderSky.reset();
        }

        if (!mInjection->isOverriddenWater())
        {
            mWater = mStartWater->buildClone();
            mBlenderWater = std::make_shared<LLSettingsBlenderTimeDelta>(mWater, mStartWater, mNextInstance->getWater(), mTransitionTime);
            mBlenderWater->setOnFinished(
                [this](LLSettingsBlender::ptr_t blender) {
                mBlenderWater.reset();

                if (!mBlenderSky && !mBlenderWater)
                    mInjection->setBaseDayInstance(mNextInstance);
                else
                    mInjection->mInjectedWater->setSource(mNextInstance->getWater());
            });
        }
        else
        {
            mWater = mInjection->getWater();
            mBlenderWater.reset();
        }

    }

}

