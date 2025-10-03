/**
 * @file llexperiencelog.cpp
 * @brief llexperiencelog implementation
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

#include "llviewerprecompiledheaders.h"
#include "llexperiencelog.h"

#include "lldispatcher.h"
#include "llsdserialize.h"
#include "llviewergenericmessage.h"
#include "llnotificationsutil.h"
#include "lltrans.h"
#include "llerror.h"
#include "lldate.h"


class LLExperienceLogDispatchHandler : public LLDispatchHandler
{
public:
    virtual bool operator()(
        const LLDispatcher* dispatcher,
        const std::string& key,
        const LLUUID& invoice,
        const sparam_t& strings)
    {
        LLSD message;

        sparam_t::const_iterator it = strings.begin();
        if(it != strings.end()){
            const std::string& llsdRaw = *it++;
            std::istringstream llsdData(llsdRaw);
            if (!LLSDSerialize::deserialize(message, llsdData, llsdRaw.length()))
            {
                LL_WARNS() << "LLExperienceLogDispatchHandler: Attempted to read parameter data into LLSD but failed:" << llsdRaw << LL_ENDL;
            }
        }
        message["public_id"] = invoice;

        // Object Name
        if(it != strings.end())
        {
            message["ObjectName"] = *it++;
        }

        // parcel Name
        if(it != strings.end())
        {
            message["ParcelName"] = *it++;
        }
        message["Count"] = 1;

        LLExperienceLog::instance().handleExperienceMessage(message);
        return true;
    }
};

static LLExperienceLogDispatchHandler experience_log_dispatch_handler;

void LLExperienceLog::handleExperienceMessage(LLSD& message)
{
    time_t now;
    time(&now);
    char daybuf[16];/* Flawfinder: ignore */
    char time_of_day[16];/* Flawfinder: ignore */
    strftime(daybuf, 16, "%Y-%m-%d", localtime(&now));
    strftime(time_of_day, 16, " %H:%M:%S", localtime(&now));
    message["Time"] = time_of_day;

    std::string day = daybuf;

    if(!mEvents.has(day))
    {
        mEvents[day] = LLSD::emptyArray();
    }
    LLSD& dayEvents = mEvents[day];
    if(dayEvents.size() > 0)
    {
        LLSD& last = *(dayEvents.rbeginArray());
        if( last["public_id"].asUUID() == message["public_id"].asUUID()
            && last["ObjectName"].asString() == message["ObjectName"].asString()
            && last["OwnerID"].asUUID() == message["OwnerID"].asUUID()
            && last["ParcelName"].asString() == message["ParcelName"].asString()
            && last["Permission"].asInteger() == message["Permission"].asInteger())
        {
            last["Count"] = last["Count"].asInteger() + 1;
            last["Time"] = time_of_day;
            mSignals(last);
            return;
        }
    }
    message["Time"] = time_of_day;
    mEvents[day].append(message);
    mEventsToSave[day].append(message);
    mSignals(message);
}

LLExperienceLog::LLExperienceLog()
    : mMaxDays(7)
    , mPageSize(25)
    , mNotifyNewEvent(false)
{
}

void LLExperienceLog::initialize()
{
    loadEvents();
    if(!gGenericDispatcher.isHandlerPresent("ExperienceEvent"))
    {
        gGenericDispatcher.addHandler("ExperienceEvent", &experience_log_dispatch_handler);
    }
}

std::string LLExperienceLog::getFilename()
{
    return gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "experience_events.xml");
}


std::string LLExperienceLog::getPermissionString( const LLSD& message, const std::string& base )
{
    std::ostringstream buf;
    if(message.has("Permission"))
    {
        buf << base << message["Permission"].asInteger();
        std::string entry;
        if(LLTrans::findString(entry, buf.str()))
        {
            buf.str(entry);
        }
    }

    if(buf.str().empty())
    {
        buf << base << "Unknown";

        buf.str(LLTrans::getString(buf.str(), message));
    }

    return buf.str();
}

void LLExperienceLog::notify( LLSD& message )
{
    message["EventType"] = getPermissionString(message, "ExperiencePermission");
    if(message.has("IsAttachment") && message["IsAttachment"].asBoolean())
    {
        LLNotificationsUtil::add("ExperienceEventAttachment", message);
    }
    else
    {
        LLNotificationsUtil::add("ExperienceEvent", message);
    }
    message.erase("EventType");
}

void LLExperienceLog::saveEvents()
{
    std::string filename = getFilename();
    LLSD settings = LLSD::emptyMap().with("Events", mEventsToSave);

    settings["MaxDays"] = (int)mMaxDays;
    settings["Notify"] = mNotifyNewEvent;
    settings["PageSize"] = (int)mPageSize;

    llofstream stream(filename.c_str());
    LLSDSerialize::toPrettyXML(settings, stream);
}


void LLExperienceLog::loadEvents()
{
    LLSD settings = LLSD::emptyMap();

    std::string filename = getFilename();
    llifstream stream(filename.c_str());
    LLSDSerialize::fromXMLDocument(settings, stream);

    if(settings.has("MaxDays"))
    {
        setMaxDays((U32)settings["MaxDays"].asInteger());
    }
    if(settings.has("Notify"))
    {
        setNotifyNewEvent(settings["Notify"].asBoolean());
    }
    if(settings.has("PageSize"))
    {
        setPageSize((U32)settings["PageSize"].asInteger());
    }
    mEvents.clear();
    if(mMaxDays > 0 && settings.has("Events"))
    {
        mEvents = settings["Events"];
        mEventsToSave = mEvents;
    }
}

LLExperienceLog::~LLExperienceLog()
{
    saveEvents();
}

void LLExperienceLog::eraseExpired()
{
    while(mEvents.size() > mMaxDays && mMaxDays > 0)
    {
        mEvents.erase(mEvents.beginMap()->first);
    }
}

bool LLExperienceLog::isNotExpired(std::string& date)
{
    LLDate event_date;
    S32 month, day, year;
    S32 matched = sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day);
    if (matched != 3) return false;
    event_date.fromYMDHMS(year, month, day);
    const U32 seconds_in_day = 24 * 60 * 60;
    S32 curr_year = 0, curr_month = 0, curr_day = 0;


    LLDate curr_date = LLDate::now();
    curr_date.split(&curr_year, &curr_month, &curr_day);
    curr_date.fromYMDHMS(curr_year, curr_month, curr_day); // Set hour, min, and sec to 0

    LLDate boundary_date =  LLDate(curr_date.secondsSinceEpoch() - seconds_in_day*getMaxDays());
    return event_date >= boundary_date;

}

const LLSD& LLExperienceLog::getEvents() const
{
    return mEvents;
}

void LLExperienceLog::clear()
{
    mEvents.clear();
}

void LLExperienceLog::setMaxDays( U32 val )
{
    mMaxDays = val;
}

LLExperienceLog::callback_connection_t LLExperienceLog::addUpdateSignal( const callback_slot_t& cb )
{
    return mSignals.connect(cb);
}

void LLExperienceLog::setNotifyNewEvent( bool val )
{
    mNotifyNewEvent = val;
    if(!val && mNotifyConnection.connected())
    {
        mNotifyConnection.disconnect();
    }
    else if( val && !mNotifyConnection.connected())
    {
        mNotifyConnection = addUpdateSignal(boost::function<void(LLSD&)>(LLExperienceLog::notify));
    }
}
