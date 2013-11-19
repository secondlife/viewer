/** 
 * @file llavatarconstants.h
 * @brief some useful short term constants for Indra
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_AVATAR_CONSTANTS_H
#define LL_AVATAR_CONSTANTS_H

// If this string is passed to dataserver in AvatarPropertiesUpdate 
// then no change is made to user.profile_web
const char* const BLACKLIST_PROFILE_WEB_STR = "featureWebProfilesDisabled";

// If profile web pages are feature blacklisted then this URL is 
// shown in the profile instead of the user's set URL
const char* const BLACKLIST_PROFILE_WEB_URL = "http://secondlife.com/app/webdisabled";

// Maximum number of avatar picks
const S32 MAX_AVATAR_PICKS = 10;

// For Flags in AvatarPropertiesReply
const U32 AVATAR_ALLOW_PUBLISH			= 0x1 << 0;	// whether profile is externally visible or not
const U32 AVATAR_MATURE_PUBLISH			= 0x1 << 1;	// profile is "mature"
const U32 AVATAR_IDENTIFIED				= 0x1 << 2;	// whether avatar has provided payment info
const U32 AVATAR_TRANSACTED				= 0x1 << 3;	// whether avatar has actively used payment info
const U32 AVATAR_ONLINE					= 0x1 << 4; // the online status of this avatar, if known.
const U32 AVATAR_AGEVERIFIED			= 0x1 << 5;	// whether avatar has been age-verified

char const* const VISIBILITY_DEFAULT = "default";
char const* const VISIBILITY_HIDDEN = "hidden";
char const* const VISIBILITY_VISIBLE = "visible";
char const* const VISIBILITY_INVISIBLE = "invisible";

#endif

