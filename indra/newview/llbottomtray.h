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

#include "llmenugl.h"

#include "llpanel.h"
#include "llimview.h"
#include "llcombobox.h"

class LLChicletPanel;
class LLLineEditor;
class LLLayoutStack;
class LLNotificationChiclet;
class LLSpeakButton;
class LLNearbyChatBar;
class LLIMChiclet;

class LLBottomTray 
	: public LLSingleton<LLBottomTray>
	, public LLPanel
	, public LLIMSessionObserver
	, public LLVoiceClientStatusObserver
{
	LOG_CLASS(LLBottomTray);
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
	void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);

	virtual void reshape(S32 width, S32 height, BOOL called_from_parent);

	virtual void onFocusLost();
	virtual void setVisible(BOOL visible);

	// Implements LLVoiceClientStatusObserver::onChange() to enable the speak
	// button when voice is available
	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

	void showBottomTrayContextMenu(S32 x, S32 y, MASK mask);

	void showGestureButton(BOOL visible);
	void showMoveButton(BOOL visible);
	void showCameraButton(BOOL visible);
	void showSnapshotButton(BOOL visible);

private:
	typedef enum e_resize_status_type
	{
		  RS_NORESIZE			= 0x0000
		, RS_CHICLET_PANEL		= 0x0001
		, RS_CHATBAR_INPUT		= 0x0002
		, RS_BUTTON_SNAPSHOT	= 0x0004
		, RS_BUTTON_CAMERA		= 0x0008
		, RS_BUTTON_MOVEMENT	= 0x0010
		, RS_BUTTON_GESTURES	= 0x0020
		, RS_BUTTON_SPEAK		= 0x0040
		, RS_RESIZABLE_BUTTONS			= /*RS_BUTTON_SNAPSHOT | */RS_BUTTON_CAMERA | RS_BUTTON_MOVEMENT | RS_BUTTON_GESTURES
	}EResizeState;

	void updateResizeState(S32 new_width, S32 cur_width);
	void verifyChildControlsSizes();
	void processWidthDecreased(S32 delta_width);
	void processWidthIncreased(S32 delta_width);
	void log(LLView* panel, const std::string& descr);
	bool processShowButton(EResizeState shown_object_type, S32* available_width, S32* buttons_required_width);
	void processHideButton(EResizeState shown_object_type, S32* required_width, S32* buttons_freed_width);
	bool canButtonBeShown(LLPanel* panel) const;
	void initStateProcessedObjectMap();
	void showTrayButton(EResizeState shown_object_type, bool visible);

	MASK mResizeState;

	typedef std::map<EResizeState, LLPanel*> state_object_map_t;
	state_object_map_t mStateProcessedObjectMap;

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
	LLSpeakButton* 		mSpeakBtn;
	LLNearbyChatBar*	mNearbyChatBar;
	LLLayoutStack*		mToolbarStack;
	LLMenuGL*			mBottomTrayContextMenu;
	LLPanel*			mMovementPanel;
	LLPanel*			mCamPanel;
	LLPanel*			mSnapshotPanel;
	LLPanel*			mGesturePanel;
	LLButton*			mCamButton;
	LLButton*			mMovementButton;
};

#endif // LL_LLBOTTOMPANEL_H
