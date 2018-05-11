/** 
 * @file llfloatereditextdaycycle.h
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

#ifndef LL_LLFLOATEREDITEXTDAYCYCLE_H
#define LL_LLFLOATEREDITEXTDAYCYCLE_H

#include "llfloater.h"
#include "llsettingsdaycycle.h"
#include <boost/signals2.hpp>

class LLCheckBoxCtrl;
class LLComboBox;
class LLLineEditor;
class LLMultiSliderCtrl;
class LLTimeCtrl;

typedef std::shared_ptr<LLSettingsBase> LLSettingsBasePtr_t;

class SliderKey
{
public:
	SliderKey(LLSettingsBasePtr_t kf, F32 t) : keyframe(kf), time(t) {}

	LLSettingsBasePtr_t keyframe;
	F32 time;
};


/**
 * Floater for creating or editing a day cycle.
 */
class LLFloaterEditExtDayCycle : public LLFloater
{
	LOG_CLASS(LLFloaterEditExtDayCycle);

public:
    typedef boost::signals2::signal<void(LLSettingsDay::ptr_t)>            edit_commit_signal_t;
    typedef boost::signals2::connection     connection_t;

	LLFloaterEditExtDayCycle(const LLSD &key);

    BOOL	postBuild();
 	void	onOpen(const LLSD& key);
 	void	onClose(bool app_quitting);


    /*TEMP*/
    void    onUpload();

    void    onVisibilityChange(BOOL new_visibility);

// 	/*virtual*/ void	draw();
    connection_t setEditCommitSignal(edit_commit_signal_t::slot_type cb);

private:

// 	/// sync the time slider with day cycle structure
// 	void syncTimeSlider();
// 
// 	// 	makes sure key slider has what's in day cycle
// 	void loadTrack();
// 
// 	/// makes sure day cycle data structure has what's in menu
// 	void applyTrack();
// 
// 	/// refresh the sky presets combobox

	void onBtnSave();
	void onBtnCancel();
	void onAddTrack();
	void onRemoveTrack();
	void onCommitName(class LLLineEditor* caller, void* user_data);
	void onTrackSelectionCallback(const LLSD& user_data);

	void selectTrack(U32 track_index);
	void updateTabs();
	void updateSkyTabs();
	void updateWaterTabs();
	void updateSlider(); //track->slider
	//void updateTrack(); // slider->track, todo: better name

// 	/// refresh the day cycle combobox
// 	void refreshDayCyclesList();
// 
// 	/// add a slider to the track
	void addSliderKey(F32 time, const LLSettingsBasePtr_t key);
// 
// 	void initCallbacks();
// //	LLWLParamKey getSelectedDayCycle();
// 	bool isNewDay() const;
// 	void dumpTrack();
// 	void enableEditing(bool enable);
// 	void reset();
// 	void saveRegionDayCycle();
// 
// 	void setApplyProgress(bool started);
// 	bool getApplyProgress() const;
// 
// 	void onTimeSliderMoved();	/// time slider moved
// 	void onKeyTimeMoved();		/// a key frame moved
// 	void onKeyTimeChanged();	/// a key frame's time changed
// 	void onAddKey();			/// new key added on slider
// 	void onDeleteKey();			/// a key frame deleted
// 
// 	void onRegionSettingsChange();
// 	void onRegionChange();
// 	void onRegionSettingsApplied(bool success);
// 	void onRegionInfoUpdate();
// 
// 	void onDayCycleNameEdited();
// 	void onDayCycleSelected();
// 
// 	bool onSaveAnswer(const LLSD& notification, const LLSD& response);
// 	void onSaveConfirmed();
// 
// 	void onDayCycleListChange();
// 	void onSkyPresetListChange();
// 
// 	static std::string getRegionName();

    LLSettingsDay::ptr_t    mSavedDay;
    LLSettingsDay::ptr_t    mEditDay;
	U32 mCurrentTrack;

    LLButton*			mSaveButton;
    LLButton*			mCancelButton;
    LLButton*           mUploadButton;

    edit_commit_signal_t    mCommitSignal;

//	LLComboBox*			mDayCyclesCombo;
// 	LLMultiSliderCtrl*	mTimeSlider;
    LLMultiSliderCtrl*  mKeysSlider;
    LLView*             mSkyTabContainer;
    LLView*             mWaterTabContainer;
    // 	LLTimeCtrl*			mTimeCtrl;
// 	LLCheckBoxCtrl*		mMakeDefaultCheckBox;

	// map of sliders to parameters
	std::map<std::string, SliderKey> mSliderToKey;
};

#endif // LL_LLFloaterEditExtDayCycle_H
