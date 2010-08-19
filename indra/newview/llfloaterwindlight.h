/** 
 * @file llfloaterwindlight.h
 * @brief LLFloaterWindLight class definition
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
