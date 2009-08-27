/** 
 * @file llpluginmessageclasses.h
 * @brief This file defines the versions of existing message classes for LLPluginMessage.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2008, Linden Research, Inc.
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

#ifndef LL_LLPLUGINMESSAGECLASSES_H
#define LL_LLPLUGINMESSAGECLASSES_H

// Version strings for each plugin message class.  
// Backwards-compatible changes (i.e. changes which only add new messges) should increment the minor version (i.e. "1.0" -> "1.1").
// Non-backwards-compatible changes (which delete messages or change their semantics) should increment the major version (i.e. "1.1" -> "2.0").
// Plugins will supply the set of message classes they understand, with version numbers, as part of their init_response message.
// The contents and semantics of the base:init message must NEVER change in a non-backwards-compatible way, as a special case.

#define LLPLUGIN_MESSAGE_CLASS_INTERNAL "internal"
#define LLPLUGIN_MESSAGE_CLASS_INTERNAL_VERSION "1.0"

#define LLPLUGIN_MESSAGE_CLASS_BASE "base"
#define LLPLUGIN_MESSAGE_CLASS_BASE_VERSION "1.0"

#define LLPLUGIN_MESSAGE_CLASS_MEDIA "media"
#define LLPLUGIN_MESSAGE_CLASS_MEDIA_VERSION "1.0"

#define LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER "media_browser"
#define LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER_VERSION "1.0"

#define LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME "media_time"
#define LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME_VERSION "1.0"

#endif // LL_LLPLUGINMESSAGECLASSES_H
