/** 
 * @file llextendedstatus.h
 * @date   August 2007
 * @brief extended status codes for curl/vfs/resident asset storage and delivery
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
	LL_EXSTAT_CURL_RESULT	= 1UL<<30, // serviced by curl - use 1L if we really implement the below
	LL_EXSTAT_RES_RESULT	= 2UL<<30, // serviced by resident copy
	LL_EXSTAT_VFS_RESULT	= 3UL<<30, // serviced by vfs


	// Common Status Codes
	//
	LL_EXSTAT_NONE				= 0x00000, // No extra info here - sorry!
	LL_EXSTAT_NULL_UUID			= 0x10001, // null asset ID
	LL_EXSTAT_NO_UPSTREAM		= 0x10002, // attempt to upload without a valid upstream method/provider
	LL_EXSTAT_REQUEST_DROPPED	= 0x10003, // request was dropped unserviced
	LL_EXSTAT_NONEXISTENT_FILE	= 0x10004, // trying to upload a file that doesn't exist
	LL_EXSTAT_BLOCKED_FILE		= 0x10005, // trying to upload a file that we can't open


	// curl status codes:
	//
	// Mask off LL_EXSTAT_CURL_RESULT for original result and
	// see: libraries/include/curl/curl.h


	// Memory-Resident status codes:
	// None at present


	// VFS status codes:
	LL_EXSTAT_VFS_CACHED	= LL_EXSTAT_VFS_RESULT | 0x0001,
	LL_EXSTAT_VFS_CORRUPT	= LL_EXSTAT_VFS_RESULT | 0x0002,
};


#endif // LL_LLEXTENDEDSTATUS_H
