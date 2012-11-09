/**
 * @file llimconversation.h
 * @brief LLIMConversation class implements the common behavior of LNearbyChatBar
 * @brief and LLIMFloater for hosting both in LLIMContainer
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_IMCONVERSATION_H
#define LL_IMCONVERSATION_H

#include "lllayoutstack.h"
#include "llparticipantlist.h"
#include "lltransientdockablefloater.h"
#include "llviewercontrol.h"
#include "lleventtimer.h"
#include "llimview.h"
#include "llconversationmodel.h"
#include "llconversationview.h"
#include "lltexteditor.h"

class LLPanelChatControlPanel;
class LLChatEntry;
class LLChatHistory;

class LLIMConversation
	: public LLTransientDockableFloater
{

public:
	LOG_CLASS(LLIMConversation);

	LLIMConversation(const LLSD& session_id);
	~LLIMConversation();

	// reload all message with new settings of visual modes
	static void processChatHistoryStyleUpdate();

	/**
	 * Returns true if chat is displayed in multi tabbed floater
	 *         false if chat is displayed in multiple windows
	 */
	static bool isChatMultiTab();

	// add conversation to container
	static void addToHost(const LLUUID& session_id);

	bool isHostAttached() {return mIsHostAttached;}
	void setHostAttached(bool is_attached) {mIsHostAttached = is_attached;}

    static LLIMConversation* findConversation(const LLUUID& uuid);
    static LLIMConversation* getConversation(const LLUUID& uuid);

	// show/hide the translation check box
	void showTranslationCheckbox(const BOOL visible = FALSE);

	bool isNearbyChat() {return mIsNearbyChat;}

	// LLFloater overrides
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void setVisible(BOOL visible);
	/*virtual*/ void setFocus(BOOL focus);
	
	// Handle the left hand participant list widgets
	void addConversationViewParticipant(LLConversationItem* item);
	void removeConversationViewParticipant(const LLUUID& participant_id);
	void updateConversationViewParticipant(const LLUUID& participant_id);
	void refreshConversation();
	void buildConversationViewParticipant();

	void setSortOrder(const LLConversationSort& order);
	
	virtual void updateMessages() {}

protected:

	// callback for click on any items of the visual states menu
	void onIMSessionMenuItemClicked(const LLSD& userdata);

	// callback for check/uncheck of the expanded/collapse mode's switcher
	bool onIMCompactExpandedMenuItemCheck(const LLSD& userdata);

	//
	bool onIMShowModesMenuItemCheck(const LLSD& userdata);
	bool onIMShowModesMenuItemEnable(const LLSD& userdata);
	static void onSlide(LLIMConversation *self);
	virtual void onTearOffClicked();

	// refresh a visual state of the Call button
	void updateCallBtnState(bool callIsActive);

	void hideOrShowTitle(); // toggle the floater's drag handle
	void hideAllStandardButtons();

	/// Update floater header and toolbar buttons when hosted/torn off state is toggled.
	void updateHeaderAndToolbar();

	// Update the input field help text and other places that need the session name
	virtual void updateSessionName(const std::string& name);

	// set the enable/disable state for the Call button
	virtual void enableDisableCallBtn();

	// process focus events to set a currently active session
	/* virtual */ void onFocusLost();
	/* virtual */ void onFocusReceived();

	// prepare chat's params and out one message to chatHistory
	void appendMessage(const LLChat& chat, const LLSD &args = 0);

	std::string appendTime();

	bool mIsNearbyChat;
	bool mIsP2PChat;

	LLIMModel::LLIMSession* mSession;

	// Participants list: model and view
	LLConversationViewParticipant* createConversationViewParticipant(LLConversationItem* item);
	
	LLUUID mSessionID; 
	LLLayoutPanel* mParticipantListPanel;	// add the widgets to that see mConversationsListPanel
	LLParticipantList* getParticipantList();
	conversations_widgets_map mConversationsWidgets;
	LLConversationViewModel mConversationViewModel;
	LLFolderView* mConversationsRoot;

	LLChatHistory* mChatHistory;
	LLChatEntry* mInputEditor;
	int mInputEditorTopPad; // padding between input field and chat history

	LLButton* mExpandCollapseBtn;
	LLButton* mTearOffBtn;
	LLButton* mCloseBtn;

private:
	/// Refreshes the floater at a constant rate.
	virtual void refresh() = 0;

	/**
	 * Adjusts chat history height to fit vertically with input chat field
	 * and avoid overlapping, since input chat field can be vertically expanded.
	 * Implementation: chat history bottom "follows" top+top_pad of input chat field
	 */
	void reshapeChatHistory();

	bool checkIfTornOff();
    bool mIsHostAttached;

	LLTimer* mRefreshTimer; ///< Defines the rate at which refresh() is called.
};


#endif // LL_IMCONVERSATION_H
