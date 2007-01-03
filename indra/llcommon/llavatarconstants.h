/** 
 * @file indra_constants.h
 * @brief some useful short term constants for Indra
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

#endif

