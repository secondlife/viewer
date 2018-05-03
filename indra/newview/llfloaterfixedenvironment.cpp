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
}

LLFloaterFixedEnvironment::LLFloaterFixedEnvironment(const LLSD &key) :
    LLFloater(key)
{
}

BOOL LLFloaterFixedEnvironment::postBuild()
{
    mTab = getChild<LLTabContainer>(CONTROL_TAB_AREA);
    mTxtName = getChild<LLLineEditor>(FIELD_SETTINGS_NAME);

    mTxtName->setCommitOnFocusLost(TRUE);
    mTxtName->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNameChanged(mTxtName->getValue().asString()); });

    getChild<LLButton>(BUTTON_NAME_LOAD)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonLoad(); });
    getChild<LLButton>(BUTTON_NAME_IMPORT)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonImport(); });
    getChild<LLButton>(BUTTON_NAME_COMMIT)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonApply(); });
    getChild<LLButton>(BUTTON_NAME_CANCEL)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonCancel(); });

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

    getChild<LLButton>(BUTTON_NAME_LOAD)->setEnabled(enableApplyAndLoad);
    getChild<LLButton>(BUTTON_NAME_COMMIT)->setEnabled(enableApplyAndLoad);

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

void LLFloaterFixedEnvironment::onButtonApply()
{
    doApplyFixedSettings();
}

void LLFloaterFixedEnvironment::onButtonCancel()
{
    // *TODO*: If changed issue a warning?
    this->closeFloater();
}

//-------------------------------------------------------------------------
bool LLFloaterFixedEnvironment::canUseInventory() const
{
    return !gAgent.getRegionCapability("UpdateSettingsAgentInventory").empty();
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

void LLFloaterFixedEnvironmentWater::doApplyFixedSettings() 
{
    LLSettingsVOBase::createInventoryItem(mSettings);

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

void LLFloaterFixedEnvironmentSky::doApplyFixedSettings() 
{
    LLSettingsVOBase::createInventoryItem(mSettings);
}

//=========================================================================

#if 0
// virtual
BOOL LLFloaterEditSky::postBuild()
{
	mSkyPresetNameEditor = getChild<LLLineEditor>("sky_preset_name");
	mSkyPresetCombo = getChild<LLComboBox>("sky_preset_combo");
	mMakeDefaultCheckBox = getChild<LLCheckBoxCtrl>("make_default_cb");
	mSaveButton = getChild<LLButton>("save");
    mSkyAdapter = boost::make_shared<LLSkySettingsAdapter>();

    LLEnvironment::instance().setSkyListChange(boost::bind(&LLFloaterEditSky::onSkyPresetListChange, this));

	initCallbacks();

// 	// Create the sun position scrubber on the slider.
// 	getChild<LLMultiSliderCtrl>("WLSunPos")->addSlider(12.f);

	return TRUE;
}

// virtual
void LLFloaterEditSky::onOpen(const LLSD& key)
{
	bool new_preset = isNewPreset();
	std::string param = key.asString();
	std::string floater_title = getString(std::string("title_") + param);
	std::string hint = getString(std::string("hint_" + param));

	// Update floater title.
	setTitle(floater_title);

	// Update the hint at the top.
	getChild<LLUICtrl>("hint")->setValue(hint);

	// Hide the hint to the right of the combo if we're invoked to create a new preset.
	getChildView("note")->setVisible(!new_preset);

	// Switch between the sky presets combobox and preset name input field.
	mSkyPresetCombo->setVisible(!new_preset);
	mSkyPresetNameEditor->setVisible(new_preset);

	reset();
}

// virtual
void LLFloaterEditSky::onClose(bool app_quitting)
{
	if (!app_quitting) // there's no point to change environment if we're quitting
	{
        LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
	}
}

// virtual
void LLFloaterEditSky::draw()
{
	syncControls();
	LLFloater::draw();
}

void LLFloaterEditSky::initCallbacks(void)
{
	// *TODO: warn user if a region environment update comes while we're editing a region sky preset.

	mSkyPresetNameEditor->setKeystrokeCallback(boost::bind(&LLFloaterEditSky::onSkyPresetNameEdited, this), NULL);
	mSkyPresetCombo->setCommitCallback(boost::bind(&LLFloaterEditSky::onSkyPresetSelected, this));
	mSkyPresetCombo->setTextEntryCallback(boost::bind(&LLFloaterEditSky::onSkyPresetNameEdited, this));

	mSaveButton->setCommitCallback(boost::bind(&LLFloaterEditSky::onBtnSave, this));
	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterEditSky::onBtnCancel, this));

	// Connect to region info updates.
	LLRegionInfoModel::instance().setUpdateCallback(boost::bind(&LLFloaterEditSky::onRegionInfoUpdate, this));

	//-------------------------------------------------------------------------
// LEGACY_ATMOSPHERICS
    // ambient
    getChild<LLUICtrl>("WLAmbient")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &mSkyAdapter->mAmbient));

	// blue horizon/density
	getChild<LLUICtrl>("WLBlueHorizon")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &mSkyAdapter->mBlueHorizon));
    getChild<LLUICtrl>("WLBlueDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &mSkyAdapter->mBlueDensity));

	// haze density, horizon, mult, and altitude
    getChild<LLUICtrl>("WLHazeDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mHazeDensity));
    getChild<LLUICtrl>("WLHazeHorizon")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mHazeHorizon));
    getChild<LLUICtrl>("WLDensityMult")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mDensityMult));
    getChild<LLUICtrl>("WLDistanceMult")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mDistanceMult));
    getChild<LLUICtrl>("WLMaxAltitude")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mMaxAlt));

	// sunlight
    getChild<LLUICtrl>("WLSunlight")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &mSkyAdapter->mSunlight));

	// glow
    getChild<LLUICtrl>("WLGlowR")->setCommitCallback(boost::bind(&LLFloaterEditSky::onGlowRMoved, this, _1, &mSkyAdapter->mGlow));
    getChild<LLUICtrl>("WLGlowB")->setCommitCallback(boost::bind(&LLFloaterEditSky::onGlowBMoved, this, _1, &mSkyAdapter->mGlow));

	// time of day
//     getChild<LLUICtrl>("WLSunPos")->setCommitCallback(boost::bind(&LLFloaterEditSky::onSunMoved, this, _1, &mSkyAdapter->mLightnorm));     // multi-slider
// 	getChild<LLTimeCtrl>("WLDayTime")->setCommitCallback(boost::bind(&LLFloaterEditSky::onTimeChanged, this));                          // time ctrl
//     getChild<LLUICtrl>("WLEastAngle")->setCommitCallback(boost::bind(&LLFloaterEditSky::onSunMoved, this, _1, &mSkyAdapter->mLightnorm));
    getChild<LLJoystickQuaternion>("WLSunRotation")->setCommitCallback(boost::bind(&LLFloaterEditSky::onSunRotationChanged, this));
    getChild<LLJoystickQuaternion>("WLMoonRotation")->setCommitCallback(boost::bind(&LLFloaterEditSky::onMoonRotationChanged, this));

	// Clouds

	// Cloud Color
    getChild<LLUICtrl>("WLCloudColor")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &mSkyAdapter->mCloudColor));

	// Cloud
    getChild<LLUICtrl>("WLCloudX")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &mSkyAdapter->mCloudMain));
    getChild<LLUICtrl>("WLCloudY")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlGMoved, this, _1, &mSkyAdapter->mCloudMain));
    getChild<LLUICtrl>("WLCloudDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlBMoved, this, _1, &mSkyAdapter->mCloudMain));

	// Cloud Detail
    getChild<LLUICtrl>("WLCloudDetailX")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &mSkyAdapter->mCloudDetail));
    getChild<LLUICtrl>("WLCloudDetailY")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlGMoved, this, _1, &mSkyAdapter->mCloudDetail));
    getChild<LLUICtrl>("WLCloudDetailDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlBMoved, this, _1, &mSkyAdapter->mCloudDetail));

	// Cloud extras
    getChild<LLUICtrl>("WLCloudCoverage")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mCloudCoverage));
    getChild<LLUICtrl>("WLCloudScale")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mCloudScale));
	getChild<LLUICtrl>("WLCloudScrollX")->setCommitCallback(boost::bind(&LLFloaterEditSky::onCloudScrollXMoved, this, _1));
	getChild<LLUICtrl>("WLCloudScrollY")->setCommitCallback(boost::bind(&LLFloaterEditSky::onCloudScrollYMoved, this, _1));
    

	// Dome
    getChild<LLUICtrl>("WLGamma")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mWLGamma));
	getChild<LLUICtrl>("WLStarAlpha")->setCommitCallback(boost::bind(&LLFloaterEditSky::onStarAlphaMoved, this, _1));
}

//=================================================================================================

void LLFloaterEditSky::syncControls()
{
    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();
    mEditSettings = psky;

    std::string name = psky->getName();

    mSkyPresetNameEditor->setText(name);
    mSkyPresetCombo->setValue(name);

// LEGACY_ATMOSPHERICS
    // ambient
    mSkyAdapter->mAmbient.setColor3( psky->getAmbientColor() );
	setColorSwatch("WLAmbient", mSkyAdapter->mAmbient, WL_SUN_AMBIENT_SLIDER_SCALE);

	// blue horizon / density
	mSkyAdapter->mBlueHorizon.setColor3( psky->getBlueHorizon() );
	setColorSwatch("WLBlueHorizon", mSkyAdapter->mBlueHorizon, WL_BLUE_HORIZON_DENSITY_SCALE);
    mSkyAdapter->mBlueDensity.setColor3( psky->getBlueDensity() );
	setColorSwatch("WLBlueDensity", mSkyAdapter->mBlueDensity, WL_BLUE_HORIZON_DENSITY_SCALE);

	// haze density, horizon, mult, and altitude
    mSkyAdapter->mHazeDensity = psky->getHazeDensity();
	childSetValue("WLHazeDensity", (F32) mSkyAdapter->mHazeDensity);
    mSkyAdapter->mHazeHorizon = psky->getHazeHorizon();
	childSetValue("WLHazeHorizon", (F32) mSkyAdapter->mHazeHorizon);
    mSkyAdapter->mDensityMult = psky->getDensityMultiplier();
	childSetValue("WLDensityMult", ((F32) mSkyAdapter->mDensityMult) * mSkyAdapter->mDensityMult.getMult());
    mSkyAdapter->mMaxAlt = psky->getMaxY();
    mSkyAdapter->mDistanceMult = psky->getDistanceMultiplier();
	childSetValue("WLDistanceMult", (F32) mSkyAdapter->mDistanceMult);
	childSetValue("WLMaxAltitude", (F32) mSkyAdapter->mMaxAlt);

	// Lighting

	// sunlight
    mSkyAdapter->mSunlight.setColor3( psky->getSunlightColor() );
	setColorSwatch("WLSunlight", mSkyAdapter->mSunlight, WL_SUN_AMBIENT_SLIDER_SCALE);

	// glow
    mSkyAdapter->mGlow.setColor3( psky->getGlow() );
	childSetValue("WLGlowR", 2 - mSkyAdapter->mGlow.getRed() / 20.0f);
	childSetValue("WLGlowB", -mSkyAdapter->mGlow.getBlue() / 5.0f);

	

//     LLSettingsSky::azimalt_t azal = psky->getSunRotationAzAl();
// 
// 	F32 time24 = sun_pos_to_time24(azal.second / F_TWO_PI);
// 	getChild<LLMultiSliderCtrl>("WLSunPos")->setCurSliderValue(time24, TRUE);
// 	getChild<LLTimeCtrl>("WLDayTime")->setTime24(time24);
// 	childSetValue("WLEastAngle", azal.first / F_TWO_PI);
    getChild<LLJoystickQuaternion>("WLSunRotation")->setRotation(psky->getSunRotation());
    getChild<LLJoystickQuaternion>("WLMoonRotation")->setRotation(psky->getMoonRotation());

	// Clouds

	// Cloud Color
    mSkyAdapter->mCloudColor.setColor3( psky->getCloudColor() );
	setColorSwatch("WLCloudColor", mSkyAdapter->mCloudColor, WL_CLOUD_SLIDER_SCALE);

	// Cloud
    mSkyAdapter->mCloudMain.setColor3( psky->getCloudPosDensity1() );
	childSetValue("WLCloudX", mSkyAdapter->mCloudMain.getRed());
	childSetValue("WLCloudY", mSkyAdapter->mCloudMain.getGreen());
	childSetValue("WLCloudDensity", mSkyAdapter->mCloudMain.getBlue());

	// Cloud Detail
	mSkyAdapter->mCloudDetail.setColor3( psky->getCloudPosDensity2() );
	childSetValue("WLCloudDetailX", mSkyAdapter->mCloudDetail.getRed());
	childSetValue("WLCloudDetailY", mSkyAdapter->mCloudDetail.getGreen());
	childSetValue("WLCloudDetailDensity", mSkyAdapter->mCloudDetail.getBlue());

	// Cloud extras
    mSkyAdapter->mCloudCoverage = psky->getCloudShadow();
    mSkyAdapter->mCloudScale = psky->getCloudScale();
	childSetValue("WLCloudCoverage", (F32) mSkyAdapter->mCloudCoverage);
	childSetValue("WLCloudScale", (F32) mSkyAdapter->mCloudScale);

	// cloud scrolling
    LLVector2 scroll_rate = psky->getCloudScrollRate();

    // LAPRAS: These should go away...
    childDisable("WLCloudLockX");
 	childDisable("WLCloudLockY");

	// disable if locked, enable if not
	childEnable("WLCloudScrollX");
	childEnable("WLCloudScrollY");

	// *HACK cloud scrolling is off my an additive of 10
	childSetValue("WLCloudScrollX", scroll_rate[0] - 10.0f);
	childSetValue("WLCloudScrollY", scroll_rate[1] - 10.0f);

	// Tweak extras

    mSkyAdapter->mWLGamma = psky->getGamma();
	childSetValue("WLGamma", (F32) mSkyAdapter->mWLGamma);

	childSetValue("WLStarAlpha", psky->getStarBrightness());
}

void LLFloaterEditSky::setColorSwatch(const std::string& name, const WLColorControl& from_ctrl, F32 k)
{
	// Set the value, dividing it by <k> first.
	LLColor4 color = from_ctrl.getColor4();
	getChild<LLColorSwatchCtrl>(name)->set(color / k);
}

// color control callbacks
void LLFloaterEditSky::onColorControlMoved(LLUICtrl* ctrl, WLColorControl* color_ctrl)
{
	LLColorSwatchCtrl* swatch = static_cast<LLColorSwatchCtrl*>(ctrl);
	LLColor4 color_vec(swatch->get().mV);

	// Multiply RGB values by the appropriate factor.
	F32 k = WL_CLOUD_SLIDER_SCALE;
	if (color_ctrl->getIsSunOrAmbientColor())
	{
		k = WL_SUN_AMBIENT_SLIDER_SCALE;
	}
	else if (color_ctrl->getIsBlueHorizonOrDensity())
	{
		k = WL_BLUE_HORIZON_DENSITY_SCALE;
	}

	color_vec *= k; // intensity isn't affected by the multiplication

    // Set intensity to maximum of the RGB values.
    color_vec.mV[3] = color_max(color_vec);

	// Apply the new RGBI value.
	color_ctrl->setColor4(color_vec);
	color_ctrl->update(mEditSettings);
}

void LLFloaterEditSky::onColorControlRMoved(LLUICtrl* ctrl, void* userdata)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

    F32 red_value = sldr_ctrl->getValueF32();
    F32 k = 1.0f;

	if (color_ctrl->getIsSunOrAmbientColor())
	{
		k = WL_SUN_AMBIENT_SLIDER_SCALE;
	}
	if (color_ctrl->getIsBlueHorizonOrDensity())
	{
		k = WL_BLUE_HORIZON_DENSITY_SCALE;
	}
    color_ctrl->setRed(red_value * k);

    adjustIntensity(color_ctrl, red_value, k);
    color_ctrl->update(mEditSettings);
}

void LLFloaterEditSky::onColorControlGMoved(LLUICtrl* ctrl, void* userdata)
{
    LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
    WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

    F32 green_value = sldr_ctrl->getValueF32();
    F32 k = 1.0f;

    if (color_ctrl->getIsSunOrAmbientColor())
    {
        k = WL_SUN_AMBIENT_SLIDER_SCALE;
    }
    if (color_ctrl->getIsBlueHorizonOrDensity())
    {
        k = WL_BLUE_HORIZON_DENSITY_SCALE;
    }
    color_ctrl->setGreen(green_value * k);

    adjustIntensity(color_ctrl, green_value, k);
    color_ctrl->update(mEditSettings);
}

void LLFloaterEditSky::onColorControlBMoved(LLUICtrl* ctrl, void* userdata)
{
    LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
    WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

    F32 blue_value = sldr_ctrl->getValueF32();
    F32 k = 1.0f;

    if (color_ctrl->getIsSunOrAmbientColor())
    {
        k = WL_SUN_AMBIENT_SLIDER_SCALE;
    }
    if (color_ctrl->getIsBlueHorizonOrDensity())
    {
        k = WL_BLUE_HORIZON_DENSITY_SCALE;
    }
    color_ctrl->setBlue(blue_value * k);

    adjustIntensity(color_ctrl, blue_value, k);
    color_ctrl->update(mEditSettings);
}

void LLFloaterEditSky::adjustIntensity(WLColorControl *ctrl, F32 val, F32 scale)
{
    if (ctrl->getHasSliderName())
    {
        LLColor4 color = ctrl->getColor4();
        F32 i = color_max(color) / scale;
        ctrl->setIntensity(i);
        std::string name = ctrl->getSliderName();
        name.append("I");

        childSetValue(name, i);
    }
}


/// GLOW SPECIFIC CODE
void LLFloaterEditSky::onGlowRMoved(LLUICtrl* ctrl, void* userdata)
{

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	// scaled by 20
	color_ctrl->setRed((2 - sldr_ctrl->getValueF32()) * 20);

	color_ctrl->update(mEditSettings);
}

/// \NOTE that we want NEGATIVE (-) B
void LLFloaterEditSky::onGlowBMoved(LLUICtrl* ctrl, void* userdata)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	/// \NOTE that we want NEGATIVE (-) B and NOT by 20 as 20 is too big
	color_ctrl->setBlue(-sldr_ctrl->getValueF32() * 5);

	color_ctrl->update(mEditSettings);
}

void LLFloaterEditSky::onFloatControlMoved(LLUICtrl* ctrl, void* userdata)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLFloatControl * floatControl = static_cast<WLFloatControl *>(userdata);

	floatControl->setValue(sldr_ctrl->getValueF32() / floatControl->getMult());

	floatControl->update(mEditSettings);
}


// Lighting callbacks

// time of day
void LLFloaterEditSky::onSunMoved(LLUICtrl* ctrl, void* userdata)
{
	LLMultiSliderCtrl* sun_msldr = getChild<LLMultiSliderCtrl>("WLSunPos");
	LLSliderCtrl* east_sldr = getChild<LLSliderCtrl>("WLEastAngle");
	LLTimeCtrl* time_ctrl = getChild<LLTimeCtrl>("WLDayTime");
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	F32 time24  = sun_msldr->getCurSliderValue();
	time_ctrl->setTime24(time24); // sync the time ctrl with the new sun position

	// get the two angles
    F32 azimuth = F_TWO_PI * east_sldr->getValueF32();
    F32 altitude = F_TWO_PI * time24_to_sun_pos(time24);
    mEditSettings->setSunRotation(azimuth, altitude);
    mEditSettings->setMoonRotation(azimuth + F_PI, -altitude);

    LLVector4 sunnorm( mEditSettings->getSunDirection(), 1.f );

	color_ctrl->update(mEditSettings);
}

void LLFloaterEditSky::onTimeChanged()
{
	F32 time24 = getChild<LLTimeCtrl>("WLDayTime")->getTime24();
	getChild<LLMultiSliderCtrl>("WLSunPos")->setCurSliderValue(time24, TRUE);
    onSunMoved(getChild<LLUICtrl>("WLSunPos"), &(mSkyAdapter->mLightnorm));
}

void LLFloaterEditSky::onSunRotationChanged()
{
    LLJoystickQuaternion* sun_spinner = getChild<LLJoystickQuaternion>("WLSunRotation");
    LLQuaternion sunrot(sun_spinner->getRotation());

    mEditSettings->setSunRotation(sunrot);
}

void LLFloaterEditSky::onMoonRotationChanged()
{
    LLJoystickQuaternion* moon_spinner = getChild<LLJoystickQuaternion>("WLMoonRotation");
    LLQuaternion moonrot(moon_spinner->getRotation());

    mEditSettings->setMoonRotation(moonrot);
}

void LLFloaterEditSky::onStarAlphaMoved(LLUICtrl* ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

    mEditSettings->setStarBrightness(sldr_ctrl->getValueF32());
}

// Clouds
void LLFloaterEditSky::onCloudScrollXMoved(LLUICtrl* ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	// *HACK  all cloud scrolling is off by an additive of 10.
    mEditSettings->setCloudScrollRateX(sldr_ctrl->getValueF32() + 10.0f);
}

void LLFloaterEditSky::onCloudScrollYMoved(LLUICtrl* ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	// *HACK  all cloud scrolling is off by an additive of 10.
    mEditSettings->setCloudScrollRateY(sldr_ctrl->getValueF32() + 10.0f);
}

//=================================================================================================

void LLFloaterEditSky::reset()
{
	if (isNewPreset())
	{
		mSkyPresetNameEditor->setValue(LLSD());
		mSaveButton->setEnabled(FALSE); // will be enabled as soon as users enters a name
	}
	else
	{
		refreshSkyPresetsList();

		// Disable controls until a sky preset to edit is selected.
		enableEditing(false);
	}
}

bool LLFloaterEditSky::isNewPreset() const
{
	return mKey.asString() == "new";
}

void LLFloaterEditSky::refreshSkyPresetsList()
{
	mSkyPresetCombo->removeall();

    LLEnvironment::list_name_id_t list = LLEnvironment::instance().getSkyList();

    for (LLEnvironment::list_name_id_t::iterator it = list.begin(); it != list.end(); ++it)
    {
        mSkyPresetCombo->add((*it).first, LLSDArray((*it).first)((*it).second));
    }

	mSkyPresetCombo->setLabel(getString("combo_label"));
}

void LLFloaterEditSky::enableEditing(bool enable)
{
	// Enable/disable the tab and their contents.
	LLTabContainer* tab_container = getChild<LLTabContainer>("WindLight Tabs");
	tab_container->setEnabled(enable);
	for (S32 i = 0; i < tab_container->getTabCount(); ++i)
	{
		tab_container->enableTabButton(i, enable);
		tab_container->getPanelByIndex(i)->setCtrlsEnabled(enable);
	}

	// Enable/disable saving.
	mSaveButton->setEnabled(enable);
	mMakeDefaultCheckBox->setEnabled(enable);
}

void LLFloaterEditSky::saveRegionSky()
{
#if 0
	LLWLParamKey key(getSelectedSkyPreset());
	llassert(key.scope == LLEnvKey::SCOPE_REGION);

	LL_DEBUGS("Windlight") << "Saving region sky preset: " << key.name  << LL_ENDL;
	LLWLParamManager& wl_mgr = LLWLParamManager::instance();
	wl_mgr.mCurParams.mName = key.name;
	wl_mgr.setParamSet(key, wl_mgr.mCurParams);

	// *TODO: save to cached region settings.
	LL_WARNS("Windlight") << "Saving region sky is not fully implemented yet" << LL_ENDL;
#endif
}

std::string LLFloaterEditSky::getSelectedPresetName() const
{
    std::string name;
    if (mSkyPresetNameEditor->getVisible())
    {
        name = mSkyPresetNameEditor->getText();
    }
    else
    {
        LLSD combo_val = mSkyPresetCombo->getValue();
        name = combo_val[0].asString();
    }

    return name;
}

void LLFloaterEditSky::onSkyPresetNameEdited()
{
    std::string name = mSkyPresetNameEditor->getText();
    LLSettingsWater::ptr_t psky = LLEnvironment::instance().getCurrentWater();

    psky->setName(name);
}

void LLFloaterEditSky::onSkyPresetSelected()
{
    std::string name;

    name = getSelectedPresetName();

    LLSettingsSky::ptr_t psky = LLEnvironment::instance().findSkyByName(name);

    if (!psky)
    {
        LL_WARNS("WATEREDIT") << "Could not find water preset" << LL_ENDL;
        enableEditing(false);
        return;
    }

    psky = psky->buildClone();
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, psky);
    mEditSettings = psky;

    syncControls();
    enableEditing(true);

}

bool LLFloaterEditSky::onSaveAnswer(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	// If they choose save, do it.  Otherwise, don't do anything
	if (option == 0)
	{
		onSaveConfirmed();
	}

	return false;
}

void LLFloaterEditSky::onSaveConfirmed()
{
    // Save currently displayed water params to the selected preset.
    std::string name = mEditSettings->getName();

    LL_DEBUGS("Windlight") << "Saving sky preset " << name << LL_ENDL;

    LLEnvironment::instance().addSky(mEditSettings);

    // Change preference if requested.
    if (mMakeDefaultCheckBox->getEnabled() && mMakeDefaultCheckBox->getValue())
    {
        LL_DEBUGS("Windlight") << name << " is now the new preferred sky preset" << LL_ENDL;
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, mEditSettings);
    }

    closeFloater();
}

void LLFloaterEditSky::onBtnSave()
{
    LLEnvironment::instance().addSky(mEditSettings);
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, mEditSettings);

    closeFloater();
}

void LLFloaterEditSky::onBtnCancel()
{
	closeFloater();
}

void LLFloaterEditSky::onSkyPresetListChange()
{
    refreshSkyPresetsList();
}

void LLFloaterEditSky::onRegionSettingsChange()
{
#if 0
	// If creating a new sky, don't bother.
	if (isNewPreset())
	{
		return;
	}

	if (getSelectedSkyPreset().scope == LLEnvKey::SCOPE_REGION) // if editing a region sky
	{
		// reset the floater to its initial state
		reset();

		// *TODO: Notify user?
	}
	else // editing a local sky
	{
		refreshSkyPresetsList();
	}
#endif
}

void LLFloaterEditSky::onRegionInfoUpdate()
{
#if 0
	bool can_edit = true;

	// If we've selected a region sky preset for editing.
	if (getSelectedSkyPreset().scope == LLEnvKey::SCOPE_REGION)
	{
		// check whether we have the access
		can_edit = LLEnvManagerNew::canEditRegionSettings();
	}

	enableEditing(can_edit);
#endif
}
#endif

