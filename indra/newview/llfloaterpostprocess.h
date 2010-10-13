/** 
 * @file llfloaterpostprocess.h
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

#ifndef LL_LLFLOATERPOSTPROCESS_H
#define LL_LLFLOATERPOSTPROCESS_H

#include "llfloater.h"

class LLButton;
class LLComboBox;
class LLLineEditor;
class LLSliderCtrl;
class LLTabContainer;
class LLPanelPermissions;
class LLPanelObject;
class LLPanelVolume;
class LLPanelContents;
class LLPanelFace;

/**
 * Menu for adjusting the post process settings of the world
 */
class LLFloaterPostProcess : public LLFloater
{
public:

	LLFloaterPostProcess(const LLSD& key);
	virtual ~LLFloaterPostProcess();
	/*virtual*/	BOOL	postBuild();

	/// post process callbacks
	static void onBoolToggle(LLUICtrl* ctrl, void* userData);
	static void onFloatControlMoved(LLUICtrl* ctrl, void* userData);
	static void onColorControlRMoved(LLUICtrl* ctrl, void* userData);
	static void onColorControlGMoved(LLUICtrl* ctrl, void* userData);
	static void onColorControlBMoved(LLUICtrl* ctrl, void* userData);
	static void onColorControlIMoved(LLUICtrl* ctrl, void* userData);
	void onLoadEffect(LLComboBox* comboBox);
	void onSaveEffect(LLLineEditor* editBox);
	void onChangeEffectName(LLUICtrl* ctrl);

	/// prompts a user when overwriting an effect
	bool saveAlertCallback(const LLSD& notification, const LLSD& response);

	/// sync up sliders
	void syncMenu();

/*
	void refresh();
*/
public:
};

#endif
