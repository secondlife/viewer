/** 
 * @file LLIMMgr.cpp
 * @brief Container for Instant Messaging
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llimview.h"

#include "llavatarnamecache.h"	// IDEVO
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llbutton.h"
#include "llhttpclient.h"
#include "llsdutil_math.h"
#include "llstring.h"
#include "lltextutil.h"
#include "lltrans.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llagentui.h"
#include "llappviewer.h"
#include "llavatariconctrl.h"
#include "llcallingcard.h"
#include "llchat.h"
#include "llimfloater.h"
#include "llgroupiconctrl.h"
#include "llmd5.h"
#include "llmutelist.h"
#include "llrecentpeople.h"
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llnearbychat.h"
#include "llspeakers.h" //for LLIMSpeakerMgr
#include "lltextbox.h"
#include "lltoolbarview.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"


const static std::string ADHOC_NAME_SUFFIX(" Conference");

const static std::string NEARBY_P2P_BY_OTHER("nearby_P2P_by_other");
const static std::string NEARBY_P2P_BY_AGENT("nearby_P2P_by_agent");

/** Timeout of outgoing session initialization (in seconds) */
const static U32 SESSION_INITIALIZATION_TIMEOUT = 30;

std::string LLCallDialogManager::sPreviousSessionlName = "";
LLIMModel::LLIMSession::SType LLCallDialogManager::sPreviousSessionType = LLIMModel::LLIMSession::P2P_SESSION;
std::string LLCallDialogManager::sCurrentSessionlName = "";
LLIMModel::LLIMSession* LLCallDialogManager::sSession = NULL;
LLVoiceChannel::EState LLCallDialogManager::sOldState = LLVoiceChannel::STATE_READY;
const LLUUID LLOutgoingCallDialog::OCD_KEY = LLUUID("7CF78E11-0CFE-498D-ADB9-1417BF03DDB4");
//
// Globals
//
LLIMMgr* gIMMgr = NULL;


BOOL LLSessionTimeoutTimer::tick()
{
	if (mSessionId.isNull()) return TRUE;

	LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(mSessionId);
	if (session && !session->mSessionInitialized)
	{
		gIMMgr->showSessionStartError("session_initialization_timed_out_error", mSessionId);
	}
	return TRUE;
}

static void on_avatar_name_cache_toast(const LLUUID& agent_id,
									   const LLAvatarName& av_name,
									   LLSD msg)
{
	LLSD args;
	args["MESSAGE"] = msg["message"];
	args["TIME"] = msg["time"];
	// *TODO: Can this ever be an object name or group name?
	args["FROM"] = av_name.getCompleteName();
	args["FROM_ID"] = msg["from_id"];
	args["SESSION_ID"] = msg["session_id"];
	LLNotificationsUtil::add("IMToast", args, LLSD(), boost::bind(&LLIMFloater::show, msg["session_id"].asUUID()));
}

void toast_callback(const LLSD& msg){
	// do not show toast in busy mode or it goes from agent
	if (gAgent.getBusy() || gAgent.getID() == msg["from_id"])
	{
		return;
	}

	// check whether incoming IM belongs to an active session or not
	if (LLIMModel::getInstance()->getActiveSessionID().notNull()
			&& LLIMModel::getInstance()->getActiveSessionID() == msg["session_id"])
	{
		return;
	}

	// Skip toasting for system messages
	if (msg["from_id"].asUUID() == LLUUID::null)
	{
		return;
	}

	// *NOTE Skip toasting if the user disable it in preferences/debug settings ~Alexandrea
	LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(
				msg["session_id"]);
	if (!gSavedSettings.getBOOL("EnableGroupChatPopups")
			&& session->isGroupSessionType())
	{
		return;
	}
	if (!gSavedSettings.getBOOL("EnableIMChatPopups")
			&& !session->isGroupSessionType())
	{
		return;
	}

	// Skip toasting if we have open window of IM with this session id
	LLIMFloater* open_im_floater = LLIMFloater::findInstance(msg["session_id"]);
	if (open_im_floater && open_im_floater->getVisible())
	{
		return;
	}

	LLAvatarNameCache::get(msg["from_id"].asUUID(),
		boost::bind(&on_avatar_name_cache_toast,
			_1, _2, msg));
}

void LLIMModel::setActiveSessionID(const LLUUID& session_id)
{
	// check if such an ID really exists
	if (!findIMSession(session_id))
	{
		llwarns << "Trying to set as active a non-existent session!" << llendl;
		return;
	}

	mActiveSessionID = session_id;
}

LLIMModel::LLIMModel() 
{
	addNewMsgCallback(boost::bind(&LLIMFloater::newIMCallback, _1));
	addNewMsgCallback(boost::bind(&toast_callback, _1));
}

LLIMModel::LLIMSession::LLIMSession(const LLUUID& session_id, const std::string& name, const EInstantMessage& type, const LLUUID& other_participant_id, const uuid_vec_t& ids, bool voice)
:	mSessionID(session_id),
	mName(name),
	mType(type),
	mParticipantUnreadMessageCount(0),
	mNumUnread(0),
	mOtherParticipantID(other_participant_id),
	mInitialTargetIDs(ids),
	mVoiceChannel(NULL),
	mSpeakers(NULL),
	mSessionInitialized(false),
	mCallBackEnabled(true),
	mTextIMPossible(true),
	mOtherParticipantIsAvatar(true),
	mStartCallOnInitialize(false),
	mStartedAsIMCall(voice)
{
	// set P2P type by default
	mSessionType = P2P_SESSION;

	if (IM_NOTHING_SPECIAL == mType || IM_SESSION_P2P_INVITE == mType)
	{
		mVoiceChannel  = new LLVoiceChannelP2P(session_id, name, other_participant_id);
		mOtherParticipantIsAvatar = LLVoiceClient::getInstance()->isParticipantAvatar(mSessionID);

		// check if it was AVALINE call
		if (!mOtherParticipantIsAvatar)
		{
			mSessionType = AVALINE_SESSION;
		} 
	}
	else
	{
		mVoiceChannel = new LLVoiceChannelGroup(session_id, name);

		// determine whether it is group or conference session
		if (gAgent.isInGroup(mSessionID))
		{
			mSessionType = GROUP_SESSION;
		}
		else
		{
			mSessionType = ADHOC_SESSION;
		} 
	}

	if(mVoiceChannel)
	{
		mVoiceChannelStateChangeConnection = mVoiceChannel->setStateChangedCallback(boost::bind(&LLIMSession::onVoiceChannelStateChanged, this, _1, _2, _3));
	}
		
	mSpeakers = new LLIMSpeakerMgr(mVoiceChannel);

	// All participants will be added to the list of people we've recently interacted with.

	// we need to add only _active_ speakers...so comment this. 
	// may delete this later on cleanup
	//mSpeakers->addListener(&LLRecentPeople::instance(), "add");

	//we need to wait for session initialization for outgoing ad-hoc and group chat session
	//correct session id for initiated ad-hoc chat will be received from the server
	if (!LLIMModel::getInstance()->sendStartSession(mSessionID, mOtherParticipantID, 
		mInitialTargetIDs, mType))
	{
		//we don't need to wait for any responses
		//so we're already initialized
		mSessionInitialized = true;
	}
	else
	{
		//tick returns TRUE - timer will be deleted after the tick
		new LLSessionTimeoutTimer(mSessionID, SESSION_INITIALIZATION_TIMEOUT);
	}

	if (IM_NOTHING_SPECIAL == mType)
	{
		mCallBackEnabled = LLVoiceClient::getInstance()->isSessionCallBackPossible(mSessionID);
		mTextIMPossible = LLVoiceClient::getInstance()->isSessionTextIMPossible(mSessionID);
	}

	buildHistoryFileName();

	if ( gSavedPerAccountSettings.getBOOL("LogShowHistory") )
	{
		std::list<LLSD> chat_history;

		//involves parsing of a chat history
		LLLogChat::loadAllHistory(mHistoryFileName, chat_history);
		addMessagesFromHistory(chat_history);
	}

	// Localizing name of ad-hoc session. STORM-153
	// Changing name should happen here- after the history file was created, so that
	// history files have consistent (English) names in different locales.
	if (isAdHocSessionType() && IM_SESSION_INVITE == mType)
	{
		LLAvatarNameCache::get(mOtherParticipantID,
							   boost::bind(&LLIMModel::LLIMSession::onAdHocNameCache,
							   this, _2));
	}
}

void LLIMModel::LLIMSession::onAdHocNameCache(const LLAvatarName& av_name)
{
	if (av_name.mIsTemporaryName)
	{
		S32 separator_index = mName.rfind(" ");
		std::string name = mName.substr(0, separator_index);
		++separator_index;
		std::string conference_word = mName.substr(separator_index, mName.length());

		// additional check that session name is what we expected
		if ("Conference" == conference_word)
		{
			LLStringUtil::format_map_t args;
			args["[AGENT_NAME]"] = name;
			LLTrans::findString(mName, "conference-title-incoming", args);
		}
	}
	else
	{
		LLStringUtil::format_map_t args;
		args["[AGENT_NAME]"] = av_name.getCompleteName();
		LLTrans::findString(mName, "conference-title-incoming", args);
	}
}

void LLIMModel::LLIMSession::onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state, const LLVoiceChannel::EDirection& direction)
{
	std::string you_joined_call = LLTrans::getString("you_joined_call");
	std::string you_started_call = LLTrans::getString("you_started_call");
	std::string other_avatar_name = "";

	std::string message;

	switch(mSessionType)
	{
	case AVALINE_SESSION:
		// no text notifications
		break;
	case P2P_SESSION:
		gCacheName->getFullName(mOtherParticipantID, other_avatar_name); // voice

		if(direction == LLVoiceChannel::INCOMING_CALL)
		{
			switch(new_state)
			{
			case LLVoiceChannel::STATE_CALL_STARTED :
				{
					LLStringUtil::format_map_t string_args;
					string_args["[NAME]"] = other_avatar_name;
					message = LLTrans::getString("name_started_call", string_args);
					LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, message);				
					break;
				}
			case LLVoiceChannel::STATE_CONNECTED :
				LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, you_joined_call);
			default:
				break;
			}
		}
		else // outgoing call
		{
			switch(new_state)
			{
			case LLVoiceChannel::STATE_CALL_STARTED :
				LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, you_started_call);
				break;
			case LLVoiceChannel::STATE_CONNECTED :
				message = LLTrans::getString("answered_call");
				LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, message);
			default:
				break;
			}
		}
		break;

	case GROUP_SESSION:
	case ADHOC_SESSION:
		if(direction == LLVoiceChannel::INCOMING_CALL)
		{
			switch(new_state)
			{
			case LLVoiceChannel::STATE_CONNECTED :
				LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, you_joined_call);
			default:
				break;
			}
		}
		else // outgoing call
		{
			switch(new_state)
			{
			case LLVoiceChannel::STATE_CALL_STARTED :
				LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, you_started_call);
				break;
			default:
				break;
			}
		}
	}
	// Update speakers list when connected
	if (LLVoiceChannel::STATE_CONNECTED == new_state)
	{
		mSpeakers->update(true);
	}
}

LLIMModel::LLIMSession::~LLIMSession()
{
	delete mSpeakers;
	mSpeakers = NULL;

	// End the text IM session if necessary
	if(LLVoiceClient::getInstance() && mOtherParticipantID.notNull())
	{
		switch(mType)
		{
		case IM_NOTHING_SPECIAL:
		case IM_SESSION_P2P_INVITE:
			LLVoiceClient::getInstance()->endUserIMSession(mOtherParticipantID);
			break;

		default:
			// Appease the linux compiler
			break;
		}
	}

	mVoiceChannelStateChangeConnection.disconnect();

	// HAVE to do this here -- if it happens in the LLVoiceChannel destructor it will call the wrong version (since the object's partially deconstructed at that point).
	mVoiceChannel->deactivate();

	delete mVoiceChannel;
	mVoiceChannel = NULL;
}

void LLIMModel::LLIMSession::sessionInitReplyReceived(const LLUUID& new_session_id)
{
	mSessionInitialized = true;

	if (new_session_id != mSessionID)
	{
		mSessionID = new_session_id;
		mVoiceChannel->updateSessionID(new_session_id);
	}
}

void LLIMModel::LLIMSession::addMessage(const std::string& from, const LLUUID& from_id, const std::string& utf8_text, const std::string& time, const bool is_history)
{
	LLSD message;
	message["from"] = from;
	message["from_id"] = from_id;
	message["message"] = utf8_text;
	message["time"] = time; 
	message["index"] = (LLSD::Integer)mMsgs.size(); 
	message["is_history"] = is_history;

	mMsgs.push_front(message); 

	if (mSpeakers && from_id.notNull())
	{
		mSpeakers->speakerChatted(from_id);
		mSpeakers->setSpeakerTyping(from_id, FALSE);
	}
}

void LLIMModel::LLIMSession::addMessagesFromHistory(const std::list<LLSD>& history)
{
	std::list<LLSD>::const_iterator it = history.begin();
	while (it != history.end())
	{
		const LLSD& msg = *it;

		std::string from = msg[IM_FROM];
		LLUUID from_id;
		if (msg[IM_FROM_ID].isDefined())
		{
			from_id = msg[IM_FROM_ID].asUUID();
		}
		else
		{
			// convert it to a legacy name if we have a complete name
			std::string legacy_name = gCacheName->buildLegacyName(from);
 			gCacheName->getUUID(legacy_name, from_id);
		}

		std::string timestamp = msg[IM_TIME];
		std::string text = msg[IM_TEXT];

		addMessage(from, from_id, text, timestamp, true);

		it++;
	}
}

void LLIMModel::LLIMSession::chatFromLogFile(LLLogChat::ELogLineType type, const LLSD& msg, void* userdata)
{
	if (!userdata) return;

	LLIMSession* self = (LLIMSession*) userdata;

	if (type == LLLogChat::LOG_LINE)
	{
		self->addMessage("", LLSD(), msg["message"].asString(), "", true);
	}
	else if (type == LLLogChat::LOG_LLSD)
	{
		self->addMessage(msg["from"].asString(), msg["from_id"].asUUID(), msg["message"].asString(), msg["time"].asString(), true);
	}
}

LLIMModel::LLIMSession* LLIMModel::findIMSession(const LLUUID& session_id) const
{
	return get_if_there(mId2SessionMap, session_id,
		(LLIMModel::LLIMSession*) NULL);
}

//*TODO consider switching to using std::set instead of std::list for holding LLUUIDs across the whole code
LLIMModel::LLIMSession* LLIMModel::findAdHocIMSession(const uuid_vec_t& ids)
{
	S32 num = ids.size();
	if (!num) return NULL;

	if (mId2SessionMap.empty()) return NULL;

	std::map<LLUUID, LLIMSession*>::const_iterator it = mId2SessionMap.begin();
	for (; it != mId2SessionMap.end(); ++it)
	{
		LLIMSession* session = (*it).second;
	
		if (!session->isAdHoc()) continue;
		if (session->mInitialTargetIDs.size() != num) continue;

		std::list<LLUUID> tmp_list(session->mInitialTargetIDs.begin(), session->mInitialTargetIDs.end());

		uuid_vec_t::const_iterator iter = ids.begin();
		while (iter != ids.end())
		{
			tmp_list.remove(*iter);
			++iter;
			
			if (tmp_list.empty()) 
			{
				break;
			}
		}

		if (tmp_list.empty() && iter == ids.end())
		{
			return session;
		}
	}

	return NULL;
}

bool LLIMModel::LLIMSession::isOutgoingAdHoc()
{
	return IM_SESSION_CONFERENCE_START == mType;
}

bool LLIMModel::LLIMSession::isAdHoc()
{
	return IM_SESSION_CONFERENCE_START == mType || (IM_SESSION_INVITE == mType && !gAgent.isInGroup(mSessionID));
}

bool LLIMModel::LLIMSession::isP2P()
{
	return IM_NOTHING_SPECIAL == mType;
}

bool LLIMModel::LLIMSession::isOtherParticipantAvaline()
{
	return !mOtherParticipantIsAvatar;
}

void LLIMModel::LLIMSession::buildHistoryFileName()
{
	mHistoryFileName = mName;

	//ad-hoc requires sophisticated chat history saving schemes
	if (isAdHoc())
	{
		/* in case of outgoing ad-hoc sessions we need to make specilized names
		* if this naming system is ever changed then the filtering definitions in 
		* lllogchat.cpp need to be change acordingly so that the filtering for the
		* date stamp code introduced in STORM-102 will work properly and not add
		* a date stamp to the Ad-hoc conferences.
		*/
		if (mInitialTargetIDs.size())
		{
			std::set<LLUUID> sorted_uuids(mInitialTargetIDs.begin(), mInitialTargetIDs.end());
			mHistoryFileName = mName + " hash" + generateHash(sorted_uuids);
		}
		else
		{
			//in case of incoming ad-hoc sessions
			mHistoryFileName = mName + " " + LLLogChat::timestamp(true) + " " + mSessionID.asString().substr(0, 4);
		}
	}
	else if (isP2P()) // look up username to use as the log name
	{
		LLAvatarName av_name;
		// For outgoing sessions we already have a cached name
		// so no need for a callback in LLAvatarNameCache::get()
		if (LLAvatarNameCache::get(mOtherParticipantID, &av_name))
		{
			if (av_name.mUsername.empty())
			{
				// Display names are off, use mDisplayName which will be the legacy name
				mHistoryFileName = LLCacheName::buildUsername(av_name.mDisplayName);
			}
			else
			{
				mHistoryFileName =  av_name.mUsername;
			}
		}
		else
		{
			// Incoming P2P sessions include a name that we can use to build a history file name
			mHistoryFileName = LLCacheName::buildUsername(mName);
		}
	}
}

//static
std::string LLIMModel::LLIMSession::generateHash(const std::set<LLUUID>& sorted_uuids)
{
	LLMD5 md5_uuid;
	
	std::set<LLUUID>::const_iterator it = sorted_uuids.begin();
	while (it != sorted_uuids.end())
	{
		md5_uuid.update((unsigned char*)(*it).mData, 16);
		it++;
	}
	md5_uuid.finalize();

	LLUUID participants_md5_hash;
	md5_uuid.raw_digest((unsigned char*) participants_md5_hash.mData);
	return participants_md5_hash.asString();
}

void LLIMModel::processSessionInitializedReply(const LLUUID& old_session_id, const LLUUID& new_session_id)
{
	LLIMSession* session = findIMSession(old_session_id);
	if (session)
	{
		session->sessionInitReplyReceived(new_session_id);

		if (old_session_id != new_session_id)
		{
			mId2SessionMap.erase(old_session_id);
			mId2SessionMap[new_session_id] = session;

			gIMMgr->notifyObserverSessionIDUpdated(old_session_id, new_session_id);
		}

		LLIMFloater* im_floater = LLIMFloater::findInstance(old_session_id);
		if (im_floater)
		{
			im_floater->sessionInitReplyReceived(new_session_id);
		}

		// auto-start the call on session initialization?
		if (session->mStartCallOnInitialize)
		{
			gIMMgr->startCall(new_session_id);
		}
	}
}

void LLIMModel::testMessages()
{
	LLUUID bot1_id("d0426ec6-6535-4c11-a5d9-526bb0c654d9");
	LLUUID bot1_session_id;
	std::string from = "IM Tester";

	bot1_session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, bot1_id);
	newSession(bot1_session_id, from, IM_NOTHING_SPECIAL, bot1_id);
	addMessage(bot1_session_id, from, bot1_id, "Test Message: Hi from testerbot land!");

	LLUUID bot2_id;
	std::string firstname[] = {"Roflcopter", "Joe"};
	std::string lastname[] = {"Linden", "Tester", "Resident", "Schmoe"};

	S32 rand1 = ll_rand(sizeof firstname)/(sizeof firstname[0]);
	S32 rand2 = ll_rand(sizeof lastname)/(sizeof lastname[0]);
	
	from = firstname[rand1] + " " + lastname[rand2];
	bot2_id.generate(from);
	LLUUID bot2_session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, bot2_id);
	newSession(bot2_session_id, from, IM_NOTHING_SPECIAL, bot2_id);
	addMessage(bot2_session_id, from, bot2_id, "Test Message: Hello there, I have a question. Can I bother you for a second? ");
	addMessage(bot2_session_id, from, bot2_id, "Test Message: OMGWTFBBQ.");
}

//session name should not be empty
bool LLIMModel::newSession(const LLUUID& session_id, const std::string& name, const EInstantMessage& type, 
						   const LLUUID& other_participant_id, const uuid_vec_t& ids, bool voice)
{
	if (name.empty())
	{
		llwarns << "Attempt to create a new session with empty name; id = " << session_id << llendl;
		return false;
	}

	if (findIMSession(session_id))
	{
		llwarns << "IM Session " << session_id << " already exists" << llendl;
		return false;
	}

	LLIMSession* session = new LLIMSession(session_id, name, type, other_participant_id, ids, voice);
	mId2SessionMap[session_id] = session;

	// When notifying observer, name of session is used instead of "name", because they may not be the
	// same if it is an adhoc session (in this case name is localized in LLIMSession constructor).
	std::string session_name = LLIMModel::getInstance()->getName(session_id);
	LLIMMgr::getInstance()->notifyObserverSessionAdded(session_id, session_name, other_participant_id);

	return true;

}

bool LLIMModel::newSession(const LLUUID& session_id, const std::string& name, const EInstantMessage& type, const LLUUID& other_participant_id, bool voice)
{
	uuid_vec_t no_ids;
	return newSession(session_id, name, type, other_participant_id, no_ids, voice);
}

bool LLIMModel::clearSession(const LLUUID& session_id)
{
	if (mId2SessionMap.find(session_id) == mId2SessionMap.end()) return false;
	delete (mId2SessionMap[session_id]);
	mId2SessionMap.erase(session_id);
	return true;
}

void LLIMModel::getMessagesSilently(const LLUUID& session_id, std::list<LLSD>& messages, int start_index)
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return;
	}

	int i = session->mMsgs.size() - start_index;

	for (std::list<LLSD>::iterator iter = session->mMsgs.begin();
		iter != session->mMsgs.end() && i > 0;
		iter++)
	{
		LLSD msg;
		msg = *iter;
		messages.push_back(*iter);
		i--;
	}
}

void LLIMModel::sendNoUnreadMessages(const LLUUID& session_id)
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return;
	}

	session->mNumUnread = 0;
	session->mParticipantUnreadMessageCount = 0;
	
	LLSD arg;
	arg["session_id"] = session_id;
	arg["num_unread"] = 0;
	arg["participant_unread"] = session->mParticipantUnreadMessageCount;
	mNoUnreadMsgsSignal(arg);
}

void LLIMModel::getMessages(const LLUUID& session_id, std::list<LLSD>& messages, int start_index)
{
	getMessagesSilently(session_id, messages, start_index);

	sendNoUnreadMessages(session_id);
}

bool LLIMModel::addToHistory(const LLUUID& session_id, const std::string& from, const LLUUID& from_id, const std::string& utf8_text) {
	
	LLIMSession* session = findIMSession(session_id);

	if (!session) 
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return false;
	}

	session->addMessage(from, from_id, utf8_text, LLLogChat::timestamp(false)); //might want to add date separately

	return true;
}

bool LLIMModel::logToFile(const std::string& file_name, const std::string& from, const LLUUID& from_id, const std::string& utf8_text)
{
	if (gSavedPerAccountSettings.getBOOL("LogInstantMessages"))
	{	
		std::string from_name = from;

		LLAvatarName av_name;
		if (!from_id.isNull() && 
			LLAvatarNameCache::get(from_id, &av_name) &&
			!av_name.mIsDisplayNameDefault)
		{	
			from_name = av_name.getCompleteName();
		}

		LLLogChat::saveHistory(file_name, from_name, from_id, utf8_text);
		return true;
	}
	else
	{
		return false;
	}
}

bool LLIMModel::proccessOnlineOfflineNotification(
	const LLUUID& session_id, 
	const std::string& utf8_text)
{
	// Add system message to history
	return addMessage(session_id, SYSTEM_FROM, LLUUID::null, utf8_text);
}

bool LLIMModel::addMessage(const LLUUID& session_id, const std::string& from, const LLUUID& from_id, 
						   const std::string& utf8_text, bool log2file /* = true */) { 

	LLIMSession* session = addMessageSilently(session_id, from, from_id, utf8_text, log2file);
	if (!session) return false;

	//good place to add some1 to recent list
	//other places may be called from message history.
	if( !from_id.isNull() &&
		( session->isP2PSessionType() || session->isAdHocSessionType() ) )
		LLRecentPeople::instance().add(from_id);

	// notify listeners
	LLSD arg;
	arg["session_id"] = session_id;
	arg["num_unread"] = session->mNumUnread;
	arg["participant_unread"] = session->mParticipantUnreadMessageCount;
	arg["message"] = utf8_text;
	arg["from"] = from;
	arg["from_id"] = from_id;
	arg["time"] = LLLogChat::timestamp(false);
	mNewMsgSignal(arg);

	return true;
}

LLIMModel::LLIMSession* LLIMModel::addMessageSilently(const LLUUID& session_id, const std::string& from, const LLUUID& from_id, 
													 const std::string& utf8_text, bool log2file /* = true */)
{
	LLIMSession* session = findIMSession(session_id);

	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return NULL;
	}

	// replace interactive system message marker with correct from string value
	std::string from_name = from;
	if (INTERACTIVE_SYSTEM_FROM == from)
	{
		from_name = SYSTEM_FROM;
	}

	addToHistory(session_id, from_name, from_id, utf8_text);
	if (log2file)
	{
		logToFile(getHistoryFileName(session_id), from_name, from_id, utf8_text);
	}
	
	session->mNumUnread++;

	//update count of unread messages from real participant
	if (!(from_id.isNull() || from_id == gAgentID || SYSTEM_FROM == from)
			// we should increment counter for interactive system messages()
			|| INTERACTIVE_SYSTEM_FROM == from)
	{
		++(session->mParticipantUnreadMessageCount);
	}

	return session;
}


const std::string LLIMModel::getName(const LLUUID& session_id) const
{
	LLIMSession* session = findIMSession(session_id);

	if (!session) 
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return LLTrans::getString("no_session_message");
	}

	return session->mName;
}

const S32 LLIMModel::getNumUnread(const LLUUID& session_id) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return -1;
	}

	return session->mNumUnread;
}

const LLUUID& LLIMModel::getOtherParticipantID(const LLUUID& session_id) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return LLUUID::null;
	}

	return session->mOtherParticipantID;
}

EInstantMessage LLIMModel::getType(const LLUUID& session_id) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return IM_COUNT;
	}

	return session->mType;
}

LLVoiceChannel* LLIMModel::getVoiceChannel( const LLUUID& session_id ) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return NULL;
	}

	return session->mVoiceChannel;
}

LLIMSpeakerMgr* LLIMModel::getSpeakerManager( const LLUUID& session_id ) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << " does not exist " << llendl;
		return NULL;
	}

	return session->mSpeakers;
}

const std::string& LLIMModel::getHistoryFileName(const LLUUID& session_id) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << " does not exist " << llendl;
		return LLStringUtil::null;
	}

	return session->mHistoryFileName;
}


// TODO get rid of other participant ID
void LLIMModel::sendTypingState(LLUUID session_id, LLUUID other_participant_id, BOOL typing) 
{
	std::string name;
	LLAgentUI::buildFullname(name);

	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		other_participant_id,
		name,
		std::string("typing"),
		IM_ONLINE,
		(typing ? IM_TYPING_START : IM_TYPING_STOP),
		session_id);
	gAgent.sendReliableMessage();
}

void LLIMModel::sendLeaveSession(const LLUUID& session_id, const LLUUID& other_participant_id)
{
	if(session_id.notNull())
	{
		std::string name;
		LLAgentUI::buildFullname(name);
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			other_participant_id,
			name, 
			LLStringUtil::null,
			IM_ONLINE,
			IM_SESSION_LEAVE,
			session_id);
		gAgent.sendReliableMessage();
	}
}

//*TODO this method is better be moved to the LLIMMgr
void LLIMModel::sendMessage(const std::string& utf8_text,
					 const LLUUID& im_session_id,
					 const LLUUID& other_participant_id,
					 EInstantMessage dialog)
{
	std::string name;
	bool sent = false;
	LLAgentUI::buildFullname(name);

	const LLRelationship* info = NULL;
	info = LLAvatarTracker::instance().getBuddyInfo(other_participant_id);
	
	U8 offline = (!info || info->isOnline()) ? IM_ONLINE : IM_OFFLINE;
	
	if((offline == IM_OFFLINE) && (LLVoiceClient::getInstance()->isOnlineSIP(other_participant_id)))
	{
		// User is online through the OOW connector, but not with a regular viewer.  Try to send the message via SLVoice.
		sent = LLVoiceClient::getInstance()->sendTextMessage(other_participant_id, utf8_text);
	}
	
	if(!sent)
	{
		// Send message normally.

		// default to IM_SESSION_SEND unless it's nothing special - in
		// which case it's probably an IM to everyone.
		U8 new_dialog = dialog;

		if ( dialog != IM_NOTHING_SPECIAL )
		{
			new_dialog = IM_SESSION_SEND;
		}
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			other_participant_id,
			name.c_str(),
			utf8_text.c_str(),
			offline,
			(EInstantMessage)new_dialog,
			im_session_id);
		gAgent.sendReliableMessage();
	}

	// If there is a mute list and this is not a group chat...
	if ( LLMuteList::getInstance() )
	{
		// ... the target should not be in our mute list for some message types.
		// Auto-remove them if present.
		switch( dialog )
		{
		case IM_NOTHING_SPECIAL:
		case IM_GROUP_INVITATION:
		case IM_INVENTORY_OFFERED:
		case IM_SESSION_INVITE:
		case IM_SESSION_P2P_INVITE:
		case IM_SESSION_CONFERENCE_START:
		case IM_SESSION_SEND: // This one is marginal - erring on the side of hearing.
		case IM_LURE_USER:
		case IM_GODLIKE_LURE_USER:
		case IM_FRIENDSHIP_OFFERED:
			LLMuteList::getInstance()->autoRemove(other_participant_id, LLMuteList::AR_IM);
			break;
		default: ; // do nothing
		}
	}

	if((dialog == IM_NOTHING_SPECIAL) && 
	   (other_participant_id.notNull()))
	{
		// Do we have to replace the /me's here?
		std::string from;
		LLAgentUI::buildFullname(from);
		LLIMModel::getInstance()->addMessage(im_session_id, from, gAgentID, utf8_text);

		//local echo for the legacy communicate panel
		std::string history_echo;
		LLAgentUI::buildFullname(history_echo);

		history_echo += ": " + utf8_text;

		LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(im_session_id);
		if (speaker_mgr)
		{
			speaker_mgr->speakerChatted(gAgentID);
			speaker_mgr->setSpeakerTyping(gAgentID, FALSE);
		}
	}

	// Add the recipient to the recent people list.
	bool is_not_group_id = LLGroupMgr::getInstance()->getGroupData(other_participant_id) == NULL;

	if (is_not_group_id)
	{
		LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(im_session_id);
		if( session == 0)//??? shouldn't really happen
		{
			LLRecentPeople::instance().add(other_participant_id);
			return;
		}
		// IM_SESSION_INVITE means that this is an Ad-hoc incoming chat
		//		(it can be also Group chat but it is checked above)
		// In this case mInitialTargetIDs contains Ad-hoc session ID and it should not be added
		// to Recent People to prevent showing of an item with (???)(???). See EXT-8246.
		// Concrete participants will be added into this list once they sent message in chat.
		if (IM_SESSION_INVITE == dialog) return;
			
		if (IM_SESSION_CONFERENCE_START == dialog) // outgoing ad-hoc session
		{
			// Add only online members of conference to recent list (EXT-8658)
			addSpeakersToRecent(im_session_id);
		}
		else // outgoing P2P session
		{
			// Add the recepient of the session.
			if (!session->mInitialTargetIDs.empty())
			{
				LLRecentPeople::instance().add(*(session->mInitialTargetIDs.begin()));
			}
		}
	}
}

void LLIMModel::addSpeakersToRecent(const LLUUID& im_session_id)
{
	LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(im_session_id);
	LLSpeakerMgr::speaker_list_t speaker_list;
	if(speaker_mgr != NULL)
	{
		speaker_mgr->getSpeakerList(&speaker_list, true);
	}
	for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
	{
		const LLPointer<LLSpeaker>& speakerp = *it;
		LLRecentPeople::instance().add(speakerp->mID);
	}
}

void session_starter_helper(
	const LLUUID& temp_session_id,
	const LLUUID& other_participant_id,
	EInstantMessage im_type)
{
	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	msg->nextBlockFast(_PREHASH_MessageBlock);
	msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
	msg->addUUIDFast(_PREHASH_ToAgentID, other_participant_id);
	msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
	msg->addU8Fast(_PREHASH_Dialog, im_type);
	msg->addUUIDFast(_PREHASH_ID, temp_session_id);
	msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary

	std::string name;
	LLAgentUI::buildFullname(name);

	msg->addStringFast(_PREHASH_FromAgentName, name);
	msg->addStringFast(_PREHASH_Message, LLStringUtil::null);
	msg->addU32Fast(_PREHASH_ParentEstateID, 0);
	msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
	msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
}

void start_deprecated_conference_chat(
	const LLUUID& temp_session_id,
	const LLUUID& creator_id,
	const LLUUID& other_participant_id,
	const LLSD& agents_to_invite)
{
	U8* bucket;
	U8* pos;
	S32 count;
	S32 bucket_size;

	// *FIX: this could suffer from endian issues
	count = agents_to_invite.size();
	bucket_size = UUID_BYTES * count;
	bucket = new U8[bucket_size];
	pos = bucket;

	for(S32 i = 0; i < count; ++i)
	{
		LLUUID agent_id = agents_to_invite[i].asUUID();
		
		memcpy(pos, &agent_id, UUID_BYTES);
		pos += UUID_BYTES;
	}

	session_starter_helper(
		temp_session_id,
		other_participant_id,
		IM_SESSION_CONFERENCE_START);

	gMessageSystem->addBinaryDataFast(
		_PREHASH_BinaryBucket,
		bucket,
		bucket_size);

	gAgent.sendReliableMessage();
 
	delete[] bucket;
}

class LLStartConferenceChatResponder : public LLHTTPClient::Responder
{
public:
	LLStartConferenceChatResponder(
		const LLUUID& temp_session_id,
		const LLUUID& creator_id,
		const LLUUID& other_participant_id,
		const LLSD& agents_to_invite)
	{
		mTempSessionID = temp_session_id;
		mCreatorID = creator_id;
		mOtherParticipantID = other_participant_id;
		mAgents = agents_to_invite;
	}

	virtual void error(U32 statusNum, const std::string& reason)
	{
		//try an "old school" way.
		if ( statusNum == 400 )
		{
			start_deprecated_conference_chat(
				mTempSessionID,
				mCreatorID,
				mOtherParticipantID,
				mAgents);
		}

		//else throw an error back to the client?
		//in theory we should have just have these error strings
		//etc. set up in this file as opposed to the IMMgr,
		//but the error string were unneeded here previously
		//and it is not worth the effort switching over all
		//the possible different language translations
	}

private:
	LLUUID mTempSessionID;
	LLUUID mCreatorID;
	LLUUID mOtherParticipantID;

	LLSD mAgents;
};

// Returns true if any messages were sent, false otherwise.
// Is sort of equivalent to "does the server need to do anything?"
bool LLIMModel::sendStartSession(
	const LLUUID& temp_session_id,
	const LLUUID& other_participant_id,
	const uuid_vec_t& ids,
	EInstantMessage dialog)
{
	if ( dialog == IM_SESSION_GROUP_START )
	{
		session_starter_helper(
			temp_session_id,
			other_participant_id,
			dialog);
		gMessageSystem->addBinaryDataFast(
				_PREHASH_BinaryBucket,
				EMPTY_BINARY_BUCKET,
				EMPTY_BINARY_BUCKET_SIZE);
		gAgent.sendReliableMessage();

		return true;
	}
	else if ( dialog == IM_SESSION_CONFERENCE_START )
	{
		LLSD agents;
		for (int i = 0; i < (S32) ids.size(); i++)
		{
			agents.append(ids[i]);
		}

		//we have a new way of starting conference calls now
		LLViewerRegion* region = gAgent.getRegion();
		if (region)
		{
			std::string url = region->getCapability(
				"ChatSessionRequest");
			LLSD data;
			data["method"] = "start conference";
			data["session-id"] = temp_session_id;

			data["params"] = agents;

			LLHTTPClient::post(
				url,
				data,
				new LLStartConferenceChatResponder(
					temp_session_id,
					gAgent.getID(),
					other_participant_id,
					data["params"]));
		}
		else
		{
			start_deprecated_conference_chat(
				temp_session_id,
				gAgent.getID(),
				other_participant_id,
				agents);
		}

		//we also need to wait for reply from the server in case of ad-hoc chat (we'll get new session id)
		return true;
	}

	return false;
}

//
// Helper Functions
//

class LLViewerChatterBoxInvitationAcceptResponder :
	public LLHTTPClient::Responder
{
public:
	LLViewerChatterBoxInvitationAcceptResponder(
		const LLUUID& session_id,
		LLIMMgr::EInvitationType invitation_type)
	{
		mSessionID = session_id;
		mInvitiationType = invitation_type;
	}

	void result(const LLSD& content)
	{
		if ( gIMMgr)
		{
			LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
			if (speaker_mgr)
			{
				//we've accepted our invitation
				//and received a list of agents that were
				//currently in the session when the reply was sent
				//to us.  Now, it is possible that there were some agents
				//to slip in/out between when that message was sent to us
				//and now.

				//the agent list updates we've received have been
				//accurate from the time we were added to the session
				//but unfortunately, our base that we are receiving here
				//may not be the most up to date.  It was accurate at
				//some point in time though.
				speaker_mgr->setSpeakers(content);

				//we now have our base of users in the session
				//that was accurate at some point, but maybe not now
				//so now we apply all of the udpates we've received
				//in case of race conditions
				speaker_mgr->updateSpeakers(gIMMgr->getPendingAgentListUpdates(mSessionID));
			}

			if (LLIMMgr::INVITATION_TYPE_VOICE == mInvitiationType)
			{
				gIMMgr->startCall(mSessionID, LLVoiceChannel::INCOMING_CALL);
			}

			if ((mInvitiationType == LLIMMgr::INVITATION_TYPE_VOICE 
				|| mInvitiationType == LLIMMgr::INVITATION_TYPE_IMMEDIATE)
				&& LLIMModel::getInstance()->findIMSession(mSessionID))
			{
				// TODO remove in 2010, for voice calls we do not open an IM window
				//LLIMFloater::show(mSessionID);
			}

			gIMMgr->clearPendingAgentListUpdates(mSessionID);
			gIMMgr->clearPendingInvitation(mSessionID);
		}
	}

	void error(U32 statusNum, const std::string& reason)
	{		
		//throw something back to the viewer here?
		if ( gIMMgr )
		{
			gIMMgr->clearPendingAgentListUpdates(mSessionID);
			gIMMgr->clearPendingInvitation(mSessionID);
			if ( 404 == statusNum )
			{
				std::string error_string;
				error_string = "session_does_not_exist_error";
				gIMMgr->showSessionStartError(error_string, mSessionID);
			}
		}
	}

private:
	LLUUID mSessionID;
	LLIMMgr::EInvitationType mInvitiationType;
};


// the other_participant_id is either an agent_id, a group_id, or an inventory
// folder item_id (collection of calling cards)

// static
LLUUID LLIMMgr::computeSessionID(
	EInstantMessage dialog,
	const LLUUID& other_participant_id)
{
	LLUUID session_id;
	if (IM_SESSION_GROUP_START == dialog)
	{
		// slam group session_id to the group_id (other_participant_id)
		session_id = other_participant_id;
	}
	else if (IM_SESSION_CONFERENCE_START == dialog)
	{
		session_id.generate();
	}
	else if (IM_SESSION_INVITE == dialog)
	{
		// use provided session id for invites
		session_id = other_participant_id;
	}
	else
	{
		LLUUID agent_id = gAgent.getID();
		if (other_participant_id == agent_id)
		{
			// if we try to send an IM to ourselves then the XOR would be null
			// so we just make the session_id the same as the agent_id
			session_id = agent_id;
		}
		else
		{
			// peer-to-peer or peer-to-asset session_id is the XOR
			session_id = other_participant_id ^ agent_id;
		}
	}
	return session_id;
}

void
LLIMMgr::showSessionStartError(
	const std::string& error_string,
	const LLUUID session_id)
{
	if (!hasSession(session_id)) return;

	LLSD args;
	args["REASON"] = LLTrans::getString(error_string);
	args["RECIPIENT"] = LLIMModel::getInstance()->getName(session_id);

	LLSD payload;
	payload["session_id"] = session_id;

	LLNotificationsUtil::add(
		"ChatterBoxSessionStartError",
		args,
		payload,
		LLIMMgr::onConfirmForceCloseError);
}

void
LLIMMgr::showSessionEventError(
	const std::string& event_string,
	const std::string& error_string,
	const LLUUID session_id)
{
	LLSD args;
	LLStringUtil::format_map_t event_args;

	event_args["RECIPIENT"] = LLIMModel::getInstance()->getName(session_id);

	args["REASON"] =
		LLTrans::getString(error_string);
	args["EVENT"] =
		LLTrans::getString(event_string, event_args);

	LLNotificationsUtil::add(
		"ChatterBoxSessionEventError",
		args);
}

void
LLIMMgr::showSessionForceClose(
	const std::string& reason_string,
	const LLUUID session_id)
{
	if (!hasSession(session_id)) return;

	LLSD args;

	args["NAME"] = LLIMModel::getInstance()->getName(session_id);
	args["REASON"] = LLTrans::getString(reason_string);

	LLSD payload;
	payload["session_id"] = session_id;

	LLNotificationsUtil::add(
		"ForceCloseChatterBoxSession",
		args,
		payload,
		LLIMMgr::onConfirmForceCloseError);
}

//static
bool
LLIMMgr::onConfirmForceCloseError(
	const LLSD& notification,
	const LLSD& response)
{
	//only 1 option really
	LLUUID session_id = notification["payload"]["session_id"];

	LLFloater* floater = LLIMFloater::findInstance(session_id);
	if ( floater )
	{
		floater->closeFloater(FALSE);
	}
	return false;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLCallDialogManager
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LLCallDialogManager::LLCallDialogManager()
{
}

LLCallDialogManager::~LLCallDialogManager()
{
}

void LLCallDialogManager::initClass()
{
	LLVoiceChannel::setCurrentVoiceChannelChangedCallback(LLCallDialogManager::onVoiceChannelChanged);
}

void LLCallDialogManager::onVoiceChannelChanged(const LLUUID &session_id)
{
	LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(session_id);
	if(!session)
	{		
		sPreviousSessionlName = sCurrentSessionlName;
		sCurrentSessionlName = ""; // Empty string results in "Nearby Voice Chat" after substitution
		return;
	}
	
	if (sSession)
	{
		// store previous session type to process Avaline calls in dialogs
		sPreviousSessionType = sSession->mSessionType;
	}

	sSession = session;

	static boost::signals2::connection prev_channel_state_changed_connection;
	// disconnect previously connected callback to avoid have invalid sSession in onVoiceChannelStateChanged()
	prev_channel_state_changed_connection.disconnect();
	prev_channel_state_changed_connection =
		sSession->mVoiceChannel->setStateChangedCallback(boost::bind(LLCallDialogManager::onVoiceChannelStateChanged, _1, _2, _3, _4));

	if(sCurrentSessionlName != session->mName)
	{
		sPreviousSessionlName = sCurrentSessionlName;
		sCurrentSessionlName = session->mName;
	}

	if (LLVoiceChannel::getCurrentVoiceChannel()->getState() == LLVoiceChannel::STATE_CALL_STARTED &&
		LLVoiceChannel::getCurrentVoiceChannel()->getCallDirection() == LLVoiceChannel::OUTGOING_CALL)
	{
		
		//*TODO get rid of duplicated code
		LLSD mCallDialogPayload;
		mCallDialogPayload["session_id"] = sSession->mSessionID;
		mCallDialogPayload["session_name"] = sSession->mName;
		mCallDialogPayload["other_user_id"] = sSession->mOtherParticipantID;
		mCallDialogPayload["old_channel_name"] = sPreviousSessionlName;
		mCallDialogPayload["old_session_type"] = sPreviousSessionType;
		mCallDialogPayload["state"] = LLVoiceChannel::STATE_CALL_STARTED;
		mCallDialogPayload["disconnected_channel_name"] = sSession->mName;
		mCallDialogPayload["session_type"] = sSession->mSessionType;

		LLOutgoingCallDialog* ocd = LLFloaterReg::getTypedInstance<LLOutgoingCallDialog>("outgoing_call", LLOutgoingCallDialog::OCD_KEY);
		if(ocd)
		{
			ocd->show(mCallDialogPayload);
		}	
	}

}

void LLCallDialogManager::onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state, const LLVoiceChannel::EDirection& direction, bool ended_by_agent)
{
	LLSD mCallDialogPayload;
	LLOutgoingCallDialog* ocd = NULL;

	if(sOldState == new_state)
	{
		return;
	}

	sOldState = new_state;

	mCallDialogPayload["session_id"] = sSession->mSessionID;
	mCallDialogPayload["session_name"] = sSession->mName;
	mCallDialogPayload["other_user_id"] = sSession->mOtherParticipantID;
	mCallDialogPayload["old_channel_name"] = sPreviousSessionlName;
	mCallDialogPayload["old_session_type"] = sPreviousSessionType;
	mCallDialogPayload["state"] = new_state;
	mCallDialogPayload["disconnected_channel_name"] = sSession->mName;
	mCallDialogPayload["session_type"] = sSession->mSessionType;
	mCallDialogPayload["ended_by_agent"] = ended_by_agent;

	switch(new_state)
	{			
	case LLVoiceChannel::STATE_CALL_STARTED :
		// do not show "Calling to..." if it is incoming call
		if(direction == LLVoiceChannel::INCOMING_CALL)
		{
			return;
		}
		break;

	case LLVoiceChannel::STATE_HUNG_UP:
		// this state is coming before session is changed, so, put it into payload map
		mCallDialogPayload["old_session_type"] = sSession->mSessionType;
		break;

	case LLVoiceChannel::STATE_CONNECTED :
		ocd = LLFloaterReg::findTypedInstance<LLOutgoingCallDialog>("outgoing_call", LLOutgoingCallDialog::OCD_KEY);
		if (ocd)
		{
			ocd->closeFloater();
		}
		return;

	default:
		break;
	}

	ocd = LLFloaterReg::getTypedInstance<LLOutgoingCallDialog>("outgoing_call", LLOutgoingCallDialog::OCD_KEY);
	if(ocd)
	{
		ocd->show(mCallDialogPayload);
	}	
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLCallDialog
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LLCallDialog::LLCallDialog(const LLSD& payload)
	: LLDockableFloater(NULL, false, payload),

	  mPayload(payload),
	  mLifetime(DEFAULT_LIFETIME)
{
	setAutoFocus(FALSE);
	// force docked state since this floater doesn't save it between recreations
	setDocked(true);
}

LLCallDialog::~LLCallDialog()
{
	LLUI::removePopup(this);
}

BOOL LLCallDialog::postBuild()
{
	if (!LLDockableFloater::postBuild() || !gToolBarView)
		return FALSE;
	
	dockToToolbarButton("speak");
	
	return TRUE;
}

void LLCallDialog::dockToToolbarButton(const std::string& toolbarButtonName)
{
	LLDockControl::DocAt dock_pos = getDockControlPos(toolbarButtonName);
	LLView *anchor_panel = gToolBarView->findChildView(toolbarButtonName);

	setUseTongue(anchor_panel);

	setDockControl(new LLDockControl(anchor_panel, this, getDockTongue(dock_pos), dock_pos));
}

LLDockControl::DocAt LLCallDialog::getDockControlPos(const std::string& toolbarButtonName)
{
	LLCommandId command_id(toolbarButtonName);
	S32 toolbar_loc = gToolBarView->hasCommand(command_id);
	
	LLDockControl::DocAt doc_at = LLDockControl::TOP;
	
	switch (toolbar_loc)
	{
		case LLToolBarView::TOOLBAR_LEFT:
			doc_at = LLDockControl::RIGHT;
			break;
			
		case LLToolBarView::TOOLBAR_RIGHT:
			doc_at = LLDockControl::LEFT;
			break;
	}
	
	return doc_at;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLOutgoingCallDialog
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LLOutgoingCallDialog::LLOutgoingCallDialog(const LLSD& payload) :
LLCallDialog(payload)
{
	LLOutgoingCallDialog* instance = LLFloaterReg::findTypedInstance<LLOutgoingCallDialog>("outgoing_call", LLOutgoingCallDialog::OCD_KEY);
	if(instance && instance->getVisible())
	{
		instance->onCancel(instance);
	}	
}

void LLCallDialog::draw()
{
	if (lifetimeHasExpired())
	{
		onLifetimeExpired();
	}

	if (getDockControl() != NULL)
	{
		LLDockableFloater::draw();
	}
}

// virtual
void LLCallDialog::onOpen(const LLSD& key)
{
	LLDockableFloater::onOpen(key);

	// it should be over the all floaters. EXT-5116
	LLUI::addPopup(this);
}

void LLCallDialog::setIcon(const LLSD& session_id, const LLSD& participant_id)
{
	// *NOTE: 12/28/2009: check avaline calls: LLVoiceClient::isParticipantAvatar returns false for them
	bool participant_is_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(session_id);

	bool is_group = participant_is_avatar && gAgent.isInGroup(session_id);

	LLAvatarIconCtrl* avatar_icon = getChild<LLAvatarIconCtrl>("avatar_icon");
	LLGroupIconCtrl* group_icon = getChild<LLGroupIconCtrl>("group_icon");

	avatar_icon->setVisible(!is_group);
	group_icon->setVisible(is_group);

	if (is_group)
	{
		group_icon->setValue(session_id);
	}
	else if (participant_is_avatar)
	{
		avatar_icon->setValue(participant_id);
	}
	else
	{
		avatar_icon->setValue("Avaline_Icon");
		avatar_icon->setToolTip(std::string(""));
	}
}

bool LLCallDialog::lifetimeHasExpired()
{
	if (mLifetimeTimer.getStarted())
	{
		F32 elapsed_time = mLifetimeTimer.getElapsedTimeF32();
		if (elapsed_time > mLifetime) 
		{
			return true;
		}
	}
	return false;
}

void LLCallDialog::onLifetimeExpired()
{
	mLifetimeTimer.stop();
	closeFloater();
}

void LLOutgoingCallDialog::show(const LLSD& key)
{
	mPayload = key;

	//will be false only if voice in parcel is disabled and channel we leave is nearby(checked further)
	bool show_oldchannel = LLViewerParcelMgr::getInstance()->allowAgentVoice();

	// hide all text at first
	hideAllText();

	// init notification's lifetime
	std::istringstream ss( getString("lifetime") );
	if (!(ss >> mLifetime))
	{
		mLifetime = DEFAULT_LIFETIME;
	}

	// customize text strings
	// tell the user which voice channel they are leaving
	if (!mPayload["old_channel_name"].asString().empty())
	{
		bool was_avaline_call = LLIMModel::LLIMSession::AVALINE_SESSION == mPayload["old_session_type"].asInteger();

		std::string old_caller_name = mPayload["old_channel_name"].asString();
		if (was_avaline_call)
		{
			old_caller_name = LLTextUtil::formatPhoneNumber(old_caller_name);
		}

		getChild<LLUICtrl>("leaving")->setTextArg("[CURRENT_CHAT]", old_caller_name);
		show_oldchannel = true;
	}
	else
	{
		getChild<LLUICtrl>("leaving")->setTextArg("[CURRENT_CHAT]", getString("localchat"));		
	}

	if (!mPayload["disconnected_channel_name"].asString().empty())
	{
		std::string channel_name = mPayload["disconnected_channel_name"].asString();
		if (LLIMModel::LLIMSession::AVALINE_SESSION == mPayload["session_type"].asInteger())
		{
			channel_name = LLTextUtil::formatPhoneNumber(channel_name);
		}
		getChild<LLUICtrl>("nearby")->setTextArg("[VOICE_CHANNEL_NAME]", channel_name);

		// skipping "You will now be reconnected to nearby" in notification when call is ended by disabling voice,
		// so no reconnection to nearby chat happens (EXT-4397)
		bool voice_works = LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();
		std::string reconnect_nearby = voice_works ? LLTrans::getString("reconnect_nearby") : std::string();
		getChild<LLUICtrl>("nearby")->setTextArg("[RECONNECT_NEARBY]", reconnect_nearby);

		const std::string& nearby_str = mPayload["ended_by_agent"] ? NEARBY_P2P_BY_AGENT : NEARBY_P2P_BY_OTHER;
		getChild<LLUICtrl>(nearby_str)->setTextArg("[RECONNECT_NEARBY]", reconnect_nearby);
	}

	std::string callee_name = mPayload["session_name"].asString();

	LLUUID session_id = mPayload["session_id"].asUUID();
	bool is_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(session_id);

	if (callee_name == "anonymous")
	{
		callee_name = getString("anonymous");
	}
	else if (!is_avatar)
	{
		callee_name = LLTextUtil::formatPhoneNumber(callee_name);
	}
	
	LLSD callee_id = mPayload["other_user_id"];
	// Beautification:  Since you know who you called, just show display name
	std::string title = callee_name;
	std::string final_callee_name = callee_name;
	if (mPayload["session_type"].asInteger() == LLIMModel::LLIMSession::P2P_SESSION)
	{
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(callee_id, &av_name))
		{
			final_callee_name = av_name.mDisplayName;
			title = av_name.getCompleteName();
		}
	}
	getChild<LLUICtrl>("calling")->setTextArg("[CALLEE_NAME]", final_callee_name);
	getChild<LLUICtrl>("connecting")->setTextArg("[CALLEE_NAME]", final_callee_name);

	setTitle(title);

	// for outgoing group calls callee_id == group id == session id
	setIcon(callee_id, callee_id);

	// stop timer by default
	mLifetimeTimer.stop();

	// show only necessary strings and controls
	switch(mPayload["state"].asInteger())
	{
	case LLVoiceChannel::STATE_CALL_STARTED :
		getChild<LLTextBox>("calling")->setVisible(true);
		getChild<LLButton>("Cancel")->setVisible(true);
		if(show_oldchannel)
		{
			getChild<LLTextBox>("leaving")->setVisible(true);
		}
		break;
	// STATE_READY is here to show appropriate text for ad-hoc and group calls when floater is shown(EXT-6893)
	case LLVoiceChannel::STATE_READY :
	case LLVoiceChannel::STATE_RINGING :
		if(show_oldchannel)
		{
			getChild<LLTextBox>("leaving")->setVisible(true);
		}
		getChild<LLTextBox>("connecting")->setVisible(true);
		break;
	case LLVoiceChannel::STATE_ERROR :
		getChild<LLTextBox>("noanswer")->setVisible(true);
		getChild<LLButton>("Cancel")->setVisible(false);
		setCanClose(true);
		mLifetimeTimer.start();
		break;
	case LLVoiceChannel::STATE_HUNG_UP :
		if (mPayload["session_type"].asInteger() == LLIMModel::LLIMSession::P2P_SESSION)
		{
			const std::string& nearby_str = mPayload["ended_by_agent"] ? NEARBY_P2P_BY_AGENT : NEARBY_P2P_BY_OTHER;
			getChild<LLTextBox>(nearby_str)->setVisible(true);
		} 
		else
		{
			getChild<LLTextBox>("nearby")->setVisible(true);
		}
		getChild<LLButton>("Cancel")->setVisible(false);
		setCanClose(true);
		mLifetimeTimer.start();
	}

	openFloater(LLOutgoingCallDialog::OCD_KEY);
}

void LLOutgoingCallDialog::hideAllText()
{
	getChild<LLTextBox>("calling")->setVisible(false);
	getChild<LLTextBox>("leaving")->setVisible(false);
	getChild<LLTextBox>("connecting")->setVisible(false);
	getChild<LLTextBox>("nearby_P2P_by_other")->setVisible(false);
	getChild<LLTextBox>("nearby_P2P_by_agent")->setVisible(false);
	getChild<LLTextBox>("nearby")->setVisible(false);
	getChild<LLTextBox>("noanswer")->setVisible(false);
}

//static
void LLOutgoingCallDialog::onCancel(void* user_data)
{
	LLOutgoingCallDialog* self = (LLOutgoingCallDialog*)user_data;

	if (!gIMMgr)
		return;

	LLUUID session_id = self->mPayload["session_id"].asUUID();
	gIMMgr->endCall(session_id);
	
	self->closeFloater();
}


BOOL LLOutgoingCallDialog::postBuild()
{
	BOOL success = LLCallDialog::postBuild();

	childSetAction("Cancel", onCancel, this);

	setCanDrag(FALSE);

	return success;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIncomingCallDialog
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LLIncomingCallDialog::LLIncomingCallDialog(const LLSD& payload) :
LLCallDialog(payload)
{
}

void LLIncomingCallDialog::onLifetimeExpired()
{
	std::string session_handle = mPayload["session_handle"].asString();
	if (LLVoiceClient::getInstance()->isValidChannel(session_handle))
	{
		// restart notification's timer if call is still valid
		mLifetimeTimer.start();
	}
	else
	{
		// close invitation if call is already not valid
		mLifetimeTimer.stop();
		LLUUID session_id = mPayload["session_id"].asUUID();
		gIMMgr->clearPendingAgentListUpdates(session_id);
		gIMMgr->clearPendingInvitation(session_id);
		closeFloater();
	}
}

BOOL LLIncomingCallDialog::postBuild()
{
	LLCallDialog::postBuild();

	LLUUID session_id = mPayload["session_id"].asUUID();
	LLSD caller_id = mPayload["caller_id"];
	std::string caller_name = mPayload["caller_name"].asString();
	
	// init notification's lifetime
	std::istringstream ss( getString("lifetime") );
	if (!(ss >> mLifetime))
	{
		mLifetime = DEFAULT_LIFETIME;
	}

	std::string call_type;
	if (gAgent.isInGroup(session_id))
	{
		LLStringUtil::format_map_t args;
		LLGroupData data;
		if (gAgent.getGroupData(session_id, data))
		{
			args["[GROUP]"] = data.mName;
			call_type = getString(mPayload["notify_box_type"], args);
		}
	}
	else
	{
		call_type = getString(mPayload["notify_box_type"]);
	}
		
	
	// check to see if this is an Avaline call
	bool is_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(session_id);
	if (caller_name == "anonymous")
	{
		caller_name = getString("anonymous");
		setCallerName(caller_name, caller_name, call_type);
	}
	else if (!is_avatar)
	{
		caller_name = LLTextUtil::formatPhoneNumber(caller_name);
		setCallerName(caller_name, caller_name, call_type);
	}
	else
	{
		// Get the full name information
		LLAvatarNameCache::get(caller_id,
			boost::bind(&LLIncomingCallDialog::onAvatarNameCache,
				this, _1, _2, call_type));
	}

	setIcon(session_id, caller_id);

	childSetAction("Accept", onAccept, this);
	childSetAction("Reject", onReject, this);
	childSetAction("Start IM", onStartIM, this);
	setDefaultBtn("Accept");

	std::string notify_box_type = mPayload["notify_box_type"].asString();
	if(notify_box_type != "VoiceInviteGroup" && notify_box_type != "VoiceInviteAdHoc")
	{
		// starting notification's timer for P2P and AVALINE invitations
		mLifetimeTimer.start();
	}
	else
	{
		mLifetimeTimer.stop();
	}

	//it's not possible to connect to existing Ad-Hoc/Group chat through incoming ad-hoc call
	//and no IM for avaline
	getChildView("Start IM")->setVisible( is_avatar && notify_box_type != "VoiceInviteAdHoc" && notify_box_type != "VoiceInviteGroup");

	setCanDrag(FALSE);

	return TRUE;
}

void LLIncomingCallDialog::setCallerName(const std::string& ui_title,
										 const std::string& ui_label,
										 const std::string& call_type)
{
	setTitle(ui_title);

	// call_type may be a string like " is calling."
	LLUICtrl* caller_name_widget = getChild<LLUICtrl>("caller name");
	caller_name_widget->setValue(ui_label + " " + call_type);
}

void LLIncomingCallDialog::onAvatarNameCache(const LLUUID& agent_id,
											 const LLAvatarName& av_name,
											 const std::string& call_type)
{
	std::string title = av_name.getCompleteName();
	setCallerName(title, av_name.mDisplayName, call_type);
}

void LLIncomingCallDialog::onOpen(const LLSD& key)
{
	LLCallDialog::onOpen(key);

	LLStringUtil::format_map_t args;
	LLGroupData data;
	// if it's a group call, retrieve group name to use it in question
	if (gAgent.getGroupData(key["session_id"].asUUID(), data))
	{
		args["[GROUP]"] = data.mName;
	}
	// tell the user which voice channel they would be leaving
	LLVoiceChannel *voice = LLVoiceChannel::getCurrentVoiceChannel();
	if (voice && !voice->getSessionName().empty())
	{
		args["[CURRENT_CHAT]"] = voice->getSessionName();
		getChild<LLUICtrl>("question")->setValue(getString(key["question_type"].asString(), args));
	}
	else
	{
		args["[CURRENT_CHAT]"] = getString("localchat");
		getChild<LLUICtrl>("question")->setValue(getString(key["question_type"].asString(), args));
	}
}

//static
void LLIncomingCallDialog::onAccept(void* user_data)
{
	LLIncomingCallDialog* self = (LLIncomingCallDialog*)user_data;
	processCallResponse(0, self->mPayload);
	self->closeFloater();
}

//static
void LLIncomingCallDialog::onReject(void* user_data)
{
	LLIncomingCallDialog* self = (LLIncomingCallDialog*)user_data;
	processCallResponse(1, self->mPayload);
	self->closeFloater();
}

//static
void LLIncomingCallDialog::onStartIM(void* user_data)
{
	LLIncomingCallDialog* self = (LLIncomingCallDialog*)user_data;
	processCallResponse(2, self->mPayload);
	self->closeFloater();
}

// static
void LLIncomingCallDialog::processCallResponse(S32 response, const LLSD &payload)
{
	if (!gIMMgr || gDisconnected)
		return;

	LLUUID session_id = payload["session_id"].asUUID();
	LLUUID caller_id = payload["caller_id"].asUUID();
	std::string session_name = payload["session_name"].asString();
	EInstantMessage type = (EInstantMessage)payload["type"].asInteger();
	LLIMMgr::EInvitationType inv_type = (LLIMMgr::EInvitationType)payload["inv_type"].asInteger();
	bool voice = true;
	switch(response)
	{
	case 2: // start IM: just don't start the voice chat
	{
		voice = false;
		/* FALLTHROUGH */
	}
	case 0: // accept
	{
		if (type == IM_SESSION_P2P_INVITE)
		{
			// create a normal IM session
			session_id = gIMMgr->addP2PSession(
				session_name,
				caller_id,
				payload["session_handle"].asString(),
				payload["session_uri"].asString());

			if (voice)
			{
				gIMMgr->startCall(session_id, LLVoiceChannel::INCOMING_CALL);
			}

			gIMMgr->clearPendingAgentListUpdates(session_id);
			gIMMgr->clearPendingInvitation(session_id);
		}
		else
		{
			//session name should not be empty, but it can contain spaces so we don't trim
			std::string correct_session_name = session_name;
			if (session_name.empty())
			{
				llwarns << "Received an empty session name from a server" << llendl;
				
				switch(type){
				case IM_SESSION_CONFERENCE_START:
				case IM_SESSION_GROUP_START:
				case IM_SESSION_INVITE:		
					if (gAgent.isInGroup(session_id))
					{
						LLGroupData data;
						if (!gAgent.getGroupData(session_id, data)) break;
						correct_session_name = data.mName;
					}
					else
					{
						// *NOTE: really should be using callbacks here
						LLAvatarName av_name;
						if (LLAvatarNameCache::get(caller_id, &av_name))
						{
							correct_session_name = av_name.getCompleteName();
							correct_session_name.append(ADHOC_NAME_SUFFIX); 
						}
					}
					llinfos << "Corrected session name is " << correct_session_name << llendl; 
					break;
				default: 
					llwarning("Received an empty session name from a server and failed to generate a new proper session name", 0);
					break;
				}
			}
			
			LLUUID new_session_id = gIMMgr->addSession(correct_session_name, type, session_id, true);

			std::string url = gAgent.getRegion()->getCapability(
				"ChatSessionRequest");

			if (voice)
			{
				LLSD data;
				data["method"] = "accept invitation";
				data["session-id"] = session_id;
				LLHTTPClient::post(
					url,
					data,
					new LLViewerChatterBoxInvitationAcceptResponder(
						session_id,
						inv_type));

				// send notification message to the corresponding chat 
				if (payload["notify_box_type"].asString() == "VoiceInviteGroup" || payload["notify_box_type"].asString() == "VoiceInviteAdHoc")
				{
					LLStringUtil::format_map_t string_args;
					string_args["[NAME]"] = payload["caller_name"].asString();
					std::string message = LLTrans::getString("name_started_call", string_args);
					LLIMModel::getInstance()->addMessageSilently(session_id, SYSTEM_FROM, LLUUID::null, message);
				}
			}
		}
		if (voice)
		{
			break;
		}
	}
	case 1: // decline
	{
		if (type == IM_SESSION_P2P_INVITE)
		{
			if(LLVoiceClient::getInstance())
			{
				std::string s = payload["session_handle"].asString();
				LLVoiceClient::getInstance()->declineInvite(s);
			}
		}
		else
		{
			std::string url = gAgent.getRegion()->getCapability(
				"ChatSessionRequest");

			LLSD data;
			data["method"] = "decline invitation";
			data["session-id"] = session_id;
			LLHTTPClient::post(
				url,
				data,
				NULL);
		}
	}

	gIMMgr->clearPendingAgentListUpdates(session_id);
	gIMMgr->clearPendingInvitation(session_id);
	}
}

bool inviteUserResponse(const LLSD& notification, const LLSD& response)
{
	if (!gIMMgr)
		return false;

	const LLSD& payload = notification["payload"];
	LLUUID session_id = payload["session_id"].asUUID();
	EInstantMessage type = (EInstantMessage)payload["type"].asInteger();
	LLIMMgr::EInvitationType inv_type = (LLIMMgr::EInvitationType)payload["inv_type"].asInteger();
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch(option) 
	{
	case 0: // accept
		{
			if (type == IM_SESSION_P2P_INVITE)
			{
				// create a normal IM session
				session_id = gIMMgr->addP2PSession(
					payload["session_name"].asString(),
					payload["caller_id"].asUUID(),
					payload["session_handle"].asString(),
					payload["session_uri"].asString());

				gIMMgr->startCall(session_id);

				gIMMgr->clearPendingAgentListUpdates(session_id);
				gIMMgr->clearPendingInvitation(session_id);
			}
			else
			{
				LLUUID new_session_id = gIMMgr->addSession(
					payload["session_name"].asString(),
					type,
					session_id, true);

				std::string url = gAgent.getRegion()->getCapability(
					"ChatSessionRequest");

				LLSD data;
				data["method"] = "accept invitation";
				data["session-id"] = session_id;
				LLHTTPClient::post(
					url,
					data,
					new LLViewerChatterBoxInvitationAcceptResponder(
						session_id,
						inv_type));
			}
		}
		break;
	case 2: // mute (also implies ignore, so this falls through to the "ignore" case below)
	{
		// mute the sender of this invite
		if (!LLMuteList::getInstance()->isMuted(payload["caller_id"].asUUID()))
		{
			LLMute mute(payload["caller_id"].asUUID(), payload["caller_name"].asString(), LLMute::AGENT);
			LLMuteList::getInstance()->add(mute);
		}
	}
	/* FALLTHROUGH */
	
	case 1: // decline
	{
		if (type == IM_SESSION_P2P_INVITE)
		{
		  std::string s = payload["session_handle"].asString();
		  LLVoiceClient::getInstance()->declineInvite(s);
		}
		else
		{
			std::string url = gAgent.getRegion()->getCapability(
				"ChatSessionRequest");

			LLSD data;
			data["method"] = "decline invitation";
			data["session-id"] = session_id;
			LLHTTPClient::post(
				url,
				data,
				NULL);				
		}
	}

	gIMMgr->clearPendingAgentListUpdates(session_id);
	gIMMgr->clearPendingInvitation(session_id);
	break;
	}
	
	return false;
}

//
// Member Functions
//

LLIMMgr::LLIMMgr()
{
	mPendingInvitations = LLSD::emptyMap();
	mPendingAgentListUpdates = LLSD::emptyMap();

	LLIMModel::getInstance()->addNewMsgCallback(boost::bind(&LLIMFloater::sRemoveTypingIndicator, _1));
}

// Add a message to a session. 
void LLIMMgr::addMessage(
	const LLUUID& session_id,
	const LLUUID& target_id,
	const std::string& from,
	const std::string& msg,
	const std::string& session_name,
	EInstantMessage dialog,
	U32 parent_estate_id,
	const LLUUID& region_id,
	const LLVector3& position,
	bool link_name) // If this is true, then we insert the name and link it to a profile
{
	LLUUID other_participant_id = target_id;
	LLUUID new_session_id = session_id;
	if (new_session_id.isNull())
	{
		//no session ID...compute new one
		new_session_id = computeSessionID(dialog, other_participant_id);
	}

	//*NOTE session_name is empty in case of incoming P2P sessions
	std::string fixed_session_name = from;
	if(!session_name.empty() && session_name.size()>1)
	{
		fixed_session_name = session_name;
	}

	bool new_session = !hasSession(new_session_id);
	if (new_session)
	{
		LLIMModel::getInstance()->newSession(new_session_id, fixed_session_name, dialog, other_participant_id);

		// When we get a new IM, and if you are a god, display a bit
		// of information about the source. This is to help liaisons
		// when answering questions.
		if(gAgent.isGodlike())
		{
			// *TODO:translate (low priority, god ability)
			std::ostringstream bonus_info;
			bonus_info << LLTrans::getString("***")+ " "+ LLTrans::getString("IMParentEstate") + ":" + " "
				<< parent_estate_id
				<< ((parent_estate_id == 1) ? "," + LLTrans::getString("IMMainland") : "")
				<< ((parent_estate_id == 5) ? "," + LLTrans::getString ("IMTeen") : "");

			// once we have web-services (or something) which returns
			// information about a region id, we can print this out
			// and even have it link to map-teleport or something.
			//<< "*** region_id: " << region_id << std::endl
			//<< "*** position: " << position << std::endl;

			LLIMModel::instance().addMessage(new_session_id, from, other_participant_id, bonus_info.str());
		}

		// Logically it would make more sense to reject the session sooner, in another area of the
		// code, but the session has to be established inside the server before it can be left.
		if (LLMuteList::getInstance()->isMuted(other_participant_id) && !LLMuteList::getInstance()->isLinden(from))
		{
			llwarns << "Leaving IM session from initiating muted resident " << from << llendl;
			if(!gIMMgr->leaveSession(new_session_id))
			{
				llinfos << "Session " << new_session_id << " does not exist." << llendl;
			}
			return;
		}

		make_ui_sound("UISndNewIncomingIMSession");
	}

	bool skip_message = (gSavedSettings.getBOOL("VoiceCallsFriendsOnly") &&
		LLAvatarTracker::instance().getBuddyInfo(other_participant_id) == NULL);

	if (!LLMuteList::getInstance()->isMuted(other_participant_id, LLMute::flagTextChat) && !skip_message)
	{
		LLIMModel::instance().addMessage(new_session_id, from, other_participant_id, msg);
	}
}

void LLIMMgr::addSystemMessage(const LLUUID& session_id, const std::string& message_name, const LLSD& args)
{
	LLUIString message;
	
	// null session id means near me (chat history)
	if (session_id.isNull())
	{
		message = LLTrans::getString(message_name);
		message.setArgs(args);

		LLChat chat(message);
		chat.mSourceType = CHAT_SOURCE_SYSTEM;
		
		LLFloater* chat_bar = LLFloaterReg::getInstance("chat_bar");
		LLNearbyChat* nearby_chat = chat_bar->findChild<LLNearbyChat>("nearby_chat");

		if(nearby_chat)
		{
			nearby_chat->addMessage(chat);
		}
	}
	else // going to IM session
	{
		message = LLTrans::getString(message_name + "-im");
		message.setArgs(args);
		if (hasSession(session_id))
		{
			gIMMgr->addMessage(session_id, LLUUID::null, SYSTEM_FROM, message.getString());
		}
		// log message to file
		else
		{
			std::string session_name;
			// since we select user to share item with - his name is already in cache
			gCacheName->getFullName(args["user_id"], session_name);
			session_name = LLCacheName::buildUsername(session_name);
			LLIMModel::instance().logToFile(session_name, SYSTEM_FROM, LLUUID::null, message.getString());
		}
	}
}

S32 LLIMMgr::getNumberOfUnreadIM()
{
	std::map<LLUUID, LLIMModel::LLIMSession*>::iterator it;
	
	S32 num = 0;
	for(it = LLIMModel::getInstance()->mId2SessionMap.begin(); it != LLIMModel::getInstance()->mId2SessionMap.end(); ++it)
	{
		num += (*it).second->mNumUnread;
	}

	return num;
}

S32 LLIMMgr::getNumberOfUnreadParticipantMessages()
{
	std::map<LLUUID, LLIMModel::LLIMSession*>::iterator it;

	S32 num = 0;
	for(it = LLIMModel::getInstance()->mId2SessionMap.begin(); it != LLIMModel::getInstance()->mId2SessionMap.end(); ++it)
	{
		num += (*it).second->mParticipantUnreadMessageCount;
	}

	return num;
}

void LLIMMgr::autoStartCallOnStartup(const LLUUID& session_id)
{
	LLIMModel::LLIMSession *session = LLIMModel::getInstance()->findIMSession(session_id);
	if (!session) return;
	
	if (session->mSessionInitialized)
	{
		startCall(session_id);
	}
	else
	{
		session->mStartCallOnInitialize = true;
	}	
}

LLUUID LLIMMgr::addP2PSession(const std::string& name,
							const LLUUID& other_participant_id,
							const std::string& voice_session_handle,
							const std::string& caller_uri)
{
	LLUUID session_id = addSession(name, IM_NOTHING_SPECIAL, other_participant_id, true);

	LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);
	if (speaker_mgr)
	{
		LLVoiceChannelP2P* voice_channel = dynamic_cast<LLVoiceChannelP2P*>(speaker_mgr->getVoiceChannel());
		if (voice_channel)
		{
			voice_channel->setSessionHandle(voice_session_handle, caller_uri);
		}
	}
	return session_id;
}

// This adds a session to the talk view. The name is the local name of
// the session, dialog specifies the type of session. If the session
// exists, it is brought forward.  Specifying id = NULL results in an
// im session to everyone. Returns the uuid of the session.
LLUUID LLIMMgr::addSession(
	const std::string& name,
	EInstantMessage dialog,
	const LLUUID& other_participant_id, bool voice)
{
	LLDynamicArray<LLUUID> ids;
	ids.put(other_participant_id);
	return addSession(name, dialog, other_participant_id, ids, voice);
}

// Adds a session using the given session_id.  If the session already exists 
// the dialog type is assumed correct. Returns the uuid of the session.
LLUUID LLIMMgr::addSession(
	const std::string& name,
	EInstantMessage dialog,
	const LLUUID& other_participant_id,
	const LLDynamicArray<LLUUID>& ids, bool voice)
{
	if (0 == ids.getLength())
	{
		return LLUUID::null;
	}

	if (name.empty())
	{
		llwarning("Session name cannot be null!", 0);
		return LLUUID::null;
	}

	LLUUID session_id = computeSessionID(dialog,other_participant_id);

	bool new_session = !LLIMModel::getInstance()->findIMSession(session_id);

	//works only for outgoing ad-hoc sessions
	if (new_session && IM_SESSION_CONFERENCE_START == dialog && ids.size())
	{
		LLIMModel::LLIMSession* ad_hoc_found = LLIMModel::getInstance()->findAdHocIMSession(ids);
		if (ad_hoc_found)
		{
			new_session = false;
			session_id = ad_hoc_found->mSessionID;
		}
	}

	if (new_session)
	{
		LLIMModel::getInstance()->newSession(session_id, name, dialog, other_participant_id, ids, voice);
	}

	//we don't need to show notes about online/offline, mute/unmute users' statuses for existing sessions
	if (!new_session) return session_id;
	
	//Per Plan's suggestion commented "explicit offline status warning" out to make Dessie happier (see EXT-3609)
	//*TODO After February 2010 remove this commented out line if no one will be missing that warning
	//noteOfflineUsers(session_id, floater, ids);

	// Only warn for regular IMs - not group IMs
	if( dialog == IM_NOTHING_SPECIAL )
	{
		noteMutedUsers(session_id, ids);
	}

	return session_id;
}

bool LLIMMgr::leaveSession(const LLUUID& session_id)
{
	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
	if (!im_session) return false;

	LLIMModel::getInstance()->sendLeaveSession(session_id, im_session->mOtherParticipantID);
	gIMMgr->removeSession(session_id);
	return true;
}

// Removes data associated with a particular session specified by session_id
void LLIMMgr::removeSession(const LLUUID& session_id)
{
	llassert_always(hasSession(session_id));
	
	clearPendingInvitation(session_id);
	clearPendingAgentListUpdates(session_id);

	LLIMModel::getInstance()->clearSession(session_id);

	notifyObserverSessionRemoved(session_id);
}

void LLIMMgr::inviteToSession(
	const LLUUID& session_id, 
	const std::string& session_name, 
	const LLUUID& caller_id, 
	const std::string& caller_name,
	EInstantMessage type,
	EInvitationType inv_type,
	const std::string& session_handle,
	const std::string& session_uri)
{
	std::string notify_box_type;
	// voice invite question is different from default only for group call (EXT-7118)
	std::string question_type = "VoiceInviteQuestionDefault";

	BOOL voice_invite = FALSE;
	bool is_linden = LLMuteList::getInstance()->isLinden(caller_name);


	if(type == IM_SESSION_P2P_INVITE)
	{
		//P2P is different...they only have voice invitations
		notify_box_type = "VoiceInviteP2P";
		voice_invite = TRUE;
	}
	else if ( gAgent.isInGroup(session_id) )
	{
		//only really old school groups have voice invitations
		notify_box_type = "VoiceInviteGroup";
		question_type = "VoiceInviteQuestionGroup";
		voice_invite = TRUE;
	}
	else if ( inv_type == INVITATION_TYPE_VOICE )
	{
		//else it's an ad-hoc
		//and a voice ad-hoc
		notify_box_type = "VoiceInviteAdHoc";
		voice_invite = TRUE;
	}
	else if ( inv_type == INVITATION_TYPE_IMMEDIATE )
	{
		notify_box_type = "InviteAdHoc";
	}

	LLSD payload;
	payload["session_id"] = session_id;
	payload["session_name"] = session_name;
	payload["caller_id"] = caller_id;
	payload["caller_name"] = caller_name;
	payload["type"] = type;
	payload["inv_type"] = inv_type;
	payload["session_handle"] = session_handle;
	payload["session_uri"] = session_uri;
	payload["notify_box_type"] = notify_box_type;
	payload["question_type"] = question_type;

	//ignore invites from muted residents
	if (LLMuteList::getInstance()->isMuted(caller_id) && !is_linden)
	{
		if (voice_invite && "VoiceInviteQuestionDefault" == question_type)
		{
			llinfos << "Rejecting voice call from initiating muted resident " << caller_name << llendl;
			LLIncomingCallDialog::processCallResponse(1, payload);
		}
		return;
	}

	LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(session_id);
	if (channelp && channelp->callStarted())
	{
		// you have already started a call to the other user, so just accept the invite
		LLIncomingCallDialog::processCallResponse(0, payload);
		return;
	}

	if (voice_invite)
	{
		if	(	// if we are rejecting group calls 
				(gSavedSettings.getBOOL("VoiceCallsRejectGroup") && notify_box_type == "VoiceInviteGroup") ||
				// or we're rejecting non-friend voice calls and this isn't a friend	
				(gSavedSettings.getBOOL("VoiceCallsFriendsOnly") && (LLAvatarTracker::instance().getBuddyInfo(caller_id) == NULL))
			)
		{
			// silently decline the call
			LLIncomingCallDialog::processCallResponse(1, payload);
			return;
		}
	}

	if ( !mPendingInvitations.has(session_id.asString()) )
	{
		if (caller_name.empty())
		{
			gCacheName->get(caller_id, false,  // voice
				boost::bind(&LLIMMgr::onInviteNameLookup, payload, _1, _2, _3));
		}
		else
		{
			LLFloaterReg::showInstance("incoming_call", payload, FALSE);
		}
		
		// Add the caller to the Recent List here (at this point 
		// "incoming_call" floater is shown and the recipient can
		// reject the call), because even if a recipient will reject
		// the call, the caller should be added to the recent list
		// anyway. STORM-507.
		if(type == IM_SESSION_P2P_INVITE)
			LLRecentPeople::instance().add(caller_id);
		
		mPendingInvitations[session_id.asString()] = LLSD();
	}
}

void LLIMMgr::onInviteNameLookup(LLSD payload, const LLUUID& id, const std::string& name, bool is_group)
{
	payload["caller_name"] = name;
	payload["session_name"] = payload["caller_name"].asString();

	std::string notify_box_type = payload["notify_box_type"].asString();

	LLFloaterReg::showInstance("incoming_call", payload, FALSE);
}

//*TODO disconnects all sessions
void LLIMMgr::disconnectAllSessions()
{
	//*TODO disconnects all IM sessions
}

BOOL LLIMMgr::hasSession(const LLUUID& session_id)
{
	return LLIMModel::getInstance()->findIMSession(session_id) != NULL;
}

void LLIMMgr::clearPendingInvitation(const LLUUID& session_id)
{
	if ( mPendingInvitations.has(session_id.asString()) )
	{
		mPendingInvitations.erase(session_id.asString());
	}
}

void LLIMMgr::processAgentListUpdates(const LLUUID& session_id, const LLSD& body)
{
	LLIMFloater* im_floater = LLIMFloater::findInstance(session_id);
	if ( im_floater )
	{
		im_floater->processAgentListUpdates(body);
	}
	LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);
	if (speaker_mgr)
	{
		speaker_mgr->updateSpeakers(body);

		// also the same call is added into LLVoiceClient::participantUpdatedEvent because
		// sometimes it is called AFTER LLViewerChatterBoxSessionAgentListUpdates::post()
		// when moderation state changed too late. See EXT-3544.
		speaker_mgr->update(true);
	}
	else
	{
		//we don't have a speaker manager yet..something went wrong
		//we are probably receiving an update here before
		//a start or an acceptance of an invitation.  Race condition.
		gIMMgr->addPendingAgentListUpdates(
			session_id,
			body);
	}
}

LLSD LLIMMgr::getPendingAgentListUpdates(const LLUUID& session_id)
{
	if ( mPendingAgentListUpdates.has(session_id.asString()) )
	{
		return mPendingAgentListUpdates[session_id.asString()];
	}
	else
	{
		return LLSD();
	}
}

void LLIMMgr::addPendingAgentListUpdates(
	const LLUUID& session_id,
	const LLSD& updates)
{
	LLSD::map_const_iterator iter;

	if ( !mPendingAgentListUpdates.has(session_id.asString()) )
	{
		//this is a new agent list update for this session
		mPendingAgentListUpdates[session_id.asString()] = LLSD::emptyMap();
	}

	if (
		updates.has("agent_updates") &&
		updates["agent_updates"].isMap() &&
		updates.has("updates") &&
		updates["updates"].isMap() )
	{
		//new school update
		LLSD update_types = LLSD::emptyArray();
		LLSD::array_iterator array_iter;

		update_types.append("agent_updates");
		update_types.append("updates");

		for (
			array_iter = update_types.beginArray();
			array_iter != update_types.endArray();
			++array_iter)
		{
			//we only want to include the last update for a given agent
			for (
				iter = updates[array_iter->asString()].beginMap();
				iter != updates[array_iter->asString()].endMap();
				++iter)
			{
				mPendingAgentListUpdates[session_id.asString()][array_iter->asString()][iter->first] =
					iter->second;
			}
		}
	}
	else if (
		updates.has("updates") &&
		updates["updates"].isMap() )
	{
		//old school update where the SD contained just mappings
		//of agent_id -> "LEAVE"/"ENTER"

		//only want to keep last update for each agent
		for (
			iter = updates["updates"].beginMap();
			iter != updates["updates"].endMap();
			++iter)
		{
			mPendingAgentListUpdates[session_id.asString()]["updates"][iter->first] =
				iter->second;
		}
	}
}

void LLIMMgr::clearPendingAgentListUpdates(const LLUUID& session_id)
{
	if ( mPendingAgentListUpdates.has(session_id.asString()) )
	{
		mPendingAgentListUpdates.erase(session_id.asString());
	}
}

void LLIMMgr::notifyObserverSessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
	for (session_observers_list_t::iterator it = mSessionObservers.begin(); it != mSessionObservers.end(); it++)
	{
		(*it)->sessionAdded(session_id, name, other_participant_id);
	}
}

void LLIMMgr::notifyObserverSessionRemoved(const LLUUID& session_id)
{
	for (session_observers_list_t::iterator it = mSessionObservers.begin(); it != mSessionObservers.end(); it++)
	{
		(*it)->sessionRemoved(session_id);
	}
}

void LLIMMgr::notifyObserverSessionIDUpdated( const LLUUID& old_session_id, const LLUUID& new_session_id )
{
	for (session_observers_list_t::iterator it = mSessionObservers.begin(); it != mSessionObservers.end(); it++)
	{
		(*it)->sessionIDUpdated(old_session_id, new_session_id);
	}

}

void LLIMMgr::addSessionObserver(LLIMSessionObserver *observer)
{
	mSessionObservers.push_back(observer);
}

void LLIMMgr::removeSessionObserver(LLIMSessionObserver *observer)
{
	mSessionObservers.remove(observer);
}

bool LLIMMgr::startCall(const LLUUID& session_id, LLVoiceChannel::EDirection direction)
{
	LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(session_id);
	if (!voice_channel) return false;
	
	voice_channel->setCallDirection(direction);
	voice_channel->activate();
	return true;
}

bool LLIMMgr::endCall(const LLUUID& session_id)
{
	LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(session_id);
	if (!voice_channel) return false;

	voice_channel->deactivate();
	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
	if (im_session)
	{
		// need to update speakers' state
		im_session->mSpeakers->update(FALSE);
	}
	return true;
}

bool LLIMMgr::isVoiceCall(const LLUUID& session_id)
{
	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
	if (!im_session) return false;

	return im_session->mStartedAsIMCall;
}

void LLIMMgr::addNotifiedNonFriendSessionID(const LLUUID& session_id)
{
	mNotifiedNonFriendSessions.insert(session_id);
}

bool LLIMMgr::isNonFriendSessionNotified(const LLUUID& session_id)
{
	return mNotifiedNonFriendSessions.end() != mNotifiedNonFriendSessions.find(session_id);

}

void LLIMMgr::noteOfflineUsers(
	const LLUUID& session_id,
	const LLDynamicArray<LLUUID>& ids)
{
	S32 count = ids.count();
	if(count == 0)
	{
		const std::string& only_user = LLTrans::getString("only_user_message");
		LLIMModel::getInstance()->addMessage(session_id, SYSTEM_FROM, LLUUID::null, only_user);
	}
	else
	{
		const LLRelationship* info = NULL;
		LLAvatarTracker& at = LLAvatarTracker::instance();
		LLIMModel& im_model = LLIMModel::instance();
		for(S32 i = 0; i < count; ++i)
		{
			info = at.getBuddyInfo(ids.get(i));
			LLAvatarName av_name;
			if (info
				&& !info->isOnline()
				&& LLAvatarNameCache::get(ids.get(i), &av_name))
			{
				LLUIString offline = LLTrans::getString("offline_message");
				// Use display name only because this user is your friend
				offline.setArg("[NAME]", av_name.mDisplayName);
				im_model.proccessOnlineOfflineNotification(session_id, offline);
			}
		}
	}
}

void LLIMMgr::noteMutedUsers(const LLUUID& session_id,
								  const LLDynamicArray<LLUUID>& ids)
{
	// Don't do this if we don't have a mute list.
	LLMuteList *ml = LLMuteList::getInstance();
	if( !ml )
	{
		return;
	}

	S32 count = ids.count();
	if(count > 0)
	{
		LLIMModel* im_model = LLIMModel::getInstance();
		
		for(S32 i = 0; i < count; ++i)
		{
			if( ml->isMuted(ids.get(i)) )
			{
				LLUIString muted = LLTrans::getString("muted_message");

				im_model->addMessage(session_id, SYSTEM_FROM, LLUUID::null, muted);
				break;
			}
		}
	}
}

void LLIMMgr::processIMTypingStart(const LLIMInfo* im_info)
{
	processIMTypingCore(im_info, TRUE);
}

void LLIMMgr::processIMTypingStop(const LLIMInfo* im_info)
{
	processIMTypingCore(im_info, FALSE);
}

void LLIMMgr::processIMTypingCore(const LLIMInfo* im_info, BOOL typing)
{
	LLUUID session_id = computeSessionID(im_info->mIMType, im_info->mFromID);
	LLIMFloater* im_floater = LLIMFloater::findInstance(session_id);
	if ( im_floater )
	{
		im_floater->processIMTyping(im_info, typing);
	}
}

class LLViewerChatterBoxSessionStartReply : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("Used for receiving a reply to a request to initialize an ChatterBox session");
		desc.postAPI();
		desc.input(
			"{\"client_session_id\": UUID, \"session_id\": UUID, \"success\" boolean, \"reason\": string");
		desc.source(__FILE__, __LINE__);
	}

	virtual void post(ResponsePtr response,
					  const LLSD& context,
					  const LLSD& input) const
	{
		LLSD body;
		LLUUID temp_session_id;
		LLUUID session_id;
		bool success;

		body = input["body"];
		success = body["success"].asBoolean();
		temp_session_id = body["temp_session_id"].asUUID();

		if ( success )
		{
			session_id = body["session_id"].asUUID();

			LLIMModel::getInstance()->processSessionInitializedReply(temp_session_id, session_id);

			LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);
			if (speaker_mgr)
			{
				speaker_mgr->setSpeakers(body);
				speaker_mgr->updateSpeakers(gIMMgr->getPendingAgentListUpdates(session_id));
			}

			LLIMFloater* im_floater = LLIMFloater::findInstance(session_id);
			if ( im_floater )
			{
				if ( body.has("session_info") )
				{
					im_floater->processSessionUpdate(body["session_info"]);
				}
			}

			gIMMgr->clearPendingAgentListUpdates(session_id);
		}
		else
		{
			//throw an error dialog and close the temp session's floater
			gIMMgr->showSessionStartError(body["error"].asString(), temp_session_id);
		}

		gIMMgr->clearPendingAgentListUpdates(session_id);
	}
};

class LLViewerChatterBoxSessionEventReply : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("Used for receiving a reply to a ChatterBox session event");
		desc.postAPI();
		desc.input(
			"{\"event\": string, \"reason\": string, \"success\": boolean, \"session_id\": UUID");
		desc.source(__FILE__, __LINE__);
	}

	virtual void post(ResponsePtr response,
					  const LLSD& context,
					  const LLSD& input) const
	{
		LLUUID session_id;
		bool success;

		LLSD body = input["body"];
		success = body["success"].asBoolean();
		session_id = body["session_id"].asUUID();

		if ( !success )
		{
			//throw an error dialog
			gIMMgr->showSessionEventError(
				body["event"].asString(),
				body["error"].asString(),
				session_id);
		}
	}
};

class LLViewerForceCloseChatterBoxSession: public LLHTTPNode
{
public:
	virtual void post(ResponsePtr response,
					  const LLSD& context,
					  const LLSD& input) const
	{
		LLUUID session_id;
		std::string reason;

		session_id = input["body"]["session_id"].asUUID();
		reason = input["body"]["reason"].asString();

		gIMMgr->showSessionForceClose(reason, session_id);
	}
};

class LLViewerChatterBoxSessionAgentListUpdates : public LLHTTPNode
{
public:
	virtual void post(
		ResponsePtr responder,
		const LLSD& context,
		const LLSD& input) const
	{
		const LLUUID& session_id = input["body"]["session_id"].asUUID();
		gIMMgr->processAgentListUpdates(session_id, input["body"]);
	}
};

class LLViewerChatterBoxSessionUpdate : public LLHTTPNode
{
public:
	virtual void post(
		ResponsePtr responder,
		const LLSD& context,
		const LLSD& input) const
	{
		LLUUID session_id = input["body"]["session_id"].asUUID();
		LLIMFloater* im_floater = LLIMFloater::findInstance(session_id);
		if ( im_floater )
		{
			im_floater->processSessionUpdate(input["body"]["info"]);
		}
		LLIMSpeakerMgr* im_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);
		if (im_mgr)
		{
			im_mgr->processSessionUpdate(input["body"]["info"]);
		}
	}
};


class LLViewerChatterBoxInvitation : public LLHTTPNode
{
public:

	virtual void post(
		ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{
		//for backwards compatiblity reasons...we need to still
		//check for 'text' or 'voice' invitations...bleh
		if ( input["body"].has("instantmessage") )
		{
			LLSD message_params =
				input["body"]["instantmessage"]["message_params"];

			//do something here to have the IM invite behave
			//just like a normal IM
			//this is just replicated code from process_improved_im
			//and should really go in it's own function -jwolk

			std::string message = message_params["message"].asString();
			std::string name = message_params["from_name"].asString();
			LLUUID from_id = message_params["from_id"].asUUID();
			LLUUID session_id = message_params["id"].asUUID();
			std::vector<U8> bin_bucket = message_params["data"]["binary_bucket"].asBinary();
			U8 offline = (U8)message_params["offline"].asInteger();
			
			time_t timestamp =
				(time_t) message_params["timestamp"].asInteger();

			BOOL is_busy = gAgent.getBusy();
			BOOL is_muted = LLMuteList::getInstance()->isMuted(
				from_id,
				name,
				LLMute::flagTextChat);

			if (is_busy || is_muted)
			{
				return;
			}

			// standard message, not from system
			std::string saved;
			if(offline == IM_OFFLINE)
			{
				LLStringUtil::format_map_t args;
				args["[LONG_TIMESTAMP]"] = formatted_time(timestamp);
				saved = LLTrans::getString("Saved_message", args);
			}
			std::string buffer = saved + message;

			if(from_id == gAgentID)
			{
				return;
			}
			gIMMgr->addMessage(
				session_id,
				from_id,
				name,
				buffer,
				std::string((char*)&bin_bucket[0]),
				IM_SESSION_INVITE,
				message_params["parent_estate_id"].asInteger(),
				message_params["region_id"].asUUID(),
				ll_vector3_from_sd(message_params["position"]),
				true);

			if (LLMuteList::getInstance()->isMuted(from_id, name, LLMute::flagTextChat))
			{
				return;
			}

			//K now we want to accept the invitation
			std::string url = gAgent.getRegion()->getCapability(
				"ChatSessionRequest");

			if ( url != "" )
			{
				LLSD data;
				data["method"] = "accept invitation";
				data["session-id"] = session_id;
				LLHTTPClient::post(
					url,
					data,
					new LLViewerChatterBoxInvitationAcceptResponder(
						session_id,
						LLIMMgr::INVITATION_TYPE_INSTANT_MESSAGE));
			}
		} //end if invitation has instant message
		else if ( input["body"].has("voice") )
		{
			if(!LLVoiceClient::getInstance()->voiceEnabled() || !LLVoiceClient::getInstance()->isVoiceWorking())
			{
				// Don't display voice invites unless the user has voice enabled.
				return;
			}

			gIMMgr->inviteToSession(
				input["body"]["session_id"].asUUID(), 
				input["body"]["session_name"].asString(), 
				input["body"]["from_id"].asUUID(),
				input["body"]["from_name"].asString(),
				IM_SESSION_INVITE,
				LLIMMgr::INVITATION_TYPE_VOICE);
		}
		else if ( input["body"].has("immediate") )
		{
			gIMMgr->inviteToSession(
				input["body"]["session_id"].asUUID(), 
				input["body"]["session_name"].asString(), 
				input["body"]["from_id"].asUUID(),
				input["body"]["from_name"].asString(),
				IM_SESSION_INVITE,
				LLIMMgr::INVITATION_TYPE_IMMEDIATE);
		}
	}
};

LLHTTPRegistration<LLViewerChatterBoxSessionStartReply>
   gHTTPRegistrationMessageChatterboxsessionstartreply(
	   "/message/ChatterBoxSessionStartReply");

LLHTTPRegistration<LLViewerChatterBoxSessionEventReply>
   gHTTPRegistrationMessageChatterboxsessioneventreply(
	   "/message/ChatterBoxSessionEventReply");

LLHTTPRegistration<LLViewerForceCloseChatterBoxSession>
    gHTTPRegistrationMessageForceclosechatterboxsession(
		"/message/ForceCloseChatterBoxSession");

LLHTTPRegistration<LLViewerChatterBoxSessionAgentListUpdates>
    gHTTPRegistrationMessageChatterboxsessionagentlistupdates(
	    "/message/ChatterBoxSessionAgentListUpdates");

LLHTTPRegistration<LLViewerChatterBoxSessionUpdate>
    gHTTPRegistrationMessageChatterBoxSessionUpdate(
	    "/message/ChatterBoxSessionUpdate");

LLHTTPRegistration<LLViewerChatterBoxInvitation>
    gHTTPRegistrationMessageChatterBoxInvitation(
		"/message/ChatterBoxInvitation");

