/** 
 * @file llfloatereditdaycycle.cpp
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

#include "llfloatereditdaycycle.h"

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
#include "lldaycyclemanager.h"
#include "llenvmanager.h"
#include "llregioninfomodel.h"
#include "llviewerregion.h"
#include "llwlparammanager.h"

const F32 LLFloaterEditDayCycle::sHoursPerDay = 24.0f;

LLFloaterEditDayCycle::LLFloaterEditDayCycle(const LLSD &key)
:	LLFloater(key)
,	mDayCycleNameEditor(NULL)
,	mDayCyclesCombo(NULL)
,	mTimeSlider(NULL)
,	mKeysSlider(NULL)
,	mSkyPresetsCombo(NULL)
,	mTimeCtrl(NULL)
,	mMakeDefaultCheckBox(NULL)
,	mSaveButton(NULL)
{
}

// virtual
BOOL LLFloaterEditDayCycle::postBuild()
{
	mDayCycleNameEditor = getChild<LLLineEditor>("day_cycle_name");
	mDayCyclesCombo = getChild<LLComboBox>("day_cycle_combo");

	mTimeSlider = getChild<LLMultiSliderCtrl>("WLTimeSlider");
	mKeysSlider = getChild<LLMultiSliderCtrl>("WLDayCycleKeys");
	mSkyPresetsCombo = getChild<LLComboBox>("WLSkyPresets");
	mTimeCtrl = getChild<LLTimeCtrl>("time");
	mSaveButton = getChild<LLButton>("save");
	mMakeDefaultCheckBox = getChild<LLCheckBoxCtrl>("make_default_cb");

	initCallbacks();

	// add the time slider
	mTimeSlider->addSlider();

	return TRUE;
}

// virtual
void LLFloaterEditDayCycle::onOpen(const LLSD& key)
{
	bool new_day = isNewDay();
	std::string param = key.asString();
	std::string floater_title = getString(std::string("title_") + param);
	std::string hint = getString(std::string("hint_" + param));

	// Update floater title.
	setTitle(floater_title);

	// Update the hint at the top.
	getChild<LLUICtrl>("hint")->setValue(hint);

	// Hide the hint to the right of the combo if we're invoked to create a new preset.
	getChildView("note")->setVisible(!new_day);

	// Switch between the day cycle presets combobox and day cycle name input field.
	mDayCyclesCombo->setVisible(!new_day);
	mDayCycleNameEditor->setVisible(new_day);

	// TODO: Make sure only one instance of the floater exists?

	reset();
}

// virtual
void LLFloaterEditDayCycle::onClose(bool app_quitting)
{
	if (!app_quitting) // there's no point to change environment if we're quitting
	{
		LLEnvManagerNew::instance().usePrefs(); // revert changes made to current day cycle
	}
}

// virtual
void LLFloaterEditDayCycle::draw()
{
	syncTimeSlider();
	LLFloater::draw();
}

void LLFloaterEditDayCycle::initCallbacks(void)
{
	mDayCycleNameEditor->setKeystrokeCallback(boost::bind(&LLFloaterEditDayCycle::onDayCycleNameEdited, this), NULL);
	mDayCyclesCombo->setCommitCallback(boost::bind(&LLFloaterEditDayCycle::onDayCycleSelected, this));
	mDayCyclesCombo->setTextEntryCallback(boost::bind(&LLFloaterEditDayCycle::onDayCycleNameEdited, this));
	mTimeSlider->setCommitCallback(boost::bind(&LLFloaterEditDayCycle::onTimeSliderMoved, this));
	mKeysSlider->setCommitCallback(boost::bind(&LLFloaterEditDayCycle::onKeyTimeMoved, this));
	mTimeCtrl->setCommitCallback(boost::bind(&LLFloaterEditDayCycle::onKeyTimeChanged, this));
	mSkyPresetsCombo->setCommitCallback(boost::bind(&LLFloaterEditDayCycle::onKeyPresetChanged, this));

	getChild<LLButton>("WLAddKey")->setClickedCallback(boost::bind(&LLFloaterEditDayCycle::onAddKey, this));
	getChild<LLButton>("WLDeleteKey")->setClickedCallback(boost::bind(&LLFloaterEditDayCycle::onDeleteKey, this));

	mSaveButton->setCommitCallback(boost::bind(&LLFloaterEditDayCycle::onBtnSave, this));
	mSaveButton->setRightMouseDownCallback(boost::bind(&LLFloaterEditDayCycle::dumpTrack, this));
	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterEditDayCycle::onBtnCancel, this));

	// Connect to env manager events.
	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
	env_mgr.setRegionSettingsChangeCallback(boost::bind(&LLFloaterEditDayCycle::onRegionSettingsChange, this));
	env_mgr.setRegionChangeCallback(boost::bind(&LLFloaterEditDayCycle::onRegionChange, this));
	env_mgr.setRegionSettingsAppliedCallback(boost::bind(&LLFloaterEditDayCycle::onRegionSettingsApplied, this, _1));

	// Connect to day cycle manager events.
	LLDayCycleManager::instance().setModifyCallback(boost::bind(&LLFloaterEditDayCycle::onDayCycleListChange, this));

	// Connect to sky preset list changes.
	LLWLParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEditDayCycle::onSkyPresetListChange, this));

	// Connect to region info updates.
	LLRegionInfoModel::instance().setUpdateCallback(boost::bind(&LLFloaterEditDayCycle::onRegionInfoUpdate, this));
}

void LLFloaterEditDayCycle::syncTimeSlider()
{
	// set time
	mTimeSlider->setCurSliderValue((F32)LLWLParamManager::getInstance()->mAnimator.getDayTime() * sHoursPerDay);
}

void LLFloaterEditDayCycle::loadTrack()
{
	// clear the slider
	mKeysSlider->clear();
	mSliderToKey.clear();

	// add sliders

	lldebugs << "Adding " << LLWLParamManager::getInstance()->mDay.mTimeMap.size() << " keys to slider" << llendl;

	LLWLDayCycle& cur_dayp = LLWLParamManager::instance().mDay;
	for (std::map<F32, LLWLParamKey>::iterator it = cur_dayp.mTimeMap.begin(); it != cur_dayp.mTimeMap.end(); ++it)
	{
		addSliderKey(it->first * sHoursPerDay, it->second);
	}

	// set drop-down menu to match preset of currently-selected keyframe (one is automatically selected initially)
	const std::string& cur_sldr = mKeysSlider->getCurSlider();
	if (strlen(cur_sldr.c_str()) > 0)	// only do this if there is a curSldr, otherwise we put an invalid entry into the map
	{
		mSkyPresetsCombo->selectByValue(mSliderToKey[cur_sldr].keyframe.toStringVal());
	}

	syncTimeSlider();
}

void LLFloaterEditDayCycle::applyTrack()
{
	lldebugs << "Applying track (" << mSliderToKey.size() << ")" << llendl;

	// if no keys, do nothing
	if (mSliderToKey.size() == 0)
	{
		lldebugs << "No keys, not syncing" << llendl;
		return;
	}

	llassert_always(mSliderToKey.size() == mKeysSlider->getValue().size());

	// create a new animation track
	LLWLParamManager::getInstance()->mDay.clearKeyframes();

	// add the keys one by one
	for (std::map<std::string, SliderKey>::iterator it = mSliderToKey.begin();
		it != mSliderToKey.end(); ++it)
	{
		LLWLParamManager::getInstance()->mDay.addKeyframe(it->second.time / sHoursPerDay,
			it->second.keyframe);
	}

	// set the param manager's track to the new one
	LLWLParamManager::getInstance()->resetAnimator(
		mTimeSlider->getCurSliderValue() / sHoursPerDay, false);

	LLWLParamManager::getInstance()->mAnimator.update(
		LLWLParamManager::getInstance()->mCurParams);
}

void LLFloaterEditDayCycle::refreshSkyPresetsList()
{
	// Don't allow selecting region skies for a local day cycle,
	// because thus we may end up with invalid day cycle.
	bool include_region_skies = getSelectedDayCycle().scope == LLEnvKey::SCOPE_REGION;

	mSkyPresetsCombo->removeall();

	LLWLParamManager::preset_name_list_t region_presets;
	LLWLParamManager::preset_name_list_t user_presets, sys_presets;
	LLWLParamManager::instance().getPresetNames(region_presets, user_presets, sys_presets);

	if (include_region_skies)
	{
		// Add region presets.
		for (LLWLParamManager::preset_name_list_t::const_iterator it = region_presets.begin(); it != region_presets.end(); ++it)
		{
			std::string preset_name = *it;
			std::string item_title = preset_name + " (" + getRegionName() + ")";
			mSkyPresetsCombo->add(preset_name, LLWLParamKey(*it, LLEnvKey::SCOPE_REGION).toStringVal());
		}

		if (!region_presets.empty())
		{
			mSkyPresetsCombo->addSeparator();
		}
	}

	// Add user presets.
	for (LLWLParamManager::preset_name_list_t::const_iterator it = user_presets.begin(); it != user_presets.end(); ++it)
	{
		mSkyPresetsCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toStringVal());
	}

	if (!user_presets.empty())
	{
		mSkyPresetsCombo->addSeparator();
	}

	// Add system presets.
	for (LLWLParamManager::preset_name_list_t::const_iterator it = sys_presets.begin(); it != sys_presets.end(); ++it)
	{
		mSkyPresetsCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toStringVal());
	}

	// set defaults on combo boxes
	mSkyPresetsCombo->selectFirstItem();
}

void LLFloaterEditDayCycle::refreshDayCyclesList()
{
	llassert(isNewDay() == false);

	mDayCyclesCombo->removeall();

#if 0 // Disable editing existing day cycle until the workflow is clear enough.
	const LLSD& region_day = LLEnvManagerNew::instance().getRegionSettings().getWLDayCycle();
	if (region_day.size() > 0)
	{
		LLWLParamKey key(getRegionName(), LLEnvKey::SCOPE_REGION);
		mDayCyclesCombo->add(key.name, key.toLLSD());
		mDayCyclesCombo->addSeparator();
	}
#endif

	LLDayCycleManager::preset_name_list_t user_days, sys_days;
	LLDayCycleManager::instance().getPresetNames(user_days, sys_days);

	// Add user days.
	for (LLDayCycleManager::preset_name_list_t::const_iterator it = user_days.begin(); it != user_days.end(); ++it)
	{
		mDayCyclesCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toLLSD());
	}

	if (user_days.size() > 0)
	{
		mDayCyclesCombo->addSeparator();
	}

	// Add system days.
	for (LLDayCycleManager::preset_name_list_t::const_iterator it = sys_days.begin(); it != sys_days.end(); ++it)
	{
		mDayCyclesCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toLLSD());
	}

	mDayCyclesCombo->setLabel(getString("combo_label"));
}

void LLFloaterEditDayCycle::onTimeSliderMoved()
{
	/// get the slider value
	F32 val = mTimeSlider->getCurSliderValue() / sHoursPerDay;

	// set the value, turn off animation
	LLWLParamManager::getInstance()->mAnimator.setDayTime((F64)val);
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	// then call update once
	LLWLParamManager::getInstance()->mAnimator.update(
		LLWLParamManager::getInstance()->mCurParams);
}

void LLFloaterEditDayCycle::onKeyTimeMoved()
{
	if (mKeysSlider->getValue().size() == 0)
	{
		return;
	}

	// make sure we have a slider
	const std::string& cur_sldr = mKeysSlider->getCurSlider();
	if (cur_sldr == "")
	{
		return;
	}

	F32 time24 = mKeysSlider->getCurSliderValue();

	// check to see if a key exists
	LLWLParamKey key = mSliderToKey[cur_sldr].keyframe;
	lldebugs << "Setting key time: " << time24 << LL_ENDL;
	mSliderToKey[cur_sldr].time = time24;

	// if it exists, turn on check box
	mSkyPresetsCombo->selectByValue(key.toStringVal());

	mTimeCtrl->setTime24(time24);

	applyTrack();
}

void LLFloaterEditDayCycle::onKeyTimeChanged()
{
	// if no keys, skipped
	if (mSliderToKey.size() == 0)
	{
		return;
	}

	F32 time24 = mTimeCtrl->getTime24();

	const std::string& cur_sldr = mKeysSlider->getCurSlider();
	mKeysSlider->setCurSliderValue(time24, TRUE);
	F32 time = mKeysSlider->getCurSliderValue() / sHoursPerDay;

	// now set the key's time in the sliderToKey map
	lldebugs << "Setting key time: " << time << LL_ENDL;
	mSliderToKey[cur_sldr].time = time;

	applyTrack();
}

void LLFloaterEditDayCycle::onKeyPresetChanged()
{
	// do nothing if no sliders
	if (mKeysSlider->getValue().size() == 0)
	{
		return;
	}

	// change the map

	std::string stringVal = mSkyPresetsCombo->getSelectedValue().asString();
	LLWLParamKey new_key(stringVal);
	llassert(!new_key.name.empty());
	const std::string& cur_sldr = mKeysSlider->getCurSlider();

	// if null, don't use
	if (cur_sldr == "")
	{
		return;
	}

	mSliderToKey[cur_sldr].keyframe = new_key;

	// Apply changes to current day cycle.
	applyTrack();
}

void LLFloaterEditDayCycle::onAddKey()
{
	llassert_always(mSliderToKey.size() == mKeysSlider->getValue().size());

	S32 max_sliders;
	LLEnvKey::EScope scope = LLEnvKey::SCOPE_LOCAL; // *TODO: editing region day cycle
	switch (scope)
	{
		case LLEnvKey::SCOPE_LOCAL:
			max_sliders = 20; // *HACK this should be LLWLPacketScrubber::MAX_LOCAL_KEY_FRAMES;
			break;
		case LLEnvKey::SCOPE_REGION:
			max_sliders = 12; // *HACK this should be LLWLPacketScrubber::MAX_REGION_KEY_FRAMES;
			break;
		default:
			max_sliders = (S32) mKeysSlider->getMaxValue();
			break;
	}

	if ((S32)mSliderToKey.size() >= max_sliders)
	{
		LLSD args;
		args["SCOPE"] = LLEnvManagerNew::getScopeString(scope);
		args["MAX"] = max_sliders;
		LLNotificationsUtil::add("DayCycleTooManyKeyframes", args, LLSD(), LLNotificationFunctorRegistry::instance().DONOTHING);
		return;
	}

	// add the slider key
	std::string key_val = mSkyPresetsCombo->getSelectedValue().asString();
	LLWLParamKey sky_params(key_val);
	llassert(!sky_params.name.empty());

	F32 time = mTimeSlider->getCurSliderValue();
	addSliderKey(time, sky_params);

	// apply the change to current day cycles
	applyTrack();
}

void LLFloaterEditDayCycle::addSliderKey(F32 time, LLWLParamKey keyframe)
{
	// make a slider
	const std::string& sldr_name = mKeysSlider->addSlider(time);
	if (sldr_name.empty())
	{
		return;
	}

	// set the key
	SliderKey newKey(keyframe, mKeysSlider->getCurSliderValue());

	llassert_always(sldr_name != LLStringUtil::null);

	// add to map
	mSliderToKey.insert(std::pair<std::string, SliderKey>(sldr_name, newKey));

	llassert_always(mSliderToKey.size() == mKeysSlider->getValue().size());
}

LLWLParamKey LLFloaterEditDayCycle::getSelectedDayCycle()
{
	LLWLParamKey dc_key;

	if (mDayCycleNameEditor->getVisible())
	{
		dc_key.name = mDayCycleNameEditor->getText();
		dc_key.scope = LLEnvKey::SCOPE_LOCAL;
	}
	else
	{
		LLSD combo_val = mDayCyclesCombo->getValue();

		if (!combo_val.isArray()) // manually typed text
		{
			dc_key.name = combo_val.asString();
			dc_key.scope = LLEnvKey::SCOPE_LOCAL;
		}
		else
		{
			dc_key.fromLLSD(combo_val);
		}
	}

	return dc_key;
}

bool LLFloaterEditDayCycle::isNewDay() const
{
	return mKey.asString() == "new";
}

void LLFloaterEditDayCycle::dumpTrack()
{
	LL_DEBUGS("Windlight") << "Dumping day cycle" << LL_ENDL;

	LLWLDayCycle& cur_dayp = LLWLParamManager::instance().mDay;
	for (std::map<F32, LLWLParamKey>::iterator it = cur_dayp.mTimeMap.begin(); it != cur_dayp.mTimeMap.end(); ++it)
	{
		F32 time = it->first * 24.0f;
		S32 h = (S32) time;
		S32 m = (S32) ((time - h) * 60.0f);
		LL_DEBUGS("Windlight") << llformat("(%.3f) %02d:%02d", time, h, m) << " => " << it->second.name << LL_ENDL;
	}
}

void LLFloaterEditDayCycle::enableEditing(bool enable)
{
	mSkyPresetsCombo->setEnabled(enable);
	mTimeCtrl->setEnabled(enable);
	getChild<LLPanel>("day_cycle_slider_panel")->setCtrlsEnabled(enable);
	mSaveButton->setEnabled(enable);
	mMakeDefaultCheckBox->setEnabled(enable);
}

void LLFloaterEditDayCycle::reset()
{
	// clear the slider
	mKeysSlider->clear();
	mSliderToKey.clear();

	refreshSkyPresetsList();

	if (isNewDay())
	{
		mDayCycleNameEditor->setValue(LLSD());
		F32 time = 0.5f * sHoursPerDay;
		mSaveButton->setEnabled(FALSE); // will be enabled as soon as users enters a name
		mTimeSlider->setCurSliderValue(time);

		addSliderKey(time, LLWLParamKey("Default", LLEnvKey::SCOPE_LOCAL));
		onKeyTimeMoved(); // update the time control and sky sky combo

		applyTrack();
	}
	else
	{
		refreshDayCyclesList();

		// Disable controls until a day cycle  to edit is selected.
		enableEditing(false);
	}
}

void LLFloaterEditDayCycle::saveRegionDayCycle()
{
	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
	LLWLDayCycle& cur_dayp = LLWLParamManager::instance().mDay; // the day cycle being edited

	// Get current day cycle and the sky preset it references.
	LLSD day_cycle = cur_dayp.asLLSD();
	LLSD sky_map;
	cur_dayp.getSkyMap(sky_map);

	// Apply it to the region.
	LLEnvironmentSettings new_region_settings;
	new_region_settings.saveParams(day_cycle, sky_map, env_mgr.getRegionSettings().getWaterParams(), 0.0f);

#if 1
	LLEnvManagerNew::instance().setRegionSettings(new_region_settings);
#else // Temporary disabled ability to upload new region settings from the Day Cycle Editor.
	if (!LLEnvManagerNew::instance().sendRegionSettings(new_region_settings))
	{
		llwarns << "Error applying region environment settings" << llendl;
		return;
	}

	setApplyProgress(true);
#endif
}

void LLFloaterEditDayCycle::setApplyProgress(bool started)
{
	LLLoadingIndicator* indicator = getChild<LLLoadingIndicator>("progress_indicator");

	indicator->setVisible(started);

	if (started)
	{
		indicator->start();
	}
	else
	{
		indicator->stop();
	}
}

bool LLFloaterEditDayCycle::getApplyProgress() const
{
	return getChild<LLLoadingIndicator>("progress_indicator")->getVisible();
}

void LLFloaterEditDayCycle::onDeleteKey()
{
	if (mSliderToKey.size() == 0)
	{
		return;
	}
	else if (mSliderToKey.size() == 1)
	{
		LLNotifications::instance().add("EnvCannotDeleteLastDayCycleKey", LLSD(), LLSD());
		return;
	}

	// delete from map
	const std::string& sldr_name = mKeysSlider->getCurSlider();
	std::map<std::string, SliderKey>::iterator mIt = mSliderToKey.find(sldr_name);
	mSliderToKey.erase(mIt);

	mKeysSlider->deleteCurSlider();

	if (mSliderToKey.size() == 0)
	{
		return;
	}

	const std::string& name = mKeysSlider->getCurSlider();
	mSkyPresetsCombo->selectByValue(mSliderToKey[name].keyframe.toStringVal());
	F32 time24 = mSliderToKey[name].time;

	mTimeCtrl->setTime24(time24);

	applyTrack();
}

void LLFloaterEditDayCycle::onRegionSettingsChange()
{
	LL_DEBUGS("Windlight") << "Region settings changed" << LL_ENDL;

	if (getApplyProgress()) // our region settings have being applied
	{
		setApplyProgress(false);

		// Change preference if requested.
		if (mMakeDefaultCheckBox->getValue())
		{
			LL_DEBUGS("Windlight") << "Changed environment preference to region settings" << llendl;
			LLEnvManagerNew::instance().setUseRegionSettings(true);
		}

		closeFloater();
	}
}

void LLFloaterEditDayCycle::onRegionChange()
{
	LL_DEBUGS("Windlight") << "Region changed" << LL_ENDL;

	// If we're editing the region day cycle
	if (getSelectedDayCycle().scope == LLEnvKey::SCOPE_REGION)
	{
		reset(); // undoes all unsaved changes
	}
}

void LLFloaterEditDayCycle::onRegionSettingsApplied(bool success)
{
	LL_DEBUGS("Windlight") << "Region settings applied: " << success << LL_ENDL;

	if (!success)
	{
		// stop progress indicator
		setApplyProgress(false);
	}
}

void LLFloaterEditDayCycle::onRegionInfoUpdate()
{
	LL_DEBUGS("Windlight") << "Region info updated" << LL_ENDL;
	bool can_edit = true;

	// If we've selected the region day cycle for editing.
	if (getSelectedDayCycle().scope == LLEnvKey::SCOPE_REGION)
	{
		// check whether we have the access
		can_edit = LLEnvManagerNew::canEditRegionSettings();
	}

	enableEditing(can_edit);
}

void LLFloaterEditDayCycle::onDayCycleNameEdited()
{
	// Disable saving a day cycle having empty name.
	LLWLParamKey key = getSelectedDayCycle();
	mSaveButton->setEnabled(!key.name.empty());
}

void LLFloaterEditDayCycle::onDayCycleSelected()
{
	LLSD day_data;
	LLWLParamKey dc_key = getSelectedDayCycle();
	bool can_edit = true;

	if (dc_key.scope == LLEnvKey::SCOPE_LOCAL)
	{
		if (!LLDayCycleManager::instance().getPreset(dc_key.name, day_data))
		{
			llwarns << "No day cycle named " << dc_key.name << llendl;
			return;
		}
	}
	else
	{
		day_data = LLEnvManagerNew::instance().getRegionSettings().getWLDayCycle();
		if (day_data.size() == 0)
		{
			llwarns << "Empty region day cycle" << llendl;
			llassert(day_data.size() > 0);
			return;
		}

		can_edit = LLEnvManagerNew::canEditRegionSettings();
	}

	// We may need to add or remove region skies from the list.
	refreshSkyPresetsList();

	F32 slider_time = mTimeSlider->getCurSliderValue() / sHoursPerDay;
	LLWLParamManager::instance().applyDayCycleParams(day_data, dc_key.scope, slider_time);
	loadTrack();

	enableEditing(can_edit);
}

void LLFloaterEditDayCycle::onBtnSave()
{
	LLDayCycleManager& day_mgr = LLDayCycleManager::instance();
	LLWLParamKey selected_day = getSelectedDayCycle();

	if (selected_day.scope == LLEnvKey::SCOPE_REGION)
	{
		saveRegionDayCycle();
		closeFloater();
		return;
	}

	std::string name = selected_day.name;
	if (name.empty())
	{
		// *TODO: show an alert
		llwarns << "Empty day cycle name" << llendl;
		return;
	}

	// Don't allow overwriting system presets.
	if (day_mgr.isSystemPreset(name))
	{
		LLNotificationsUtil::add("WLNoEditDefault");
		return;
	}

	// Save, ask for confirmation for overwriting an existing preset.
	if (day_mgr.presetExists(name))
	{
		LLNotificationsUtil::add("WLSavePresetAlert", LLSD(), LLSD(), boost::bind(&LLFloaterEditDayCycle::onSaveAnswer, this, _1, _2));
	}
	else
	{
		// new preset, hence no confirmation needed
		onSaveConfirmed();
	}
}

void LLFloaterEditDayCycle::onBtnCancel()
{
	closeFloater();
}

bool LLFloaterEditDayCycle::onSaveAnswer(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	// If they choose save, do it.  Otherwise, don't do anything
	if (option == 0)
	{
		onSaveConfirmed();
	}

	return false;
}

void LLFloaterEditDayCycle::onSaveConfirmed()
{
	std::string name = getSelectedDayCycle().name;

	// Save preset.
	LLSD data = LLWLParamManager::instance().mDay.asLLSD();
	LL_DEBUGS("Windlight") << "Saving day cycle " << name << ": " << data << LL_ENDL;
	LLDayCycleManager::instance().savePreset(name, data);

	// Change preference if requested.
	if (mMakeDefaultCheckBox->getValue())
	{
		LL_DEBUGS("Windlight") << name << " is now the new preferred day cycle" << llendl;
		LLEnvManagerNew::instance().setUseDayCycle(name);
	}

	closeFloater();
}

void LLFloaterEditDayCycle::onDayCycleListChange()
{
	if (!isNewDay())
	{
		refreshDayCyclesList();
	}
}

void LLFloaterEditDayCycle::onSkyPresetListChange()
{
	refreshSkyPresetsList();

	// Refresh sliders from the currently visible day cycle.
	loadTrack();
}

// static
std::string LLFloaterEditDayCycle::getRegionName()
{
	return gAgent.getRegion() ? gAgent.getRegion()->getName() : LLTrans::getString("Unknown");
}
