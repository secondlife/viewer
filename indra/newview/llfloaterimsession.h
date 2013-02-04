/** 
 * @file llfloaterimsession.h
 * @brief LLFloaterIMSession class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_FLOATERIMSESSION_H
#define LL_FLOATERIMSESSION_H

#include "llimview.h"
#include "llfloaterimsessiontab.h"
#include "llinstantmessage.h"
#include "lllogchat.h"
#include "lltooldraganddrop.h"
#include "llvoicechannel.h"
#include "llvoiceclient.h"

class LLAvatarName;
class LLButton;
class LLChatEntry;
class LLTextEditor;
class LLPanelChatControlPanel;
class LLChatHistory;
class LLInventoryItem;
class LLInventoryCategory;

typedef boost::signals2::signal<void(const LLUUID& session_id)> floater_showed_signal_t;

/**
 * Individual IM window that appears at the bottom of the screen,
 * optionally "docked" to the bottom tray.
 */
class LLFloaterIMSession
    : public LLVoiceClientStatusObserver
    , public LLFloaterIMSessionTab
{
	LOG_CLASS(LLFloaterIMSession);
public:
	LLFloaterIMSession(const LLUUID& session_id);

	virtual ~LLFloaterIMSession();

	void initIMSession(const LLUUID& session_id);
	void initIMFloater();

	// LLView overrides
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setVisible(BOOL visible);
	/*virtual*/ BOOL getVisible();
	// Check typing timeout timer.

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		EAcceptance* accept,
		std::string& tooltip_msg);

	static LLFloaterIMSession* findInstance(const LLUUID& session_id);
	static LLFloaterIMSession* getInstance(const LLUUID& session_id);

	// LLFloater overrides
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void setDocked(bool docked, bool pop_on_undock = true);
	// Make IM conversion visible and update the message history
	static LLFloaterIMSession* show(const LLUUID& session_id);

	// Toggle panel specified by session_id
	// Returns true iff panel became visible
	static bool toggle(const LLUUID& session_id);

	void sessionInitReplyReceived(const LLUUID& im_session_id);

	// get new messages from LLIMModel
	/*virtual*/ void updateMessages();
	void reloadMessages(bool clean_messages = false);
	static void onSendMsg(LLUICtrl*, void*);
	void sendMsgFromInputEditor();
	void sendMsg(const std::string& msg);

	// callback for LLIMModel on new messages
	// route to specific floater if it is visible
	static void newIMCallback(const LLSD& data);

	// called when docked floater's position has been set by chiclet
	void setPositioned(bool b) { mPositioned = b; };

	void onVisibilityChange(const LLSD& new_visibility);
	bool enableGearMenuItem(const LLSD& userdata);
	void GearDoToSelected(const LLSD& userdata);
	bool checkGearMenuItem(const LLSD& userdata);

	// Implements LLVoiceClientStatusObserver::onChange() to enable the call
	// button when voice is available
	void onChange(EStatusType status, const std::string &channelURI,
			bool proximal);

	virtual LLTransientFloaterMgr::ETransientGroup getGroup() { return LLTransientFloaterMgr::IM; }
	virtual void onVoiceChannelStateChanged(
			const LLVoiceChannel::EState& old_state,
			const LLVoiceChannel::EState& new_state);

	void processIMTyping(const LLIMInfo* im_info, BOOL typing);
	void processAgentListUpdates(const LLSD& body);
	void processSessionUpdate(const LLSD& session_update);

	//used as a callback on receiving new IM message
	static void sRemoveTypingIndicator(const LLSD& data);
	static void onIMChicletCreated(const LLUUID& session_id);
    const LLUUID& getOtherParticipantUUID() {return mOtherParticipantUUID;}

	static boost::signals2::connection setIMFloaterShowedCallback(const floater_showed_signal_t::slot_type& cb);
	static floater_showed_signal_t sIMFloaterShowedSignal;

	bool needsTitleOverwrite() { return mSessionNameUpdatedForTyping && mOtherTyping; }
private:

	/*virtual*/ void refresh();

    /*virtual*/ void onTearOffClicked();
	/*virtual*/ void onClickCloseBtn();

	// Update the window title and input field help text
	/*virtual*/ void updateSessionName(const std::string& name);

	bool dropPerson(LLUUID* person_id, bool drop);

	BOOL isInviteAllowed() const;
	BOOL inviteToSession(const uuid_vec_t& agent_ids);
	static void onInputEditorFocusReceived( LLFocusableElement* caller,void* userdata );
	static void onInputEditorFocusLost(LLFocusableElement* caller, void* userdata);
	static void onInputEditorKeystroke(LLTextEditor* caller, void* userdata);
	void setTyping(bool typing);
	void onAddButtonClicked();
	void addSessionParticipants(const uuid_vec_t& uuids);
	void addP2PSessionParticipants(const LLSD& notification, const LLSD& response, const uuid_vec_t& uuids);
	void sendParticipantsAddedNotification(const uuid_vec_t& uuids);
	bool canAddSelectedToChat(const uuid_vec_t& uuids);

	void onCallButtonClicked();

	void boundVoiceChannel();

	// Add the "User is typing..." indicator.
	void addTypingIndicator(const LLIMInfo* im_info);

	// Remove the "User is typing..." indicator.
	void removeTypingIndicator(const LLIMInfo* im_info = NULL);

	static void closeHiddenIMToasts();

	static void confirmLeaveCallCallback(const LLSD& notification, const LLSD& response);

	S32 mLastMessageIndex;

	EInstantMessage mDialog;
	LLUUID mOtherParticipantUUID;
	bool mPositioned;

	LLUIString mTypingStart;
	bool mMeTyping;
	bool mOtherTyping;
	bool mShouldSendTypingState;
	LLFrameTimer mTypingTimer;
	LLFrameTimer mTypingTimeoutTimer;
	bool mSessionNameUpdatedForTyping;

	bool mSessionInitialized;
	LLSD mQueuedMsgsForInit;

	uuid_vec_t mInvitedParticipants;
	uuid_vec_t mPendingParticipants;

	// connection to voice channel state change signal
	boost::signals2::connection mVoiceChannelStateChangeConnection;
};

#endif  // LL_FLOATERIMSESSION_H
