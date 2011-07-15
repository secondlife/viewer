/** 
 * @file llfloatereditdaycycle.h
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

#ifndef LL_LLFLOATEREDITDAYCYCLE_H
#define LL_LLFLOATEREDITDAYCYCLE_H

#include "llfloater.h"

#include "llwlparammanager.h" // for LLWLParamKey

class LLCheckBoxCtrl;
class LLComboBox;
class LLLineEditor;
class LLMultiSliderCtrl;
class LLTimeCtrl;

/**
 * Floater for creating or editing a day cycle.
 */
class LLFloaterEditDayCycle : public LLFloater
{
	LOG_CLASS(LLFloaterEditDayCycle);

public:
	LLFloaterEditDayCycle(const LLSD &key);

	/*virtual*/	BOOL	postBuild();
	/*virtual*/ void	onOpen(const LLSD& key);
	/*virtual*/ void	onClose(bool app_quitting);
	/*virtual*/ void	draw();

private:

	/// sync the time slider with day cycle structure
	void syncTimeSlider();

	// 	makes sure key slider has what's in day cycle
	void loadTrack();

	/// makes sure day cycle data structure has what's in menu
	void applyTrack();

	/// refresh the sky presets combobox
	void refreshSkyPresetsList();

	/// refresh the day cycle combobox
	void refreshDayCyclesList();

	/// add a slider to the track
	void addSliderKey(F32 time, LLWLParamKey keyframe);

	void initCallbacks();
	LLWLParamKey getSelectedDayCycle();
	bool isNewDay() const;
	void dumpTrack();
	void enableEditing(bool enable);
	void reset();
	void saveRegionDayCycle();

	void setApplyProgress(bool started);
	bool getApplyProgress() const;

	void onTimeSliderMoved();	/// time slider moved
	void onKeyTimeMoved();		/// a key frame moved
	void onKeyTimeChanged();	/// a key frame's time changed
	void onKeyPresetChanged();	/// sky preset selected
	void onAddKey();			/// new key added on slider
	void onDeleteKey();			/// a key frame deleted

	void onRegionSettingsChange();
	void onRegionChange();
	void onRegionSettingsApplied(bool success);
	void onRegionInfoUpdate();

	void onDayCycleNameEdited();
	void onDayCycleSelected();
	void onBtnSave();
	void onBtnCancel();

	bool onSaveAnswer(const LLSD& notification, const LLSD& response);
	void onSaveConfirmed();

	void onDayCycleListChange();
	void onSkyPresetListChange();

	static std::string getRegionName();

	/// convenience class for holding keyframes mapped to sliders
	struct SliderKey
	{
	public:
		SliderKey(LLWLParamKey kf, F32 t) : keyframe(kf), time(t) {}
		SliderKey() : keyframe(), time(0.f) {} // Don't use this default constructor

		LLWLParamKey keyframe;
		F32 time;
	};

	static const F32 sHoursPerDay;

	LLLineEditor*		mDayCycleNameEditor;
	LLComboBox*			mDayCyclesCombo;
	LLMultiSliderCtrl*	mTimeSlider;
	LLMultiSliderCtrl*	mKeysSlider;
	LLComboBox*			mSkyPresetsCombo;
	LLTimeCtrl*			mTimeCtrl;
	LLCheckBoxCtrl*		mMakeDefaultCheckBox;
	LLButton*			mSaveButton;

	// map of sliders to parameters
	std::map<std::string, SliderKey> mSliderToKey;
};

#endif // LL_LLFLOATEREDITDAYCYCLE_H
