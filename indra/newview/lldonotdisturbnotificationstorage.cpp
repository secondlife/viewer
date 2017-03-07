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
#include "llfloaterreg.h"
#include "llimview.h"
#include "llnotifications.h"
#include "llnotificationhandler.h"
#include "llnotificationstorage.h"
#include "llscriptfloater.h"
#include "llsd.h"
#include "llsingleton.h"
#include "lluuid.h"

static const F32 DND_TIMER = 3.0;
const char * LLDoNotDisturbNotificationStorage::toastName = "IMToast";
const char * LLDoNotDisturbNotificationStorage::offerName = "UserGiveItem";

LLDoNotDisturbNotificationStorageTimer::LLDoNotDisturbNotificationStorageTimer() : LLEventTimer(DND_TIMER)
{

}

LLDoNotDisturbNotificationStorageTimer::~LLDoNotDisturbNotificationStorageTimer()
{

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
	: LLNotificationStorage("")
    , mDirty(false)
{
    nameToPayloadParameterMap[toastName] = "SESSION_ID";
    nameToPayloadParameterMap[offerName] = "object_id";
}

LLDoNotDisturbNotificationStorage::~LLDoNotDisturbNotificationStorage()
{
}

void LLDoNotDisturbNotificationStorage::initialize()
{
	setFileName(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "dnd_notifications.xml"));
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

static LLTrace::BlockTimerStatHandle FTM_SAVE_DND_NOTIFICATIONS("Save DND Notifications");

void LLDoNotDisturbNotificationStorage::saveNotifications()
{
	LL_RECORD_BLOCK_TIME(FTM_SAVE_DND_NOTIFICATIONS);

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

		if (!notificationPtr->isRespondedTo() && !notificationPtr->isCancelled() &&
			!notificationPtr->isExpired() && !notificationPtr->isPersistent())
		{
			data.append(notificationPtr->asLLSD(true));
		}
	}

	writeNotifications(output);

    resetDirty();
}

static LLTrace::BlockTimerStatHandle FTM_LOAD_DND_NOTIFICATIONS("Load DND Notifications");

void LLDoNotDisturbNotificationStorage::loadNotifications()
{
	LL_RECORD_BLOCK_TIME(FTM_LOAD_DND_NOTIFICATIONS);
	
	LL_INFOS("LLDoNotDisturbNotificationStorage") << "start loading notifications" << LL_ENDL;

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
	bool group_ad_hoc_toast_exists = false;
	S32 toastSessionType;
    bool offerExists = false;
	
	for (LLSD::array_const_iterator notification_it = data.beginArray();
		 notification_it != data.endArray();
		 ++notification_it)
	{
		LLSD notification_params = *notification_it;
        const LLUUID& notificationID = notification_params["id"];
        std::string notificationName = notification_params["name"];
        LLNotificationPtr notification = instance.find(notificationID);

        if(notificationName == toastName)
        {
			toastSessionType = notification_params["payload"]["SESSION_TYPE"];
			if(toastSessionType == LLIMModel::LLIMSession::P2P_SESSION)
			{
				imToastExists = true;
			}
			//Don't add group/ad-hoc messages to the notification system because
			//this means the group/ad-hoc session has to be re-created
			else if(toastSessionType == LLIMModel::LLIMSession::GROUP_SESSION 
					|| toastSessionType == LLIMModel::LLIMSession::ADHOC_SESSION)
			{
				//Just allows opening the conversation log for group/ad-hoc messages upon startup
				group_ad_hoc_toast_exists = true;
				continue;
			}
        }
        else if(notificationName == offerName)
        {
            offerExists = true;
        }
		
		//Notification already exists due to persistent storage adding it first into the notification system
		if(notification)
		{
			notification->setDND(true);
			instance.update(instance.find(notificationID));
		}
		//New notification needs to be added
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

    bool isConversationLoggingAllowed = gSavedPerAccountSettings.getS32("KeepConversationLogTranscripts") > 0;
	if(group_ad_hoc_toast_exists && isConversationLoggingAllowed)
	{
		LLFloaterReg::showInstance("conversation");
	}

    if(imToastExists || group_ad_hoc_toast_exists || offerExists)
    {
		make_ui_sound_deferred("UISndNewIncomingIMSession");
    }

    //writes out empty .xml file (since LLCommunicationChannel::mHistory is empty)
	saveNotifications();

	LL_INFOS("LLDoNotDisturbNotificationStorage") << "finished loading notifications" << LL_ENDL;
}

void LLDoNotDisturbNotificationStorage::updateNotifications()
{

	LLNotificationChannelPtr channelPtr = getCommunicationChannel();
	LLCommunicationChannel *commChannel = dynamic_cast<LLCommunicationChannel*>(channelPtr.get());
	llassert(commChannel != NULL);

    LLNotifications& instance = LLNotifications::instance();
    bool imToastExists = false;
    bool offerExists = false;
  
    for (LLCommunicationChannel::history_list_t::const_iterator it = commChannel->beginHistory();
        it != commChannel->endHistory();
        ++it)
    {
        LLNotificationPtr notification = it->second;
        std::string notificationName = notification->getName();

        if(notificationName == toastName)
        {
            imToastExists = true;
        }
        else if(notificationName == offerName)
        {
            offerExists = true;
        }

        //Notification already exists in notification pipeline (same instance of app running)
        if (notification)
        {
            notification->setDND(true);
            instance.update(notification);
        }
    }

    if(imToastExists || offerExists)
    {
        make_ui_sound("UISndNewIncomingIMSession");
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

void LLDoNotDisturbNotificationStorage::removeNotification(const char * name, const LLUUID& id)
{
    LLNotifications& instance = LLNotifications::instance();
    LLNotificationChannelPtr channelPtr = getCommunicationChannel();
    LLCommunicationChannel *commChannel = dynamic_cast<LLCommunicationChannel*>(channelPtr.get());
    LLNotificationPtr notification;
    LLSD payload;
    LLUUID notificationObjectID;
    std::string notificationName;
    std::string payloadVariable = nameToPayloadParameterMap[name];
    LLCommunicationChannel::history_list_t::iterator it;
    std::vector<LLCommunicationChannel::history_list_t::iterator> itemsToRemove;

    //Find notification with the matching session id
    for (it = commChannel->beginHistory();
        it != commChannel->endHistory(); 
        ++it)
    {
        notification = it->second;
        payload = notification->getPayload();
        notificationObjectID = payload[payloadVariable].asUUID();
        notificationName = notification->getName();

        if((notificationName == name)
            && id == notificationObjectID)
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
