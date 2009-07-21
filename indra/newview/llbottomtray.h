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

#include "llpanel.h"
#include "llimview.h"
#include "llchat.h"
#include "llgesturemgr.h"
#include "llcombobox.h"

class LLChicletPanel;
class LLNotificationChiclet;
class LLTalkButton;

class LLGestureComboBox
	: public LLComboBox
	, public LLGestureManagerObserver
{
protected:
	LLGestureComboBox(const LLComboBox::Params&);
	friend class LLUICtrlFactory;
public:
	~LLGestureComboBox();

	void refreshGestures();
	void onCommitGesture(LLUICtrl* ctrl);
	virtual void draw();

	// LLGestureManagerObserver trigger
	virtual void changed() { refreshGestures(); }

protected:
	LLFrameTimer mGestureLabelTimer;
};

class LLBottomTray 
	: public LLUISingleton<LLBottomTray>
	, public LLPanel
	, public LLIMSessionObserver
{
	friend class LLUISingleton<LLBottomTray>;
public:
	~LLBottomTray();

	LLLineEditor*		getChatBox()	{return mChatBox;}
	LLChicletPanel*		getChicletPanel()	{return mChicletPanel;}
	LLNotificationChiclet*	getIMWell()	{return mIMWell;}
	LLNotificationChiclet*	getSysWell()	{return mSysWell;}

	void onChatBoxCommit();
	static void sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate);
	static void sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate);
	static void onChatBoxKeystroke(LLLineEditor* caller, void* userdata);
	static void onChatBoxFocusLost(LLFocusableElement* caller, void* userdata);

	BOOL inputEditorHasFocus();
	std::string getCurrentChat();

	/*virtual*/void draw();
	void refreshStandUp();

	void onCommitGesture(LLUICtrl* ctrl);
	void onCommitStandUp(LLUICtrl* ctrl);	
	void refreshGestures();

	// LLIMSessionObserver observe triggers
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	virtual void sessionRemoved(const LLUUID& session_id);

	virtual void onFocusLost();
	virtual BOOL handleKeyHere(KEY key, MASK mask);
	virtual void setVisible(BOOL visible);

	static void startChat(const char* line);
	static void stopChat();

protected:

	LLBottomTray(const LLSD& key = LLSD());

	void sendChat( EChatType type );
	static LLWString stripChannelNumber(const LLWString &mesg, S32* channel);

	void onChicletClick(LLUICtrl* ctrl);

	void setChicletPanelVisible(bool visible);

	// Which non-zero channel did we last chat on?
	static S32 sLastSpecialChatChannel;

	LLLineEditor*		mChatBox;
	LLChicletPanel* 	mChicletPanel;
	LLNotificationChiclet* 	mIMWell;
	LLNotificationChiclet* 	mSysWell;
	LLTalkButton* 		mTalkBtn;
	LLGestureComboBox*	mGestureCombo;
	LLButton*           mStandUpBtn;
};

#endif // LL_LLBOTTOMPANEL_H
