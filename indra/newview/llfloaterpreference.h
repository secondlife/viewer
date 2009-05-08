/** 
 * @file llfloaterpreference.h
 * @brief LLPreferenceCore class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#ifndef LL_LLFLOATERPREFERENCE_H
#define LL_LLFLOATERPREFERENCE_H

#include "llfloater.h"

class LLPanelGeneral;
class LLPanelInput;
class LLPanelLCD;
class LLPanelDisplay;
class LLPanelAudioPrefs;
class LLPanelDebug;
class LLPanelNetwork;
class LLPanelWeb;
class LLMessageSystem;
class LLPrefsChat;
class LLPrefsVoice;
class LLPrefsIM;
class LLPanelMsgs;
class LLPanelSkins;
class LLScrollListCtrl;

class LLSD;

// Floater to control preferences (display, audio, bandwidth, general.
class LLFloaterPreference : public LLFloater
{
public: 
	LLFloaterPreference(const LLSD& key);
	~LLFloaterPreference();

	void apply();
	void cancel();
	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& key);
	virtual void onClose(bool app_quitting);

	// static data update, called from message handler
	static void updateUserInfo(const std::string& visibility, bool im_via_email, const std::string& email);

	// refresh all the graphics preferences menus
	static void refreshEnabledGraphics();
	
protected:
	static void		onClickAbout(void*);
	static void		onBtnOK(void*);
	static void		onBtnCancel(void*);
	static void		onBtnApply(void*);
	
private:
	LLPanelSkins			*mSkinsPanel;
	LLPanelInput			*mInputPanel;
	LLPanelNetwork	        *mNetworkPanel;
	LLPanelDisplay	        *mDisplayPanel;
	LLPanelAudioPrefs		*mAudioPanel;
	LLPrefsChat				*mPrefsChat;
	LLPrefsVoice			*mPrefsVoice;
	LLPrefsIM				*mPrefsIM;
	LLPanelWeb				*mWebPanel;
	LLPanelMsgs				*mMsgPanel;
};

class LLPanelPreference : public LLPanel
{
public:
	/*virtual*/ BOOL postBuild();
	
	virtual void apply();
	virtual void cancel();
	
private:
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;
};

#endif  // LL_LLPREFERENCEFLOATER_H
