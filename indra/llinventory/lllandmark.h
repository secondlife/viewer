/** 
 * @file lllandmark.h
 * @brief Landmark asset class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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


#ifndef LL_LLLANDMARK_H
#define LL_LLLANDMARK_H

#include <map>
#include <boost/function.hpp>
#include "llframetimer.h"
#include "lluuid.h"
#include "v3dmath.h"

class LLMessageSystem;
class LLHost;

class LLLandmark
{
public:
    // for calling back interested parties when a region handle comes back.
    typedef boost::function<void(const LLUUID& region_id, const U64& region_handle)> region_handle_callback_t;

    ~LLLandmark() {}

    // returns true if the position is known.
    bool getGlobalPos(LLVector3d& pos);

    // setter used in conjunction if more information needs to be
    // collected from the server.
    void setGlobalPos(const LLVector3d& pos);

    // return true if the region is known
    bool getRegionID(LLUUID& region_id);

    // return the local coordinates if known
    LLVector3 getRegionPos() const;

    // constructs a new LLLandmark from a string
    // return NULL if there's an error
    static LLLandmark* constructFromString(const char *buffer, const S32 buffer_size);

    // register callbacks that this class handles
    static void registerCallbacks(LLMessageSystem* msg);

    // request information about region_id to region_handle.Pass in a
    // callback pointer which will be erase but NOT deleted after the
    // callback is made. This function may call into the message
    // system to get the information.
    static void requestRegionHandle(
        LLMessageSystem* msg,
        const LLHost& upstream_host,
        const LLUUID& region_id,
        region_handle_callback_t callback);

    // Call this method to create a lookup for this region. This
    // simplifies a lot of the code.
    static void setRegionHandle(const LLUUID& region_id, U64 region_handle);
        
private:
    LLLandmark();
    LLLandmark(const LLVector3d& pos);

    static void processRegionIDAndHandle(LLMessageSystem* msg, void**);
    static void expireOldEntries();

private:
    LLUUID mRegionID;
    LLVector3 mRegionPos;
    bool mGlobalPositionKnown;
    LLVector3d mGlobalPos;
    
    struct CacheInfo
    {
        U64 mRegionHandle;
        LLFrameTimer mTimer;
    };

    static std::pair<LLUUID, U64> mLocalRegion;
    typedef std::map<LLUUID, CacheInfo> region_map_t;
    static region_map_t mRegions;
    typedef std::multimap<LLUUID, region_handle_callback_t> region_callback_map_t;
    static region_callback_map_t sRegionCallbackMap;
};

#endif
