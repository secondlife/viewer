/**
 * @file   llnotificationslistener.h
 * @author Brad Kittenbrink
 * @date   2009-07-08
 * @brief  Wrap subset of LLNotifications API in event API for test scripts.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#ifndef LL_LLNOTIFICATIONSLISTENER_H
#define LL_LLNOTIFICATIONSLISTENER_H

#include "lleventdispatcher.h"

class LLNotifications;
class LLSD;

class LLNotificationsListener : public LLDispatchListener
{
public:
    LLNotificationsListener(LLNotifications & notifications);

    void requestAdd(LLSD const & event_data) const;

private:
	void NotificationResponder(const std::string& replypump, 
							   const LLSD& notification, 
							   const LLSD& response);
	LLNotifications & mNotifications;
};

#endif // LL_LLNOTIFICATIONSLISTENER_H
