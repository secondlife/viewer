/**
 * @file lllandmark.cpp
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

#include "linden_common.h"
#include "lllandmark.h"

#include <errno.h>

#include "message.h"
#include "llregionhandle.h"

std::pair<LLUUID, U64> LLLandmark::mLocalRegion;
LLLandmark::region_map_t LLLandmark::mRegions;
LLLandmark::region_callback_map_t LLLandmark::sRegionCallbackMap;

LLLandmark::LLLandmark() :
    mGlobalPositionKnown(false)
{
}

LLLandmark::LLLandmark(const LLVector3d& pos) :
    mGlobalPositionKnown(true),
    mGlobalPos( pos )
{
}

bool LLLandmark::getGlobalPos(LLVector3d& pos)
{
    if(mGlobalPositionKnown)
    {
        pos = mGlobalPos;
    }
    else if(mRegionID.notNull())
    {
        F32 g_x = -1.0;
        F32 g_y = -1.0;
        if(mRegionID == mLocalRegion.first)
        {
            from_region_handle(mLocalRegion.second, &g_x, &g_y);
        }
        else
        {
            region_map_t::iterator it = mRegions.find(mRegionID);
            if(it != mRegions.end())
            {
                from_region_handle((*it).second.mRegionHandle, &g_x, &g_y);
            }
        }
        if((g_x > 0.f) && (g_y > 0.f))
        {
            pos.mdV[0] = g_x + mRegionPos.mV[0];
            pos.mdV[1] = g_y + mRegionPos.mV[1];
            pos.mdV[2] = mRegionPos.mV[2];
            setGlobalPos(pos);
        }
    }
    return mGlobalPositionKnown;
}

void LLLandmark::setGlobalPos(const LLVector3d& pos)
{
    mGlobalPos = pos;
    mGlobalPositionKnown = true;
}

bool LLLandmark::getRegionID(LLUUID& region_id)
{
    if(mRegionID.notNull())
    {
        region_id = mRegionID;
        return true;
    }
    return false;
}

LLVector3 LLLandmark::getRegionPos() const
{
    return mRegionPos;
}


// static
LLLandmark* LLLandmark::constructFromString(const char *buffer, const S32 buffer_size)
{
    S32 chars_read = 0;
    S32 chars_read_total = 0;
    S32 count = 0;
    U32 version = 0;

    bool bad_block = false;
    LLLandmark* result = NULL;

    // read version
    count = sscanf( buffer, "Landmark version %u\n%n", &version, &chars_read );
    chars_read_total += chars_read;

    if (count != 1
        || chars_read_total >= buffer_size)
    {
        bad_block = true;
    }

    if (!bad_block)
    {
        switch (version)
        {
            case 1:
            {
                LLVector3d pos;
                // read position
                count = sscanf(buffer + chars_read_total, "position %lf %lf %lf\n%n", pos.mdV + VX, pos.mdV + VY, pos.mdV + VZ, &chars_read);
                if (count != 3)
                {
                    bad_block = true;
                }
                else
                {
                    LL_DEBUGS("Landmark") << "Landmark read: " << pos << LL_ENDL;
                    result = new LLLandmark(pos);
                }
                break;
            }
            case 2:
            {
                // *NOTE: Changing the buffer size will require changing the
                // scanf call below.
                char region_id_str[MAX_STRING];
                LLVector3 pos;
                LLUUID region_id;
                count = sscanf( buffer + chars_read_total,
                                "region_id %254s\n%n",
                                region_id_str,
                                &chars_read);
                chars_read_total += chars_read;

                if (count != 1
                    || chars_read_total >= buffer_size
                    || !LLUUID::validate(region_id_str))
                {
                    bad_block = true;
                }

                if (!bad_block)
                {
                    region_id.set(region_id_str);
                    if (region_id.isNull())
                    {
                        bad_block = true;
                    }
                }

                if (!bad_block)
                {
                    count = sscanf(buffer + chars_read_total, "local_pos %f %f %f\n%n", pos.mV + VX, pos.mV + VY, pos.mV + VZ, &chars_read);
                    if (count != 3)
                    {
                        bad_block = true;
                    }
                    else
                    {
                        result = new LLLandmark;
                        result->mRegionID = region_id;
                        result->mRegionPos = pos;
                    }
                }
                break;
            }
            default:
            {
                LL_INFOS("Landmark") << "Encountered Unknown landmark version " << version << LL_ENDL;
                break;
            }
        }
    }

    if (bad_block)
    {
        LL_INFOS("Landmark") << "Bad Landmark Asset: bad _DATA_ block." << LL_ENDL;
    }
    return result;
}


// static
void LLLandmark::registerCallbacks(LLMessageSystem* msg)
{
    msg->setHandlerFunc("RegionIDAndHandleReply", &processRegionIDAndHandle);
}

// static
void LLLandmark::requestRegionHandle(
    LLMessageSystem* msg,
    const LLHost& upstream_host,
    const LLUUID& region_id,
    region_handle_callback_t callback)
{
    if(region_id.isNull())
    {
        // don't bother with checking - it's 0.
        LL_DEBUGS("Landmark") << "requestRegionHandle: null" << LL_ENDL;
        if(callback)
        {
            const U64 U64_ZERO = 0;
            callback(region_id, U64_ZERO);
        }
    }
    else
    {
        if(region_id == mLocalRegion.first)
        {
            LL_DEBUGS("Landmark") << "requestRegionHandle: local" << LL_ENDL;
            if(callback)
            {
                callback(region_id, mLocalRegion.second);
            }
        }
        else
        {
            region_map_t::iterator it = mRegions.find(region_id);
            if(it == mRegions.end())
            {
                LL_DEBUGS("Landmark") << "requestRegionHandle: upstream" << LL_ENDL;
                if(callback)
                {
                    region_callback_map_t::value_type vt(region_id, callback);
                    sRegionCallbackMap.insert(vt);
                }
                LL_DEBUGS("Landmark") << "Landmark requesting information about: "
                         << region_id << LL_ENDL;
                msg->newMessage("RegionHandleRequest");
                msg->nextBlock("RequestBlock");
                msg->addUUID("RegionID", region_id);
                msg->sendReliable(upstream_host);
            }
            else if(callback)
            {
                // we have the answer locally - just call the callack.
                LL_DEBUGS("Landmark") << "requestRegionHandle: ready" << LL_ENDL;
                callback(region_id, (*it).second.mRegionHandle);
            }
        }
    }

    // As good a place as any to expire old entries.
    expireOldEntries();
}

// static
void LLLandmark::setRegionHandle(const LLUUID& region_id, U64 region_handle)
{
    mLocalRegion.first = region_id;
    mLocalRegion.second = region_handle;
}


// static
void LLLandmark::processRegionIDAndHandle(LLMessageSystem* msg, void**)
{
    LLUUID region_id;
    msg->getUUID("ReplyBlock", "RegionID", region_id);
    mRegions.erase(region_id);
    CacheInfo info;
    const F32 CACHE_EXPIRY_SECONDS = 60.0f * 10.0f; // ten minutes
    info.mTimer.setTimerExpirySec(CACHE_EXPIRY_SECONDS);
    msg->getU64("ReplyBlock", "RegionHandle", info.mRegionHandle);
    region_map_t::value_type vt(region_id, info);
    mRegions.insert(vt);

#if LL_DEBUG
    U32 grid_x, grid_y;
    grid_from_region_handle(info.mRegionHandle, &grid_x, &grid_y);
    LL_DEBUGS() << "Landmark got reply for region: " << region_id << " "
             << grid_x << "," << grid_y << LL_ENDL;
#endif

    // make all the callbacks here.
    region_callback_map_t::iterator it;
    while((it = sRegionCallbackMap.find(region_id)) != sRegionCallbackMap.end())
    {
        (*it).second(region_id, info.mRegionHandle);
        sRegionCallbackMap.erase(it);
    }
}

// static
void LLLandmark::expireOldEntries()
{
    for(region_map_t::iterator it = mRegions.begin(); it != mRegions.end(); )
    {
        if((*it).second.mTimer.hasExpired())
        {
            mRegions.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}
