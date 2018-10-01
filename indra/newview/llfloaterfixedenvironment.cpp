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

#include <boost/make_shared.hpp>

// libs
#include "llbutton.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llsliderctrl.h"
#include "lltabcontainer.h"
#include "llfilepicker.h"
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
const std::string LLFloaterFixedEnvironment::KEY_INVENTORY_ID("inventory_id");


//=========================================================================
LLFloaterFixedEnvironment::LLFloaterFixedEnvironment(const LLSD &key) :
    LLFloater(key),
    mFlyoutControl(nullptr),
    mInventoryId(),
    mInventoryItem(nullptr),
    mIsDirty(false),
    mCanCopy(false),
    mCanMod(false)
{
}

LLFloaterFixedEnvironment::~LLFloaterFixedEnvironment()
{
    delete mFlyoutControl;
}

BOOL LLFloaterFixedEnvironment::postBuild()
{
    mTab = getChild<LLTabContainer>(CONTROL_TAB_AREA);
    mTxtName = getChild<LLLineEditor>(FIELD_SETTINGS_NAME);

    mTxtName->setCommitOnFocusLost(TRUE);
    mTxtName->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNameChanged(mTxtName->getValue().asString()); });

    getChild<LLButton>(BUTTON_NAME_IMPORT)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonImport(); });
    getChild<LLButton>(BUTTON_NAME_CANCEL)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onClickCloseBtn(); });
    getChild<LLButton>(BUTTON_NAME_LOAD)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonLoad(); });

    mFlyoutControl = new LLFlyoutComboBtnCtrl(this, BUTTON_NAME_COMMIT, BUTTON_NAME_FLYOUT, XML_FLYOUTMENU_FILE, false);
    mFlyoutControl->setAction([this](LLUICtrl *ctrl, const LLSD &data) { onButtonApply(ctrl, data); });
    mFlyoutControl->setMenuItemVisible(ACTION_COMMIT, false);

    return TRUE;
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
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_FAST);

}

void LLFloaterFixedEnvironment::onClose(bool app_quitting)
{
    doCloseInventoryFloater(app_quitting);

    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
    LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);

    mSettings.reset();
    syncronizeTabs();
}

void LLFloaterFixedEnvironment::onFocusReceived()
{
    if (isInVisibleChain())
    {
        updateEditEnvironment();
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_FAST);
    }
}

void LLFloaterFixedEnvironment::onFocusLost()
{
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
            panel->refresh();
            panel->setCanChangeSettings(mCanMod);
        }
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
            LLUUID::null, "SELECT SETTINGS");

        mInventoryFloater = picker->getHandle();

        picker->setCommitCallback([this](LLUICtrl *, const LLSD &data){ onPickerCommitSetting(data.asUUID()); });
    }

    return picker;
}

void LLFloaterFixedEnvironment::loadInventoryItem(const LLUUID  &inventoryId)
{
    if (inventoryId.isNull())
    {
        mInventoryItem = nullptr;
        mInventoryId.setNull();
        mCanMod = true;
        mCanCopy = true;
        return;
    }

    mInventoryId = inventoryId;
    LL_INFOS("SETTINGS") << "Setting edit inventory item to " << mInventoryId << "." << LL_ENDL;
    mInventoryItem = gInventory.getItem(mInventoryId);

    if (!mInventoryItem)
    {
        LL_WARNS("SETTINGS") << "Could not find inventory item with Id = " << mInventoryId << LL_ENDL;
        LLNotificationsUtil::add("CantFindInvItem");
        closeFloater();

        mInventoryId.setNull();
        mInventoryItem = nullptr;
        return;
    }

    if (mInventoryItem->getAssetUUID().isNull())
    {
        LL_WARNS("ENVIRONMENT") << "Asset ID in inventory item is NULL (" << mInventoryId << ")" << LL_ENDL;
        LLNotificationsUtil::add("UnableEditItem");
        closeFloater();

        mInventoryId.setNull();
        mInventoryItem = nullptr;
        return;
    }

    mCanCopy = mInventoryItem->getPermissions().allowCopyBy(gAgent.getID());
    mCanMod = mInventoryItem->getPermissions().allowModifyBy(gAgent.getID());

    LLSettingsVOBase::getSettingsAsset(mInventoryItem->getAssetUUID(),
        [this](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onAssetLoaded(asset_id, settings, status); });
}


void LLFloaterFixedEnvironment::checkAndConfirmSettingsLoss(LLFloaterFixedEnvironment::on_confirm_fn cb)
{
    if (isDirty())
    {
        LLSD args(LLSDMap("TYPE", mSettings->getSettingsType())
            ("NAME", mSettings->getName()));

        // create and show confirmation textbox
        LLNotificationsUtil::add("SettingsConfirmLoss", args, LLSD(),
            [cb](const LLSD&notif, const LLSD&resp)
            {
                S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
                if (opt == 0)
                    cb();
            });
    }
    else if (cb)
    {
        cb();
    }
}

void LLFloaterFixedEnvironment::onPickerCommitSetting(LLUUID asset_id)
{
    mInventoryItem = NULL;
    mInventoryId.setNull();
    if (!mInventoryFloater.isDead())
    {
        LLFloaterSettingsPicker *picker = static_cast<LLFloaterSettingsPicker *>(mInventoryFloater.get());
        if (picker)
        {
            mInventoryId = picker->findItemID(asset_id, false);
            mInventoryItem = gInventory.getItem(mInventoryId);
        }
    }

    LLSettingsVOBase::getSettingsAsset(asset_id,
        [this](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onAssetLoaded(asset_id, settings, status); });
}

void LLFloaterFixedEnvironment::onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status)
{
    if (mInventoryItem && mInventoryItem->getAssetUUID() != asset_id)
    {
        LL_WARNS("ENVIRONMENT") << "Discarding obsolete asset callback" << LL_ENDL;
        return;
    }

    if (!settings || status)
    {
        LLSD args;
        args["NAME"] = (mInventoryItem) ? mInventoryItem->getName() : "Unknown";
        LLNotificationsUtil::add("FailedToFindSettings", args);
        closeFloater();
        return;
    }

    mSettings = settings;
    if (mInventoryItem)
        mSettings->setName(mInventoryItem->getName());

    updateEditEnvironment();
    syncronizeTabs();
    refresh();
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

    if (ctrl_action == ACTION_SAVE)
    {
        doApplyUpdateInventory();
    }
    else if (ctrl_action == ACTION_SAVEAS)
    {
        LLSD args;
        args["DESC"] = mSettings->getName();
        LLNotificationsUtil::add("SaveSettingAs", args, LLSD(), boost::bind(&LLFloaterFixedEnvironment::onSaveAsCommit, this, _1, _2));
    }
    else if ((ctrl_action == ACTION_APPLY_LOCAL) ||
        (ctrl_action == ACTION_APPLY_PARCEL) ||
        (ctrl_action == ACTION_APPLY_REGION))
    {
        doApplyEnvironment(ctrl_action);
    }
    else
    {
        LL_WARNS("ENVIRONMENT") << "Unknown settings action '" << ctrl_action << "'" << LL_ENDL;
    }
}

void LLFloaterFixedEnvironment::onSaveAsCommit(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (0 == option)
    {
        std::string settings_name = response["message"].asString();
        LLStringUtil::trim(settings_name);
        doApplyCreateNewInventory(settings_name);
    }
}

void LLFloaterFixedEnvironment::onClickCloseBtn(bool app_quitting)
{
    if (!app_quitting)
        checkAndConfirmSettingsLoss([this](){ closeFloater(); });
    else
        closeFloater();
}

void LLFloaterFixedEnvironment::onButtonLoad()
{
    checkAndConfirmSettingsLoss([this](){ doSelectFromInventory(); });
}

void LLFloaterFixedEnvironment::doApplyCreateNewInventory(std::string settings_name)
{
    LLUUID parent_id = mInventoryItem ? mInventoryItem->getParentUUID() : gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS);
    // This method knows what sort of settings object to create.
    LLSettingsVOBase::createInventoryItem(mSettings, parent_id, settings_name,
            [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
}

void LLFloaterFixedEnvironment::doApplyUpdateInventory()
{
    LL_WARNS("LAPRAS") << "Update inventory for " << mInventoryId << LL_ENDL;
    if (mInventoryId.isNull())
    {
        LL_WARNS("LAPRAS") << "Inventory ID is NULL. Creating New!!!" << LL_ENDL;
        LLSettingsVOBase::createInventoryItem(mSettings, gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS), std::string(),
            [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
    }
    else
    {
        LL_WARNS("LAPRAS") << "Updating inventory ID " << mInventoryId << LL_ENDL;
        LLSettingsVOBase::updateInventoryItem(mSettings, mInventoryId,
            [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryUpdated(asset_id, inventory_id, results); });
    }
}

void LLFloaterFixedEnvironment::doApplyEnvironment(const std::string &where)
{
    if (where == ACTION_APPLY_LOCAL)
    {
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, mSettings);
    }
    else if (where == ACTION_APPLY_PARCEL)
    {
        LLParcel *parcel(LLViewerParcelMgr::instance().getAgentOrSelectedParcel());

        if ((!parcel) || (parcel->getLocalID() == INVALID_PARCEL_ID))
        {
            LL_WARNS("ENVIRONMENT") << "Can not identify parcel. Not applying." << LL_ENDL;
            LLNotificationsUtil::add("WLParcelApplyFail");
            return;
        }

        if (mInventoryItem && !isDirty())
        {
            LLEnvironment::instance().updateParcel(parcel->getLocalID(), mInventoryItem->getAssetUUID(), -1, -1);
        }
        else if (mSettings->getSettingsType() == "sky")
        {
            LLEnvironment::instance().updateParcel(parcel->getLocalID(), std::static_pointer_cast<LLSettingsSky>(mSettings), -1, -1);
        }
        else if (mSettings->getSettingsType() == "water")
        {
            LLEnvironment::instance().updateParcel(parcel->getLocalID(), std::static_pointer_cast<LLSettingsWater>(mSettings), -1, -1);
        }
    }
    else if (where == ACTION_APPLY_REGION)
    {
        if (mInventoryItem && !isDirty())
        {
            LLEnvironment::instance().updateRegion(mInventoryItem->getAssetUUID(), -1, -1);
        }
        else if (mSettings->getSettingsType() == "sky")
        {
            LLEnvironment::instance().updateRegion(std::static_pointer_cast<LLSettingsSky>(mSettings), -1, -1);
        }
        else if (mSettings->getSettingsType() == "water")
        {
            LLEnvironment::instance().updateRegion(std::static_pointer_cast<LLSettingsWater>(mSettings), -1, -1);
        }
    }
    else
    {
        LL_WARNS("ENVIRONMENT") << "Unknown apply '" << where << "'" << LL_ENDL;
        return;
    }

}

void LLFloaterFixedEnvironment::doCloseInventoryFloater(bool quitting)
{
    LLFloater* floaterp = mInventoryFloater.get();

    if (floaterp)
    {
        floaterp->closeFloater(quitting);
    }
}

void LLFloaterFixedEnvironment::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
    LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been created with asset " << asset_id << " results are:" << results << LL_ENDL;
    
    if (inventory_id.isNull() || !results["success"].asBoolean())
    {
        LLNotificationsUtil::add("CantCreateInventory");
        return;
    }

    if (mInventoryItem)
    {
        LLPermissions perms = mInventoryItem->getPermissions();

        LLInventoryItem *created_item = gInventory.getItem(mInventoryId);

        if (created_item)
        {
            created_item->setPermissions(perms);
            created_item->updateServer(false);
        }
    }
    clearDirtyFlag();
    setFocus(TRUE);                 // Call back the focus...
    loadInventoryItem(inventory_id);
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
    picker->setFocus(TRUE);
}

void LLFloaterFixedEnvironment::onPanelDirtyFlagChanged(bool value)
{
    if (value)
        setDirtyFlag();
}

//-------------------------------------------------------------------------
bool LLFloaterFixedEnvironment::canUseInventory() const
{
    return LLEnvironment::instance().isInventoryEnabled();
}

bool LLFloaterFixedEnvironment::canApplyRegion() const
{
    return gAgent.canManageEstate();
}

bool LLFloaterFixedEnvironment::canApplyParcel() const
{
    return LLEnvironment::instance().canAgentUpdateParcelEnvironment();
}

//=========================================================================
LLFloaterFixedEnvironmentWater::LLFloaterFixedEnvironmentWater(const LLSD &key):
    LLFloaterFixedEnvironment(key)
{}

BOOL LLFloaterFixedEnvironmentWater::postBuild()
{
    if (!LLFloaterFixedEnvironment::postBuild())
        return FALSE;

    LLPanelSettingsWater * panel;
    panel = new LLPanelSettingsWaterMainTab;
    panel->buildFromFile("panel_settings_water.xml");
    panel->setWater(std::static_pointer_cast<LLSettingsWater>(mSettings));
    panel->setOnDirtyFlagChanged( [this] (LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
    mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

    return TRUE;
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
    (new LLFilePickerReplyThread(boost::bind(&LLFloaterFixedEnvironmentWater::loadWaterSettingFromFile, this, _1), LLFilePicker::FFLOAD_XML, false))->getFile();
}

void LLFloaterFixedEnvironmentWater::loadWaterSettingFromFile(const std::vector<std::string>& filenames)
{
    if (filenames.size() < 1) return;
    std::string filename = filenames[0];
    LL_WARNS("LAPRAS") << "Selected file: " << filename << LL_ENDL;
    LLSettingsWater::ptr_t legacywater = LLEnvironment::createWaterFromLegacyPreset(filename);

    if (!legacywater)
    {   
        LLSD args(LLSDMap("FILE", filename));
        LLNotificationsUtil::add("WLImportFail", args);
        return;
    }

    loadInventoryItem(LLUUID::null);

    setDirtyFlag();
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, legacywater);
    setEditSettings(legacywater);
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_FAST, true);
}

//=========================================================================
LLFloaterFixedEnvironmentSky::LLFloaterFixedEnvironmentSky(const LLSD &key) :
    LLFloaterFixedEnvironment(key)
{}

BOOL LLFloaterFixedEnvironmentSky::postBuild()
{
    if (!LLFloaterFixedEnvironment::postBuild())
        return FALSE;

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

    if (gSavedSettings.getBOOL("RenderUseAdvancedAtmospherics"))
    {
        panel = new LLPanelSettingsSkyDensityTab;
        panel->buildFromFile("panel_settings_sky_density.xml");
        panel->setSky(std::static_pointer_cast<LLSettingsSky>(mSettings));
        panel->setOnDirtyFlagChanged([this](LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
        mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(false));
    }
    return TRUE;
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

        // TODO: Should we grab water and keep it around for reference?
    }

    LLFloaterFixedEnvironment::onOpen(key);
}

void LLFloaterFixedEnvironmentSky::doImportFromDisk()
{   // Load a a legacy Windlight XML from disk.
    (new LLFilePickerReplyThread(boost::bind(&LLFloaterFixedEnvironmentSky::loadSkySettingFromFile, this, _1), LLFilePicker::FFLOAD_XML, false))->getFile();
}

void LLFloaterFixedEnvironmentSky::loadSkySettingFromFile(const std::vector<std::string>& filenames)
{
    if (filenames.size() < 1) return;
    std::string filename = filenames[0];

    LL_WARNS("LAPRAS") << "Selected file: " << filename << LL_ENDL;
    LLSettingsSky::ptr_t legacysky = LLEnvironment::createSkyFromLegacyPreset(filename);

    if (!legacysky)
    {   
        LLSD args(LLSDMap("FILE", filename));
        LLNotificationsUtil::add("WLImportFail", args);

        return;
    }

    loadInventoryItem(LLUUID::null);

    clearDirtyFlag();
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, legacysky);
    setEditSettings(legacysky);
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_FAST, true);
}

//=========================================================================

void LLSettingsEditPanel::setCanChangeSettings(bool enabled)
{
    setEnabled(enabled);
    setAllChildrenEnabled(enabled);
}
