/** 
 * @file llloginflags.h
 * @brief Login flag constants.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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
