/** 
 * @file llcursortypes.h
 * @brief Cursor types
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
	UI_CURSOR_COUNT			// Number of elements in this enum (NOT a cursor)
};

LL_COMMON_API ECursorType getCursorFromString(const std::string& cursor_string);

#endif // LL_LLCURSORTYPES_H
