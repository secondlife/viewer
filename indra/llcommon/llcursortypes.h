/** 
 * @file llcursortypes.h
 * @brief Cursor types
 *
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
 */

#ifndef LL_LLCURSORTYPES_H
#define LL_LLCURSORTYPES_H

// If you add types here, add them in LLCursor::getCursorFromString
enum ECursorType {
	UI_CURSOR_ARROW,
	UI_CURSOR_WAIT,
	UI_CURSOR_HAND,
	UI_CURSOR_IBEAM,
	UI_CURSOR_CROSS,
	UI_CURSOR_SIZENWSE,
	UI_CURSOR_SIZENESW,
	UI_CURSOR_SIZEWE,
	UI_CURSOR_SIZENS,
	UI_CURSOR_NO,
	UI_CURSOR_WORKING,
	UI_CURSOR_TOOLGRAB,
	UI_CURSOR_TOOLLAND,
	UI_CURSOR_TOOLFOCUS,
	UI_CURSOR_TOOLCREATE,
	UI_CURSOR_ARROWDRAG,
	UI_CURSOR_ARROWCOPY,	// drag with copy
	UI_CURSOR_ARROWDRAGMULTI,
	UI_CURSOR_ARROWCOPYMULTI,	// drag with copy
	UI_CURSOR_NOLOCKED,
	UI_CURSOR_ARROWLOCKED,
	UI_CURSOR_GRABLOCKED,
	UI_CURSOR_TOOLTRANSLATE,
	UI_CURSOR_TOOLROTATE,
	UI_CURSOR_TOOLSCALE,
	UI_CURSOR_TOOLCAMERA,
	UI_CURSOR_TOOLPAN,
	UI_CURSOR_TOOLZOOMIN,
	UI_CURSOR_TOOLPICKOBJECT3,
	UI_CURSOR_TOOLPLAY,
	UI_CURSOR_TOOLPAUSE,
	UI_CURSOR_TOOLMEDIAOPEN,
	UI_CURSOR_PIPETTE,
	UI_CURSOR_TOOLSIT,
	UI_CURSOR_TOOLBUY,
	UI_CURSOR_TOOLOPEN,
	UI_CURSOR_TOOLPATHFINDING,
	UI_CURSOR_TOOLPATHFINDING_PATH_START,
	UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD,
	UI_CURSOR_TOOLPATHFINDING_PATH_END,
	UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD,
	UI_CURSOR_TOOLNO,
	UI_CURSOR_COUNT			// Number of elements in this enum (NOT a cursor)
};

LL_COMMON_API ECursorType getCursorFromString(const std::string& cursor_string);

#endif // LL_LLCURSORTYPES_H
