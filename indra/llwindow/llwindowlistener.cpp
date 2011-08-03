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
#include <map>

LLWindowListener::LLWindowListener(LLWindowCallbacks *window, const KeyboardGetter& kbgetter)
	: LLEventAPI("LLWindow", "Inject input events into the LLWindow instance"),
	  mWindow(window),
	  mKbGetter(kbgetter)
{
	std::string keySomething =
		"Given [\"keysym\"], [\"keycode\"] or [\"char\"], inject the specified ";
	std::string keyExplain =
		"(integer keycode values, or keysym string from any addKeyName() call in\n"
		"http://hg.secondlife.com/viewer-development/src/tip/indra/llwindow/llkeyboard.cpp )";
	std::string mask =
		"Specify optional [\"mask\"] as an array containing any of \"CTL\", \"ALT\",\n"
		"\"SHIFT\" or \"MAC_CONTROL\"; the corresponding modifier bits will be combined\n"
		"to form the mask used with the event.";

	std::string mouseSomething =
		"Given [\"button\"], [\"x\"] and [\"y\"], inject the given mouse ";
	std::string mouseExplain =
		"(button values \"LEFT\", \"MIDDLE\", \"RIGHT\")";

	add("keyDown",
		keySomething + "keypress event.\n" + keyExplain + '\n' + mask,
		&LLWindowListener::keyDown);
	add("keyUp",
		keySomething + "key release event.\n" + keyExplain + '\n' + mask,
		&LLWindowListener::keyUp);
	add("mouseDown",
		mouseSomething + "click event.\n" + mouseExplain + '\n' + mask,
		&LLWindowListener::mouseDown);
	add("mouseUp",
		mouseSomething + "release event.\n" + mouseExplain + '\n' + mask,
		&LLWindowListener::mouseUp);
	add("mouseMove",
		std::string("Given [\"x\"] and [\"y\"], inject the given mouse movement event.\n") +
		mask,
		&LLWindowListener::mouseMove);
	add("mouseScroll",
		"Given an integer number of [\"clicks\"], inject the given mouse scroll event.\n"
		"(positive clicks moves downward through typical content)",
		&LLWindowListener::mouseScroll);
}

template <typename MAPPED>
class StringLookup
{
private:
	std::string mDesc;
	typedef std::map<std::string, MAPPED> Map;
	Map mMap;

public:
	StringLookup(const std::string& desc): mDesc(desc) {}

	MAPPED lookup(const typename Map::key_type& key) const
	{
		typename Map::const_iterator found = mMap.find(key);
		if (found == mMap.end())
		{
			LL_WARNS("LLWindowListener") << "Unknown " << mDesc << " '" << key << "'" << LL_ENDL;
			return MAPPED();
		}
		return found->second;
	}

protected:
	void add(const typename Map::key_type& key, const typename Map::mapped_type& value)
	{
		mMap.insert(typename Map::value_type(key, value));
	}
};

// helper for getMask()
static MASK lookupMask_(const std::string& maskname)
{
	// It's unclear to me whether MASK_MAC_CONTROL is important, but it's not
	// supported by maskFromString(). Handle that specially.
	if (maskname == "MAC_CONTROL")
	{
		return MASK_MAC_CONTROL;
	}
	else
	{
		// In case of lookup failure, return MASK_NONE, which won't affect our
		// caller's OR.
		MASK mask(MASK_NONE);
		LLKeyboard::maskFromString(maskname, &mask);
		return mask;
	}
}

static MASK getMask(const LLSD& event)
{
	LLSD masknames(event["mask"]);
	if (! masknames.isArray())
	{
		// If event["mask"] is a single string, perform normal lookup on it.
		return lookupMask_(masknames);
	}

	// Here event["mask"] is an array of mask-name strings. OR together their
	// corresponding bits.
	MASK mask(MASK_NONE);
	for (LLSD::array_const_iterator ai(masknames.beginArray()), aend(masknames.endArray());
		 ai != aend; ++ai)
	{
		mask |= lookupMask_(*ai);
	}
	return mask;
}

static KEY getKEY(const LLSD& event)
{
    if (event.has("keysym"))
	{
		// Initialize to KEY_NONE; that way we can ignore the bool return from
		// keyFromString() and, in the lookup-fail case, simply return KEY_NONE.
		KEY key(KEY_NONE);
		LLKeyboard::keyFromString(event["keysym"], &key);
		return key;
	}
	else if (event.has("keycode"))
	{
		return KEY(event["keycode"].asInteger());
	}
	else
	{
		return KEY(event["char"].asString()[0]);
	}
}

void LLWindowListener::keyDown(LLSD const & evt)
{
	mKbGetter()->handleTranslatedKeyDown(getKEY(evt), getMask(evt));
}

void LLWindowListener::keyUp(LLSD const & evt)
{
	mKbGetter()->handleTranslatedKeyUp(getKEY(evt), getMask(evt));
}

// for WhichButton
typedef BOOL (LLWindowCallbacks::*MouseFunc)(LLWindow *, LLCoordGL, MASK);
struct Actions
{
	Actions(const MouseFunc& d, const MouseFunc& u): down(d), up(u), valid(true) {}
	Actions(): valid(false) {}
	MouseFunc down, up;
	bool valid;
};

struct WhichButton: public StringLookup<Actions>
{
	WhichButton(): StringLookup<Actions>("mouse button")
	{
		add("LEFT",		Actions(&LLWindowCallbacks::handleMouseDown,
								&LLWindowCallbacks::handleMouseUp));
		add("RIGHT",	Actions(&LLWindowCallbacks::handleRightMouseDown,
								&LLWindowCallbacks::handleRightMouseUp));
		add("MIDDLE",	Actions(&LLWindowCallbacks::handleMiddleMouseDown,
								&LLWindowCallbacks::handleMiddleMouseUp));
	}
};
static WhichButton buttons;

static LLCoordGL getPos(const LLSD& event)
{
	return LLCoordGL(event["x"].asInteger(), event["y"].asInteger());
}

void LLWindowListener::mouseDown(LLSD const & evt)
{
	Actions actions(buttons.lookup(evt["button"]));
	if (actions.valid)
	{
		(mWindow->*(actions.down))(NULL, getPos(evt), getMask(evt));
	}
}

void LLWindowListener::mouseUp(LLSD const & evt)
{
	Actions actions(buttons.lookup(evt["button"]));
	if (actions.valid)
	{
		(mWindow->*(actions.up))(NULL, getPos(evt), getMask(evt));
	}
}

void LLWindowListener::mouseMove(LLSD const & evt)
{
	mWindow->handleMouseMove(NULL, getPos(evt), getMask(evt));
}

void LLWindowListener::mouseScroll(LLSD const & evt)
{
	S32 clicks = evt["clicks"].asInteger();

	mWindow->handleScrollWheel(NULL, clicks);
}

