/** 
 * @file llexperiencelog.h
 * @brief llexperiencelog and related class definitions
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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



#ifndef LL_LLEXPERIENCELOG_H
#define LL_LLEXPERIENCELOG_H

#include "llsingleton.h"

class LLExperienceLog : public LLSingleton<LLExperienceLog>
{
    LLSINGLETON(LLExperienceLog);
public:
    typedef boost::signals2::signal<void(LLSD&)> 
        callback_signal_t;
    typedef callback_signal_t::slot_type callback_slot_t;
    typedef boost::signals2::connection callback_connection_t;
    callback_connection_t addUpdateSignal(const callback_slot_t& cb);

    void initialize();

    U32 getMaxDays() const { return mMaxDays; }
    void setMaxDays(U32 val);

    bool getNotifyNewEvent() const { return mNotifyNewEvent; }
    void setNotifyNewEvent(bool val);

    U32 getPageSize() const { return mPageSize; }
    void setPageSize(U32 val) { mPageSize = val; }

    const LLSD& getEvents()const;
    void clear();

    virtual ~LLExperienceLog();

    static void notify(LLSD& message);
    static std::string getFilename();
    static std::string getPermissionString(const LLSD& message, const std::string& base);
    void setEventsToSave(LLSD new_events){mEventsToSave = new_events; }
    bool isNotExpired(std::string& date);
    void handleExperienceMessage(LLSD& message);

protected:
    void loadEvents();
    void saveEvents();
    void eraseExpired();



    LLSD mEvents;
    LLSD mEventsToSave;
    callback_signal_t mSignals;
    callback_connection_t mNotifyConnection;
    U32 mMaxDays;
    U32 mPageSize;
    bool mNotifyNewEvent;

    friend class LLExperienceLogDispatchHandler;
};




#endif // LL_LLEXPERIENCELOG_H
