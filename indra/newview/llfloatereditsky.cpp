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

LLFloaterEditSky::LLFloaterEditSky(const LLSD &key)
:	LLFloater(key)
,	mSkyPresetNameEditor(NULL)
,	mSkyPresetCombo(NULL)
,	mMakeDefaultCheckBox(NULL)
,	mSaveButton(NULL)
{
}

// virtual
BOOL LLFloaterEditSky::postBuild()
{
	mSkyPresetNameEditor = getChild<LLLineEditor>("sky_preset_name");
	mSkyPresetCombo = getChild<LLComboBox>("sky_preset_combo");
	mMakeDefaultCheckBox = getChild<LLCheckBoxCtrl>("make_default_cb");
	mSaveButton = getChild<LLButton>("save");

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
		LLEnvManagerNew::instance().usePrefs(); // revert changes made to current environment
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

	LLEnvManagerNew::instance().setRegionSettingsChangeCallback(boost::bind(&LLFloaterEditSky::onRegionSettingsChange, this));
	LLWLParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEditSky::onSkyPresetListChange, this));

	// Connect to region info updates.
	LLRegionInfoModel::instance().setUpdateCallback(boost::bind(&LLFloaterEditSky::onRegionInfoUpdate, this));

	//-------------------------------------------------------------------------

	LLWLParamManager& param_mgr = LLWLParamManager::instance();

	// blue horizon
	getChild<LLUICtrl>("WLBlueHorizon")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &param_mgr.mBlueHorizon));

	// haze density, horizon, mult, and altitude
	getChild<LLUICtrl>("WLHazeDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &param_mgr.mHazeDensity));
	getChild<LLUICtrl>("WLHazeHorizon")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &param_mgr.mHazeHorizon));
	getChild<LLUICtrl>("WLDensityMult")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &param_mgr.mDensityMult));
	getChild<LLUICtrl>("WLMaxAltitude")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &param_mgr.mMaxAlt));

	// blue density
	getChild<LLUICtrl>("WLBlueDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &param_mgr.mBlueDensity));

	// Lighting

	// sunlight
	getChild<LLUICtrl>("WLSunlight")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &param_mgr.mSunlight));

	// glow
	getChild<LLUICtrl>("WLGlowR")->setCommitCallback(boost::bind(&LLFloaterEditSky::onGlowRMoved, this, _1, &param_mgr.mGlow));
	getChild<LLUICtrl>("WLGlowB")->setCommitCallback(boost::bind(&LLFloaterEditSky::onGlowBMoved, this, _1, &param_mgr.mGlow));

	// ambient
	getChild<LLUICtrl>("WLAmbient")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &param_mgr.mAmbient));

	// time of day
	getChild<LLUICtrl>("WLSunPos")->setCommitCallback(boost::bind(&LLFloaterEditSky::onSunMoved, this, _1, &param_mgr.mLightnorm));     // multi-slider
	getChild<LLTimeCtrl>("WLDayTime")->setCommitCallback(boost::bind(&LLFloaterEditSky::onTimeChanged, this));                          // time ctrl
	getChild<LLUICtrl>("WLEastAngle")->setCommitCallback(boost::bind(&LLFloaterEditSky::onSunMoved, this, _1, &param_mgr.mLightnorm));

	// Clouds

	// Cloud Color
	getChild<LLUICtrl>("WLCloudColor")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlMoved, this, _1, &param_mgr.mCloudColor));

	// Cloud
	getChild<LLUICtrl>("WLCloudX")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &param_mgr.mCloudMain));
	getChild<LLUICtrl>("WLCloudY")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlGMoved, this, _1, &param_mgr.mCloudMain));
	getChild<LLUICtrl>("WLCloudDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlBMoved, this, _1, &param_mgr.mCloudMain));

	// Cloud Detail
	getChild<LLUICtrl>("WLCloudDetailX")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &param_mgr.mCloudDetail));
	getChild<LLUICtrl>("WLCloudDetailY")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlGMoved, this, _1, &param_mgr.mCloudDetail));
	getChild<LLUICtrl>("WLCloudDetailDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlBMoved, this, _1, &param_mgr.mCloudDetail));

	// Cloud extras
	getChild<LLUICtrl>("WLCloudCoverage")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &param_mgr.mCloudCoverage));
	getChild<LLUICtrl>("WLCloudScale")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &param_mgr.mCloudScale));
	getChild<LLUICtrl>("WLCloudLockX")->setCommitCallback(boost::bind(&LLFloaterEditSky::onCloudScrollXToggled, this, _1));
	getChild<LLUICtrl>("WLCloudLockY")->setCommitCallback(boost::bind(&LLFloaterEditSky::onCloudScrollYToggled, this, _1));
	getChild<LLUICtrl>("WLCloudScrollX")->setCommitCallback(boost::bind(&LLFloaterEditSky::onCloudScrollXMoved, this, _1));
	getChild<LLUICtrl>("WLCloudScrollY")->setCommitCallback(boost::bind(&LLFloaterEditSky::onCloudScrollYMoved, this, _1));
	getChild<LLUICtrl>("WLDistanceMult")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &param_mgr.mDistanceMult));

	// Dome
	getChild<LLUICtrl>("WLGamma")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &param_mgr.mWLGamma));
	getChild<LLUICtrl>("WLStarAlpha")->setCommitCallback(boost::bind(&LLFloaterEditSky::onStarAlphaMoved, this, _1));
}

//=================================================================================================

void LLFloaterEditSky::syncControls()
{
	bool err;

	LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

	LLWLParamSet& cur_params = param_mgr->mCurParams;

	// blue horizon
	param_mgr->mBlueHorizon = cur_params.getVector(param_mgr->mBlueHorizon.mName, err);
	setColorSwatch("WLBlueHorizon", param_mgr->mBlueHorizon, WL_BLUE_HORIZON_DENSITY_SCALE);

	// haze density, horizon, mult, and altitude
	param_mgr->mHazeDensity = cur_params.getFloat(param_mgr->mHazeDensity.mName, err);
	childSetValue("WLHazeDensity", (F32) param_mgr->mHazeDensity);
	param_mgr->mHazeHorizon = cur_params.getFloat(param_mgr->mHazeHorizon.mName, err);
	childSetValue("WLHazeHorizon", (F32) param_mgr->mHazeHorizon);
	param_mgr->mDensityMult = cur_params.getFloat(param_mgr->mDensityMult.mName, err);
	childSetValue("WLDensityMult", ((F32) param_mgr->mDensityMult) * param_mgr->mDensityMult.mult);
	param_mgr->mMaxAlt = cur_params.getFloat(param_mgr->mMaxAlt.mName, err);
	childSetValue("WLMaxAltitude", (F32) param_mgr->mMaxAlt);

	// blue density
	param_mgr->mBlueDensity = cur_params.getVector(param_mgr->mBlueDensity.mName, err);
	setColorSwatch("WLBlueDensity", param_mgr->mBlueDensity, WL_BLUE_HORIZON_DENSITY_SCALE);

	// Lighting

	// sunlight
	param_mgr->mSunlight = cur_params.getVector(param_mgr->mSunlight.mName, err);
	setColorSwatch("WLSunlight", param_mgr->mSunlight, WL_SUN_AMBIENT_SLIDER_SCALE);

	// glow
	param_mgr->mGlow = cur_params.getVector(param_mgr->mGlow.mName, err);
	childSetValue("WLGlowR", 2 - param_mgr->mGlow.r / 20.0f);
	childSetValue("WLGlowB", -param_mgr->mGlow.b / 5.0f);

	// ambient
	param_mgr->mAmbient = cur_params.getVector(param_mgr->mAmbient.mName, err);
	setColorSwatch("WLAmbient", param_mgr->mAmbient, WL_SUN_AMBIENT_SLIDER_SCALE);

	F32 time24 = sun_pos_to_time24(param_mgr->mCurParams.getFloat("sun_angle",err) / F_TWO_PI);
	getChild<LLMultiSliderCtrl>("WLSunPos")->setCurSliderValue(time24, TRUE);
	getChild<LLTimeCtrl>("WLDayTime")->setTime24(time24);
	childSetValue("WLEastAngle", param_mgr->mCurParams.getFloat("east_angle",err) / F_TWO_PI);

	// Clouds

	// Cloud Color
	param_mgr->mCloudColor = cur_params.getVector(param_mgr->mCloudColor.mName, err);
	setColorSwatch("WLCloudColor", param_mgr->mCloudColor, WL_CLOUD_SLIDER_SCALE);

	// Cloud
	param_mgr->mCloudMain = cur_params.getVector(param_mgr->mCloudMain.mName, err);
	childSetValue("WLCloudX", param_mgr->mCloudMain.r);
	childSetValue("WLCloudY", param_mgr->mCloudMain.g);
	childSetValue("WLCloudDensity", param_mgr->mCloudMain.b);

	// Cloud Detail
	param_mgr->mCloudDetail = cur_params.getVector(param_mgr->mCloudDetail.mName, err);
	childSetValue("WLCloudDetailX", param_mgr->mCloudDetail.r);
	childSetValue("WLCloudDetailY", param_mgr->mCloudDetail.g);
	childSetValue("WLCloudDetailDensity", param_mgr->mCloudDetail.b);

	// Cloud extras
	param_mgr->mCloudCoverage = cur_params.getFloat(param_mgr->mCloudCoverage.mName, err);
	param_mgr->mCloudScale = cur_params.getFloat(param_mgr->mCloudScale.mName, err);
	childSetValue("WLCloudCoverage", (F32) param_mgr->mCloudCoverage);
	childSetValue("WLCloudScale", (F32) param_mgr->mCloudScale);

	// cloud scrolling
	bool lockX = !param_mgr->mCurParams.getEnableCloudScrollX();
	bool lockY = !param_mgr->mCurParams.getEnableCloudScrollY();
	childSetValue("WLCloudLockX", lockX);
	childSetValue("WLCloudLockY", lockY);

	// disable if locked, enable if not
	if (lockX)
	{
		childDisable("WLCloudScrollX");
	}
	else
	{
		childEnable("WLCloudScrollX");
	}
	if (lockY)
	{
		childDisable("WLCloudScrollY");
	}
	else
	{
		childEnable("WLCloudScrollY");
	}

	// *HACK cloud scrolling is off my an additive of 10
	childSetValue("WLCloudScrollX", param_mgr->mCurParams.getCloudScrollX() - 10.0f);
	childSetValue("WLCloudScrollY", param_mgr->mCurParams.getCloudScrollY() - 10.0f);

	param_mgr->mDistanceMult = cur_params.getFloat(param_mgr->mDistanceMult.mName, err);
	childSetValue("WLDistanceMult", (F32) param_mgr->mDistanceMult);

	// Tweak extras

	param_mgr->mWLGamma = cur_params.getFloat(param_mgr->mWLGamma.mName, err);
	childSetValue("WLGamma", (F32) param_mgr->mWLGamma);

	childSetValue("WLStarAlpha", param_mgr->mCurParams.getStarBrightness());
}

void LLFloaterEditSky::setColorSwatch(const std::string& name, const WLColorControl& from_ctrl, F32 k)
{
	// Set the value, dividing it by <k> first.
	LLVector4 color_vec = from_ctrl;
	getChild<LLColorSwatchCtrl>(name)->set(LLColor4(color_vec / k));
}

// color control callbacks
void LLFloaterEditSky::onColorControlMoved(LLUICtrl* ctrl, WLColorControl* color_ctrl)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLColorSwatchCtrl* swatch = static_cast<LLColorSwatchCtrl*>(ctrl);
	LLVector4 color_vec(swatch->get().mV);

	// Set intensity to maximum of the RGB values.
	color_vec.mV[3] = llmax(color_vec.mV[0], llmax(color_vec.mV[1], color_vec.mV[2]));

	// Multiply RGB values by the appropriate factor.
	F32 k = WL_CLOUD_SLIDER_SCALE;
	if (color_ctrl->isSunOrAmbientColor)
	{
		k = WL_SUN_AMBIENT_SLIDER_SCALE;
	}
	if (color_ctrl->isBlueHorizonOrDensity)
	{
		k = WL_BLUE_HORIZON_DENSITY_SCALE;
	}

	color_vec *= k; // intensity isn't affected by the multiplication

	// Apply the new RGBI value.
	*color_ctrl = color_vec;
	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditSky::onColorControlRMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	color_ctrl->r = sldr_ctrl->getValueF32();
	if (color_ctrl->isSunOrAmbientColor)
	{
		color_ctrl->r *= WL_SUN_AMBIENT_SLIDER_SCALE;
	}
	if (color_ctrl->isBlueHorizonOrDensity)
	{
		color_ctrl->r *= WL_BLUE_HORIZON_DENSITY_SCALE;
	}

	// move i if it's the max
	if (color_ctrl->r >= color_ctrl->g && color_ctrl->r >= color_ctrl->b && color_ctrl->hasSliderName)
	{
		color_ctrl->i = color_ctrl->r;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		if (color_ctrl->isSunOrAmbientColor)
		{
			childSetValue(name, color_ctrl->r / WL_SUN_AMBIENT_SLIDER_SCALE);
		}
		else if	(color_ctrl->isBlueHorizonOrDensity)
		{
			childSetValue(name, color_ctrl->r / WL_BLUE_HORIZON_DENSITY_SCALE);
		}
		else
		{
			childSetValue(name, color_ctrl->r);
		}
	}

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);

	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditSky::onColorControlGMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	color_ctrl->g = sldr_ctrl->getValueF32();
	if (color_ctrl->isSunOrAmbientColor)
	{
		color_ctrl->g *= WL_SUN_AMBIENT_SLIDER_SCALE;
	}
	if (color_ctrl->isBlueHorizonOrDensity)
	{
		color_ctrl->g *= WL_BLUE_HORIZON_DENSITY_SCALE;
	}

	// move i if it's the max
	if (color_ctrl->g >= color_ctrl->r && color_ctrl->g >= color_ctrl->b && color_ctrl->hasSliderName)
	{
		color_ctrl->i = color_ctrl->g;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		if (color_ctrl->isSunOrAmbientColor)
		{
			childSetValue(name, color_ctrl->g / WL_SUN_AMBIENT_SLIDER_SCALE);
		}
		else if (color_ctrl->isBlueHorizonOrDensity)
		{
			childSetValue(name, color_ctrl->g / WL_BLUE_HORIZON_DENSITY_SCALE);
		}
		else
		{
			childSetValue(name, color_ctrl->g);
		}
	}

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);

	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditSky::onColorControlBMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	color_ctrl->b = sldr_ctrl->getValueF32();
	if (color_ctrl->isSunOrAmbientColor)
	{
		color_ctrl->b *= WL_SUN_AMBIENT_SLIDER_SCALE;
	}
	if (color_ctrl->isBlueHorizonOrDensity)
	{
		color_ctrl->b *= WL_BLUE_HORIZON_DENSITY_SCALE;
	}

	// move i if it's the max
	if (color_ctrl->b >= color_ctrl->r && color_ctrl->b >= color_ctrl->g && color_ctrl->hasSliderName)
	{
		color_ctrl->i = color_ctrl->b;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		if (color_ctrl->isSunOrAmbientColor)
		{
			childSetValue(name, color_ctrl->b / WL_SUN_AMBIENT_SLIDER_SCALE);
		}
		else if (color_ctrl->isBlueHorizonOrDensity)
		{
			childSetValue(name, color_ctrl->b / WL_BLUE_HORIZON_DENSITY_SCALE);
		}
		else
		{
			childSetValue(name, color_ctrl->b);
		}
	}

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);

	LLWLParamManager::getInstance()->propagateParameters();
}

/// GLOW SPECIFIC CODE
void LLFloaterEditSky::onGlowRMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	// scaled by 20
	color_ctrl->r = (2 - sldr_ctrl->getValueF32()) * 20;

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

/// \NOTE that we want NEGATIVE (-) B
void LLFloaterEditSky::onGlowBMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	/// \NOTE that we want NEGATIVE (-) B and NOT by 20 as 20 is too big
	color_ctrl->b = -sldr_ctrl->getValueF32() * 5;

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditSky::onFloatControlMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLFloatControl * floatControl = static_cast<WLFloatControl *>(userdata);

	floatControl->x = sldr_ctrl->getValueF32() / floatControl->mult;

	floatControl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}


// Lighting callbacks

// time of day
void LLFloaterEditSky::onSunMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLMultiSliderCtrl* sun_msldr = getChild<LLMultiSliderCtrl>("WLSunPos");
	LLSliderCtrl* east_sldr = getChild<LLSliderCtrl>("WLEastAngle");
	LLTimeCtrl* time_ctrl = getChild<LLTimeCtrl>("WLDayTime");
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	F32 time24  = sun_msldr->getCurSliderValue();
	time_ctrl->setTime24(time24); // sync the time ctrl with the new sun position

	// get the two angles
	LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

	param_mgr->mCurParams.setSunAngle(F_TWO_PI * time24_to_sun_pos(time24));
	param_mgr->mCurParams.setEastAngle(F_TWO_PI * east_sldr->getValueF32());

	// set the sun vector
	color_ctrl->r = -sin(param_mgr->mCurParams.getEastAngle()) *
		cos(param_mgr->mCurParams.getSunAngle());
	color_ctrl->g = sin(param_mgr->mCurParams.getSunAngle());
	color_ctrl->b = cos(param_mgr->mCurParams.getEastAngle()) *
		cos(param_mgr->mCurParams.getSunAngle());
	color_ctrl->i = 1.f;

	color_ctrl->update(param_mgr->mCurParams);
	param_mgr->propagateParameters();
}

void LLFloaterEditSky::onTimeChanged()
{
	F32 time24 = getChild<LLTimeCtrl>("WLDayTime")->getTime24();
	getChild<LLMultiSliderCtrl>("WLSunPos")->setCurSliderValue(time24, TRUE);
	onSunMoved(getChild<LLUICtrl>("WLSunPos"), &LLWLParamManager::instance().mLightnorm);
}

void LLFloaterEditSky::onStarAlphaMoved(LLUICtrl* ctrl)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	LLWLParamManager::getInstance()->mCurParams.setStarBrightness(sldr_ctrl->getValueF32());
}

// Clouds
void LLFloaterEditSky::onCloudScrollXMoved(LLUICtrl* ctrl)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	// *HACK  all cloud scrolling is off by an additive of 10.
	LLWLParamManager::getInstance()->mCurParams.setCloudScrollX(sldr_ctrl->getValueF32() + 10.0f);
}

void LLFloaterEditSky::onCloudScrollYMoved(LLUICtrl* ctrl)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	// *HACK  all cloud scrolling is off by an additive of 10.
	LLWLParamManager::getInstance()->mCurParams.setCloudScrollY(sldr_ctrl->getValueF32() + 10.0f);
}

void LLFloaterEditSky::onCloudScrollXToggled(LLUICtrl* ctrl)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLCheckBoxCtrl* cb_ctrl = static_cast<LLCheckBoxCtrl*>(ctrl);

	bool lock = cb_ctrl->get();
	LLWLParamManager::getInstance()->mCurParams.setEnableCloudScrollX(!lock);

	LLSliderCtrl* sldr = getChild<LLSliderCtrl>("WLCloudScrollX");

	if (cb_ctrl->get())
	{
		sldr->setEnabled(false);
	}
	else
	{
		sldr->setEnabled(true);
	}

}

void LLFloaterEditSky::onCloudScrollYToggled(LLUICtrl* ctrl)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLCheckBoxCtrl* cb_ctrl = static_cast<LLCheckBoxCtrl*>(ctrl);
	bool lock = cb_ctrl->get();
	LLWLParamManager::getInstance()->mCurParams.setEnableCloudScrollY(!lock);

	LLSliderCtrl* sldr = getChild<LLSliderCtrl>("WLCloudScrollY");

	if (cb_ctrl->get())
	{
		sldr->setEnabled(false);
	}
	else
	{
		sldr->setEnabled(true);
	}
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
	LLWLParamKey key(getSelectedSkyPreset());
	llassert(key.scope == LLEnvKey::SCOPE_REGION);

	LL_DEBUGS("Windlight") << "Saving region sky preset: " << key.name  << llendl;
	LLWLParamManager& wl_mgr = LLWLParamManager::instance();
	wl_mgr.mCurParams.mName = key.name;
	wl_mgr.setParamSet(key, wl_mgr.mCurParams);

	// *TODO: save to cached region settings.
	LL_WARNS("Windlight") << "Saving region sky is not fully implemented yet" << LL_ENDL;
}

LLWLParamKey LLFloaterEditSky::getSelectedSkyPreset()
{
	LLWLParamKey key;

	if (mSkyPresetNameEditor->getVisible())
	{
		key.name = mSkyPresetNameEditor->getText();
		key.scope = LLEnvKey::SCOPE_LOCAL;
	}
	else
	{
		LLSD combo_val = mSkyPresetCombo->getValue();

		if (!combo_val.isArray()) // manually typed text
		{
			key.name = combo_val.asString();
			key.scope = LLEnvKey::SCOPE_LOCAL;
		}
		else
		{
			key.fromLLSD(combo_val);
		}
	}

	return key;
}

void LLFloaterEditSky::onSkyPresetNameEdited()
{
	// Disable saving a sky preset having empty name.
	LLWLParamKey key = getSelectedSkyPreset();
	mSaveButton->setEnabled(!key.name.empty());
}

void LLFloaterEditSky::onSkyPresetSelected()
{
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
		LL_DEBUGS("Windlight") << key.name << " is now the new preferred sky preset" << llendl;
		LLEnvManagerNew::instance().setUseSkyPreset(key.name);
	}

	closeFloater();
}

void LLFloaterEditSky::onBtnSave()
{
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
		llwarns << "Empty sky preset name" << llendl;
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
}

void LLFloaterEditSky::onBtnCancel()
{
	closeFloater();
}

void LLFloaterEditSky::onSkyPresetListChange()
{
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
}

void LLFloaterEditSky::onRegionSettingsChange()
{
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
}

void LLFloaterEditSky::onRegionInfoUpdate()
{
	bool can_edit = true;

	// If we've selected a region sky preset for editing.
	if (getSelectedSkyPreset().scope == LLEnvKey::SCOPE_REGION)
	{
		// check whether we have the access
		can_edit = LLEnvManagerNew::canEditRegionSettings();
	}

	enableEditing(can_edit);
}
