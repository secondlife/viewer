/** 
 * @file llfloaterwindlight.h
 * @brief LLFloaterWindLight class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

	LLFloaterWindLight();
	virtual ~LLFloaterWindLight();
	
	/// initialize all
	void initCallbacks(void);

	/// one and one instance only
	static LLFloaterWindLight* instance();

	// help button stuff
	static void onClickHelp(void* data);
	void initHelpBtn(const std::string& name, const std::string& xml_alert);

	static void newPromptCallback(S32 option, const std::string& text, void* userData);

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
	static void saveAlertCallback(S32 option, void* userdata);

	/// when user hits the save preset button
	static void onDeletePreset(void* userData);

	/// prompts a user when overwriting a preset
	static void deleteAlertCallback(S32 option, void* userdata);

	/// what to do when you change the preset name
	static void onChangePresetName(LLUICtrl* ctrl, void* userData);

	/// when user hits the save preset button
	static void onOpenDayCycle(void* userData);

	/// handle cloud scrolling
	static void onCloudScrollXMoved(LLUICtrl* ctrl, void* userData);
	static void onCloudScrollYMoved(LLUICtrl* ctrl, void* userData);
	static void onCloudScrollXToggled(LLUICtrl* ctrl, void* userData);
	static void onCloudScrollYToggled(LLUICtrl* ctrl, void* userData);

	//// menu management

	/// show off our menu
	static void show();

	/// return if the menu exists or not
	static bool isOpen();

	/// stuff to do on exit
	virtual void onClose(bool app_quitting);

	/// sync up sliders with parameters
	void syncMenu();

	/// turn off animated skies
	static void deactivateAnimator();

private:
	// one instance on the inside
	static LLFloaterWindLight* sWindLight;

	static std::set<std::string> sDefaultPresets;
};


#endif
