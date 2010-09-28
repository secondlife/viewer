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

#include "llfloaterwindlight.h"
#include "llfloaterwater.h"
#include "llfloaterdaycycle.h"
#include "llfloaterregioninfo.h"
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
#include "llviewercontrol.h"

#include "llcheckboxctrl.h"

#include "pipeline.h"

#include <sstream>

LLFloaterEnvSettings* LLFloaterEnvSettings::sEnvSettings = NULL;

LLFloaterEnvSettings::LLFloaterEnvSettings() : LLFloater(std::string("Environment Settings Floater"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_env_settings.xml", NULL, FALSE);
	
	// load it up
	initCallbacks();
}

LLFloaterEnvSettings::~LLFloaterEnvSettings()
{
}

void LLFloaterEnvSettings::onClickHelp(void* data)
{
	LLFloaterEnvSettings* self = (LLFloaterEnvSettings*)data;
	LLNotifications::instance().add(self->contextualNotification("EnvSettingsHelpButton"));
}

void LLFloaterEnvSettings::onUseRegionEnvironment(LLUICtrl* ctrl, void* data)
{
	LLFloaterEnvSettings* self = (LLFloaterEnvSettings*)data;
	LLCheckBoxCtrl* checkbox = (LLCheckBoxCtrl*)self->getChildView("RegionWLOptIn");
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
	childSetCommitCallback("EnvTimeSlider", onChangeDayTime, NULL);
	childSetCommitCallback("EnvCloudSlider", onChangeCloudCoverage, NULL);
	childSetCommitCallback("EnvWaterFogSlider", onChangeWaterFogDensity, 
		&LLWaterParamManager::getInstance()->mFogDensity);	

	// color picker
	childSetCommitCallback("EnvWaterColor", onChangeWaterColor, 
		&LLWaterParamManager::getInstance()->mFogColor);

	// WL Top
	childSetAction("EnvAdvancedSkyButton", onOpenAdvancedSky, NULL);
	childSetAction("EnvAdvancedWaterButton", onOpenAdvancedWater, NULL);
	childSetAction("EnvUseEstateTimeButton", onUseEstateTime, NULL);
	childSetAction("EnvUseLocalTimeButton", onUseLocalTime, NULL);
	childSetAction("EnvSettingsHelpButton", onClickHelp, this);

	childSetCommitCallback("RegionWLOptIn", onUseRegionEnvironment, this);
}


// menu maintenance functions

void LLFloaterEnvSettings::syncMenu()
{
	LLSliderCtrl* sldr;
	sldr = sEnvSettings->getChild<LLSliderCtrl>("EnvTimeSlider");

	// sync the clock
	F32 val = (F32)LLWLParamManager::getInstance()->mAnimator.getDayTime();
	std::string timeStr = LLWLAnimator::timeToString(val);

	LLTextBox* textBox;
	textBox = sEnvSettings->getChild<LLTextBox>("EnvTimeText");

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
	LLColorSwatchCtrl* colCtrl = sEnvSettings->getChild<LLColorSwatchCtrl>("EnvWaterColor");
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


// static instance of it
LLFloaterEnvSettings* LLFloaterEnvSettings::instance()
{
	if (!sEnvSettings)
	{
		sEnvSettings = new LLFloaterEnvSettings();
		// sEnvSettings->open();
		// sEnvSettings->setFocus(TRUE);
	}
	return sEnvSettings;
}

void LLFloaterEnvSettings::setControlsEnabled(bool enable)
{
	if(enable)
	{
		// reenable UI elements, resync sliders, and reload saved settings
		childEnable("EnvAdvancedSkyButton");
		childEnable("EnvAdvancedWaterButton");
		childEnable("EnvUseEstateTimeButton");
		childShow("EnvTimeText");
		childShow("EnvWaterColor");
		childShow("EnvTimeSlider");
		childShow("EnvCloudSlider");
		childShow("EnvWaterFogSlider");
		syncMenu();
	}
	else
	{
		// disable UI elements the user shouldn't be able to see to protect potentially proprietary WL region settings from being visible
		LLFloaterWindLight::instance()->close();
		LLFloaterWater::instance()->close();
		LLFloaterDayCycle::instance()->close();
		childDisable("EnvAdvancedSkyButton");
		childDisable("EnvAdvancedWaterButton");
		childDisable("EnvUseEstateTimeButton");
		childHide("EnvTimeText");
		childHide("EnvWaterColor");
		childHide("EnvTimeSlider");
		childHide("EnvCloudSlider");
		childHide("EnvWaterFogSlider");
	}
}


void LLFloaterEnvSettings::show()
{
	LLFloaterEnvSettings* envSettings = instance();
	envSettings->syncMenu();

	// comment in if you want the menu to rebuild each time
	//LLUICtrlFactory::getInstance()->buildFloater(envSettings, "floater_env_settings.xml");
	//envSettings->initCallbacks();

	// Set environment opt-in checkbox based on saved value -- only need to do once, not every time syncMenu is called
	
	bool opt_in = gSavedSettings.getBOOL("UseEnvironmentFromRegion");

	sEnvSettings->childSetVisible("RegionWLOptIn", LLEnvManager::getInstance()->regionCapable());
	sEnvSettings->setOptIn(opt_in);		
	sEnvSettings->getChildView("RegionWLOptIn")->setValue(LLSD::Boolean(opt_in));
	sEnvSettings->getChildView("RegionWLOptIn")->setToolTip(sEnvSettings->getString("region_environment_tooltip"));
	envSettings->open();
}

bool LLFloaterEnvSettings::isOpen()
{
	if (sEnvSettings != NULL) 
	{
		return true;
	}
	return false;
}

// virtual
void LLFloaterEnvSettings::onClose(bool app_quitting)
{
	if (sEnvSettings)
	{
		sEnvSettings->setVisible(FALSE);
	}
}


void LLFloaterEnvSettings::onChangeDayTime(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldr;
	sldr = sEnvSettings->getChild<LLSliderCtrl>("EnvTimeSlider");

	// deactivate animator
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	F32 val = sldr->getValueF32() + 0.25f;
	if(val > 1.0) 
	{
		val--;
	}

	LLWLParamManager::getInstance()->mAnimator.setDayTime((F64)val);
	LLWLParamManager::getInstance()->mAnimator.update(
		LLWLParamManager::getInstance()->mCurParams);
}

void LLFloaterEnvSettings::onChangeCloudCoverage(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldr;
	sldr = sEnvSettings->getChild<LLSliderCtrl>("EnvCloudSlider");
	
	// deactivate animator
	//LLWLParamManager::getInstance()->mAnimator.mIsRunning = false;
	//LLWLParamManager::getInstance()->mAnimator.mUseLindenTime = false;

	F32 val = sldr->getValueF32();
	LLWLParamManager::getInstance()->mCurParams.set("cloud_shadow", val);
}

void LLFloaterEnvSettings::onChangeWaterFogDensity(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldr;
	sldr = sEnvSettings->getChild<LLSliderCtrl>("EnvWaterFogSlider");
	
	if(NULL == userData)
	{
		return;
	}

	WaterExpFloatControl * expFloatControl = static_cast<WaterExpFloatControl *>(userData);
	
	F32 val = sldr->getValueF32();
	expFloatControl->mExp = val;
	LLWaterParamManager::getInstance()->setDensitySliderValue(val);

	expFloatControl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}

void LLFloaterEnvSettings::onChangeWaterColor(LLUICtrl* ctrl, void* userData)
{
	LLColorSwatchCtrl* swatch = static_cast<LLColorSwatchCtrl*>(ctrl);
	WaterColorControl * colorControl = static_cast<WaterColorControl *>(userData);	
	*colorControl = swatch->get();

	colorControl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}


void LLFloaterEnvSettings::onOpenAdvancedSky(void* userData)
{
	LLFloaterWindLight::show();
}

void LLFloaterEnvSettings::onOpenAdvancedWater(void* userData)
{
	LLFloaterWater::show();
}


void LLFloaterEnvSettings::onUseEstateTime(void* userData)
{
	if(LLFloaterWindLight::isOpen())
	{
		// select the blank value in 
		LLFloaterWindLight* wl = LLFloaterWindLight::instance();
		LLComboBox* box = wl->getChild<LLComboBox>("WLPresetsCombo");
		box->selectByValue("");
	}

	LLWLParamManager::getInstance()->mAnimator.activate(LLWLAnimator::TIME_LINDEN);
}

void LLFloaterEnvSettings::onUseLocalTime(void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.setDayTime(LLWLAnimator::getLocalTime());
	LLWLParamManager::getInstance()->mAnimator.activate(LLWLAnimator::TIME_LOCAL);
}
