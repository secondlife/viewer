/** 
 * @file llimpanel.h
 * @brief LLIMPanel class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_IMPANEL_H
#define LL_IMPANEL_H

#include "llfloater.h"
#include "lluuid.h"
#include "lldarray.h"
#include "llinstantmessage.h"

class LLLineEditor;
class LLViewerTextEditor;
class LLInventoryItem;
class LLInventoryCategory;

class LLFloaterIMPanel : public LLFloater
{
public:
	// The session id is the id of the session this is for. The target
	// refers to the user (or group) that where this session serves as
	// the default. For example, if you open a session though a
	// calling card, a new session id will be generated, but the
	// target_id will be the agent referenced by the calling card.
	LLFloaterIMPanel(const std::string& name,
					 const LLRect& rect,
					 const std::string& session_label,
					 const LLUUID& session_id,
					 const LLUUID& target_id,
					 EInstantMessage dialog);
	LLFloaterIMPanel(const std::string& name,
					 const LLRect& rect,
					 const std::string& session_label,
					 const LLUUID& session_id,
					 const LLUUID& target_id,
					 const LLDynamicArray<LLUUID>& ids,
					 EInstantMessage dialog);


	/*virtual*/ BOOL postBuild();

	// Check typing timeout timer.
	/*virtual*/ void draw();

	/*virtual*/ void onClose(bool app_quitting = FALSE);

	// add target ids to the session. 
	// Return TRUE if successful, otherwise FALSE.
	BOOL inviteToSession(const LLDynamicArray<LLUUID>& agent_ids);

	void addHistoryLine(const std::string &utf8msg, 
						const LLColor4& color = LLColor4::white, 
						bool log_to_file = true);
	void setInputFocus( BOOL b );

	void selectAll();
	void selectNone();
	void setVisible(BOOL b);

	BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
						   BOOL drop, EDragAndDropType cargo_type,
						   void *cargo_data, EAcceptance *accept,
						   LLString& tooltip_msg);

	static void		onInputEditorFocusReceived( LLUICtrl* caller, void* userdata );
	static void		onInputEditorFocusLost(LLUICtrl* caller, void* userdata);
	static void		onInputEditorKeystroke(LLLineEditor* caller, void* userdata);
	static void		onTabClick( void* userdata );

	static void		onClickProfile( void* userdata );		//  Profile button pressed
	static void		onClickClose( void* userdata );

	const LLUUID& getSessionID() const { return mSessionUUID; }
	const LLUUID& getOtherParticipantID() const { return mOtherParticipantUUID; }
	const EInstantMessage getDialogType() const { return mDialog; }

	void sessionInitReplyReceived(const LLUUID& im_session_id);

	// Handle other participant in the session typing.
	void processIMTyping(const LLIMInfo* im_info, BOOL typing);
	static void chatFromLogFile(LLString line, void* userdata);

private:
	// called by constructors
	void init(const LLString& session_label);

	// Called by UI methods.
	void sendMsg();

	// for adding agents via the UI. Return TRUE if possible, do it if 
	BOOL dropCallingCard(LLInventoryItem* item, BOOL drop);
	BOOL dropCategory(LLInventoryCategory* category, BOOL drop);

	// test if local agent can add agents.
	BOOL isAddAllowed() const;

	// Called whenever the user starts or stops typing.
	// Sends the typing state to the other user if necessary.
	void setTyping(BOOL typing);

	// Add the "User is typing..." indicator.
	void addTypingIndicator(const LLIMInfo* im_info);

	// Remove the "User is typing..." indicator.
	void removeTypingIndicator();

	void sendTypingState(BOOL typing);
	
	static LLFloaterIMPanel* sInstance;

private:
	LLLineEditor* mInputEditor;
	LLViewerTextEditor* mHistoryEditor;

	// The value of the mSessionUUID depends on how the IM session was started:
	//   one-on-one  ==> random id
	//   group ==> group_id
	//   inventory folder ==> folder item_id 
	//   911 ==> Gaurdian_Angel_Group_ID ^ gAgent.getID()
	LLUUID mSessionUUID;

	BOOL mSessionInitRequested;
	BOOL mSessionInitialized;
	LLSD mQueuedMsgsForInit;

	// The value mOtherParticipantUUID depends on how the IM session was started:
	//   one-on-one = recipient's id
	//   group ==> group_id
	//   inventory folder ==> first target id in list
	//   911 ==> sender
	LLUUID mOtherParticipantUUID;
	LLDynamicArray<LLUUID> mSessionInitialTargetIDs;

	EInstantMessage mDialog;

	// Are you currently typing?
	BOOL mTyping;

	// Is other user currently typing?
	BOOL mOtherTyping;

	// Where does the "User is typing..." line start?
	S32 mTypingLineStartIndex;
	//Where does the "Starting session..." line start?
	S32 mSessionStartMsgPos;

	BOOL mSentTypingState;
	
	// Optimization:  Don't send "User is typing..." until the
	// user has actually been typing for a little while.  Prevents
	// extra IMs for brief "lol" type utterences.
	LLFrameTimer mFirstKeystrokeTimer;

	// Timer to detect when user has stopped typing.
	LLFrameTimer mLastKeystrokeTimer;

	void disableWhileSessionStarting();
};


#endif  // LL_IMPANEL_H
