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
#include "llfloaterimsession.h"
#include "llimview.h"

class LLConversationLogObserver;
struct ConversationParams;

typedef LLIMModel::LLIMSession::SType SessionType;

/*
 * This class represents a particular session(conversation) of any type(im/voice/p2p/group/...) by storing some of session's data.
 * Each LLConversation object has a corresponding visual representation in a form of LLConversationLogListItem.
 */
class LLConversation
{
public:

	LLConversation(const ConversationParams& params);
	LLConversation(const LLIMModel::LLIMSession& session);
	LLConversation(const LLConversation& conversation);

	~LLConversation();

	const SessionType&	getConversationType()	const	{ return mConversationType; }
	const std::string&	getConversationName()	const	{ return mConversationName; }
	const std::string&	getHistoryFileName()	const	{ return mHistoryFileName; }
	const LLUUID&		getSessionID()			const	{ return mSessionID; }
	const LLUUID&		getParticipantID()		const	{ return mParticipantID; }
	const std::string&	getTimestamp()			const	{ return mTimestamp; }
	const U64Seconds&
						getTime()				const	{ return mTime; }
	bool				hasOfflineMessages()	const	{ return mHasOfflineIMs; }

	void setConversationName(std::string conv_name) { mConversationName = conv_name; }
	void setOfflineMessages(bool new_messages) { mHasOfflineIMs = new_messages; }
	bool isOlderThan(U32Days days) const;

	/*
	 * updates last interaction time
	 */
	void updateTimestamp();

	/*
	 * Resets flag of unread offline message to false when im floater with this conversation is opened.
	 */
	void onIMFloaterShown(const LLUUID& session_id);

	/*
	 * returns string representation(in form of: mm/dd/yyyy hh:mm) of time when conversation was started
	 */
	static const std::string createTimestamp(const U64Seconds& utc_time);

private:

	/*
	 * If conversation has unread offline messages sets callback for opening LLFloaterIMSession
	 * with this conversation.
	 */
	void setListenIMFloaterOpened();

	boost::signals2::connection mIMFloaterShowedConnection;

	U64Seconds mTime; // last interaction time
	SessionType		mConversationType;
	std::string		mConversationName;
	std::string		mHistoryFileName;
	LLUUID			mSessionID;
	LLUUID			mParticipantID;
	bool			mHasOfflineIMs;
	std::string		mTimestamp; // last interaction time in form of: mm/dd/yyyy hh:mm
};

/**
 * LLConversationLog stores all agent's conversations.
 * This class is responsible for creating and storing LLConversation objects when im or voice session starts.
 * Also this class saves/retrieves conversations to/from file.
 *
 * Also please note that it may be several conversations with the same sessionID stored in the conversation log.
 * To distinguish two conversations with the same sessionID it's also needed to compare their creation date.
 */

class LLConversationLog : public LLSingleton<LLConversationLog>, LLIMSessionObserver
{
	LLSINGLETON(LLConversationLog);
public:

	void removeConversation(const LLConversation& conversation);

	/**
	 * Returns first conversation with matched session_id
	 */
	const LLConversation*				getConversation(const LLUUID& session_id);
	const std::vector<LLConversation>&	getConversations() { return mConversations; }

	void addObserver(LLConversationLogObserver* observer);
	void removeObserver(LLConversationLogObserver* observer);

	// LLIMSessionObserver triggers
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id, BOOL has_offline_msg);
    virtual void sessionActivated(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id) {}; // Stub
	virtual void sessionRemoved(const LLUUID& session_id){}											// Stub
	virtual void sessionVoiceOrIMStarted(const LLUUID& session_id){};								// Stub
	virtual void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id){};	// Stub

	void notifyObservers();

	void onNewMessageReceived(const LLSD& data);

	/**
	 * public method which is called on viewer exit to save conversation log
	 */
	void cache();
	bool moveLog(const std::string &originDirectory, const std::string &targetDirectory);
	void getListOfBackupLogs(std::vector<std::string>& list_of_backup_logs);
	void deleteBackupLogs();

	void onClearLog();
	void onClearLogResponse(const LLSD& notification, const LLSD& response);

	bool getIsLoggingEnabled() { return mLoggingEnabled; }
	bool isLogEmpty() { return mConversations.empty(); }

	/**
	 * constructs file name in which conversations log will be saved
	 * file name is conversation.log
	 */
	std::string getFileName();
	LLConversation* findConversation(const LLIMModel::LLIMSession* session);

private:

	virtual ~LLConversationLog()
	{
		if (mAvatarNameCacheConnection.connected())
		{
			mAvatarNameCacheConnection.disconnect();
		}
	}
	
	void enableLogging(S32 log_mode);

	/**
	 * adds conversation to the conversation list and notifies observers
	 */
	void logConversation(const LLUUID& session_id, BOOL has_offline_msg);

	void notifyParticularConversationObservers(const LLUUID& session_id, U32 mask);

	bool saveToFile(const std::string& filename);
	bool loadFromFile(const std::string& filename);

	void onAvatarNameCache(const LLUUID& participant_id, const LLAvatarName& av_name, const LLIMModel::LLIMSession* session);

	void createConversation(const LLIMModel::LLIMSession* session);
	void updateConversationTimestamp(LLConversation* conversation);
	void updateConversationName(const LLIMModel::LLIMSession* session, const std::string& name);
	void updateOfflineIMs(const LLIMModel::LLIMSession* session, BOOL new_messages);



	typedef std::vector<LLConversation> conversations_vec_t;
	std::vector<LLConversation>				mConversations;
	std::set<LLConversationLogObserver*>	mObservers;

	LLFriendObserver* mFriendObserver;		// Observer of the LLAvatarTracker instance

	boost::signals2::connection mNewMessageSignalConnection;
	boost::signals2::connection mAvatarNameCacheConnection;

	bool mLoggingEnabled;
};

class LLConversationLogObserver
{
public:

	enum EConversationChange
		{
			CHANGED_TIME = 1, // last interaction time changed
			CHANGED_NAME = 2,  // conversation name changed
			CHANGED_OfflineIMs = 3
		};

	virtual ~LLConversationLogObserver(){}
	virtual void changed() = 0;
	virtual void changed(const LLUUID& session_id, U32 mask){};
};

#endif /* LLCONVERSATIONLOG_H_ */
