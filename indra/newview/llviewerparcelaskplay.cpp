/**
 * @file llviewerparcelaskplay.cpp
 * @brief stores data about parcel media user wants to auto-play and shows related notifications
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#include "llviewerparcelaskplay.h"

#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llstartup.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llagent.h"
#include "llsdserialize.h"

#include <boost/lexical_cast.hpp>


// class LLViewerParcelAskPlay

LLViewerParcelAskPlay::LLViewerParcelAskPlay() :
pNotification(NULL)
{
}

LLViewerParcelAskPlay::~LLViewerParcelAskPlay()
{

}

void LLViewerParcelAskPlay::initSingleton()
{

}
void LLViewerParcelAskPlay::cleanupSingleton()
{
    cancelNotification();
}

void LLViewerParcelAskPlay::askToPlay(const LLUUID &region_id, const S32 &parcel_id, const std::string &url, ask_callback cb)
{
    EAskPlayMode mode = getPlayMode(region_id, parcel_id);

    switch (mode)
    {
    case ASK_PLAY_IGNORE:
        cb(region_id, parcel_id, url, false);
        break;
    case ASK_PLAY_PLAY:
        cb(region_id, parcel_id, url, true);
        break;
    case ASK_PLAY_ASK:
    default:
        {
            // create or re-create notification
            // Note: will create and immediately cancel one notification if region has both media and music
            // since ask play does not distinguish media from music and media can be used as music
            cancelNotification();

            if (LLStartUp::getStartupState() > STATE_PRECACHE)
            {
                LLSD args;
                args["URL"] = url;
                LLSD payload;
                payload["url"] = url; // or we can extract it from notification["substitutions"]
                payload["parcel_id"] = parcel_id;
                payload["region_id"] = region_id;
                pNotification = LLNotificationsUtil::add("ParcelPlayingMedia", args, payload, boost::bind(onAskPlayResponse, _1, _2, cb));
            }
            else
            {
                // workaround: avoid 'new notifications arrived' on startup and just play
                // (alternative: move to different channel, may be create new one...)
                cb(region_id, parcel_id, url, true);
            }
        }
    }
}

void LLViewerParcelAskPlay::cancelNotification()
{
    if (pNotification)
    {
        if (!pNotification->isCancelled())
        {
            // Force a responce
            // Alternative is to mark notification as unique
            pNotification->setIgnored(false);
            LLNotifications::getInstance()->cancel(pNotification);
        }
        pNotification = NULL;
    }
}

void LLViewerParcelAskPlay::resetCurrentParcelSetting()
{
    LLParcel *agent_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (agent_parcel && gAgent.getRegion())
    {
        LLViewerParcelAskPlay::resetSetting(gAgent.getRegion()->getRegionID(), agent_parcel->getLocalID());
    }
}

void LLViewerParcelAskPlay::resetSetting(const LLUUID &region_id, const S32 &parcel_id)
{
    region_map_t::iterator found = mRegionMap.find(region_id);
    if (found != mRegionMap.end())
    {
        found->second.erase(parcel_id);
    }
}

void LLViewerParcelAskPlay::resetSettings()
{
    if (LLViewerParcelAskPlay::instanceExists())
    {
        LLViewerParcelAskPlay::getInstance()->mRegionMap.clear();
    }
    LLFile::remove(getAskPlayFilename());
}

void LLViewerParcelAskPlay::setSetting(const LLUUID &region_id, const S32 &parcel_id, const LLViewerParcelAskPlay::ParcelData &data)
{
    mRegionMap[region_id][parcel_id] = data;
}

LLViewerParcelAskPlay::ParcelData* LLViewerParcelAskPlay::getSetting(const LLUUID &region_id, const S32 &parcel_id)
{
    region_map_t::iterator found = mRegionMap.find(region_id);
    if (found != mRegionMap.end())
    {
        parcel_data_map_t::iterator found_parcel = found->second.find(parcel_id);
        if (found_parcel != found->second.end())
        {
            return &(found_parcel->second);
        }
    }
    return NULL;
}

LLViewerParcelAskPlay::EAskPlayMode LLViewerParcelAskPlay::getPlayMode(const LLUUID &region_id, const S32 &parcel_id)
{
    EAskPlayMode mode = ASK_PLAY_ASK;
    ParcelData* data = getSetting(region_id, parcel_id);
    if (data)
    {
        mode = data->mMode;
        // refresh date
        data->mDate = LLDate::now();
    }
    return mode;
}

void LLViewerParcelAskPlay::setPlayMode(const LLUUID &region_id, const S32 &parcel_id, LLViewerParcelAskPlay::EAskPlayMode mode)
{
    ParcelData data;
    data.mMode = mode;
    data.mDate = LLDate::now();
    setSetting(region_id, parcel_id, data);
}

//static
void LLViewerParcelAskPlay::onAskPlayResponse(const LLSD& notification, const LLSD& response, ask_callback cb)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

    LLUUID region_id = notification["payload"]["region_id"];
    S32 parcel_id = notification["payload"]["parcel_id"];
    std::string url = notification["payload"]["url"];

    bool play = option == 1;

    cb(region_id, parcel_id, url, play);

    LLViewerParcelAskPlay *inst = getInstance();
    bool save_choice = inst->pNotification->isIgnored(); // checkbox selected
    if (save_choice)
    {
        EAskPlayMode mode = (play) ? ASK_PLAY_PLAY : ASK_PLAY_IGNORE;
        inst->setPlayMode(region_id, parcel_id, mode);
    }
}

// static
std::string LLViewerParcelAskPlay::getAskPlayFilename()
{
    return gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "media_autoplay.xml");
}

void LLViewerParcelAskPlay::loadSettings()
{
    mRegionMap.clear();

    std::string path = getAskPlayFilename();
    if (!gDirUtilp->fileExists(path))
    {
        return;
    }

    LLSD autoplay_llsd;
    llifstream file;
    file.open(path.c_str());
    if (!file.is_open())
    {
        return;
    }
    S32 status = LLSDSerialize::fromXML(autoplay_llsd, file);
    file.close();

    if (status == LLSDParser::PARSE_FAILURE || !autoplay_llsd.isMap())
    {
        return;
    }

    for (LLSD::map_const_iterator iter_region = autoplay_llsd.beginMap();
        iter_region != autoplay_llsd.endMap(); ++iter_region)
    {
        LLUUID region_id = LLUUID(iter_region->first);
        mRegionMap[region_id] = parcel_data_map_t();

        const LLSD &parcel_map = iter_region->second;

        if (parcel_map.isMap())
        {
            for (LLSD::map_const_iterator iter_parcel = parcel_map.beginMap();
                iter_parcel != parcel_map.endMap(); ++iter_parcel)
            {
                if (!iter_parcel->second.isMap())
                {
                    break;
                }
                S32 parcel_id = boost::lexical_cast<S32>(iter_parcel->first.c_str());
                ParcelData data;
                data.mMode = (EAskPlayMode)(iter_parcel->second["mode"].asInteger());
                data.mDate = iter_parcel->second["date"].asDate();
                mRegionMap[region_id][parcel_id] = data;
            }
        }
    }
}

void LLViewerParcelAskPlay::saveSettings()
{
    LLSD write_llsd;
    std::string key;
    for (region_map_t::iterator iter_region = mRegionMap.begin();
        iter_region != mRegionMap.end(); ++iter_region)
    {
        if (iter_region->second.empty())
        {
            continue;
        }
        key = iter_region->first.asString();
        write_llsd[key] = LLSD();
        
        for (parcel_data_map_t::iterator iter_parcel = iter_region->second.begin();
            iter_parcel != iter_region->second.end(); ++iter_parcel)
        {
            if ((iter_parcel->second.mDate.secondsSinceEpoch() + (F64SecondsImplicit)U32Days(30)) > LLTimer::getTotalSeconds())
            {
                // write unexpired parcels
                std::string parcel_id = boost::lexical_cast<std::string>(iter_parcel->first);
                write_llsd[key][parcel_id] = LLSD();
                write_llsd[key][parcel_id]["mode"] = (LLSD::Integer)iter_parcel->second.mMode;
                write_llsd[key][parcel_id]["date"] = iter_parcel->second.mDate;
            }
        }
    }
    
    llofstream file;
    file.open(getAskPlayFilename().c_str());
    if (file.is_open())
    {
        LLSDSerialize::toPrettyXML(write_llsd, file);
        file.close();
    }
}

