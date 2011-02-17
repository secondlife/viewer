/** 
 * @file llwindowlistener.cpp
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

#include "linden_common.h"

#include "llwindowlistener.h"

#include "llcoord.h"
#include "llkeyboard.h"
#include "llwindowcallbacks.h"

LLWindowListener::LLWindowListener(LLWindowCallbacks *window, LLKeyboard * keyboard)
	: LLEventAPI("LLWindow", "Inject input events into the LLWindow instance"),
	  mWindow(window),
	  mKeyboard(keyboard)
{
	add("keyDown",
		"Given [\"keycode\"] or [\"char\"], will inject the given keypress event.",
		&LLWindowListener::keyDown);
	add("keyUp",
		"Given [\"keycode\"] or [\"char\"], will inject the given key release event.",
		&LLWindowListener::keyUp);
	add("mouseDown",
		"Given [\"button\"], [\"x\"] and [\"y\"], will inject the given mouse click event.",
		&LLWindowListener::mouseDown);
	add("mouseUp",
		"Given [\"button\"], [\"x\"] and [\"y\"], will inject the given mouse release event.",
		&LLWindowListener::mouseUp);
	add("mouseMove",
		"Given [\"x\"] and [\"y\"], will inject the given mouse movement event.",
		&LLWindowListener::mouseMove);
	add("mouseScroll",
		"Given a number of [\"clicks\"], will inject the given mouse scroll event.",
		&LLWindowListener::mouseScroll);
}

void LLWindowListener::keyDown(LLSD const & evt)
{
	if(NULL == mKeyboard)
	{
		// *HACK to handle the fact that LLWindow subclasses have to initialize
		// things in an inconvenient order
		mKeyboard = gKeyboard;
	}

	KEY keycode = 0;
	if(evt.has("keycode"))
	{
		keycode = KEY(evt["keycode"].asInteger());
	}
	else
	{
		keycode = KEY(evt["char"].asString()[0]);
	}

	// *TODO - figure out how to handle the mask
	mKeyboard->handleTranslatedKeyDown(keycode, 0);
}

void LLWindowListener::keyUp(LLSD const & evt)
{
	if(NULL == mKeyboard)
	{
		// *HACK to handle the fact that LLWindow subclasses have to initialize
		// things in an inconvenient order
		mKeyboard = gKeyboard;
	}

	KEY keycode = 0;
	if(evt.has("keycode"))
	{
		keycode = KEY(evt["keycode"].asInteger());
	}
	else
	{
		keycode = KEY(evt["char"].asString()[0]);
	}

	// *TODO - figure out how to handle the mask
	mKeyboard->handleTranslatedKeyDown(keycode, 0);
}

void LLWindowListener::mouseDown(LLSD const & evt)
{
	LLCoordGL pos(evt["x"].asInteger(), evt["y"].asInteger());

	std::string const & button = evt["button"].asString();

	if(button == "LEFT")
	{
		// *TODO - figure out how to handle the mask
		mWindow->handleMouseDown(NULL, pos, 0);
	}
	else if (button == "RIGHT")
	{
		// *TODO - figure out how to handle the mask
		mWindow->handleRightMouseDown(NULL, pos, 0);
	}
	else if (button == "MIDDLE")
	{
		// *TODO - figure out how to handle the mask
		mWindow->handleMiddleMouseDown(NULL, pos, 0);
	}
	else
	{
		llwarns << "ignoring unknown mous button \"" << button << '\"' << llendl;
	}
}

void LLWindowListener::mouseUp(LLSD const & evt)
{
	LLCoordGL pos(evt["x"].asInteger(), evt["y"].asInteger());

	std::string const & button = evt["button"].asString();

	if(button == "LEFT")
	{
		// *TODO - figure out how to handle the mask
		mWindow->handleMouseUp(NULL, pos, 0);
	}
	else if (button == "RIGHT")
	{
		// *TODO - figure out how to handle the mask
		mWindow->handleRightMouseUp(NULL, pos, 0);
	}
	else if (button == "MIDDLE")
	{
		// *TODO - figure out how to handle the mask
		mWindow->handleMiddleMouseUp(NULL, pos, 0);
	}
	else
	{
		llwarns << "ignoring unknown mous button \"" << button << '\"' << llendl;
	}
}

void LLWindowListener::mouseMove(LLSD const & evt)
{
	LLCoordGL pos(evt["x"].asInteger(), evt["y"].asInteger());

	// *TODO - figure out how to handle the mask
	mWindow->handleMouseMove(NULL, pos, 0);
}

void LLWindowListener::mouseScroll(LLSD const & evt)
{
	S32 clicks = evt["clicks"].asInteger();

	mWindow->handleScrollWheel(NULL, clicks);
}

