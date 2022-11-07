/** 
 * @file llworldmapmessage.h
 * @brief Handling of the messages to the DB made by and for the world map.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLWORLDMAPMESSAGE_H
#define LL_LLWORLDMAPMESSAGE_H

#include "boost/function.hpp"

// Handling of messages (send and process) as well as SLURL callback if necessary
class LLMessageSystem;

class LLWorldMapMessage : public LLSingleton<LLWorldMapMessage>
{
    LLSINGLETON(LLWorldMapMessage);
    ~LLWorldMapMessage();

public:
    typedef boost::function<void(U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport)>
        url_callback_t;

    // Process incoming answers to map stuff requests
    static void processMapBlockReply(LLMessageSystem*, void**);
    static void processMapItemReply(LLMessageSystem*, void**);

    // Request data for all regions in a rectangular area. Coordinates in grids (i.e. meters / 256).
    void sendMapBlockRequest(U16 min_x, U16 min_y, U16 max_x, U16 max_y, bool return_nonexistent = false);

    // Various methods to request LLSimInfo data to the simulator and asset DB
    void sendNamedRegionRequest(std::string region_name);
    void sendNamedRegionRequest(std::string region_name, 
        url_callback_t callback,
        const std::string& callback_url,
        bool teleport);
    void sendHandleRegionRequest(U64 region_handle, 
        url_callback_t callback,
        const std::string& callback_url,
        bool teleport);

    // Request item data for regions
    // Note: the handle works *only* when requesting agent count (type = MAP_ITEM_AGENT_LOCATIONS). In that case,
    // the request will actually be transitting through the spaceserver (all that is done on the sim).
    // All other values of type do create a global grid request to the asset DB. So no need to try to get, say,
    // the events for one particular region. For such a request, the handle is ignored.
    void sendItemRequest(U32 type, U64 handle = 0);

private:
    // Search for region (by name or handle) for SLURL processing and teleport
    // None of this relies explicitly on the LLWorldMap instance so better handle it here
    std::string     mSLURLRegionName;
    U64             mSLURLRegionHandle;
    std::string     mSLURL;
    url_callback_t  mSLURLCallback;
    bool            mSLURLTeleport;
};

#endif // LL_LLWORLDMAPMESSAGE_H
