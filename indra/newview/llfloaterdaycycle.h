/** 
 * @file llfloaterdaycycle.h
 * @brief LLFloaterDayCycle class definition
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

#ifndef LL_LLFLOATERDAYCYCLE_H
#define LL_LLFLOATERDAYCYCLE_H

#include "llfloater.h"

#include <vector>
#include "llwlparamset.h"
#include "llwlanimator.h"
#include "llwlparammanager.h"

struct WLColorControl;
struct WLFloatControl;

/// convenience class for holding keyframes mapped to sliders
struct LLWLCycleSliderKey
{
public:
	LLWLCycleSliderKey(LLWLParamKey kf, F32 t) : keyframe(kf), time(t) {}
	LLWLCycleSliderKey() : keyframe(), time(0.f) {} // Don't use this default constructor
	
	LLWLParamKey keyframe;
	F32 time;
};

/// Menu for all of windlight's functionality.
/// Menuing system for adjusting the atmospheric settings of the world.
class LLFloaterDayCycle : public LLFloater
{
	LOG_CLASS(LLFloaterDayCycle);
public:
	LLFloaterDayCycle(const LLSD &key);
	virtual ~LLFloaterDayCycle();
	/*virtual*/ BOOL postBuild();

	// map of sliders to parameters
	static std::map<std::string, LLWLCycleSliderKey> sSliderToKey;

	/// help button stuff
	static void onClickHelp(void* data);
	void initHelpBtn(const std::string& name, const std::string& xml_alert);

	/// initialize all
	void initCallbacks(void);

	/// one and one instance only
	static LLFloaterDayCycle* instance();

	/// on time slider moved
	static void onTimeSliderMoved(LLUICtrl* ctrl, void* userData);

	/// what happens when you move the key frame
	static void onKeyTimeMoved(LLUICtrl* ctrl, void* userData);

	/// what happens when you change the key frame's time
	static void onKeyTimeChanged(LLUICtrl* ctrl, void* userData);

	/// if you change the combo box, change the frame
	static void onKeyPresetChanged(LLUICtrl* ctrl, void* userData);
	
	/// run this when user says to run the sky animation
	static void onRunAnimSky(void* userData);

	/// run this when user says to stop the sky animation
	static void onStopAnimSky(void* userData);

	/// if you change the combo box, change the frame
	static void onTimeRateChanged(LLUICtrl* ctrl, void* userData);

	/// add a new key on slider
	static void onAddKey(void* userData);

	/// delete any and all reference to a preset
	void deletePreset(LLWLParamKey keyframe);

	/// delete a key frame
	static void onDeleteKey(void* userData);

	/// button to load day
	static void onLoadDayCycle(void* userData);

	/// button to save day
	static void onSaveDayCycle(void* userData);

	/// toggle for Linden time
	static void onUseLindenTime(void* userData);


	//// menu management

	/// show off our menu
	static void show(LLEnvKey::EScope scope = LLEnvKey::SCOPE_LOCAL);

	/// return if the menu exists or not
	static bool isOpen();

	/// stuff to do on exit
	virtual void onClose(bool app_quitting);

	/// sync up sliders with day cycle structure
	static void syncMenu();

	// 	makes sure key slider has what's in day cycle
	static void syncSliderTrack();

	/// makes sure day cycle data structure has what's in menu
	static void syncTrack();

	/// refresh combox box from param manager
	static void refreshPresetsFromParamManager();

	/// add a slider to the track
	static void addSliderKey(F32 time, LLWLParamKey keyframe);

private:
	static LLFloaterDayCycle* sDayCycle;	// one instance on the inside
	static const F32 sHoursPerDay;
	static LLEnvKey::EScope sScope;
	static std::string sOriginalTitle;
	static LLWLAnimator::ETime sPreviousTimeType;
};


#endif
