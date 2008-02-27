/** 
 * @file llfloaterhardwaresettings.h
 * @brief Menu of all the different graphics hardware settings
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_LLFLOATER_HARDWARE_SETTINGS_H
#define LL_LLFLOATER_HARDWARE_SETTINGS_H

#include "llfloater.h"

class LLSliderCtrl;

/// Menuing system for all of windlight's functionality
class LLFloaterHardwareSettings : public LLFloater
{
	friend class LLPreferenceCore;

public:

	LLFloaterHardwareSettings();
	virtual ~LLFloaterHardwareSettings();
	
	virtual BOOL postBuild();

	/// initialize all the callbacks for the menu
	void initCallbacks(void);

	/// one and one instance only
	static LLFloaterHardwareSettings* instance();
	
	/// callback for the menus help button
	static void onClickHelp(void* data);

	/// OK button
	static void onBtnOK( void* userdata );
	
	//// menu management

	/// show off our menu
	static void show();

	/// return if the menu exists or not
	static bool isOpen();

	/// stuff to do on exit
	virtual void onClose(bool app_quitting);

	/// sync up menu with parameters
	void refresh();

	/// Apply the changed values.
	void apply();
	
	/// don't apply the changed values
	void cancel();

	/// refresh the enabled values
	void refreshEnabledState();

protected:
	LLSliderCtrl*	mCtrlVideoCardMem;

	BOOL mUseVBO;
	BOOL mUseAniso;
	F32 mGamma;
	S32 mVideoCardMem;
	F32 mFogRatio;
	BOOL mProbeHardwareOnStartup;

private:
	// one instance on the inside
	static LLFloaterHardwareSettings* sHardwareSettings;
};

#endif

