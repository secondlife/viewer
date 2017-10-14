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

#include "llmemory.h"
#include "llsd.h"

#include "llsettingssky.h"
#include "llsettingswater.h"

class LLViewerCamera;
class LLGLSLShader;

//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
class LLEnvironment : public LLSingleton<LLEnvironment>
{
    LLSINGLETON(LLEnvironment);
    LOG_CLASS(LLEnvironment);

public:
    class UserPrefs
    {
        friend class LLEnvironment;
    public:
        UserPrefs();

        bool        getUseRegionSettings() const { return mUseRegionSettings; }
        bool        getUseDayCycle() const { return mUseDayCycle; }
        bool        getUseFixedSky() const { return !getUseDayCycle(); }

        std::string getWaterPresetName() const { return mWaterPresetName; }
        std::string getSkyPresetName() const { return mSkyPresetName; }
        std::string getDayCycleName() const { return mDayCycleName; }

        void setUseRegionSettings(bool val);
        void setUseWaterPreset(const std::string& name);
        void setUseSkyPreset(const std::string& name);
        void setUseDayCycle(const std::string& name);

    private:
        void load();
        void store();

        bool			mUseRegionSettings;
        bool			mUseDayCycle;
        bool            mPersistEnvironment;
        std::string		mWaterPresetName;
        std::string		mSkyPresetName;
        std::string		mDayCycleName;
    };

    virtual ~LLEnvironment();

    void                    loadPreferences();
    const UserPrefs &       getPreferences() const { return mUserPrefs; }

    LLSettingsSky::ptr_t    getCurrentSky() const { return mCurrentSky; }
    LLSettingsWater::ptr_t  getCurrentWater() const { return mCurrentWater; }

    void                    update(const LLViewerCamera * cam);

    void                    updateGLVariablesForSettings(LLGLSLShader *shader, const LLSettingsBase::ptr_t &psetting);
    void                    updateShaderUniforms(LLGLSLShader *shader);

    void                    addSky(const LLSettingsSky::ptr_t &sky);
    void                    selectSky(const std::string &name);
    void                    addWater(const LLSettingsWater::ptr_t &sky);
    void                    selectWater(const std::string &name);

    inline LLVector2        getCloudScrollDelta() const { return mCloudScrollDelta; }

    F32                     getCamHeight() const;
    F32                     getWaterHeight() const;
    bool                    getIsDayTime() const;   // "Day Time" is defined as the sun above the horizon.
    bool                    getIsNightTime() const { return !getIsDayTime(); } // "Not Day Time" 

    inline F32              getSceneLightStrength() const { return mSceneLightStrength; }
    inline void             setSceneLightStrength(F32 light_strength) { mSceneLightStrength = light_strength; }

    inline LLVector4        getLightDirection() const { return LLVector4(mCurrentSky->getLightDirection(), 0.0f); }
    inline LLVector4        getClampedLightDirection() const { return LLVector4(mCurrentSky->getClampedLightDirection(), 0.0f); }
    inline LLVector4        getRotatedLight() const { return mRotatedLight; }


private:
    static const F32        SUN_DELTA_YAW;
    static const F32        NIGHTTIME_ELEVATION_COS;

    typedef std::map<std::string, LLSettingsBase::ptr_t> NamedSettingMap_t;
    typedef std::map<LLUUID, LLSettingsBase::ptr_t> AssetSettingMap_t;

    LLVector2               mCloudScrollDelta;  // cumulative cloud delta

    LLSettingsSky::ptr_t    mCurrentSky;
    LLSettingsWater::ptr_t  mCurrentWater;

    NamedSettingMap_t       mSkysByName;
    AssetSettingMap_t       mSkysById;

    NamedSettingMap_t       mWaterByName;
    AssetSettingMap_t       mWaterById;

    F32                     mSceneLightStrength;
    LLVector4               mRotatedLight;

    UserPrefs               mUserPrefs;

    //void addSky(const LLUUID &id, const LLSettingsSky::ptr_t &sky);
    void removeSky(const std::string &name);
    //void removeSky(const LLUUID &id);
    void clearAllSkys();

    //void addWater(const LLUUID &id, const LLSettingsSky::ptr_t &sky);
    void removeWater(const std::string &name);
    //void removeWater(const LLUUID &id);
    void clearAllWater();

    void updateCloudScroll();
};


#endif // LL_ENVIRONMENT_H

