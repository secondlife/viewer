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

// Build time optimization, generate once in .cpp file
#ifndef LLBOTTOMTRAY_CPP
extern template class LLBottomTray* LLSingleton<class LLBottomTray>::getInstance();
#endif

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
	LLNearbyChatBar*		getNearbyChatBar()	{return mNearbyChatBar;}

	void onCommitGesture(LLUICtrl* ctrl);

	// LLIMSessionObserver observe triggers
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	virtual void sessionRemoved(const LLUUID& session_id);
	void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);

	S32 getTotalUnreadIMCount();

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
	
	/**
	 * Creates IM Chiclet based on session type (IM chat or Group chat)
	 */
	LLIMChiclet* createIMChiclet(const LLUUID& session_id);

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

		/**
		 * Specifies buttons which can be hidden when bottom tray is shrunk.
		 * They are: Gestures, Movement (Move), Camera (View), Snapshot
		 */
		, RS_BUTTONS_CAN_BE_HIDDEN = RS_BUTTON_SNAPSHOT | RS_BUTTON_CAMERA | RS_BUTTON_MOVEMENT | RS_BUTTON_GESTURES
	}EResizeState;

	S32 processWidthDecreased(S32 delta_width);
	void processWidthIncreased(S32 delta_width);
	void log(LLView* panel, const std::string& descr);
	bool processShowButton(EResizeState shown_object_type, S32* available_width);
	void processHideButton(EResizeState processed_object_type, S32* required_width, S32* buttons_freed_width);

	/**
	 * Shrinks shown buttons to reduce total taken space.
	 *
	 * @param - required_width - width which buttons can use to be shrunk. It is a negative value.
	 * It is increased on the value processed by buttons.
	 */
	void processShrinkButtons(S32* required_width, S32* buttons_freed_width);
	void processShrinkButton(EResizeState processed_object_type, S32* required_width);

	/**
	 * Extends shown buttons to increase total taken space.
	 *
	 * @param - available_width - width which buttons can use to be extended. It is a positive value.
	 * It is decreased on the value processed by buttons.
	 */
	void processExtendButtons(S32* available_width);
	void processExtendButton(EResizeState processed_object_type, S32* available_width);

	/**
	 * Determines if specified by type object can be shown. It should be hidden by shrink before.
	 *
	 * Processes buttons a such way to show buttons in constant order:
	 *   - Gestures, Move, View, Snapshot
	 */
	bool canButtonBeShown(EResizeState processed_object_type) const;
	void initStateProcessedObjectMap();

	/**
	 * Sets passed visibility to object specified by resize type.
	 */
	void setTrayButtonVisible(EResizeState shown_object_type, bool visible);

	/**
	 * Sets passed visibility to object specified by resize type if it is possible.
	 *
	 * If it is impossible to show required button due to there is no enough room in bottom tray
	 * it will no be shown. Is called via context menu commands.
	 * In this case Alert Dialog will be shown to notify user about that.
	 *
	 * Method also stores resize state to be processed while future bottom tray extending:
	 *  - if hidden while resizing button should be hidden it will not be shown while extending;
	 *  - if hidden via context menu button should be shown but there is no enough room for now
	 *    it will be shown while extending.
	 */
	void setTrayButtonVisibleIfPossible(EResizeState shown_object_type, bool visible);

	MASK mResizeState;

	typedef std::map<EResizeState, LLPanel*> state_object_map_t;
	state_object_map_t mStateProcessedObjectMap;

	typedef std::map<EResizeState, S32> state_object_width_map_t;
	state_object_width_map_t mObjectDefaultWidthMap;

protected:

	LLBottomTray(const LLSD& key = LLSD());

	void onChicletClick(LLUICtrl* ctrl);

	static void* createNearbyChatBar(void* userdata);

	void updateContextMenu(S32 x, S32 y, MASK mask);
	void onContextMenuItemClicked(const LLSD& userdata);
	bool onContextMenuItemEnabled(const LLSD& userdata);

	LLChicletPanel* 	mChicletPanel;
	LLPanel*			mSpeakPanel;
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
