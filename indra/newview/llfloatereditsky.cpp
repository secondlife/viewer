/** 
 * @file llfloatereditsky.h
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
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llsliderctrl.h"
#include "lltabcontainer.h"

// newview
#include "llagent.h"
#include "llregioninfomodel.h"
#include "llviewerregion.h"

#undef max

static const F32 WL_SUN_AMBIENT_SLIDER_SCALE = 3.0f;

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
	getChild<LLUICtrl>("WLBlueHorizonR")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &param_mgr.mBlueHorizon));
	getChild<LLUICtrl>("WLBlueHorizonR")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &param_mgr.mBlueHorizon));
	getChild<LLUICtrl>("WLBlueHorizonG")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlGMoved, this, _1, &param_mgr.mBlueHorizon));
	getChild<LLUICtrl>("WLBlueHorizonB")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlBMoved, this, _1, &param_mgr.mBlueHorizon));
	getChild<LLUICtrl>("WLBlueHorizonI")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlIMoved, this, _1, &param_mgr.mBlueHorizon));

	// haze density, horizon, mult, and altitude
	getChild<LLUICtrl>("WLHazeDensity")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &param_mgr.mHazeDensity));
	getChild<LLUICtrl>("WLHazeHorizon")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &param_mgr.mHazeHorizon));
	getChild<LLUICtrl>("WLDensityMult")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &param_mgr.mDensityMult));
	getChild<LLUICtrl>("WLMaxAltitude")->setCommitCallback(boost::bind(&LLFloaterEditSky::onFloatControlMoved, this, _1, &param_mgr.mMaxAlt));

	// blue density
	getChild<LLUICtrl>("WLBlueDensityR")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &param_mgr.mBlueDensity));
	getChild<LLUICtrl>("WLBlueDensityG")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlGMoved, this, _1, &param_mgr.mBlueDensity));
	getChild<LLUICtrl>("WLBlueDensityB")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlBMoved, this, _1, &param_mgr.mBlueDensity));
	getChild<LLUICtrl>("WLBlueDensityI")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlIMoved, this, _1, &param_mgr.mBlueDensity));

	// Lighting

	// sunlight
	getChild<LLUICtrl>("WLSunlightR")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &param_mgr.mSunlight));
	getChild<LLUICtrl>("WLSunlightG")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlGMoved, this, _1, &param_mgr.mSunlight));
	getChild<LLUICtrl>("WLSunlightB")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlBMoved, this, _1, &param_mgr.mSunlight));
	getChild<LLUICtrl>("WLSunlightI")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlIMoved, this, _1, &param_mgr.mSunlight));

	// glow
	getChild<LLUICtrl>("WLGlowR")->setCommitCallback(boost::bind(&LLFloaterEditSky::onGlowRMoved, this, _1, &param_mgr.mGlow));
	getChild<LLUICtrl>("WLGlowB")->setCommitCallback(boost::bind(&LLFloaterEditSky::onGlowBMoved, this, _1, &param_mgr.mGlow));

	// ambient
	getChild<LLUICtrl>("WLAmbientR")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &param_mgr.mAmbient));
	getChild<LLUICtrl>("WLAmbientG")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlGMoved, this, _1, &param_mgr.mAmbient));
	getChild<LLUICtrl>("WLAmbientB")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlBMoved, this, _1, &param_mgr.mAmbient));
	getChild<LLUICtrl>("WLAmbientI")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlIMoved, this, _1, &param_mgr.mAmbient));

	// time of day
	getChild<LLUICtrl>("WLSunAngle")->setCommitCallback(boost::bind(&LLFloaterEditSky::onSunMoved, this, _1, &param_mgr.mLightnorm));
	getChild<LLUICtrl>("WLEastAngle")->setCommitCallback(boost::bind(&LLFloaterEditSky::onSunMoved, this, _1, &param_mgr.mLightnorm));

	// Clouds

	// Cloud Color
	getChild<LLUICtrl>("WLCloudColorR")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlRMoved, this, _1, &param_mgr.mCloudColor));
	getChild<LLUICtrl>("WLCloudColorG")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlGMoved, this, _1, &param_mgr.mCloudColor));
	getChild<LLUICtrl>("WLCloudColorB")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlBMoved, this, _1, &param_mgr.mCloudColor));
	getChild<LLUICtrl>("WLCloudColorI")->setCommitCallback(boost::bind(&LLFloaterEditSky::onColorControlIMoved, this, _1, &param_mgr.mCloudColor));

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
	childSetValue("WLBlueHorizonR", param_mgr->mBlueHorizon.r / 2.0);
	childSetValue("WLBlueHorizonG", param_mgr->mBlueHorizon.g / 2.0);
	childSetValue("WLBlueHorizonB", param_mgr->mBlueHorizon.b / 2.0);
	childSetValue("WLBlueHorizonI",
		std::max(param_mgr->mBlueHorizon.r / 2.0,
			std::max(param_mgr->mBlueHorizon.g / 2.0,
				param_mgr->mBlueHorizon.b / 2.0)));

	// haze density, horizon, mult, and altitude
	param_mgr->mHazeDensity = cur_params.getVector(param_mgr->mHazeDensity.mName, err);
	childSetValue("WLHazeDensity", param_mgr->mHazeDensity.r);
	param_mgr->mHazeHorizon = cur_params.getVector(param_mgr->mHazeHorizon.mName, err);
	childSetValue("WLHazeHorizon", param_mgr->mHazeHorizon.r);
	param_mgr->mDensityMult = cur_params.getVector(param_mgr->mDensityMult.mName, err);
	childSetValue("WLDensityMult", param_mgr->mDensityMult.x *
		param_mgr->mDensityMult.mult);
	param_mgr->mMaxAlt = cur_params.getVector(param_mgr->mMaxAlt.mName, err);
	childSetValue("WLMaxAltitude", param_mgr->mMaxAlt.x);

	// blue density
	param_mgr->mBlueDensity = cur_params.getVector(param_mgr->mBlueDensity.mName, err);
	childSetValue("WLBlueDensityR", param_mgr->mBlueDensity.r / 2.0);
	childSetValue("WLBlueDensityG", param_mgr->mBlueDensity.g / 2.0);
	childSetValue("WLBlueDensityB", param_mgr->mBlueDensity.b / 2.0);
	childSetValue("WLBlueDensityI",
		std::max(param_mgr->mBlueDensity.r / 2.0,
		std::max(param_mgr->mBlueDensity.g / 2.0, param_mgr->mBlueDensity.b / 2.0)));

	// Lighting

	// sunlight
	param_mgr->mSunlight = cur_params.getVector(param_mgr->mSunlight.mName, err);
	childSetValue("WLSunlightR", param_mgr->mSunlight.r / WL_SUN_AMBIENT_SLIDER_SCALE);
	childSetValue("WLSunlightG", param_mgr->mSunlight.g / WL_SUN_AMBIENT_SLIDER_SCALE);
	childSetValue("WLSunlightB", param_mgr->mSunlight.b / WL_SUN_AMBIENT_SLIDER_SCALE);
	childSetValue("WLSunlightI",
		std::max(param_mgr->mSunlight.r / WL_SUN_AMBIENT_SLIDER_SCALE,
		std::max(param_mgr->mSunlight.g / WL_SUN_AMBIENT_SLIDER_SCALE, param_mgr->mSunlight.b / WL_SUN_AMBIENT_SLIDER_SCALE)));

	// glow
	param_mgr->mGlow = cur_params.getVector(param_mgr->mGlow.mName, err);
	childSetValue("WLGlowR", 2 - param_mgr->mGlow.r / 20.0f);
	childSetValue("WLGlowB", -param_mgr->mGlow.b / 5.0f);

	// ambient
	param_mgr->mAmbient = cur_params.getVector(param_mgr->mAmbient.mName, err);
	childSetValue("WLAmbientR", param_mgr->mAmbient.r / WL_SUN_AMBIENT_SLIDER_SCALE);
	childSetValue("WLAmbientG", param_mgr->mAmbient.g / WL_SUN_AMBIENT_SLIDER_SCALE);
	childSetValue("WLAmbientB", param_mgr->mAmbient.b / WL_SUN_AMBIENT_SLIDER_SCALE);
	childSetValue("WLAmbientI",
		std::max(param_mgr->mAmbient.r / WL_SUN_AMBIENT_SLIDER_SCALE,
		std::max(param_mgr->mAmbient.g / WL_SUN_AMBIENT_SLIDER_SCALE, param_mgr->mAmbient.b / WL_SUN_AMBIENT_SLIDER_SCALE)));

	childSetValue("WLSunAngle", param_mgr->mCurParams.getFloat("sun_angle",err) / F_TWO_PI);
	childSetValue("WLEastAngle", param_mgr->mCurParams.getFloat("east_angle",err) / F_TWO_PI);

	// Clouds

	// Cloud Color
	param_mgr->mCloudColor = cur_params.getVector(param_mgr->mCloudColor.mName, err);
	childSetValue("WLCloudColorR", param_mgr->mCloudColor.r);
	childSetValue("WLCloudColorG", param_mgr->mCloudColor.g);
	childSetValue("WLCloudColorB", param_mgr->mCloudColor.b);
	childSetValue("WLCloudColorI",
		std::max(param_mgr->mCloudColor.r,
		std::max(param_mgr->mCloudColor.g, param_mgr->mCloudColor.b)));

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
	param_mgr->mCloudCoverage = cur_params.getVector(param_mgr->mCloudCoverage.mName, err);
	param_mgr->mCloudScale = cur_params.getVector(param_mgr->mCloudScale.mName, err);
	childSetValue("WLCloudCoverage", param_mgr->mCloudCoverage.x);
	childSetValue("WLCloudScale", param_mgr->mCloudScale.x);

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

	param_mgr->mDistanceMult = cur_params.getVector(param_mgr->mDistanceMult.mName, err);
	childSetValue("WLDistanceMult", param_mgr->mDistanceMult.x);

	// Tweak extras

	param_mgr->mWLGamma = cur_params.getVector(param_mgr->mWLGamma.mName, err);
	childSetValue("WLGamma", param_mgr->mWLGamma.x);

	childSetValue("WLStarAlpha", param_mgr->mCurParams.getStarBrightness());
}


// color control callbacks
void LLFloaterEditSky::onColorControlRMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	color_ctrl->r = sldr_ctrl->getValueF32();
	if (color_ctrl->isSunOrAmbientColor)
	{
		color_ctrl->r *= 3;
	}
	if (color_ctrl->isBlueHorizonOrDensity)
	{
		color_ctrl->r *= 2;
	}

	// move i if it's the max
	if (color_ctrl->r >= color_ctrl->g && color_ctrl->r >= color_ctrl->b && color_ctrl->hasSliderName)
	{
		color_ctrl->i = color_ctrl->r;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		if (color_ctrl->isSunOrAmbientColor)
		{
			childSetValue(name, color_ctrl->r / 3);
		}
		else if	(color_ctrl->isBlueHorizonOrDensity)
		{
			childSetValue(name, color_ctrl->r / 2);
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
		color_ctrl->g *= 3;
	}
	if (color_ctrl->isBlueHorizonOrDensity)
	{
		color_ctrl->g *= 2;
	}

	// move i if it's the max
	if (color_ctrl->g >= color_ctrl->r && color_ctrl->g >= color_ctrl->b && color_ctrl->hasSliderName)
	{
		color_ctrl->i = color_ctrl->g;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		if (color_ctrl->isSunOrAmbientColor)
		{
			childSetValue(name, color_ctrl->g / 3);
		}
		else if (color_ctrl->isBlueHorizonOrDensity)
		{
			childSetValue(name, color_ctrl->g / 2);
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
		color_ctrl->b *= 3;
	}
	if (color_ctrl->isBlueHorizonOrDensity)
	{
		color_ctrl->b *= 2;
	}

	// move i if it's the max
	if (color_ctrl->b >= color_ctrl->r && color_ctrl->b >= color_ctrl->g && color_ctrl->hasSliderName)
	{
		color_ctrl->i = color_ctrl->b;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		if (color_ctrl->isSunOrAmbientColor)
		{
			childSetValue(name, color_ctrl->b / 3);
		}
		else if (color_ctrl->isBlueHorizonOrDensity)
		{
			childSetValue(name, color_ctrl->b / 2);
		}
		else
		{
			childSetValue(name, color_ctrl->b);
		}
	}

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);

	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditSky::onColorControlIMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	color_ctrl->i = sldr_ctrl->getValueF32();

	// only for sliders where we pass a name
	if (color_ctrl->hasSliderName)
	{
		// set it to the top
		F32 maxVal = std::max(std::max(color_ctrl->r, color_ctrl->g), color_ctrl->b);
		F32 iVal;

		if (color_ctrl->isSunOrAmbientColor)
		{
			iVal = color_ctrl->i * 3;
		}
		else if (color_ctrl->isBlueHorizonOrDensity)
		{
			iVal = color_ctrl->i * 2;
		}
		else
		{
			iVal = color_ctrl->i;
		}

		// get the names of the other sliders
		std::string rName = color_ctrl->mSliderName;
		rName.append("R");
		std::string gName = color_ctrl->mSliderName;
		gName.append("G");
		std::string bName = color_ctrl->mSliderName;
		bName.append("B");

		// handle if at 0
		if (iVal == 0)
		{
			color_ctrl->r = 0;
			color_ctrl->g = 0;
			color_ctrl->b = 0;

		// if all at the start
		// set them all to the intensity
		}
		else if (maxVal == 0)
		{
			color_ctrl->r = iVal;
			color_ctrl->g = iVal;
			color_ctrl->b = iVal;

		}
		else
		{
			// add delta amounts to each
			F32 delta = (iVal - maxVal) / maxVal;
			color_ctrl->r *= (1.0f + delta);
			color_ctrl->g *= (1.0f + delta);
			color_ctrl->b *= (1.0f + delta);
		}

		// divide sun color vals by three
		if (color_ctrl->isSunOrAmbientColor)
		{
			childSetValue(rName, color_ctrl->r/3);
			childSetValue(gName, color_ctrl->g/3);
			childSetValue(bName, color_ctrl->b/3);
		}
		else if (color_ctrl->isBlueHorizonOrDensity)
		{
			childSetValue(rName, color_ctrl->r/2);
			childSetValue(gName, color_ctrl->g/2);
			childSetValue(bName, color_ctrl->b/2);
		}
		else
		{
			// set the sliders to the new vals
			childSetValue(rName, color_ctrl->r);
			childSetValue(gName, color_ctrl->g);
			childSetValue(bName, color_ctrl->b);
		}
	}

	// now update the current parameters and send them to shaders
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

	LLSliderCtrl* sun_sldr = getChild<LLSliderCtrl>("WLSunAngle");
	LLSliderCtrl* east_sldr = getChild<LLSliderCtrl>("WLEastAngle");

	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	// get the two angles
	LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

	param_mgr->mCurParams.setSunAngle(F_TWO_PI * sun_sldr->getValueF32());
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

	std::string region_name = gAgent.getRegion() ? gAgent.getRegion()->getName() : LLTrans::getString("Unknown");
	const std::map<LLWLParamKey, LLWLParamSet> &sky_params_map = LLWLParamManager::getInstance()->mParamList;
	for (std::map<LLWLParamKey, LLWLParamSet>::const_iterator it = sky_params_map.begin(); it != sky_params_map.end(); it++)
	{
		const LLWLParamKey& key = it->first;
		std::string item_title = key.name;
		if (key.scope == LLEnvKey::SCOPE_REGION)
		{
			item_title += " (" + region_name + ")";
		}
		mSkyPresetCombo->add(item_title, key.toLLSD());
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

	// If we've selected the region day cycle for editing.
	if (getSelectedSkyPreset().scope == LLEnvKey::SCOPE_REGION)
	{
		// check whether we have the access
		can_edit = LLEnvManagerNew::canEditRegionSettings();
	}

	enableEditing(can_edit);
}
