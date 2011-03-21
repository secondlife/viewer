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
		"(integer keycode values, or keysym \"XXXX\" from any KEY_XXXX, in\n"
		"http://hg.secondlife.com/viewer-development/src/tip/indra/llcommon/indra_constants.h )";
	std::string mask =
		"Specify optional [\"mask\"] as an array containing any of \"CONTROL\", \"ALT\",\n"
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

// for WhichKeysym. KeyProxy is like the typedef KEY, except that KeyProxy()
// (default-constructed) is guaranteed to have the value KEY_NONE.
class KeyProxy
{
public:
	KeyProxy(KEY k): mKey(k) {}
	KeyProxy(): mKey(KEY_NONE) {}
	operator KEY() const { return mKey; }

private:
	KEY mKey;
};

struct WhichKeysym: public StringLookup<KeyProxy>
{
	WhichKeysym(): StringLookup<KeyProxy>("keysym")
	{
		add("RETURN",		KEY_RETURN);
		add("LEFT",			KEY_LEFT);
		add("RIGHT",		KEY_RIGHT);
		add("UP",			KEY_UP);
		add("DOWN",			KEY_DOWN);
		add("ESCAPE",		KEY_ESCAPE);
		add("BACKSPACE",	KEY_BACKSPACE);
		add("DELETE",		KEY_DELETE);
		add("SHIFT",		KEY_SHIFT);
		add("CONTROL",		KEY_CONTROL);
		add("ALT",			KEY_ALT);
		add("HOME",			KEY_HOME);
		add("END",			KEY_END);
		add("PAGE_UP",		KEY_PAGE_UP);
		add("PAGE_DOWN",	KEY_PAGE_DOWN);
		add("HYPHEN",		KEY_HYPHEN);
		add("EQUALS",		KEY_EQUALS);
		add("INSERT",		KEY_INSERT);
		add("CAPSLOCK",		KEY_CAPSLOCK);
		add("TAB",			KEY_TAB);
		add("ADD",			KEY_ADD);
		add("SUBTRACT",		KEY_SUBTRACT);
		add("MULTIPLY",		KEY_MULTIPLY);
		add("DIVIDE",		KEY_DIVIDE);
		add("F1",			KEY_F1);
		add("F2",			KEY_F2);
		add("F3",			KEY_F3);
		add("F4",			KEY_F4);
		add("F5",			KEY_F5);
		add("F6",			KEY_F6);
		add("F7",			KEY_F7);
		add("F8",			KEY_F8);
		add("F9",			KEY_F9);
		add("F10",			KEY_F10);
		add("F11",			KEY_F11);
		add("F12",			KEY_F12);

		add("PAD_UP",		KEY_PAD_UP);
		add("PAD_DOWN",		KEY_PAD_DOWN);
		add("PAD_LEFT",		KEY_PAD_LEFT);
		add("PAD_RIGHT",	KEY_PAD_RIGHT);
		add("PAD_HOME",		KEY_PAD_HOME);
		add("PAD_END",		KEY_PAD_END);
		add("PAD_PGUP",		KEY_PAD_PGUP);
		add("PAD_PGDN",		KEY_PAD_PGDN);
		add("PAD_CENTER",	KEY_PAD_CENTER); // the 5 in the middle
		add("PAD_INS",		KEY_PAD_INS);
		add("PAD_DEL",		KEY_PAD_DEL);
		add("PAD_RETURN",	KEY_PAD_RETURN);
		add("PAD_ADD",		KEY_PAD_ADD); // not used
		add("PAD_SUBTRACT", KEY_PAD_SUBTRACT); // not used
		add("PAD_MULTIPLY", KEY_PAD_MULTIPLY); // not used
		add("PAD_DIVIDE",	KEY_PAD_DIVIDE); // not used

		add("BUTTON0",		KEY_BUTTON0);
		add("BUTTON1",		KEY_BUTTON1);
		add("BUTTON2",		KEY_BUTTON2);
		add("BUTTON3",		KEY_BUTTON3);
		add("BUTTON4",		KEY_BUTTON4);
		add("BUTTON5",		KEY_BUTTON5);
		add("BUTTON6",		KEY_BUTTON6);
		add("BUTTON7",		KEY_BUTTON7);
		add("BUTTON8",		KEY_BUTTON8);
		add("BUTTON9",		KEY_BUTTON9);
		add("BUTTON10",		KEY_BUTTON10);
		add("BUTTON11",		KEY_BUTTON11);
		add("BUTTON12",		KEY_BUTTON12);
		add("BUTTON13",		KEY_BUTTON13);
		add("BUTTON14",		KEY_BUTTON14);
		add("BUTTON15",		KEY_BUTTON15);
	}
};
static WhichKeysym keysyms;

struct WhichMask: public StringLookup<MASK>
{
	WhichMask(): StringLookup<MASK>("shift mask")
	{
		add("NONE",			MASK_NONE);
		add("CONTROL",		MASK_CONTROL); // Mapped to cmd on Macs
		add("ALT",			MASK_ALT);
		add("SHIFT",		MASK_SHIFT);
		add("MAC_CONTROL",	MASK_MAC_CONTROL); // Un-mapped Ctrl key on Macs, not used on Windows
	}
};
static WhichMask masks;

static MASK getMask(const LLSD& event)
{
	MASK mask(MASK_NONE);
	LLSD masknames(event["mask"]);
	for (LLSD::array_const_iterator ai(masknames.beginArray()), aend(masknames.endArray());
		 ai != aend; ++ai)
	{
		mask |= masks.lookup(*ai);
	}
	return mask;
}

static KEY getKEY(const LLSD& event)
{
    if (event.has("keysym"))
	{
		return keysyms.lookup(event["keysym"]);
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

