/**
* @file llsettingsvo.h
* @author Rider Linden
* @brief Subclasses for viewer specific settings behaviors.
*
* $LicenseInfo:2011&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2017, Linden Research, Inc.
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

#ifndef LL_SETTINGS_VO_H
#define LL_SETTINGS_VO_H

#include "llsettingsbase.h"
#include "llsettingssky.h"
#include "llsettingswater.h"
#include "llsettingsdaycycle.h"

#include "llsdserialize.h"

#include "llextendedstatus.h"
#include <boost/signals2.hpp>

class LLInventoryItem;
class LLGLSLShader;

//=========================================================================
class LLSettingsVOBase : public LLSettingsBase
{
public:
    typedef std::function<void(LLUUID asset_id, LLSettingsBase::ptr_t settins, S32 status, LLExtStat extstat)>  asset_download_fn;
    typedef std::function<void(LLInventoryItem *inv_item, LLSettingsBase::ptr_t settings, S32 status, LLExtStat extstat)> inventory_download_fn;
    typedef std::function<void(LLUUID asset_id, LLUUID inventory_id, LLUUID object_id, LLSD results)>           inventory_result_fn;

    static void     createNewInventoryItem(LLSettingsType::type_e stype, const LLUUID& parent_id, std::function<void(const LLUUID&)> created_cb);
    static void     createNewInventoryItem(LLSettingsType::type_e stype, const LLUUID &parent_id, inventory_result_fn callback = inventory_result_fn());
    static void     createInventoryItem(const LLSettingsBase::ptr_t &settings, const LLUUID &parent_id, std::string settings_name, inventory_result_fn callback = inventory_result_fn());
    static void     createInventoryItem(const LLSettingsBase::ptr_t &settings, U32 next_owner_perm, const LLUUID &parent_id, std::string settings_name, inventory_result_fn callback = inventory_result_fn());

    static void     updateInventoryItem(const LLSettingsBase::ptr_t &settings, LLUUID inv_item_id, inventory_result_fn callback = inventory_result_fn(), bool update_name = true);
    static void     updateInventoryItem(const LLSettingsBase::ptr_t &settings, LLUUID object_id, LLUUID inv_item_id, inventory_result_fn callback = inventory_result_fn());

    static void     getSettingsAsset(const LLUUID &assetId, asset_download_fn callback);
    static void     getSettingsInventory(const LLUUID &inventoryId, inventory_download_fn callback = inventory_download_fn());

    static bool     exportFile(const LLSettingsBase::ptr_t &settings, const std::string &filename, LLSDSerialize::ELLSD_Serialize format = LLSDSerialize::LLSD_NOTATION);
    static LLSettingsBase::ptr_t importFile(const std::string &filename);
    static LLSettingsBase::ptr_t createFromLLSD(const LLSD &settings);

private:
    struct SettingsSaveData
    {
        typedef std::shared_ptr<SettingsSaveData> ptr_t;
        std::string             mType;
        std::string             mTempFile;
        LLSettingsBase::ptr_t   mSettings;
        LLTransactionID         mTransId;
    };

    LLSettingsVOBase() {}

    static void     onInventoryItemCreated(const LLUUID &inventoryId, LLSettingsBase::ptr_t settings, inventory_result_fn callback);

    static void     onAgentAssetUploadComplete(LLUUID itemId, LLUUID newAssetId, LLUUID newItemId, LLSD response, LLSettingsBase::ptr_t psettings, inventory_result_fn callback);
    static void     onTaskAssetUploadComplete(LLUUID itemId, LLUUID taskId, LLUUID newAssetId, LLSD response, LLSettingsBase::ptr_t psettings, inventory_result_fn callback);

    static void     onAssetDownloadComplete(const LLUUID &asset_id, S32 status, LLExtStat ext_status, asset_download_fn callback);
};

//=========================================================================
class LLSettingsVOSky : public LLSettingsSky
{
public:
    LLSettingsVOSky(const LLSD &data, bool advanced = false);

    static ptr_t    buildSky(LLSD settings);

    static ptr_t buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings, LLSD &messages);
    static ptr_t    buildDefaultSky();
    virtual ptr_t   buildClone() SETTINGS_OVERRIDE;

    static ptr_t buildFromLegacyPresetFile(const std::string &name, const std::string &path, LLSD &messages);

    static LLSD     convertToLegacy(const ptr_t &, bool isAdvanced);

    bool isAdvanced() const { return  m_isAdvanced; }

protected:
    LLSettingsVOSky();

    // Interpret new settings in terms of old atmospherics params
    static void convertAtmosphericsToLegacy(LLSD& legacy, LLSD& settings);

    virtual void    updateSettings() override;

    virtual void    applyToUniforms(void*) override;
    virtual void    applySpecial(void *, bool) override;

    virtual parammapping_t getParameterMap() const override;

    bool m_isAdvanced = false;
    F32 mSceneLightStrength = 3.0f;
};

//=========================================================================
class LLSettingsVOWater : public LLSettingsWater
{
public:
    LLSettingsVOWater(const LLSD &data);

    static ptr_t    buildWater(LLSD settings);

    static ptr_t buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings, LLSD &messages);
    static ptr_t    buildDefaultWater();
    virtual ptr_t   buildClone() SETTINGS_OVERRIDE;

    static ptr_t buildFromLegacyPresetFile(const std::string &name, const std::string &path, LLSD &messages);

    static LLSD     convertToLegacy(const ptr_t &);

protected:
    LLSettingsVOWater();

    virtual void    updateSettings() override;
    virtual void    applyToUniforms(void*) override;
    virtual void    applySpecial(void *, bool) override;

    virtual parammapping_t getParameterMap() const override;


private:
    static const F32 WATER_FOG_LIGHT_CLAMP;

};

//=========================================================================
class LLSettingsVODay : public LLSettingsDay
{
public:
    typedef std::function<void(LLSettingsDay::ptr_t day)>  asset_built_fn;

    // Todo: find a way to make this cnstructor private
    // It shouldn't be used outside shared_prt and LLSettingsVODay
    // outside of settings only use buildDay(settings)
    LLSettingsVODay(const LLSD &data);

    static ptr_t    buildDay(LLSD settings);

    static ptr_t buildFromLegacyPreset(const std::string &name, const std::string &path, const LLSD &oldsettings, LLSD &messages);
    static ptr_t buildFromLegacyPresetFile(const std::string &name, const std::string &path, LLSD &messages);
    static ptr_t    buildFromLegacyMessage(const LLUUID &regionId, LLSD daycycle, LLSD skys, LLSD water);
    static ptr_t    buildDefaultDayCycle();
    static ptr_t    buildFromEnvironmentMessage(LLSD settings);
    static void     buildFromOtherSetting(LLSettingsBase::ptr_t settings, asset_built_fn cb);
    virtual ptr_t   buildClone() SETTINGS_OVERRIDE;
    virtual ptr_t   buildDeepCloneAndUncompress() SETTINGS_OVERRIDE;

    static LLSD     convertToLegacy(const ptr_t &);

    virtual LLSettingsSkyPtr_t      getDefaultSky() const override;
    virtual LLSettingsWaterPtr_t    getDefaultWater() const override;
    virtual LLSettingsSkyPtr_t      buildSky(LLSD) const override;
    virtual LLSettingsWaterPtr_t    buildWater(LLSD) const override;

protected:
    LLSettingsVODay();

private:
    static void     combineIntoDayCycle(LLSettingsDay::ptr_t, LLSettingsBase::ptr_t, asset_built_fn);
};


#endif
