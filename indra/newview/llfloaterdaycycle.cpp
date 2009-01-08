/** 
 * @file llfloaterdaycycle.cpp
 * @brief LLFloaterDayCycle class definition
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

#include "llfloaterdaycycle.h"

#include "pipeline.h"
#include "llsky.h"

#include "llsliderctrl.h"
#include "llmultislider.h"
#include "llmultisliderctrl.h"
#include "llspinctrl.h"
#include "llcheckboxctrl.h"
#include "lluictrlfactory.h"
#include "llviewercamera.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llwlanimator.h"

#include "v4math.h"
#include "llviewerdisplay.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

#include "llwlparamset.h"
#include "llwlparammanager.h"
#include "llpostprocess.h"
#include "llfloaterwindlight.h"


LLFloaterDayCycle* LLFloaterDayCycle::sDayCycle = NULL;
std::map<std::string, LLWLSkyKey> LLFloaterDayCycle::sSliderToKey;
const F32 LLFloaterDayCycle::sHoursPerDay = 24.0f;

LLFloaterDayCycle::LLFloaterDayCycle() : LLFloater(std::string("Day Cycle Floater"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_day_cycle_options.xml");
	
	// add the combo boxes
	LLComboBox* keyCombo = getChild<LLComboBox>("WLKeyPresets");

	if(keyCombo != NULL) 
	{
		std::map<std::string, LLWLParamSet>::iterator mIt = 
			LLWLParamManager::instance()->mParamList.begin();
		for(; mIt != LLWLParamManager::instance()->mParamList.end(); mIt++) 
		{
			keyCombo->add(std::string(mIt->first));
		}

		// set defaults on combo boxes
		keyCombo->selectFirstItem();
	}

	// add the time slider
	LLMultiSliderCtrl* sldr = getChild<LLMultiSliderCtrl>("WLTimeSlider");

	sldr->addSlider();

	// load it up
	initCallbacks();
}

LLFloaterDayCycle::~LLFloaterDayCycle()
{
}

void LLFloaterDayCycle::onClickHelp(void* data)
{
	LLFloaterDayCycle* self = LLFloaterDayCycle::instance();

	std::string xml_alert = *(std::string *) data;
	LLNotifications::instance().add(self->contextualNotification(xml_alert));
}

void LLFloaterDayCycle::initHelpBtn(const std::string& name, const std::string& xml_alert)
{
	childSetAction(name, onClickHelp, new std::string(xml_alert));
}

void LLFloaterDayCycle::initCallbacks(void) 
{
	initHelpBtn("WLDayCycleHelp", "HelpDayCycle");

	// WL Day Cycle
	childSetCommitCallback("WLTimeSlider", onTimeSliderMoved, NULL);
	childSetCommitCallback("WLDayCycleKeys", onKeyTimeMoved, NULL);
	childSetCommitCallback("WLCurKeyHour", onKeyTimeChanged, NULL);
	childSetCommitCallback("WLCurKeyMin", onKeyTimeChanged, NULL);
	childSetCommitCallback("WLKeyPresets", onKeyPresetChanged, NULL);

	childSetCommitCallback("WLLengthOfDayHour", onTimeRateChanged, NULL);
	childSetCommitCallback("WLLengthOfDayMin", onTimeRateChanged, NULL);
	childSetCommitCallback("WLLengthOfDaySec", onTimeRateChanged, NULL);
	childSetAction("WLUseLindenTime", onUseLindenTime, NULL);
	childSetAction("WLAnimSky", onRunAnimSky, NULL);
	childSetAction("WLStopAnimSky", onStopAnimSky, NULL);

	childSetAction("WLLoadDayCycle", onLoadDayCycle, NULL);
	childSetAction("WLSaveDayCycle", onSaveDayCycle, NULL);

	childSetAction("WLAddKey", onAddKey, NULL);
	childSetAction("WLDeleteKey", onDeleteKey, NULL);
}

void LLFloaterDayCycle::syncMenu()
{
//	std::map<std::string, LLVector4> & currentParams = LLWLParamManager::instance()->mCurParams.mParamValues;
	
	// set time
	LLMultiSliderCtrl* sldr = LLFloaterDayCycle::sDayCycle->getChild<LLMultiSliderCtrl>("WLTimeSlider");
	sldr->setCurSliderValue((F32)LLWLParamManager::instance()->mAnimator.getDayTime() * sHoursPerDay);

	LLSpinCtrl* secSpin = sDayCycle->getChild<LLSpinCtrl>("WLLengthOfDaySec");
	LLSpinCtrl* minSpin = sDayCycle->getChild<LLSpinCtrl>("WLLengthOfDayMin");
	LLSpinCtrl* hourSpin = sDayCycle->getChild<LLSpinCtrl>("WLLengthOfDayHour");

	F32 curRate;
	F32 hours, min, sec;

	// get the current rate
	curRate = LLWLParamManager::instance()->mDay.mDayRate;
	hours = (F32)((int)(curRate / 60 / 60));
	curRate -= (hours * 60 * 60);
	min = (F32)((int)(curRate / 60));
	curRate -= (min * 60);
	sec = curRate;

	hourSpin->setValue(hours);
	minSpin->setValue(min);
	secSpin->setValue(sec);

	// turn off Use Estate Time button if it's already being used
	if(	LLWLParamManager::instance()->mAnimator.mUseLindenTime == true)
	{
		LLFloaterDayCycle::sDayCycle->childDisable("WLUseLindenTime");
	} 
	else 
	{
		LLFloaterDayCycle::sDayCycle->childEnable("WLUseLindenTime");
	}
}

void LLFloaterDayCycle::syncSliderTrack()
{
	// clear the slider
	LLMultiSliderCtrl* kSldr = sDayCycle->getChild<LLMultiSliderCtrl>("WLDayCycleKeys");

	kSldr->clear();
	sSliderToKey.clear();

	// add sliders
	std::map<F32, std::string>::iterator mIt = 
		LLWLParamManager::instance()->mDay.mTimeMap.begin();
	for(; mIt != LLWLParamManager::instance()->mDay.mTimeMap.end(); mIt++) 
	{
		addSliderKey(mIt->first * sHoursPerDay, mIt->second);
	}
}

void LLFloaterDayCycle::syncTrack()
{
	// if no keys, do nothing
	if(sSliderToKey.size() == 0) 
	{
		return;
	}
	
	LLMultiSliderCtrl* sldr;
	sldr = sDayCycle->getChild<LLMultiSliderCtrl>( 
		"WLDayCycleKeys");
	llassert_always(sSliderToKey.size() == sldr->getValue().size());
	
	LLMultiSliderCtrl* tSldr;
	tSldr = sDayCycle->getChild<LLMultiSliderCtrl>( 
		"WLTimeSlider");

	// create a new animation track
	LLWLParamManager::instance()->mDay.clearKeys();
	
	// add the keys one by one
	std::map<std::string, LLWLSkyKey>::iterator mIt = sSliderToKey.begin();
	for(; mIt != sSliderToKey.end(); mIt++) 
	{
		LLWLParamManager::instance()->mDay.addKey(mIt->second.time / sHoursPerDay, 
			mIt->second.presetName);
	}
	
	// set the param manager's track to the new one
	LLWLParamManager::instance()->resetAnimator(
		tSldr->getCurSliderValue() / sHoursPerDay, false);

	LLWLParamManager::instance()->mAnimator.update(
		LLWLParamManager::instance()->mCurParams);
}

// static
LLFloaterDayCycle* LLFloaterDayCycle::instance()
{
	if (!sDayCycle)
	{
		sDayCycle = new LLFloaterDayCycle();
		sDayCycle->open();
		sDayCycle->setFocus(TRUE);
	}
	return sDayCycle;
}

bool LLFloaterDayCycle::isOpen()
{
	if (sDayCycle != NULL) 
	{
		return true;
	}
	return false;
}

void LLFloaterDayCycle::show()
{
	LLFloaterDayCycle* dayCycle = instance();
	dayCycle->syncMenu();
	syncSliderTrack();

	// comment in if you want the menu to rebuild each time
	//LLUICtrlFactory::getInstance()->buildFloater(dayCycle, "floater_day_cycle_options.xml");
	//dayCycle->initCallbacks();

	dayCycle->open();
}

// virtual
void LLFloaterDayCycle::onClose(bool app_quitting)
{
	if (sDayCycle)
	{
		sDayCycle->setVisible(FALSE);
	}
}

void LLFloaterDayCycle::onRunAnimSky(void* userData)
{
	// if no keys, do nothing
	if(sSliderToKey.size() == 0) 
	{
		return;
	}
	
	LLMultiSliderCtrl* sldr;
	sldr = sDayCycle->getChild<LLMultiSliderCtrl>("WLDayCycleKeys");
	llassert_always(sSliderToKey.size() == sldr->getValue().size());

	LLMultiSliderCtrl* tSldr;
	tSldr = sDayCycle->getChild<LLMultiSliderCtrl>("WLTimeSlider");

	// turn off linden time
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

	// set the param manager's track to the new one
	LLWLParamManager::instance()->resetAnimator(
		tSldr->getCurSliderValue() / sHoursPerDay, true);

	llassert_always(LLWLParamManager::instance()->mAnimator.mTimeTrack.size() == sldr->getValue().size());
}

void LLFloaterDayCycle::onStopAnimSky(void* userData)
{
	// if no keys, do nothing
	if(sSliderToKey.size() == 0) {
		return;
	}

	// turn off animation and using linden time
	LLWLParamManager::instance()->mAnimator.mIsRunning = false;
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;
}

void LLFloaterDayCycle::onUseLindenTime(void* userData)
{
	LLFloaterWindLight* wl = LLFloaterWindLight::instance();
	LLComboBox* box = wl->getChild<LLComboBox>("WLPresetsCombo");
	box->selectByValue("");	

	LLWLParamManager::instance()->mAnimator.mIsRunning = true;
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = true;	
}

void LLFloaterDayCycle::onLoadDayCycle(void* userData)
{
	LLWLParamManager::instance()->mDay.loadDayCycle("Default.xml");
	
	// sync it all up
	syncSliderTrack();
	syncMenu();

	// set the param manager's track to the new one
	LLMultiSliderCtrl* tSldr;
	tSldr = sDayCycle->getChild<LLMultiSliderCtrl>( 
		"WLTimeSlider");
	LLWLParamManager::instance()->resetAnimator(
		tSldr->getCurSliderValue() / sHoursPerDay, false);

	// and draw it
	LLWLParamManager::instance()->mAnimator.update(
		LLWLParamManager::instance()->mCurParams);
}

void LLFloaterDayCycle::onSaveDayCycle(void* userData)
{
	LLWLParamManager::instance()->mDay.saveDayCycle("Default.xml");
}


void LLFloaterDayCycle::onTimeSliderMoved(LLUICtrl* ctrl, void* userData)
{
	LLMultiSliderCtrl* sldr = sDayCycle->getChild<LLMultiSliderCtrl>( 
		"WLTimeSlider");

	/// get the slider value
	F32 val = sldr->getCurSliderValue() / sHoursPerDay;
	
	// set the value, turn off animation
	LLWLParamManager::instance()->mAnimator.setDayTime((F64)val);
	LLWLParamManager::instance()->mAnimator.mIsRunning = false;
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

	// then call update once
	LLWLParamManager::instance()->mAnimator.update(
		LLWLParamManager::instance()->mCurParams);
}

void LLFloaterDayCycle::onKeyTimeMoved(LLUICtrl* ctrl, void* userData)
{
	LLComboBox* comboBox = sDayCycle->getChild<LLComboBox>("WLKeyPresets");
	LLMultiSliderCtrl* sldr = sDayCycle->getChild<LLMultiSliderCtrl>("WLDayCycleKeys");
	LLSpinCtrl* hourSpin = sDayCycle->getChild<LLSpinCtrl>("WLCurKeyHour");
	LLSpinCtrl* minSpin = sDayCycle->getChild<LLSpinCtrl>("WLCurKeyMin");

	if(sldr->getValue().size() == 0) {
		return;
	}

	// make sure we have a slider
	const std::string& curSldr = sldr->getCurSlider();
	if(curSldr == "") {
		return;
	}

	F32 time = sldr->getCurSliderValue();

	// check to see if a key exists
	std::string presetName = sSliderToKey[curSldr].presetName;
	sSliderToKey[curSldr].time = time;

	// if it exists, turn on check box
	comboBox->selectByValue(presetName);

	// now set the spinners
	F32 hour = (F32)((S32)time);
	F32 min = (time - hour) * 60;

	// handle imprecision
	if(min >= 59) {
		min = 0;
		hour += 1;
	}

	hourSpin->set(hour);
	minSpin->set(min);

	syncTrack();

}

void LLFloaterDayCycle::onKeyTimeChanged(LLUICtrl* ctrl, void* userData)
{
	// if no keys, skipped
	if(sSliderToKey.size() == 0) {
		return;
	}

	LLMultiSliderCtrl* sldr = sDayCycle->getChild<LLMultiSliderCtrl>( 
		"WLDayCycleKeys");
	LLSpinCtrl* hourSpin = sDayCycle->getChild<LLSpinCtrl>( 
		"WLCurKeyHour");
	LLSpinCtrl* minSpin = sDayCycle->getChild<LLSpinCtrl>( 
		"WLCurKeyMin");

	F32 hour = hourSpin->get();
	F32 min = minSpin->get();
	F32 val = hour + min / 60.0f;

	const std::string& curSldr = sldr->getCurSlider();
	sldr->setCurSliderValue(val, TRUE);
	F32 time = sldr->getCurSliderValue() / sHoursPerDay;

	// now set the key's time in the sliderToKey map
	std::string presetName = sSliderToKey[curSldr].presetName;
	sSliderToKey[curSldr].time = time;

	syncTrack();
}

void LLFloaterDayCycle::onKeyPresetChanged(LLUICtrl* ctrl, void* userData)
{
	// get the time
	LLComboBox* comboBox = sDayCycle->getChild<LLComboBox>( 
		"WLKeyPresets");
	LLMultiSliderCtrl* sldr = sDayCycle->getChild<LLMultiSliderCtrl>( 
		"WLDayCycleKeys");

	// do nothing if no sliders
	if(sldr->getValue().size() == 0) {
		return;
	}

	// change the map
	std::string newPreset(comboBox->getSelectedValue().asString());
	const std::string& curSldr = sldr->getCurSlider();

	// if null, don't use
	if(curSldr == "") {
		return;
	}

	sSliderToKey[curSldr].presetName = newPreset;

	syncTrack();
}

void LLFloaterDayCycle::onTimeRateChanged(LLUICtrl* ctrl, void* userData)
{
	// get the time
	LLSpinCtrl* secSpin = sDayCycle->getChild<LLSpinCtrl>( 
		"WLLengthOfDaySec");

	LLSpinCtrl* minSpin = sDayCycle->getChild<LLSpinCtrl>( 
		"WLLengthOfDayMin");

	LLSpinCtrl* hourSpin = sDayCycle->getChild<LLSpinCtrl>( 
		"WLLengthOfDayHour");

	F32 hour;
	hour = (F32)hourSpin->getValue().asReal();
	F32 min;
	min = (F32)minSpin->getValue().asReal();
	F32 sec;
	sec = (F32)secSpin->getValue().asReal();

	F32 time = 60.0f * 60.0f * hour + 60.0f * min + sec;
	if(time <= 0) {
		time = 1;
	}
	LLWLParamManager::instance()->mDay.mDayRate = time;

	syncTrack();
}

void LLFloaterDayCycle::onAddKey(void* userData)
{
	LLComboBox* comboBox = sDayCycle->getChild<LLComboBox>( 
		"WLKeyPresets");
	LLMultiSliderCtrl* kSldr = sDayCycle->getChild<LLMultiSliderCtrl>( 
		"WLDayCycleKeys");
	LLMultiSliderCtrl* tSldr = sDayCycle->getChild<LLMultiSliderCtrl>( 
		"WLTimeSlider");
	
	llassert_always(sSliderToKey.size() == kSldr->getValue().size());

	// get the values
	std::string newPreset(comboBox->getSelectedValue().asString());

	// add the slider key
	addSliderKey(tSldr->getCurSliderValue(), newPreset);

	syncTrack();
}

void LLFloaterDayCycle::addSliderKey(F32 time, const std::string & presetName)
{
	LLMultiSliderCtrl* kSldr = sDayCycle->getChild<LLMultiSliderCtrl>( 
		"WLDayCycleKeys");

	// make a slider
	const std::string& sldrName = kSldr->addSlider(time);
	if(sldrName == "") {
		return;
	}

	// set the key
	LLWLSkyKey newKey;
	newKey.presetName = presetName;
	newKey.time = kSldr->getCurSliderValue();

	llassert_always(sldrName != LLStringUtil::null);

	// add to map
	sSliderToKey.insert(std::pair<std::string, LLWLSkyKey>(sldrName, newKey));

	llassert_always(sSliderToKey.size() == kSldr->getValue().size());

}

void LLFloaterDayCycle::deletePreset(std::string& presetName)
{
	LLMultiSliderCtrl* sldr = sDayCycle->getChild<LLMultiSliderCtrl>("WLDayCycleKeys");

	/// delete any reference
	std::map<std::string, LLWLSkyKey>::iterator curr_preset, next_preset;
	for(curr_preset = sSliderToKey.begin(); curr_preset != sSliderToKey.end(); curr_preset = next_preset)
	{
		next_preset = curr_preset;
		++next_preset;
		if (curr_preset->second.presetName == presetName)
		{
			sldr->deleteSlider(curr_preset->first);
			sSliderToKey.erase(curr_preset);
		}
	}
}

void LLFloaterDayCycle::onDeleteKey(void* userData)
{
	if(sSliderToKey.size() == 0) {
		return;
	}

	LLComboBox* comboBox = sDayCycle->getChild<LLComboBox>( 
		"WLKeyPresets");
	LLMultiSliderCtrl* sldr = sDayCycle->getChild<LLMultiSliderCtrl>("WLDayCycleKeys");

	// delete from map
	const std::string& sldrName = sldr->getCurSlider();
	std::map<std::string, LLWLSkyKey>::iterator mIt = sSliderToKey.find(sldrName);
	sSliderToKey.erase(mIt);

	sldr->deleteCurSlider();

	if(sSliderToKey.size() == 0) {
		return;
	}

	const std::string& name = sldr->getCurSlider();
	comboBox->selectByValue(sSliderToKey[name].presetName);
	F32 time = sSliderToKey[name].time;

	LLSpinCtrl* hourSpin = sDayCycle->getChild<LLSpinCtrl>("WLCurKeyHour");
	LLSpinCtrl* minSpin = sDayCycle->getChild<LLSpinCtrl>("WLCurKeyMin");

	// now set the spinners
	F32 hour = (F32)((S32)time);
	F32 min = (time - hour) / 60;
	hourSpin->set(hour);
	minSpin->set(min);

	syncTrack();

}
