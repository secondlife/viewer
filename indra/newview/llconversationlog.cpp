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

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llconversationlog.h"
#include "lltrans.h"

const int CONVERSATION_LIFETIME = 30; // lifetime of LLConversation is 30 days by spec

struct Conversation_params
{
	Conversation_params(time_t time)
	:	mTime(time),
		mTimestamp(LLConversation::createTimestamp(time)),
		mIsConversationPast(true)
	{}

	time_t		mTime;
	std::string	mTimestamp;
	SessionType	mConversationType;
	std::string	mConversationName;
	std::string	mHistoryFileName;
	LLUUID		mSessionID;
	LLUUID		mParticipantID;
	bool		mIsVoice;
	bool		mIsConversationPast;
	bool		mHasOfflineIMs;
};

/************************************************************************/
/*             LLConversation implementation                            */
/************************************************************************/

LLConversation::LLConversation(const Conversation_params& params)
:	mTime(params.mTime),
	mTimestamp(params.mTimestamp),
	mConversationType(params.mConversationType),
	mConversationName(params.mConversationName),
	mHistoryFileName(params.mHistoryFileName),
	mSessionID(params.mSessionID),
	mParticipantID(params.mParticipantID),
	mIsVoice(params.mIsVoice),
	mIsConversationPast(params.mIsConversationPast),
	mHasOfflineIMs(params.mHasOfflineIMs)
{
	setListenIMFloaterOpened();
}

LLConversation::LLConversation(const LLIMModel::LLIMSession& session)
:	mTime(time_corrected()),
	mTimestamp(createTimestamp(mTime)),
	mConversationType(session.mSessionType),
	mConversationName(session.mName),
	mHistoryFileName(session.mHistoryFileName),
	mSessionID(session.mSessionID),
	mParticipantID(session.mOtherParticipantID),
	mIsVoice(session.mStartedAsIMCall),
	mIsConversationPast(false),
	mHasOfflineIMs(session.mHasOfflineMessage)
{
	setListenIMFloaterOpened();
}

LLConversation::LLConversation(const LLConversation& conversation)
{
	mTime				= conversation.getTime();
	mTimestamp			= conversation.getTimestamp();
	mConversationType	= conversation.getConversationType();
	mConversationName	= conversation.getConversationName();
	mHistoryFileName	= conversation.getHistoryFileName();
	mSessionID			= conversation.getSessionID();
	mParticipantID		= conversation.getParticipantID();
	mIsVoice			= conversation.isVoice();
	mIsConversationPast = conversation.isConversationPast();
	mHasOfflineIMs		= conversation.hasOfflineMessages();

	setListenIMFloaterOpened();
}

LLConversation::~LLConversation()
{
	mIMFloaterShowedConnection.disconnect();
}

void LLConversation::setIsVoice(bool is_voice)
{
	if (mIsConversationPast)
		return;

	mIsVoice = is_voice;
}

void LLConversation::onIMFloaterShown(const LLUUID& session_id)
{
	if (mSessionID == session_id)
	{
		mHasOfflineIMs = false;
	}
}

// static
const std::string LLConversation::createTimestamp(const time_t& utc_time)
{
	std::string timeStr;
	LLSD substitution;
	substitution["datetime"] = (S32) utc_time;

	timeStr = "["+LLTrans::getString ("TimeMonth")+"]/["
				 +LLTrans::getString ("TimeDay")+"]/["
				 +LLTrans::getString ("TimeYear")+"] ["
				 +LLTrans::getString ("TimeHour")+"]:["
				 +LLTrans::getString ("TimeMin")+"]";


	LLStringUtil::format (timeStr, substitution);
	return timeStr;
}

bool LLConversation::isOlderThan(U32 days) const
{
	time_t now = time_corrected();
	U32 age = (U32)((now - mTime) / SEC_PER_DAY); // age of conversation in days

	return age > days;
}

void LLConversation::setListenIMFloaterOpened()
{
	LLIMFloater* floater = LLIMFloater::findInstance(mSessionID);

	bool has_offline_ims = !mIsVoice && mHasOfflineIMs;
	bool ims_are_read = LLIMFloater::isVisible(floater) && floater->hasFocus();

	// we don't need to listen for im floater with this conversation is opened
	// if floater is already opened or this conversation doesn't have unread offline messages
	if (has_offline_ims && !ims_are_read)
	{
		mIMFloaterShowedConnection = LLIMFloater::setIMFloaterShowedCallback(boost::bind(&LLConversation::onIMFloaterShown, this, _1));
	}
}

/************************************************************************/
/*             LLConversationLogFriendObserver implementation           */
/************************************************************************/

// Note : An LLSingleton like LLConversationLog cannot be an LLFriendObserver 
// at the same time.
// This is because avatar observers are deleted by the observed object which 
// conflicts with the way LLSingleton are deleted.

class LLConversationLogFriendObserver : public LLFriendObserver
{
public:
	LLConversationLogFriendObserver() {}
	virtual ~LLConversationLogFriendObserver() {}
	virtual void changed(U32 mask);
};

void LLConversationLogFriendObserver::changed(U32 mask)
{
	if (mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE))
	{
		LLConversationLog::instance().notifyObservers();
	}
}

/************************************************************************/
/*             LLConversationLog implementation                         */
/************************************************************************/

LLConversationLog::LLConversationLog()
{
	loadFromFile(getFileName());

	LLControlVariable* ctrl = gSavedPerAccountSettings.getControl("LogInstantMessages").get();
	if (ctrl)
	{
		ctrl->getSignal()->connect(boost::bind(&LLConversationLog::observeIMSession, this));

		if (ctrl->getValue().asBoolean())
		{
			LLIMMgr::instance().addSessionObserver(this);
		}
	}

	mFriendObserver = new LLConversationLogFriendObserver;
	LLAvatarTracker::instance().addObserver(mFriendObserver);

}

void LLConversationLog::observeIMSession()
{
	if (gSavedPerAccountSettings.getBOOL("LogInstantMessages"))
	{
		LLIMMgr::instance().addSessionObserver(this);
	}
	else
	{
		LLIMMgr::instance().removeSessionObserver(this);
	}
}

void LLConversationLog::logConversation(const LLConversation& conversation)
{
	mConversations.push_back(conversation);
	notifyObservers();
}

void LLConversationLog::removeConversation(const LLConversation& conversation)
{
	conversations_vec_t::iterator conv_it = mConversations.begin();
	for(; conv_it != mConversations.end(); ++conv_it)
	{
		if (conv_it->getSessionID() == conversation.getSessionID() && conv_it->getTime() == conversation.getTime())
		{
			mConversations.erase(conv_it);
			notifyObservers();
			return;
		}
	}
}

const LLConversation* LLConversationLog::getConversation(const LLUUID& session_id)
{
	conversations_vec_t::const_iterator conv_it = mConversations.begin();
	for(; conv_it != mConversations.end(); ++conv_it)
	{
		if (conv_it->getSessionID() == session_id)
		{
			return &*conv_it;
		}
	}

	return NULL;
}

void LLConversationLog::addObserver(LLConversationLogObserver* observer)
{
	mObservers.insert(observer);
}

void LLConversationLog::removeObserver(LLConversationLogObserver* observer)
{
	mObservers.erase(observer);
}

void LLConversationLog::sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
	LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(session_id);
	if (session)
	{
		if (LLIMModel::LLIMSession::P2P_SESSION == session->mSessionType)
		{
			LLAvatarNameCache::get(session->mOtherParticipantID, boost::bind(&LLConversationLog::onAvatarNameCache, this, _1, _2, session));
		}
		else
		{
			LLConversation conversation(*session);
			LLConversationLog::instance().logConversation(conversation);
			session->mVoiceChannel->setStateChangedCallback(boost::bind(&LLConversationLog::onVoiceChannelConnected, this, _5, _2));
		}
	}
}

void LLConversationLog::sessionRemoved(const LLUUID& session_id)
{
	conversations_vec_t::reverse_iterator rev_iter = mConversations.rbegin();

	for (; rev_iter != mConversations.rend(); ++rev_iter)
	{
		if (rev_iter->getSessionID() == session_id && !rev_iter->isConversationPast())
		{
			rev_iter->setIsPast(true);
			return; // return here because only one session with session_id may be active
		}
	}
}

void LLConversationLog::cache()
{
	saveToFile(getFileName());
}

std::string LLConversationLog::getFileName()
{
	std::string filename = "conversation";
	return gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, filename) + ".log";
}

bool LLConversationLog::saveToFile(const std::string& filename)
{
	if(!filename.size())
	{
		llwarns << "Call log list filename is empty!" << llendl;
		return false;
	}

	LLFILE* fp = LLFile::fopen(filename, "wb");
	if (!fp)
	{
		llwarns << "Couldn't open call log list" << filename << llendl;
		return false;
	}

	std::string participant_id;
	std::string conversation_id;

	conversations_vec_t::const_iterator conv_it = mConversations.begin();
	for (; conv_it != mConversations.end(); ++conv_it)
	{
		conv_it->getSessionID().toString(conversation_id);
		conv_it->getParticipantID().toString(participant_id);

		// examples of two file entries
		// [1343221177] 0 1 0 John Doe| 7e4ec5be-783f-49f5-71dz-16c58c64c145 4ec62a74-c246-0d25-2af6-846beac2aa55 john.doe|
		// [1343222639] 2 0 0 Ad-hoc Conference| c3g67c89-c479-4c97-b21d-32869bcfe8rc 68f1c33e-4135-3e3e-a897-8c9b23115c09 Ad-hoc Conference hash597394a0-9982-766d-27b8-c75560213b9a|

		fprintf(fp, "[%d] %d %d %d %s| %s %s %s|\n",
				(S32)conv_it->getTime(),
				(S32)conv_it->getConversationType(),
				(S32)conv_it->isVoice(),
				(S32)conv_it->hasOfflineMessages(),
				     conv_it->getConversationName().c_str(),
				participant_id.c_str(),
				conversation_id.c_str(),
				conv_it->getHistoryFileName().c_str());
	}
	fclose(fp);
	return true;
}
bool LLConversationLog::loadFromFile(const std::string& filename)
{
	if(!filename.size())
	{
		llwarns << "Call log list filename is empty!" << llendl;
		return false;
	}

	LLFILE* fp = LLFile::fopen(filename, "rb");
	if (!fp)
	{
		llwarns << "Couldn't open call log list" << filename << llendl;
		return false;
	}

	char buffer[MAX_STRING];
	char conv_name_buffer[MAX_STRING];
	char part_id_buffer[MAX_STRING];
	char conv_id_buffer[MAX_STRING];
	char history_file_name[MAX_STRING];
	int is_voice;
	int has_offline_ims;
	int stype;
	S32 time;

	while (!feof(fp) && fgets(buffer, MAX_STRING, fp))
	{
		conv_name_buffer[0] = '\0';
		part_id_buffer[0]	= '\0';
		conv_id_buffer[0]	= '\0';

		sscanf(buffer, "[%d] %d %d %d %[^|]| %s %s %[^|]|",
				&time,
				&stype,
				&is_voice,
				&has_offline_ims,
				conv_name_buffer,
				part_id_buffer,
				conv_id_buffer,
				history_file_name);

		Conversation_params params(time);
		params.mConversationType = (SessionType)stype;
		params.mIsVoice = is_voice;
		params.mHasOfflineIMs = has_offline_ims;
		params.mConversationName = std::string(conv_name_buffer);
		params.mParticipantID = LLUUID(part_id_buffer);
		params.mSessionID = LLUUID(conv_id_buffer);
		params.mHistoryFileName = std::string(history_file_name);

		LLConversation conversation(params);

		// CHUI-325
		// The conversation log should be capped to the last 30 days. Conversations with the last utterance
		// being over 30 days old should be purged from the conversation log text file on login.
		if (conversation.isOlderThan(CONVERSATION_LIFETIME))
		{
			continue;
		}

		mConversations.push_back(conversation);
	}
	fclose(fp);

	LLFile::remove(filename);
	cache();

	notifyObservers();
	return true;
}

void LLConversationLog::notifyObservers()
{
	std::set<LLConversationLogObserver*>::const_iterator iter = mObservers.begin();
	for (; iter != mObservers.end(); ++iter)
	{
		(*iter)->changed();
	}
}

void LLConversationLog::notifyPrticularConversationObservers(const LLUUID& session_id, U32 mask)
{
	std::set<LLConversationLogObserver*>::const_iterator iter = mObservers.begin();
	for (; iter != mObservers.end(); ++iter)
	{
		(*iter)->changed(session_id, mask);
	}
}

void LLConversationLog::onVoiceChannelConnected(const LLUUID& session_id, const LLVoiceChannel::EState& state)
{
	conversations_vec_t::reverse_iterator rev_iter = mConversations.rbegin();

	for (; rev_iter != mConversations.rend(); ++rev_iter)
	{
		if (rev_iter->getSessionID() == session_id && !rev_iter->isConversationPast() && LLVoiceChannel::STATE_CALL_STARTED == state)
		{
			rev_iter->setIsVoice(true);
			notifyPrticularConversationObservers(session_id, LLConversationLogObserver::VOICE_STATE);
			return; // return here because only one session with session_id may be active
		}
	}
}

void LLConversationLog::onAvatarNameCache(const LLUUID& participant_id, const LLAvatarName& av_name, LLIMModel::LLIMSession* session)
{
	LLConversation conversation(*session);
	conversation.setConverstionName(av_name.getCompleteName());
	LLConversationLog::instance().logConversation(conversation);
	session->mVoiceChannel->setStateChangedCallback(boost::bind(&LLConversationLog::onVoiceChannelConnected, this, _5, _2));
}
