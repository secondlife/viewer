/**
 * @file llconversationlog.h
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LLCONVERSATIONLOG_H_
#define LLCONVERSATIONLOG_H_

#include "llcallingcard.h"
#include "llimfloater.h"
#include "llimview.h"

class LLConversationLogObserver;
struct Conversation_params;

typedef LLIMModel::LLIMSession::SType SessionType;

/*
 * This class represents a particular session(conversation) of any type(im/voice/p2p/group/...) by storing some of session's data.
 * Each LLConversation object has a corresponding visual representation in a form of LLConversationLogListItem.
 */
class LLConversation
{
public:

	LLConversation(const Conversation_params& params);
	LLConversation(const LLIMModel::LLIMSession& session);
	LLConversation(const LLConversation& conversation);

	~LLConversation();

	const SessionType&	getConversationType()	const	{ return mConversationType; }
	const std::string&	getConversationName()	const	{ return mConversationName; }
	const std::string&	getHistoryFileName()	const	{ return mHistoryFileName; }
	const LLUUID&		getSessionID()			const	{ return mSessionID; }
	const LLUUID&		getParticipantID()		const	{ return mParticipantID; }
	const std::string&	getTimestamp()			const	{ return mTimestamp; }
	const time_t&		getTime()				const	{ return mTime; }
	bool				isVoice()				const	{ return mIsVoice; }
	bool				hasOfflineMessages()	const	{ return mHasOfflineIMs; }

	/*
	 * Resets flag of unread offline message to false when im floater with this conversation is opened.
	 */
	void onIMFloaterShown(const LLUUID& session_id);

	/*
	 * returns string representation(in form of: mm/dd/yyyy hh:mm) of time when conversation was started
	 */
	static const std::string createTimestamp(const time_t& utc_time);

private:

	/*
	 * If conversation has unread offline messages sets callback for opening LLIMFloater
	 * with this conversation.
	 */
	void setListenIMFloaterOpened();

	boost::signals2::connection mIMFloaterShowedConnection;

	time_t			mTime; // start time of conversation
	SessionType		mConversationType;
	std::string		mConversationName;
	std::string		mHistoryFileName;
	LLUUID			mSessionID;
	LLUUID			mParticipantID;
	bool			mIsVoice;
	bool			mHasOfflineIMs;
	std::string		mTimestamp; // conversation start time in form of: mm/dd/yyyy hh:mm
};

/**
 * LLConversationLog stores all agent's conversations.
 * This class is responsible for creating and storing LLConversation objects when im or voice session starts.
 * Also this class saves/retrieves conversations to/from file.
 *
 * Also please note that it may be several conversations with the same sessionID stored in the conversation log.
 * To distinguish two conversations with the same sessionID it's also needed to compare their creation date.
 */

class LLConversationLog : public LLSingleton<LLConversationLog>, LLIMSessionObserver, LLFriendObserver
{
	friend class LLSingleton<LLConversationLog>;
public:

	/**
	 * adds conversation to the conversation list and notifies observers
	 */
	void logConversation(const LLConversation& conversation);
	void removeConversation(const LLConversation& conversation);

	/**
	 * Returns first conversation with matched session_id
	 */
	const LLConversation* getConversation(const LLUUID& session_id);

	void addObserver(LLConversationLogObserver* observer);
	void removeObserver(LLConversationLogObserver* observer);

	const std::vector<LLConversation>& getConversations() { return mConversations; }

	// LLIMSessionObserver triggers
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	virtual void sessionVoiceOrIMStarted(const LLUUID& session_id){}							// Stub
	virtual void sessionRemoved(const LLUUID& session_id){}										// Stub
	virtual void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id){}	// Stub

	// LLFriendObserver trigger
	virtual void changed(U32 mask);

	/**
	 * public method which is called on viewer exit to save conversation log
	 */
	void cache();

private:

	LLConversationLog();
	void notifyObservers();

	/**
	 * constructs file name in which conversations log will be saved
	 * file name template: agentID.call_log.
	 * For example: a086icaa-782d-88d0-ae29-987a55c99sss.call_log
	 */
	std::string getFileName();

	bool saveToFile(const std::string& filename);
	bool loadFromFile(const std::string& filename);

	typedef std::vector<LLConversation> conversations_vec_t;
	std::vector<LLConversation>				mConversations;
	std::set<LLConversationLogObserver*>	mObservers;
};

class LLConversationLogObserver
{
public:
	virtual ~LLConversationLogObserver(){}
	virtual void changed() = 0;
};

#endif /* LLCONVERSATIONLOG_H_ */
