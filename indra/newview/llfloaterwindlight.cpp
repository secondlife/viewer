/** 
 * @file llfloaterwindlight.cpp
 * @brief LLFloaterWindLight class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llfloaterwindlight.h"

#include "pipeline.h"
#include "llsky.h"

#include "llfloaterreg.h"
#include "llsliderctrl.h"
#include "llmultislider.h"
#include "llmultisliderctrl.h"
#include "llspinctrl.h"
#include "llcheckboxctrl.h"
#include "lluictrlfactory.h"
#include "llviewercamera.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llfloaterdaycycle.h"
#include "llboost.h"
#include "llnotifications.h"

#include "v4math.h"
#include "llviewerdisplay.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llsavedsettingsglue.h"

#include "llwlparamset.h"
#include "llwlparammanager.h"
#include "llpostprocess.h"

#undef max

LLFloaterWindLight* LLFloaterWindLight::sWindLight = NULL;
std::set<LLWLParamKey> LLFloaterWindLight::sDefaultPresets;
LLEnvKey::EScope LLFloaterWindLight::sScope;
std::string LLFloaterWindLight::sOriginalTitle;

static const F32 WL_SUN_AMBIENT_SLIDER_SCALE = 3.0f;

LLFloaterWindLight::LLFloaterWindLight(const LLSD &key) : LLFloater(key)
{
}

BOOL LLFloaterWindLight::postBuild()
{
	sWindLight = this;
	sOriginalTitle = getTitle();
	
	// add the combo boxes
	LLComboBox* comboBox = getChild<LLComboBox>("WLPresetsCombo");

	if(comboBox != NULL) {
		
		std::map<LLWLParamKey, LLWLParamSet>::iterator mIt = 
			LLWLParamManager::getInstance()->mParamList.begin();
		for(; mIt != LLWLParamManager::getInstance()->mParamList.end(); mIt++) 
		{
			comboBox->add(mIt->first.toString(), mIt->first.toLLSD());
		}

		// entry for when we're in estate time
		comboBox->add(LLStringUtil::null);

		// set defaults on combo boxes
		comboBox->selectByValue(LLSD("Default"));
	}

	// add the list of presets
	std::string def_days = getString("WLDefaultSkyNames");

	// no editing or deleting of the blank string
	LLWLParamKey blank("", LLEnvKey::SCOPE_LOCAL);
	sDefaultPresets.insert(blank);
	boost_tokenizer tokens(def_days, boost::char_separator<char>(":"));
	for (boost_tokenizer::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		std::string tok(*token_iter);
		LLWLParamKey key(tok, LLEnvKey::SCOPE_LOCAL);
		sDefaultPresets.insert(key);
	}

	// load it up
	initCallbacks();

	return TRUE;
}

LLFloaterWindLight::~LLFloaterWindLight()
{
}

void LLFloaterWindLight::initCallbacks(void) 
{
	LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

	// blue horizon
	childSetCommitCallback("WLBlueHorizonR", onColorControlRMoved, &param_mgr->mBlueHorizon);
	childSetCommitCallback("WLBlueHorizonG", onColorControlGMoved, &param_mgr->mBlueHorizon);
	childSetCommitCallback("WLBlueHorizonB", onColorControlBMoved, &param_mgr->mBlueHorizon);
	childSetCommitCallback("WLBlueHorizonI", onColorControlIMoved, &param_mgr->mBlueHorizon);

	// haze density, horizon, mult, and altitude
	childSetCommitCallback("WLHazeDensity", onColorControlRMoved, &param_mgr->mHazeDensity);
	childSetCommitCallback("WLHazeHorizon", onColorControlRMoved, &param_mgr->mHazeHorizon);
	childSetCommitCallback("WLDensityMult", onFloatControlMoved, &param_mgr->mDensityMult);
	childSetCommitCallback("WLMaxAltitude", onFloatControlMoved, &param_mgr->mMaxAlt);

	// blue density
	childSetCommitCallback("WLBlueDensityR", onColorControlRMoved, &param_mgr->mBlueDensity);
	childSetCommitCallback("WLBlueDensityG", onColorControlGMoved, &param_mgr->mBlueDensity);
	childSetCommitCallback("WLBlueDensityB", onColorControlBMoved, &param_mgr->mBlueDensity);
	childSetCommitCallback("WLBlueDensityI", onColorControlIMoved, &param_mgr->mBlueDensity);

	// Lighting
	
	// sunlight
	childSetCommitCallback("WLSunlightR", onColorControlRMoved, &param_mgr->mSunlight);
	childSetCommitCallback("WLSunlightG", onColorControlGMoved, &param_mgr->mSunlight);
	childSetCommitCallback("WLSunlightB", onColorControlBMoved, &param_mgr->mSunlight);
	childSetCommitCallback("WLSunlightI", onColorControlIMoved, &param_mgr->mSunlight);

	// glow
	childSetCommitCallback("WLGlowR", onGlowRMoved, &param_mgr->mGlow);
	childSetCommitCallback("WLGlowB", onGlowBMoved, &param_mgr->mGlow);

	// ambient
	childSetCommitCallback("WLAmbientR", onColorControlRMoved, &param_mgr->mAmbient);
	childSetCommitCallback("WLAmbientG", onColorControlGMoved, &param_mgr->mAmbient);
	childSetCommitCallback("WLAmbientB", onColorControlBMoved, &param_mgr->mAmbient);
	childSetCommitCallback("WLAmbientI", onColorControlIMoved, &param_mgr->mAmbient);

	// time of day
	childSetCommitCallback("WLSunAngle", onSunMoved, &param_mgr->mLightnorm);
	childSetCommitCallback("WLEastAngle", onSunMoved, &param_mgr->mLightnorm);

	// Clouds

	// Cloud Color
	childSetCommitCallback("WLCloudColorR", onColorControlRMoved, &param_mgr->mCloudColor);
	childSetCommitCallback("WLCloudColorG", onColorControlGMoved, &param_mgr->mCloudColor);
	childSetCommitCallback("WLCloudColorB", onColorControlBMoved, &param_mgr->mCloudColor);
	childSetCommitCallback("WLCloudColorI", onColorControlIMoved, &param_mgr->mCloudColor);

	// Cloud
	childSetCommitCallback("WLCloudX", onColorControlRMoved, &param_mgr->mCloudMain);
	childSetCommitCallback("WLCloudY", onColorControlGMoved, &param_mgr->mCloudMain);
	childSetCommitCallback("WLCloudDensity", onColorControlBMoved, &param_mgr->mCloudMain);

	// Cloud Detail
	childSetCommitCallback("WLCloudDetailX", onColorControlRMoved, &param_mgr->mCloudDetail);
	childSetCommitCallback("WLCloudDetailY", onColorControlGMoved, &param_mgr->mCloudDetail);
	childSetCommitCallback("WLCloudDetailDensity", onColorControlBMoved, &param_mgr->mCloudDetail);

	// Cloud extras
	static std::string use_classic_clouds = "SkyUseClassicClouds";

	childSetCommitCallback("WLCloudCoverage", onFloatControlMoved, &param_mgr->mCloudCoverage);
	childSetCommitCallback("WLCloudScale", onFloatControlMoved, &param_mgr->mCloudScale);
	childSetCommitCallback("WLCloudLockX", onCloudScrollXToggled, NULL);
	childSetCommitCallback("WLCloudLockY", onCloudScrollYToggled, NULL);
	childSetCommitCallback("WLCloudScrollX", onCloudScrollXMoved, NULL);
	childSetCommitCallback("WLCloudScrollY", onCloudScrollYMoved, NULL);
	childSetCommitCallback("WLDistanceMult", onFloatControlMoved, &param_mgr->mDistanceMult);
	getChild<LLUICtrl>("DrawClassicClouds")->setCommitCallback(boost::bind(LLSavedSettingsGlue::setBOOL, _1, "SkyUseClassicClouds"));

	// WL Top
	childSetAction("WLDayCycleMenuButton", onOpenDayCycle, NULL);
	// Load/save
	LLComboBox* comboBox = getChild<LLComboBox>("WLPresetsCombo");

	//childSetAction("WLLoadPreset", onLoadPreset, comboBox);
	childSetAction("WLNewPreset", onNewPreset, comboBox);
	childSetAction("WLSavePreset", onSavePreset, comboBox);
	childSetAction("WLDeletePreset", onDeletePreset, comboBox);

	comboBox->setCommitCallback(boost::bind(&LLFloaterWindLight::onChangePresetName, _1));

	// Dome
	childSetCommitCallback("WLGamma", onFloatControlMoved, &param_mgr->mWLGamma);
	childSetCommitCallback("WLStarAlpha", onStarAlphaMoved, NULL);
}

bool LLFloaterWindLight::newPromptCallback(const LLSD& notification, const LLSD& response)
{
	llassert(sWindLight);
	std::string text = response["message"].asString();
	LLWLParamKey newKey(text, LLEnvKey::SCOPE_LOCAL);
	S32 option = LLNotification::getSelectedOption(notification, response);

	if(text == "")
	{
		return false;
	}

	if(option == 0) {
		LLComboBox* comboBox = sWindLight->getChild<LLComboBox>( 
			"WLPresetsCombo");

		LLFloaterDayCycle* sDayCycle = NULL;
		LLComboBox* keyCombo = NULL;
		if(LLFloaterDayCycle::isOpen()) 
		{
			sDayCycle = LLFloaterDayCycle::instance();
			keyCombo = sDayCycle->getChild<LLComboBox>( 
				"WLKeyPresets");
		}

		// add the current parameters to the list
		// see if it's there first
		std::map<LLWLParamKey, LLWLParamSet>::iterator mIt = 
			LLWLParamManager::getInstance()->mParamList.find(newKey);

		// if not there, add a new one
		if(mIt == LLWLParamManager::getInstance()->mParamList.end()) 
		{
			LLWLParamManager::getInstance()->addParamSet(newKey, 
				LLWLParamManager::getInstance()->mCurParams);
			comboBox->add(newKey.toString(), newKey.toLLSD());
			comboBox->sortByName();

			// add a blank to the bottom
			comboBox->selectFirstItem();
			if(comboBox->getSimple() == "")
			{
				comboBox->remove(0);
			}
			comboBox->add(LLStringUtil::null);

			comboBox->setSelectedByValue(text, true);
			if(LLFloaterDayCycle::isOpen()) 
			{
				keyCombo->add(text);
				keyCombo->sortByName();
			}
			LLWLParamManager::getInstance()->savePreset(newKey);

		// otherwise, send a message to the user
		} 
		else 
		{
			LLNotifications::instance().add("ExistsSkyPresetAlert", LLSD(), LLSD());
		}
	}
	return false;
}

void LLFloaterWindLight::syncMenu()
{
	bool err;

	LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

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
}


// static
LLFloaterWindLight* LLFloaterWindLight::instance()
{
	if (!sWindLight)
	{
		lldebugs << "Creating WL floater" << llendl;
		sWindLight = LLFloaterReg::getTypedInstance<LLFloaterWindLight>("env_windlight");
		llassert(sWindLight);
	}
	return sWindLight;
}

// static
void LLFloaterWindLight::show(LLEnvKey::EScope scope)
{
	LLFloaterWindLight* windLight = instance();
	if(scope != sScope && ((LLView*)windLight)->getVisible())
	{
		LLNotifications::instance().add("EnvOtherScopeAlreadyOpen", LLSD(), LLSD());
		return;
	}
	sScope = scope;
	std::string scope_str = "";
	switch(sScope)
	{
		case LLEnvKey::SCOPE_LOCAL:
			scope_str = LLTrans::getString("LocalSettings");
			break;
		case LLEnvKey::SCOPE_REGION:
			scope_str = LLTrans::getString("RegionSettings");
			break;
	}
	std::string title = sOriginalTitle + " (" + scope_str + ")";
	windLight->setTitle(title);
	windLight->syncMenu();

	LLEnvManager::instance().startEditingScope(scope);

	// comment in if you want the menu to rebuild each time
	//LLUICtrlFactory::getInstance()->buildFloater(windLight, "floater_windlight_options.xml");
	//windLight->initCallbacks();

	windLight->openFloater();
}

bool LLFloaterWindLight::isOpen()
{
	return LLFloater::isShown(sWindLight);
}

// virtual
void LLFloaterWindLight::onClose(bool app_quitting)
{
	lldebugs << "Destroying WL floater" << llendl;
	sWindLight = NULL;
}

// color control callbacks
void LLFloaterWindLight::onColorControlRMoved(LLUICtrl* ctrl, void* userData)
{
	llassert(sWindLight);
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl * colorControl = static_cast<WLColorControl *>(userData);

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
			sWindLight->childSetValue(name, colorControl->r / 3);
		} else if(colorControl->isBlueHorizonOrDensity) {
			sWindLight->childSetValue(name, colorControl->r / 2);
		} else {
			sWindLight->childSetValue(name, colorControl->r);
		}
	}

	colorControl->update(LLWLParamManager::getInstance()->mCurParams);

	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onColorControlGMoved(LLUICtrl* ctrl, void* userData)
{
	llassert(sWindLight);
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl * colorControl = static_cast<WLColorControl *>(userData);

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
			sWindLight->childSetValue(name, colorControl->g / 3);
		} else if(colorControl->isBlueHorizonOrDensity) {
			sWindLight->childSetValue(name, colorControl->g / 2);
		} else {
			sWindLight->childSetValue(name, colorControl->g);
		}
	}

	colorControl->update(LLWLParamManager::getInstance()->mCurParams);

	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onColorControlBMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl * colorControl = static_cast<WLColorControl *>(userData);

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
			sWindLight->childSetValue(name, colorControl->b / 3);
		} else if(colorControl->isBlueHorizonOrDensity) {
			sWindLight->childSetValue(name, colorControl->b / 2);
		} else {
			sWindLight->childSetValue(name, colorControl->b);
		}
	}

	colorControl->update(LLWLParamManager::getInstance()->mCurParams);

	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onColorControlIMoved(LLUICtrl* ctrl, void* userData)
{
	llassert(sWindLight);
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl * colorControl = static_cast<WLColorControl *>(userData);

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
			sWindLight->childSetValue(rName, colorControl->r/3);
			sWindLight->childSetValue(gName, colorControl->g/3);
			sWindLight->childSetValue(bName, colorControl->b/3);	
		
		} 
		else if(colorControl->isBlueHorizonOrDensity) 
		{
			sWindLight->childSetValue(rName, colorControl->r/2);
			sWindLight->childSetValue(gName, colorControl->g/2);
			sWindLight->childSetValue(bName, colorControl->b/2);	
		
		} 
		else 
		{
			// set the sliders to the new vals
			sWindLight->childSetValue(rName, colorControl->r);
			sWindLight->childSetValue(gName, colorControl->g);
			sWindLight->childSetValue(bName, colorControl->b);
		}
	}

	// now update the current parameters and send them to shaders
	colorControl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

/// GLOW SPECIFIC CODE
void LLFloaterWindLight::onGlowRMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl * colorControl = static_cast<WLColorControl *>(userData);

	// scaled by 20
	colorControl->r = (2 - sldrCtrl->getValueF32()) * 20;

	colorControl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

/// \NOTE that we want NEGATIVE (-) B
void LLFloaterWindLight::onGlowBMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl * colorControl = static_cast<WLColorControl *>(userData);

	/// \NOTE that we want NEGATIVE (-) B and NOT by 20 as 20 is too big
	colorControl->b = -sldrCtrl->getValueF32() * 5;

	colorControl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onFloatControlMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	WLFloatControl * floatControl = static_cast<WLFloatControl *>(userData);

	floatControl->x = sldrCtrl->getValueF32() / floatControl->mult;

	floatControl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onBoolToggle(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLCheckBoxCtrl* cbCtrl = static_cast<LLCheckBoxCtrl*>(ctrl);

	bool value = cbCtrl->get();
	(*(static_cast<BOOL *>(userData))) = value;
}


// Lighting callbacks

// time of day
void LLFloaterWindLight::onSunMoved(LLUICtrl* ctrl, void* userData)
{
	llassert(sWindLight);
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sunSldr = sWindLight->getChild<LLSliderCtrl>("WLSunAngle");
	LLSliderCtrl* eastSldr = sWindLight->getChild<LLSliderCtrl>("WLEastAngle");

	WLColorControl * colorControl = static_cast<WLColorControl *>(userData);
	
	// get the two angles
	LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

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

void LLFloaterWindLight::onFloatTweakMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	F32 * tweak = static_cast<F32 *>(userData);

	(*tweak) = sldrCtrl->getValueF32();
	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onStarAlphaMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	LLWLParamManager::getInstance()->mCurParams.setStarBrightness(sldrCtrl->getValueF32());
}

void LLFloaterWindLight::onNewPreset(void* userData)
{
	LLNotifications::instance().add("NewSkyPreset", LLSD(), LLSD(), newPromptCallback);
}

void LLFloaterWindLight::onSavePreset(void* userData)
{
	llassert(sWindLight);
	// get the name
	LLComboBox* comboBox = sWindLight->getChild<LLComboBox>( 
		"WLPresetsCombo");

	// don't save the empty name
	if(comboBox->getSelectedItemLabel() == "")
	{
		return;
	}

	// If region scope, save immediately.
	// We don't actually save to file in this case, but just update the preset
	// so that the changes can be uploaded to server.
	LLWLParamKey key(comboBox->getSelectedValue());
	if (key.scope == LLEnvKey::SCOPE_REGION)
	{
		// *TODO: Eliminate code duplication.
		LL_DEBUGS("Windlight") << "Saving region sky preset: " << key.name  << llendl;
		LLWLParamManager * param_mgr = LLWLParamManager::getInstance();
		param_mgr->mCurParams.mName = key.name;
		param_mgr->setParamSet(key, param_mgr->mCurParams);
		return;
	}

	// check to see if it's a default and shouldn't be overwritten
	std::set<LLWLParamKey>::iterator sIt = sDefaultPresets.find(LLWLParamKey(key));
	if(sIt != sDefaultPresets.end() && !gSavedSettings.getBOOL("SkyEditPresets")) 
	{
		LLNotifications::instance().add("WLNoEditDefault", LLSD(), LLSD());
		return;
	}

	LLWLParamManager::getInstance()->mCurParams.mName = 
		comboBox->getSelectedItemLabel();

	LLNotifications::instance().add("WLSavePresetAlert", LLSD(), LLSD(), saveAlertCallback);
}

bool LLFloaterWindLight::saveAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	// if they choose save, do it.  Otherwise, don't do anything
	if(option == 0) 
	{
		LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

		// we should only "save" local presets; those with other scopes should be "save as"
		LLWLParamKey key(param_mgr->mCurParams.mName, LLEnvKey::SCOPE_LOCAL);

		param_mgr->setParamSet(key, param_mgr->mCurParams);
		
		// comment this back in to save to file
		param_mgr->savePreset(key);
	}
	return false;
}

void LLFloaterWindLight::onDeletePreset(void* userData)
{
	llassert(sWindLight);
	LLComboBox* combo_box = sWindLight->getChild<LLComboBox>( 
		"WLPresetsCombo");

	if(combo_box->getSelectedValue().isUndefined())
	{
		return;
	}

	LLSD args;
	args["SKY"] = combo_box->getSelectedValue()[0].asString();
	LLNotifications::instance().add("WLDeletePresetAlert", args, LLSD(), 
									boost::bind(&LLFloaterWindLight::deleteAlertCallback, sWindLight, _1, _2));
}

bool LLFloaterWindLight::deleteAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	// if they choose delete, do it.  Otherwise, don't do anything
	if(option == 0) 
	{
		LLComboBox* combo_box = getChild<LLComboBox>( 
			"WLPresetsCombo");
		LLFloaterDayCycle* day_cycle = NULL;
		LLComboBox* key_combo = NULL;
		LLMultiSliderCtrl* mult_sldr = NULL;

		if(LLFloaterDayCycle::isOpen()) 
		{
			day_cycle = LLFloaterDayCycle::instance();
			key_combo = day_cycle->getChild<LLComboBox>( 
				"WLKeyPresets");
			mult_sldr = day_cycle->getChild<LLMultiSliderCtrl>("WLDayCycleKeys");
		}

		LLWLParamKey key(combo_box->getSelectedValue());

		// check to see if it's a default and shouldn't be deleted
		std::set<LLWLParamKey>::iterator sIt = sDefaultPresets.find(key);
		if(sIt != sDefaultPresets.end()) 
		{
			LLNotifications::instance().add("WLNoEditDefault", LLSD(), LLSD());
			return false;
		}

		LLWLParamManager::getInstance()->removeParamSet(key, true);
		
		// remove and choose another
		S32 new_index = combo_box->getCurrentIndex();

		combo_box->remove(key.toString());
		if(key_combo != NULL) 
		{
			key_combo->remove(key.toString());

			// remove from slider, as well
			day_cycle->deletePreset(key);
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
	llassert(sWindLight);
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLComboBox * combo_box = static_cast<LLComboBox*>(ctrl);
	
	if(combo_box->getSimple() == "")
	{
		return;
	}
	
	LLWLParamManager::getInstance()->loadPreset(LLWLParamKey(combo_box->getSelectedValue()));
	sWindLight->syncMenu();
}

void LLFloaterWindLight::onOpenDayCycle(void* userData)
{
	LLFloaterDayCycle::show(sScope);
}

// Clouds
void LLFloaterWindLight::onCloudScrollXMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	// *HACK  all cloud scrolling is off by an additive of 10. 
	LLWLParamManager::getInstance()->mCurParams.setCloudScrollX(sldrCtrl->getValueF32() + 10.0f);
}

void LLFloaterWindLight::onCloudScrollYMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	// *HACK  all cloud scrolling is off by an additive of 10. 
	LLWLParamManager::getInstance()->mCurParams.setCloudScrollY(sldrCtrl->getValueF32() + 10.0f);
}

void LLFloaterWindLight::onCloudScrollXToggled(LLUICtrl* ctrl, void* userData)
{
	llassert(sWindLight);
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLCheckBoxCtrl* cbCtrl = static_cast<LLCheckBoxCtrl*>(ctrl);

	bool lock = cbCtrl->get();
	LLWLParamManager::getInstance()->mCurParams.setEnableCloudScrollX(!lock);

	LLSliderCtrl* sldr = sWindLight->getChild<LLSliderCtrl>( 
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

void LLFloaterWindLight::onCloudScrollYToggled(LLUICtrl* ctrl, void* userData)
{
	llassert(sWindLight);
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLCheckBoxCtrl* cbCtrl = static_cast<LLCheckBoxCtrl*>(ctrl);
	bool lock = cbCtrl->get();
	LLWLParamManager::getInstance()->mCurParams.setEnableCloudScrollY(!lock);

	LLSliderCtrl* sldr = sWindLight->getChild<LLSliderCtrl>( 
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
