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
#include "llwlparammanager.h"

struct WLColorControl;
struct WLFloatControl;

/// Menuing system for all of windlight's functionality
class LLFloaterWindLight : public LLFloater
{
	LOG_CLASS(LLFloaterWindLight);
public:
	LLFloaterWindLight(const LLSD &key);
	virtual ~LLFloaterWindLight();
	BOOL postBuild();

	/// initialize all
	void initCallbacks(void);

	/// one and one instance only
	static LLFloaterWindLight* instance();

	static bool newPromptCallback(const LLSD& notification, const LLSD& response);

	/// general purpose callbacks for dealing with color controllers
	static void onColorControlRMoved(LLUICtrl* ctrl, void* userData);
	static void onColorControlGMoved(LLUICtrl* ctrl, void* userData);
	static void onColorControlBMoved(LLUICtrl* ctrl, void* userData);
	static void onColorControlIMoved(LLUICtrl* ctrl, void* userData);
	static void onFloatControlMoved(LLUICtrl* ctrl, void* userData);
	static void onBoolToggle(LLUICtrl* ctrl, void* userData);

	/// lighting callbacks for glow
	static void onGlowRMoved(LLUICtrl* ctrl, void* userData);
	//static void onGlowGMoved(LLUICtrl* ctrl, void* userData);
	static void onGlowBMoved(LLUICtrl* ctrl, void* userData);

	/// lighting callbacks for sun
	static void onSunMoved(LLUICtrl* ctrl, void* userData);

	/// handle if float is changed
	static void onFloatTweakMoved(LLUICtrl* ctrl, void* userData);

	/// for handling when the star slider is moved to adjust the alpha
	static void onStarAlphaMoved(LLUICtrl* ctrl, void* userData);

	/// when user hits the load preset button
	static void onNewPreset(void* userData);

	/// when user hits the save preset button
	static void onSavePreset(void* userData);

	/// prompts a user when overwriting a preset
	static bool saveAlertCallback(const LLSD& notification, const LLSD& response);

	/// when user hits the save preset button
	static void onDeletePreset(void* userData);

	/// prompts a user when overwriting a preset
	bool deleteAlertCallback(const LLSD& notification, const LLSD& response);

	/// what to do when you change the preset name
	static void onChangePresetName(LLUICtrl* ctrl);

	/// when user hits the save preset button
	static void onOpenDayCycle(void* userData);

	/// handle cloud scrolling
	static void onCloudScrollXMoved(LLUICtrl* ctrl, void* userData);
	static void onCloudScrollYMoved(LLUICtrl* ctrl, void* userData);
	static void onCloudScrollXToggled(LLUICtrl* ctrl, void* userData);
	static void onCloudScrollYToggled(LLUICtrl* ctrl, void* userData);

	//// menu management

	/// show off our menu
	static void show(LLEnvKey::EScope scope = LLEnvKey::SCOPE_LOCAL);

	/// return if the menu exists or not
	static bool isOpen();

	/// stuff to do on exit
	virtual void onClose(bool app_quitting);

	/// sync up sliders with parameters
	void syncMenu();

private:
	static LLFloaterWindLight* sWindLight;	// one instance on the inside
	static std::set<LLWLParamKey> sDefaultPresets;
	static LLEnvKey::EScope sScope;
	static std::string sOriginalTitle;
};


#endif
