/**
 * @file llviewerparcelaskplay.h
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

#ifndef LLVIEWERPARCELASKPLAY_H
#define LLVIEWERPARCELASKPLAY_H

#include "llnotificationptr.h"
#include "lluuid.h"

class LLViewerParcelAskPlay : public LLSingleton<LLViewerParcelAskPlay>
{
    LLSINGLETON(LLViewerParcelAskPlay);
    ~LLViewerParcelAskPlay();
    void initSingleton();
    void cleanupSingleton();
public:
    // functor expects functor(region_id, parcel_id, url, play/stop)
    typedef boost::function<void(const LLUUID&, const S32&, const std::string&, const bool&)> ask_callback;
    void        askToPlay(const LLUUID &region_id, const S32 &parcel_id, const std::string &url, ask_callback cb);
    void        cancelNotification();

    void        resetCurrentParcelSetting();
    void        resetSetting(const LLUUID &region_id, const S32 &parcel_id);
    static void resetSettings();

    void        loadSettings();
    void        saveSettings();

    S32         hasData() { return !mRegionMap.empty(); } // subsitution for 'isInitialized'

private:
    enum EAskPlayMode{
        ASK_PLAY_IGNORE = 0,
        ASK_PLAY_PLAY,
        ASK_PLAY_ASK
    };

    class ParcelData
    {
    public:
        LLDate mDate;
        EAskPlayMode mMode;
    };

    void               setSetting(const LLUUID &region_id, const S32 &parcel_id, const ParcelData &data);
    ParcelData*        getSetting(const LLUUID &region_id, const S32 &parcel_id);
    EAskPlayMode       getPlayMode(const LLUUID &region_id, const S32 &parcel_id);
    void               setPlayMode(const LLUUID &region_id, const S32 &parcel_id, EAskPlayMode);

    static void        onAskPlayResponse(const LLSD& notification, const LLSD& response, ask_callback cb);

    static std::string getAskPlayFilename();

private:
    // Variables

    typedef std::map<S32, ParcelData> parcel_data_map_t;
    typedef std::map<LLUUID, parcel_data_map_t> region_map_t;
    region_map_t mRegionMap;

    // only one notification is supposed to exists and be visible
    LLNotificationPtr pNotification;
};


#endif // LLVIEWERPARCELASKPLAY_H
