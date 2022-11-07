/**
 * @file llnotificationsutil.h
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
#ifndef LLNOTIFICATIONSUTIL_H
#define LLNOTIFICATIONSUTIL_H

// The vast majority of clients of the notifications system just want to add 
// a notification to the screen, so define this lightweight public interface
// to avoid including the heavyweight llnotifications.h

#include "llnotificationptr.h"
#include "lluuid.h"

#include <boost/function.hpp>

class LLSD;

namespace LLNotificationsUtil
{
    LLNotificationPtr add(const std::string& name);
    
    LLNotificationPtr add(const std::string& name, 
                          const LLSD& substitutions);
    
    LLNotificationPtr add(const std::string& name, 
                          const LLSD& substitutions, 
                          const LLSD& payload);
    
    LLNotificationPtr add(const std::string& name, 
                          const LLSD& substitutions, 
                          const LLSD& payload, 
                          const std::string& functor_name);

    LLNotificationPtr add(const std::string& name, 
                          const LLSD& substitutions, 
                          const LLSD& payload, 
                          boost::function<void (const LLSD&, const LLSD&)> functor);
    
    S32 getSelectedOption(const LLSD& notification, const LLSD& response);

    void cancel(LLNotificationPtr pNotif);

    LLNotificationPtr find(LLUUID uuid);
}

#endif
