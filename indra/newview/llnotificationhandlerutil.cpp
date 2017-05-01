/**
 * @file llnotificationofferhandler.cpp
 * @brief Provides set of utility methods for notifications processing.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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


#include "llviewerprecompiledheaders.h" // must be first include

#include "llavatarnamecache.h"

#include "llfloaterreg.h"
#include "llnotifications.h"
#include "llurlaction.h"

#include "llagent.h"
#include "llfloaterimsession.h"
#include "llimview.h"
#include "llfloaterimnearbychat.h"
#include "llnotificationhandler.h"

using namespace LLNotificationsUI;

LLNotificationHandler::LLNotificationHandler(const std::string& name, const std::string& notification_type, const std::string& parentName)
:	LLNotificationChannel(name, parentName, LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, notification_type))
{}

LLSystemNotificationHandler::LLSystemNotificationHandler(const std::string& name, const std::string& notification_type)
	: LLNotificationHandler(name, notification_type, "System")
{}

LLCommunicationNotificationHandler::LLCommunicationNotificationHandler(const std::string& name, const std::string& notification_type)
	: LLNotificationHandler(name, notification_type, "Communication")
{}

// static
bool LLHandlerUtil::isIMFloaterOpened(const LLNotificationPtr& notification)
{
	bool res = false;

	LLUUID from_id = notification->getPayload()["from_id"];
	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, from_id);
	LLFloaterIMSession* im_floater = LLFloaterReg::findTypedInstance<LLFloaterIMSession>("impanel", session_id);

	if (im_floater != NULL)
	{
		res = im_floater->getVisible() == TRUE;
	}

	return res;
}

// static
void LLHandlerUtil::logToIM(const EInstantMessage& session_type,
		const std::string& session_name, const std::string& from_name,
		const std::string& message, const LLUUID& session_owner_id,
		const LLUUID& from_id)
{
	std::string from = from_name;
	if (from_name.empty())
	{
		from = SYSTEM_FROM;
	}

	LLUUID session_id = LLIMMgr::computeSessionID(session_type,
			session_owner_id);
	LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(
			session_id);
	if (session == NULL)
	{
		// replace interactive system message marker with correct from string value
		if (INTERACTIVE_SYSTEM_FROM == from_name)
		{
			from = SYSTEM_FROM;
		}

		// Build a new format username or firstname_lastname for legacy names
		// to use it for a history log filename.
		std::string user_name = LLCacheName::buildUsername(session_name);
		LLIMModel::instance().logToFile(user_name, from, from_id, message);
	}
	else
	{
		S32 unread = session->mNumUnread;
		S32 participant_unread = session->mParticipantUnreadMessageCount;
		LLIMModel::instance().addMessageSilently(session_id, from, from_id,
				message);
		// we shouldn't increment counters when logging, so restore them
		session->mNumUnread = unread;
		session->mParticipantUnreadMessageCount = participant_unread;

		// update IM floater messages
		updateIMFLoaterMesages(session_id);
	}
}

void log_name_callback(const LLAvatarName& av_name, const std::string& from_name, 
					   const std::string& message, const LLUUID& from_id)

{
	LLHandlerUtil::logToIM(IM_NOTHING_SPECIAL, av_name.getUserName(), from_name, message,
					from_id, LLUUID());
}

// static
void LLHandlerUtil::logToIMP2P(const LLNotificationPtr& notification, bool to_file_only)
{
	if (!gCacheName)
	{
		return;
	}

	LLUUID from_id = notification->getPayload()["from_id"];

	if (from_id.isNull())
	{
		// Normal behavior for system generated messages, don't spam.
		// LL_WARNS() << " from_id for notification " << notification->getName() << " is null " << LL_ENDL;
		return;
	}

	if(to_file_only)
	{
		LLAvatarNameCache::get(from_id, boost::bind(&log_name_callback, _2, "", notification->getMessage(), LLUUID()));
	}
	else
	{
		LLAvatarNameCache::get(from_id, boost::bind(&log_name_callback, _2, INTERACTIVE_SYSTEM_FROM, notification->getMessage(), from_id));
	}
}

// static
void LLHandlerUtil::logGroupNoticeToIMGroup(
		const LLNotificationPtr& notification)
{

	const LLSD& payload = notification->getPayload();
	LLGroupData groupData;
	if (!gAgent.getGroupData(payload["group_id"].asUUID(), groupData))
	{
		LL_WARNS()
						<< "Group notice for unknown group: "
								<< payload["group_id"].asUUID() << LL_ENDL;
		return;
	}

	const std::string group_name = groupData.mName;
	const std::string sender_name = payload["sender_name"].asString();

	LLUUID sender_id;
	if (payload.has("sender_id"))
	{
		sender_id = payload["sender_id"].asUUID();
	}
	
	if (sender_id.notNull())
	{
		// Legacy support and fallback method
		// if we can't retrieve sender id from group notice system message, try to lookup it from cache
		sender_id = LLAvatarNameCache::findIdByName(sender_name);
	}

	logToIM(IM_SESSION_GROUP_START, group_name, sender_name, payload["message"],
			payload["group_id"], sender_id);
}

// static
void LLHandlerUtil::logToNearbyChat(const LLNotificationPtr& notification, EChatSourceType type)
{
    LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	if (nearby_chat)
	{
		LLChat chat_msg(notification->getMessage());
		chat_msg.mSourceType = type;
		chat_msg.mFromName = SYSTEM_FROM;
		chat_msg.mFromID = LLUUID::null;
		nearby_chat->addMessage(chat_msg);
	}
}

// static
LLUUID LLHandlerUtil::spawnIMSession(const std::string& name, const LLUUID& from_id)
{
	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, from_id);

	LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(
			session_id);
	if (session == NULL)
	{
		session_id = LLIMMgr::instance().addSession(name, IM_NOTHING_SPECIAL, from_id);
	}

	return session_id;
}

// static
std::string LLHandlerUtil::getSubstitutionName(const LLNotificationPtr& notification)
{
	std::string res = notification->getSubstitutions().has("NAME")
		? notification->getSubstitutions()["NAME"]
		: notification->getSubstitutions()["[NAME]"];
	if (res.empty())
	{
		LLUUID from_id = notification->getPayload()["FROM_ID"];

		//*TODO all keys everywhere should be made of the same case, there is a mix of keys in lower and upper cases
		if (from_id.isNull()) 
		{
			from_id = notification->getPayload()["from_id"];
		}
		LLAvatarName av_name;
		if(LLAvatarNameCache::get(from_id, &av_name))
		{
			res = av_name.getUserName();
		}
		else
		{
			res = "";
		}
	}
	return res;
}

// static
void LLHandlerUtil::addNotifPanelToIM(const LLNotificationPtr& notification)
{
	const std::string name = LLHandlerUtil::getSubstitutionName(notification);
	LLUUID from_id = notification->getPayload()["from_id"];

	LLUUID session_id = spawnIMSession(name, from_id);
	// add offer to session
	LLIMModel::LLIMSession * session = LLIMModel::getInstance()->findIMSession(
			session_id);
	llassert_always(session != NULL);

	LLSD offer;
	offer["notification_id"] = notification->getID();
	offer["from"] = SYSTEM_FROM;
	offer["time"] = LLLogChat::timestamp(false);
	offer["index"] = (LLSD::Integer)session->mMsgs.size();
	session->mMsgs.push_front(offer);


	// update IM floater and counters
	LLSD arg;
	arg["session_id"] = session_id;
	arg["num_unread"] = ++(session->mNumUnread);
	arg["participant_unread"] = ++(session->mParticipantUnreadMessageCount);
	LLIMModel::getInstance()->mNewMsgSignal(arg);
}

// static
void LLHandlerUtil::updateIMFLoaterMesages(const LLUUID& session_id)
{
	LLFloaterIMSession* im_floater = LLFloaterIMSession::findInstance(session_id);
	if (im_floater != NULL && im_floater->getVisible())
	{
		im_floater->updateMessages();
	}
}

// static
void LLHandlerUtil::updateVisibleIMFLoaterMesages(const LLNotificationPtr& notification)
{
	const std::string name = LLHandlerUtil::getSubstitutionName(notification);
	LLUUID from_id = notification->getPayload()["from_id"];
	LLUUID session_id = spawnIMSession(name, from_id);

	updateIMFLoaterMesages(session_id);
}

// static
void LLHandlerUtil::decIMMesageCounter(const LLNotificationPtr& notification)
{
	const std::string name = LLHandlerUtil::getSubstitutionName(notification);
	LLUUID from_id = notification->getPayload()["from_id"];
	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, from_id);

	LLIMModel::LLIMSession * session = LLIMModel::getInstance()->findIMSession(session_id);

	if (session)
	{
	LLSD arg;
	arg["session_id"] = session_id;
	session->mNumUnread--;
	arg["num_unread"] = session->mNumUnread;
	session->mParticipantUnreadMessageCount--;
	arg["participant_unread"] = session->mParticipantUnreadMessageCount;
	LLIMModel::getInstance()->mNewMsgSignal(arg);
}
}

