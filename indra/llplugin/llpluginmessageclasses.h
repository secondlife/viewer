/** 
 * @file llpluginmessageclasses.h
 * @brief This file defines the versions of existing message classes for LLPluginMessage.
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 * @endcond
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
