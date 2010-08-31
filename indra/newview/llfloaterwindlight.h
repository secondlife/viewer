/** 
 * @file llfloaterwindlight.h
 * @brief LLFloaterWindLight class definition
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

/*
 * Menu for adjusting the atmospheric settings of the world
 */

#ifndef LL_LLFLOATERWINDLIGHT_H
#define LL_LLFLOATERWINDLIGHT_H

#include "llfloater.h"

#include <vector>
#include "llwlparamset.h"

struct WLColorControl;
struct WLFloatControl;


/// Menuing system for all of windlight's functionality
class LLFloaterWindLight : public LLFloater
{
public:

	LLFloaterWindLight(const LLSD& key);
	virtual ~LLFloaterWindLight();
	/*virtual*/	BOOL	postBuild();	
	/// initialize all
	void initCallbacks(void);

	bool newPromptCallback(const LLSD& notification, const LLSD& response);

	/// general purpose callbacks for dealing with color controllers
	void onColorControlRMoved(LLUICtrl* ctrl, WLColorControl* userData);
	void onColorControlGMoved(LLUICtrl* ctrl, WLColorControl* userData);
	void onColorControlBMoved(LLUICtrl* ctrl, WLColorControl* userData);
	void onColorControlIMoved(LLUICtrl* ctrl, WLColorControl* userData);
	void onFloatControlMoved(LLUICtrl* ctrl, WLFloatControl* userData);

	/// lighting callbacks for glow
	void onGlowRMoved(LLUICtrl* ctrl, WLColorControl* userData);
	//static void onGlowGMoved(LLUICtrl* ctrl, void* userData);
	void onGlowBMoved(LLUICtrl* ctrl, WLColorControl* userData);

	/// lighting callbacks for sun
	void onSunMoved(LLUICtrl* ctrl, WLColorControl* userData);

	/// for handling when the star slider is moved to adjust the alpha
	void onStarAlphaMoved(LLUICtrl* ctrl);

	/// when user hits the load preset button
	void onNewPreset();

	/// when user hits the save preset button
	void onSavePreset();

	/// prompts a user when overwriting a preset
	bool saveAlertCallback(const LLSD& notification, const LLSD& response);

	/// when user hits the save preset button
	void onDeletePreset();

	/// prompts a user when overwriting a preset
	bool deleteAlertCallback(const LLSD& notification, const LLSD& response);

	/// what to do when you change the preset name
	void onChangePresetName(LLUICtrl* ctrl);

	/// when user hits the save preset button
	void onOpenDayCycle();

	/// handle cloud scrolling
	void onCloudScrollXMoved(LLUICtrl* ctrl);
	void onCloudScrollYMoved(LLUICtrl* ctrl);
	void onCloudScrollXToggled(LLUICtrl* ctrl);
	void onCloudScrollYToggled(LLUICtrl* ctrl);

	/// sync up sliders with parameters
	void syncMenu();

	/// turn off animated skies
	static void deactivateAnimator();

private:
	static std::set<std::string> sDefaultPresets;
};


#endif
