/**
 * @file llfloaterfixedenvironment.cpp
 * @brief Floaters to create and edit fixed settings for sky and water.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#include "llfloaterfixedenvironment.h"

// libs
#include "llbutton.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llsliderctrl.h"
#include "lltabcontainer.h"
#include "llfilepicker.h"
#include "lllocalbitmaps.h"
#include "llsettingspicker.h"
#include "llviewermenufile.h" // LLFilePickerReplyThread
#include "llviewerparcelmgr.h"

// newview
#include "llpaneleditwater.h"
#include "llpaneleditsky.h"

#include "llsettingssky.h"
#include "llsettingswater.h"

#include "llenvironment.h"
#include "llagent.h"
#include "llparcel.h"
#include "lltrans.h"

#include "llsettingsvo.h"
#include "llinventorymodel.h"

extern LLControlGroup gSavedSettings;

namespace
{
    const std::string FIELD_SETTINGS_NAME("settings_name");

    const std::string CONTROL_TAB_AREA("tab_settings");

    const std::string BUTTON_NAME_IMPORT("btn_import");
    const std::string BUTTON_NAME_COMMIT("btn_commit");
    const std::string BUTTON_NAME_CANCEL("btn_cancel");
    const std::string BUTTON_NAME_FLYOUT("btn_flyout");
    const std::string BUTTON_NAME_LOAD("btn_load");

    const std::string ACTION_SAVE("save_settings");
    const std::string ACTION_SAVEAS("save_as_new_settings");
    const std::string ACTION_COMMIT("commit_changes");
    const std::string ACTION_APPLY_LOCAL("apply_local");
    const std::string ACTION_APPLY_PARCEL("apply_parcel");
    const std::string ACTION_APPLY_REGION("apply_region");

    const std::string XML_FLYOUTMENU_FILE("menu_save_settings.xml");
}


//=========================================================================
LLFloaterFixedEnvironment::LLFloaterFixedEnvironment(const LLSD &key) :
    LLFloaterEditEnvironmentBase(key),
    mFlyoutControl(nullptr)
{
}

LLFloaterFixedEnvironment::~LLFloaterFixedEnvironment()
{
    delete mFlyoutControl;
}

bool LLFloaterFixedEnvironment::postBuild()
{
    mTab = getChild<LLTabContainer>(CONTROL_TAB_AREA);
    mTxtName = getChild<LLLineEditor>(FIELD_SETTINGS_NAME);

    mTxtName->setCommitOnFocusLost(true);
    mTxtName->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNameChanged(mTxtName->getValue().asString()); });

    getChild<LLButton>(BUTTON_NAME_IMPORT)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonImport(); });
    getChild<LLButton>(BUTTON_NAME_CANCEL)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onClickCloseBtn(); });
    getChild<LLButton>(BUTTON_NAME_LOAD)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonLoad(); });

    mFlyoutControl = new LLFlyoutComboBtnCtrl(this, BUTTON_NAME_COMMIT, BUTTON_NAME_FLYOUT, XML_FLYOUTMENU_FILE, false);
    mFlyoutControl->setAction([this](LLUICtrl *ctrl, const LLSD &data) { onButtonApply(ctrl, data); });
    mFlyoutControl->setMenuItemVisible(ACTION_COMMIT, false);

    return true;
}

void LLFloaterFixedEnvironment::onOpen(const LLSD& key)
{
    LLUUID invid;

    if (key.has(KEY_INVENTORY_ID))
    {
        invid = key[KEY_INVENTORY_ID].asUUID();
    }

    loadInventoryItem(invid);
    LL_INFOS("SETTINGS") << "Setting edit inventory item to " << mInventoryId << "." << LL_ENDL;

    updateEditEnvironment();
    syncronizeTabs();
    refresh();
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_INSTANT);

}

void LLFloaterFixedEnvironment::onClose(bool app_quitting)
{
    doCloseInventoryFloater(app_quitting);

    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
    LLEnvironment::instance().setCurrentEnvironmentSelection(LLEnvironment::ENV_LOCAL);
    LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);

    mSettings.reset();
    syncronizeTabs();
}

void LLFloaterFixedEnvironment::refresh()
{
    if (!mSettings)
    {
        // disable everything.
        return;
    }

    bool is_inventory_avail = canUseInventory();

    mFlyoutControl->setMenuItemEnabled(ACTION_SAVE, is_inventory_avail && mCanMod && !mInventoryId.isNull());
    mFlyoutControl->setMenuItemEnabled(ACTION_SAVEAS, is_inventory_avail && mCanCopy);
    mFlyoutControl->setMenuItemEnabled(ACTION_APPLY_PARCEL, canApplyParcel());
    mFlyoutControl->setMenuItemEnabled(ACTION_APPLY_REGION, canApplyRegion());

    mTxtName->setValue(mSettings->getName());
    mTxtName->setEnabled(mCanMod);

    S32 count = mTab->getTabCount();

    for (S32 idx = 0; idx < count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(mTab->getPanelByIndex(idx));
        if (panel)
        {
            panel->setCanChangeSettings(mCanMod);
            panel->refresh();
        }
    }
}

void LLFloaterFixedEnvironment::setEditSettingsAndUpdate(const LLSettingsBase::ptr_t &settings)
{
    mSettings = settings; // shouldn't this do buildDeepCloneAndUncompress() ?
    updateEditEnvironment();
    syncronizeTabs();
    refresh();
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);

    // teach user about HDR settings
    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
    if (mSettings
        && mSettings->getSettingsType() == "sky"
        && should_auto_adjust()
        && ((LLSettingsSky*)mSettings.get())->canAutoAdjust()
        && ((LLSettingsSky*)mSettings.get())->getReflectionProbeAmbiance(true) != 0.f)
    {
        LLNotificationsUtil::add("AutoAdjustHDRSky");
    }
}

void LLFloaterFixedEnvironment::syncronizeTabs()
{
    S32 count = mTab->getTabCount();

    for (S32 idx = 0; idx < count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(mTab->getPanelByIndex(idx));
        if (panel)
            panel->setSettings(mSettings);
    }
}

LLFloaterSettingsPicker * LLFloaterFixedEnvironment::getSettingsPicker()
{
    LLFloaterSettingsPicker *picker = static_cast<LLFloaterSettingsPicker *>(mInventoryFloater.get());

    // Show the dialog
    if (!picker)
    {
        picker = new LLFloaterSettingsPicker(this,
            LLUUID::null);

        mInventoryFloater = picker->getHandle();

        picker->setCommitCallback([this](LLUICtrl *, const LLSD &data){ onPickerCommitSetting(data["ItemId"].asUUID() /*data["Track"]*/); });
    }

    return picker;
}

void LLFloaterFixedEnvironment::onPickerCommitSetting(LLUUID item_id)
{
    loadInventoryItem(item_id);
}

void LLFloaterFixedEnvironment::onNameChanged(const std::string &name)
{
    mSettings->setName(name);
    setDirtyFlag();
}

void LLFloaterFixedEnvironment::onButtonImport()
{
    checkAndConfirmSettingsLoss([this](){ doImportFromDisk(); });
}

void LLFloaterFixedEnvironment::onButtonApply(LLUICtrl *ctrl, const LLSD &data)
{
    std::string ctrl_action = ctrl->getName();

    std::string local_desc;
    LLSettingsBase::ptr_t setting_clone;
    bool is_local = false; // because getString can be empty
    if (mSettings->getSettingsType() == "water")
    {
        LLSettingsWater::ptr_t water = std::static_pointer_cast<LLSettingsWater>(mSettings);
        if (water)
        {
            setting_clone = water->buildClone();
            // LLViewerFetchedTexture and check for FTT_LOCAL_FILE or check LLLocalBitmapMgr
            if (LLLocalBitmapMgr::getInstance()->isLocal(water->getNormalMapID()))
            {
                local_desc = LLTrans::getString("EnvironmentNormalMap");
                is_local = true;
            }
            else if (LLLocalBitmapMgr::getInstance()->isLocal(water->getTransparentTextureID()))
            {
                local_desc = LLTrans::getString("EnvironmentTransparent");
                is_local = true;
            }
        }
    }
    else if (mSettings->getSettingsType() == "sky")
    {
        LLSettingsSky::ptr_t sky = std::static_pointer_cast<LLSettingsSky>(mSettings);
        if (sky)
        {
            setting_clone = sky->buildClone();
            if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getSunTextureId()))
            {
                local_desc = LLTrans::getString("EnvironmentSun");
                is_local = true;
            }
            else if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getMoonTextureId()))
            {
                local_desc = LLTrans::getString("EnvironmentMoon");
                is_local = true;
            }
            else if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getCloudNoiseTextureId()))
            {
                local_desc = LLTrans::getString("EnvironmentCloudNoise");
                is_local = true;
            }
            else if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getBloomTextureId()))
            {
                local_desc = LLTrans::getString("EnvironmentBloom");
                is_local = true;
            }
        }
    }

    if (is_local)
    {
        LLSD args;
        args["FIELD"] = local_desc;
        LLNotificationsUtil::add("WLLocalTextureFixedBlock", args);
        return;
    }

    if (ctrl_action == ACTION_SAVE)
    {
        doApplyUpdateInventory(setting_clone);
        clearDirtyFlag();
    }
    else if (ctrl_action == ACTION_SAVEAS)
    {
        LLSD args;
        args["DESC"] = mSettings->getName();
        LLNotificationsUtil::add("SaveSettingAs", args, LLSD(), boost::bind(&LLFloaterFixedEnvironment::onSaveAsCommit, this, _1, _2, setting_clone));
    }
    else if ((ctrl_action == ACTION_APPLY_LOCAL) ||
        (ctrl_action == ACTION_APPLY_PARCEL) ||
        (ctrl_action == ACTION_APPLY_REGION))
    {
        doApplyEnvironment(ctrl_action, setting_clone);
    }
    else
    {
        LL_WARNS("ENVIRONMENT") << "Unknown settings action '" << ctrl_action << "'" << LL_ENDL;
    }
}

void LLFloaterFixedEnvironment::onClickCloseBtn(bool app_quitting)
{
    if (!app_quitting)
        checkAndConfirmSettingsLoss([this](){ closeFloater(); clearDirtyFlag(); });
    else
        closeFloater();
}

void LLFloaterFixedEnvironment::onButtonLoad()
{
    checkAndConfirmSettingsLoss([this](){ doSelectFromInventory(); });
}

void LLFloaterFixedEnvironment::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
    LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been created with asset " << asset_id << " results are:" << results << LL_ENDL;

    if (inventory_id.isNull() || !results["success"].asBoolean())
    {
        LLNotificationsUtil::add("CantCreateInventory");
        return;
    }
    onInventoryCreated(asset_id, inventory_id);
}

void LLFloaterFixedEnvironment::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id)
{
    bool can_trans = true;
    if (mInventoryItem)
    {
        LLPermissions perms = mInventoryItem->getPermissions();

        LLInventoryItem *created_item = gInventory.getItem(mInventoryId);

        if (created_item)
        {
            can_trans = perms.allowOperationBy(PERM_TRANSFER, gAgent.getID());
            created_item->setPermissions(perms);
            created_item->updateServer(false);
        }
    }
    clearDirtyFlag();
    setFocus(true);                 // Call back the focus...
    loadInventoryItem(inventory_id, can_trans);
}

void LLFloaterFixedEnvironment::onInventoryUpdated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
    LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been updated with asset " << asset_id << " results are:" << results << LL_ENDL;

    clearDirtyFlag();
    if (inventory_id != mInventoryId)
    {
        loadInventoryItem(inventory_id);
    }
}


void LLFloaterFixedEnvironment::clearDirtyFlag()
{
    mIsDirty = false;

    S32 count = mTab->getTabCount();

    for (S32 idx = 0; idx < count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(mTab->getPanelByIndex(idx));
        if (panel)
            panel->clearIsDirty();
    }

}

void LLFloaterFixedEnvironment::doSelectFromInventory()
{
    LLFloaterSettingsPicker *picker = getSettingsPicker();

    picker->setSettingsFilter(mSettings->getSettingsTypeValue());
    picker->openFloater();
    picker->setFocus(true);
}

//=========================================================================
LLFloaterFixedEnvironmentWater::LLFloaterFixedEnvironmentWater(const LLSD &key):
    LLFloaterFixedEnvironment(key)
{}

bool LLFloaterFixedEnvironmentWater::postBuild()
{
    if (!LLFloaterFixedEnvironment::postBuild())
        return false;

    LLPanelSettingsWater * panel;
    panel = new LLPanelSettingsWaterMainTab;
    panel->buildFromFile("panel_settings_water.xml");
    panel->setWater(std::static_pointer_cast<LLSettingsWater>(mSettings));
    panel->setOnDirtyFlagChanged( [this] (LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
    mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

    return true;
}

void LLFloaterFixedEnvironmentWater::updateEditEnvironment(void)
{
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT,
        std::static_pointer_cast<LLSettingsWater>(mSettings));
}

void LLFloaterFixedEnvironmentWater::onOpen(const LLSD& key)
{
    if (!mSettings)
    {
        // Initialize the settings, take a snapshot of the current water.
        mSettings = LLEnvironment::instance().getEnvironmentFixedWater(LLEnvironment::ENV_CURRENT)->buildClone();
        mSettings->setName("Snapshot water (new)");

        // TODO: Should we grab sky and keep it around for reference?
    }

    LLFloaterFixedEnvironment::onOpen(key);
}

void LLFloaterFixedEnvironmentWater::doImportFromDisk()
{   // Load a a legacy Windlight XML from disk.
    LLFilePickerReplyThread::startPicker(boost::bind(&LLFloaterFixedEnvironmentWater::loadWaterSettingFromFile, this, _1), LLFilePicker::FFLOAD_XML, false);
}

void LLFloaterFixedEnvironmentWater::loadWaterSettingFromFile(const std::vector<std::string>& filenames)
{
    LLSD messages;
    if (filenames.size() < 1) return;
    std::string filename = filenames[0];
    LL_DEBUGS("ENVEDIT") << "Selected file: " << filename << LL_ENDL;
    LLSettingsWater::ptr_t legacywater = LLEnvironment::createWaterFromLegacyPreset(filename, messages);

    if (!legacywater)
    {
        LLNotificationsUtil::add("WLImportFail", messages);
        return;
    }

    loadInventoryItem(LLUUID::null);

    setDirtyFlag();
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, legacywater);
    setEditSettings(legacywater);
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT, true);
}

//=========================================================================
LLFloaterFixedEnvironmentSky::LLFloaterFixedEnvironmentSky(const LLSD &key) :
    LLFloaterFixedEnvironment(key)
{}

bool LLFloaterFixedEnvironmentSky::postBuild()
{
    if (!LLFloaterFixedEnvironment::postBuild())
        return false;

    LLPanelSettingsSky * panel;
    panel = new LLPanelSettingsSkyAtmosTab;
    panel->buildFromFile("panel_settings_sky_atmos.xml");
    panel->setSky(std::static_pointer_cast<LLSettingsSky>(mSettings));
    panel->setOnDirtyFlagChanged([this](LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
    mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

    panel = new LLPanelSettingsSkyCloudTab;
    panel->buildFromFile("panel_settings_sky_clouds.xml");
    panel->setSky(std::static_pointer_cast<LLSettingsSky>(mSettings));
    panel->setOnDirtyFlagChanged([this](LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
    mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(false));

    panel = new LLPanelSettingsSkySunMoonTab;
    panel->buildFromFile("panel_settings_sky_sunmoon.xml");
    panel->setSky(std::static_pointer_cast<LLSettingsSky>(mSettings));
    panel->setOnDirtyFlagChanged([this](LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
    mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(false));

    return true;
}

void LLFloaterFixedEnvironmentSky::updateEditEnvironment(void)
{
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT,
        std::static_pointer_cast<LLSettingsSky>(mSettings));
}

void LLFloaterFixedEnvironmentSky::onOpen(const LLSD& key)
{
    if (!mSettings)
    {
        // Initialize the settings, take a snapshot of the current water.
        mSettings = LLEnvironment::instance().getEnvironmentFixedSky(LLEnvironment::ENV_CURRENT)->buildClone();
        mSettings->setName("Snapshot sky (new)");
        LLEnvironment::instance().saveBeaconsState();
        // TODO: Should we grab water and keep it around for reference?
    }

    LLFloaterFixedEnvironment::onOpen(key);
}

void LLFloaterFixedEnvironmentSky::onClose(bool app_quitting)
{
    LLEnvironment::instance().revertBeaconsState();

    LLFloaterFixedEnvironment::onClose(app_quitting);
}

void LLFloaterFixedEnvironmentSky::doImportFromDisk()
{   // Load a a legacy Windlight XML from disk.
    LLFilePickerReplyThread::startPicker(boost::bind(&LLFloaterFixedEnvironmentSky::loadSkySettingFromFile, this, _1), LLFilePicker::FFLOAD_XML, false);
}

void LLFloaterFixedEnvironmentSky::loadSkySettingFromFile(const std::vector<std::string>& filenames)
{
    if (filenames.size() < 1) return;
    std::string filename = filenames[0];
    LLSD messages;

    LL_DEBUGS("ENVEDIT") << "Selected file: " << filename << LL_ENDL;
    LLSettingsSky::ptr_t legacysky = LLEnvironment::createSkyFromLegacyPreset(filename, messages);

    if (!legacysky)
    {
        LLNotificationsUtil::add("WLImportFail", messages);

        return;
    }

    loadInventoryItem(LLUUID::null);

    setDirtyFlag();
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, legacysky);
    setEditSettings(legacysky);
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT, true);
}

//=========================================================================
