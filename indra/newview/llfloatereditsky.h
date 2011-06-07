/** 
 * @file llfloatereditsky.h
 * @brief Floater to create or edit a sky preset
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

#ifndef LL_LLFLOATEREDITSKY_H
#define LL_LLFLOATEREDITSKY_H

#include "llfloater.h"
#include "llwlparammanager.h"

class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLLineEditor;

/**
 * Floater for creating or editing a sky preset.
 */
class LLFloaterEditSky : public LLFloater
{
	LOG_CLASS(LLFloaterEditSky);

public:
	LLFloaterEditSky(const LLSD &key);

	/*virtual*/	BOOL	postBuild();
	/*virtual*/ void	onOpen(const LLSD& key);
	/*virtual*/ void	onClose(bool app_quitting);
	/*virtual*/ void	draw();

private:
	void initCallbacks(void);

	//-- WL stuff begins ------------------------------------------------------

	// general purpose callbacks for dealing with color controllers
	void onColorControlRMoved(LLUICtrl* ctrl, void* userdata);
	void onColorControlGMoved(LLUICtrl* ctrl, void* userdata);
	void onColorControlBMoved(LLUICtrl* ctrl, void* userdata);
	void onColorControlIMoved(LLUICtrl* ctrl, void* userdata);
	void onFloatControlMoved(LLUICtrl* ctrl, void* userdata);

	// lighting callbacks for glow
	void onGlowRMoved(LLUICtrl* ctrl, void* userdata);
	void onGlowBMoved(LLUICtrl* ctrl, void* userdata);

	// lighting callbacks for sun
	void onSunMoved(LLUICtrl* ctrl, void* userdata);

	// for handling when the star slider is moved to adjust the alpha
	void onStarAlphaMoved(LLUICtrl* ctrl);

	// handle cloud scrolling
	void onCloudScrollXMoved(LLUICtrl* ctrl);
	void onCloudScrollYMoved(LLUICtrl* ctrl);
	void onCloudScrollXToggled(LLUICtrl* ctrl);
	void onCloudScrollYToggled(LLUICtrl* ctrl);

	void syncControls(); /// sync up sliders with parameters

	//-- WL stuff ends --------------------------------------------------------

	bool isNewPreset() const;
	void refreshSkyPresetsList();
	void enableEditing(bool enable);
	void saveRegionSky();
	LLWLParamKey getSelectedSkyPreset();

	void onSkyPresetNameEdited();
	void onSkyPresetSelected();
	bool onSaveAnswer(const LLSD& notification, const LLSD& response);
	void onSaveConfirmed();

	void onBtnSave();
	void onBtnCancel();

	void onSkyPresetListChange();

	LLLineEditor*	mSkyPresetNameEditor;
	LLComboBox*		mSkyPresetCombo;
	LLCheckBoxCtrl*	mMakeDefaultCheckBox;
	LLButton*		mSaveButton;
};

#endif // LL_LLFLOATEREDITSKY_H
