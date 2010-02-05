/** 
 * @file llimfloater.h
 * @brief LLIMFloater class definition
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

#ifndef LL_IMFLOATER_H
#define LL_IMFLOATER_H

#include "llinstantmessage.h"
#include "lllogchat.h"
#include "lltooldraganddrop.h"
#include "lltransientdockablefloater.h"

class LLLineEditor;
class LLPanelChatControlPanel;
class LLChatHistory;
class LLInventoryItem;
class LLInventoryCategory;

/**
 * Individual IM window that appears at the bottom of the screen,
 * optionally "docked" to the bottom tray.
 */
class LLIMFloater : public LLTransientDockableFloater
{
public:
	LLIMFloater(const LLUUID& session_id);

	virtual ~LLIMFloater();
	
	// LLView overrides
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setVisible(BOOL visible);
	// Check typing timeout timer.
	/*virtual*/ void draw();

	// LLFloater overrides
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void setDocked(bool docked, bool pop_on_undock = true);

	// Make IM conversion visible and update the message history
	static LLIMFloater* show(const LLUUID& session_id);

	// Toggle panel specified by session_id
	// Returns true iff panel became visible
	static bool toggle(const LLUUID& session_id);

	static LLIMFloater* findInstance(const LLUUID& session_id);

	static LLIMFloater* getInstance(const LLUUID& session_id);

	void sessionInitReplyReceived(const LLUUID& im_session_id);

	// get new messages from LLIMModel
	void updateMessages();
	void reloadMessages();
	static void onSendMsg( LLUICtrl*, void*);
	void sendMsg();

	// callback for LLIMModel on new messages
	// route to specific floater if it is visible
	static void newIMCallback(const LLSD& data);

	// called when docked floater's position has been set by chiclet
	void setPositioned(bool b) { mPositioned = b; };

	void onVisibilityChange(const LLSD& new_visibility);
	void processIMTyping(const LLIMInfo* im_info, BOOL typing);
	void processAgentListUpdates(const LLSD& body);
	void processSessionUpdate(const LLSD& session_update);

	void updateChatHistoryStyle();
	static void processChatHistoryStyleUpdate(const LLSD& newvalue);

	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
							   BOOL drop, EDragAndDropType cargo_type,
							   void *cargo_data, EAcceptance *accept,
							   std::string& tooltip_msg);

	/**
	 * Returns true if chat is displayed in multi tabbed floater
	 *         false if chat is displayed in multiple windows
	 */
	static bool isChatMultiTab();

	static void initIMFloater();

	//used as a callback on receiving new IM message
	static void sRemoveTypingIndicator(const LLSD& data);

	static void onIMChicletCreated(const LLUUID& session_id);

	virtual LLTransientFloaterMgr::ETransientGroup getGroup() { return LLTransientFloaterMgr::IM; }

private:
	// process focus events to set a currently active session
	/* virtual */ void onFocusLost();
	/* virtual */ void onFocusReceived();

	BOOL dropCallingCard(LLInventoryItem* item, BOOL drop);
	BOOL dropCategory(LLInventoryCategory* category, BOOL drop);

	BOOL isInviteAllowed() const;
	BOOL inviteToSession(const std::vector<LLUUID>& agent_ids);
	
	static void		onInputEditorFocusReceived( LLFocusableElement* caller, void* userdata );
	static void		onInputEditorFocusLost(LLFocusableElement* caller, void* userdata);
	static void		onInputEditorKeystroke(LLLineEditor* caller, void* userdata);
	void			setTyping(bool typing);
	void			onSlide();
	static void*	createPanelIMControl(void* userdata);
	static void*	createPanelGroupControl(void* userdata);
	static void* 	createPanelAdHocControl(void* userdata);
	// gets a rect that bounds possible positions for the LLIMFloater on a screen (EXT-1111)
	void getAllowedRect(LLRect& rect);

	// Add the "User is typing..." indicator.
	void addTypingIndicator(const LLIMInfo* im_info);

	// Remove the "User is typing..." indicator.
	void removeTypingIndicator(const LLIMInfo* im_info = NULL);

	LLPanelChatControlPanel* mControlPanel;
	LLUUID mSessionID;
	S32 mLastMessageIndex;

	EInstantMessage mDialog;
	LLUUID mOtherParticipantUUID;
	LLChatHistory* mChatHistory;
	LLLineEditor* mInputEditor;
	bool mPositioned;

	std::string mSavedTitle;
	LLUIString mTypingStart;
	bool mMeTyping;
	bool mOtherTyping;
	bool mShouldSendTypingState;
	LLFrameTimer mTypingTimer;
	LLFrameTimer mTypingTimeoutTimer;

	bool mSessionInitialized;
	LLSD mQueuedMsgsForInit;
};


#endif  // LL_IMFLOATER_H
