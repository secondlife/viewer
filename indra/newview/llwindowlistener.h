/** 
 * @file llwindowlistener.h
 * @brief EventAPI interface for injecting input into LLWindow
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

#ifndef LL_LLWINDOWLISTENER_H
#define LL_LLWINDOWLISTENER_H

#include "lleventapi.h"
#include <boost/function.hpp>

class LLKeyboard;
class LLViewerWindow;

class LLWindowListener : public LLEventAPI
{
public:
	typedef boost::function<LLKeyboard*()> KeyboardGetter;
	LLWindowListener(LLViewerWindow * window, const KeyboardGetter& kbgetter);

	void getInfo(LLSD const & evt);
	void getPaths(LLSD const & evt);
	void keyDown(LLSD const & evt);
	void keyUp(LLSD const & evt);
	void mouseDown(LLSD const & evt);
	void mouseUp(LLSD const & evt);
	void mouseMove(LLSD const & evt);
	void mouseScroll(LLSD const & evt);

private:
	LLViewerWindow * mWindow;
	KeyboardGetter mKbGetter;
};


#endif // LL_LLWINDOWLISTENER_H
