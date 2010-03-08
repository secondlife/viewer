/** 
 * @file LLIMMgr.h
 * @brief Container for Instant Messaging
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

#ifndef LL_LLIMVIEW_H
#define LL_LLIMVIEW_H

#include "lldockablefloater.h"
#include "llinstantmessage.h"

#include "lllogchat.h"
#include "llvoicechannel.h"



class LLFriendObserver;
class LLCallDialogManager;	
class LLIMSpeakerMgr;


class LLIMModel :  public LLSingleton<LLIMModel>
{
public:

	struct LLIMSession
	{
		typedef enum e_session_type
		{   // for now we have 4 predefined types for a session
			P2P_SESSION,
			GROUP_SESSION,
			ADHOC_SESSION,
			AVALINE_SESSION,
		} SType;

		LLIMSession(const LLUUID& session_id, const std::string& name, 
			const EInstantMessage& type, const LLUUID& other_participant_id, const std::vector<LLUUID>& ids, bool voice);
		virtual ~LLIMSession();

		void sessionInitReplyReceived(const LLUUID& new_session_id);
		void addMessagesFromHistory(const std::list<LLSD>& history);
		void addMessage(const std::string& from, const LLUUID& from_id, const std::string& utf8_text, const std::string& time, const bool is_history = false);
		void onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state, const LLVoiceChannel::EDirection& direction);
		
		/** @deprecated */
		static void chatFromLogFile(LLLogChat::ELogLineType type, const LLSD& msg, void* userdata);

		bool isAdHoc();
		bool isP2P();
		bool isOtherParticipantAvaline();

		bool isP2PSessionType() const { return mSessionType == P2P_SESSION;}
		bool isAdHocSessionType() const { return mSessionType == ADHOC_SESSION;}
		bool isGroupSessionType() const { return mSessionType == GROUP_SESSION;}
		bool isAvalineSessionType() const { return mSessionType == AVALINE_SESSION;}

		//*TODO make private
		/** ad-hoc sessions involve sophisticated chat history file naming schemes */
		void buildHistoryFileName();

		//*TODO make private
		static std::string generateHash(const std::set<LLUUID>& sorted_uuids);

		LLUUID mSessionID;
		std::string mName;
		EInstantMessage mType;
		SType mSessionType;
		LLUUID mOtherParticipantID;
		std::vector<LLUUID> mInitialTargetIDs;
		std::string mHistoryFileName;

		// connection to voice channel state change signal
		boost::signals2::connection mVoiceChannelStateChangeConnection;

		//does NOT include system messages and agent's messages
		S32 mParticipantUnreadMessageCount;

		// does include all incoming messages
		S32 mNumUnread;

		std::list<LLSD> mMsgs;

		LLVoiceChannel* mVoiceChannel;
		LLIMSpeakerMgr* mSpeakers;

		bool mSessionInitialized;

		//true if calling back the session URI after the session has closed is possible.
		//Currently this will be false only for PSTN P2P calls.
		bool mCallBackEnabled;

		bool mTextIMPossible;
		bool mOtherParticipantIsAvatar;
		bool mStartCallOnInitialize;

		//if IM session is created for a voice call
		bool mStartedAsIMCall;
	};
	

	LLIMModel();


	//we should control the currently active session
	LLUUID	mActiveSessionID;
	void	setActiveSessionID(const LLUUID& session_id);
	void	resetActiveSessionID() { mActiveSessionID.setNull(); }
	LLUUID	getActiveSessionID() { return mActiveSessionID; }

	/** Session id to session object */
	std::map<LLUUID, LLIMSession*> mId2SessionMap;

	typedef boost::signals2::signal<void(const LLSD&)> session_signal_t;
	typedef boost::function<void(const LLSD&)> session_callback_t;
	session_signal_t mNewMsgSignal;
	session_signal_t mNoUnreadMsgsSignal;
	
	/** 
	 * Find an IM Session corresponding to session_id
	 * Returns NULL if the session does not exist
	 */
	LLIMSession* findIMSession(const LLUUID& session_id) const;

	/** 
	 * Find an Ad-Hoc IM Session with specified participants
	 * @return first found Ad-Hoc session or NULL if the session does not exist
	 */
	LLIMSession* findAdHocIMSession(const std::vector<LLUUID>& ids);

	/**
	 * Rebind session data to a new session id.
	 */
	void processSessionInitializedReply(const LLUUID& old_session_id, const LLUUID& new_session_id);

	boost::signals2::connection addNewMsgCallback( session_callback_t cb ) { return mNewMsgSignal.connect(cb); }
	boost::signals2::connection addNoUnreadMsgsCallback( session_callback_t cb ) { return mNoUnreadMsgsSignal.connect(cb); }

	/**
	 * Create new session object in a model
	 * @param name session name should not be empty, will return false if empty
	 */
	bool newSession(const LLUUID& session_id, const std::string& name, const EInstantMessage& type, const LLUUID& other_participant_id, 
		const std::vector<LLUUID>& ids, bool voice = false);

	bool newSession(const LLUUID& session_id, const std::string& name, const EInstantMessage& type,
		const LLUUID& other_participant_id, bool voice = false);

	/**
	 * Remove all session data associated with a session specified by session_id
	 */
	bool clearSession(const LLUUID& session_id);

	/**
	 * Populate supplied std::list with messages starting from index specified by start_index
	 */
	void getMessages(const LLUUID& session_id, std::list<LLSD>& messages, int start_index = 0);

	/**
	 * Add a message to an IM Model - the message is saved in a message store associated with a session specified by session_id
	 * and also saved into a file if log2file is specified.
	 * It sends new message signal for each added message.
	 */
	bool addMessage(const LLUUID& session_id, const std::string& from, const LLUUID& other_participant_id, const std::string& utf8_text, bool log2file = true);

	/**
	 * Similar to addMessage(...) above but won't send a signal about a new message added
	 */
	LLIMModel::LLIMSession* addMessageSilently(const LLUUID& session_id, const std::string& from, const LLUUID& from_id, 
		const std::string& utf8_text, bool log2file = true);

	/**
	 * Add a system message to an IM Model
	 */
	bool proccessOnlineOfflineNotification(const LLUUID& session_id, const std::string& utf8_text);

	/**
	 * Get a session's name. 
	 * For a P2P chat - it's an avatar's name, 
	 * For a group chat - it's a group's name
	 * For an ad-hoc chat - is received from the server and is in a from of "<Avatar's name> conference"
	 */
	const std::string& getName(const LLUUID& session_id) const;

	/** 
	 * Get number of unread messages in a session with session_id
	 * Returns -1 if the session with session_id doesn't exist
	 */
	const S32 getNumUnread(const LLUUID& session_id) const;

	/**
	 * Get uuid of other participant in a session with session_id
	 * Returns LLUUID::null if the session doesn't exist
	 *
 	 * *TODO what to do with other participants in ad-hoc and group chats?
	 */
	const LLUUID& getOtherParticipantID(const LLUUID& session_id) const;

	/**
	 * Get type of a session specified by session_id
	 * Returns EInstantMessage::IM_COUNT if the session does not exist
	 */
	EInstantMessage getType(const LLUUID& session_id) const;

	/**
	 * Get voice channel for the session specified by session_id
	 * Returns NULL if the session does not exist
	 */
	LLVoiceChannel* getVoiceChannel(const LLUUID& session_id) const;

	/**
	* Get im speaker manager for the session specified by session_id
	* Returns NULL if the session does not exist
	*/
	LLIMSpeakerMgr* getSpeakerManager(const LLUUID& session_id) const;

	const std::string& getHistoryFileName(const LLUUID& session_id) const;

	static void sendLeaveSession(const LLUUID& session_id, const LLUUID& other_participant_id);
	static bool sendStartSession(const LLUUID& temp_session_id, const LLUUID& other_participant_id,
						  const std::vector<LLUUID>& ids, EInstantMessage dialog);
	static void sendTypingState(LLUUID session_id, LLUUID other_participant_id, BOOL typing);
	static void sendMessage(const std::string& utf8_text, const LLUUID& im_session_id,
								const LLUUID& other_participant_id, EInstantMessage dialog);

	void testMessages();

	/**
	 * Saves an IM message into a file
	 */
	bool logToFile(const std::string& file_name, const std::string& from, const LLUUID& from_id, const std::string& utf8_text);

private:
	
	/**
	 * Add message to a list of message associated with session specified by session_id
	 */
	bool addToHistory(const LLUUID& session_id, const std::string& from, const LLUUID& from_id, const std::string& utf8_text); 

	/**
	 * Save an IM message into a file
	 */
	bool logToFile(const LLUUID& session_id, const std::string& from, const LLUUID& from_id, const std::string& utf8_text);
};

class LLIMSessionObserver
{
public:
	virtual ~LLIMSessionObserver() {}
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id) = 0;
	virtual void sessionRemoved(const LLUUID& session_id) = 0;
	virtual void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id) = 0;
};


class LLIMMgr : public LLSingleton<LLIMMgr>
{
	friend class LLIMModel;

public:
	enum EInvitationType
	{
		INVITATION_TYPE_INSTANT_MESSAGE = 0,
		INVITATION_TYPE_VOICE = 1,
		INVITATION_TYPE_IMMEDIATE = 2
	};

	LLIMMgr();
	virtual ~LLIMMgr() {};

	// Add a message to a session. The session can keyed to sesion id
	// or agent id.
	void addMessage(const LLUUID& session_id,
					const LLUUID& target_id,
					const std::string& from,
					const std::string& msg,
					const std::string& session_name = LLStringUtil::null,
					EInstantMessage dialog = IM_NOTHING_SPECIAL,
					U32 parent_estate_id = 0,
					const LLUUID& region_id = LLUUID::null,
					const LLVector3& position = LLVector3::zero,
					bool link_name = false);

	void addSystemMessage(const LLUUID& session_id, const std::string& message_name, const LLSD& args);

	// This adds a session to the talk view. The name is the local
	// name of the session, dialog specifies the type of
	// session. Since sessions can be keyed off of first recipient or
	// initiator, the session can be matched against the id
	// provided. If the session exists, it is brought forward.  This
	// method accepts a group id or an agent id. Specifying id = NULL
	// results in an im session to everyone. Returns the uuid of the
	// session.
	LLUUID addSession(const std::string& name,
					  EInstantMessage dialog,
					  const LLUUID& other_participant_id, bool voice = false);

	// Adds a session using a specific group of starting agents
	// the dialog type is assumed correct. Returns the uuid of the session.
	LLUUID addSession(const std::string& name,
					  EInstantMessage dialog,
					  const LLUUID& other_participant_id,
					  const LLDynamicArray<LLUUID>& ids, bool voice = false);

	/**
	 * Creates a P2P session with the requisite handle for responding to voice calls.
	 * 
	 * @param name session name, cannot be null
	 * @param caller_uri - sip URI of caller. It should be always be passed into the method to avoid
	 * incorrect working of LLVoiceChannel instances. See EXT-2985.
	 */	
	LLUUID addP2PSession(const std::string& name,
					  const LLUUID& other_participant_id,
					  const std::string& voice_session_handle,
					  const std::string& caller_uri);

	/**
	 * Leave the session with session id. Send leave session notification
	 * to the server and removes all associated session data
	 * @return false if the session with specified id was not exist
	 */
	bool leaveSession(const LLUUID& session_id);

	void inviteToSession(
		const LLUUID& session_id, 
		const std::string& session_name, 
		const LLUUID& caller, 
		const std::string& caller_name,
		EInstantMessage type,
		EInvitationType inv_type, 
		const std::string& session_handle = LLStringUtil::null,
		const std::string& session_uri = LLStringUtil::null);

	void processIMTypingStart(const LLIMInfo* im_info);
	void processIMTypingStop(const LLIMInfo* im_info);

	// automatically start a call once the session has initialized
	void autoStartCallOnStartup(const LLUUID& session_id);

	// Calc number of all unread IMs
	S32 getNumberOfUnreadIM();

	/**
	 * Calculates number of unread IMs from real participants in all stored sessions
	 */
	S32 getNumberOfUnreadParticipantMessages();

	// This method is used to go through all active sessions and
	// disable all of them. This method is usally called when you are
	// forced to log out or similar situations where you do not have a
	// good connection.
	void disconnectAllSessions();

	BOOL hasSession(const LLUUID& session_id);

	static LLUUID computeSessionID(EInstantMessage dialog, const LLUUID& other_participant_id);

	void clearPendingInvitation(const LLUUID& session_id);

	void processAgentListUpdates(const LLUUID& session_id, const LLSD& body);
	LLSD getPendingAgentListUpdates(const LLUUID& session_id);
	void addPendingAgentListUpdates(
		const LLUUID& sessioN_id,
		const LLSD& updates);
	void clearPendingAgentListUpdates(const LLUUID& session_id);

	void addSessionObserver(LLIMSessionObserver *);
	void removeSessionObserver(LLIMSessionObserver *);

	//show error statuses to the user
	void showSessionStartError(const std::string& error_string, const LLUUID session_id);
	void showSessionEventError(const std::string& event_string, const std::string& error_string, const LLUUID session_id);
	void showSessionForceClose(const std::string& reason, const LLUUID session_id);
	static bool onConfirmForceCloseError(const LLSD& notification, const LLSD& response);

	/**
	 * Start call in a session
	 * @return false if voice channel doesn't exist
	 **/
	bool startCall(const LLUUID& session_id, LLVoiceChannel::EDirection direction = LLVoiceChannel::OUTGOING_CALL);

	/**
	 * End call in a session
	 * @return false if voice channel doesn't exist
	 **/
	bool endCall(const LLUUID& session_id);

	bool isVoiceCall(const LLUUID& session_id);

private:

	/**
	 * Remove data associated with a particular session specified by session_id
	 */
	void removeSession(const LLUUID& session_id);

	// This simple method just iterates through all of the ids, and
	// prints a simple message if they are not online. Used to help
	// reduce 'hello' messages to the linden employees unlucky enough
	// to have their calling card in the default inventory.
	void noteOfflineUsers(const LLUUID& session_id, const LLDynamicArray<LLUUID>& ids);
	void noteMutedUsers(const LLUUID& session_id, const LLDynamicArray<LLUUID>& ids);

	void processIMTypingCore(const LLIMInfo* im_info, BOOL typing);

	static void onInviteNameLookup(LLSD payload, const LLUUID& id, const std::string& name, bool is_group);

	void notifyObserverSessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	void notifyObserverSessionRemoved(const LLUUID& session_id);
	void notifyObserverSessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);

private:
	
	typedef std::list <LLIMSessionObserver *> session_observers_list_t;
	session_observers_list_t mSessionObservers;

	LLSD mPendingInvitations;
	LLSD mPendingAgentListUpdates;
};

class LLCallDialogManager : public LLInitClass<LLCallDialogManager>
{
public:
	LLCallDialogManager();
	~LLCallDialogManager();

	static void initClass();
	static void onVoiceChannelChanged(const LLUUID &session_id);
	static void onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state, const LLVoiceChannel::EDirection& direction, bool ended_by_agent);

protected:
	static std::string sPreviousSessionlName;
	static LLIMModel::LLIMSession::SType sPreviousSessionType;
	static std::string sCurrentSessionlName;
	static LLIMModel::LLIMSession* sSession;
	static LLVoiceChannel::EState sOldState;
};

class LLCallDialog : public LLDockableFloater
{
public:
	LLCallDialog(const LLSD& payload);
	~LLCallDialog() {}

	virtual BOOL postBuild();

	// check timer state
	/*virtual*/ void draw();
	/*virtual*/ void onOpen(const LLSD& key);

protected:
	// lifetime timer for a notification
	LLTimer	mLifetimeTimer;
	// notification's lifetime in seconds
	S32		mLifetime;
	static const S32 DEFAULT_LIFETIME = 5;
	virtual bool lifetimeHasExpired();
	virtual void onLifetimeExpired();

	virtual void getAllowedRect(LLRect& rect);

	/**
	 * Sets icon depend on session.
	 *
	 * If passed session_id is a group id group icon will be shown, otherwise avatar icon for participant_id
	 *
	 * @param session_id - UUID of session
	 * @param participant_id - UUID of other participant
	 */
	void setIcon(const LLSD& session_id, const LLSD& participant_id);

	LLSD mPayload;
};

class LLIncomingCallDialog : public LLCallDialog
{
public:
	LLIncomingCallDialog(const LLSD& payload);

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	static void onAccept(void* user_data);
	static void onReject(void* user_data);
	static void onStartIM(void* user_data);

private:
	/*virtual*/ void onLifetimeExpired();
	void processCallResponse(S32 response);
};

class LLOutgoingCallDialog : public LLCallDialog
{
public:
	LLOutgoingCallDialog(const LLSD& payload);

	/*virtual*/ BOOL postBuild();
	void show(const LLSD& key);

	static void onCancel(void* user_data);
	static const LLUUID OCD_KEY;

private:
	// hide all text boxes
	void hideAllText();
};

// Globals
extern LLIMMgr *gIMMgr;

#endif  // LL_LLIMView_H
