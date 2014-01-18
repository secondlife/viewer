/** 
 * @file llfloatereditwater.h
 * @brief Floater to create or edit a water preset
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

#ifndef LL_LLFLOATEREDITWATER_H
#define LL_LLFLOATEREDITWATER_H

#include "llfloater.h"
#include "llenvmanager.h" // for LLEnvKey

class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLLineEditor;

struct WaterVector2Control;
struct WaterVector3Control;
struct WaterColorControl;
struct WaterFloatControl;
struct WaterExpFloatControl;

class LLFloaterEditWater : public LLFloater
{
	LOG_CLASS(LLFloaterEditWater);

public:
	LLFloaterEditWater(const LLSD &key);

	/*virtual*/	BOOL	postBuild();
	/*virtual*/ void	onOpen(const LLSD& key);
	/*virtual*/ void	onClose(bool app_quitting);
	/*virtual*/ void	draw();

private:
	void initCallbacks(void);

	//-- WL stuff begins ------------------------------------------------------

	void syncControls(); /// sync up sliders with parameters

	// general purpose callbacks for dealing with color controllers
	void onColorControlRMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);
	void onColorControlGMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);
	void onColorControlBMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);
	void onColorControlAMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);
	void onColorControlIMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);

	void onVector3ControlXMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl);
	void onVector3ControlYMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl);
	void onVector3ControlZMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl);

	void onVector2ControlXMoved(LLUICtrl* ctrl, WaterVector2Control* vector_ctrl);
	void onVector2ControlYMoved(LLUICtrl* ctrl, WaterVector2Control* vector_ctrl);

	void onFloatControlMoved(LLUICtrl* ctrl, WaterFloatControl* floatControl);

	void onExpFloatControlMoved(LLUICtrl* ctrl, WaterExpFloatControl* expFloatControl);

	void onWaterFogColorMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl);

	void onNormalMapPicked(LLUICtrl* ctrl); /// handle if they choose a new normal map

	//-- WL stuff ends --------------------------------------------------------

	void reset();
	bool isNewPreset() const;
	void refreshWaterPresetsList();
	void enableEditing(bool enable);
	void saveRegionWater();

	std::string			getCurrentPresetName() const;
	LLEnvKey::EScope	getCurrentScope() const;
	void				getSelectedPreset(std::string& name, LLEnvKey::EScope& scope) const;

	void onWaterPresetNameEdited();
	void onWaterPresetSelected();
	bool onSaveAnswer(const LLSD& notification, const LLSD& response);
	void onSaveConfirmed();

	void onBtnSave();
	void onBtnCancel();

	void onWaterPresetListChange();
	void onRegionSettingsChange();
	void onRegionInfoUpdate();

	LLLineEditor*	mWaterPresetNameEditor;
	LLComboBox*		mWaterPresetCombo;
	LLCheckBoxCtrl*	mMakeDefaultCheckBox;
	LLButton*		mSaveButton;
};

#endif // LL_LLFLOATEREDITWATER_H
