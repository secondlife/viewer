/**
 * @file lldragdrop32.cpp
 * @brief Handler for Windows specific drag and drop (OS to client) code
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#if LL_WINDOWS

#if LL_OS_DRAGDROP_ENABLED

#ifndef LL_LLDRAGDROP32_H
#define LL_LLDRAGDROP32_H

#define NOMINMAX
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

#define NOMINMAX
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
