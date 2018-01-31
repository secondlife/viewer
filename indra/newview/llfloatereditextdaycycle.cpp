/** 
 * @file llfloatereditextdaycycle.cpp
 * @brief Floater to create or edit a day cycle
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

#include "llfloatereditextdaycycle.h"

// libs
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llloadingindicator.h"
#include "llmultisliderctrl.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llspinctrl.h"
#include "lltimectrl.h"

// newview
#include "llagent.h"
#include "llregioninfomodel.h"
#include "llviewerregion.h"

#include "llenvironment.h"
#include "lltrans.h"


LLFloaterEditExtDayCycle::LLFloaterEditExtDayCycle(const LLSD &key):	
    LLFloater(key),
    mDayPresetsCombo(NULL),
    mSaveButton(NULL),
    mCancelButton(NULL)
//    mDayCyclesCombo(NULL)
// ,	mTimeSlider(NULL)
// ,	mKeysSlider(NULL)
// ,	mTimeCtrl(NULL)
// ,	mMakeDefaultCheckBox(NULL)
// ,	
{
}

// virtual
BOOL LLFloaterEditExtDayCycle::postBuild()
{
//	mDayCyclesCombo = getChild<LLComboBox>("day_cycle_preset_combo");

// 	mTimeSlider = getChild<LLMultiSliderCtrl>("WLTimeSlider");
// 	mKeysSlider = getChild<LLMultiSliderCtrl>("WLDayCycleKeys");
 	mDayPresetsCombo = getChild<LLComboBox>("day_cycle_preset_combo");
// 	mTimeCtrl = getChild<LLTimeCtrl>("time");
 	mSaveButton = getChild<LLButton>("save_btn");
    mCancelButton = getChild<LLButton>("cancel_btn");
// 	mMakeDefaultCheckBox = getChild<LLCheckBoxCtrl>("make_default_cb");


    mDayPresetsCombo->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onDayPresetChanged, this));
    mSaveButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnSave, this));
    mCancelButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnCancel, this));

	//initCallbacks();

// 	// add the time slider
// 	mTimeSlider->addSlider();

	return TRUE;
}

void LLFloaterEditExtDayCycle::onOpen(const LLSD& key)
{
    refreshSkyPresetsList();
}

void LLFloaterEditExtDayCycle::onClose(bool app_quitting)
{
	if (!app_quitting) // there's no point to change environment if we're quitting
	{
        /* TODO: don't restore this environment.  We may have gotten here from land or region. */
        LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);
        LLEnvironment::instance().updateEnvironment();
	}
}

void LLFloaterEditExtDayCycle::onVisibilityChange(BOOL new_visibility)
{
    if (new_visibility)
    {
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, mEditDay, LLSettingsDay::DEFAULT_DAYLENGTH, LLSettingsDay::DEFAULT_DAYOFFSET);
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT);
    }
    else
    {
        /* TODO: don't restore this environment.  We may have gotten here from land or region. */
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
    }
}

//-------------------------------------------------------------------------
void LLFloaterEditExtDayCycle::onDayPresetChanged()
{
    std::string dayname = mDayPresetsCombo->getSelectedValue().asString();

    LLSettingsDay::ptr_t pday = LLEnvironment::instance().findDayCycleByName(dayname);

    if (pday)
    {
        pday = pday->buildClone();
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, pday, LLSettingsDay::DEFAULT_DAYLENGTH, LLSettingsDay::DEFAULT_DAYOFFSET);
        mEditDay = pday;
    }

}

void LLFloaterEditExtDayCycle::onBtnSave()
{
    if (!mCommitSignal.empty())
        mCommitSignal(mEditDay);
    closeFloater();
}

void LLFloaterEditExtDayCycle::onBtnCancel()
{
	closeFloater();
}


//-------------------------------------------------------------------------
void LLFloaterEditExtDayCycle::refreshSkyPresetsList()
{
    mDayPresetsCombo->removeall();

    LLEnvironment::list_name_id_t cyclelist = LLEnvironment::instance().getDayCycleList();

    mDayPresetsCombo->removeall();


    for (LLEnvironment::list_name_id_t::iterator it = cyclelist.begin(); it != cyclelist.end(); ++it)
    {
        mDayPresetsCombo->add((*it).first);
    }

    // set defaults on combo boxes
    mDayPresetsCombo->selectFirstItem();
}

LLFloaterEditExtDayCycle::connection_t LLFloaterEditExtDayCycle::setEditCommitSignal(LLFloaterEditExtDayCycle::edit_commit_signal_t::slot_type cb)
{
    return mCommitSignal.connect(cb);
}

// 
// virtual
// void LLFloaterEditExtDayCycle::draw()
// {
// 	syncTimeSlider();
// 	LLFloater::draw();
// }
// 
// void LLFloaterEditExtDayCycle::initCallbacks(void)
// {
// #if 0
// 	mDayCycleNameEditor->setKeystrokeCallback(boost::bind(&LLFloaterEditExtDayCycle::onDayCycleNameEdited, this), NULL);
// 	mDayCyclesCombo->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onDayCycleSelected, this));
// 	mDayCyclesCombo->setTextEntryCallback(boost::bind(&LLFloaterEditExtDayCycle::onDayCycleNameEdited, this));
// 	mTimeSlider->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onTimeSliderMoved, this));
// 	mKeysSlider->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onKeyTimeMoved, this));
// 	mTimeCtrl->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onKeyTimeChanged, this));
// 	mSkyPresetsCombo->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onKeyPresetChanged, this));
// 
// 	getChild<LLButton>("WLAddKey")->setClickedCallback(boost::bind(&LLFloaterEditExtDayCycle::onAddKey, this));
// 	getChild<LLButton>("WLDeleteKey")->setClickedCallback(boost::bind(&LLFloaterEditExtDayCycle::onDeleteKey, this));
// 
// 	mSaveButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnSave, this));
// 	mSaveButton->setRightMouseDownCallback(boost::bind(&LLFloaterEditExtDayCycle::dumpTrack, this));
// 	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnCancel, this));
// 
// 	// Connect to env manager events.
// 	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
// 	env_mgr.setRegionSettingsChangeCallback(boost::bind(&LLFloaterEditExtDayCycle::onRegionSettingsChange, this));
// 	gAgent.addRegionChangedCallback(boost::bind(&LLFloaterEditExtDayCycle::onRegionChange, this));
// 	env_mgr.setRegionSettingsAppliedCallback(boost::bind(&LLFloaterEditExtDayCycle::onRegionSettingsApplied, this, _1));
// 	// Connect to day cycle manager events.
// 	LLDayCycleManager::instance().setModifyCallback(boost::bind(&LLFloaterEditExtDayCycle::onDayCycleListChange, this));
// 
// 	// Connect to sky preset list changes.
// 	LLWLParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEditExtDayCycle::onSkyPresetListChange, this));
// 
// 
// 	// Connect to region info updates.
// 	LLRegionInfoModel::instance().setUpdateCallback(boost::bind(&LLFloaterEditExtDayCycle::onRegionInfoUpdate, this));
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::syncTimeSlider()
// {
// #if 0
// 	// set time
// 	mTimeSlider->setCurSliderValue((F32)LLWLParamManager::getInstance()->mAnimator.getDayTime() * sHoursPerDay);
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::loadTrack()
// {
// 	// clear the slider
// 	mKeysSlider->clear();
// 	mSliderToKey.clear();
// 
// 	// add sliders
// 
// 	LL_DEBUGS() << "Adding " << LLWLParamManager::getInstance()->mDay.mTimeMap.size() << " keys to slider" << LL_ENDL;
// 
// 	LLWLDayCycle& cur_dayp = LLWLParamManager::instance().mDay;
// 	for (std::map<F32, LLWLParamKey>::iterator it = cur_dayp.mTimeMap.begin(); it != cur_dayp.mTimeMap.end(); ++it)
// 	{
// 		addSliderKey(it->first * sHoursPerDay, it->second);
// 	}
// 
// 	// set drop-down menu to match preset of currently-selected keyframe (one is automatically selected initially)
// 	const std::string& cur_sldr = mKeysSlider->getCurSlider();
// 	if (strlen(cur_sldr.c_str()) > 0)	// only do this if there is a curSldr, otherwise we put an invalid entry into the map
// 	{
// 		mSkyPresetsCombo->selectByValue(mSliderToKey[cur_sldr].keyframe.toStringVal());
// 	}
// 
// 	syncTimeSlider();
// }
// 
// void LLFloaterEditExtDayCycle::applyTrack()
// {
// #if 0
// 	LL_DEBUGS() << "Applying track (" << mSliderToKey.size() << ")" << LL_ENDL;
// 
// 	// if no keys, do nothing
// 	if (mSliderToKey.size() == 0)
// 	{
// 		LL_DEBUGS() << "No keys, not syncing" << LL_ENDL;
// 		return;
// 	}
// 
// 	llassert_always(mSliderToKey.size() == mKeysSlider->getValue().size());
// 
// 	// create a new animation track
// 	LLWLParamManager::getInstance()->mDay.clearKeyframes();
// 
// 	// add the keys one by one
// 	for (std::map<std::string, SliderKey>::iterator it = mSliderToKey.begin();
// 		it != mSliderToKey.end(); ++it)
// 	{
// 		LLWLParamManager::getInstance()->mDay.addKeyframe(it->second.time / sHoursPerDay,
// 			it->second.keyframe);
// 	}
// 
// 	// set the param manager's track to the new one
// 	LLWLParamManager::getInstance()->resetAnimator(
// 		mTimeSlider->getCurSliderValue() / sHoursPerDay, false);
// 
// 	LLWLParamManager::getInstance()->mAnimator.update(
// 		LLWLParamManager::getInstance()->mCurParams);
// #endif
// }

// void LLFloaterEditExtDayCycle::refreshDayCyclesList()
// {
// #if 0
// 	llassert(isNewDay() == false);
// 
// 	mDayCyclesCombo->removeall();
// 
// #if 0 // Disable editing existing day cycle until the workflow is clear enough.
// 	const LLSD& region_day = LLEnvManagerNew::instance().getRegionSettings().getWLDayCycle();
// 	if (region_day.size() > 0)
// 	{
// 		LLWLParamKey key(getRegionName(), LLEnvKey::SCOPE_REGION);
// 		mDayCyclesCombo->add(key.name, key.toLLSD());
// 		mDayCyclesCombo->addSeparator();
// 	}
// #endif
// 
// 	LLDayCycleManager::preset_name_list_t user_days, sys_days;
// 	LLDayCycleManager::instance().getPresetNames(user_days, sys_days);
// 
// 	// Add user days.
// 	for (LLDayCycleManager::preset_name_list_t::const_iterator it = user_days.begin(); it != user_days.end(); ++it)
// 	{
// 		mDayCyclesCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toLLSD());
// 	}
// 
// 	if (user_days.size() > 0)
// 	{
// 		mDayCyclesCombo->addSeparator();
// 	}
// 
// 	// Add system days.
// 	for (LLDayCycleManager::preset_name_list_t::const_iterator it = sys_days.begin(); it != sys_days.end(); ++it)
// 	{
// 		mDayCyclesCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toLLSD());
// 	}
// 
// 	mDayCyclesCombo->setLabel(getString("combo_label"));
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onTimeSliderMoved()
// {
// #if 0
// 	/// get the slider value
// 	F32 val = mTimeSlider->getCurSliderValue() / sHoursPerDay;
// 
// 	// set the value, turn off animation
// 	LLWLParamManager::getInstance()->mAnimator.setDayTime((F64)val);
// 	LLWLParamManager::getInstance()->mAnimator.deactivate();
// 
// 	// then call update once
// 	LLWLParamManager::getInstance()->mAnimator.update(
// 		LLWLParamManager::getInstance()->mCurParams);
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onKeyTimeMoved()
// {
// #if 0
// 	if (mKeysSlider->getValue().size() == 0)
// 	{
// 		return;
// 	}
// 
// 	// make sure we have a slider
// 	const std::string& cur_sldr = mKeysSlider->getCurSlider();
// 	if (cur_sldr == "")
// 	{
// 		return;
// 	}
// 
// 	F32 time24 = mKeysSlider->getCurSliderValue();
// 
// 	// check to see if a key exists
// 	LLWLParamKey key = mSliderToKey[cur_sldr].keyframe;
// 	LL_DEBUGS() << "Setting key time: " << time24 << LL_ENDL;
// 	mSliderToKey[cur_sldr].time = time24;
// 
// 	// if it exists, turn on check box
// 	mSkyPresetsCombo->selectByValue(key.toStringVal());
// 
// 	mTimeCtrl->setTime24(time24);
// 
// 	applyTrack();
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onKeyTimeChanged()
// {
// #if 0
// 	// if no keys, skipped
// 	if (mSliderToKey.size() == 0)
// 	{
// 		return;
// 	}
// 
// 	F32 time24 = mTimeCtrl->getTime24();
// 
// 	const std::string& cur_sldr = mKeysSlider->getCurSlider();
// 	mKeysSlider->setCurSliderValue(time24, TRUE);
// 	F32 time = mKeysSlider->getCurSliderValue() / sHoursPerDay;
// 
// 	// now set the key's time in the sliderToKey map
// 	LL_DEBUGS() << "Setting key time: " << time << LL_ENDL;
// 	mSliderToKey[cur_sldr].time = time;
// 
// 	applyTrack();
// #endif
// }
// 
// 
// void LLFloaterEditExtDayCycle::onAddKey()
// {
// #if 0
// 	llassert_always(mSliderToKey.size() == mKeysSlider->getValue().size());
// 
// 	S32 max_sliders;
// 	LLEnvKey::EScope scope = LLEnvKey::SCOPE_LOCAL; // *TODO: editing region day cycle
// 	switch (scope)
// 	{
// 		case LLEnvKey::SCOPE_LOCAL:
// 			max_sliders = 20; // *HACK this should be LLWLPacketScrubber::MAX_LOCAL_KEY_FRAMES;
// 			break;
// 		case LLEnvKey::SCOPE_REGION:
// 			max_sliders = 12; // *HACK this should be LLWLPacketScrubber::MAX_REGION_KEY_FRAMES;
// 			break;
// 		default:
// 			max_sliders = (S32) mKeysSlider->getMaxValue();
// 			break;
// 	}
// 
// #if 0
// 	if ((S32)mSliderToKey.size() >= max_sliders)
// 	{
// 		LLSD args;
// 		args["SCOPE"] = LLEnvManagerNew::getScopeString(scope);
// 		args["MAX"] = max_sliders;
// 		LLNotificationsUtil::add("DayCycleTooManyKeyframes", args, LLSD(), LLNotificationFunctorRegistry::instance().DONOTHING);
// 		return;
// 	}
// #endif
// 
// 	// add the slider key
// 	std::string key_val = mSkyPresetsCombo->getSelectedValue().asString();
// 	LLWLParamKey sky_params(key_val);
// 	llassert(!sky_params.name.empty());
// 
// 	F32 time = mTimeSlider->getCurSliderValue();
// 	addSliderKey(time, sky_params);
// 
// 	// apply the change to current day cycles
// 	applyTrack();
// #endif
// }
// 
// #if 0
// void LLFloaterEditExtDayCycle::addSliderKey(F32 time, LLWLParamKey keyframe)
// {
// 	// make a slider
// 	const std::string& sldr_name = mKeysSlider->addSlider(time);
// 	if (sldr_name.empty())
// 	{
// 		return;
// 	}
// 
// 	// set the key
// 	SliderKey newKey(keyframe, mKeysSlider->getCurSliderValue());
// 
// 	llassert_always(sldr_name != LLStringUtil::null);
// 
// 	// add to map
// 	mSliderToKey.insert(std::pair<std::string, SliderKey>(sldr_name, newKey));
// 
// 	llassert_always(mSliderToKey.size() == mKeysSlider->getValue().size());
// }
// #endif
// 
// #if 0
// LLWLParamKey LLFloaterEditExtDayCycle::getSelectedDayCycle()
// {
// 	LLWLParamKey dc_key;
// 
// 	if (mDayCycleNameEditor->getVisible())
// 	{
// 		dc_key.name = mDayCycleNameEditor->getText();
// 		dc_key.scope = LLEnvKey::SCOPE_LOCAL;
// 	}
// 	else
// 	{
// 		LLSD combo_val = mDayCyclesCombo->getValue();
// 
// 		if (!combo_val.isArray()) // manually typed text
// 		{
// 			dc_key.name = combo_val.asString();
// 			dc_key.scope = LLEnvKey::SCOPE_LOCAL;
// 		}
// 		else
// 		{
// 			dc_key.fromLLSD(combo_val);
// 		}
// 	}
// 
// 	return dc_key;
// }
// #endif
// 
// bool LLFloaterEditExtDayCycle::isNewDay() const
// {
// 	return mKey.asString() == "new";
// }
// 
// void LLFloaterEditExtDayCycle::dumpTrack()
// {
// #if 0
// 	LL_DEBUGS("Windlight") << "Dumping day cycle" << LL_ENDL;
// 
// 	LLWLDayCycle& cur_dayp = LLWLParamManager::instance().mDay;
// 	for (std::map<F32, LLWLParamKey>::iterator it = cur_dayp.mTimeMap.begin(); it != cur_dayp.mTimeMap.end(); ++it)
// 	{
// 		F32 time = it->first * 24.0f;
// 		S32 h = (S32) time;
// 		S32 m = (S32) ((time - h) * 60.0f);
// 		LL_DEBUGS("Windlight") << llformat("(%.3f) %02d:%02d", time, h, m) << " => " << it->second.name << LL_ENDL;
// 	}
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::enableEditing(bool enable)
// {
// 	mSkyPresetsCombo->setEnabled(enable);
// 	mTimeCtrl->setEnabled(enable);
// 	getChild<LLPanel>("day_cycle_slider_panel")->setCtrlsEnabled(enable);
// 	mSaveButton->setEnabled(enable);
// 	mMakeDefaultCheckBox->setEnabled(enable);
// }
// 
// void LLFloaterEditExtDayCycle::reset()
// {
// #if 0
// 	// clear the slider
// 	mKeysSlider->clear();
// 	mSliderToKey.clear();
// 
// 	refreshSkyPresetsList();
// 
// 	if (isNewDay())
// 	{
// 		mDayCycleNameEditor->setValue(LLSD());
// 		F32 time = 0.5f * sHoursPerDay;
// 		mSaveButton->setEnabled(FALSE); // will be enabled as soon as users enters a name
// 		mTimeSlider->setCurSliderValue(time);
// 
// 		addSliderKey(time, LLWLParamKey("Default", LLEnvKey::SCOPE_LOCAL));
// 		onKeyTimeMoved(); // update the time control and sky sky combo
// 
// 		applyTrack();
// 	}
// 	else
// 	{
// 		refreshDayCyclesList();
// 
// 		// Disable controls until a day cycle  to edit is selected.
// 		enableEditing(false);
// 	}
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::saveRegionDayCycle()
// {
// #if 0
// 	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
// 	LLWLDayCycle& cur_dayp = LLWLParamManager::instance().mDay; // the day cycle being edited
// 
// 	// Get current day cycle and the sky preset it references.
// 	LLSD day_cycle = cur_dayp.asLLSD();
// 	LLSD sky_map;
// 	cur_dayp.getSkyMap(sky_map);
// 
// 	// Apply it to the region.
// 	LLEnvironmentSettings new_region_settings;
// 	new_region_settings.saveParams(day_cycle, sky_map, env_mgr.getRegionSettings().getWaterParams(), 0.0f);
// 
// #if 1
// 	LLEnvManagerNew::instance().setRegionSettings(new_region_settings);
// #else // Temporary disabled ability to upload new region settings from the Day Cycle Editor.
// 	if (!LLEnvManagerNew::instance().sendRegionSettings(new_region_settings))
// 	{
// 		LL_WARNS() << "Error applying region environment settings" << LL_ENDL;
// 		return;
// 	}
// 
// 	setApplyProgress(true);
// #endif
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::setApplyProgress(bool started)
// {
// 	LLLoadingIndicator* indicator = getChild<LLLoadingIndicator>("progress_indicator");
// 
// 	indicator->setVisible(started);
// 
// 	if (started)
// 	{
// 		indicator->start();
// 	}
// 	else
// 	{
// 		indicator->stop();
// 	}
// }
// 
// bool LLFloaterEditExtDayCycle::getApplyProgress() const
// {
// 	return getChild<LLLoadingIndicator>("progress_indicator")->getVisible();
// }
// 
// void LLFloaterEditExtDayCycle::onDeleteKey()
// {
// #if 0
// 	if (mSliderToKey.size() == 0)
// 	{
// 		return;
// 	}
// 	else if (mSliderToKey.size() == 1)
// 	{
// 		LLNotifications::instance().add("EnvCannotDeleteLastDayCycleKey", LLSD(), LLSD());
// 		return;
// 	}
// 
// 	// delete from map
// 	const std::string& sldr_name = mKeysSlider->getCurSlider();
// 	std::map<std::string, SliderKey>::iterator mIt = mSliderToKey.find(sldr_name);
// 	mSliderToKey.erase(mIt);
// 
// 	mKeysSlider->deleteCurSlider();
// 
// 	if (mSliderToKey.size() == 0)
// 	{
// 		return;
// 	}
// 
// 	const std::string& name = mKeysSlider->getCurSlider();
// 	mSkyPresetsCombo->selectByValue(mSliderToKey[name].keyframe.toStringVal());
// 	F32 time24 = mSliderToKey[name].time;
// 
// 	mTimeCtrl->setTime24(time24);
// 
// 	applyTrack();
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onRegionSettingsChange()
// {
// #if 0
// 	LL_DEBUGS("Windlight") << "Region settings changed" << LL_ENDL;
// 
// 	if (getApplyProgress()) // our region settings have being applied
// 	{
// 		setApplyProgress(false);
// 
// 		// Change preference if requested.
// 		if (mMakeDefaultCheckBox->getValue())
// 		{
// 			LL_DEBUGS("Windlight") << "Changed environment preference to region settings" << LL_ENDL;
// 			LLEnvManagerNew::instance().setUseRegionSettings(true);
// 		}
// 
// 		closeFloater();
// 	}
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onRegionChange()
// {
// #if 0
// 	LL_DEBUGS("Windlight") << "Region changed" << LL_ENDL;
// 
// 	// If we're editing the region day cycle
// 	if (getSelectedDayCycle().scope == LLEnvKey::SCOPE_REGION)
// 	{
// 		reset(); // undoes all unsaved changes
// 	}
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onRegionSettingsApplied(bool success)
// {
// 	LL_DEBUGS("Windlight") << "Region settings applied: " << success << LL_ENDL;
// 
// 	if (!success)
// 	{
// 		// stop progress indicator
// 		setApplyProgress(false);
// 	}
// }
// 
// void LLFloaterEditExtDayCycle::onRegionInfoUpdate()
// {
// #if 0
// 	LL_DEBUGS("Windlight") << "Region info updated" << LL_ENDL;
// 	bool can_edit = true;
// 
// 	// If we've selected the region day cycle for editing.
// 	if (getSelectedDayCycle().scope == LLEnvKey::SCOPE_REGION)
// 	{
// 		// check whether we have the access
// 		can_edit = LLEnvManagerNew::canEditRegionSettings();
// 	}
// 
// 	enableEditing(can_edit);
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onDayCycleNameEdited()
// {
// #if 0
// 	// Disable saving a day cycle having empty name.
// 	LLWLParamKey key = getSelectedDayCycle();
// 	mSaveButton->setEnabled(!key.name.empty());
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onDayCycleSelected()
// {
// #if 0
// 
// 	LLSD day_data;
// 	LLWLParamKey dc_key = getSelectedDayCycle();
// 	bool can_edit = true;
// 
// 	if (dc_key.scope == LLEnvKey::SCOPE_LOCAL)
// 	{
// 		if (!LLDayCycleManager::instance().getPreset(dc_key.name, day_data))
// 		{
// 			LL_WARNS() << "No day cycle named " << dc_key.name << LL_ENDL;
// 			return;
// 		}
// 	}
// 	else
// 	{
// 		day_data = LLEnvManagerNew::instance().getRegionSettings().getWLDayCycle();
// 		if (day_data.size() == 0)
// 		{
// 			LL_WARNS() << "Empty region day cycle" << LL_ENDL;
// 			llassert(day_data.size() > 0);
// 			return;
// 		}
// 
// 		can_edit = LLEnvManagerNew::canEditRegionSettings();
// 	}
// 
// 	// We may need to add or remove region skies from the list.
// 	refreshSkyPresetsList();
// 
// 	F32 slider_time = mTimeSlider->getCurSliderValue() / sHoursPerDay;
// 	LLWLParamManager::instance().applyDayCycleParams(day_data, dc_key.scope, slider_time);
// 	loadTrack();
// #endif
// 	enableEditing(false);
// }
// 
// bool LLFloaterEditExtDayCycle::onSaveAnswer(const LLSD& notification, const LLSD& response)
// {
// 	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
// 
// 	// If they choose save, do it.  Otherwise, don't do anything
// 	if (option == 0)
// 	{
// 		onSaveConfirmed();
// 	}
// 
// 	return false;
// }
// 
// void LLFloaterEditExtDayCycle::onSaveConfirmed()
// {
// #if 0
// 	std::string name = getSelectedDayCycle().name;
// 
// 	// Save preset.
// 	LLSD data = LLWLParamManager::instance().mDay.asLLSD();
// 	LL_DEBUGS("Windlight") << "Saving day cycle " << name << ": " << data << LL_ENDL;
// 	LLDayCycleManager::instance().savePreset(name, data);
// 
// 	// Change preference if requested.
// 	if (mMakeDefaultCheckBox->getValue())
// 	{
// 		LL_DEBUGS("Windlight") << name << " is now the new preferred day cycle" << LL_ENDL;
// 		LLEnvManagerNew::instance().setUseDayCycle(name);
// 	}
// #endif
// 	closeFloater();
// }
// 
// void LLFloaterEditExtDayCycle::onDayCycleListChange()
// {
// 	if (!isNewDay())
// 	{
// 		refreshDayCyclesList();
// 	}
// }
// 
// void LLFloaterEditExtDayCycle::onSkyPresetListChange()
// {
// 	refreshSkyPresetsList();
// 
// 	// Refresh sliders from the currently visible day cycle.
// 	loadTrack();
// }
// 
// static
// std::string LLFloaterEditExtDayCycle::getRegionName()
// {
// 	return gAgent.getRegion() ? gAgent.getRegion()->getName() : LLTrans::getString("Unknown");
// }
