/** 
 * @file llfloaterskysettings.h
 * @brief LLFloaterEnvSettings class definition
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
 * Simple menu for adjusting the atmospheric settings of the world
 */

#ifndef LL_LLFLOATERENVSETTINGS_H
#define LL_LLFLOATERENVSETTINGS_H

#include "llfloater.h"

struct WaterColorControl;
struct WaterExpFloatControl;

/// Menuing system for all of windlight's functionality
class LLFloaterEnvSettings : public LLFloater
{
public:

	LLFloaterEnvSettings(const LLSD& key);
	/*virtual*/ ~LLFloaterEnvSettings();
	/*virtual*/	BOOL	postBuild();	
	/// initialize all the callbacks for the menu
	void initCallbacks(void);

	/// handle if time of day is changed
	void onChangeDayTime(LLUICtrl* ctrl);

	/// handle if cloud coverage is changed
	void onChangeCloudCoverage(LLUICtrl* ctrl);

	/// handle change in water fog density
	void onChangeWaterFogDensity(LLUICtrl* ctrl, WaterExpFloatControl* expFloatControl);

	/// handle change in water fog color
	void onChangeWaterColor(LLUICtrl* ctrl, WaterColorControl* colorControl);

	/// open the advanced sky settings menu
	void onOpenAdvancedSky();

	/// open the advanced water settings menu
	void onOpenAdvancedWater();

	/// sync time with the server
	void onUseEstateTime();

	//// menu management

	/// sync up sliders with parameters
	void syncMenu();

	/// convert the present time to a digital clock time
	std::string timeToString(F32 curTime);

private:
};


#endif
