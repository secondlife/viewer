/**
 * @file llnotificationofferhandler.cpp
 * @brief Provides set of utility methods for notifications processing.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 *
 * Copyright (c) 2000-2009, Linden Research, Inc.
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


#include "llviewerprecompiledheaders.h" // must be first include

#include "llnotificationhandler.h"
#include "llnotifications.h"
#include "llimview.h"
#include "llagent.h"
#include "llfloaterreg.h"
#include "llnearbychat.h"

using namespace LLNotificationsUI;

const static std::string GRANTED_MODIFY_RIGHTS("GrantedModifyRights"),
		REVOKED_MODIFY_RIGHTS("RevokedModifyRights"), OBJECT_GIVE_ITEM(
				"ObjectGiveItem"), OBJECT_GIVE_ITEM_UNKNOWN_USER(
				"ObjectGiveItemUnknownUser"), PAYMENT_RECIVED("PaymentRecived"),
						ADD_FRIEND_WITH_MESSAGE("AddFriendWithMessage"),
						USER_GIVE_ITEM("UserGiveItem"),
						INVENTORY_ACCEPTED("InventoryAccepted"),
						INVENTORY_DECLINED("InventoryDeclined"),
						OFFER_FRIENDSHIP("OfferFriendship"),
						FRIENDSHIP_ACCEPTED("FriendshipAccepted"),
						FRIENDSHIP_OFFERED("FriendshipOffered"),
						FRIEND_ONLINE("FriendOnline"), FRIEND_OFFLINE("FriendOffline"),
						SERVER_OBJECT_MESSAGE("ServerObjectMessage"),
						TELEPORT_OFFERED("TeleportOffered");

// static
bool LLHandlerUtil::canLogToIM(const LLNotificationPtr& notification)
{
	return GRANTED_MODIFY_RIGHTS == notification->getName()
			|| REVOKED_MODIFY_RIGHTS == notification->getName()
			|| PAYMENT_RECIVED == notification->getName()
			|| OFFER_FRIENDSHIP == notification->getName()
			|| FRIENDSHIP_OFFERED == notification->getName()
			|| SERVER_OBJECT_MESSAGE == notification->getName()
			|| INVENTORY_ACCEPTED == notification->getName()
			|| INVENTORY_DECLINED == notification->getName();
}

// static
bool LLHandlerUtil::canLogToNearbyChat(const LLNotificationPtr& notification)
{
	return notification->getType() == "notifytip"
			&&  FRIEND_ONLINE != notification->getName()
			&& FRIEND_OFFLINE != notification->getName()
			&& INVENTORY_ACCEPTED != notification->getName()
			&& INVENTORY_DECLINED != notification->getName();
}

// static
bool LLHandlerUtil::canSpawnIMSession(const LLNotificationPtr& notification)
{
	return OFFER_FRIENDSHIP == notification->getName()
			|| FRIENDSHIP_ACCEPTED == notification->getName()
			|| USER_GIVE_ITEM == notification->getName()
			|| INVENTORY_ACCEPTED == notification->getName()
			|| INVENTORY_DECLINED == notification->getName();
}

// static
bool LLHandlerUtil::canSpawnSessionAndLogToIM(const LLNotificationPtr& notification)
{
	return canLogToIM(notification) && canSpawnIMSession(notification);
}

// static
void LLHandlerUtil::logToIM(const EInstantMessage& session_type,
		const std::string& session_name, const std::string& from_name,
		const std::string& message, const LLUUID& session_owner_id,
		const LLUUID& from_id)
{
	LLUUID session_id = LLIMMgr::computeSessionID(session_type,
			session_owner_id);
	LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(
			session_id);
	if (session == NULL)
	{
		LLIMModel::instance().logToFile(session_name, from_name, from_id, message);
	}
	else
	{
		// store active session id
		const LLUUID & active_session_id =
				LLIMModel::instance().getActiveSessionID();

		// set searched session as active to avoid IM toast popup
		LLIMModel::instance().setActiveSessionID(session_id);

		LLIMModel::instance().addMessage(session_id, from_name, from_id,
				message);

		// restore active session id
		if (active_session_id.isNull())
		{
			LLIMModel::instance().resetActiveSessionID();
		}
		else
		{
			LLIMModel::instance().setActiveSessionID(active_session_id);
		}
	}
}

// static
void LLHandlerUtil::logToIMP2P(const LLNotificationPtr& notification)
{
	const std::string name = LLHandlerUtil::getSubstitutionName(notification);

	const std::string session_name = notification->getPayload().has(
			"SESSION_NAME") ? notification->getPayload()["SESSION_NAME"].asString() : name;

	// don't create IM p2p session with objects, it's necessary condition to log
	if (notification->getName() != OBJECT_GIVE_ITEM && notification->getName()
			!= OBJECT_GIVE_ITEM_UNKNOWN_USER)
	{
		LLUUID from_id = notification->getPayload()["from_id"];

		logToIM(IM_NOTHING_SPECIAL, session_name, name, notification->getMessage(),
				from_id, from_id);
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
		llwarns
						<< "Group notice for unkown group: "
								<< payload["group_id"].asUUID() << llendl;
	}

	const std::string group_name = groupData.mName;
	const std::string sender_name = payload["sender_name"].asString();

	// we can't retrieve sender id from group notice system message, so try to lookup it from cache
	LLUUID sender_id;
	gCacheName->getUUID(sender_name, sender_id);

	logToIM(IM_SESSION_GROUP_START, group_name, sender_name, payload["message"],
			payload["group_id"], sender_id);
}

// static
void LLHandlerUtil::logToNearbyChat(const LLNotificationPtr& notification, EChatSourceType type)
{
	LLNearbyChat* nearby_chat = LLFloaterReg::getTypedInstance<LLNearbyChat>("nearby_chat", LLSD());
	if(nearby_chat)
	{
		LLChat chat_msg(notification->getMessage());
		chat_msg.mSourceType = type;
		nearby_chat->addMessage(chat_msg);
	}
}

// static
void LLHandlerUtil::spawnIMSession(const std::string& name, const LLUUID& from_id)
{
	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, from_id);

	LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(
			session_id);
	if (session == NULL)
	{
		LLIMMgr::instance().addSession(name, IM_NOTHING_SPECIAL, from_id);
	}
}

// static
std::string LLHandlerUtil::getSubstitutionName(const LLNotificationPtr& notification)
{
	return notification->getSubstitutions().has("NAME")
		? notification->getSubstitutions()["NAME"]
		: notification->getSubstitutions()["[NAME]"];
}
