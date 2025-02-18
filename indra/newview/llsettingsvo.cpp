/**
* @file llsettingsvo.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llviewercontrol.h"
#include "llsettingsvo.h"

#include "pipeline.h"

#include <algorithm>
#include <cstdio>
#include "lltrace.h"
#include "llfasttimer.h"
#include "v3colorutil.h"

#include "llglslshader.h"
#include "llviewershadermgr.h"

#include "llagent.h"
#include "llassettype.h"
#include "llfloaterperms.h"
#include "llnotificationsutil.h"

#include "llviewerregion.h"
#include "llviewerassetupload.h"
#include "llviewerinventory.h"

#include "llenvironment.h"
#include "llsky.h"

#include "llpermissions.h"

#include "llinventorymodel.h"
#include "llassetstorage.h"
#include "llfilesystem.h"
#include "lldrawpoolwater.h"

#include <boost/algorithm/string/replace.hpp>
#include "llinventoryobserver.h"
#include "llinventorydefines.h"
#include "llworld.h"

#include "lltrans.h"

#undef  VERIFY_LEGACY_CONVERSION

extern bool gCubeSnapshot;

//=========================================================================
namespace
{
    LLSD ensure_array_4(LLSD in, F32 fill);
    LLSD read_legacy_preset_data(const std::string &name, const std::string& path, LLSD  &messages);

    //-------------------------------------------------------------------------
    class LLSettingsInventoryCB : public LLInventoryCallback
    {
    public:
        typedef std::function<void(const LLUUID &)> callback_t;

        LLSettingsInventoryCB(callback_t cbfn) :
            mCbfn(cbfn)
        { }

        void fire(const LLUUID& inv_item) override  { if (mCbfn) mCbfn(inv_item); }

    private:
        callback_t  mCbfn;
    };

    //-------------------------------------------------------------------------
}


//=========================================================================
void LLSettingsVOBase::createNewInventoryItem(LLSettingsType::type_e stype, const LLUUID& parent_id, std::function<void(const LLUUID&)> created_cb)
{
    inventory_result_fn cb = NULL;

    if (created_cb != NULL)
    {
        cb = [created_cb](LLUUID asset_id, LLUUID inventory_id, LLUUID object_id, LLSD results)
        {
            created_cb(inventory_id);
        };
    }
    createNewInventoryItem(stype, parent_id, cb);
}

void LLSettingsVOBase::createNewInventoryItem(LLSettingsType::type_e stype, const LLUUID &parent_id, inventory_result_fn callback)
{
    LLTransactionID tid;
    U32 nextOwnerPerm = LLFloaterPerms::getNextOwnerPerms("Settings");
    nextOwnerPerm |= PERM_COPY;

    if (!LLEnvironment::instance().isInventoryEnabled())
    {
        LL_WARNS("SETTINGS") << "Region does not support settings inventory objects." << LL_ENDL;
        LLNotificationsUtil::add("SettingsUnsuported");
        return;
    }

    tid.generate();

    LLPointer<LLInventoryCallback> cb = new LLSettingsInventoryCB([callback](const LLUUID &inventoryId) {
        LLSettingsVOBase::onInventoryItemCreated(inventoryId, LLSettingsBase::ptr_t(), callback);
    });

    create_inventory_settings(gAgent.getID(), gAgent.getSessionID(),
        parent_id, LLTransactionID::tnull,
        LLSettingsType::getDefaultName(stype), "",
        stype, nextOwnerPerm, cb);
}


void LLSettingsVOBase::createInventoryItem(const LLSettingsBase::ptr_t &settings, const LLUUID &parent_id, std::string settings_name, inventory_result_fn callback)
{
    U32 nextOwnerPerm = LLFloaterPerms::getNextOwnerPerms("Settings");
    createInventoryItem(settings, nextOwnerPerm, parent_id, settings_name, callback);
}

void LLSettingsVOBase::createInventoryItem(const LLSettingsBase::ptr_t &settings, U32 next_owner_perm, const LLUUID &parent_id, std::string settings_name, inventory_result_fn callback)
{
    LLTransactionID tid;

    if (!LLEnvironment::instance().isInventoryEnabled())
    {
        LL_WARNS("SETTINGS") << "Region does not support settings inventory objects." << LL_ENDL;
        LLNotificationsUtil::add("SettingsUnsuported");
        return;
    }

    tid.generate();

    LLPointer<LLInventoryCallback> cb = new LLSettingsInventoryCB([settings, callback](const LLUUID &inventoryId) {
            LLSettingsVOBase::onInventoryItemCreated(inventoryId, settings, callback);
        });

    if (settings_name.empty())
    {
        settings_name = settings->getName();
    }
    create_inventory_settings(gAgent.getID(), gAgent.getSessionID(),
        parent_id, tid,
        settings_name, "",
        settings->getSettingsTypeValue(), next_owner_perm, cb);
}

void LLSettingsVOBase::onInventoryItemCreated(const LLUUID &inventoryId, LLSettingsBase::ptr_t settings, inventory_result_fn callback)
{
    LLViewerInventoryItem *pitem = gInventory.getItem(inventoryId);
    if (pitem)
    {
        LLPermissions perm = pitem->getPermissions();
        if (perm.getMaskEveryone() != PERM_COPY)
        {
            perm.setMaskEveryone(PERM_COPY);
            pitem->setPermissions(perm);
            pitem->updateServer(false);
        }
    }
    if (!settings)
    {   // The item was created as new with no settings passed in.  Simulator should have given it the default for the type... check ID,
        // no need to upload asset.
        LLUUID asset_id;
        if (pitem)
        {
            asset_id = pitem->getAssetUUID();
        }
        if (callback)
            callback(asset_id, inventoryId, LLUUID::null, LLSD());
        return;
    }
    // We need to update some inventory stuff here.... maybe.
    updateInventoryItem(settings, inventoryId, callback, false);
}

void LLSettingsVOBase::updateInventoryItem(const LLSettingsBase::ptr_t &settings, LLUUID inv_item_id, inventory_result_fn callback, bool update_name)
{
    const LLViewerRegion* region = gAgent.getRegion();
    if (!region)
    {
        LL_WARNS("SETTINGS") << "Not connected to a region, cannot save setting." << LL_ENDL;
        return;
    }

    std::string agent_url(region->getCapability("UpdateSettingsAgentInventory"));

    if (!LLEnvironment::instance().isInventoryEnabled())
    {
        LL_WARNS("SETTINGS") << "Region does not support settings inventory objects." << LL_ENDL;
        LLNotificationsUtil::add("SettingsUnsuported");
        return;
    }

    LLViewerInventoryItem *inv_item = gInventory.getItem(inv_item_id);
    if (inv_item)
    {
        bool need_update(false);
        LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(inv_item);

        if (settings->getFlag(LLSettingsBase::FLAG_NOTRANS) && new_item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
        {
            LLPermissions perm(inv_item->getPermissions());
            perm.setBaseBits(LLUUID::null, false, PERM_TRANSFER);
            perm.setOwnerBits(LLUUID::null, false, PERM_TRANSFER);
            new_item->setPermissions(perm);
            need_update |= true;
        }
        if (update_name && (settings->getName() != new_item->getName()))
        {
            new_item->rename(settings->getName());
            settings->setName(new_item->getName()); // account for corrections
            need_update |= true;
        }
        if (need_update)
        {
            new_item->updateServer(false);
            gInventory.updateItem(new_item);
            gInventory.notifyObservers();
        }
    }

    std::stringstream buffer;
    LLSD settingdata(settings->getSettings());
    LLSDSerialize::serialize(settingdata, buffer, LLSDSerialize::LLSD_NOTATION);

    LLResourceUploadInfo::ptr_t uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(inv_item_id, LLAssetType::AT_SETTINGS, buffer.str(),
        [settings, callback](LLUUID itemId, LLUUID newAssetId, LLUUID newItemId, LLSD response)
        {
            LLSettingsVOBase::onAgentAssetUploadComplete(itemId, newAssetId, newItemId, response, settings, callback);
        },
        nullptr);

    LLViewerAssetUpload::EnqueueInventoryUpload(agent_url, uploadInfo);
}

void LLSettingsVOBase::updateInventoryItem(const LLSettingsBase::ptr_t &settings, LLUUID object_id, LLUUID inv_item_id, inventory_result_fn callback)
{
    const LLViewerRegion* region = gAgent.getRegion();
    if (!region)
    {
        LL_WARNS("SETTINGS") << "Not connected to a region, cannot save setting." << LL_ENDL;
        return;
    }

    std::string agent_url(region->getCapability("UpdateSettingsAgentInventory"));

    if (!LLEnvironment::instance().isInventoryEnabled())
    {
        LL_WARNS("SETTINGS") << "Region does not support settings inventory objects." << LL_ENDL;
        LLNotificationsUtil::add("SettingsUnsuported");
        return;
    }

    std::stringstream buffer;
    LLSD settingdata(settings->getSettings());

    LLSDSerialize::serialize(settingdata, buffer, LLSDSerialize::LLSD_NOTATION);

    LLResourceUploadInfo::ptr_t uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(object_id, inv_item_id, LLAssetType::AT_SETTINGS, buffer.str(),
        [settings, callback](LLUUID itemId, LLUUID taskId, LLUUID newAssetId, LLSD response)
        {
            LLSettingsVOBase::onTaskAssetUploadComplete(itemId, taskId, newAssetId, response, settings, callback);
        },
        nullptr);

    LLViewerAssetUpload::EnqueueInventoryUpload(agent_url, uploadInfo);
}

void LLSettingsVOBase::onAgentAssetUploadComplete(LLUUID itemId, LLUUID newAssetId, LLUUID newItemId, LLSD response, LLSettingsBase::ptr_t psettings, inventory_result_fn callback)
{
    LL_INFOS("SETTINGS") << "itemId:" << itemId << " newAssetId:" << newAssetId << " newItemId:" << newItemId << " response:" << response << LL_ENDL;
    psettings->setAssetId(newAssetId);
    if (callback)
        callback( newAssetId, itemId, LLUUID::null, response );
}

void LLSettingsVOBase::onTaskAssetUploadComplete(LLUUID itemId, LLUUID taskId, LLUUID newAssetId, LLSD response, LLSettingsBase::ptr_t psettings, inventory_result_fn callback)
{
    LL_INFOS("SETTINGS") << "Upload to task complete!" << LL_ENDL;
    psettings->setAssetId(newAssetId);
    if (callback)
        callback(newAssetId, itemId, taskId, response);
}


void LLSettingsVOBase::getSettingsAsset(const LLUUID &assetId, LLSettingsVOBase::asset_download_fn callback)
{
    gAssetStorage->getAssetData(assetId, LLAssetType::AT_SETTINGS,
        [callback](const LLUUID &asset_id, LLAssetType::EType, void *, S32 status, LLExtStat ext_status)
            { onAssetDownloadComplete(asset_id, status, ext_status, callback); },
        nullptr, true);

}

void LLSettingsVOBase::onAssetDownloadComplete(const LLUUID &asset_id, S32 status, LLExtStat ext_status, LLSettingsVOBase::asset_download_fn callback)
{
    LLSettingsBase::ptr_t settings;
    if (!status)
    {
        LLFileSystem file(asset_id, LLAssetType::AT_SETTINGS, LLFileSystem::READ);
        S32 size = file.getSize();

        std::string buffer(size + 1, '\0');
        file.read((U8 *)buffer.data(), size);

        std::stringstream llsdstream(buffer);
        LLSD llsdsettings;

        if (LLSDSerialize::deserialize(llsdsettings, llsdstream, LLSDSerialize::SIZE_UNLIMITED))
        {
            settings = createFromLLSD(llsdsettings);
        }

        if (!settings)
        {
            status = 1;
            LL_WARNS("SETTINGS") << "Unable to create settings object." << LL_ENDL;
        }
        else
        {
            settings->setAssetId(asset_id);
        }
    }
    else
    {
        LL_WARNS("SETTINGS") << "Error retrieving asset " << asset_id << ". Status code=" << status << "(" << LLAssetStorage::getErrorString(status) << ") ext_status=" << (U32)ext_status << LL_ENDL;
    }
    if (callback)
        callback(asset_id, settings, status, ext_status);
}

void LLSettingsVOBase::getSettingsInventory(const LLUUID &inventoryId, inventory_download_fn callback)
{

}

bool LLSettingsVOBase::exportFile(const LLSettingsBase::ptr_t &settings, const std::string &filename, LLSDSerialize::ELLSD_Serialize format)
{
    try
    {
        std::ofstream file(filename, std::ios::out | std::ios::trunc);
        file.exceptions(std::ios_base::failbit | std::ios_base::badbit);

        if (!file)
        {
            LL_WARNS("SETTINGS") << "Unable to open '" << filename << "' for writing." << LL_ENDL;
            return false;
        }

        LLSDSerialize::serialize(settings->getSettings(), file, format);
    }
    catch (const std::ios_base::failure &e)
    {
        LL_WARNS("SETTINGS") << "Unable to save settings to file '" << filename << "': " << e.what() << LL_ENDL;
        return false;
    }

    return true;
}

LLSettingsBase::ptr_t LLSettingsVOBase::importFile(const std::string &filename)
{
    LLSD settings;

    try
    {
        std::ifstream file(filename, std::ios::in);
        file.exceptions(std::ios_base::failbit | std::ios_base::badbit);

        if (!file)
        {
            LL_WARNS("SETTINGS") << "Unable to open '" << filename << "' for reading." << LL_ENDL;
            return LLSettingsBase::ptr_t();
        }

        if (!LLSDSerialize::deserialize(settings, file, LLSDSerialize::SIZE_UNLIMITED))
        {
            LL_WARNS("SETTINGS") << "Unable to deserialize settings from '" << filename << "'" << LL_ENDL;
            return LLSettingsBase::ptr_t();
        }
    }
    catch (const std::ios_base::failure &e)
    {
        LL_WARNS("SETTINGS") << "Unable to save settings to file '" << filename << "': " << e.what() << LL_ENDL;
        return LLSettingsBase::ptr_t();
    }

    return createFromLLSD(settings);
}

LLSettingsBase::ptr_t LLSettingsVOBase::createFromLLSD(const LLSD &settings)
{
    if (!settings.has(SETTING_TYPE))
    {
        LL_WARNS("SETTINGS") << "No settings type in LLSD" << LL_ENDL;
        return LLSettingsBase::ptr_t();
    }

    std::string settingtype = settings[SETTING_TYPE].asString();

    LLSettingsBase::ptr_t psetting;

    if (settingtype == "water")
    {
        return LLSettingsVOWater::buildWater(settings);
    }
    else if (settingtype == "sky")
    {
        return LLSettingsVOSky::buildSky(settings);
    }
    else if (settingtype == "daycycle")
    {
        return LLSettingsVODay::buildDay(settings);
    }

    LL_WARNS("SETTINGS") << "Unable to determine settings type for '" << settingtype << "'." << LL_ENDL;
    return LLSettingsBase::ptr_t();

}

//=========================================================================
LLSettingsVOSky::LLSettingsVOSky(const LLSD &data, bool isAdvanced)
: LLSettingsSky(data)
, m_isAdvanced(isAdvanced)
{
}

LLSettingsVOSky::LLSettingsVOSky()
: LLSettingsSky()
, m_isAdvanced(false)
{
}

//-------------------------------------------------------------------------
LLSettingsSky::ptr_t LLSettingsVOSky::buildSky(LLSD settings)
{
    LLSettingsSky::validation_list_t validations = LLSettingsSky::validationList();

    LLSD results = LLSettingsBase::settingValidation(settings, validations);

    if (!results["success"].asBoolean())
    {
        LL_WARNS("SETTINGS") << "Sky setting validation failed!\n" << results << LL_ENDL;
        LLSettingsSky::ptr_t();
    }

    return std::make_shared<LLSettingsVOSky>(settings, true);
}


LLSettingsSky::ptr_t LLSettingsVOSky::buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings, LLSD &messages)
{

    LLSD newsettings = LLSettingsSky::translateLegacySettings(oldsettings);
    if (newsettings.isUndefined())
    {
        messages["REASONS"] = LLTrans::getString("SettingTranslateError", LLSDMap("NAME", name));
        return LLSettingsSky::ptr_t();
    }

    newsettings[SETTING_NAME] = name;

    LLSettingsSky::validation_list_t validations = LLSettingsSky::validationList();
    LLSD results = LLSettingsBase::settingValidation(newsettings, validations);
    if (!results["success"].asBoolean())
    {
        messages["REASONS"] = LLTrans::getString("SettingValidationError", LLSDMap("NAME", name));
        LL_WARNS("SETTINGS") << "Sky setting validation failed!\n" << results << LL_ENDL;
        LLSettingsSky::ptr_t();
    }

    LLSettingsSky::ptr_t skyp = std::make_shared<LLSettingsVOSky>(newsettings);

#ifdef VERIFY_LEGACY_CONVERSION
    LLSD oldsettings = LLSettingsVOSky::convertToLegacy(skyp, isAdvanced());

    if (!llsd_equals(oldsettings, oldsettings))
    {
        LL_WARNS("SKY") << "Conversion to/from legacy does not match!\n"
            << "Old: " << oldsettings
            << "new: " << oldsettings << LL_ENDL;
    }

#endif

    return skyp;
}

LLSettingsSky::ptr_t LLSettingsVOSky::buildFromLegacyPresetFile(const std::string &name, const std::string &path, LLSD &messages)
{
    LLSD legacy_data = read_legacy_preset_data(name, path, messages);

    if (!legacy_data)
    {   // messages filled in by read_legacy_preset_data
        LL_WARNS("SETTINGS") << "Could not load legacy Windlight \"" << name << "\" from " << path << LL_ENDL;
        return ptr_t();
    }

    return buildFromLegacyPreset(LLURI::unescape(name), legacy_data, messages);
}


LLSettingsSky::ptr_t LLSettingsVOSky::buildDefaultSky()
{
    static LLSD default_settings;

    if (!default_settings.size())
    {
        default_settings = LLSettingsSky::defaults();

        default_settings[SETTING_NAME] = DEFAULT_SETTINGS_NAME;

        LLSettingsSky::validation_list_t validations = LLSettingsSky::validationList();
        LLSD results = LLSettingsBase::settingValidation(default_settings, validations);
        if (!results["success"].asBoolean())
        {
            LL_WARNS("SETTINGS") << "Sky setting validation failed!\n" << results << LL_ENDL;
            LLSettingsSky::ptr_t();
        }
    }

    LLSettingsSky::ptr_t skyp = std::make_shared<LLSettingsVOSky>(default_settings);
    return skyp;
}

LLSettingsSky::ptr_t LLSettingsVOSky::buildClone()
{
    LLSD settings = cloneSettings();
    U32 flags = getFlags();

    LLSettingsSky::validation_list_t validations = LLSettingsSky::validationList();
    LLSD results = LLSettingsBase::settingValidation(settings, validations);
    if (!results["success"].asBoolean())
    {
        LL_WARNS("SETTINGS") << "Sky setting validation failed!\n" << results << LL_ENDL;
        LLSettingsSky::ptr_t();
    }

    LLSettingsSky::ptr_t skyp = std::make_shared<LLSettingsVOSky>(settings);
    skyp->setFlags(flags);
    return skyp;
}

void LLSettingsVOSky::convertAtmosphericsToLegacy(LLSD& legacy, LLSD& settings)
{
    // These may need to be inferred from new settings' density profiles
    // if the legacy settings values are not available.
    if (settings.has(SETTING_LEGACY_HAZE))
    {
        LLSD legacyhaze = settings[SETTING_LEGACY_HAZE];

        // work-around for setter formerly putting ambient values in wrong loc...
        if (legacyhaze.has(SETTING_AMBIENT))
        {
            legacy[SETTING_AMBIENT] = ensure_array_4(legacyhaze[SETTING_AMBIENT], 1.0f);
        }
        else if (settings.has(SETTING_AMBIENT))
        {
            legacy[SETTING_AMBIENT] = ensure_array_4(settings[SETTING_AMBIENT], 1.0f);
        }

        legacy[SETTING_BLUE_DENSITY] = ensure_array_4(legacyhaze[SETTING_BLUE_DENSITY], 1.0);
        legacy[SETTING_BLUE_HORIZON] = ensure_array_4(legacyhaze[SETTING_BLUE_HORIZON], 1.0);

        legacy[SETTING_DENSITY_MULTIPLIER]  = llsd::array(legacyhaze[SETTING_DENSITY_MULTIPLIER].asReal(), 0.0f, 0.0f, 1.0f);
        legacy[SETTING_DISTANCE_MULTIPLIER] = llsd::array(legacyhaze[SETTING_DISTANCE_MULTIPLIER].asReal(), 0.0f, 0.0f, 1.0f);

        legacy[SETTING_HAZE_DENSITY]        = llsd::array(legacyhaze[SETTING_HAZE_DENSITY], 0.0f, 0.0f, 1.0f);
        legacy[SETTING_HAZE_HORIZON]        = llsd::array(legacyhaze[SETTING_HAZE_HORIZON], 0.0f, 0.0f, 1.0f);
    }
}

LLSD LLSettingsVOSky::convertToLegacy(const LLSettingsSky::ptr_t &psky, bool isAdvanced)
{
    LLSD legacy(LLSD::emptyMap());
    LLSD settings = psky->getSettings();

    convertAtmosphericsToLegacy(legacy, settings);

    legacy[SETTING_CLOUD_COLOR] = ensure_array_4(settings[SETTING_CLOUD_COLOR], 1.0);
    legacy[SETTING_CLOUD_POS_DENSITY1] = ensure_array_4(settings[SETTING_CLOUD_POS_DENSITY1], 1.0);
    legacy[SETTING_CLOUD_POS_DENSITY2] = ensure_array_4(settings[SETTING_CLOUD_POS_DENSITY2], 1.0);
    legacy[SETTING_CLOUD_SCALE] = llsd::array(settings[SETTING_CLOUD_SCALE], LLSD::Real(0.0), LLSD::Real(0.0), LLSD::Real(1.0));
    legacy[SETTING_CLOUD_SCROLL_RATE] = settings[SETTING_CLOUD_SCROLL_RATE];
    legacy[SETTING_LEGACY_ENABLE_CLOUD_SCROLL] = llsd::array(LLSD::Boolean(!is_approx_zero((F32)settings[SETTING_CLOUD_SCROLL_RATE][0].asReal())),
        LLSD::Boolean(!is_approx_zero((F32)settings[SETTING_CLOUD_SCROLL_RATE][1].asReal())));
    legacy[SETTING_CLOUD_SHADOW] = llsd::array(settings[SETTING_CLOUD_SHADOW].asReal(), 0.0f, 0.0f, 1.0f);
    legacy[SETTING_GAMMA] = llsd::array(settings[SETTING_GAMMA], 0.0f, 0.0f, 1.0f);
    legacy[SETTING_GLOW] = ensure_array_4(settings[SETTING_GLOW], 1.0);
    legacy[SETTING_LIGHT_NORMAL] = ensure_array_4(psky->getLightDirection().getValue(), 0.0f);
    legacy[SETTING_MAX_Y] = llsd::array(settings[SETTING_MAX_Y], 0.0f, 0.0f, 1.0f);
    legacy[SETTING_STAR_BRIGHTNESS] = settings[SETTING_STAR_BRIGHTNESS].asReal() / 250.0f; // convert from 0-500 -> 0-2 ala pre-FS-compat changes
    legacy[SETTING_SUNLIGHT_COLOR] = ensure_array_4(settings[SETTING_SUNLIGHT_COLOR], 1.0f);

    LLVector3 dir = psky->getLightDirection();

    F32 phi     = asin(dir.mV[2]);
    F32 cos_phi = cosf(phi);
    F32 theta   = (cos_phi != 0) ? asin(dir.mV[1] / cos_phi) : 0.0f;

    theta = -theta;

    // get angles back into valid ranges for legacy viewer...
    //
    while (theta < 0)
    {
        theta += F_PI * 2;
    }

    if (theta > 4 * F_PI)
    {
        theta = fmod(theta, 2 * F_PI);
    }

    while (phi < -F_PI)
    {
        phi += 2 * F_PI;
    }

    if (phi > 3 * F_PI)
    {
        phi = F_PI + fmod(phi - F_PI, 2 * F_PI);
    }

    legacy[SETTING_LEGACY_EAST_ANGLE] = theta;
    legacy[SETTING_LEGACY_SUN_ANGLE]  = phi;

   return legacy;
}

//-------------------------------------------------------------------------
void LLSettingsVOSky::updateSettings()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_ENVIRONMENT;
    LLSettingsSky::updateSettings();
    LLVector3 sun_direction  = getSunDirection();
    LLVector3 moon_direction = getMoonDirection();

    // Want the dot prod of sun w/ high noon vector (0,0,1), which is just the z component
    F32 dp = llmax(sun_direction[2], 0.0f); // clamped to 0 when sun is down

    // Since WL scales everything by 2, there should always be at least a 2:1 brightness ratio
    // between sunlight and point lights in windlight to normalize point lights.
    //
    // After some A/B comparison of relesae vs EEP, tweak to allow strength to fall below 2
    // at night, for better match. (mSceneLightStrength is a divisor, so lower value means brighter
    // local lights)
    static LLCachedControl<F32> sdr(gSavedSettings, "RenderSunDynamicRange", 1.f);
    F32 sun_dynamic_range = llmax(sdr(), 0.0001f);
    mSceneLightStrength = 2.0f * (0.75f + sun_dynamic_range * dp);

    gSky.setSunAndMoonDirectionsCFR(sun_direction, moon_direction);
    gSky.setSunTextures(getSunTextureId(), getNextSunTextureId());
    gSky.setMoonTextures(getMoonTextureId(), getNextMoonTextureId());
    gSky.setCloudNoiseTextures(getCloudNoiseTextureId(), getNextCloudNoiseTextureId());
    gSky.setBloomTextures(getBloomTextureId(), getNextBloomTextureId());

    gSky.setSunScale(getSunScale());
    gSky.setMoonScale(getMoonScale());
}

void draw_color(LLShaderUniforms* shader, const LLColor3& col, S32 shader_key)
{
    // always identify as a radiance pass if desaturating irradiance is disabled
    static LLCachedControl<bool> desaturate_irradiance(gSavedSettings, "RenderDesaturateIrradiance", true);

    LLVector4 vect4(col.mV[0], col.mV[1], col.mV[2]);

    if (desaturate_irradiance && gCubeSnapshot && !gPipeline.mReflectionMapManager.isRadiancePass())
    { // maximize and remove tinting if this is an irradiance map render pass and the parameter feeds into the sky background color
        auto max_vec = [](LLVector4 col)
        {
            LLColor3 color(col);
            F32 h, s, l;
            color.calcHSL(&h, &s, &l);

            col.mV[0] = col.mV[1] = col.mV[2] = l;
            return col;
        };

        switch (shader_key)
        {
        case LLShaderMgr::BLUE_HORIZON:
        case LLShaderMgr::BLUE_DENSITY:
            vect4 = max_vec(vect4);
            break;
        }
    }

    //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << vect4 << LL_ENDL;
    shader->uniform3fv(shader_key, LLVector3(vect4.mV));
}

inline void draw_real(LLShaderUniforms* shader, F32 value, S32 shader_key)
{
    shader->uniform1f(shader_key, value);
}

void LLSettingsVOSky::applyToUniforms(void* ptarget)
{
    LLShaderUniforms* shader = &((LLShaderUniforms*)ptarget)[LLGLSLShader::SG_ANY];

    draw_color(shader, getAmbientColor(), LLShaderMgr::AMBIENT);
    draw_color(shader, getBlueDensity(), LLShaderMgr::BLUE_DENSITY);
    draw_color(shader, getBlueHorizon(), LLShaderMgr::BLUE_HORIZON);
    draw_real(shader, getHazeDensity(), LLShaderMgr::HAZE_DENSITY);
    draw_real(shader, getHazeHorizon(), LLShaderMgr::HAZE_HORIZON);
    draw_real(shader, getDensityMultiplier(), LLShaderMgr::DENSITY_MULTIPLIER);
    draw_real(shader, getDistanceMultiplier(), LLShaderMgr::DISTANCE_MULTIPLIER);
    draw_color(shader, getCloudPosDensity2(), LLShaderMgr::CLOUD_POS_DENSITY2);
    draw_real(shader, getCloudScale(), LLShaderMgr::CLOUD_SCALE);
    draw_real(shader, getCloudShadow(), LLShaderMgr::CLOUD_SHADOW);
    draw_real(shader, getCloudVariance(), LLShaderMgr::CLOUD_VARIANCE);
    draw_color(shader, getGlow(), LLShaderMgr::GLOW);
    draw_real(shader, getMaxY(), LLShaderMgr::MAX_Y);
    draw_real(shader, getMoonBrightness(), LLShaderMgr::MOON_BRIGHTNESS);
    draw_real(shader, getSkyMoistureLevel(), LLShaderMgr::MOISTURE_LEVEL);
    draw_real(shader, getSkyDropletRadius(), LLShaderMgr::DROPLET_RADIUS);
    draw_real(shader, getSkyIceLevel(), LLShaderMgr::ICE_LEVEL);
    draw_real(shader, getReflectionProbeAmbiance(), LLShaderMgr::REFLECTION_PROBE_AMBIANCE);
}

void LLSettingsVOSky::applySpecial(void *ptarget, bool force)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    LLVector3 light_direction = LLVector3(LLEnvironment::instance().getClampedLightNorm().mV);

    bool irradiance_pass = gCubeSnapshot && !gPipeline.mReflectionMapManager.isRadiancePass();

    LLShaderUniforms* shader = &((LLShaderUniforms*)ptarget)[LLGLSLShader::SG_DEFAULT];
    {
        shader->uniform3fv(LLViewerShaderMgr::LIGHTNORM, light_direction);
        shader->uniform3fv(LLShaderMgr::WL_CAMPOSLOCAL, LLViewerCamera::getInstance()->getOrigin());
    }

    shader = &((LLShaderUniforms*)ptarget)[LLGLSLShader::SG_SKY];

    shader->uniform3fv(LLViewerShaderMgr::LIGHTNORM, light_direction);

    // Legacy? SETTING_CLOUD_SCROLL_RATE("cloud_scroll_rate")
    LLVector4 vect_c_p_d1(mCloudPosDensity1.mV[0], mCloudPosDensity1.mV[1], mCloudPosDensity1.mV[2]);
    LLVector4 cloud_scroll( LLEnvironment::instance().getCloudScrollDelta() );

    // SL-13084 EEP added support for custom cloud textures -- flip them horizontally to match the preview of Clouds > Cloud Scroll
    // Keep in Sync!
    // * indra\newview\llsettingsvo.cpp
    // * indra\newview\app_settings\shaders\class2\windlight\cloudsV.glsl
    // * indra\newview\app_settings\shaders\class1\deferred\cloudsV.glsl
    cloud_scroll[0] = -cloud_scroll[0];
    vect_c_p_d1 += cloud_scroll;
    shader->uniform3fv(LLShaderMgr::CLOUD_POS_DENSITY1, LLVector3(vect_c_p_d1.mV));

    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

    // TODO -- make these getters return vec3s
    LLVector3 sun_light_color = LLVector3(psky->getSunlightColor().mV);
    LLVector3 moon_light_color = LLVector3(psky->getMoonlightColor().mV);

    shader->uniform3fv(LLShaderMgr::SUNLIGHT_COLOR, sun_light_color);
    shader->uniform3fv(LLShaderMgr::MOONLIGHT_COLOR, moon_light_color);

    shader->uniform3fv(LLShaderMgr::CLOUD_COLOR, LLVector3(psky->getCloudColor().mV));

    shader = &((LLShaderUniforms*)ptarget)[LLGLSLShader::SG_ANY];
    shader->uniform1f(LLShaderMgr::SCENE_LIGHT_STRENGTH, mSceneLightStrength);

    LLColor3 ambient(getTotalAmbient());

    F32 g = getGamma();

    static LLCachedControl<bool> hdr(gSavedSettings, "RenderHDREnabled");
    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
    static LLCachedControl<F32> auto_adjust_ambient_scale(gSavedSettings, "RenderSkyAutoAdjustAmbientScale", 0.75f);
    static LLCachedControl<F32> auto_adjust_hdr_scale(gSavedSettings, "RenderSkyAutoAdjustHDRScale", 2.f);
    static LLCachedControl<F32> auto_adjust_blue_horizon_scale(gSavedSettings, "RenderSkyAutoAdjustBlueHorizonScale", 1.f);
    static LLCachedControl<F32> auto_adjust_blue_density_scale(gSavedSettings, "RenderSkyAutoAdjustBlueDensityScale", 1.f);
    static LLCachedControl<F32> auto_adjust_sun_color_scale(gSavedSettings, "RenderSkyAutoAdjustSunColorScale", 1.f);
    static LLCachedControl<F32> sunlight_scale(gSavedSettings, "RenderSkySunlightScale", 1.5f);
    static LLCachedControl<F32> sunlight_hdr_scale(gSavedSettings, "RenderHDRSkySunlightScale", 1.5f);
    static LLCachedControl<F32> ambient_scale(gSavedSettings, "RenderSkyAmbientScale", 1.5f);
    static LLCachedControl<F32> tonemap_mix_setting(gSavedSettings, "RenderTonemapMix", 1.f);

    // sky is a "classic" sky following pre SL 7.0 shading
    bool classic_mode = psky->canAutoAdjust() && !should_auto_adjust();

    if (!classic_mode)
    {
        psky->setTonemapMix(tonemap_mix_setting);
    }

    shader->uniform1f(LLShaderMgr::SKY_SUNLIGHT_SCALE, hdr ? sunlight_hdr_scale : sunlight_scale);
    shader->uniform1f(LLShaderMgr::SKY_AMBIENT_SCALE, ambient_scale);
    shader->uniform1i(LLShaderMgr::CLASSIC_MODE, classic_mode);

    LLRender::sClassicMode = classic_mode;

    F32 probe_ambiance = getReflectionProbeAmbiance();

    if (irradiance_pass)
    { // during an irradiance map update, disable ambient lighting (direct lighting only) and desaturate sky color (avoid tinting the world blue)
        shader->uniform3fv(LLShaderMgr::AMBIENT, LLVector3::zero.mV);
    }
    else
    {
        if (psky->getReflectionProbeAmbiance() != 0.f)
        {
            shader->uniform3fv(LLShaderMgr::AMBIENT, LLVector3(ambient.mV));
            shader->uniform1f(LLShaderMgr::SKY_HDR_SCALE, sqrtf(g)*2.0f); // use a modifier here so 1.0 maps to the "most desirable" default and the maximum value doesn't go off the rails

            // Low quality setting
            if (!LLPipeline::sReflectionProbesEnabled)
                probe_ambiance = DEFAULT_AUTO_ADJUST_PROBE_AMBIANCE;
        }
        else if (psky->canAutoAdjust() && should_auto_adjust)
        { // auto-adjust legacy sky to take advantage of probe ambiance
            shader->uniform3fv(LLShaderMgr::AMBIENT, (ambient * auto_adjust_ambient_scale).mV);
            shader->uniform1f(LLShaderMgr::SKY_HDR_SCALE, auto_adjust_hdr_scale);
            LLColor3 blue_horizon = getBlueHorizon() * auto_adjust_blue_horizon_scale;
            LLColor3 blue_density = getBlueDensity() * auto_adjust_blue_density_scale;
            sun_light_color = sun_light_color * auto_adjust_sun_color_scale;

            shader->uniform3fv(LLShaderMgr::SUNLIGHT_COLOR, sun_light_color.mV);
            shader->uniform3fv(LLShaderMgr::BLUE_DENSITY, blue_density.mV);
            shader->uniform3fv(LLShaderMgr::BLUE_HORIZON, blue_horizon.mV);

            probe_ambiance = sAutoAdjustProbeAmbiance;
        }
        else
        {
            shader->uniform1f(LLShaderMgr::SKY_HDR_SCALE, 1.f);
            shader->uniform3fv(LLShaderMgr::AMBIENT, LLVector3(ambient.mV));
        }
    }

    shader->uniform1f(LLShaderMgr::REFLECTION_PROBE_AMBIANCE, probe_ambiance);

    shader->uniform1i(LLShaderMgr::SUN_UP_FACTOR, getIsSunUp() ? 1 : 0);
    shader->uniform1f(LLShaderMgr::SUN_MOON_GLOW_FACTOR, getSunMoonGlowFactor());
    shader->uniform1f(LLShaderMgr::DENSITY_MULTIPLIER, getDensityMultiplier());
    shader->uniform1f(LLShaderMgr::DISTANCE_MULTIPLIER, getDistanceMultiplier());

    shader->uniform1f(LLShaderMgr::GAMMA, g);
}

LLSettingsSky::parammapping_t LLSettingsVOSky::getParameterMap() const
{
    static parammapping_t param_map;

    if (param_map.empty())
    {
// LEGACY_ATMOSPHERICS

        // Todo: default 'legacy' values duplicate the ones from functions like getBlueDensity() find a better home for them
        // There is LLSettingsSky::defaults(), but it doesn't contain everything since it is geared towards creating new settings.
        param_map[SETTING_AMBIENT] = DefaultParam(LLShaderMgr::AMBIENT, LLColor3(0.25f, 0.25f, 0.25f).getValue());
        param_map[SETTING_BLUE_DENSITY] = DefaultParam(LLShaderMgr::BLUE_DENSITY, LLColor3(0.2447f, 0.4487f, 0.7599f).getValue());
        param_map[SETTING_BLUE_HORIZON] = DefaultParam(LLShaderMgr::BLUE_HORIZON, LLColor3(0.4954f, 0.4954f, 0.6399f).getValue());
        param_map[SETTING_HAZE_DENSITY] = DefaultParam(LLShaderMgr::HAZE_DENSITY, LLSD(0.7f));
        param_map[SETTING_HAZE_HORIZON] = DefaultParam(LLShaderMgr::HAZE_HORIZON, LLSD(0.19f));
        param_map[SETTING_DENSITY_MULTIPLIER] = DefaultParam(LLShaderMgr::DENSITY_MULTIPLIER, LLSD(0.0001f));
        param_map[SETTING_DISTANCE_MULTIPLIER] = DefaultParam(LLShaderMgr::DISTANCE_MULTIPLIER, LLSD(0.8f));

        // Following values are always present, so we can just zero these ones, but used values from defaults()
        LLSD sky_defaults = LLSettingsSky::defaults();

        param_map[SETTING_CLOUD_POS_DENSITY2] = DefaultParam(LLShaderMgr::CLOUD_POS_DENSITY2, sky_defaults[SETTING_CLOUD_POS_DENSITY2]);
        param_map[SETTING_CLOUD_SCALE] = DefaultParam(LLShaderMgr::CLOUD_SCALE, sky_defaults[SETTING_CLOUD_SCALE]);
        param_map[SETTING_CLOUD_SHADOW] = DefaultParam(LLShaderMgr::CLOUD_SHADOW, sky_defaults[SETTING_CLOUD_SHADOW]);
        param_map[SETTING_CLOUD_VARIANCE] = DefaultParam(LLShaderMgr::CLOUD_VARIANCE, sky_defaults[SETTING_CLOUD_VARIANCE]);
        param_map[SETTING_GLOW] = DefaultParam(LLShaderMgr::GLOW, sky_defaults[SETTING_GLOW]);
        param_map[SETTING_MAX_Y] = DefaultParam(LLShaderMgr::MAX_Y, sky_defaults[SETTING_MAX_Y]);

        //param_map[SETTING_SUNLIGHT_COLOR] = DefaultParam(LLShaderMgr::SUNLIGHT_COLOR, sky_defaults[SETTING_SUNLIGHT_COLOR]);
        //param_map[SETTING_CLOUD_COLOR] = DefaultParam(LLShaderMgr::CLOUD_COLOR, sky_defaults[SETTING_CLOUD_COLOR]);

        param_map[SETTING_MOON_BRIGHTNESS] = DefaultParam(LLShaderMgr::MOON_BRIGHTNESS, sky_defaults[SETTING_MOON_BRIGHTNESS]);
        param_map[SETTING_SKY_MOISTURE_LEVEL] = DefaultParam(LLShaderMgr::MOISTURE_LEVEL, sky_defaults[SETTING_SKY_MOISTURE_LEVEL]);
        param_map[SETTING_SKY_DROPLET_RADIUS] = DefaultParam(LLShaderMgr::DROPLET_RADIUS, sky_defaults[SETTING_SKY_DROPLET_RADIUS]);
        param_map[SETTING_SKY_ICE_LEVEL] = DefaultParam(LLShaderMgr::ICE_LEVEL, sky_defaults[SETTING_SKY_ICE_LEVEL]);

        param_map[SETTING_REFLECTION_PROBE_AMBIANCE] = DefaultParam(LLShaderMgr::REFLECTION_PROBE_AMBIANCE, sky_defaults[SETTING_REFLECTION_PROBE_AMBIANCE]);
// AdvancedAtmospherics TODO
// Provide mappings for new shader params here
    }

    return param_map;
}

//=========================================================================
const F32 LLSettingsVOWater::WATER_FOG_LIGHT_CLAMP(0.3f);

//-------------------------------------------------------------------------
LLSettingsVOWater::LLSettingsVOWater(const LLSD &data) :
    LLSettingsWater(data)
{

}

LLSettingsVOWater::LLSettingsVOWater() :
    LLSettingsWater()
{

}

LLSettingsWater::ptr_t LLSettingsVOWater::buildWater(LLSD settings)
{
    LLSettingsWater::validation_list_t validations = LLSettingsWater::validationList();
    LLSD results = LLSettingsWater::settingValidation(settings, validations);
    if (!results["success"].asBoolean())
    {
        LL_WARNS("SETTINGS") << "Water setting validation failed!\n" << results << LL_ENDL;
        LLSettingsWater::ptr_t();
    }

    return std::make_shared<LLSettingsVOWater>(settings);
}

//-------------------------------------------------------------------------
LLSettingsWater::ptr_t LLSettingsVOWater::buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings, LLSD &messages)
{
    LLSD newsettings(LLSettingsWater::translateLegacySettings(oldsettings));
    if (newsettings.isUndefined())
    {
        messages["REASONS"] = LLTrans::getString("SettingTranslateError", LLSDMap("NAME", name));
        return LLSettingsWater::ptr_t();
    }

    newsettings[SETTING_NAME] = name;
    LLSettingsWater::validation_list_t validations = LLSettingsWater::validationList();
    LLSD results = LLSettingsWater::settingValidation(newsettings, validations);
    if (!results["success"].asBoolean())
    {
        messages["REASONS"] = LLTrans::getString("SettingValidationError", name);
        LL_WARNS("SETTINGS") << "Water setting validation failed!: " << results << LL_ENDL;
        return LLSettingsWater::ptr_t();
    }

    LLSettingsWater::ptr_t waterp = std::make_shared<LLSettingsVOWater>(newsettings);

#ifdef VERIFY_LEGACY_CONVERSION
    LLSD oldsettings = LLSettingsVOWater::convertToLegacy(waterp);

    if (!llsd_equals(oldsettings, oldsettings))
    {
        LL_WARNS("WATER") << "Conversion to/from legacy does not match!\n"
            << "Old: " << oldsettings
            << "new: " << oldsettings << LL_ENDL;
    }

#endif
    return waterp;
}

LLSettingsWater::ptr_t LLSettingsVOWater::buildFromLegacyPresetFile(const std::string &name, const std::string &path, LLSD &messages)
{
    LLSD legacy_data = read_legacy_preset_data(name, path, messages);

    if (!legacy_data)
    {   // messages filled in by read_legacy_preset_data
        LL_WARNS("SETTINGS") << "Could not load legacy Windlight \"" << name << "\" from " << path << LL_ENDL;
        return ptr_t();
    }

    return buildFromLegacyPreset(LLURI::unescape(name), legacy_data, messages);
}


LLSettingsWater::ptr_t LLSettingsVOWater::buildDefaultWater()
{
    static LLSD default_settings;

    if (!default_settings.size())
    {
        default_settings = LLSettingsWater::defaults();

        default_settings[SETTING_NAME] = DEFAULT_SETTINGS_NAME;

        LLSettingsWater::validation_list_t validations = LLSettingsWater::validationList();
        LLSD results = LLSettingsWater::settingValidation(default_settings, validations);
        if (!results["success"].asBoolean())
        {
            LL_WARNS("SETTINGS") << "Water setting validation failed!: " << results << LL_ENDL;
            return LLSettingsWater::ptr_t();
        }
    }

    LLSettingsWater::ptr_t waterp = std::make_shared<LLSettingsVOWater>(default_settings);

    return waterp;
}

LLSettingsWater::ptr_t LLSettingsVOWater::buildClone()
{
    LLSD settings = cloneSettings();
    U32 flags = getFlags();
    LLSettingsWater::validation_list_t validations = LLSettingsWater::validationList();
    LLSD results = LLSettingsWater::settingValidation(settings, validations);
    if (!results["success"].asBoolean())
    {
        LL_WARNS("SETTINGS") << "Water setting validation failed!: " << results << LL_ENDL;
        return LLSettingsWater::ptr_t();
    }

    LLSettingsWater::ptr_t waterp = std::make_shared<LLSettingsVOWater>(settings);
    waterp->setFlags(flags);
    return waterp;
}

LLSD LLSettingsVOWater::convertToLegacy(const LLSettingsWater::ptr_t &pwater)
{
    LLSD legacy(LLSD::emptyMap());
    LLSD settings = pwater->getSettings();

    legacy[SETTING_LEGACY_BLUR_MULTIPLIER] = settings[SETTING_BLUR_MULTIPLIER];
    legacy[SETTING_LEGACY_FOG_COLOR] = ensure_array_4(settings[SETTING_FOG_COLOR], 1.0f);
    legacy[SETTING_LEGACY_FOG_DENSITY] = settings[SETTING_FOG_DENSITY];
    legacy[SETTING_LEGACY_FOG_MOD] = settings[SETTING_FOG_MOD];
    legacy[SETTING_LEGACY_FRESNEL_OFFSET] = settings[SETTING_FRESNEL_OFFSET];
    legacy[SETTING_LEGACY_FRESNEL_SCALE] = settings[SETTING_FRESNEL_SCALE];
    legacy[SETTING_LEGACY_NORMAL_MAP] = settings[SETTING_NORMAL_MAP];
    legacy[SETTING_LEGACY_NORMAL_SCALE] = settings[SETTING_NORMAL_SCALE];
    legacy[SETTING_LEGACY_SCALE_ABOVE] = settings[SETTING_SCALE_ABOVE];
    legacy[SETTING_LEGACY_SCALE_BELOW] = settings[SETTING_SCALE_BELOW];
    legacy[SETTING_LEGACY_WAVE1_DIR] = settings[SETTING_WAVE1_DIR];
    legacy[SETTING_LEGACY_WAVE2_DIR] = settings[SETTING_WAVE2_DIR];

    return legacy;
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

void LLSettingsVOWater::applyToUniforms(void*)
{

}

void LLSettingsVOWater::applySpecial(void *ptarget, bool force)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    LLEnvironment& env = LLEnvironment::instance();

    auto group = LLGLSLShader::SG_ANY;
    LLShaderUniforms* shader = &((LLShaderUniforms*)ptarget)[group];

    {
        F32 water_height = env.getWaterHeight();

        if (LLViewerCamera::instance().cameraUnderWater())
        { // when the camera is under water, use the water height at the camera position
            LLViewerRegion* region = LLWorld::instance().getRegionFromPosAgent(LLViewerCamera::instance().getOrigin());
            if (region)
            {
                water_height = region->getWaterHeight();
            }
        }

        //transform water plane to eye space
        glm::vec3 norm(0.f, 0.f, 1.f);
        glm::vec3 p(0.f, 0.f, water_height);

        glm::mat4 mat = get_current_modelview();
        glm::mat4 invtrans = glm::transpose(glm::inverse(mat));
        invtrans[0][3] = invtrans[1][3] = invtrans[2][3] = 0.f;

        glm::vec3 enorm;
        glm::vec3 ep;
        enorm = mul_mat4_vec3(invtrans, norm);
        enorm = glm::normalize(enorm);
        ep = mul_mat4_vec3(mat, p);

        LLVector4 waterPlane(enorm.x, enorm.y, enorm.z, -glm::dot(ep, enorm));

        norm = glm::vec3(gPipeline.mHeroProbeManager.mMirrorNormal);
        p    = glm::vec3(gPipeline.mHeroProbeManager.mMirrorPosition);
        enorm = mul_mat4_vec3(invtrans, norm);
        enorm = glm::normalize(enorm);
        ep = mul_mat4_vec3(mat, p);

        glm::vec4 mirrorPlane(enorm, -glm::dot(ep, enorm));

        LLDrawPoolAlpha::sWaterPlane = waterPlane;

        shader->uniform4fv(LLShaderMgr::WATER_WATERPLANE, waterPlane.mV);
        shader->uniform4fv(LLShaderMgr::CLIP_PLANE, glm::value_ptr(mirrorPlane));
        LLVector4 light_direction = env.getClampedLightNorm();

        if (gPipeline.mHeroProbeManager.isMirrorPass())
        {
            shader->uniform1f(LLShaderMgr::MIRROR_FLAG, 1);
        }
        else
        {
            shader->uniform1f(LLShaderMgr::MIRROR_FLAG, 0);
        }

        F32 waterFogKS = 1.f / llmax(light_direction.mV[2], WATER_FOG_LIGHT_CLAMP);

        shader->uniform1f(LLShaderMgr::WATER_FOGKS, waterFogKS);

        F32 eyedepth = LLViewerCamera::getInstance()->getOrigin().mV[2] - water_height;
        bool underwater = (eyedepth <= 0.0f);

        F32 waterFogDensity = env.getCurrentWater()->getModifiedWaterFogDensity(underwater);
        shader->uniform1f(LLShaderMgr::WATER_FOGDENSITY, waterFogDensity);

        LLColor4 fog_color(env.getCurrentWater()->getWaterFogColor());
        shader->uniform4fv(LLShaderMgr::WATER_FOGCOLOR, fog_color.mV);

        shader->uniform3fv(LLShaderMgr::WATER_FOGCOLOR_LINEAR, linearColor3(fog_color).mV);

        F32 blend_factor = (F32)env.getCurrentWater()->getBlendFactor();
        shader->uniform1f(LLShaderMgr::BLEND_FACTOR, blend_factor);

        // update to normal lightnorm, water shader itself will use rotated lightnorm as necessary
        shader->uniform3fv(LLShaderMgr::LIGHTNORM, light_direction.mV);
    }
}

void LLSettingsVOWater::updateSettings()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    // base class clears dirty flag so as to not trigger recursive update
    LLSettingsBase::updateSettings();

    LLDrawPoolWater* pwaterpool = (LLDrawPoolWater*)gPipeline.getPool(LLDrawPool::POOL_WATER);
    if (pwaterpool)
    {
        pwaterpool->setTransparentTextures(getTransparentTextureID(), getNextTransparentTextureID());
        pwaterpool->setOpaqueTexture(GetDefaultOpaqueTextureAssetId());
        pwaterpool->setNormalMaps(getNormalMapID(), getNextNormalMapID());
    }
}

LLSettingsWater::parammapping_t LLSettingsVOWater::getParameterMap() const
{
    static parammapping_t param_map;

    return param_map;
}

//=========================================================================
LLSettingsVODay::LLSettingsVODay(const LLSD &data):
    LLSettingsDay(data)
{}

LLSettingsVODay::LLSettingsVODay():
    LLSettingsDay()
{}

LLSettingsDay::ptr_t LLSettingsVODay::buildDay(LLSD settings)
{
    LLSettingsDay::validation_list_t validations = LLSettingsDay::validationList();
    LLSD results = LLSettingsDay::settingValidation(settings, validations);
    if (!results["success"].asBoolean())
    {
        LL_WARNS("SETTINGS") << "Day setting validation failed!\n" << results << LL_ENDL;
        LLSettingsDay::ptr_t();
    }

    LLSettingsDay::ptr_t pday = std::make_shared<LLSettingsVODay>(settings);
    if (pday)
        pday->initialize();

    return pday;
}

//-------------------------------------------------------------------------
LLSettingsDay::ptr_t LLSettingsVODay::buildFromLegacyPreset(const std::string &name, const std::string &path, const LLSD &oldsettings, LLSD &messages)
{
    LLSD newsettings(defaults());
    std::set<std::string> framenames;
    std::set<std::string> notfound;

    // expected and correct folder sctructure is to have
    // three folders in widnlight's root: days, water, skies
    std::string base_path(gDirUtilp->getDirName(path));
    std::string water_path(base_path);
    std::string sky_path(base_path);
    std::string day_path(base_path);

    gDirUtilp->append(water_path, "water");
    gDirUtilp->append(sky_path, "skies");
    gDirUtilp->append(day_path, "days");

    if (!gDirUtilp->fileExists(day_path))
    {
        LL_WARNS("SETTINGS") << "File " << name << ".xml is not in \"days\" folder." << LL_ENDL;
    }

    if (!gDirUtilp->fileExists(water_path))
    {
        LL_WARNS("SETTINGS") << "Failed to find accompaniying water folder for file " << name
            << ".xml. Falling back to using default folder" << LL_ENDL;

        water_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight");
        gDirUtilp->append(water_path, "water");
    }

    if (!gDirUtilp->fileExists(sky_path))
    {
        LL_WARNS("SETTINGS") << "Failed to find accompaniying skies folder for file " << name
            << ".xml. Falling back to using default folder" << LL_ENDL;

        sky_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight");
        gDirUtilp->append(sky_path, "skies");
    }

    newsettings[SETTING_NAME] = name;

    LLSD watertrack = llsd::array(
        LLSDMap(SETTING_KEYKFRAME, LLSD::Real(0.0f))
        (SETTING_KEYNAME, "water:Default"));

    LLSD skytrack = LLSD::emptyArray();

    for (LLSD::array_const_iterator it = oldsettings.beginArray(); it != oldsettings.endArray(); ++it)
    {
        std::string framename = (*it)[1].asString();
        LLSD entry = LLSDMap(SETTING_KEYKFRAME, (*it)[0].asReal())
            (SETTING_KEYNAME, "sky:" + framename);
        framenames.insert(framename);
        skytrack.append(entry);
    }

    newsettings[SETTING_TRACKS] = llsd::array(watertrack, skytrack);

    LLSD frames(LLSD::emptyMap());

    {
        LLSettingsWater::ptr_t pwater = LLSettingsVOWater::buildFromLegacyPresetFile("Default", water_path, messages);
        if (!pwater)
        {   // messages filled in by buildFromLegacyPresetFile
            return LLSettingsDay::ptr_t();
        }
        frames["water:Default"] = pwater->getSettings();
    }

    for (std::set<std::string>::iterator itn = framenames.begin(); itn != framenames.end(); ++itn)
    {
        LLSettingsSky::ptr_t psky = LLSettingsVOSky::buildFromLegacyPresetFile((*itn), sky_path, messages);
        if (!psky)
        {   // messages filled in by buildFromLegacyPresetFile
            return LLSettingsDay::ptr_t();
        }
        frames["sky:" + (*itn)] = psky->getSettings();
    }

    newsettings[SETTING_FRAMES] = frames;

    LLSettingsDay::validation_list_t validations = LLSettingsDay::validationList();
    LLSD results = LLSettingsDay::settingValidation(newsettings, validations);
    if (!results["success"].asBoolean())
    {
        messages["REASONS"] = LLTrans::getString("SettingValidationError", LLSDMap("NAME", name));
        LL_WARNS("SETTINGS") << "Day setting validation failed!: " << results << LL_ENDL;
        return LLSettingsDay::ptr_t();
    }

    LLSettingsDay::ptr_t dayp = std::make_shared<LLSettingsVODay>(newsettings);

#ifdef VERIFY_LEGACY_CONVERSION
    LLSD testsettings = LLSettingsVODay::convertToLegacy(dayp);

    if (!llsd_equals(oldsettings, testsettings))
    {
        LL_WARNS("DAYCYCLE") << "Conversion to/from legacy does not match!\n"
            << "Old: " << oldsettings
            << "new: " << testsettings << LL_ENDL;
    }

#endif

    dayp->initialize();

    return dayp;
}

LLSettingsDay::ptr_t LLSettingsVODay::buildFromLegacyPresetFile(const std::string &name, const std::string &path, LLSD &messages)
{
    LLSD legacy_data = read_legacy_preset_data(name, path, messages);

    if (!legacy_data)
    {   // messages filled in by read_legacy_preset_data
        LL_WARNS("SETTINGS") << "Could not load legacy Windlight \"" << name << "\" from " << path << LL_ENDL;
        return ptr_t();
    }
    // Name for LLSettingsDay only, path to get related files from filesystem
    return buildFromLegacyPreset(LLURI::unescape(name), path, legacy_data, messages);
}



LLSettingsDay::ptr_t LLSettingsVODay::buildFromLegacyMessage(const LLUUID &regionId, LLSD daycycle, LLSD skydefs, LLSD waterdef)
{
    LLSD frames(LLSD::emptyMap());

    for (LLSD::map_iterator itm = skydefs.beginMap(); itm != skydefs.endMap(); ++itm)
    {
        std::string newname = "sky:" + (*itm).first;
        LLSD newsettings = LLSettingsSky::translateLegacySettings((*itm).second);

        newsettings[SETTING_NAME] = newname;
        frames[newname] = newsettings;

        LL_WARNS("SETTINGS") << "created region sky '" << newname << "'" << LL_ENDL;
    }

    LLSD watersettings = LLSettingsWater::translateLegacySettings(waterdef);
    std::string watername = "water:"+ watersettings[SETTING_NAME].asString();
    watersettings[SETTING_NAME] = watername;
    frames[watername] = watersettings;

    LLSD watertrack = llsd::array(
            LLSDMap(SETTING_KEYKFRAME, LLSD::Real(0.0f))
            (SETTING_KEYNAME, watername));

    LLSD skytrack(LLSD::emptyArray());
    for (LLSD::array_const_iterator it = daycycle.beginArray(); it != daycycle.endArray(); ++it)
    {
        LLSD entry = LLSDMap(SETTING_KEYKFRAME, (*it)[0].asReal())
            (SETTING_KEYNAME, "sky:" + (*it)[1].asString());
        skytrack.append(entry);
    }

    LLSD newsettings = LLSDMap
        ( SETTING_NAME, "Region (legacy)" )
        ( SETTING_TRACKS, llsd::array(watertrack, skytrack))
        ( SETTING_FRAMES, frames )
        ( SETTING_TYPE, "daycycle" );

    LLSettingsSky::validation_list_t validations = LLSettingsDay::validationList();
    LLSD results = LLSettingsDay::settingValidation(newsettings, validations);
    if (!results["success"].asBoolean())
    {
        LL_WARNS("SETTINGS") << "Day setting validation failed!:" << results << LL_ENDL;
        return LLSettingsDay::ptr_t();
    }

    LLSettingsDay::ptr_t dayp = std::make_shared<LLSettingsVODay>(newsettings);

    if (dayp)
    {
        // true for validation - either validate here, or when cloning for floater.
        dayp->initialize(true);
    }
    return dayp;
}



LLSettingsDay::ptr_t LLSettingsVODay::buildDefaultDayCycle()
{
    static LLSD default_settings;

    if (!default_settings.size())
    {
        default_settings = LLSettingsDay::defaults();
        default_settings[SETTING_NAME] = DEFAULT_SETTINGS_NAME;

        LLSettingsDay::validation_list_t validations = LLSettingsDay::validationList();
        LLSD results = LLSettingsDay::settingValidation(default_settings, validations);
        if (!results["success"].asBoolean())
        {
            LL_WARNS("SETTINGS") << "Day setting validation failed!\n" << results << LL_ENDL;
            LLSettingsDay::ptr_t();
        }
    }

    LLSettingsDay::ptr_t dayp = std::make_shared<LLSettingsVODay>(default_settings);

    dayp->initialize();
    return dayp;
}

LLSettingsDay::ptr_t LLSettingsVODay::buildFromEnvironmentMessage(LLSD settings)
{
    LLSettingsDay::validation_list_t validations = LLSettingsDay::validationList();
    LLSD results = LLSettingsDay::settingValidation(settings, validations);
    if (!results["success"].asBoolean())
    {
        LL_WARNS("SETTINGS") << "Day setting validation failed!\n" << results << LL_ENDL;
        LLSettingsDay::ptr_t();
    }

    LLSettingsDay::ptr_t dayp = std::make_shared<LLSettingsVODay>(settings);

    dayp->initialize();
    return dayp;
}


void LLSettingsVODay::buildFromOtherSetting(LLSettingsBase::ptr_t settings, LLSettingsVODay::asset_built_fn cb)
{
    if (settings->getSettingsType() == "daycycle")
    {
        if (cb)
            cb(std::static_pointer_cast<LLSettingsDay>(settings));
    }
    else
    {
        LLSettingsVOBase::getSettingsAsset(LLSettingsDay::GetDefaultAssetId(),
            [settings, cb](LLUUID, LLSettingsBase::ptr_t pday, S32, LLExtStat){ combineIntoDayCycle(std::static_pointer_cast<LLSettingsDay>(pday), settings, cb); });
    }
}

void LLSettingsVODay::combineIntoDayCycle(LLSettingsDay::ptr_t pday, LLSettingsBase::ptr_t settings, asset_built_fn cb)
{
    if (settings->getSettingsType() == "sky")
    {
        pday->setName("sky: " + settings->getName());
        pday->clearCycleTrack(1);
        pday->setSettingsAtKeyframe(settings, 0.0, 1);
    }
    else if (settings->getSettingsType() == "water")
    {
        pday->setName("water: " + settings->getName());
        pday->clearCycleTrack(0);
        pday->setSettingsAtKeyframe(settings, 0.0, 0);
    }
    else
    {
        pday.reset();
    }

    if (cb)
        cb(pday);
}


LLSettingsDay::ptr_t LLSettingsVODay::buildClone()
{
    LLSD settings = cloneSettings();

    LLSettingsDay::validation_list_t validations = LLSettingsDay::validationList();
    LLSD results = LLSettingsDay::settingValidation(settings, validations);
    if (!results["success"].asBoolean())
    {
        LL_WARNS("SETTINGS") << "Day setting validation failed!\n" << results << LL_ENDL;
        LLSettingsDay::ptr_t();
    }

    LLSettingsDay::ptr_t dayp = std::make_shared<LLSettingsVODay>(settings);

    dayp->setName(getName());

    if (U32 flags = getFlags())
    {
        dayp->setFlags(flags);
    }

    dayp->initialize();
    return dayp;
}

LLSettingsDay::ptr_t LLSettingsVODay::buildDeepCloneAndUncompress()
{
    // no need for SETTING_TRACKS or SETTING_FRAMES, so take base LLSD
    LLSD settings = llsd_clone(getSettings());

    U32 flags = getFlags();
    LLSettingsDay::ptr_t day_clone = std::make_shared<LLSettingsVODay>(settings);

    for (U32 i = 0; i < LLSettingsDay::TRACK_MAX; ++i)
    {
        const LLSettingsDay::CycleTrack_t& track = getCycleTrackConst(i);
        LLSettingsDay::CycleTrack_t::const_iterator iter = track.begin();
        while (iter != track.end())
        {
            // 'Unpack', usually for editing
            // - frames 'share' settings multiple times
            // - settings can reuse LLSDs they were initialized from
            // We do not want for edited frame to change multiple frames in same track, so do a clone
            day_clone->setSettingsAtKeyframe(iter->second->buildDerivedClone(), iter->first, i);
            iter++;
        }
    }
    day_clone->setFlags(flags);
    return day_clone;
}

LLSD LLSettingsVODay::convertToLegacy(const LLSettingsVODay::ptr_t &pday)
{
    CycleTrack_t &trackwater = pday->getCycleTrack(TRACK_WATER);

    LLSettingsWater::ptr_t pwater;
    if (!trackwater.empty())
    {
        pwater = std::static_pointer_cast<LLSettingsWater>((*trackwater.begin()).second);
    }

    if (!pwater)
        pwater = LLSettingsVOWater::buildDefaultWater();

    LLSD llsdwater = LLSettingsVOWater::convertToLegacy(pwater);

    CycleTrack_t &tracksky = pday->getCycleTrack(1);   // first sky track
    std::map<std::string, LLSettingsSky::ptr_t> skys;

    LLSD llsdcycle(LLSD::emptyArray());

    for(CycleTrack_t::iterator it = tracksky.begin(); it != tracksky.end(); ++it)
    {
        size_t hash = (*it).second->getHash();
        std::stringstream name;

        name << hash;

        skys[name.str()] = std::static_pointer_cast<LLSettingsSky>((*it).second);

        F32 frame = ((tracksky.size() == 1) && (it == tracksky.begin())) ? -1.0f : (*it).first;
        llsdcycle.append( llsd::array(LLSD::Real(frame), name.str()) );
    }

    LLSD llsdskylist(LLSD::emptyMap());

    for (std::map<std::string, LLSettingsSky::ptr_t>::iterator its = skys.begin(); its != skys.end(); ++its)
    {
        LLSD llsdsky = LLSettingsVOSky::convertToLegacy((*its).second, false);
        llsdsky[SETTING_NAME] = (*its).first;

        llsdskylist[(*its).first] = llsdsky;
    }

    return llsd::array(LLSD::emptyMap(), llsdcycle, llsdskylist, llsdwater);
}

LLSettingsSkyPtr_t  LLSettingsVODay::getDefaultSky() const
{
    return LLSettingsVOSky::buildDefaultSky();
}

LLSettingsWaterPtr_t LLSettingsVODay::getDefaultWater() const
{
    return LLSettingsVOWater::buildDefaultWater();
}

LLSettingsSkyPtr_t LLSettingsVODay::buildSky(LLSD settings) const
{
    LLSettingsSky::ptr_t skyp = std::make_shared<LLSettingsVOSky>(settings);

    if (skyp->validate())
        return skyp;

    return LLSettingsSky::ptr_t();
}

LLSettingsWaterPtr_t LLSettingsVODay::buildWater(LLSD settings) const
{
    LLSettingsWater::ptr_t waterp = std::make_shared<LLSettingsVOWater>(settings);

    if (waterp->validate())
        return waterp;

    return LLSettingsWater::ptr_t();
}

//=========================================================================
namespace
{
    LLSD ensure_array_4(LLSD in, F32 fill)
    {
        if (in.size() >= 4)
            return in;

        LLSD out(LLSD::emptyArray());

        for (S32 idx = 0; idx < in.size(); ++idx)
        {
            out.append(in[idx]);
        }

        while (out.size() < 4)
        {
            out.append(LLSD::Real(fill));
        }
        return out;
    }

    // This is a disturbing hack
    std::string legacy_name_to_filename(const std::string &name, bool convertdash = false)
    {
        std::string fixedname(LLURI::escape(name));

        if (convertdash)
            boost::algorithm::replace_all(fixedname, "-", "%2D");

        return fixedname;
    }

    //---------------------------------------------------------------------
    LLSD read_legacy_preset_data(const std::string &name, const std::string& path, LLSD &messages)
    {
        llifstream xml_file;

        std::string full_path(path);
        std::string full_name(name);
        full_name += ".xml";
        gDirUtilp->append(full_path, full_name);

        xml_file.open(full_path.c_str());
        if (!xml_file)
        {
            std::string bad_path(full_path);
            full_path = path;
            full_name = legacy_name_to_filename(name);
            full_name += ".xml";
            gDirUtilp->append(full_path, full_name);

            LL_INFOS("LEGACYSETTING") << "Could not open \"" << bad_path << "\" trying escaped \"" << full_path << "\"" << LL_ENDL;

            xml_file.open(full_path.c_str());
            if (!xml_file)
            {
                LL_WARNS("LEGACYSETTING") << "Unable to open legacy windlight \"" << name << "\" from " << path << LL_ENDL;

                full_path = path;
                full_name = legacy_name_to_filename(name, true);
                full_name += ".xml";
                gDirUtilp->append(full_path, full_name);
                xml_file.open(full_path.c_str());
                if (!xml_file)
                {
                    messages["REASONS"] = LLTrans::getString("SettingImportFileError", LLSDMap("FILE", bad_path));
                    LL_WARNS("LEGACYSETTING") << "Unable to open legacy windlight \"" << name << "\" from " << path << LL_ENDL;
                    return LLSD();
                }
            }
        }

        LLSD params_data;
        LLPointer<LLSDParser> parser = new LLSDXMLParser();
        if (parser->parse(xml_file, params_data, LLSDSerialize::SIZE_UNLIMITED) == LLSDParser::PARSE_FAILURE)
        {
            xml_file.close();
            messages["REASONS"] = LLTrans::getString("SettingParseFileError", LLSDMap("FILE", full_path));
            return LLSD();
        }
        xml_file.close();

        return params_data;
    }
}
