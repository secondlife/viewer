/** 
 * @file llerrorlegacy.h
 * @date   January 2007
 * @brief old things from the older error system
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLERRORLEGACY_H
#define LL_LLERRORLEGACY_H



/*
	LEGACY -- DO NOT USE THIS STUFF ANYMORE
*/

// Specific error codes
const int LL_ERR_NOERR = 0;
const int LL_ERR_ASSET_REQUEST_FAILED = -1;
//const int LL_ERR_ASSET_REQUEST_INVALID = -2;
const int LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE = -3;
const int LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE = -4;
const int LL_ERR_INSUFFICIENT_PERMISSIONS = -5;
const int LL_ERR_EOF = -39;
const int LL_ERR_CANNOT_OPEN_FILE = -42;
const int LL_ERR_FILE_NOT_FOUND = -43;
const int LL_ERR_FILE_EMPTY     = -44;
const int LL_ERR_TCP_TIMEOUT    = -23016;
const int LL_ERR_CIRCUIT_GONE   = -23017;
const int LL_ERR_PRICE_MISMATCH = -23018;



// Define one of these for different error levels in release...
// #define RELEASE_SHOW_DEBUG // Define this if you want your release builds to show lldebug output.
#define RELEASE_SHOW_INFO // Define this if you want your release builds to show llinfo output
#define RELEASE_SHOW_WARN // Define this if you want your release builds to show llwarn output.


//////////////////////////////////////////
//
//  Implementation - ignore
//
//
#ifdef _DEBUG
#define SHOW_DEBUG
#define SHOW_WARN
#define SHOW_INFO
#define SHOW_ASSERT
#else // _DEBUG

#ifdef LL_RELEASE_WITH_DEBUG_INFO
#define SHOW_ASSERT
#endif // LL_RELEASE_WITH_DEBUG_INFO

#ifdef RELEASE_SHOW_DEBUG
#define SHOW_DEBUG
#endif

#ifdef RELEASE_SHOW_WARN
#define SHOW_WARN
#endif

#ifdef RELEASE_SHOW_INFO
#define SHOW_INFO
#endif

#ifdef RELEASE_SHOW_ASSERT
#define SHOW_ASSERT
#endif

#endif // _DEBUG



#define lldebugst(type)			lldebugs
#define llendflush				llendl


#define llerror(msg, num)		llerrs << "Error # " << num << ": " << msg << llendl;

#define llwarning(msg, num)		llwarns << "Warning # " << num << ": " << msg << llendl;

#define llassert_always(func)	if (!(func)) llerrs << "ASSERT (" << #func << ")" << llendl;

#ifdef SHOW_ASSERT
#define llassert(func)			llassert_always(func)
#define llverify(func)			llassert_always(func)
#else
#define llassert(func)
#define llverify(func)			do {if (func) {}} while(0)
#endif

// handy compile-time assert - enforce those template parameters! 
#define cassert(expn) typedef char __C_ASSERT__[(expn)?1:-1]   /* Flawfinder: ignore */
	//XXX: used in two places in llcommon/llskipmap.h

#endif // LL_LLERRORLEGACY_H
