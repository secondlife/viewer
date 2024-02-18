/** 
 * @file llwindowheadless.cpp
 * @brief Headless implementation of LLWindow class
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
#include "indra_constants.h"

#include "llwindowheadless.h"
#include "llkeyboardheadless.h"

//
// LLWindowHeadless
//
LLWindowHeadless::LLWindowHeadless(LLWindowCallbacks* callbacks, const std::string& title, const std::string& name, S32 x, S32 y, S32 width, S32 height,
							 U32 flags,  bool fullscreen, bool clear_background,
							 bool enable_vsync, bool use_gl, bool ignore_pixel_depth)
	: LLWindow(callbacks, fullscreen, flags)
{
	// Initialize a headless keyboard.
	gKeyboard = new LLKeyboardHeadless();
	gKeyboard->setCallbacks(callbacks);
}


LLWindowHeadless::~LLWindowHeadless()
{
}

void LLWindowHeadless::swapBuffers()
{
}
