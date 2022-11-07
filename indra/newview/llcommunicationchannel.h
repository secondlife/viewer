/** 
* @file   llcommunicationchannel.h
* @brief  Header file for llcommunicationchannel
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
#ifndef LL_LLCOMMUNICATIONCHANNEL_H
#define LL_LLCOMMUNICATIONCHANNEL_H

#include <string>
#include <map>

#include "lldate.h"
#include "llerror.h"
#include "llnotifications.h"

class LLCommunicationChannel : public LLNotificationChannel
{
    LOG_CLASS(LLCommunicationChannel);
public:
    LLCommunicationChannel(const std::string& pName, const std::string& pParentName);
    virtual ~LLCommunicationChannel();

    static bool filterByDoNotDisturbStatus(LLNotificationPtr);

    typedef std::multimap<LLDate, LLNotificationPtr> history_list_t;
    S32 getHistorySize() const; 
    history_list_t::const_iterator beginHistory() const;
    history_list_t::const_iterator endHistory() const;
    history_list_t::iterator beginHistory();
    history_list_t::iterator endHistory();  

    void clearHistory();
    void removeItemFromHistory(LLNotificationPtr p);

protected:
    virtual void onDelete(LLNotificationPtr p);
    virtual void onFilterFail(LLNotificationPtr pNotificationPtr);

private:

    history_list_t mHistory;
};

#endif // LL_LLCOMMUNICATIONCHANNEL_H

