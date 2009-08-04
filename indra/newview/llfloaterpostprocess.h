/** 
 * @file llfloaterpostprocess.h
 * @brief LLFloaterPostProcess class definition
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
