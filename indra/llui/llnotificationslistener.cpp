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
    mNotifications.add(event_data["name"], event_data["substitutions"], event_data["payload"]);
}
