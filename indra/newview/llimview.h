/** 
 * @file LLIMMgr.h
 * @brief Container for Instant Messaging
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLIMVIEW_H
#define LL_LLIMVIEW_H

#include "llfloater.h"
#include "llinstantmessage.h"
#include "lluuid.h"

class LLFloaterChatterBox;
class LLUUID;
class LLFloaterIMPanel;
class LLFriendObserver;
class LLFloaterIM;

class LLIMMgr : public LLSingleton<LLIMMgr>
{
public:
	LLIMMgr();
	virtual ~LLIMMgr();

	// Add a message to a session. The session can keyed to sesion id
	// or agent id.
	void addMessage(const LLUUID& session_id,
					const LLUUID& target_id,
					const char* from,
					const char* msg,
					const char* session_name = NULL,
					EInstantMessage dialog = IM_NOTHING_SPECIAL,
					U32 parent_estate_id = 0,
					const LLUUID& region_id = LLUUID::null,
					const LLVector3& position = LLVector3::zero);

	void addSystemMessage(const LLUUID& session_id, const LLString& message_name, const LLString::format_map_t& args);

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
					  const LLString& voice_session_handle);

	// This removes the panel referenced by the uuid, and then
	// restores internal consistency. The internal pointer is not
	// deleted.
	void removeSession(const LLUUID& session_id);

	void inviteToSession(const LLUUID& session_id, 
						const LLString& session_name, 
						const LLUUID& caller, 
						const LLString& caller_name,
						EInstantMessage type,
						const LLString& session_handle = LLString::null);

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

	void		setFloaterOpen(BOOL open);		/*Flawfinder: ignore*/
	BOOL		getFloaterOpen();

	LLFloaterChatterBox* getFloater();

	// This method is used to go through all active sessions and
	// disable all of them. This method is usally called when you are
	// forced to log out or similar situations where you do not have a
	// good connection.
	void disconnectAllSessions();

	static void	toggle(void*);

	// This is a helper function to determine what kind of im session
	// should be used for the given agent.
	static EInstantMessage defaultIMTypeForAgent(const LLUUID& agent_id);

	BOOL hasSession(const LLUUID& session_id);

	// This method returns the im panel corresponding to the uuid
	// provided. The uuid must be a session id. Returns NULL if there
	// is no matching panel.
	LLFloaterIMPanel* findFloaterBySession(const LLUUID& session_id);

	static LLUUID computeSessionID(EInstantMessage dialog, const LLUUID& other_participant_id);

	void clearPendingVoiceInviation(const LLUUID& session_id);
	
private:
	class LLIMSessionInvite;

	// create a panel and update internal representation for
	// consistency. Returns the pointer, caller (the class instance
	// since it is a private method) is not responsible for deleting
	// the pointer.
	LLFloaterIMPanel* createFloater(const LLUUID& session_id,
									const LLUUID& target_id,
									const std::string& name,
									EInstantMessage dialog,
									BOOL user_initiated = FALSE);

	LLFloaterIMPanel* createFloater(const LLUUID& session_id,
									const LLUUID& target_id,
									const std::string& name,
									const LLDynamicArray<LLUUID>& ids,
									EInstantMessage dialog,
									BOOL user_initiated = FALSE);

	// This simple method just iterates through all of the ids, and
	// prints a simple message if they are not online. Used to help
	// reduce 'hello' messages to the linden employees unlucky enough
	// to have their calling card in the default inventory.
	void noteOfflineUsers(LLFloaterIMPanel* panel, const LLDynamicArray<LLUUID>& ids);

	void processIMTypingCore(const LLIMInfo* im_info, BOOL typing);

	static void inviteUserResponse(S32 option, void* user_data);
	static void onInviteNameLookup(const LLUUID& id, const char* first, const char* last, BOOL is_group, void* userdata);

private:
	std::set<LLViewHandle> mFloaters;
	LLFriendObserver* mFriendObserver;

	// An IM has been received that you haven't seen yet.
	BOOL mIMReceived;

	LLSD mPendingVoiceInvitations;
};


class LLFloaterIM : public LLMultiFloater
{
public:
	LLFloaterIM();
	/*virtual*/ BOOL postBuild();
};

// Globals
extern LLIMMgr *gIMMgr;

#endif  // LL_LLIMView_H
