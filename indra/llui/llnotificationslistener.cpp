/**
 * @file   llnotificationslistener.cpp
 * @author Brad Kittenbrink
 * @date   2009-07-08
 * @brief  Implementation for llnotificationslistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llnotificationslistener.h"

#include "llnotifications.h"

LLNotificationsListener::LLNotificationsListener(LLNotifications & notifications) :
    LLDispatchListener("LLNotifications", "op"),
    mNotifications(notifications)
{
    add("requestAdd", &LLNotificationsListener::requestAdd);
}

void LLNotificationsListener::requestAdd(const LLSD& event_data) const
{
	if(event_data.has("reply"))
	{
		mNotifications.add(event_data["name"], 
						   event_data["substitutions"], 
						   event_data["payload"],
						   boost::bind(&LLNotificationsListener::NotificationResponder, 
									   this, 
									   event_data["reply"].asString(), 
									   _1, _2
									   )
						   );
	}
	else
	{
		mNotifications.add(event_data["name"], 
						   event_data["substitutions"], 
						   event_data["payload"]);
	}
}

void LLNotificationsListener::NotificationResponder(const std::string& reply_pump, 
										const LLSD& notification, 
										const LLSD& response) const
{
	LLSD reponse_event;
	reponse_event["notification"] = notification;
	reponse_event["response"] = response;
	LLEventPumps::getInstance()->obtain(reply_pump).post(reponse_event);
}
