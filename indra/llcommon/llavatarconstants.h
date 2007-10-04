/** 
 * @file llavatarconstants.h
 * @brief some useful short term constants for Indra
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

static const std::string VISIBILITY_DEFAULT("default");
static const std::string VISIBILITY_HIDDEN("hidden");
static const std::string VISIBILITY_VISIBLE("visible");
static const std::string VISIBILITY_INVISIBLE("invisible");

#endif

