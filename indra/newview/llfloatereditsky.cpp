/** 
 * @file llfloatereditsky.cpp
 * @brief Floater to create or edit a sky preset
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

#include "llfloatereditsky.h"

#include <boost/make_shared.hpp>

// libs
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llmultisliderctrl.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llsliderctrl.h"
#include "lltabcontainer.h"
#include "lltimectrl.h"
#include "lljoystickbutton.h"

// newview
#include "llagent.h"
#include "llcolorswatch.h"
#include "llregioninfomodel.h"
#include "llviewerregion.h"

#include "v3colorutil.h"
#include "llenvironment.h"
#include "llenvadapters.h"

namespace
{
    const F32 WL_SUN_AMBIENT_SLIDER_SCALE(3.0f);
    const F32 WL_BLUE_HORIZON_DENSITY_SCALE(2.0f);
    const F32 WL_CLOUD_SLIDER_SCALE(1.0f);
}


static F32 time24_to_sun_pos(F32 time24)
{
	F32 sun_pos = fmodf((time24 - 6) / 24.0f, 1.0f);
	if (sun_pos < 0) ++sun_pos;
	return sun_pos;
}

LLFloaterEditSky::LLFloaterEditSky(const LLSD &key):	
    LLFloater(key),	
    mSkyPresetNameEditor(NULL),	
    mSkyPresetCombo(NULL),	
    mMakeDefaultCheckBox(NULL),	
    mSaveButton(NULL),
    mSkyAdapter()
{
}

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

	// Lighting

	// sunlight
    mSkyAdapter->mSunlight.setColor3( psky->getSunlightColor() );
	setColorSwatch("WLSunlight", mSkyAdapter->mSunlight, WL_SUN_AMBIENT_SLIDER_SCALE);

	// glow
    mSkyAdapter->mGlow.setColor3( psky->getGlow() );
	childSetValue("WLGlowR", 2 - mSkyAdapter->mGlow.getRed() / 20.0f);
	childSetValue("WLGlowB", -mSkyAdapter->mGlow.getBlue() / 5.0f);

// LLSettingsSky::azimalt_t azal = psky->getSunRotationAzAl();
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
