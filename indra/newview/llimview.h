/** 
 * @file llimview.h
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

class LLFloaterNewIM;
class LLUUID;
class LLFloaterIMPanel;
class LLFriendObserver;
class LLFloaterIM;

class LLIMView : public LLView
{
public:
	LLIMView(const std::string& name, const LLRect& rect);
	~LLIMView();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	// Add a message to a session. The session can keyed to sesion id
	// or agent id.
	void addMessage(const LLUUID& session_id, const LLUUID& target_id,
					const char* from, const char* msg,
					const char* session_name = NULL,
					EInstantMessage dialog = IM_NOTHING_SPECIAL,
					U32 parent_estate_id = 0,
					const LLUUID& region_id = LLUUID::null,
					const LLVector3& position = LLVector3::zero);

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

	// Adds a session using the given session_id. If the session already exists 
	// the dialog type is assumed correct. Returns the uuid of the session.
	LLUUID addSession(const std::string& name,
					  EInstantMessage dialog,
					  const LLUUID& session_id,
					  const LLDynamicArray<LLUUID>& ids);

	// This removes the panel referenced by the uuid, and then
	// restores internal consistency. The internal pointer is not
	// deleted.
	void removeSession(const LLUUID& session_id);

	void processIMTypingStart(const LLIMInfo* im_info);
	void processIMTypingStop(const LLIMInfo* im_info);

	// Rebuild stuff
	void refresh();

	void notifyNewIM();

	// IM received that you haven't seen yet
	BOOL getIMReceived() const;

	void		setFloaterOpen(BOOL open);		/*Flawfinder: ignore*/
	BOOL		getFloaterOpen();

	LLFloaterIM * getFloater() { return mTalkFloater; }

	// close any sessions which are not available in the newimpanel.
	void pruneSessions();

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

private:
	// create a panel and update internal representation for
	// consistency. Returns the pointer, caller (the class instance
	// since it is a private method) is not responsible for deleting
	// the pointer.
	LLFloaterIMPanel* createFloater(const LLUUID& session_id, const LLUUID& target_id,
							const std::string& name, EInstantMessage dialog, BOOL user_initiated = FALSE);

	// This simple method just iterates through all of the ids, and
	// prints a simple message if they are not online. Used to help
	// reduce 'hello' messages to the linden employees unlucky enough
	// to have their calling card in the default inventory.
	void noteOfflineUsers(LLFloaterIMPanel* panel, const LLDynamicArray<LLUUID>& ids);

	void processIMTypingCore(const LLIMInfo* im_info, BOOL typing);

public:
	LLFloaterIM*		mTalkFloater;
	LLFloaterNewIM*		mNewIMFloater;

private:
	std::set<LLViewHandle> mFloaters;
	LLFriendObserver* mFriendObserver;

	// An IM has been received that you haven't seen yet.
	BOOL mIMReceived;
};


class LLFloaterIM : public LLMultiFloater
{
public:
	LLFloaterIM();
	///*virtual*/ BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point = LLTabContainerCommon::END);
};

// Globals
extern LLIMView *gIMView;

#endif  // LL_LLIMView_H
