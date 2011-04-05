/** 
 * @file llfloaterwindlight.h
 * @brief LLFloaterWater class definition
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

#ifndef LL_LLFLOATER_WATER_H
#define LL_LLFLOATER_WATER_H

#include "llfloater.h"

#include "llenvmanager.h"

struct WaterVector2Control;
struct WaterVector3Control;
struct WaterColorControl;
struct WaterFloatControl;
struct WaterExpFloatControl;

/// Menuing system for all of windlight's functionality
class LLFloaterWater : public LLFloater
{
public:

	LLFloaterWater(const LLSD& key);
	virtual ~LLFloaterWater();
	/*virtual*/	BOOL	postBuild();
	/// initialize all
	void initCallbacks(void);

	static void show(LLEnvKey::EScope scope = LLEnvKey::SCOPE_LOCAL);

	bool newPromptCallback(const LLSD& notification, const LLSD& response);

	/// general purpose callbacks for dealing with color controllers
	void onColorControlRMoved(LLUICtrl* ctrl, WaterColorControl* colorControl);
	void onColorControlGMoved(LLUICtrl* ctrl, WaterColorControl* colorControl);
	void onColorControlBMoved(LLUICtrl* ctrl, WaterColorControl* colorControl);
	void onColorControlAMoved(LLUICtrl* ctrl, WaterColorControl* colorControl);
	void onColorControlIMoved(LLUICtrl* ctrl, WaterColorControl* colorControl);

	void onVector3ControlXMoved(LLUICtrl* ctrl, WaterVector3Control* vectorControl);
	void onVector3ControlYMoved(LLUICtrl* ctrl, WaterVector3Control* vectorControl);
	void onVector3ControlZMoved(LLUICtrl* ctrl, WaterVector3Control* vectorControl);

	void onVector2ControlXMoved(LLUICtrl* ctrl, WaterVector2Control* vectorControl);
	void onVector2ControlYMoved(LLUICtrl* ctrl, WaterVector2Control* vectorControl);
	
	void onFloatControlMoved(LLUICtrl* ctrl, WaterFloatControl* floatControl);

	void onExpFloatControlMoved(LLUICtrl* ctrl, WaterExpFloatControl* expFloatControl);

	void onWaterFogColorMoved(LLUICtrl* ctrl, WaterColorControl* colorControl);

	/// handle if they choose a new normal map
	void onNormalMapPicked(LLUICtrl* ctrl);

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

	/// sync up sliders with parameters
	void syncMenu();

private:
	static std::set<std::string> sDefaultPresets;
	static LLEnvKey::EScope sScope;
	static std::string sOriginalTitle;
};


#endif
