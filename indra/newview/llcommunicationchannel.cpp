/** 
* @file llcommunicationchannel.cpp
* @brief Implementation of llcommunicationchannel
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

#include "llviewerprecompiledheaders.h" // must be first include

#include "llcommunicationchannel.h"

#include <string>
#include <map>

#include "llagent.h"
#include "lldate.h"
#include "llnotifications.h"


LLCommunicationChannel::LLCommunicationChannel(const std::string& pName, const std::string& pParentName)
    : LLNotificationChannel(pName, pParentName, filterByDoNotDisturbStatus)
    , mHistory()
{
}

LLCommunicationChannel::~LLCommunicationChannel()
{
}

bool LLCommunicationChannel::filterByDoNotDisturbStatus(LLNotificationPtr)
{
    return !gAgent.isDoNotDisturb();
}

S32 LLCommunicationChannel::getHistorySize() const
{
    return mHistory.size();
}

LLCommunicationChannel::history_list_t::const_iterator LLCommunicationChannel::beginHistory() const
{
    return mHistory.begin();
}

LLCommunicationChannel::history_list_t::const_iterator LLCommunicationChannel::endHistory() const
{
    return mHistory.end();
}

LLCommunicationChannel::history_list_t::iterator LLCommunicationChannel::beginHistory()
{
    return mHistory.begin();
}

LLCommunicationChannel::history_list_t::iterator LLCommunicationChannel::endHistory()
{
    return mHistory.end();
}

void LLCommunicationChannel::clearHistory()
{
    mHistory.clear();
}

void LLCommunicationChannel::removeItemFromHistory(LLNotificationPtr p)
{
    //Find the notification and removes it from mHistory
    for(history_list_t::iterator it = beginHistory(); it != endHistory(); ++it)
    {
        if(it->second == p)
        {
            mHistory.erase(it);
            break;
        }
    }
}

void LLCommunicationChannel::onDelete(LLNotificationPtr p) 
{
    removeItemFromHistory(p);
}

void LLCommunicationChannel::onFilterFail(LLNotificationPtr pNotificationPtr)
{
    std::string notificationType = pNotificationPtr->getType();
    if (((notificationType == "groupnotify")
        || (notificationType == "offer")
        || (notificationType == "notifytoast"))
        && !pNotificationPtr->isCancelled())
    {
        mHistory.insert(history_list_t::value_type(pNotificationPtr->getDate(), pNotificationPtr));
    }
}
