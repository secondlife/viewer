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

#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

#include "llwindowlistener.h"

#include "llcoord.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"
#include "llwindowcallbacks.h"
#include "llui.h"
#include "llview.h"
#include "llviewinject.h"
#include "llviewerwindow.h"
#include "llviewerkeyboard.h"
#include "llrootview.h"
#include "llsdutil.h"
#include "stringize.h"
#include <typeinfo>
#include <map>
#include <boost/scoped_ptr.hpp>
#include <boost/lambda/core.hpp>
#include <boost/lambda/bind.hpp>

namespace bll = boost::lambda;

LLWindowListener::LLWindowListener(LLViewerWindow *window, const KeyboardGetter& kbgetter)
	: LLEventAPI("LLWindow", "Inject input events into the LLWindow instance"),
	  mWindow(window),
	  mKbGetter(kbgetter)
{
	std::string keySomething =
		"Given [\"keysym\"], [\"keycode\"] or [\"char\"], inject the specified ";
	std::string keyExplain =
		"(integer keycode values, or keysym string from any addKeyName() call in\n"
		"http://hg.secondlife.com/viewer-development/src/tip/indra/llwindow/llkeyboard.cpp )\n";
	std::string mask =
		"Specify optional [\"mask\"] as an array containing any of \"CTL\", \"ALT\",\n"
		"\"SHIFT\" or \"MAC_CONTROL\"; the corresponding modifier bits will be combined\n"
		"to form the mask used with the event.";

	std::string given = "Given ";
	std::string mouseParams =
		"optional [\"path\"], optional [\"x\"] and [\"y\"], inject the requested mouse ";
	std::string buttonParams =
		std::string("[\"button\"], ") + mouseParams;
	std::string buttonExplain =
		"(button values \"LEFT\", \"MIDDLE\", \"RIGHT\")\n";
	std::string paramsExplain =
		"[\"path\"] is as for LLUI::resolvePath(), described in\n"
		"http://hg.secondlife.com/viewer-development/src/tip/indra/llui/llui.h\n"
		"If you omit [\"path\"], you must specify both [\"x\"] and [\"y\"].\n"
		"If you specify [\"path\"] without both [\"x\"] and [\"y\"], will synthesize (x, y)\n"
		"in the center of the LLView selected by [\"path\"].\n"
		"You may specify [\"path\"] with both [\"x\"] and [\"y\"], will use your (x, y).\n"
		"This may cause the LLView selected by [\"path\"] to reject the event.\n"
		"Optional [\"reply\"] requests a reply event on the named LLEventPump.\n"
		"reply[\"error\"] isUndefined (None) on success, else an explanatory message.\n";

	add("getInfo",
		"Get information about the ui element specified by [\"path\"]",
		&LLWindowListener::getInfo,
		LLSDMap("reply", LLSD()));
	add("getPaths",
		"Send on [\"reply\"] an event in which [\"paths\"] is an array of valid LLView\n"
		"pathnames. Optional [\"under\"] pathname specifies the base node under which\n"
		"to list; all nodes from root if no [\"under\"].",
		&LLWindowListener::getPaths,
		LLSDMap("reply", LLSD()));
	add("keyDown",
		keySomething + "keypress event.\n" + keyExplain + mask,
		&LLWindowListener::keyDown);
	add("keyUp",
		keySomething + "key release event.\n" + keyExplain + mask,
		&LLWindowListener::keyUp);
	add("mouseDown",
		given + buttonParams + "click event.\n" + buttonExplain + paramsExplain + mask,
		&LLWindowListener::mouseDown);
	add("mouseUp",
		given + buttonParams + "release event.\n" + buttonExplain + paramsExplain + mask,
		&LLWindowListener::mouseUp);
	add("mouseMove",
		given + mouseParams + "movement event.\n" + paramsExplain + mask,
		&LLWindowListener::mouseMove);
	add("mouseScroll",
		"Given an integer number of [\"clicks\"], inject the requested mouse scroll event.\n"
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

namespace {

// helper for getMask()
MASK lookupMask_(const std::string& maskname)
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

MASK getMask(const LLSD& event)
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

KEY getKEY(const LLSD& event)
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

} // namespace

void LLWindowListener::getInfo(LLSD const & evt)
{
	Response response(LLSD(), evt);
	
	if (evt.has("path"))
	{
		std::string path(evt["path"]);
		LLView * target_view = LLUI::resolvePath(LLUI::getRootView(), path);
		if (target_view != 0)
		{
			response.setResponse(target_view->getInfo());
		}
		else 
		{
			response.error(STRINGIZE(evt["op"].asString() << " request "
											"specified invalid \"path\": '" << path << "'"));
		}
	}
	else 
	{
		response.error(
			STRINGIZE(evt["op"].asString() << "request did not provide a path" ));
	}
}

void LLWindowListener::getPaths(LLSD const & request)
{
	Response response(LLSD(), request);
	LLView *root(LLUI::getRootView()), *base(NULL);
	// Capturing request["under"] as string means we conflate the case in
	// which there is no ["under"] key with the case in which its value is the
	// empty string. That seems to make sense to me.
	std::string under(request["under"]);

	// Deal with optional "under" parameter
	if (under.empty())
	{
		base = root;
	}
	else
	{
		base = LLUI::resolvePath(root, under);
		if (! base)
		{
			return response.error(STRINGIZE(request["op"].asString() << " request "
											"specified invalid \"under\" path: '" << under << "'"));
		}
	}

	// Traverse the entire subtree under 'base', collecting pathnames
	for (LLView::tree_iterator_t ti(base->beginTreeDFS()), tend(base->endTreeDFS());
		 ti != tend; ++ti)
	{
		response["paths"].append((*ti)->getPathname());
	}
}

void LLWindowListener::keyDown(LLSD const & evt)
{
	Response response(LLSD(), evt);
	
	if (evt.has("path"))
	{
		std::string path(evt["path"]);
		LLView * target_view = LLUI::resolvePath(LLUI::getRootView(), path);
		if (target_view == 0) 
		{
			response.error(STRINGIZE(evt["op"].asString() << " request "
											"specified invalid \"path\": '" << path << "'"));
		}
		else if(target_view->isAvailable())
		{
			response.setResponse(target_view->getInfo());
			
			gFocusMgr.setKeyboardFocus(target_view);
			KEY key = getKEY(evt);
			MASK mask = getMask(evt);
			gViewerKeyboard.handleKey(key, mask, false);
			if(key < 0x80) mWindow->handleUnicodeChar(key, mask);
		}
		else 
		{
			response.error(STRINGIZE(evt["op"].asString() << " request "
											"element specified by \"path\": '" << path << "'" 
											<< " is not visible"));
		}
	}
	else 
	{
		mKbGetter()->handleTranslatedKeyDown(getKEY(evt), getMask(evt));
	}
}

void LLWindowListener::keyUp(LLSD const & evt)
{
	Response response(LLSD(), evt);

	if (evt.has("path"))
	{
		std::string path(evt["path"]);
		LLView * target_view = LLUI::resolvePath(LLUI::getRootView(), path);
		if (target_view == 0 )
		{
			response.error(STRINGIZE(evt["op"].asString() << " request "
											"specified invalid \"path\": '" << path << "'"));
		}
		else if (target_view->isAvailable())
		{
			response.setResponse(target_view->getInfo());

			gFocusMgr.setKeyboardFocus(target_view);
			mKbGetter()->handleTranslatedKeyUp(getKEY(evt), getMask(evt));
		}
		else 
		{
			response.error(STRINGIZE(evt["op"].asString() << " request "
											"element specified byt \"path\": '" << path << "'" 
											<< " is not visible"));
		}
	}
	else 
	{
		mKbGetter()->handleTranslatedKeyUp(getKEY(evt), getMask(evt));
	}
}

// for WhichButton
typedef BOOL (LLWindowCallbacks::*MouseMethod)(LLWindow *, LLCoordGL, MASK);
struct Actions
{
	Actions(const MouseMethod& d, const MouseMethod& u): down(d), up(u), valid(true) {}
	Actions(): valid(false) {}
	MouseMethod down, up;
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

typedef boost::function<bool(LLCoordGL, MASK)> MouseFunc;

static void mouseEvent(const MouseFunc& func, const LLSD& request)
{
	// Ensure we send response
	LLEventAPI::Response response(LLSD(), request);
	// We haven't yet established whether the incoming request has "x" and "y",
	// but capture this anyway, with 0 for omitted values.
	LLCoordGL pos(request["x"].asInteger(), request["y"].asInteger());
	bool has_pos(request.has("x") && request.has("y"));

	boost::scoped_ptr<LLView::TemporaryDrilldownFunc> tempfunc;

	// Documentation for mouseDown(), mouseUp() and mouseMove() claims you
	// must either specify ["path"], or both of ["x"] and ["y"]. You MAY
	// specify all. Let's say that passing "path" as an empty string is
	// equivalent to not passing it at all.
	std::string path(request["path"]);
	if (path.empty())
	{
		// Without "path", you must specify both "x" and "y".
		if (! has_pos)
		{
			return response.error(STRINGIZE(request["op"].asString() << " request "
											"without \"path\" must specify both \"x\" and \"y\": "
											<< request));
		}
	}
	else // ! path.empty()
	{
		LLView* root   = LLUI::getRootView();
		LLView* target = LLUI::resolvePath(root, path);
		if (! target)
		{
			return response.error(STRINGIZE(request["op"].asString() << " request "
											"specified invalid \"path\": '" << path << "'"));
		}

		response.setResponse(target->getInfo());

		// The intent of this test is to prevent trying to drill down to a
		// widget in a hidden floater, or on a tab that's not current, etc.
		if (! target->isInVisibleChain())
		{
			return response.error(STRINGIZE(request["op"].asString() << " request "
											"specified \"path\" not currently visible: '"
											<< path << "'"));
		}

		// This test isn't folded in with the above error case since you can
		// (e.g.) pop up a tooltip even for a disabled widget.
		if (! target->isInEnabledChain())
		{
			response.warn(STRINGIZE(request["op"].asString() << " request "
									"specified \"path\" not currently enabled: '"
									<< path << "'"));
		}

		if (! has_pos)
		{
			LLRect rect(target->calcScreenRect());
			pos.set(rect.getCenterX(), rect.getCenterY());
			// nonstandard warning tactic: probably usual case; we want event
			// sender to know synthesized (x, y), but maybe don't need to log?
			response["warnings"].append(STRINGIZE("using center point ("
												  << pos.mX << ", " << pos.mY << ")"));
		}

/*==========================================================================*|
		// NEVER MIND: the LLView tree defines priority handler layers in
		// front of the normal widget set, so this has never yet produced
		// anything but spam warnings. (sigh)

		// recursive childFromPoint() should give us the frontmost, leafmost
		// widget at the specified (x, y).
		LLView* frontmost = root->childFromPoint(pos.mX, pos.mY, true);
		if (frontmost != target)
		{
			response.warn(STRINGIZE(request["op"].asString() << " request "
									"specified \"path\" = '" << path
									<< "', but frontmost LLView at (" << pos.mX << ", " << pos.mY
									<< ") is '" << LLView::getPathname(frontmost) << "'"));
		}
|*==========================================================================*/

		// Instantiate a TemporaryDrilldownFunc to route incoming mouse events
		// to the target LLView*. But put it on the heap since "path" is
		// optional. Nonetheless, manage it with a boost::scoped_ptr so it
		// will be destroyed when we leave.
		tempfunc.reset(new LLView::TemporaryDrilldownFunc(llview::TargetEvent(target)));
	}

	// The question of whether the requested LLView actually handled the
	// specified event is important enough, and its handling unclear enough,
	// to warrant a separate response attribute. Instead of deciding here to
	// make it a warning, or an error, let caller decide.
	response["handled"] = func(pos, getMask(request));

	// On exiting this scope, response will send, tempfunc will restore the
	// normal pointInView(x, y) containment logic, etc.
}

void LLWindowListener::mouseDown(LLSD const & request)
{
	Actions actions(buttons.lookup(request["button"]));
	if (actions.valid)
	{
		// Normally you can pass NULL to an LLWindow* without compiler
		// complaint, but going through boost::lambda::bind() evidently
		// bypasses that special case: it only knows you're trying to pass an
		// int to a pointer. Explicitly cast NULL to the desired pointer type.
		mouseEvent(bll::bind(actions.down, mWindow,
							 static_cast<LLWindow*>(NULL), bll::_1, bll::_2),
				   request);
	}
}

void LLWindowListener::mouseUp(LLSD const & request)
{
	Actions actions(buttons.lookup(request["button"]));
	if (actions.valid)
	{
		mouseEvent(bll::bind(actions.up, mWindow,
							 static_cast<LLWindow*>(NULL), bll::_1, bll::_2),
				   request);
	}
}

void LLWindowListener::mouseMove(LLSD const & request)
{
	// We want to call the same central mouseEvent() routine for
	// handleMouseMove() as for button clicks. But handleMouseMove() returns
	// void, whereas mouseEvent() accepts a function returning bool -- and
	// uses that bool return. Use (void-lambda-expression, true) to construct
	// a callable that returns bool anyway. Pass 'true' because we expect that
	// our caller will usually treat 'false' as a problem.
	mouseEvent((bll::bind(&LLWindowCallbacks::handleMouseMove, mWindow,
						  static_cast<LLWindow*>(NULL), bll::_1, bll::_2),
				true),
			   request);
}

void LLWindowListener::mouseScroll(LLSD const & request)
{
	S32 clicks = request["clicks"].asInteger();

	mWindow->handleScrollWheel(NULL, clicks);
}
