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

// newview
#include "llagent.h"
#include "llcolorswatch.h"
#include "llregioninfomodel.h"
#include "llviewerregion.h"

#include "v3colorutil.h"
#include "llenvironment.h"
#include "llenvadapters.h"

static const F32 WL_SUN_AMBIENT_SLIDER_SCALE = 3.0f;
static const F32 WL_BLUE_HORIZON_DENSITY_SCALE = 2.0f;
static const F32 WL_CLOUD_SLIDER_SCALE = 1.0f;

static F32 sun_pos_to_time24(F32 sun_pos)
{
	return fmodf(sun_pos * 24.0f + 6, 24.0f);
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

	initCallbacks();

	// Create the sun position scrubber on the slider.
	getChild<LLMultiSliderCtrl>("WLSunPos")->addSlider(12.f);

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
//		LLEnvManagerNew::instance().usePrefs(); // revert changes made to current environment
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

	// blue horizon
	getChild<LLUICtrl>("WLBlueHorizon")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &mSkyAdapter->mBlueHorizon));

	// haze density, horizon, mult, and altitude
    getChild<LLUICtrl>("WLHazeDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mHazeDensity));
    getChild<LLUICtrl>("WLHazeHorizon")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mHazeHorizon));
    getChild<LLUICtrl>("WLDensityMult")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mDensityMult));
    getChild<LLUICtrl>("WLMaxAltitude")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mMaxAlt));

	// blue density
    getChild<LLUICtrl>("WLBlueDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &mSkyAdapter->mBlueDensity));

	// Lighting

	// sunlight
    getChild<LLUICtrl>("WLSunlight")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &mSkyAdapter->mSunlight));

	// glow
    getChild<LLUICtrl>("WLGlowR")->setCommitCallback(boost::bind(&LLFloaterEditSky::onGlowRMoved, this, _1, &mSkyAdapter->mGlow));
    getChild<LLUICtrl>("WLGlowB")->setCommitCallback(boost::bind(&LLFloaterEditSky::onGlowBMoved, this, _1, &mSkyAdapter->mGlow));

	// ambient
    getChild<LLUICtrl>("WLAmbient")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &mSkyAdapter->mAmbient));

	// time of day
    getChild<LLUICtrl>("WLSunPos")->setCommitCallback(boost::bind(&LLFloaterEditSky::onSunMoved, this, _1, &mSkyAdapter->mLightnorm));     // multi-slider
	getChild<LLTimeCtrl>("WLDayTime")->setCommitCallback(boost::bind(&LLFloaterEditSky::onTimeChanged, this));                          // time ctrl
    getChild<LLUICtrl>("WLEastAngle")->setCommitCallback(boost::bind(&LLFloaterEditSky::onSunMoved, this, _1, &mSkyAdapter->mLightnorm));

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
    getChild<LLUICtrl>("WLDistanceMult")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mDistanceMult));

	// Dome
    getChild<LLUICtrl>("WLGamma")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &mSkyAdapter->mWLGamma));
	getChild<LLUICtrl>("WLStarAlpha")->setCommitCallback(boost::bind(&LLFloaterEditSky::onStarAlphaMoved, this, _1));
}

//=================================================================================================

void LLFloaterEditSky::syncControls()
{
    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();
    mEditSettings = psky;


	// blue horizon
	mSkyAdapter->mBlueHorizon.setColor3( psky->getBlueHorizon() );
	setColorSwatch("WLBlueHorizon", mSkyAdapter->mBlueHorizon, WL_BLUE_HORIZON_DENSITY_SCALE);

	// haze density, horizon, mult, and altitude
    mSkyAdapter->mHazeDensity = psky->getHazeDensity();
	childSetValue("WLHazeDensity", (F32) mSkyAdapter->mHazeDensity);
    mSkyAdapter->mHazeHorizon = psky->getHazeHorizon();
	childSetValue("WLHazeHorizon", (F32) mSkyAdapter->mHazeHorizon);
    mSkyAdapter->mDensityMult = psky->getDensityMultiplier();
	childSetValue("WLDensityMult", ((F32) mSkyAdapter->mDensityMult) * mSkyAdapter->mDensityMult.getMult());
    mSkyAdapter->mMaxAlt = psky->getMaxY();
	childSetValue("WLMaxAltitude", (F32) mSkyAdapter->mMaxAlt);

	// blue density
    mSkyAdapter->mBlueDensity.setColor3( psky->getBlueDensity() );
	setColorSwatch("WLBlueDensity", mSkyAdapter->mBlueDensity, WL_BLUE_HORIZON_DENSITY_SCALE);

	// Lighting

	// sunlight
    mSkyAdapter->mSunlight.setColor3( psky->getSunlightColor() );
	setColorSwatch("WLSunlight", mSkyAdapter->mSunlight, WL_SUN_AMBIENT_SLIDER_SCALE);

	// glow
    mSkyAdapter->mGlow.setColor3( psky->getGlow() );
	childSetValue("WLGlowR", 2 - mSkyAdapter->mGlow.getRed() / 20.0f);
	childSetValue("WLGlowB", -mSkyAdapter->mGlow.getBlue() / 5.0f);

	// ambient
    mSkyAdapter->mAmbient.setColor3( psky->getAmbientColor() );
	setColorSwatch("WLAmbient", mSkyAdapter->mAmbient, WL_SUN_AMBIENT_SLIDER_SCALE);

    LLSettingsSky::azimalt_t azal = psky->getSunRotationAzAl();

	F32 time24 = sun_pos_to_time24(azal.second / F_TWO_PI);
	getChild<LLMultiSliderCtrl>("WLSunPos")->setCurSliderValue(time24, TRUE);
	getChild<LLTimeCtrl>("WLDayTime")->setTime24(time24);
	childSetValue("WLEastAngle", azal.first / F_TWO_PI);

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

    mSkyAdapter->mDistanceMult = psky->getDistanceMultiplier();
	childSetValue("WLDistanceMult", (F32) mSkyAdapter->mDistanceMult);

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
	//LLWLParamManager::getInstance()->mAnimator.deactivate();

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
#if 0
	mSkyPresetCombo->removeall();

	LLWLParamManager::preset_name_list_t region_presets, user_presets, sys_presets;
	LLWLParamManager::instance().getPresetNames(region_presets, user_presets, sys_presets);

#if 0 // Disable editing region skies until the workflow is clear enough.
	// Add region presets.
	std::string region_name = gAgent.getRegion() ? gAgent.getRegion()->getName() : LLTrans::getString("Unknown");
	for (LLWLParamManager::preset_name_list_t::const_iterator it = region_presets.begin(); it != region_presets.end(); ++it)
	{
		std::string item_title = *it + " (" + region_name + ")";
		mSkyPresetCombo->add(item_title, LLWLParamKey(*it, LLEnvKey::SCOPE_REGION).toLLSD());
	}
	if (region_presets.size() > 0)
	{
		mSkyPresetCombo->addSeparator();
	}
#endif

	// Add user presets.
	for (LLWLParamManager::preset_name_list_t::const_iterator it = user_presets.begin(); it != user_presets.end(); ++it)
	{
		mSkyPresetCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toLLSD());
	}
	if (user_presets.size() > 0)
	{
		mSkyPresetCombo->addSeparator();
	}

	// Add system presets.
	for (LLWLParamManager::preset_name_list_t::const_iterator it = sys_presets.begin(); it != sys_presets.end(); ++it)
	{
		mSkyPresetCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toLLSD());
	}

	mSkyPresetCombo->setLabel(getString("combo_label"));
#endif
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

// LLWLParamKey LLFloaterEditSky::getSelectedSkyPreset()
// {
// 	LLWLParamKey key;
// 
// 	if (mSkyPresetNameEditor->getVisible())
// 	{
// 		key.name = mSkyPresetNameEditor->getText();
// 		key.scope = LLEnvKey::SCOPE_LOCAL;
// 	}
// 	else
// 	{
// 		LLSD combo_val = mSkyPresetCombo->getValue();
// 
// 		if (!combo_val.isArray()) // manually typed text
// 		{
// 			key.name = combo_val.asString();
// 			key.scope = LLEnvKey::SCOPE_LOCAL;
// 		}
// 		else
// 		{
// 			key.fromLLSD(combo_val);
// 		}
// 	}
// 
// 	return key;
// }

void LLFloaterEditSky::onSkyPresetNameEdited()
{
#if 0
	// Disable saving a sky preset having empty name.
	LLWLParamKey key = getSelectedSkyPreset();
	mSaveButton->setEnabled(!key.name.empty());
#endif
}

void LLFloaterEditSky::onSkyPresetSelected()
{
#if 0
	LLWLParamKey key = getSelectedSkyPreset();
	LLWLParamSet sky_params;

	if (!LLWLParamManager::instance().getParamSet(key, sky_params))
	{
		// Manually entered string?
		LL_WARNS("Windlight") << "No sky preset named " << key.toString() << LL_ENDL;
		return;
	}

	LLEnvManagerNew::instance().useSkyParams(sky_params.getAll());
	//syncControls();

	bool can_edit = (key.scope == LLEnvKey::SCOPE_LOCAL || LLEnvManagerNew::canEditRegionSettings());
	enableEditing(can_edit);

	mMakeDefaultCheckBox->setEnabled(key.scope == LLEnvKey::SCOPE_LOCAL);
#endif
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
#if 0
	// Save current params to the selected preset.
	LLWLParamKey key(getSelectedSkyPreset());

	LL_DEBUGS("Windlight") << "Saving sky preset " << key.name << LL_ENDL;
	LLWLParamManager& wl_mgr = LLWLParamManager::instance();
	if (wl_mgr.hasParamSet(key))
	{
		wl_mgr.setParamSet(key, wl_mgr.mCurParams);
	}
	else
	{
		wl_mgr.addParamSet(key, wl_mgr.mCurParams);
	}

	wl_mgr.savePreset(key);

	// Change preference if requested.
	if (mMakeDefaultCheckBox->getValue())
	{
		LL_DEBUGS("Windlight") << key.name << " is now the new preferred sky preset" << LL_ENDL;
		LLEnvManagerNew::instance().setUseSkyPreset(key.name);
	}

	closeFloater();
#endif
}

void LLFloaterEditSky::onBtnSave()
{
#if 0
	LLWLParamKey selected_sky = getSelectedSkyPreset();
	LLWLParamManager& wl_mgr = LLWLParamManager::instance();

	if (selected_sky.scope == LLEnvKey::SCOPE_REGION)
	{
		saveRegionSky();
		closeFloater();
		return;
	}

	std::string name = selected_sky.name;
	if (name.empty())
	{
		// *TODO: show an alert
		LL_WARNS() << "Empty sky preset name" << LL_ENDL;
		return;
	}

	// Don't allow overwriting system presets.
	if (wl_mgr.isSystemPreset(name))
	{
		LLNotificationsUtil::add("WLNoEditDefault");
		return;
	}

	// Save, ask for confirmation for overwriting an existing preset.
	if (wl_mgr.hasParamSet(selected_sky))
	{
		LLNotificationsUtil::add("WLSavePresetAlert", LLSD(), LLSD(), boost::bind(&LLFloaterEditSky::onSaveAnswer, this, _1, _2));
	}
	else
	{
		// new preset, hence no confirmation needed
		onSaveConfirmed();
	}
#endif
}

void LLFloaterEditSky::onBtnCancel()
{
	closeFloater();
}

void LLFloaterEditSky::onSkyPresetListChange()
{
#if 0
	LLWLParamKey key = getSelectedSkyPreset(); // preset being edited
	if (!LLWLParamManager::instance().hasParamSet(key))
	{
		// Preset we've been editing doesn't exist anymore. Close the floater.
		closeFloater(false);
	}
	else
	{
		// A new preset has been added.
		// Refresh the presets list, though it may not make sense as the floater is about to be closed.
		refreshSkyPresetsList();
	}
#endif
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
