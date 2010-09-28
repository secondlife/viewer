/** 
 * @file llfloaterdaycycle.cpp
 * @brief LLFloaterDayCycle class definition
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


std::map<std::string, LLWLSkyKey> LLFloaterDayCycle::sSliderToKey;
const F32 LLFloaterDayCycle::sHoursPerDay = 24.0f;

LLFloaterDayCycle::LLFloaterDayCycle(const LLSD& key)	
: LLFloater(key)
{
}

BOOL LLFloaterDayCycle::postBuild()
{
	// add the combo boxes
	LLComboBox* keyCombo = getChild<LLComboBox>("WLKeyPresets");

	if(keyCombo != NULL) 
	{
		keyCombo->removeall();
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

	syncMenu();
	syncSliderTrack();
	
	return TRUE;
}

LLFloaterDayCycle::~LLFloaterDayCycle()
{
}

void LLFloaterDayCycle::initCallbacks(void) 
{
	// WL Day Cycle
	getChild<LLUICtrl>("WLTimeSlider")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onTimeSliderMoved, this, _1));
	getChild<LLUICtrl>("WLDayCycleKeys")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onKeyTimeMoved, this, _1));
	getChild<LLUICtrl>("WLCurKeyHour")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onKeyTimeChanged, this, _1));
	getChild<LLUICtrl>("WLCurKeyMin")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onKeyTimeChanged, this, _1));
	getChild<LLUICtrl>("WLKeyPresets")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onKeyPresetChanged, this, _1));

	getChild<LLUICtrl>("WLLengthOfDayHour")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onTimeRateChanged, this, _1));
	getChild<LLUICtrl>("WLLengthOfDayMin")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onTimeRateChanged, this, _1));
	getChild<LLUICtrl>("WLLengthOfDaySec")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onTimeRateChanged, this, _1));
	getChild<LLUICtrl>("WLUseLindenTime")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onUseLindenTime, this, _1));
	getChild<LLUICtrl>("WLAnimSky")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onRunAnimSky, this, _1));
	getChild<LLUICtrl>("WLStopAnimSky")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onStopAnimSky, this, _1));

	getChild<LLUICtrl>("WLLoadDayCycle")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onLoadDayCycle, this, _1));
	getChild<LLUICtrl>("WLSaveDayCycle")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onSaveDayCycle, this, _1));

	getChild<LLUICtrl>("WLAddKey")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onAddKey, this, _1));
	getChild<LLUICtrl>("WLDeleteKey")->setCommitCallback(boost::bind(&LLFloaterDayCycle::onDeleteKey, this, _1));
}

void LLFloaterDayCycle::syncMenu()
{
//	std::map<std::string, LLVector4> & currentParams = LLWLParamManager::instance()->mCurParams.mParamValues;
	
	// set time
	LLMultiSliderCtrl* sldr = getChild<LLMultiSliderCtrl>("WLTimeSlider");
	sldr->setCurSliderValue((F32)LLWLParamManager::instance()->mAnimator.getDayTime() * sHoursPerDay);

	LLSpinCtrl* secSpin = getChild<LLSpinCtrl>("WLLengthOfDaySec");
	LLSpinCtrl* minSpin = getChild<LLSpinCtrl>("WLLengthOfDayMin");
	LLSpinCtrl* hourSpin = getChild<LLSpinCtrl>("WLLengthOfDayHour");

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
		getChildView("WLUseLindenTime")->setEnabled(FALSE);
	} 
	else 
	{
		getChildView("WLUseLindenTime")->setEnabled(TRUE);
	}
}

void LLFloaterDayCycle::syncSliderTrack()
{
	// clear the slider
	LLMultiSliderCtrl* kSldr = getChild<LLMultiSliderCtrl>("WLDayCycleKeys");

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
	sldr = getChild<LLMultiSliderCtrl>( 
		"WLDayCycleKeys");
	llassert_always(sSliderToKey.size() == sldr->getValue().size());
	
	LLMultiSliderCtrl* tSldr;
	tSldr = getChild<LLMultiSliderCtrl>( 
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

void LLFloaterDayCycle::onRunAnimSky(LLUICtrl* ctrl)
{
	// if no keys, do nothing
	if(sSliderToKey.size() == 0) 
	{
		return;
	}
	
	LLMultiSliderCtrl* sldr;
	sldr = getChild<LLMultiSliderCtrl>("WLDayCycleKeys");
	llassert_always(sSliderToKey.size() == sldr->getValue().size());

	LLMultiSliderCtrl* tSldr;
	tSldr = getChild<LLMultiSliderCtrl>("WLTimeSlider");

	// turn off linden time
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

	// set the param manager's track to the new one
	LLWLParamManager::instance()->resetAnimator(
		tSldr->getCurSliderValue() / sHoursPerDay, true);

	llassert_always(LLWLParamManager::instance()->mAnimator.mTimeTrack.size() == sldr->getValue().size());
}

void LLFloaterDayCycle::onStopAnimSky(LLUICtrl* ctrl)
{
	// if no keys, do nothing
	if(sSliderToKey.size() == 0) {
		return;
	}

	// turn off animation and using linden time
	LLWLParamManager::instance()->mAnimator.mIsRunning = false;
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;
}

void LLFloaterDayCycle::onUseLindenTime(LLUICtrl* ctrl)
{
	LLComboBox* box = getChild<LLComboBox>("WLPresetsCombo");
	box->selectByValue("");	

	LLWLParamManager::instance()->mAnimator.mIsRunning = true;
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = true;	
}

void LLFloaterDayCycle::onLoadDayCycle(LLUICtrl* ctrl)
{
	LLWLParamManager::instance()->mDay.loadDayCycle("Default.xml");
	
	// sync it all up
	syncSliderTrack();
	syncMenu();

	// set the param manager's track to the new one
	LLMultiSliderCtrl* tSldr;
	tSldr = getChild<LLMultiSliderCtrl>( 
		"WLTimeSlider");
	LLWLParamManager::instance()->resetAnimator(
		tSldr->getCurSliderValue() / sHoursPerDay, false);

	// and draw it
	LLWLParamManager::instance()->mAnimator.update(
		LLWLParamManager::instance()->mCurParams);
}

void LLFloaterDayCycle::onSaveDayCycle(LLUICtrl* ctrl)
{
	LLWLParamManager::instance()->mDay.saveDayCycle("Default.xml");
}


void LLFloaterDayCycle::onTimeSliderMoved(LLUICtrl* ctrl)
{
	LLMultiSliderCtrl* sldr = getChild<LLMultiSliderCtrl>( 
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

void LLFloaterDayCycle::onKeyTimeMoved(LLUICtrl* ctrl)
{
	LLComboBox* comboBox = getChild<LLComboBox>("WLKeyPresets");
	LLMultiSliderCtrl* sldr = getChild<LLMultiSliderCtrl>("WLDayCycleKeys");
	LLSpinCtrl* hourSpin = getChild<LLSpinCtrl>("WLCurKeyHour");
	LLSpinCtrl* minSpin = getChild<LLSpinCtrl>("WLCurKeyMin");

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

void LLFloaterDayCycle::onKeyTimeChanged(LLUICtrl* ctrl)
{
	// if no keys, skipped
	if(sSliderToKey.size() == 0) {
		return;
	}

	LLMultiSliderCtrl* sldr = getChild<LLMultiSliderCtrl>( 
		"WLDayCycleKeys");
	LLSpinCtrl* hourSpin = getChild<LLSpinCtrl>( 
		"WLCurKeyHour");
	LLSpinCtrl* minSpin = getChild<LLSpinCtrl>( 
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

void LLFloaterDayCycle::onKeyPresetChanged(LLUICtrl* ctrl)
{
	// get the time
	LLComboBox* comboBox = getChild<LLComboBox>( 
		"WLKeyPresets");
	LLMultiSliderCtrl* sldr = getChild<LLMultiSliderCtrl>( 
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

void LLFloaterDayCycle::onTimeRateChanged(LLUICtrl* ctrl)
{
	// get the time
	LLSpinCtrl* secSpin = getChild<LLSpinCtrl>( 
		"WLLengthOfDaySec");

	LLSpinCtrl* minSpin = getChild<LLSpinCtrl>( 
		"WLLengthOfDayMin");

	LLSpinCtrl* hourSpin = getChild<LLSpinCtrl>( 
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

void LLFloaterDayCycle::onAddKey(LLUICtrl* ctrl)
{
	LLComboBox* comboBox = getChild<LLComboBox>( 
		"WLKeyPresets");
	LLMultiSliderCtrl* kSldr = getChild<LLMultiSliderCtrl>( 
		"WLDayCycleKeys");
	LLMultiSliderCtrl* tSldr = getChild<LLMultiSliderCtrl>( 
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
	LLMultiSliderCtrl* kSldr = getChild<LLMultiSliderCtrl>( 
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
	LLMultiSliderCtrl* sldr = getChild<LLMultiSliderCtrl>("WLDayCycleKeys");

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

void LLFloaterDayCycle::onDeleteKey(LLUICtrl* ctrl)
{
	if(sSliderToKey.size() == 0) {
		return;
	}

	LLComboBox* comboBox = getChild<LLComboBox>( 
		"WLKeyPresets");
	LLMultiSliderCtrl* sldr = getChild<LLMultiSliderCtrl>("WLDayCycleKeys");

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

	LLSpinCtrl* hourSpin = getChild<LLSpinCtrl>("WLCurKeyHour");
	LLSpinCtrl* minSpin = getChild<LLSpinCtrl>("WLCurKeyMin");

	// now set the spinners
	F32 hour = (F32)((S32)time);
	F32 min = (time - hour) / 60;
	hourSpin->set(hour);
	minSpin->set(min);

	syncTrack();

}
