/** 
 * @file llkeyboardheadless.cpp
 * @brief Handler for assignable key bindings
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"
#include "llkeyboardheadless.h"
#include "llwindowcallbacks.h"

LLKeyboardHeadless::LLKeyboardHeadless()
{ }

void LLKeyboardHeadless::resetMaskKeys()
{ }


BOOL LLKeyboardHeadless::handleKeyDown(const U16 key, const U32 mask)
{ return FALSE; }


BOOL LLKeyboardHeadless::handleKeyUp(const U16 key, const U32 mask)
{ return FALSE; }

MASK LLKeyboardHeadless::currentMask(BOOL for_mouse_event)
{ return MASK_NONE; }

void LLKeyboardHeadless::scanKeyboard()
{
	for (S32 key = 0; key < KEY_COUNT; key++)
	{
		// Generate callback if any event has occurred on this key this frame.
		// Can't just test mKeyLevel, because this could be a slow frame and
		// key might have gone down then up. JC
		if (mKeyLevel[key] || mKeyDown[key] || mKeyUp[key])
		{
			mCurScanKey = key;
			mCallbacks->handleScanKey(key, mKeyDown[key], mKeyUp[key], mKeyLevel[key]);
		}
	}

	// Reset edges for next frame
	for (S32 key = 0; key < KEY_COUNT; key++)
	{
		mKeyUp[key] = FALSE;
		mKeyDown[key] = FALSE;
		if (mKeyLevel[key])
		{
			mKeyLevelFrameCount[key]++;
		}
	}
}
 
