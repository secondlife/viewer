/** 
 * @file llfloaterdaycycle.h
 * @brief LLFloaterDayCycle class definition
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
	/// help button stuff
	void onClickHelp(std::string xml_alert);
	void initHelpBtn(const std::string& name, const std::string& xml_alert);

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
