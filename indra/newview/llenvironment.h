/**
 * @file llenvmanager.h
 * @brief Declaration of classes managing WindLight and water settings.
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

#ifndef LL_ENVIRONMENT_H
#define LL_ENVIRONMENT_H

#include "llsingleton.h"
#include "llmemory.h"
#include "llsd.h"

#include "llsettingsbase.h"
#include "llsettingssky.h"
#include "llsettingswater.h"
#include "llsettingsdaycycle.h"

#include <boost/signals2.hpp>

//-------------------------------------------------------------------------
class LLViewerCamera;
class LLGLSLShader;

//-------------------------------------------------------------------------
class LLEnvironment : public LLSingleton<LLEnvironment>
{
    LLSINGLETON(LLEnvironment);
    LOG_CLASS(LLEnvironment);

public:
    static const F32Seconds     TRANSITION_INSTANT;
    static const F32Seconds     TRANSITION_FAST;
    static const F32Seconds     TRANSITION_DEFAULT;
    static const F32Seconds     TRANSITION_SLOW;
    static const F32Seconds     TRANSITION_ALTITUDE;

    static const LLUUID         KNOWN_SKY_DEFAULT;
    static const LLUUID         KNOWN_WATER_DEFAULT;
    static const LLUUID         KNOWN_DAY_DEFAULT;
    static const LLUUID         KNOWN_SKY_SUNRISE;
    static const LLUUID         KNOWN_SKY_MIDDAY;
    static const LLUUID         KNOWN_SKY_SUNSET;
    static const LLUUID         KNOWN_SKY_MIDNIGHT;

    struct EnvironmentInfo
    {
        EnvironmentInfo();

        typedef std::shared_ptr<EnvironmentInfo>  ptr_t;

        S32                 mParcelId;
        LLUUID              mRegionId;
        S64Seconds          mDayLength;
        S64Seconds          mDayOffset;
        size_t              mDayHash;
        LLSD                mDaycycleData;
        std::array<F32, 4>  mAltitudes;
        bool                mIsDefault;
        bool                mIsRegion;


        static ptr_t        extract(LLSD);

    };

    enum EnvSelection_t
    {
        ENV_EDIT = 0,
        ENV_LOCAL,
        ENV_PARCEL,
        ENV_REGION,
        ENV_DEFAULT,
        ENV_END,
        ENV_CURRENT = -1,
        ENV_NONE = -2
    };

    typedef boost::signals2::connection     connection_t;

    class UserPrefs
    {
        friend class LLEnvironment;
    public:
        UserPrefs();

        bool            getUseRegionSettings() const { return mUseRegionSettings; }
        bool            getUseDayCycle() const { return mUseDayCycle; }
        bool            getUseFixedSky() const { return !getUseDayCycle(); }

        std::string     getWaterPresetName() const { return mWaterPresetName; }
        std::string     getSkyPresetName() const { return mSkyPresetName; }
        std::string     getDayCycleName() const { return mDayCycleName; }

        void            setUseRegionSettings(bool val);
        void            setUseWaterPreset(const std::string& name);
        void            setUseSkyPreset(const std::string& name);
        void            setUseDayCycle(const std::string& name);

    private:
        void            load();
        void            store();

        bool			mUseRegionSettings;
        bool			mUseDayCycle;
        bool            mPersistEnvironment;
        std::string		mWaterPresetName;
        std::string		mSkyPresetName;
        std::string		mDayCycleName;
    };

    typedef std::pair<LLSettingsSky::ptr_t, LLSettingsWater::ptr_t> fixedEnvironment_t;

    typedef std::map<std::string, LLSettingsBase::ptr_t>    namedSettingMap_t;
    typedef std::pair<std::string, LLUUID>                  name_id_t;
    typedef std::vector<name_id_t>                          list_name_id_t;
    typedef boost::signals2::signal<void()>                 change_signal_t;
    typedef std::function<void(S32, EnvironmentInfo::ptr_t)>     environment_apply_fn;

    typedef std::array<F32, 4>                              altitude_list_t;

    virtual ~LLEnvironment();

    void                        loadPreferences();
    void                        updatePreferences();
    const UserPrefs &           getPreferences() const { return mUserPrefs; }

    bool                        canEdit() const;
    bool                        isExtendedEnvironmentEnabled() const;
    bool                        isInventoryEnabled() const;
    bool                        canAgentUpdateParcelEnvironment(bool useselected = false) const;
    bool                        canAgentUpdateRegionEnvironment() const;

    LLSettingsSky::ptr_t        getCurrentSky() const { return mCurrentEnvironment->getSky(); }
    LLSettingsWater::ptr_t      getCurrentWater() const { return mCurrentEnvironment->getWater(); }

    void                        update(const LLViewerCamera * cam);

    void                        updateGLVariablesForSettings(LLGLSLShader *shader, const LLSettingsBase::ptr_t &psetting);
    void                        updateShaderUniforms(LLGLSLShader *shader);

    void                        setSelectedEnvironment(EnvSelection_t env, F64Seconds transition = TRANSITION_DEFAULT, bool forced = false);
    EnvSelection_t              getSelectedEnvironment() const                  { return mSelectedEnvironment; }

    bool                        hasEnvironment(EnvSelection_t env);
    void                        setEnvironment(EnvSelection_t env, const LLSettingsDay::ptr_t &pday, S64Seconds daylength, S64Seconds dayoffset);
    void                        setEnvironment(EnvSelection_t env, fixedEnvironment_t fixed);
    void                        setEnvironment(EnvSelection_t env, const LLSettingsBase::ptr_t &fixed); 
    void                        setEnvironment(EnvSelection_t env, const LLSettingsSky::ptr_t & fixed) { setEnvironment(env, fixedEnvironment_t(fixed, LLSettingsWater::ptr_t())); }
    void                        setEnvironment(EnvSelection_t env, const LLSettingsWater::ptr_t & fixed) { setEnvironment(env, fixedEnvironment_t(LLSettingsSky::ptr_t(), fixed)); }
    void                        setEnvironment(EnvSelection_t env, const LLSettingsSky::ptr_t & fixeds, const LLSettingsWater::ptr_t & fixedw) { setEnvironment(env, fixedEnvironment_t(fixeds, fixedw)); }
    void                        setEnvironment(EnvSelection_t env, const LLUUID &assetId);

    void                        clearEnvironment(EnvSelection_t env);
    LLSettingsDay::ptr_t        getEnvironmentDay(EnvSelection_t env);
    S64Seconds                  getEnvironmentDayLength(EnvSelection_t env);
    S64Seconds                  getEnvironmentDayOffset(EnvSelection_t env);
    fixedEnvironment_t          getEnvironmentFixed(EnvSelection_t env);
    LLSettingsSky::ptr_t        getEnvironmentFixedSky(EnvSelection_t env)      { return getEnvironmentFixed(env).first; };
    LLSettingsWater::ptr_t      getEnvironmentFixedWater(EnvSelection_t env)    { return getEnvironmentFixed(env).second; };

    void                        updateEnvironment(F64Seconds transition = TRANSITION_DEFAULT, bool forced = false);

    void                        addSky(const LLSettingsSky::ptr_t &sky);
    void                        addWater(const LLSettingsWater::ptr_t &sky);
    void                        addDayCycle(const LLSettingsDay::ptr_t &day);

    list_name_id_t              getSkyList() const;
    list_name_id_t              getWaterList() const;
    list_name_id_t              getDayCycleList() const;

    // *LAPRAS* TODO : Remove these vvv
    LLSettingsSky::ptr_t        findSkyByName(std::string name) const;
    LLSettingsWater::ptr_t      findWaterByName(std::string name) const;
    LLSettingsDay::ptr_t        findDayCycleByName(std::string name) const;
    // *LAPRAS* TODO : Remove these ^^^

    inline LLVector2            getCloudScrollDelta() const { return mCloudScrollDelta; }

    F32                         getCamHeight() const;
    F32                         getWaterHeight() const;
    bool                        getIsDayTime() const;   // "Day Time" is defined as the sun above the horizon.
    bool                        getIsNightTime() const { return !getIsDayTime(); } // "Not Day Time" 

    inline F32                  getSceneLightStrength() const { return mSceneLightStrength; }
    inline void                 setSceneLightStrength(F32 light_strength) { mSceneLightStrength = light_strength; }

    inline LLVector4            getLightDirection() const { return ((mCurrentEnvironment->getSky()) ? LLVector4(mCurrentEnvironment->getSky()->getLightDirection(), 0.0f) : LLVector4()); }
    inline LLVector4            getClampedLightDirection() const { return LLVector4(mCurrentEnvironment->getSky()->getClampedLightDirection(), 0.0f); }
    inline LLVector4            getRotatedLight() const { return mRotatedLight; }

    static LLSettingsWater::ptr_t   createWaterFromLegacyPreset(const std::string filename);
    static LLSettingsSky::ptr_t createSkyFromLegacyPreset(const std::string filename);
    static LLSettingsDay::ptr_t createDayCycleFromLegacyPreset(const std::string filename);

    // Construct a new day cycle based on the environment.  Replacing either the water or the sky tracks.
    LLSettingsDay::ptr_t        createDayCycleFromEnvironment(EnvSelection_t env, LLSettingsBase::ptr_t settings);

    //-------------------------------------------
    connection_t                setSkyListChange(const change_signal_t::slot_type& cb);
    connection_t                setWaterListChange(const change_signal_t::slot_type& cb);
    connection_t                setDayCycleListChange(const change_signal_t::slot_type& cb);

    void                        requestRegionEnvironment();

    void                        onLegacyRegionSettings(LLSD data);

    void                        requestRegion();
    void                        updateRegion(const LLUUID &asset_id, S32 day_length, S32 day_offset);
    void                        updateRegion(LLSettingsDay::ptr_t &pday, S32 day_length, S32 day_offset);
    void                        updateRegion(LLSettingsSky::ptr_t &psky, S32 day_length, S32 day_offset);
    void                        updateRegion(LLSettingsWater::ptr_t &pwater, S32 day_length, S32 day_offset);
    void                        resetRegion();
    void                        requestParcel(S32 parcel_id);
    void                        updateParcel(S32 parcel_id, const LLUUID &asset_id, S32 day_length, S32 day_offset);
    void                        updateParcel(S32 parcel_id, LLSettingsDay::ptr_t &pday, S32 day_length, S32 day_offset);
    void                        updateParcel(S32 parcel_id, LLSettingsSky::ptr_t &psky, S32 day_length, S32 day_offset);
    void                        updateParcel(S32 parcel_id, LLSettingsWater::ptr_t &pwater, S32 day_length, S32 day_offset);
    void                        resetParcel(S32 parcel_id);

    void                        selectAgentEnvironment();

    S32                         calculateSkyTrackForAltitude(F64 altitude);

    const altitude_list_t &     getRegionAltitudes() const { return mTrackAltitudes; }

protected:
    virtual void                initSingleton();

private:
    class DayInstance
    {
    public:
        enum InstanceType_t
        {
            TYPE_INVALID,
            TYPE_FIXED,
            TYPE_CYCLED
        };
        typedef std::shared_ptr<DayInstance> ptr_t;

                                    DayInstance();
        virtual                     ~DayInstance() { };

        virtual void                update(F64Seconds);

        void                        setDay(const LLSettingsDay::ptr_t &pday, S64Seconds daylength, S64Seconds dayoffset);
        void                        setSky(const LLSettingsSky::ptr_t &psky);
        void                        setWater(const LLSettingsWater::ptr_t &pwater);

        void                        initialize();
        bool                        isInitialized();

        void                        clear();

        void                        setSkyTrack(S32 trackno);

        LLSettingsDay::ptr_t        getDayCycle() const     { return mDayCycle; }
        LLSettingsSky::ptr_t        getSky() const          { return mSky; }
        LLSettingsWater::ptr_t      getWater() const        { return mWater; }
        S64Seconds                  getDayLength() const    { return mDayLength; }
        S64Seconds                  getDayOffset() const    { return mDayOffset; }
        S32                         getSkyTrack() const     { return mSkyTrack; }

        virtual void                animate();

        void                        setBlenders(const LLSettingsBlender::ptr_t &skyblend, const LLSettingsBlender::ptr_t &waterblend);

    protected:
        LLSettingsDay::ptr_t        mDayCycle;
        LLSettingsSky::ptr_t        mSky;
        LLSettingsWater::ptr_t      mWater;
        S32                         mSkyTrack;

        InstanceType_t              mType;
        bool                        mInitialized;

        S64Seconds                  mDayLength;
        S64Seconds                  mDayOffset;
        S32                         mLastTrackAltitude;

        LLSettingsBlender::ptr_t    mBlenderSky;
        LLSettingsBlender::ptr_t    mBlenderWater;

        F64                         secondsToKeyframe(S64Seconds seconds);
    };
    typedef std::array<DayInstance::ptr_t, ENV_END> InstanceArray_t;


    class DayTransition : public DayInstance
    {
    public:
                                    DayTransition(const LLSettingsSky::ptr_t &skystart, const LLSettingsWater::ptr_t &waterstart, DayInstance::ptr_t &end, S64Seconds time);
        virtual                     ~DayTransition() { };

        virtual void                update(F64Seconds);
        virtual void                animate();

    protected:
        LLSettingsSky::ptr_t        mStartSky;
        LLSettingsWater::ptr_t      mStartWater;
        DayInstance::ptr_t          mNextInstance;
        S64Seconds                  mTransitionTime;
    };

    static const F32            SUN_DELTA_YAW;
    static const F32            NIGHTTIME_ELEVATION_COS;

    typedef std::map<LLUUID, LLSettingsBase::ptr_t> AssetSettingMap_t;

    LLVector2                   mCloudScrollDelta;  // cumulative cloud delta

    InstanceArray_t             mEnvironments;

    EnvSelection_t              mSelectedEnvironment;
    DayInstance::ptr_t          mCurrentEnvironment;

    LLSettingsSky::ptr_t        mSelectedSky;
    LLSettingsWater::ptr_t      mSelectedWater;
    LLSettingsDay::ptr_t        mSelectedDay;

    LLSettingsBlender::ptr_t    mBlenderSky;
    LLSettingsBlender::ptr_t    mBlenderWater;

    typedef std::vector<LLSettingsSky::ptr_t> SkyList_t;
    typedef std::vector<LLSettingsWater::ptr_t> WaterList_t;
    typedef std::vector<LLSettingsDay::ptr_t> DayList_t;

    namedSettingMap_t           mSkysByName;
    AssetSettingMap_t           mSkysById;

    namedSettingMap_t           mWaterByName;
    AssetSettingMap_t           mWaterById;

    namedSettingMap_t           mDayCycleByName;
    AssetSettingMap_t           mDayCycleById;

    F32                         mSceneLightStrength;
    LLVector4                   mRotatedLight;

    UserPrefs                   mUserPrefs;

    change_signal_t             mSkyListChange;
    change_signal_t             mWaterListChange;
    change_signal_t             mDayCycleListChange;

    S32                         mCurrentTrack;
    altitude_list_t             mTrackAltitudes;

    DayInstance::ptr_t          getEnvironmentInstance(EnvSelection_t env, bool create = false);

    DayInstance::ptr_t          getSelectedEnvironmentInstance();


    //void addSky(const LLUUID &id, const LLSettingsSky::ptr_t &sky);
    void removeSky(const std::string &name);
    //void removeSky(const LLUUID &id);
    void clearAllSkys();

    //void addWater(const LLUUID &id, const LLSettingsSky::ptr_t &sky);
    void removeWater(const std::string &name);
    //void removeWater(const LLUUID &id);
    void clearAllWater();

    //void addDayCycle(const LLUUID &id, const LLSettingsSky::ptr_t &sky);
    void removeDayCycle(const std::string &name);
    //void removeDayCycle(const LLUUID &id);
    void clearAllDayCycles();


    void updateCloudScroll();

    void onParcelChange();

    void coroRequestEnvironment(S32 parcel_id, environment_apply_fn apply);
    void coroUpdateEnvironment(S32 parcel_id, LLSettingsDay::ptr_t pday, S32 day_length, S32 day_offset, environment_apply_fn apply);
    void coroResetEnvironment(S32 parcel_id, environment_apply_fn apply);

    void recordEnvironment(S32 parcel_id, EnvironmentInfo::ptr_t environment);

    void onAgentPositionHasChanged(const LLVector3 &localpos);

    void onSetEnvAssetLoaded(EnvSelection_t env, LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status);
    void onUpdateParcelAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, S32 parcel_id, S32 day_length, S32 day_offset);

    //=========================================================================
    void                        legacyLoadAllPresets();
    static std::string          getSysDir(const std::string &subdir);
    static std::string          getUserDir(const std::string &subdir);

};

class LLTrackBlenderLoopingManual : public LLSettingsBlender
{
public:
    LLTrackBlenderLoopingManual(const LLSettingsBase::ptr_t &target, const LLSettingsDay::ptr_t &day, S32 trackno);

    F64                         setPosition(F64 position) override;
    virtual void                switchTrack(S32 trackno, F64 position) override;
    S32                         getTrack() const { return mTrackNo; }

    typedef std::shared_ptr<LLTrackBlenderLoopingManual> ptr_t;
protected:
    LLSettingsDay::TrackBound_t getBoundingEntries(F64 position);
    F64                         getSpanLength(const LLSettingsDay::TrackBound_t &bounds) const;

private:
    LLSettingsDay::ptr_t        mDay;
    S32                         mTrackNo;
    F64                         mPosition;

    LLSettingsDay::CycleTrack_t::iterator mEndMarker;
};

#endif // LL_ENVIRONMENT_H

