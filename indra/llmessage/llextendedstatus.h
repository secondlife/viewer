/** 
 * @file llextendedstatus.h
 * @date   August 2007
 * @brief extended status codes for curl/resident asset storage and delivery
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLEXTENDEDSTATUS_H
#define LL_LLEXTENDEDSTATUS_H

enum class LLExtStat: uint32_t
{
    // Status provider groups - Top bits indicate which status type it is
    // Zero is common status code (next section)
    CURL_RESULT  = 1UL<<30, // serviced by curl - use 1L if we really implement the below
    RES_RESULT   = 2UL<<30, // serviced by resident copy
    CACHE_RESULT = 3UL<<30, // serviced by cache


    // Common Status Codes
    //
    NONE            = 0x00000, // No extra info here - sorry!
    NULL_UUID       = 0x10001, // null asset ID
    NO_UPSTREAM     = 0x10002, // attempt to upload without a valid upstream method/provider
    REQUEST_DROPPED = 0x10003, // request was dropped unserviced
    NONEXISTENT_FILE= 0x10004, // trying to upload a file that doesn't exist
    BLOCKED_FILE    = 0x10005, // trying to upload a file that we can't open

    // curl status codes:
    //
    // Mask off CURL_RESULT for original result and
    // see: libraries/include/curl/curl.h

    // Memory-Resident status codes:
    // None at present

    // CACHE status codes:
    CACHE_CACHED    = CACHE_RESULT | 0x0001,
    CACHE_CORRUPT   = CACHE_RESULT | 0x0002,
};


#endif // LL_LLEXTENDEDSTATUS_H
