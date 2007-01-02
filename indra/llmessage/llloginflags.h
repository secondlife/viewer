/** 
 * @file llloginflags.h
 * @brief Login flag constants.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLLOGINFLAGS_H
#define LL_LLLOGINFLAGS_H

// Is this your first login to Second Life?
const U32	LOGIN_FLAG_FIRST_LOGIN			= (1 << 0);

// Is this a trial account?
const U32	LOGIN_FLAG_TRIAL				= (1 << 1);

// Stipend paid since last login?
const U32	LOGIN_FLAG_STIPEND_SINCE_LOGIN	= (1 << 2);

// Is your account enabled?
const U32	LOGIN_FLAG_ENABLED				= (1 << 3);

// Is the Pacific Time zone (aka the server time zone)
// currently observing daylight savings time?
const U32 LOGIN_FLAG_PACIFIC_DAYLIGHT_TIME	= (1 << 4);

// Does the avatar have wearables or not
const U32 LOGIN_FLAG_GENDERED = (1 << 5);

// Kick flags
const U32 LOGIN_KICK_OK = 0x0;
const U32 LOGIN_KICK_NO_AGENT = (1 << 0);
const U32 LOGIN_KICK_SESSION_MISMATCH = (1 << 1);
const U32 LOGIN_KICK_NO_SIMULATOR = (1 << 2);

#endif
