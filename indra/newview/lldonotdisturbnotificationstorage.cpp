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
#include "llfloaterreg.h"
#include "llnotifications.h"
#include "llnotificationhandler.h"
#include "llnotificationstorage.h"
#include "llscriptfloater.h"
#include "llsd.h"
#include "llsingleton.h"
#include "lluuid.h"

static const F32 DND_TIMER = 3.0;

LLDoNotDisturbNotificationStorageTimer::LLDoNotDisturbNotificationStorageTimer() : LLEventTimer(DND_TIMER)
{
    mEventTimer.start();
}

LLDoNotDisturbNotificationStorageTimer::~LLDoNotDisturbNotificationStorageTimer()
{
    mEventTimer.stop();
}

BOOL LLDoNotDisturbNotificationStorageTimer::tick()
{
    LLDoNotDisturbNotificationStorage * doNotDisturbNotificationStorage =  LLDoNotDisturbNotificationStorage::getInstance();

    if(doNotDisturbNotificationStorage
        && doNotDisturbNotificationStorage->getDirty())
    {
        doNotDisturbNotificationStorage->saveNotifications();
    }
    return FALSE;
}

LLDoNotDisturbNotificationStorage::LLDoNotDisturbNotificationStorage()
	: LLSingleton<LLDoNotDisturbNotificationStorage>()
	, LLNotificationStorage(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "dnd_notifications.xml"))
    , mDirty(false)
{
}

LLDoNotDisturbNotificationStorage::~LLDoNotDisturbNotificationStorage()
{
}

void LLDoNotDisturbNotificationStorage::initialize()
{
	getCommunicationChannel()->connectFailedFilter(boost::bind(&LLDoNotDisturbNotificationStorage::onChannelChanged, this, _1));
}

bool LLDoNotDisturbNotificationStorage::getDirty()
{
    return mDirty;
}

void LLDoNotDisturbNotificationStorage::resetDirty()
{
    mDirty = false;
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

    resetDirty();
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
    bool imToastExists = false;
	
	for (LLSD::array_const_iterator notification_it = data.beginArray();
		 notification_it != data.endArray();
		 ++notification_it)
	{
		LLSD notification_params = *notification_it;
        const LLUUID& notificationID = notification_params["id"];
        std::string notificationName = notification_params["name"];
        LLNotificationPtr notification = instance.find(notificationID);

        if(notificationName == "IMToast")
        {
            imToastExists = true;
        }

        //New notification needs to be added
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

    if(imToastExists)
    {
        LLFloaterReg::showInstance("im_container");
    }

    //writes out empty .xml file (since LLCommunicationChannel::mHistory is empty)
	saveNotifications();
}

void LLDoNotDisturbNotificationStorage::updateNotifications()
{

	LLNotificationChannelPtr channelPtr = getCommunicationChannel();
	LLCommunicationChannel *commChannel = dynamic_cast<LLCommunicationChannel*>(channelPtr.get());
	llassert(commChannel != NULL);

    LLNotifications& instance = LLNotifications::instance();
    bool imToastExists = false;
  
    for (LLCommunicationChannel::history_list_t::const_iterator it = commChannel->beginHistory();
        it != commChannel->endHistory();
        ++it)
    {
        LLNotificationPtr notification = it->second;
        std::string notificationName = notification->getName();

        if(notificationName == "IMToast")
        {
            imToastExists = true;
        }

        //Notification already exists in notification pipeline (same instance of app running)
        if (notification)
        {
            notification->setDND(true);
            instance.update(notification);
        }
    }

    if(imToastExists)
    {   
        LLFloaterReg::showInstance("im_container");
    }

    //When exit DND mode, write empty notifications file
    if(commChannel->getHistorySize())
    {
	commChannel->clearHistory();
	saveNotifications();
}
}

LLNotificationChannelPtr LLDoNotDisturbNotificationStorage::getCommunicationChannel() const
{
	LLNotificationChannelPtr channelPtr = LLNotifications::getInstance()->getChannel("Communication");
	llassert(channelPtr);
	return channelPtr;
}

void LLDoNotDisturbNotificationStorage::removeIMNotification(const LLUUID& session_id)
{
    LLNotifications& instance = LLNotifications::instance();
    LLNotificationChannelPtr channelPtr = getCommunicationChannel();
    LLCommunicationChannel *commChannel = dynamic_cast<LLCommunicationChannel*>(channelPtr.get());
    LLNotificationPtr notification;
    LLSD substitutions;
    LLUUID notificationSessionID;
    LLCommunicationChannel::history_list_t::iterator it;
    std::vector<LLCommunicationChannel::history_list_t::iterator> itemsToRemove;

    //Find notification with the matching session id
    for (it = commChannel->beginHistory();
        it != commChannel->endHistory(); 
        ++it)
    {
        notification = it->second;
        substitutions = notification->getSubstitutions();
        notificationSessionID = substitutions["SESSION_ID"].asUUID();

        if(session_id == notificationSessionID)
        {
            itemsToRemove.push_back(it);
        }
    }

   
    //Remove the notifications
    if(itemsToRemove.size())
    {
    while(itemsToRemove.size())
    {
        it = itemsToRemove.back();
        notification = it->second;
            commChannel->removeItemFromHistory(notification);
        instance.cancel(notification);
        itemsToRemove.pop_back();
    }
        //Trigger saving of notifications to xml once all have been removed
        saveNotifications();
    }
}


bool LLDoNotDisturbNotificationStorage::onChannelChanged(const LLSD& pPayload)
{
	if (pPayload["sigtype"].asString() != "load")
	{
        mDirty = true;
	}

	return false;
}
