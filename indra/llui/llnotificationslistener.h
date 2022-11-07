/**
 * @file   llnotificationslistener.h
 * @author Brad Kittenbrink
 * @date   2009-07-08
 * @brief  Wrap subset of LLNotifications API in event API for test scripts.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLNOTIFICATIONSLISTENER_H
#define LL_LLNOTIFICATIONSLISTENER_H

#include "lleventapi.h"
#include "llnotificationptr.h"
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
    typedef std::map<std::string, std::shared_ptr<Forwarder> > ForwarderMap;
    ForwarderMap mForwarders;
    LLNotifications & mNotifications;
};

#endif // LL_LLNOTIFICATIONSLISTENER_H
