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

#include "lleventapi.h"
#include "llnotificationptr.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>

class LLNotifications;
class LLSD;

class LLNotificationsListener : public LLEventAPI
{
public:
    LLNotificationsListener(LLNotifications & notifications);
    ~LLNotificationsListener();

private:
    void requestAdd(LLSD const & event_data) const;

	void NotificationResponder(const std::string& replypump, 
							   const LLSD& notification, 
							   const LLSD& response) const;

    void listChannels(const LLSD& params) const;
    void listChannelNotifications(const LLSD& params) const;
    void respond(const LLSD& params) const;
    void cancel(const LLSD& params) const;
    void ignore(const LLSD& params) const;
    void forward(const LLSD& params);

    static LLSD asLLSD(LLNotificationPtr);

    class Forwarder;
    typedef std::map<std::string, boost::shared_ptr<Forwarder> > ForwarderMap;
    ForwarderMap mForwarders;
	LLNotifications & mNotifications;
};

#endif // LL_LLNOTIFICATIONSLISTENER_H
