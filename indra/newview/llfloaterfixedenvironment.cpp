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

namespace
{
    const std::string FIELD_SETTINGS_NAME("settings_name");

    const std::string CONTROL_TAB_AREA("tab_settings");

    const std::string BUTTON_NAME_IMPORT("btn_import");
    const std::string BUTTON_NAME_COMMIT("btn_commit");
    const std::string BUTTON_NAME_CANCEL("btn_cancel");
    const std::string BUTTON_NAME_FLYOUT("btn_flyout");

    const std::string ACTION_SAVE("save_settings");
    const std::string ACTION_SAVEAS("save_as_new_settings");
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
    mInventoryItem(nullptr)
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
    getChild<LLButton>(BUTTON_NAME_CANCEL)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonCancel(); });

    mFlyoutControl = new LLFlyoutComboBtnCtrl(this, BUTTON_NAME_COMMIT, BUTTON_NAME_FLYOUT, XML_FLYOUTMENU_FILE);
    mFlyoutControl->setAction([this](LLUICtrl *ctrl, const LLSD &data) { onButtonApply(ctrl, data); });

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
    mSettings.reset();
    syncronizeTabs();
}

void LLFloaterFixedEnvironment::onFocusReceived()
{
    updateEditEnvironment();
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_FAST);
}

void LLFloaterFixedEnvironment::onFocusLost()
{
    // *TODO*: If the window receiving focus is from a select color or select image control...
    // We have technically not changed out of what we are doing so don't change back to displaying
    // the local environment. (unfortunately the focus manager has 
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
}

void LLFloaterFixedEnvironment::refresh()
{
    if (!mSettings)
    {
        // disable everything.
        return;
    }

    bool is_inventory_avail = canUseInventory();

    mFlyoutControl->setMenuItemEnabled(ACTION_SAVE, is_inventory_avail);
    mFlyoutControl->setMenuItemEnabled(ACTION_SAVEAS, is_inventory_avail);
    mFlyoutControl->setMenuItemEnabled(ACTION_APPLY_PARCEL, canApplyParcel());
    mFlyoutControl->setMenuItemEnabled(ACTION_APPLY_REGION, canApplyRegion());

    mTxtName->setValue(mSettings->getName());

    S32 count = mTab->getTabCount();

    for (S32 idx = 0; idx < count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(mTab->getPanelByIndex(idx));
        if (panel)
            panel->refresh();
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

void LLFloaterFixedEnvironment::loadInventoryItem(const LLUUID  &inventoryId)
{
    if (inventoryId.isNull())
    {
        mInventoryItem = nullptr;
        mInventoryId.setNull();
        return;
    }

    mInventoryId = inventoryId;
    LL_INFOS("SETTINGS") << "Setting edit inventory item to " << mInventoryId << "." << LL_ENDL;
    mInventoryItem = gInventory.getItem(mInventoryId);

    if (!mInventoryItem)
    {
        LL_WARNS("SETTINGS") << "Could not find inventory item with Id = " << mInventoryId << LL_ENDL;
        mInventoryId.setNull();
        mInventoryItem = nullptr;
        return;
    }

    LLSettingsVOBase::getSettingsAsset(mInventoryItem->getAssetUUID(),
        [this](LLUUID asset_id, LLSettingsBase::ptr_t settins, S32 status, LLExtStat) { onAssetLoaded(asset_id, settins, status); });
}

void LLFloaterFixedEnvironment::onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status)
{
    if (!settings || status)
    {
        LLSD args;
        args["DESC"] = (mInventoryItem) ? mInventoryItem->getName() : "Unknown";
        LLNotificationsUtil::add("FailedToFindSettings", args);
        closeFloater();
        return;
    }

    mSettings = settings;
    updateEditEnvironment();
    syncronizeTabs();
    refresh();
}

void LLFloaterFixedEnvironment::onNameChanged(const std::string &name)
{
    mSettings->setName(name);
}

void LLFloaterFixedEnvironment::onButtonImport()
{
    doImportFromDisk();
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
        doApplyCreateNewInventory();
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

void LLFloaterFixedEnvironment::onButtonCancel()
{
    // *TODO*: If changed issue a warning?
    this->closeFloater();
}

void LLFloaterFixedEnvironment::doApplyCreateNewInventory()
{
    // This method knows what sort of settings object to create.
    LLSettingsVOBase::createInventoryItem(mSettings, [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
}

void LLFloaterFixedEnvironment::doApplyUpdateInventory()
{
    if (mInventoryId.isNull())
        LLSettingsVOBase::createInventoryItem(mSettings, [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
    else
        LLSettingsVOBase::updateInventoryItem(mSettings, mInventoryId, [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryUpdated(asset_id, inventory_id, results); });
}

void LLFloaterFixedEnvironment::doApplyEnvironment(const std::string &where)
{
    if (where == ACTION_APPLY_LOCAL)
    {
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, mSettings);
    }
    else if (where == ACTION_APPLY_PARCEL)
    {
        LLParcelSelectionHandle handle(LLViewerParcelMgr::instance().getParcelSelection());
        LLParcel *parcel(nullptr);

        if (handle)
            parcel = handle->getParcel();
        if (!parcel)
            parcel = LLViewerParcelMgr::instance().getAgentParcel();

        if (!parcel)
            return;

        if (mSettings->getSettingType() == "sky")
            LLEnvironment::instance().updateParcel(parcel->getLocalID(), std::static_pointer_cast<LLSettingsSky>(mSettings), -1, -1);
        else if (mSettings->getSettingType() == "water")
            LLEnvironment::instance().updateParcel(parcel->getLocalID(), std::static_pointer_cast<LLSettingsWater>(mSettings), -1, -1);
    }
    else if (where == ACTION_APPLY_REGION)
    {
        if (mSettings->getSettingType() == "sky")
            LLEnvironment::instance().updateRegion(std::static_pointer_cast<LLSettingsSky>(mSettings), -1, -1);
        else if (mSettings->getSettingType() == "water")
            LLEnvironment::instance().updateRegion(std::static_pointer_cast<LLSettingsWater>(mSettings), -1, -1);
    }
    else
    {
        LL_WARNS("ENVIRONMENT") << "Unknown apply '" << where << "'" << LL_ENDL;
        return;
    }

}

void LLFloaterFixedEnvironment::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
    LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been created with asset " << asset_id << " results are:" << results << LL_ENDL;
    
    setFocus(TRUE);                 // Call back the focus...
    loadInventoryItem(inventory_id);
}

void LLFloaterFixedEnvironment::onInventoryUpdated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
    LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been updated with asset " << asset_id << " results are:" << results << LL_ENDL;

    if (inventory_id != mInventoryId)
    {
        loadInventoryItem(inventory_id);
    }
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
    LLParcelSelectionHandle handle(LLViewerParcelMgr::instance().getParcelSelection());
    LLParcel *parcel(nullptr);

    if (handle)
        parcel = handle->getParcel();
    if (!parcel)
        parcel = LLViewerParcelMgr::instance().getAgentParcel();

    if (!parcel)
        return false;

    return parcel->allowModifyBy(gAgent.getID(), gAgent.getGroupID()) && 
        LLEnvironment::instance().isExtendedEnvironmentEnabled();
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

void LLFloaterFixedEnvironmentWater::onClose(bool app_quitting)
{
    LLFloaterFixedEnvironment::onClose(app_quitting);
}

void LLFloaterFixedEnvironmentWater::doImportFromDisk()
{   // Load a a legacy Windlight XML from disk.

    LLFilePicker& picker = LLFilePicker::instance();
    if (picker.getOpenFile(LLFilePicker::FFLOAD_XML))
    {
        std::string filename = picker.getFirstFile();

        LL_WARNS("LAPRAS") << "Selected file: " << filename << LL_ENDL;
        LLSettingsWater::ptr_t legacywater = LLEnvironment::createWaterFromLegacyPreset(filename);

        if (!legacywater)
        {   // *TODO* Put up error dialog here.  Could not create water from filename
            return;
        }

        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, legacywater);
        this->setEditSettings(legacywater);
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_FAST, true);
    }
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
    mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

    panel = new LLPanelSettingsSkyCloudTab;
    panel->buildFromFile("panel_settings_sky_clouds.xml");
    panel->setSky(std::static_pointer_cast<LLSettingsSky>(mSettings));
    mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(false));

    panel = new LLPanelSettingsSkySunMoonTab;
    panel->buildFromFile("panel_settings_sky_sunmoon.xml");
    panel->setSky(std::static_pointer_cast<LLSettingsSky>(mSettings));
    mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(false));

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

void LLFloaterFixedEnvironmentSky::onClose(bool app_quitting)
{
    LLFloaterFixedEnvironment::onClose(app_quitting);
}

void LLFloaterFixedEnvironmentSky::doImportFromDisk()
{   // Load a a legacy Windlight XML from disk.

    LLFilePicker& picker = LLFilePicker::instance();
    if (picker.getOpenFile(LLFilePicker::FFLOAD_XML))
    {
        std::string filename = picker.getFirstFile();

        LL_WARNS("LAPRAS") << "Selected file: " << filename << LL_ENDL;
        LLSettingsSky::ptr_t legacysky = LLEnvironment::createSkyFromLegacyPreset(filename);

        if (!legacysky)
        {   // *TODO* Put up error dialog here.  Could not create water from filename
            return;
        }

        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, legacysky);
        this->setEditSettings(legacysky);
        LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_FAST, true);
    }
}

//=========================================================================

