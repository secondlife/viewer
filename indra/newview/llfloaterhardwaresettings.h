/** 
 * @file llfloaterhardwaresettings.h
 * @brief Menu of all the different graphics hardware settings
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLFLOATER_HARDWARE_SETTINGS_H
#define LL_LLFLOATER_HARDWARE_SETTINGS_H

#include "llfloater.h"

/// Menuing system for all of windlight's functionality
class LLFloaterHardwareSettings : public LLFloater
{
	friend class LLFloaterPreference;

public:

	LLFloaterHardwareSettings(const LLSD& key);
	/*virtual*/ ~LLFloaterHardwareSettings();
	
	/*virtual*/ BOOL postBuild();

	/// initialize all the callbacks for the menu
	void initCallbacks(void);

	/// OK button
	static void onBtnOK( void* userdata );
	
	//// menu management

	/// show off our menu
	static void show();

	/// return if the menu exists or not
	static bool isOpen();

	/// sync up menu with parameters
	void refresh();

	/// Apply the changed values.
	void apply();
	
	/// don't apply the changed values
	void cancel();

	/// refresh the enabled values
	void refreshEnabledState();

protected:
	BOOL mUseVBO;
	BOOL mUseAniso;
	BOOL mUseFBO;
	U32 mFSAASamples;
	F32 mGamma;
	S32 mVideoCardMem;
	F32 mFogRatio;
	BOOL mProbeHardwareOnStartup;

private:
};

#endif

