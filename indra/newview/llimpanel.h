/** 
 * @file llimpanel.h
 * @brief LLIMPanel class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llfloater.h"
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

class LLVoiceChannel : public LLVoiceClientStatusObserver
{
public:
	typedef enum e_voice_channel_state
	{
		STATE_NO_CHANNEL_INFO,
		STATE_ERROR,
		STATE_HUNG_UP,
		STATE_READY,
		STATE_CALL_STARTED,
		STATE_RINGING,
		STATE_CONNECTED
	} EState;

	LLVoiceChannel(const LLUUID& session_id, const std::string& session_name);
	virtual ~LLVoiceChannel();

	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

	virtual void handleStatusChange(EStatusType status);
	virtual void handleError(EStatusType status);
	virtual void deactivate();
	virtual void activate();
	virtual void setChannelInfo(
		const std::string& uri,
		const std::string& credentials);
	virtual void getChannelInfo();
	virtual BOOL isActive();
	virtual BOOL callStarted();

	const LLUUID getSessionID() { return mSessionID; }
	EState getState() { return mState; }

	void updateSessionID(const LLUUID& new_session_id);
	const LLStringUtil::format_map_t& getNotifyArgs() { return mNotifyArgs; }

	static LLVoiceChannel* getChannelByID(const LLUUID& session_id);
	static LLVoiceChannel* getChannelByURI(std::string uri);
	static LLVoiceChannel* getCurrentVoiceChannel() { return sCurrentVoiceChannel; }
	static void initClass();
	
	static void suspend();
	static void resume();

protected:
	virtual void setState(EState state);
	void setURI(std::string uri);

	std::string	mURI;
	std::string	mCredentials;
	LLUUID		mSessionID;
	EState		mState;
	std::string	mSessionName;
	LLStringUtil::format_map_t mNotifyArgs;
	BOOL		mIgnoreNextSessionLeave;
	LLHandle<LLPanel> mLoginNotificationHandle;

	typedef std::map<LLUUID, LLVoiceChannel*> voice_channel_map_t;
	static voice_channel_map_t sVoiceChannelMap;

	typedef std::map<std::string, LLVoiceChannel*> voice_channel_map_uri_t;
	static voice_channel_map_uri_t sVoiceChannelURIMap;

	static LLVoiceChannel* sCurrentVoiceChannel;
	static LLVoiceChannel* sSuspendedVoiceChannel;
	static BOOL sSuspended;
};

class LLVoiceChannelGroup : public LLVoiceChannel
{
public:
	LLVoiceChannelGroup(const LLUUID& session_id, const std::string& session_name);

	/*virtual*/ void handleStatusChange(EStatusType status);
	/*virtual*/ void handleError(EStatusType status);
	/*virtual*/ void activate();
	/*virtual*/ void deactivate();
	/*vritual*/ void setChannelInfo(
		const std::string& uri,
		const std::string& credentials);
	/*virtual*/ void getChannelInfo();

protected:
	virtual void setState(EState state);

private:
	U32 mRetries;
	BOOL mIsRetrying;
};

class LLVoiceChannelProximal : public LLVoiceChannel, public LLSingleton<LLVoiceChannelProximal>
{
public:
	LLVoiceChannelProximal();

	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);
	/*virtual*/ void handleStatusChange(EStatusType status);
	/*virtual*/ void handleError(EStatusType status);
	/*virtual*/ BOOL isActive();
	/*virtual*/ void activate();
	/*virtual*/ void deactivate();

};

class LLVoiceChannelP2P : public LLVoiceChannelGroup
{
public:
	LLVoiceChannelP2P(const LLUUID& session_id, const std::string& session_name, const LLUUID& other_user_id);

	/*virtual*/ void handleStatusChange(EStatusType status);
	/*virtual*/ void handleError(EStatusType status);
    /*virtual*/ void activate();
	/*virtual*/ void getChannelInfo();

	void setSessionHandle(const std::string& handle);

protected:
	virtual void setState(EState state);

private:
	std::string	mSessionHandle;
	LLUUID		mOtherUserID;
	BOOL		mReceivedCall;
};

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
					 EInstantMessage dialog);
	LLFloaterIMPanel(const std::string& session_label,
					 const LLUUID& session_id,
					 const LLUUID& target_id,
					 const LLDynamicArray<LLUUID>& ids,
					 EInstantMessage dialog);
	virtual ~LLFloaterIMPanel();

	/*virtual*/ BOOL postBuild();

	// Check typing timeout timer.
	/*virtual*/ void draw();
	/*virtual*/ void onClose(bool app_quitting = FALSE);
	/*virtual*/ void onVisibilityChange(BOOL new_visibility);

	// add target ids to the session. 
	// Return TRUE if successful, otherwise FALSE.
	BOOL inviteToSession(const LLDynamicArray<LLUUID>& agent_ids);

	void addHistoryLine(const std::string &utf8msg, 
						const LLColor4& color = LLColor4::white, 
						bool log_to_file = true,
						const LLUUID& source = LLUUID::null,
						const std::string& name = LLStringUtil::null);

	void setInputFocus( BOOL b );

	void selectAll();
	void selectNone();
	void setVisible(BOOL b);

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
	void updateSpeakersList(const LLSD& speaker_updates);
	void processSessionUpdate(const LLSD& update);
	void setSpeakers(const LLSD& speaker_list);
	LLVoiceChannel* getVoiceChannel() { return mVoiceChannel; }
	EInstantMessage getDialogType() const { return mDialog; }

	void requestAutoConnect();

	void sessionInitReplyReceived(const LLUUID& im_session_id);

	// Handle other participant in the session typing.
	void processIMTyping(const LLIMInfo* im_info, BOOL typing);
	static void chatFromLogFile(LLLogChat::ELogLineType type, std::string line, void* userdata);

	//show error statuses to the user
	void showSessionStartError(const std::string& error_string);
	void showSessionEventError(
		const std::string& event_string,
		const std::string& error_string);
	void showSessionForceClose(const std::string& reason);

	static void onConfirmForceCloseError(S32 option, void* data);

private:
	// called by constructors
	void init(const std::string& session_label);

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

	std::string mSessionLabel;
	LLVoiceChannel*	mVoiceChannel;

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

	// name of other user who is currently typing
	std::string mOtherTypingName;

	// Where does the "User is typing..." line start?
	S32 mTypingLineStartIndex;
	// Where does the "Starting session..." line start?
	S32 mSessionStartMsgPos;
	
	S32 mNumUnreadMessages;

	BOOL mSentTypingState;

	BOOL mShowSpeakersOnConnect;

	BOOL mAutoConnect;

	LLIMSpeakerMgr* mSpeakers;
	LLPanelActiveSpeakers* mSpeakerPanel;
	
	// Optimization:  Don't send "User is typing..." until the
	// user has actually been typing for a little while.  Prevents
	// extra IMs for brief "lol" type utterences.
	LLFrameTimer mFirstKeystrokeTimer;

	// Timer to detect when user has stopped typing.
	LLFrameTimer mLastKeystrokeTimer;

	void disableWhileSessionStarting();

	typedef std::map<LLUUID, LLStyleSP> styleMap;
	static styleMap mStyleMap;
};


#endif  // LL_IMPANEL_H
