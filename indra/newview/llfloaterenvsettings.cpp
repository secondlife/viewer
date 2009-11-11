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
#include "lluictrlfactory.h"
#include "llsliderctrl.h"
#include "llcombobox.h"
#include "llcolorswatch.h"
#include "llwlanimator.h"

#include "llwlparamset.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
#include "llmath.h"
#include "llviewerwindow.h"

#include "pipeline.h"

#include <sstream>

LLFloaterEnvSettings::LLFloaterEnvSettings(const LLSD& key)
  : LLFloater(key)
{
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_env_settings.xml");
}
// virtual
LLFloaterEnvSettings::~LLFloaterEnvSettings()
{
}
// virtual
BOOL LLFloaterEnvSettings::postBuild()
{	
	// load it up
	initCallbacks();
	syncMenu();
	return TRUE;
}

void LLFloaterEnvSettings::initCallbacks(void) 
{
	// our three sliders
	getChild<LLUICtrl>("EnvTimeSlider")->setCommitCallback(boost::bind(&LLFloaterEnvSettings::onChangeDayTime, this, _1));
	getChild<LLUICtrl>("EnvCloudSlider")->setCommitCallback(boost::bind(&LLFloaterEnvSettings::onChangeCloudCoverage, this, _1));
	getChild<LLUICtrl>("EnvWaterFogSlider")->setCommitCallback(boost::bind(&LLFloaterEnvSettings::onChangeWaterFogDensity, this, _1, &LLWaterParamManager::instance()->mFogDensity));	

	// color picker
	getChild<LLUICtrl>("EnvWaterColor")->setCommitCallback(boost::bind(&LLFloaterEnvSettings::onChangeWaterColor, this, _1, &LLWaterParamManager::instance()->mFogColor));

	// WL Top
	getChild<LLUICtrl>("EnvAdvancedSkyButton")->setCommitCallback(boost::bind(&LLFloaterEnvSettings::onOpenAdvancedSky, this));
 	getChild<LLUICtrl>("EnvAdvancedWaterButton")->setCommitCallback(boost::bind(&LLFloaterEnvSettings::onOpenAdvancedWater, this));
	getChild<LLUICtrl>("EnvUseEstateTimeButton")->setCommitCallback(boost::bind(&LLFloaterEnvSettings::onUseEstateTime, this));
}

// menu maintenance functions

void LLFloaterEnvSettings::syncMenu()
{
	LLSliderCtrl* sldr;
	sldr = getChild<LLSliderCtrl>("EnvTimeSlider");

	// sync the clock
	F32 val = (F32)LLWLParamManager::instance()->mAnimator.getDayTime();
	std::string timeStr = timeToString(val);

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
	childSetValue("EnvCloudSlider", LLWLParamManager::instance()->mCurParams.getFloat("cloud_shadow", err));

	LLWaterParamManager * param_mgr = LLWaterParamManager::instance();
	// sync water params
	LLColor4 col = param_mgr->getFogColor();
	LLColorSwatchCtrl* colCtrl = getChild<LLColorSwatchCtrl>("EnvWaterColor");
	col.mV[3] = 1.0f;
	colCtrl->set(col);

	childSetValue("EnvWaterFogSlider", param_mgr->mFogDensity.mExp);
	param_mgr->setDensitySliderValue(param_mgr->mFogDensity.mExp);

	// turn off Use Estate Time button if it's already being used
	if(LLWLParamManager::instance()->mAnimator.mUseLindenTime)
	{
		childDisable("EnvUseEstateTimeButton");
	} else {
		childEnable("EnvUseEstateTimeButton");
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

void LLFloaterEnvSettings::onChangeDayTime(LLUICtrl* ctrl)
{
	LLSliderCtrl* sldr = static_cast<LLSliderCtrl*>(ctrl);

	// deactivate animator
	LLWLParamManager::instance()->mAnimator.mIsRunning = false;
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

	F32 val = sldr->getValueF32() + 0.25f;
	if(val > 1.0) 
	{
		val--;
	}

	LLWLParamManager::instance()->mAnimator.setDayTime((F64)val);
	LLWLParamManager::instance()->mAnimator.update(
		LLWLParamManager::instance()->mCurParams);
}

void LLFloaterEnvSettings::onChangeCloudCoverage(LLUICtrl* ctrl)
{
	LLSliderCtrl* sldr = static_cast<LLSliderCtrl*>(ctrl);
	
	// deactivate animator
	//LLWLParamManager::instance()->mAnimator.mIsRunning = false;
	//LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

	F32 val = sldr->getValueF32();
	LLWLParamManager::instance()->mCurParams.set("cloud_shadow", val);
}

void LLFloaterEnvSettings::onChangeWaterFogDensity(LLUICtrl* ctrl, WaterExpFloatControl* expFloatControl)
{
	LLSliderCtrl* sldr;
	sldr = getChild<LLSliderCtrl>("EnvWaterFogSlider");
	
	F32 val = sldr->getValueF32();
	expFloatControl->mExp = val;
	LLWaterParamManager::instance()->setDensitySliderValue(val);

	expFloatControl->update(LLWaterParamManager::instance()->mCurParams);
	LLWaterParamManager::instance()->propagateParameters();
}

void LLFloaterEnvSettings::onChangeWaterColor(LLUICtrl* ctrl, WaterColorControl* colorControl)
{
	LLColorSwatchCtrl* swatch = static_cast<LLColorSwatchCtrl*>(ctrl);
	*colorControl = swatch->get();

	colorControl->update(LLWaterParamManager::instance()->mCurParams);
	LLWaterParamManager::instance()->propagateParameters();
}


void LLFloaterEnvSettings::onOpenAdvancedSky()
{
	LLFloaterReg::showInstance("env_windlight");
}

void LLFloaterEnvSettings::onOpenAdvancedWater()
{
	LLFloaterReg::showInstance("env_water");
}


void LLFloaterEnvSettings::onUseEstateTime()
{
	LLFloaterWindLight* wl = LLFloaterReg::findTypedInstance<LLFloaterWindLight>("env_windlight");
	if(wl)
	{
		LLComboBox* box = wl->getChild<LLComboBox>("WLPresetsCombo");
		box->selectByValue("");
	}

	LLWLParamManager::instance()->mAnimator.mIsRunning = true;
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = true;
}

std::string LLFloaterEnvSettings::timeToString(F32 curTime)
{
	S32 hours;
	S32 min;

	// get hours and minutes
	hours = (S32) (24.0 * curTime);
	curTime -= ((F32) hours / 24.0f);
	min = llround(24.0f * 60.0f * curTime);

	// handle case where it's 60
	if(min == 60) 
	{
		hours++;
		min = 0;
	}

	std::string newTime = getString("timeStr");
	struct tm * timeT;
	time_t secT = time(0);
	timeT = gmtime (&secT);

	timeT->tm_hour = hours;
	timeT->tm_min = min;
	secT = mktime (timeT);
	secT -= LLStringOps::getLocalTimeOffset ();

	LLSD substitution;
	substitution["datetime"] = (S32) secT;

	LLStringUtil::format (newTime, substitution);
	return newTime;
}
