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

// newview
#include "llpaneleditwater.h"
#include "llpaneleditsky.h"

#include "llsettingssky.h"
#include "llsettingswater.h"

#include "llenvironment.h"
#include "llagent.h"

#include "llsettingsvo.h"

namespace
{
    const std::string FIELD_SETTINGS_NAME("settings_name");

    const std::string CONTROL_TAB_AREA("tab_settings");

    const std::string BUTTON_NAME_LOAD("btn_load");
    const std::string BUTTON_NAME_IMPORT("btn_import");
    const std::string BUTTON_NAME_COMMIT("btn_commit");
    const std::string BUTTON_NAME_CANCEL("btn_cancel");

    const std::string ACTION_SAVE("save_settings");
    const std::string ACTION_SAVEAS("save_as_new_settings");
    const std::string ACTION_APPLY_LOCAL("apply_local");
    const std::string ACTION_APPLY_PARCEL("apply_parcel");
    const std::string ACTION_APPLY_REGION("apply_region");
}

LLFloaterFixedEnvironment::LLFloaterFixedEnvironment(const LLSD &key) :
    LLFloater(key),
    mFlyoutControl(nullptr)
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

    getChild<LLButton>(BUTTON_NAME_LOAD)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonLoad(); });
    getChild<LLButton>(BUTTON_NAME_IMPORT)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonImport(); });
    getChild<LLButton>(BUTTON_NAME_CANCEL)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonCancel(); });

    mFlyoutControl = new LLFlyoutComboBtn(this, "btn_commit", "btn_flyout", "menu_save_settings.xml");
    mFlyoutControl->setAction([this](LLUICtrl *ctrl, const LLSD &data) { onButtonApply(ctrl, data); });

    return TRUE;
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

    bool enableApplyAndLoad = canUseInventory();

    mFlyoutControl->setMenuItemEnabled(ACTION_SAVE, enableApplyAndLoad);
    mFlyoutControl->setMenuItemEnabled(ACTION_SAVEAS, enableApplyAndLoad);

    getChild<LLButton>(BUTTON_NAME_LOAD)->setEnabled(enableApplyAndLoad);

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

void LLFloaterFixedEnvironment::onNameChanged(const std::string &name)
{
    mSettings->setName(name);
}

void LLFloaterFixedEnvironment::onButtonLoad()
{
    doLoadFromInventory();
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
        doApplyCreateNewInventory();
    }
    else if (ctrl_action == ACTION_SAVEAS)
    {
        doApplyUpdateInventory();
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
    LLSettingsVOBase::createInventoryItem(mSettings);
}

void LLFloaterFixedEnvironment::doApplyUpdateInventory()
{
    // todo update existing inventory object.
}

void LLFloaterFixedEnvironment::doApplyEnvironment(const std::string &where)
{
    LLEnvironment::EnvSelection_t env(LLEnvironment::ENV_DEFAULT);
    bool updateSimulator( where != ACTION_APPLY_LOCAL );

    if (where == ACTION_APPLY_LOCAL)
        env = LLEnvironment::ENV_LOCAL;
    else if (where == ACTION_APPLY_PARCEL)
        env = LLEnvironment::ENV_PARCEL;
    else if (where == ACTION_APPLY_REGION)
        env = LLEnvironment::ENV_REGION;
    else
    {
        LL_WARNS("ENVIRONMENT") << "Unknown apply '" << where << "'" << LL_ENDL;
        return;
    }

    LLEnvironment::instance().setEnvironment(env, mSettings);
    if (updateSimulator)
    {
        LL_WARNS("ENVIRONMENT") << "Attempting apply" << LL_ENDL;
    }
}

//-------------------------------------------------------------------------
bool LLFloaterFixedEnvironment::canUseInventory() const
{
    return !gAgent.getRegionCapability("UpdateSettingsAgentInventory").empty();
}

bool LLFloaterFixedEnvironment::canApplyRegion() const
{
    return true;
}

bool LLFloaterFixedEnvironment::canApplyParcel() const
{
    return false;
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

    updateEditEnvironment();
    syncronizeTabs();
    refresh();
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_FAST);
}

void LLFloaterFixedEnvironmentWater::onClose(bool app_quitting)
{
    mSettings.reset();
    syncronizeTabs();
}

void LLFloaterFixedEnvironmentWater::doLoadFromInventory()
{

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

        // TODO: Should we grab sky and keep it around for reference?
    }

    updateEditEnvironment();
    syncronizeTabs();
    refresh();
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_FAST);
}

void LLFloaterFixedEnvironmentSky::onClose(bool app_quitting)
{
    mSettings.reset();
    syncronizeTabs();
}

void LLFloaterFixedEnvironmentSky::doLoadFromInventory()
{

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

