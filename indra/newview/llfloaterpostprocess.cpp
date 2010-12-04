/** 
 * @file llfloaterpostprocess.cpp
 * @brief LLFloaterPostProcess class definition
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

#include "llfloaterpostprocess.h"

#include "llsliderctrl.h"
#include "llcheckboxctrl.h"
#include "llnotificationsutil.h"
#include "lluictrlfactory.h"
#include "llviewerdisplay.h"
#include "llpostprocess.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llviewerwindow.h"


LLFloaterPostProcess::LLFloaterPostProcess(const LLSD& key)
  : LLFloater(key)
{
}

LLFloaterPostProcess::~LLFloaterPostProcess()
{


}
BOOL LLFloaterPostProcess::postBuild()
{
	/// Color Filter Callbacks
	childSetCommitCallback("ColorFilterToggle", &LLFloaterPostProcess::onBoolToggle, (char*)"enable_color_filter");
	//childSetCommitCallback("ColorFilterGamma", &LLFloaterPostProcess::onFloatControlMoved, &(gPostProcess->tweaks.gamma()));
	childSetCommitCallback("ColorFilterBrightness", &LLFloaterPostProcess::onFloatControlMoved, (char*)"brightness");
	childSetCommitCallback("ColorFilterSaturation", &LLFloaterPostProcess::onFloatControlMoved, (char*)"saturation");
	childSetCommitCallback("ColorFilterContrast", &LLFloaterPostProcess::onFloatControlMoved, (char*)"contrast");

	childSetCommitCallback("ColorFilterBaseR", &LLFloaterPostProcess::onColorControlRMoved, (char*)"contrast_base");
	childSetCommitCallback("ColorFilterBaseG", &LLFloaterPostProcess::onColorControlGMoved, (char*)"contrast_base");
	childSetCommitCallback("ColorFilterBaseB", &LLFloaterPostProcess::onColorControlBMoved, (char*)"contrast_base");
	childSetCommitCallback("ColorFilterBaseI", &LLFloaterPostProcess::onColorControlIMoved, (char*)"contrast_base");

	/// Night Vision Callbacks
	childSetCommitCallback("NightVisionToggle", &LLFloaterPostProcess::onBoolToggle, (char*)"enable_night_vision");
	childSetCommitCallback("NightVisionBrightMult", &LLFloaterPostProcess::onFloatControlMoved, (char*)"brightness_multiplier");
	childSetCommitCallback("NightVisionNoiseSize", &LLFloaterPostProcess::onFloatControlMoved, (char*)"noise_size");
	childSetCommitCallback("NightVisionNoiseStrength", &LLFloaterPostProcess::onFloatControlMoved, (char*)"noise_strength");

	/// Bloom Callbacks
	childSetCommitCallback("BloomToggle", &LLFloaterPostProcess::onBoolToggle, (char*)"enable_bloom");
	childSetCommitCallback("BloomExtract", &LLFloaterPostProcess::onFloatControlMoved, (char*)"extract_low");
	childSetCommitCallback("BloomSize", &LLFloaterPostProcess::onFloatControlMoved, (char*)"bloom_width");
	childSetCommitCallback("BloomStrength", &LLFloaterPostProcess::onFloatControlMoved, (char*)"bloom_strength");

	// Effect loading and saving.
	LLComboBox* comboBox = getChild<LLComboBox>("PPEffectsCombo");
	getChild<LLComboBox>("PPLoadEffect")->setCommitCallback(boost::bind(&LLFloaterPostProcess::onLoadEffect, this, comboBox));
	comboBox->setCommitCallback(boost::bind(&LLFloaterPostProcess::onChangeEffectName, this, _1));

	LLLineEditor* editBox = getChild<LLLineEditor>("PPEffectNameEditor");
	getChild<LLComboBox>("PPSaveEffect")->setCommitCallback(boost::bind(&LLFloaterPostProcess::onSaveEffect, this, editBox));

	syncMenu();
	return TRUE;
}

// Bool Toggle
void LLFloaterPostProcess::onBoolToggle(LLUICtrl* ctrl, void* userData)
{
	char const * boolVariableName = (char const *)userData;
	
	// check the bool
	LLCheckBoxCtrl* cbCtrl = static_cast<LLCheckBoxCtrl*>(ctrl);
	gPostProcess->tweaks[boolVariableName] = cbCtrl->getValue();
}

// Float Moved
void LLFloaterPostProcess::onFloatControlMoved(LLUICtrl* ctrl, void* userData)
{
	char const * floatVariableName = (char const *)userData;
	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	gPostProcess->tweaks[floatVariableName] = sldrCtrl->getValue();
}

// Color Moved
void LLFloaterPostProcess::onColorControlRMoved(LLUICtrl* ctrl, void* userData)
{
	char const * floatVariableName = (char const *)userData;
	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	gPostProcess->tweaks[floatVariableName][0] = sldrCtrl->getValue();
}

// Color Moved
void LLFloaterPostProcess::onColorControlGMoved(LLUICtrl* ctrl, void* userData)
{
	char const * floatVariableName = (char const *)userData;
	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	gPostProcess->tweaks[floatVariableName][1] = sldrCtrl->getValue();
}

// Color Moved
void LLFloaterPostProcess::onColorControlBMoved(LLUICtrl* ctrl, void* userData)
{
	char const * floatVariableName = (char const *)userData;
	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	gPostProcess->tweaks[floatVariableName][2] = sldrCtrl->getValue();
}

// Color Moved
void LLFloaterPostProcess::onColorControlIMoved(LLUICtrl* ctrl, void* userData)
{
	char const * floatVariableName = (char const *)userData;
	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	gPostProcess->tweaks[floatVariableName][3] = sldrCtrl->getValue();
}

void LLFloaterPostProcess::onLoadEffect(LLComboBox* comboBox)
{
	LLSD::String effectName(comboBox->getSelectedValue().asString());

	gPostProcess->setSelectedEffect(effectName);

	syncMenu();
}

void LLFloaterPostProcess::onSaveEffect(LLLineEditor* editBox)
{
	std::string effectName(editBox->getValue().asString());

	if (gPostProcess->mAllEffects.has(effectName))
	{
		LLSD payload;
		payload["effect_name"] = effectName;
		LLNotificationsUtil::add("PPSaveEffectAlert", LLSD(), payload, boost::bind(&LLFloaterPostProcess::saveAlertCallback, this, _1, _2));
	}
	else
	{
		gPostProcess->saveEffect(effectName);
		syncMenu();
	}
}

void LLFloaterPostProcess::onChangeEffectName(LLUICtrl* ctrl)
{
	// get the combo box and name
	LLLineEditor* editBox = getChild<LLLineEditor>("PPEffectNameEditor");

	// set the parameter's new name
	editBox->setValue(ctrl->getValue());
}

bool LLFloaterPostProcess::saveAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	// if they choose save, do it.  Otherwise, don't do anything
	if (option == 0)
	{
		gPostProcess->saveEffect(notification["payload"]["effect_name"].asString());

		syncMenu();
	}
	return false;
}

void LLFloaterPostProcess::syncMenu()
{
	// add the combo boxe contents
	LLComboBox* comboBox = getChild<LLComboBox>("PPEffectsCombo");

	comboBox->removeall();

	LLSD::map_const_iterator currEffect;
	for(currEffect = gPostProcess->mAllEffects.beginMap();
		currEffect != gPostProcess->mAllEffects.endMap();
		++currEffect) 
	{
		comboBox->add(currEffect->first);
	}

	// set the current effect as selected.
	comboBox->selectByValue(gPostProcess->getSelectedEffect());

	/// Sync Color Filter Menu
	getChild<LLUICtrl>("ColorFilterToggle")->setValue(gPostProcess->tweaks.useColorFilter());
	//getChild<LLUICtrl>("ColorFilterGamma")->setValue(gPostProcess->tweaks.gamma());
	getChild<LLUICtrl>("ColorFilterBrightness")->setValue(gPostProcess->tweaks.brightness());
	getChild<LLUICtrl>("ColorFilterSaturation")->setValue(gPostProcess->tweaks.saturation());
	getChild<LLUICtrl>("ColorFilterContrast")->setValue(gPostProcess->tweaks.contrast());
	getChild<LLUICtrl>("ColorFilterBaseR")->setValue(gPostProcess->tweaks.contrastBaseR());
	getChild<LLUICtrl>("ColorFilterBaseG")->setValue(gPostProcess->tweaks.contrastBaseG());
	getChild<LLUICtrl>("ColorFilterBaseB")->setValue(gPostProcess->tweaks.contrastBaseB());
	getChild<LLUICtrl>("ColorFilterBaseI")->setValue(gPostProcess->tweaks.contrastBaseIntensity());
	
	/// Sync Night Vision Menu
	getChild<LLUICtrl>("NightVisionToggle")->setValue(gPostProcess->tweaks.useNightVisionShader());
	getChild<LLUICtrl>("NightVisionBrightMult")->setValue(gPostProcess->tweaks.brightMult());
	getChild<LLUICtrl>("NightVisionNoiseSize")->setValue(gPostProcess->tweaks.noiseSize());
	getChild<LLUICtrl>("NightVisionNoiseStrength")->setValue(gPostProcess->tweaks.noiseStrength());

	/// Sync Bloom Menu
	getChild<LLUICtrl>("BloomToggle")->setValue(LLSD(gPostProcess->tweaks.useBloomShader()));
	getChild<LLUICtrl>("BloomExtract")->setValue(gPostProcess->tweaks.extractLow());
	getChild<LLUICtrl>("BloomSize")->setValue(gPostProcess->tweaks.bloomWidth());
	getChild<LLUICtrl>("BloomStrength")->setValue(gPostProcess->tweaks.bloomStrength());
}
