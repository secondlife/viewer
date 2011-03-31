/** 
 * @file llfloaterenvsettings.cpp
 * @brief LLFloaterEnvSettings class definition
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

#include "llfloaterenvsettings.h"

#include "llfloaterreg.h"
#include "llfloaterwindlight.h"
#include "llfloaterwater.h"
#include "llfloaterdaycycle.h"
#include "llfloaterregioninfo.h"
#include "lluictrlfactory.h"
#include "llsliderctrl.h"
#include "llcombobox.h"
#include "llcolorswatch.h"
#include "llwlanimator.h"
#include "llnotifications.h"

#include "llwlparamset.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
#include "llmath.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"

#include "llcheckboxctrl.h"

#include "pipeline.h"

#include <sstream>

// LLFloaterEnvSettings* LLFloaterEnvSettings::sEnvSettings = NULL;

LLFloaterEnvSettings::LLFloaterEnvSettings(const LLSD &key) : LLFloater(key)
{	
	lldebugs << "Creating env settings floater" << llendl;
}

LLFloaterEnvSettings::~LLFloaterEnvSettings()
{
}

// virtual
BOOL LLFloaterEnvSettings::postBuild()
{	
	// load it up
	initCallbacks();
	syncMenu();

	LLEnvKey::EScope cur_scope = LLEnvManager::getInstance()->getNormallyDisplayedScope();
	childSetValue("RegionWLOptIn", cur_scope == LLEnvKey::SCOPE_REGION);

	return TRUE;
}

void LLFloaterEnvSettings::onUseRegionEnvironment(LLUICtrl* ctrl, void* data)
{
	//LLFloaterEnvSettings* self = (LLFloaterEnvSettings*)data;
	LLCheckBoxCtrl* checkbox = static_cast<LLCheckBoxCtrl*>(ctrl); //self->getChildView("RegionWLOptIn");
	setOptIn(checkbox->getValue().asBoolean());
}

void LLFloaterEnvSettings::setOptIn(bool opt_in)
{
	if (opt_in)
	{
		LLEnvManager::getInstance()->setNormallyDisplayedScope(LLEnvKey::SCOPE_REGION);
	}
	else
	{
		LLEnvManager::getInstance()->setNormallyDisplayedScope(LLEnvKey::SCOPE_LOCAL);
	}
}

void LLFloaterEnvSettings::initCallbacks(void) 
{
	// our three sliders
	childSetCommitCallback("EnvTimeSlider", &LLFloaterEnvSettings::onChangeDayTime, NULL);
	childSetCommitCallback("EnvCloudSlider", &LLFloaterEnvSettings::onChangeCloudCoverage, NULL);
	childSetCommitCallback("EnvWaterFogSlider", &LLFloaterEnvSettings::onChangeWaterFogDensity, &LLWaterParamManager::instance().mFogDensity);	

	// color picker
	childSetCommitCallback("EnvWaterColor", &LLFloaterEnvSettings::onChangeWaterColor, &LLWaterParamManager::instance().mFogColor);

	// WL Top
	childSetCommitCallback("EnvAdvancedSkyButton", &LLFloaterEnvSettings::onOpenAdvancedSky, NULL);
 	childSetCommitCallback("EnvAdvancedWaterButton", &LLFloaterEnvSettings::onOpenAdvancedWater, NULL);
	childSetCommitCallback("EnvUseEstateTimeButton", &LLFloaterEnvSettings::onUseEstateTime, NULL);

	childSetCommitCallback("RegionWLOptIn", &LLFloaterEnvSettings::onUseRegionEnvironment, NULL);
}


// menu maintenance functions

void LLFloaterEnvSettings::syncMenu()
{
	LLSliderCtrl* sldr;
	sldr = getChild<LLSliderCtrl>("EnvTimeSlider");

	// sync the clock
	F32 val = (F32)LLWLParamManager::getInstance()->mAnimator.getDayTime();
	std::string timeStr = LLWLAnimator::timeToString(val);

	LLTextBox* textBox;
	textBox = getChild<LLTextBox>("EnvTimeText");

	textBox->setValue(timeStr);
	
	// sync time slider which starts at 6 AM	
	val -= 0.25;
	if(val < 0) 
	{
		val++;
	}
	sldr->setValue(val);

	// sync cloud coverage
	bool err;
	childSetValue("EnvCloudSlider", LLWLParamManager::getInstance()->mCurParams.getFloat("cloud_shadow", err));

	LLWaterParamManager * param_mgr = LLWaterParamManager::getInstance();
	// sync water params
	LLColor4 col = param_mgr->getFogColor();
	LLColorSwatchCtrl* colCtrl = getChild<LLColorSwatchCtrl>("EnvWaterColor");
	col.mV[3] = 1.0f;
	colCtrl->set(col);

	childSetValue("EnvWaterFogSlider", param_mgr->mFogDensity.mExp);
	param_mgr->setDensitySliderValue(param_mgr->mFogDensity.mExp);

	// turn off Use Estate/Local Time buttons if already being used

	if(LLWLParamManager::getInstance()->mAnimator.getUseLindenTime())
	{
		childDisable("EnvUseEstateTimeButton");
	}
	else
	{
		childEnable("EnvUseEstateTimeButton");
	}

	if(LLWLParamManager::getInstance()->mAnimator.getUseLocalTime())
	{
		childDisable("EnvUseLocalTimeButton");
	}
	else
	{
		childEnable("EnvUseLocalTimeButton");
	}

	if(!gPipeline.canUseVertexShaders())
	{
		childDisable("EnvWaterColor");
		childDisable("EnvWaterColorText");
		//childDisable("EnvAdvancedWaterButton");		
	}
	else
	{
		childEnable("EnvWaterColor");
		childEnable("EnvWaterColorText");
		//childEnable("EnvAdvancedWaterButton");		
	}

	// only allow access to these if they are using windlight
	if(!gPipeline.canUseWindLightShaders())
	{

		childDisable("EnvCloudSlider");
		childDisable("EnvCloudText");
		//childDisable("EnvAdvancedSkyButton");
	}
	else
	{
		childEnable("EnvCloudSlider");
		childEnable("EnvCloudText");
		//childEnable("EnvAdvancedSkyButton");
	}
}

void LLFloaterEnvSettings::onChangeDayTime(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldr = static_cast<LLSliderCtrl*>(ctrl);

	// deactivate animator
	LLWLParamManager::instance().mAnimator.deactivate();

	F32 val = sldr->getValueF32() + 0.25f;
	if(val > 1.0) 
	{
		val--;
	}

	LLWLParamManager::instance().mAnimator.setDayTime((F64)val);
	LLWLParamManager::instance().mAnimator.update(
		LLWLParamManager::instance().mCurParams);
}

void LLFloaterEnvSettings::onChangeCloudCoverage(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldr = static_cast<LLSliderCtrl*>(ctrl);
	
	// deactivate animator
	//LLWLParamManager::instance()->mAnimator.mIsRunning = false;
	//LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

	F32 val = sldr->getValueF32();
	LLWLParamManager::instance().mCurParams.set("cloud_shadow", val);
}

void LLFloaterEnvSettings::onChangeWaterFogDensity(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldr = static_cast<LLSliderCtrl*>(ctrl);
	F32 val = sldr->getValueF32();

	WaterExpFloatControl* expFloatControl = static_cast<WaterExpFloatControl*>(userData);
	expFloatControl->mExp = val;
	LLWaterParamManager::instance().setDensitySliderValue(val);

	expFloatControl->update(LLWaterParamManager::instance().mCurParams);
	LLWaterParamManager::instance().propagateParameters();
}

void LLFloaterEnvSettings::onChangeWaterColor(LLUICtrl* ctrl, void* userData)
{
	LLColorSwatchCtrl* swatch = static_cast<LLColorSwatchCtrl*>(ctrl);
	WaterColorControl* colorControl = static_cast<WaterColorControl*>(userData);
	*colorControl = swatch->get();

	colorControl->update(LLWaterParamManager::instance().mCurParams);
	LLWaterParamManager::instance().propagateParameters();
}

void LLFloaterEnvSettings::onOpenAdvancedSky(void* userData1, void* userData2)
{
	// *TODO: make sure title is displayed correctly.
	LLFloaterReg::showInstance("env_windlight");
}

void LLFloaterEnvSettings::onOpenAdvancedWater(void* userData1, void* userData2)
{
	// *TODO: make sure title is displayed correctly.
	LLFloaterReg::showInstance("env_water");
}


void LLFloaterEnvSettings::onUseEstateTime(void* userData1, void* userData2)
{
	LLFloaterWindLight* wl = LLFloaterReg::findTypedInstance<LLFloaterWindLight>("env_windlight");
	if(wl)
	{
		LLComboBox* box = wl->getChild<LLComboBox>("WLPresetsCombo");
		box->selectByValue("");
	}

	LLWLParamManager::instance().mAnimator.activate(LLWLAnimator::TIME_LINDEN);
}
