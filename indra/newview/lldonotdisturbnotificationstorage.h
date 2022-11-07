/** 
* @file   lldonotdisturbnotificationstorage.h
* @brief  Header file for lldonotdisturbnotificationstorage
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
#ifndef LL_LLDONOTDISTURBNOTIFICATIONSTORAGE_H
#define LL_LLDONOTDISTURBNOTIFICATIONSTORAGE_H

#include "llerror.h"
#include "lleventtimer.h"
#include "llnotifications.h"
#include "llnotificationstorage.h"
#include "llsingleton.h"

class LLSD;

class LLDoNotDisturbNotificationStorageTimer : public LLEventTimer
{
public:
    LLDoNotDisturbNotificationStorageTimer();
    ~LLDoNotDisturbNotificationStorageTimer();

public:
    BOOL tick();
};

class LLDoNotDisturbNotificationStorage : public LLParamSingleton<LLDoNotDisturbNotificationStorage>, public LLNotificationStorage
{
    LLSINGLETON(LLDoNotDisturbNotificationStorage);
    ~LLDoNotDisturbNotificationStorage();

    LOG_CLASS(LLDoNotDisturbNotificationStorage);
public:
    static const char * toastName;
    static const char * offerName;

    bool getDirty();
    void resetDirty();
    void saveNotifications();
    void loadNotifications();
    void updateNotifications();
    void removeNotification(const char * name, const LLUUID& id);
    void reset();

protected:

private:
    void initialize();

    bool mDirty;
    LLDoNotDisturbNotificationStorageTimer mTimer;

    LLNotificationChannelPtr getCommunicationChannel() const;
    bool                     onChannelChanged(const LLSD& pPayload);
    std::map<std::string, std::string> nameToPayloadParameterMap;
};

#endif // LL_LLDONOTDISTURBNOTIFICATIONSTORAGE_H

