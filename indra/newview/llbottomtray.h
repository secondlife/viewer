/** 
* @file llbottomtray.h
* @brief LLBottomTray class header file
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLBOTTOMPANEL_H
#define LL_LLBOTTOMPANEL_H

#include <llmenugl.h>

#include "llpanel.h"
#include "llimview.h"
#include "llcombobox.h"

class LLChicletPanel;
class LLLineEditor;
class LLLayoutStack;
class LLNotificationChiclet;
class LLTalkButton;
class LLNearbyChatBar;
class LLIMChiclet;

class LLBottomTray 
	: public LLSingleton<LLBottomTray>
	, public LLPanel
	, public LLIMSessionObserver
{
	friend class LLSingleton<LLBottomTray>;
public:
	~LLBottomTray();

	BOOL postBuild();

	LLChicletPanel*		getChicletPanel()	{return mChicletPanel;}
	LLNotificationChiclet*	getSysWell()	{return mSysWell;}
	LLNearbyChatBar*		getNearbyChatBar()	{return mNearbyChatBar;}

	void onCommitGesture(LLUICtrl* ctrl);

	// LLIMSessionObserver observe triggers
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	virtual void sessionRemoved(const LLUUID& session_id);

	virtual void onFocusLost();
	virtual void setVisible(BOOL visible);

	void showBottomTrayContextMenu(S32 x, S32 y, MASK mask);

	void showGestureButton(BOOL visible);
	void showMoveButton(BOOL visible);
	void showCameraButton(BOOL visible);

private:

protected:

	LLBottomTray(const LLSD& key = LLSD());

	void onChicletClick(LLUICtrl* ctrl);

	static void* createNearbyChatBar(void* userdata);

	/**
	 * Creates IM Chiclet based on session type (IM chat or Group chat)
	 */
	LLIMChiclet* createIMChiclet(const LLUUID& session_id);

	LLChicletPanel* 	mChicletPanel;
	LLNotificationChiclet* 	mSysWell;
	LLTalkButton* 		mTalkBtn;
	LLNearbyChatBar*	mNearbyChatBar;
	LLLayoutStack*		mToolbarStack;
	LLMenuGL*			mBottomTrayContextMenu;
	LLPanel*			mMovementPanel;
	LLPanel*			mCamPanel;
	LLComboBox*			mGestureCombo;
};

#endif // LL_LLBOTTOMPANEL_H
