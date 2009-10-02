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

#include "lldarray.h"
#include "llfloateractivespeakers.h" //for LLIMSpeakerMgr
#include "llimpanel.h" //for voice channels
#include "llmodaldialog.h"
#include "llinstantmessage.h"
#include "lluuid.h"
#include "llmultifloater.h"

class LLFloaterChatterBox;
class LLUUID;
class LLFloaterIMPanel;
class LLFriendObserver;

class LLIMModel :  public LLSingleton<LLIMModel>
{
public:

	struct LLIMSession
	{
		LLIMSession(const LLUUID& session_id, const std::string& name, 
			const EInstantMessage& type, const LLUUID& other_participant_id);
		virtual ~LLIMSession();

		LLUUID mSessionID;
		std::string mName;
		EInstantMessage mType;
		LLUUID mOtherParticipantID;
		S32 mNumUnread;
		std::list<LLSD> mMsgs;

		LLVoiceChannel* mVoiceChannel;
		LLIMSpeakerMgr* mSpeakers;
	};
	

	LLIMModel();

	//*TODO make it non-static as LLIMMOdel is a singleton (IB)
	static std::map<LLUUID, LLIMSession*> sSessionsMap;  //mapping session_id to session

	boost::signals2::signal<void(const LLSD&)> mChangedSignal;
	
	/** 
	 * Find an IM Session corresponding to session_id
	 * Returns NULL if the session does not exist
	 */
	LLIMSession* findIMSession(const LLUUID& session_id) const;

	/**
	 * Rebind session data to a new session id.
	 */
	void updateSessionID(const LLUUID& old_session_id, const LLUUID& new_session_id);

	boost::signals2::connection addChangedCallback( boost::function<void (const LLSD& data)> cb );

	bool newSession(LLUUID session_id, std::string name, EInstantMessage type, LLUUID other_participant_id);
	bool clearSession(LLUUID session_id);
	std::list<LLSD> getMessages(LLUUID session_id, int start_index = 0);
	bool addMessage(LLUUID session_id, std::string from, LLUUID other_participant_id, std::string utf8_text);
	bool addToHistory(LLUUID session_id, std::string from, std::string utf8_text); 
	//used to get the name of the session, for use as the title
	//currently just the other avatar name
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

	static void sendLeaveSession(LLUUID session_id, LLUUID other_participant_id);
	static bool sendStartSession(const LLUUID& temp_session_id, const LLUUID& other_participant_id,
						  const std::vector<LLUUID>& ids, EInstantMessage dialog);
	static void sendTypingState(LLUUID session_id, LLUUID other_participant_id, BOOL typing);
	static void sendMessage(const std::string& utf8_text, const LLUUID& im_session_id,
								const LLUUID& other_participant_id, EInstantMessage dialog);

	void testMessages();
};

class LLIMSessionObserver
{
public:
	virtual ~LLIMSessionObserver() {}
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id) = 0;
	virtual void sessionRemoved(const LLUUID& session_id) = 0;
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
	virtual ~LLIMMgr();

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

	// This method returns TRUE if the local viewer has a session
	// currently open keyed to the uuid. The uuid can be keyed by
	// either session id or agent id.
	BOOL isIMSessionOpen(const LLUUID& uuid);

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
					  const LLUUID& other_participant_id);

	// Adds a session using a specific group of starting agents
	// the dialog type is assumed correct. Returns the uuid of the session.
	LLUUID addSession(const std::string& name,
					  EInstantMessage dialog,
					  const LLUUID& other_participant_id,
					  const LLDynamicArray<LLUUID>& ids);

	// Creates a P2P session with the requisite handle for responding to voice calls
	LLUUID addP2PSession(const std::string& name,
					  const LLUUID& other_participant_id,
					  const std::string& voice_session_handle,
					  const std::string& caller_uri = LLStringUtil::null);

	// This removes the panel referenced by the uuid, and then
	// restores internal consistency. The internal pointer is not
	// deleted.
	void removeSession(LLUUID session_id);

	void inviteToSession(
		const LLUUID& session_id, 
		const std::string& session_name, 
		const LLUUID& caller, 
		const std::string& caller_name,
		EInstantMessage type,
		EInvitationType inv_type, 
		const std::string& session_handle = LLStringUtil::null,
		const std::string& session_uri = LLStringUtil::null);

	//Updates a given session's session IDs.  Does not open,
	//create or do anything new.  If the old session doesn't
	//exist, then nothing happens.
	void updateFloaterSessionID(const LLUUID& old_session_id,
								const LLUUID& new_session_id);

	void processIMTypingStart(const LLIMInfo* im_info);
	void processIMTypingStop(const LLIMInfo* im_info);

	// Rebuild stuff
	void refresh();

	void notifyNewIM();
	void clearNewIMNotification();

	// IM received that you haven't seen yet
	BOOL getIMReceived() const;

	// Calc number of unread IMs
	S32 getNumberOfUnreadIM();

	// This method is used to go through all active sessions and
	// disable all of them. This method is usally called when you are
	// forced to log out or similar situations where you do not have a
	// good connection.
	void disconnectAllSessions();

	// This is a helper function to determine what kind of im session
	// should be used for the given agent.
	static EInstantMessage defaultIMTypeForAgent(const LLUUID& agent_id);

	BOOL hasSession(const LLUUID& session_id);

	// This method returns the im panel corresponding to the uuid
	// provided. The uuid must be a session id. Returns NULL if there
	// is no matching panel.
	LLFloaterIMPanel* findFloaterBySession(const LLUUID& session_id);

	static LLUUID computeSessionID(EInstantMessage dialog, const LLUUID& other_participant_id);

	void clearPendingInvitation(const LLUUID& session_id);

	LLSD getPendingAgentListUpdates(const LLUUID& session_id);
	void addPendingAgentListUpdates(
		const LLUUID& sessioN_id,
		const LLSD& updates);
	void clearPendingAgentListUpdates(const LLUUID& session_id);

	//HACK: need a better way of enumerating existing session, or listening to session create/destroy events
	const std::set<LLHandle<LLFloater> >& getIMFloaterHandles() { return mFloaters; }

	void addSessionObserver(LLIMSessionObserver *);
	void removeSessionObserver(LLIMSessionObserver *);

private:
	// create a panel and update internal representation for
	// consistency. Returns the pointer, caller (the class instance
	// since it is a private method) is not responsible for deleting
	// the pointer.
	LLFloaterIMPanel* createFloater(const LLUUID& session_id,
									const LLUUID& target_id,
									const std::string& name,
									EInstantMessage dialog,
									BOOL user_initiated = FALSE, 
									const LLDynamicArray<LLUUID>& ids = LLDynamicArray<LLUUID>());

	// This simple method just iterates through all of the ids, and
	// prints a simple message if they are not online. Used to help
	// reduce 'hello' messages to the linden employees unlucky enough
	// to have their calling card in the default inventory.
	void noteOfflineUsers(LLFloaterIMPanel* panel, const LLDynamicArray<LLUUID>& ids);
	void noteMutedUsers(LLFloaterIMPanel* panel, const LLDynamicArray<LLUUID>& ids);

	void processIMTypingCore(const LLIMInfo* im_info, BOOL typing);

	static void onInviteNameLookup(LLSD payload, const LLUUID& id, const std::string& first, const std::string& last, BOOL is_group);

	void notifyObserverSessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	void notifyObserverSessionRemoved(const LLUUID& session_id);

private:
	std::set<LLHandle<LLFloater> > mFloaters;
	LLFriendObserver* mFriendObserver;

	typedef std::list <LLIMSessionObserver *> session_observers_list_t;
	session_observers_list_t mSessionObservers;

	// An IM has been received that you haven't seen yet.
	BOOL mIMReceived;

	LLSD mPendingInvitations;
	LLSD mPendingAgentListUpdates;
	// ID of a session that is being removed: observers are already told
	// that this session is being removed, but it is still present in the sessions' map
	LLUUID	mBeingRemovedSessionID;
};

class LLIncomingCallDialog : public LLModalDialog
{
public:
	LLIncomingCallDialog(const LLSD& payload);

	/*virtual*/ BOOL postBuild();

	static void onAccept(void* user_data);
	static void onReject(void* user_data);
	static void onStartIM(void* user_data);

private:
	void processCallResponse(S32 response);

	LLSD mPayload;
};

// Globals
extern LLIMMgr *gIMMgr;

#endif  // LL_LLIMView_H
