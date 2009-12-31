/** 
 * @file llimpanel.h
 * @brief LLIMPanel class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_IMPANEL_H
#define LL_IMPANEL_H

#include "lldockablefloater.h"
#include "lllogchat.h"
#include "lluuid.h"
#include "lldarray.h"
#include "llinstantmessage.h"
#include "llvoiceclient.h"
#include "llstyle.h"

class LLLineEditor;
class LLViewerTextEditor;
class LLInventoryItem;
class LLInventoryCategory;
class LLIMSpeakerMgr;
class LLPanelActiveSpeakers;
class LLPanelChatControlPanel;

class LLFloaterIMPanel : public LLFloater
{
public:

	// The session id is the id of the session this is for. The target
	// refers to the user (or group) that where this session serves as
	// the default. For example, if you open a session though a
	// calling card, a new session id will be generated, but the
	// target_id will be the agent referenced by the calling card.
	LLFloaterIMPanel(const std::string& session_label,
					 const LLUUID& session_id,
					 const LLUUID& target_id,
					 const std::vector<LLUUID>& ids,
					 EInstantMessage dialog);
	virtual ~LLFloaterIMPanel();

	/*virtual*/ BOOL postBuild();

	// Check typing timeout timer.
	/*virtual*/ void draw();

	/*virtual*/ void onClose(bool app_quitting);
	void onVisibilityChange(const LLSD& new_visibility);

	// add target ids to the session. 
	// Return TRUE if successful, otherwise FALSE.
	BOOL inviteToSession(const std::vector<LLUUID>& agent_ids);

	void addHistoryLine(const std::string &utf8msg, 
						const LLColor4& color = LLColor4::white, 
						bool log_to_file = true,
						const LLUUID& source = LLUUID::null,
						const std::string& name = LLStringUtil::null);

	void setInputFocus( BOOL b );

	void selectAll();
	void selectNone();

	S32 getNumUnreadMessages() { return mNumUnreadMessages; }

	BOOL handleKeyHere(KEY key, MASK mask);
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
						   BOOL drop, EDragAndDropType cargo_type,
						   void *cargo_data, EAcceptance *accept,
						   std::string& tooltip_msg);

	static void		onInputEditorFocusReceived( LLFocusableElement* caller, void* userdata );
	static void		onInputEditorFocusLost(LLFocusableElement* caller, void* userdata);
	static void		onInputEditorKeystroke(LLLineEditor* caller, void* userdata);
	static void		onCommitChat(LLUICtrl* caller, void* userdata);
	static void		onTabClick( void* userdata );

	static void		onClickProfile( void* userdata );
	static void		onClickGroupInfo( void* userdata );
	static void		onClickClose( void* userdata );
	static void		onClickStartCall( void* userdata );
	static void		onClickEndCall( void* userdata );
	static void		onClickSend( void* userdata );
	static void		onClickToggleActiveSpeakers( void* userdata );
	static void*	createSpeakersPanel(void* data);
	static void		onKickSpeaker(void* user_data);

	//callbacks for P2P muting and volume control
	static void onClickMuteVoice(void* user_data);
	static void onVolumeChange(LLUICtrl* source, void* user_data);

	const LLUUID& getSessionID() const { return mSessionUUID; }
	const LLUUID& getOtherParticipantID() const { return mOtherParticipantUUID; }
	void processSessionUpdate(const LLSD& update);
	EInstantMessage getDialogType() const { return mDialog; }
	void setDialogType(EInstantMessage dialog) { mDialog = dialog; }

	void sessionInitReplyReceived(const LLUUID& im_session_id);

	// Handle other participant in the session typing.
	void processIMTyping(const LLIMInfo* im_info, BOOL typing);

private:
	// Called by UI methods.
	void sendMsg();

	// for adding agents via the UI. Return TRUE if possible, do it if 
	BOOL dropCallingCard(LLInventoryItem* item, BOOL drop);
	BOOL dropCategory(LLInventoryCategory* category, BOOL drop);

	// test if local agent can add agents.
	BOOL isInviteAllowed() const;

	// Called whenever the user starts or stops typing.
	// Sends the typing state to the other user if necessary.
	void setTyping(BOOL typing);

	// Add the "User is typing..." indicator.
	void addTypingIndicator(const std::string &name);

	// Remove the "User is typing..." indicator.
	void removeTypingIndicator(const LLIMInfo* im_info);

	void sendTypingState(BOOL typing);
	
private:
	LLLineEditor* mInputEditor;
	LLViewerTextEditor* mHistoryEditor;

	// The value of the mSessionUUID depends on how the IM session was started:
	//   one-on-one  ==> random id
	//   group ==> group_id
	//   inventory folder ==> folder item_id 
	//   911 ==> Gaurdian_Angel_Group_ID ^ gAgent.getID()
	LLUUID mSessionUUID;

	std::string mSessionLabel;

	BOOL mSessionInitialized;
	LLSD mQueuedMsgsForInit;

	// The value mOtherParticipantUUID depends on how the IM session was started:
	//   one-on-one = recipient's id
	//   group ==> group_id
	//   inventory folder ==> first target id in list
	//   911 ==> sender
	LLUUID mOtherParticipantUUID;
	std::vector<LLUUID> mSessionInitialTargetIDs;

	EInstantMessage mDialog;

	// Are you currently typing?
	BOOL mTyping;

	// Is other user currently typing?
	BOOL mOtherTyping;

	// name of other user who is currently typing
	std::string mOtherTypingName;

	// Where does the "User is typing..." line start?
	S32 mTypingLineStartIndex;
	// Where does the "Starting session..." line start?
	S32 mSessionStartMsgPos;
	
	S32 mNumUnreadMessages;

	BOOL mSentTypingState;

	BOOL mShowSpeakersOnConnect;

	BOOL mTextIMPossible;
	BOOL mProfileButtonEnabled;
	BOOL mCallBackEnabled;

	LLPanelActiveSpeakers* mSpeakerPanel;
	
	// Optimization:  Don't send "User is typing..." until the
	// user has actually been typing for a little while.  Prevents
	// extra IMs for brief "lol" type utterences.
	LLFrameTimer mFirstKeystrokeTimer;

	// Timer to detect when user has stopped typing.
	LLFrameTimer mLastKeystrokeTimer;

	boost::signals2::connection mFocusCallbackConnection;

	void disableWhileSessionStarting();
};

#endif  // LL_IMPANEL_H
