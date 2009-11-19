/**
 * @file lldragdrop32.cpp
 * @brief Handler for Windows specific drag and drop (OS to client) code
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#if LL_WINDOWS

#if LL_OS_DRAGDROP_ENABLED

#ifndef LL_LLDRAGDROP32_H
#define LL_LLDRAGDROP32_H

#include <windows.h>
#include <ole2.h>

class LLDragDropWin32
{
	public:
		LLDragDropWin32();
		~LLDragDropWin32();

		bool init( HWND hWnd );
		void reset();

	private:
		IDropTarget* mDropTarget;
		HWND mDropWindowHandle;
};
#endif // LL_LLDRAGDROP32_H

#else // LL_OS_DRAGDROP_ENABLED

#ifndef LL_LLDRAGDROP32_H
#define LL_LLDRAGDROP32_H

#include <windows.h>
#include <ole2.h>

// imposter class that does nothing 
class LLDragDropWin32
{
	public:
		LLDragDropWin32() {};
		~LLDragDropWin32() {};

		bool init( HWND hWnd ) { return false; };
		void reset() { };
};
#endif // LL_LLDRAGDROP32_H

#endif // LL_OS_DRAGDROP_ENABLED

#endif // LL_WINDOWS
