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

struct WLColorControl;
struct WLFloatControl;

/// convenience class for holding keys mapped to sliders
struct LLWLSkyKey
{
public:
	std::string presetName;
	F32 time;
};

/// Menu for all of windlight's functionality.
/// Menuing system for adjusting the atmospheric settings of the world.
class LLFloaterDayCycle : public LLFloater
{
public:

	LLFloaterDayCycle(const LLSD& key);
	virtual ~LLFloaterDayCycle();
	/*virtual*/	BOOL	postBuild();

	/// initialize all
	void initCallbacks(void);

	/// on time slider moved
	void onTimeSliderMoved(LLUICtrl* ctrl);

	/// what happens when you move the key frame
	void onKeyTimeMoved(LLUICtrl* ctrl);

	/// what happens when you change the key frame's time
	void onKeyTimeChanged(LLUICtrl* ctrl);

	/// if you change the combo box, change the frame
	void onKeyPresetChanged(LLUICtrl* ctrl);
	
	/// run this when user says to run the sky animation
	void onRunAnimSky(LLUICtrl* ctrl);

	/// run this when user says to stop the sky animation
	void onStopAnimSky(LLUICtrl* ctrl);

	/// if you change the combo box, change the frame
	void onTimeRateChanged(LLUICtrl* ctrl);

	/// add a new key on slider
	void onAddKey(LLUICtrl* ctrl);

	/// delete any and all reference to a preset
	void deletePreset(std::string& presetName);

	/// delete a key frame
	void onDeleteKey(LLUICtrl* ctrl);

	/// button to load day
	void onLoadDayCycle(LLUICtrl* ctrl);

	/// button to save day
	void onSaveDayCycle(LLUICtrl* ctrl);

	/// toggle for Linden time
	void onUseLindenTime(LLUICtrl* ctrl);

	/// sync up sliders with day cycle structure
	void syncMenu();

	// 	makes sure key slider has what's in day cycle
	void syncSliderTrack();

	/// makes sure day cycle data structure has what's in menu
	void syncTrack();

	/// add a slider to the track
	void addSliderKey(F32 time, const std::string& presetName);

private:

	// map of sliders to parameters
	static std::map<std::string, LLWLSkyKey> sSliderToKey;

	static const F32 sHoursPerDay;
};


#endif
