/** 
* @file lldonotdisturbnotificationstorage.cpp
* @brief Implementation of lldonotdisturbnotificationstorage
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

#include "lldonotdisturbnotificationstorage.h"

#include "llcommunicationchannel.h"
#include "lldir.h"
#include "llerror.h"
#include "llfasttimer_class.h"
#include "llnotifications.h"
#include "llnotificationhandler.h"
#include "llnotificationstorage.h"
#include "llscriptfloater.h"
#include "llsd.h"
#include "llsingleton.h"
#include "lluuid.h"

LLDoNotDisturbNotificationStorage::LLDoNotDisturbNotificationStorage()
	: LLSingleton<LLDoNotDisturbNotificationStorage>()
	, LLNotificationStorage(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "dnd_notifications.xml"))
{
}

LLDoNotDisturbNotificationStorage::~LLDoNotDisturbNotificationStorage()
{
}

void LLDoNotDisturbNotificationStorage::initialize()
{
	getCommunicationChannel()->connectFailedFilter(boost::bind(&LLDoNotDisturbNotificationStorage::onChannelChanged, this, _1));
}

static LLFastTimer::DeclareTimer FTM_SAVE_DND_NOTIFICATIONS("Save DND Notifications");

void LLDoNotDisturbNotificationStorage::saveNotifications()
{
	LLFastTimer _(FTM_SAVE_DND_NOTIFICATIONS);

	LLNotificationChannelPtr channelPtr = getCommunicationChannel();
	const LLCommunicationChannel *commChannel = dynamic_cast<LLCommunicationChannel*>(channelPtr.get());
	llassert(commChannel != NULL);

	LLSD output = LLSD::emptyMap();
	LLSD& data = output["data"];
	data = LLSD::emptyArray();

	for (LLCommunicationChannel::history_list_t::const_iterator historyIter = commChannel->beginHistory();
		historyIter != commChannel->endHistory(); ++historyIter)
	{
		LLNotificationPtr notificationPtr = historyIter->second;

		if (!notificationPtr->isRespondedTo() && !notificationPtr->isCancelled() && !notificationPtr->isExpired())
		{
			data.append(notificationPtr->asLLSD(true));
		}
	}

	writeNotifications(output);
}

static LLFastTimer::DeclareTimer FTM_LOAD_DND_NOTIFICATIONS("Load DND Notifications");

void LLDoNotDisturbNotificationStorage::loadNotifications()
{
	LLFastTimer _(FTM_LOAD_DND_NOTIFICATIONS);
	
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
	
	LLNotifications& instance = LLNotifications::instance();
	
	for (LLSD::array_const_iterator notification_it = data.beginArray();
		 notification_it != data.endArray();
		 ++notification_it)
	{
		LLSD notification_params = *notification_it;
        const LLUUID& notificationID = notification_params["id"];
        LLNotificationPtr notification = instance.find(notificationID);
		
        //Notification already exists in notification pipeline (same instance of app running)
		if (notification)
		{
            notification->setDND(true);
			instance.update(notification);
		}
        //Notification doesn't exist (different instance since restarted app while in DND mode)
		else
		{
            notification = (LLNotificationPtr) new LLNotification(notification_params.with("is_dnd", true));
			LLNotificationResponderInterface* responder = createResponder(notification_params["responder_sd"]["responder_type"], notification_params["responder_sd"]);
			if (responder == NULL)
			{
				LL_WARNS("LLDoNotDisturbNotificationStorage") << "cannot create responder for notification of type '"
					<< notification->getType() << "'" << LL_ENDL;
			}
			else
			{
				LLNotificationResponderPtr responderPtr(responder);
				notification->setResponseFunctor(responderPtr);
			}
			
			instance.add(notification);
		}
	}

	// Clear the communication channel history and rewrite the save file to empty it as well
	LLNotificationChannelPtr channelPtr = getCommunicationChannel();
	LLCommunicationChannel *commChannel = dynamic_cast<LLCommunicationChannel*>(channelPtr.get());
	llassert(commChannel != NULL);
	commChannel->clearHistory();
	
	saveNotifications();
}

LLNotificationChannelPtr LLDoNotDisturbNotificationStorage::getCommunicationChannel() const
{
	LLNotificationChannelPtr channelPtr = LLNotifications::getInstance()->getChannel("Communication");
	llassert(channelPtr);
	return channelPtr;
}


bool LLDoNotDisturbNotificationStorage::onChannelChanged(const LLSD& pPayload)
{
	if (pPayload["sigtype"].asString() != "load")
	{
		saveNotifications();
	}

	return false;
}
