/** 
 * @file llfloaterwindlight.cpp
 * @brief LLFloaterWindLight class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterwindlight.h"

#include "pipeline.h"
#include "llsky.h"

#include "llfloaterreg.h"
#include "llsliderctrl.h"
#include "llmultislider.h"
#include "llmultisliderctrl.h"
#include "llnotificationsutil.h"
#include "llspinctrl.h"
#include "llcheckboxctrl.h"
#include "lluictrlfactory.h"
#include "llviewercamera.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llfloaterdaycycle.h"
#include "llboost.h"

#include "v4math.h"
#include "llviewerdisplay.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llsavedsettingsglue.h"

#include "llwlparamset.h"
#include "llwlparammanager.h"
#include "llpostprocess.h"
#include "lltabcontainer.h"


#undef max

std::set<std::string> LLFloaterWindLight::sDefaultPresets;

static const F32 WL_SUN_AMBIENT_SLIDER_SCALE = 3.0f;

LLFloaterWindLight::LLFloaterWindLight(const LLSD& key)
  : LLFloater(key)
{
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_windlight_options.xml");
}

LLFloaterWindLight::~LLFloaterWindLight()
{
}

BOOL LLFloaterWindLight::postBuild()
{
	// add the list of presets
	std::string def_days = getString("WLDefaultSkyNames");

	// no editing or deleting of the blank string
	sDefaultPresets.insert("");
	boost_tokenizer tokens(def_days, boost::char_separator<char>(":"));
	for (boost_tokenizer::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		std::string tok(*token_iter);
		sDefaultPresets.insert(tok);
	}

	// add the combo boxes
	LLComboBox* comboBox = getChild<LLComboBox>("WLPresetsCombo");

	if(comboBox != NULL) {

		std::map<std::string, LLWLParamSet>::iterator mIt = 
			LLWLParamManager::instance()->mParamList.begin();
		for(; mIt != LLWLParamManager::instance()->mParamList.end(); mIt++) 
		{
			comboBox->add(mIt->first);
		}

		// entry for when we're in estate time
		comboBox->add(LLStringUtil::null);

		// set defaults on combo boxes
		comboBox->selectByValue(LLSD("Default"));
	}
	// load it up
	initCallbacks();

	syncMenu();
	
	return TRUE;
}
void LLFloaterWindLight::initCallbacks(void) {

	LLWLParamManager * param_mgr = LLWLParamManager::instance();

	// blue horizon
	getChild<LLUICtrl>("WLBlueHorizonR")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlRMoved, this, _1, &param_mgr->mBlueHorizon));
	getChild<LLUICtrl>("WLBlueHorizonG")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlGMoved, this, _1, &param_mgr->mBlueHorizon));
	getChild<LLUICtrl>("WLBlueHorizonB")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlBMoved, this, _1, &param_mgr->mBlueHorizon));
	getChild<LLUICtrl>("WLBlueHorizonI")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlIMoved, this, _1, &param_mgr->mBlueHorizon));

	// haze density, horizon, mult, and altitude
	getChild<LLUICtrl>("WLHazeDensity")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlRMoved, this, _1, &param_mgr->mHazeDensity));
	getChild<LLUICtrl>("WLHazeHorizon")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlRMoved, this, _1, &param_mgr->mHazeHorizon));
	getChild<LLUICtrl>("WLDensityMult")->setCommitCallback(boost::bind(&LLFloaterWindLight::onFloatControlMoved, this, _1, &param_mgr->mDensityMult));
	getChild<LLUICtrl>("WLMaxAltitude")->setCommitCallback(boost::bind(&LLFloaterWindLight::onFloatControlMoved, this, _1, &param_mgr->mMaxAlt));

	// blue density
	getChild<LLUICtrl>("WLBlueDensityR")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlRMoved, this, _1, &param_mgr->mBlueDensity));
	getChild<LLUICtrl>("WLBlueDensityG")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlGMoved, this, _1, &param_mgr->mBlueDensity));
	getChild<LLUICtrl>("WLBlueDensityB")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlBMoved, this, _1, &param_mgr->mBlueDensity));
	getChild<LLUICtrl>("WLBlueDensityI")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlIMoved, this, _1, &param_mgr->mBlueDensity));

	// Lighting
	
	// sunlight
	getChild<LLUICtrl>("WLSunlightR")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlRMoved, this, _1, &param_mgr->mSunlight));
	getChild<LLUICtrl>("WLSunlightG")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlGMoved, this, _1, &param_mgr->mSunlight));
	getChild<LLUICtrl>("WLSunlightB")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlBMoved, this, _1, &param_mgr->mSunlight));
	getChild<LLUICtrl>("WLSunlightI")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlIMoved, this, _1, &param_mgr->mSunlight));

	// glow
	getChild<LLUICtrl>("WLGlowR")->setCommitCallback(boost::bind(&LLFloaterWindLight::onGlowRMoved, this, _1, &param_mgr->mGlow));
	getChild<LLUICtrl>("WLGlowB")->setCommitCallback(boost::bind(&LLFloaterWindLight::onGlowBMoved, this, _1, &param_mgr->mGlow));

	// ambient
	getChild<LLUICtrl>("WLAmbientR")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlRMoved, this, _1, &param_mgr->mAmbient));
	getChild<LLUICtrl>("WLAmbientG")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlGMoved, this, _1, &param_mgr->mAmbient));
	getChild<LLUICtrl>("WLAmbientB")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlBMoved, this, _1, &param_mgr->mAmbient));
	getChild<LLUICtrl>("WLAmbientI")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlIMoved, this, _1, &param_mgr->mAmbient));

	// time of day
	getChild<LLUICtrl>("WLSunAngle")->setCommitCallback(boost::bind(&LLFloaterWindLight::onSunMoved, this, _1, &param_mgr->mLightnorm));
	getChild<LLUICtrl>("WLEastAngle")->setCommitCallback(boost::bind(&LLFloaterWindLight::onSunMoved, this, _1, &param_mgr->mLightnorm));

	// Clouds

	// Cloud Color
	getChild<LLUICtrl>("WLCloudColorR")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlRMoved, this, _1, &param_mgr->mCloudColor));
	getChild<LLUICtrl>("WLCloudColorG")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlGMoved, this, _1, &param_mgr->mCloudColor));
	getChild<LLUICtrl>("WLCloudColorB")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlBMoved, this, _1, &param_mgr->mCloudColor));
	getChild<LLUICtrl>("WLCloudColorI")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlIMoved, this, _1, &param_mgr->mCloudColor));

	// Cloud
	getChild<LLUICtrl>("WLCloudX")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlRMoved, this, _1, &param_mgr->mCloudMain));
	getChild<LLUICtrl>("WLCloudY")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlGMoved, this, _1, &param_mgr->mCloudMain));
	getChild<LLUICtrl>("WLCloudDensity")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlBMoved, this, _1, &param_mgr->mCloudMain));

	// Cloud Detail
	getChild<LLUICtrl>("WLCloudDetailX")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlRMoved, this, _1, &param_mgr->mCloudDetail));
	getChild<LLUICtrl>("WLCloudDetailY")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlGMoved, this, _1, &param_mgr->mCloudDetail));
	getChild<LLUICtrl>("WLCloudDetailDensity")->setCommitCallback(boost::bind(&LLFloaterWindLight::onColorControlBMoved, this, _1, &param_mgr->mCloudDetail));

	// Cloud extras
	getChild<LLUICtrl>("WLCloudCoverage")->setCommitCallback(boost::bind(&LLFloaterWindLight::onFloatControlMoved, this, _1, &param_mgr->mCloudCoverage));
	getChild<LLUICtrl>("WLCloudScale")->setCommitCallback(boost::bind(&LLFloaterWindLight::onFloatControlMoved, this, _1, &param_mgr->mCloudScale));
	getChild<LLUICtrl>("WLCloudLockX")->setCommitCallback(boost::bind(&LLFloaterWindLight::onCloudScrollXToggled, this, _1));
	getChild<LLUICtrl>("WLCloudLockY")->setCommitCallback(boost::bind(&LLFloaterWindLight::onCloudScrollYToggled, this, _1));
	getChild<LLUICtrl>("WLCloudScrollX")->setCommitCallback(boost::bind(&LLFloaterWindLight::onCloudScrollXMoved, this, _1));
	getChild<LLUICtrl>("WLCloudScrollY")->setCommitCallback(boost::bind(&LLFloaterWindLight::onCloudScrollYMoved, this, _1));
	getChild<LLUICtrl>("WLDistanceMult")->setCommitCallback(boost::bind(&LLFloaterWindLight::onFloatControlMoved, this, _1, &param_mgr->mDistanceMult));
	getChild<LLUICtrl>("DrawClassicClouds")->setCommitCallback(boost::bind(LLSavedSettingsGlue::setBOOL, _1, "SkyUseClassicClouds"));

	// WL Top
	getChild<LLUICtrl>("WLDayCycleMenuButton")->setCommitCallback(boost::bind(&LLFloaterWindLight::onOpenDayCycle, this));
	// Load/save
	LLComboBox* comboBox = getChild<LLComboBox>("WLPresetsCombo");

	//childSetAction("WLLoadPreset", onLoadPreset, comboBox);
	getChild<LLUICtrl>("WLNewPreset")->setCommitCallback(boost::bind(&LLFloaterWindLight::onNewPreset, this));
	getChild<LLUICtrl>("WLSavePreset")->setCommitCallback(boost::bind(&LLFloaterWindLight::onSavePreset, this));
	getChild<LLUICtrl>("WLDeletePreset")->setCommitCallback(boost::bind(&LLFloaterWindLight::onDeletePreset, this));

	comboBox->setCommitCallback(boost::bind(&LLFloaterWindLight::onChangePresetName, this, _1));


	// Dome
	getChild<LLUICtrl>("WLGamma")->setCommitCallback(boost::bind(&LLFloaterWindLight::onFloatControlMoved, this, _1, &param_mgr->mWLGamma));
	getChild<LLUICtrl>("WLStarAlpha")->setCommitCallback(boost::bind(&LLFloaterWindLight::onStarAlphaMoved, this, _1));
}

bool LLFloaterWindLight::newPromptCallback(const LLSD& notification, const LLSD& response)
{
	std::string text = response["message"].asString();
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if(text == "")
	{
		return false;
	}

	if(option == 0) {
		LLComboBox* comboBox = getChild<LLComboBox>("WLPresetsCombo");

		LLFloaterDayCycle* day_cycle = LLFloaterReg::findTypedInstance<LLFloaterDayCycle>("env_day_cycle");
		LLComboBox* keyCombo = NULL;
		if(day_cycle) 
		{
			keyCombo = day_cycle->getChild<LLComboBox>("WLKeyPresets");
		}

		// add the current parameters to the list
		// see if it's there first
		std::map<std::string, LLWLParamSet>::iterator mIt = 
			LLWLParamManager::instance()->mParamList.find(text);

		// if not there, add a new one
		if(mIt == LLWLParamManager::instance()->mParamList.end()) 
		{
			LLWLParamManager::instance()->addParamSet(text, 
				LLWLParamManager::instance()->mCurParams);
			comboBox->add(text);
			comboBox->sortByName();

			// add a blank to the bottom
			comboBox->selectFirstItem();
			if(comboBox->getSimple() == "")
			{
				comboBox->remove(0);
			}
			comboBox->add(LLStringUtil::null);

			comboBox->setSelectedByValue(text, true);
			if(keyCombo) 
			{
				keyCombo->add(text);
				keyCombo->sortByName();
			}
			LLWLParamManager::instance()->savePreset(text);

		// otherwise, send a message to the user
		} 
		else 
		{
			LLNotificationsUtil::add("ExistsSkyPresetAlert");
		}
	}
	return false;
}

void LLFloaterWindLight::syncMenu()
{
	bool err;

	LLWLParamManager * param_mgr = LLWLParamManager::instance();

	LLWLParamSet& currentParams = param_mgr->mCurParams;
	//std::map<std::string, LLVector4> & currentParams = param_mgr->mCurParams.mParamValues;

	// blue horizon
	param_mgr->mBlueHorizon = currentParams.getVector(param_mgr->mBlueHorizon.mName, err);
	childSetValue("WLBlueHorizonR", param_mgr->mBlueHorizon.r / 2.0);
	childSetValue("WLBlueHorizonG", param_mgr->mBlueHorizon.g / 2.0);
	childSetValue("WLBlueHorizonB", param_mgr->mBlueHorizon.b / 2.0);
	childSetValue("WLBlueHorizonI", 
		std::max(param_mgr->mBlueHorizon.r / 2.0, 
			std::max(param_mgr->mBlueHorizon.g / 2.0, 
				param_mgr->mBlueHorizon.b / 2.0)));

	// haze density, horizon, mult, and altitude
	param_mgr->mHazeDensity = currentParams.getVector(param_mgr->mHazeDensity.mName, err);
	childSetValue("WLHazeDensity", param_mgr->mHazeDensity.r);
	param_mgr->mHazeHorizon = currentParams.getVector(param_mgr->mHazeHorizon.mName, err);
	childSetValue("WLHazeHorizon", param_mgr->mHazeHorizon.r);
	param_mgr->mDensityMult = currentParams.getVector(param_mgr->mDensityMult.mName, err);
	childSetValue("WLDensityMult", param_mgr->mDensityMult.x * 
		param_mgr->mDensityMult.mult);
	param_mgr->mMaxAlt = currentParams.getVector(param_mgr->mMaxAlt.mName, err);
	childSetValue("WLMaxAltitude", param_mgr->mMaxAlt.x);

	// blue density
	param_mgr->mBlueDensity = currentParams.getVector(param_mgr->mBlueDensity.mName, err);
	childSetValue("WLBlueDensityR", param_mgr->mBlueDensity.r / 2.0);
	childSetValue("WLBlueDensityG", param_mgr->mBlueDensity.g / 2.0);
	childSetValue("WLBlueDensityB", param_mgr->mBlueDensity.b / 2.0);
	childSetValue("WLBlueDensityI", 
		std::max(param_mgr->mBlueDensity.r / 2.0, 
		std::max(param_mgr->mBlueDensity.g / 2.0, param_mgr->mBlueDensity.b / 2.0)));

	// Lighting
	
	// sunlight
	param_mgr->mSunlight = currentParams.getVector(param_mgr->mSunlight.mName, err);
	childSetValue("WLSunlightR", param_mgr->mSunlight.r / WL_SUN_AMBIENT_SLIDER_SCALE);
	childSetValue("WLSunlightG", param_mgr->mSunlight.g / WL_SUN_AMBIENT_SLIDER_SCALE);
	childSetValue("WLSunlightB", param_mgr->mSunlight.b / WL_SUN_AMBIENT_SLIDER_SCALE);
	childSetValue("WLSunlightI", 
		std::max(param_mgr->mSunlight.r / WL_SUN_AMBIENT_SLIDER_SCALE, 
		std::max(param_mgr->mSunlight.g / WL_SUN_AMBIENT_SLIDER_SCALE, param_mgr->mSunlight.b / WL_SUN_AMBIENT_SLIDER_SCALE)));

	// glow
	param_mgr->mGlow = currentParams.getVector(param_mgr->mGlow.mName, err);
	childSetValue("WLGlowR", 2 - param_mgr->mGlow.r / 20.0f);
	childSetValue("WLGlowB", -param_mgr->mGlow.b / 5.0f);
		
	// ambient
	param_mgr->mAmbient = currentParams.getVector(param_mgr->mAmbient.mName, err);
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
	param_mgr->mCloudColor = currentParams.getVector(param_mgr->mCloudColor.mName, err);
	childSetValue("WLCloudColorR", param_mgr->mCloudColor.r);
	childSetValue("WLCloudColorG", param_mgr->mCloudColor.g);
	childSetValue("WLCloudColorB", param_mgr->mCloudColor.b);
	childSetValue("WLCloudColorI", 
		std::max(param_mgr->mCloudColor.r, 
		std::max(param_mgr->mCloudColor.g, param_mgr->mCloudColor.b)));

	// Cloud
	param_mgr->mCloudMain = currentParams.getVector(param_mgr->mCloudMain.mName, err);
	childSetValue("WLCloudX", param_mgr->mCloudMain.r);
	childSetValue("WLCloudY", param_mgr->mCloudMain.g);
	childSetValue("WLCloudDensity", param_mgr->mCloudMain.b);

	// Cloud Detail
	param_mgr->mCloudDetail = currentParams.getVector(param_mgr->mCloudDetail.mName, err);
	childSetValue("WLCloudDetailX", param_mgr->mCloudDetail.r);
	childSetValue("WLCloudDetailY", param_mgr->mCloudDetail.g);
	childSetValue("WLCloudDetailDensity", param_mgr->mCloudDetail.b);

	// Cloud extras
	param_mgr->mCloudCoverage = currentParams.getVector(param_mgr->mCloudCoverage.mName, err);
	param_mgr->mCloudScale = currentParams.getVector(param_mgr->mCloudScale.mName, err);
	childSetValue("WLCloudCoverage", param_mgr->mCloudCoverage.x);
	childSetValue("WLCloudScale", param_mgr->mCloudScale.x);

	// cloud scrolling
	bool lockX = !param_mgr->mCurParams.getEnableCloudScrollX();
	bool lockY = !param_mgr->mCurParams.getEnableCloudScrollY();
	childSetValue("WLCloudLockX", lockX);
	childSetValue("WLCloudLockY", lockY);
	childSetValue("DrawClassicClouds", gSavedSettings.getBOOL("SkyUseClassicClouds"));
	
	// disable if locked, enable if not
	if(lockX) 
	{
		childDisable("WLCloudScrollX");
	} else {
		childEnable("WLCloudScrollX");
	}
	if(lockY)
	{
		childDisable("WLCloudScrollY");
	} else {
		childEnable("WLCloudScrollY");
	}

	// *HACK cloud scrolling is off my an additive of 10
	childSetValue("WLCloudScrollX", param_mgr->mCurParams.getCloudScrollX() - 10.0f);
	childSetValue("WLCloudScrollY", param_mgr->mCurParams.getCloudScrollY() - 10.0f);

	param_mgr->mDistanceMult = currentParams.getVector(param_mgr->mDistanceMult.mName, err);
	childSetValue("WLDistanceMult", param_mgr->mDistanceMult.x);

	// Tweak extras

	param_mgr->mWLGamma = currentParams.getVector(param_mgr->mWLGamma.mName, err);
	childSetValue("WLGamma", param_mgr->mWLGamma.x);

	childSetValue("WLStarAlpha", param_mgr->mCurParams.getStarBrightness());

	LLTabContainer* tab = getChild<LLTabContainer>("WindLight Tabs");
	LLPanel* panel = getChild<LLPanel>("Scattering");

	tab->enableTabButton(tab->getIndexForPanel(panel), gSavedSettings.getBOOL("RenderDeferredGI"));
}


// color control callbacks
void LLFloaterWindLight::onColorControlRMoved(LLUICtrl* ctrl, WLColorControl* colorControl)
{
	deactivateAnimator();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	colorControl->r = sldrCtrl->getValueF32();
	if(colorControl->isSunOrAmbientColor) {
		colorControl->r *= 3;
	}
	if(colorControl->isBlueHorizonOrDensity) {
		colorControl->r *= 2;
	}	

	// move i if it's the max
	if(colorControl->r >= colorControl->g && colorControl->r >= colorControl->b 
		&& colorControl->hasSliderName) {
		colorControl->i = colorControl->r;
		std::string name = colorControl->mSliderName;
		name.append("I");
		
		if(colorControl->isSunOrAmbientColor) {
			childSetValue(name, colorControl->r / 3);
		} else if(colorControl->isBlueHorizonOrDensity) {
			childSetValue(name, colorControl->r / 2);
		} else {
			childSetValue(name, colorControl->r);
		}
	}

	colorControl->update(LLWLParamManager::instance()->mCurParams);

	LLWLParamManager::instance()->propagateParameters();
}

void LLFloaterWindLight::onColorControlGMoved(LLUICtrl* ctrl, WLColorControl* colorControl)
{
	deactivateAnimator();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	colorControl->g = sldrCtrl->getValueF32();
	if(colorControl->isSunOrAmbientColor) {
		colorControl->g *= 3;
	}
	if(colorControl->isBlueHorizonOrDensity) {
		colorControl->g *= 2;
	}	

	// move i if it's the max
	if(colorControl->g >= colorControl->r && colorControl->g >= colorControl->b
		&& colorControl->hasSliderName) {
		colorControl->i = colorControl->g;
		std::string name = colorControl->mSliderName;
		name.append("I");

		if(colorControl->isSunOrAmbientColor) {
			childSetValue(name, colorControl->g / 3);
		} else if(colorControl->isBlueHorizonOrDensity) {
			childSetValue(name, colorControl->g / 2);
		} else {
			childSetValue(name, colorControl->g);
		}
	}

	colorControl->update(LLWLParamManager::instance()->mCurParams);

	LLWLParamManager::instance()->propagateParameters();
}

void LLFloaterWindLight::onColorControlBMoved(LLUICtrl* ctrl, WLColorControl* colorControl)
{
	deactivateAnimator();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	colorControl->b = sldrCtrl->getValueF32();
	if(colorControl->isSunOrAmbientColor) {
		colorControl->b *= 3;
	}
	if(colorControl->isBlueHorizonOrDensity) {
		colorControl->b *= 2;
	}	

	// move i if it's the max
	if(colorControl->b >= colorControl->r && colorControl->b >= colorControl->g
		&& colorControl->hasSliderName) {
		colorControl->i = colorControl->b;
		std::string name = colorControl->mSliderName;
		name.append("I");

		if(colorControl->isSunOrAmbientColor) {
			childSetValue(name, colorControl->b / 3);
		} else if(colorControl->isBlueHorizonOrDensity) {
			childSetValue(name, colorControl->b / 2);
		} else {
			childSetValue(name, colorControl->b);
		}
	}

	colorControl->update(LLWLParamManager::instance()->mCurParams);

	LLWLParamManager::instance()->propagateParameters();
}

void LLFloaterWindLight::onColorControlIMoved(LLUICtrl* ctrl, WLColorControl* colorControl)
{
	deactivateAnimator();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	colorControl->i = sldrCtrl->getValueF32();
	
	// only for sliders where we pass a name
	if(colorControl->hasSliderName) {
		
		// set it to the top
		F32 maxVal = std::max(std::max(colorControl->r, colorControl->g), colorControl->b);
		F32 iVal;

		if(colorControl->isSunOrAmbientColor)
		{
			iVal = colorControl->i * 3;
		} 
		else if(colorControl->isBlueHorizonOrDensity)
		{
			iVal = colorControl->i * 2;
		} 
		else 
		{
			iVal = colorControl->i;
		}

		// get the names of the other sliders
		std::string rName = colorControl->mSliderName;
		rName.append("R");
		std::string gName = colorControl->mSliderName;
		gName.append("G");
		std::string bName = colorControl->mSliderName;
		bName.append("B");

		// handle if at 0
		if(iVal == 0) {
			colorControl->r = 0;
			colorControl->g = 0;
			colorControl->b = 0;
		
		// if all at the start
		// set them all to the intensity
		} else if (maxVal == 0) {
			colorControl->r = iVal;
			colorControl->g = iVal;
			colorControl->b = iVal;

		} else {

			// add delta amounts to each
			F32 delta = (iVal - maxVal) / maxVal;
			colorControl->r *= (1.0f + delta);
			colorControl->g *= (1.0f + delta);
			colorControl->b *= (1.0f + delta);
		}

		// divide sun color vals by three
		if(colorControl->isSunOrAmbientColor) 
		{
			childSetValue(rName, colorControl->r/3);
			childSetValue(gName, colorControl->g/3);
			childSetValue(bName, colorControl->b/3);	
		
		} 
		else if(colorControl->isBlueHorizonOrDensity) 
		{
			childSetValue(rName, colorControl->r/2);
			childSetValue(gName, colorControl->g/2);
			childSetValue(bName, colorControl->b/2);	
		
		} 
		else 
		{
			// set the sliders to the new vals
			childSetValue(rName, colorControl->r);
			childSetValue(gName, colorControl->g);
			childSetValue(bName, colorControl->b);
		}
	}

	// now update the current parameters and send them to shaders
	colorControl->update(LLWLParamManager::instance()->mCurParams);
	LLWLParamManager::instance()->propagateParameters();
}

/// GLOW SPECIFIC CODE
void LLFloaterWindLight::onGlowRMoved(LLUICtrl* ctrl, WLColorControl* colorControl)
{
	deactivateAnimator();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	// scaled by 20
	colorControl->r = (2 - sldrCtrl->getValueF32()) * 20;

	colorControl->update(LLWLParamManager::instance()->mCurParams);
	LLWLParamManager::instance()->propagateParameters();
}

/// \NOTE that we want NEGATIVE (-) B
void LLFloaterWindLight::onGlowBMoved(LLUICtrl* ctrl, WLColorControl* colorControl)
{
	deactivateAnimator();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	/// \NOTE that we want NEGATIVE (-) B and NOT by 20 as 20 is too big
	colorControl->b = -sldrCtrl->getValueF32() * 5;

	colorControl->update(LLWLParamManager::instance()->mCurParams);
	LLWLParamManager::instance()->propagateParameters();
}

void LLFloaterWindLight::onFloatControlMoved(LLUICtrl* ctrl, WLFloatControl* floatControl)
{
	deactivateAnimator();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	floatControl->x = sldrCtrl->getValueF32() / floatControl->mult;

	floatControl->update(LLWLParamManager::instance()->mCurParams);
	LLWLParamManager::instance()->propagateParameters();
}

// Lighting callbacks

// time of day
void LLFloaterWindLight::onSunMoved(LLUICtrl* ctrl, WLColorControl* colorControl)
{
	deactivateAnimator();

	LLSliderCtrl* sunSldr = getChild<LLSliderCtrl>("WLSunAngle");
	LLSliderCtrl* eastSldr = getChild<LLSliderCtrl>("WLEastAngle");

	// get the two angles
	LLWLParamManager * param_mgr = LLWLParamManager::instance();

	param_mgr->mCurParams.setSunAngle(F_TWO_PI * sunSldr->getValueF32());
	param_mgr->mCurParams.setEastAngle(F_TWO_PI * eastSldr->getValueF32());

	// set the sun vector
	colorControl->r = -sin(param_mgr->mCurParams.getEastAngle()) * 
		cos(param_mgr->mCurParams.getSunAngle());
	colorControl->g = sin(param_mgr->mCurParams.getSunAngle());
	colorControl->b = cos(param_mgr->mCurParams.getEastAngle()) * 
		cos(param_mgr->mCurParams.getSunAngle());
	colorControl->i = 1.f;

	colorControl->update(param_mgr->mCurParams);
	param_mgr->propagateParameters();
}

void LLFloaterWindLight::onStarAlphaMoved(LLUICtrl* ctrl)
{
	deactivateAnimator();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	LLWLParamManager::instance()->mCurParams.setStarBrightness(sldrCtrl->getValueF32());
}

void LLFloaterWindLight::onNewPreset()
{
	LLNotificationsUtil::add("NewSkyPreset", LLSD(), LLSD(), boost::bind(&LLFloaterWindLight::newPromptCallback, this, _1, _2));
}

void LLFloaterWindLight::onSavePreset()
{
	// get the name
	LLComboBox* comboBox = getChild<LLComboBox>( 
		"WLPresetsCombo");

	// don't save the empty name
	if(comboBox->getSelectedItemLabel() == "")
	{
		return;
	}

	// check to see if it's a default and shouldn't be overwritten
	std::set<std::string>::iterator sIt = sDefaultPresets.find(
		comboBox->getSelectedItemLabel());
	if(sIt != sDefaultPresets.end() && !gSavedSettings.getBOOL("SkyEditPresets")) 
	{
		LLNotificationsUtil::add("WLNoEditDefault");
		return;
	}

	LLWLParamManager::instance()->mCurParams.mName = 
		comboBox->getSelectedItemLabel();

	LLNotificationsUtil::add("WLSavePresetAlert", LLSD(), LLSD(), boost::bind(&LLFloaterWindLight::saveAlertCallback, this, _1, _2));
}

bool LLFloaterWindLight::saveAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	// if they choose save, do it.  Otherwise, don't do anything
	if(option == 0) 
	{
		LLWLParamManager * param_mgr = LLWLParamManager::instance();

		param_mgr->setParamSet(param_mgr->mCurParams.mName, param_mgr->mCurParams);
		
		// comment this back in to save to file
		param_mgr->savePreset(param_mgr->mCurParams.mName);
	}
	return false;
}

void LLFloaterWindLight::onDeletePreset()
{
	LLComboBox* combo_box = getChild<LLComboBox>( 
		"WLPresetsCombo");

	if(combo_box->getSelectedValue().asString() == "")
	{
		return;
	}

	LLSD args;
	args["SKY"] = combo_box->getSelectedValue().asString();
	LLNotificationsUtil::add("WLDeletePresetAlert", args, LLSD(), 
									boost::bind(&LLFloaterWindLight::deleteAlertCallback, this, _1, _2));
}

bool LLFloaterWindLight::deleteAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	// if they choose delete, do it.  Otherwise, don't do anything
	if(option == 0) 
	{
		LLComboBox* combo_box = getChild<LLComboBox>("WLPresetsCombo");
		LLFloaterDayCycle* day_cycle = LLFloaterReg::findTypedInstance<LLFloaterDayCycle>("env_day_cycle");
		LLComboBox* key_combo = NULL;

		if (day_cycle) 
		{
			key_combo = day_cycle->getChild<LLComboBox>("WLKeyPresets");
		}

		std::string name(combo_box->getSelectedValue().asString());

		// check to see if it's a default and shouldn't be deleted
		std::set<std::string>::iterator sIt = sDefaultPresets.find(name);
		if(sIt != sDefaultPresets.end()) 
		{
			LLNotificationsUtil::add("WLNoEditDefault");
			return false;
		}

		LLWLParamManager::instance()->removeParamSet(name, true);
		
		// remove and choose another
		S32 new_index = combo_box->getCurrentIndex();

		combo_box->remove(name);
		if(key_combo != NULL) 
		{
			key_combo->remove(name);

			// remove from slider, as well
			day_cycle->deletePreset(name);
		}

		// pick the previously selected index after delete
		if(new_index > 0) 
		{
			new_index--;
		}
		
		if(combo_box->getItemCount() > 0) 
		{
			combo_box->setCurrentByIndex(new_index);
		}
	}
	return false;
}


void LLFloaterWindLight::onChangePresetName(LLUICtrl* ctrl)
{
	deactivateAnimator();

	std::string data = ctrl->getValue().asString();
	if(!data.empty())
	{
		LLWLParamManager::instance()->loadPreset( data);
		syncMenu();
	}
}

void LLFloaterWindLight::onOpenDayCycle()
{
	LLFloaterReg::showInstance("env_day_cycle");
}

// Clouds
void LLFloaterWindLight::onCloudScrollXMoved(LLUICtrl* ctrl)
{
	deactivateAnimator();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	// *HACK  all cloud scrolling is off by an additive of 10. 
	LLWLParamManager::instance()->mCurParams.setCloudScrollX(sldrCtrl->getValueF32() + 10.0f);
}

void LLFloaterWindLight::onCloudScrollYMoved(LLUICtrl* ctrl)
{
	deactivateAnimator();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	// *HACK  all cloud scrolling is off by an additive of 10. 
	LLWLParamManager::instance()->mCurParams.setCloudScrollY(sldrCtrl->getValueF32() + 10.0f);
}

void LLFloaterWindLight::onCloudScrollXToggled(LLUICtrl* ctrl)
{
	deactivateAnimator();

	LLCheckBoxCtrl* cbCtrl = static_cast<LLCheckBoxCtrl*>(ctrl);

	bool lock = cbCtrl->get();
	LLWLParamManager::instance()->mCurParams.setEnableCloudScrollX(!lock);

	LLSliderCtrl* sldr = getChild<LLSliderCtrl>( 
		"WLCloudScrollX");

	if(cbCtrl->get()) 
	{
		sldr->setEnabled(false);
	} 
	else 
	{
		sldr->setEnabled(true);
	}

}

void LLFloaterWindLight::onCloudScrollYToggled(LLUICtrl* ctrl)
{
	deactivateAnimator();

	LLCheckBoxCtrl* cbCtrl = static_cast<LLCheckBoxCtrl*>(ctrl);
	bool lock = cbCtrl->get();
	LLWLParamManager::instance()->mCurParams.setEnableCloudScrollY(!lock);

	LLSliderCtrl* sldr = getChild<LLSliderCtrl>( 
		"WLCloudScrollY");

	if(cbCtrl->get()) 
	{
		sldr->setEnabled(false);
	} 
	else 
	{
		sldr->setEnabled(true);
	}
}

void LLFloaterWindLight::deactivateAnimator()
{
	LLWLParamManager::instance()->mAnimator.mIsRunning = false;
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;
}
