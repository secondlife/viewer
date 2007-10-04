/** 
 * @file llviewercontrol.h
 * @brief references to viewer-specific control files
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

#ifndef LL_LLVIEWERCONTROL_H
#define LL_LLVIEWERCONTROL_H

#include "llcontrol.h"
#include "llfloater.h"
#include "lltexteditor.h"

class LLFloaterSettingsDebug : public LLFloater
{
public:
	LLFloaterSettingsDebug();
	virtual ~LLFloaterSettingsDebug();

	virtual BOOL postBuild();
	virtual void draw();

	void updateControl(LLControlBase* control);

	static void show(void*);
	static void onSettingSelect(LLUICtrl* ctrl, void* user_data);
	static void onCommitSettings(LLUICtrl* ctrl, void* user_data);
	static void onClickDefault(void* user_data);

protected:
	static LLFloaterSettingsDebug* sInstance;
	LLTextEditor* mComment;
};

// These functions found in llcontroldef.cpp *TODO: clean this up!
//setting variables are declared in this function
void declare_settings();
void fixup_settings();
void settings_setup_listeners();

// saved at end of session
extern LLControlGroup gSavedSettings;
extern LLControlGroup gSavedPerAccountSettings;

// Read-only
extern LLControlGroup gViewerArt;

// Read-only
extern LLControlGroup gColors;

// Saved at end of session
extern LLControlGroup gCrashSettings;

// Set after settings loaded
extern LLString gLastRunVersion;
extern LLString gCurrentVersion;

extern LLString gSettingsFileName;
extern LLString gPerAccountSettingsFileName;

#endif // LL_LLVIEWERCONTROL_H
