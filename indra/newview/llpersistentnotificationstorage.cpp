/** 
* @file llpersistentnotificationstorage.cpp
* @brief Implementation of llpersistentnotificationstorage
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#include "llpersistentnotificationstorage.h"

#include "llchannelmanager.h"
#include "llnotificationstorage.h"
#include "llscreenchannel.h"
#include "llscriptfloater.h"
#include "llviewermessage.h"
#include "llviewernetwork.h"
LLPersistentNotificationStorage::LLPersistentNotificationStorage():
	  LLNotificationStorage("")
	, mLoaded(false)
{
}

LLPersistentNotificationStorage::~LLPersistentNotificationStorage()
{
}

static LLTrace::BlockTimerStatHandle FTM_SAVE_NOTIFICATIONS("Save Notifications");

void LLPersistentNotificationStorage::saveNotifications()
{
	LL_RECORD_BLOCK_TIME(FTM_SAVE_NOTIFICATIONS);

	boost::intrusive_ptr<LLPersistentNotificationChannel> history_channel = boost::dynamic_pointer_cast<LLPersistentNotificationChannel>(LLNotifications::instance().getChannel("Persistent"));
	if (!history_channel)
	{
		return;
	}

	LLSD output = LLSD::emptyMap();
	LLSD& data = output["data"];

	for ( std::vector<LLNotificationPtr>::iterator it = history_channel->beginHistory(), end_it = history_channel->endHistory();
		it != end_it;
		++it)
	{
		LLNotificationPtr notification = *it;

		// After a notification was placed in Persist channel, it can become
		// responded, expired or canceled - in this case we are should not save it
		if(notification->isRespondedTo() || notification->isCancelled()
			|| notification->isExpired())
		{
			continue;
		}

		data.append(notification->asLLSD(true));
		if (data.size() >= gSavedSettings.getS32("MaxPersistentNotifications"))
		{
			LL_WARNS() << "Too many persistent notifications."
					<< " Saved " << gSavedSettings.getS32("MaxPersistentNotifications") << " of " << history_channel->size()
					<< " persistent notifications." << LL_ENDL;
			break;
		}

	}

	writeNotifications(output);
}

static LLTrace::BlockTimerStatHandle FTM_LOAD_NOTIFICATIONS("Load Notifications");

void LLPersistentNotificationStorage::loadNotifications()
{
	LL_RECORD_BLOCK_TIME(FTM_LOAD_NOTIFICATIONS);

	LL_INFOS("LLPersistentNotificationStorage") << "start loading notifications" << LL_ENDL;

	if (mLoaded)
	{
		LL_INFOS("LLPersistentNotificationStorage") << "notifications already loaded, exiting" << LL_ENDL;
		return;
	}

	mLoaded = true;
	LLSD input;
	if (!readNotifications(input) ||input.isUndefined())
	{
		return;
	}

	LLSD& data = input["data"];
	if (data.isUndefined())
	{
		return;
	}

	using namespace LLNotificationsUI;
	LLScreenChannel* notification_channel = dynamic_cast<LLScreenChannel*>(LLChannelManager::getInstance()->
		findChannelByID(LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));

	LLNotifications& instance = LLNotifications::instance();
	S32 processed_notifications = 0;
	std::vector<LLSD> notifications_array;
	for (LLSD::reverse_array_iterator notification_it = data.rbeginArray();
		notification_it != data.rendArray();
		++notification_it)
	{
		LLSD notification_params = *notification_it;
		notifications_array.push_back(notification_params);

		++processed_notifications;
		if (processed_notifications >= gSavedSettings.getS32("MaxPersistentNotifications"))
		{
			LL_WARNS() << "Too many persistent notifications."
					<< " Processed " << gSavedSettings.getS32("MaxPersistentNotifications") << " of " << data.size() << " persistent notifications." << LL_ENDL;
		    break;
		}
	}

	for (LLSD::reverse_array_iterator notification_it = notifications_array.rbegin();
			notification_it != notifications_array.rend();
			++notification_it)
	{
		LLSD notification_params = *notification_it;
		LLNotificationPtr notification(new LLNotification(notification_params));

		LLNotificationResponderPtr responder(createResponder(notification_params["name"], notification_params["responder"]));
		notification->setResponseFunctor(responder);

		instance.add(notification);

		// hide script floaters so they don't confuse the user and don't overlap startup toast
		LLScriptFloaterManager::getInstance()->setFloaterVisible(notification->getID(), false);

		if(notification_channel)
		{
			// hide saved toasts so they don't confuse the user
			notification_channel->hideToast(notification->getID());
		}
	}

	LLNotifications::instance().getChannel("Persistent")->
			connectChanged(boost::bind(&LLPersistentNotificationStorage::onPersistentChannelChanged, this, _1));
	LL_INFOS("LLPersistentNotificationStorage") << "finished loading notifications" << LL_ENDL;
}

void LLPersistentNotificationStorage::initialize()
{
	std::string file_name = "open_notifications_" + LLGridManager::getInstance()->getGrid() + ".xml";
	setFileName(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, file_name));
	setOldFileName(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "open_notifications.xml"));

	LLNotifications::instance().getChannel("Persistent")->
		connectChanged(boost::bind(&LLPersistentNotificationStorage::onPersistentChannelChanged, this, _1));
}

bool LLPersistentNotificationStorage::onPersistentChannelChanged(const LLSD& payload)
{
	// In case we received channel changed signal but haven't yet loaded notifications, do it
	if (!mLoaded)
	{
		loadNotifications();
	}
	// we ignore "load" messages, but rewrite the persistence file on any other
	const std::string sigtype = payload["sigtype"].asString();
	if ("load" != sigtype)
	{
		saveNotifications();
	}
	return false;
}

// EOF
