/** 
 * @file llfloaterskysettings.h
 * @brief LLFloaterEnvSettings class definition
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
 * Simple menu for adjusting the atmospheric settings of the world
 */

#ifndef LL_LLFLOATERENVSETTINGS_H
#define LL_LLFLOATERENVSETTINGS_H

#include "llfloater.h"


/// Menuing system for all of windlight's functionality
class LLFloaterEnvSettings : public LLFloater
{
public:

	LLFloaterEnvSettings();
	/*virtual*/ ~LLFloaterEnvSettings();
	/*virtual*/	BOOL	postBuild();	
	/// initialize all the callbacks for the menu
	void initCallbacks(void);

	/// one and one instance only
	static LLFloaterEnvSettings* instance();
	
	/// callback for the menus help button
	static void onClickHelp(void* data);
	
	/// handle if time of day is changed
	static void onChangeDayTime(LLUICtrl* ctrl, void* userData);

	/// handle if cloud coverage is changed
	static void onChangeCloudCoverage(LLUICtrl* ctrl, void* userData);

	/// handle change in water fog density
	static void onChangeWaterFogDensity(LLUICtrl* ctrl, void* userData);

	/// handle change in under water fog density
	static void onChangeUnderWaterFogMod(LLUICtrl* ctrl, void* userData);

	/// handle change in water fog color
	static void onChangeWaterColor(LLUICtrl* ctrl, void* userData);

	/// open the advanced sky settings menu
	static void onOpenAdvancedSky(void* userData);

	/// open the advanced water settings menu
	static void onOpenAdvancedWater(void* userData);

	/// sync time with the server
	static void onUseEstateTime(void* userData);

	//// menu management

	/// show off our menu
	static void show();

	/// return if the menu exists or not
	static bool isOpen();

	/// stuff to do on exit
	virtual void onClose(bool app_quitting);

	/// sync up sliders with parameters
	void syncMenu();

	/// convert the present time to a digital clock time
	std::string timeToString(F32 curTime);

private:
	// one instance on the inside
	static LLFloaterEnvSettings* sEnvSettings;
};


#endif
