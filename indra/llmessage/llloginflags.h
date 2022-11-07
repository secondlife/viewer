/** 
 * @file llloginflags.h
 * @brief Login flag constants.
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

#ifndef LL_LLLOGINFLAGS_H
#define LL_LLLOGINFLAGS_H

// Is this your first login to Second Life?
const U32   LOGIN_FLAG_FIRST_LOGIN          = (1 << 0);

// Is this a trial account?
const U32   LOGIN_FLAG_TRIAL                = (1 << 1);

// Stipend paid since last login?
const U32   LOGIN_FLAG_STIPEND_SINCE_LOGIN  = (1 << 2);

// Is your account enabled?
const U32   LOGIN_FLAG_ENABLED              = (1 << 3);

// Is the Pacific Time zone (aka the server time zone)
// currently observing daylight savings time?
const U32 LOGIN_FLAG_PACIFIC_DAYLIGHT_TIME  = (1 << 4);

// Does the avatar have wearables or not
const U32 LOGIN_FLAG_GENDERED = (1 << 5);

// Kick flags
const U32 LOGIN_KICK_OK = 0x0;
const U32 LOGIN_KICK_NO_AGENT = (1 << 0);
const U32 LOGIN_KICK_SESSION_MISMATCH = (1 << 1);
const U32 LOGIN_KICK_NO_SIMULATOR = (1 << 2);

#endif
