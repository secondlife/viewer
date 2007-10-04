/**
 * @file llurldispatcher.cpp
 * @brief Central registry for all URL handlers
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llviewerprecompiledheaders.h"

#include "llurldispatcher.h"

// viewer includes
#include "llagent.h"			// teleportViaLocation()
#include "llcommandhandler.h"
// *FIX: code in merge sl-search-8
//#include "llfloaterurldisplay.h"
#include "llfloaterdirectory.h"
#include "llfloaterhtmlhelp.h"
#include "llfloaterworldmap.h"
#include "llpanellogin.h"
#include "llstartup.h"			// gStartupState
#include "llurlsimstring.h"
#include "llviewerwindow.h"		// alertXml()
#include "llworldmap.h"

// library includes
#include "llsd.h"

// system includes
#include <boost/tokenizer.hpp>

const std::string SLURL_SL_HELP_PREFIX		= "secondlife://app.";
const std::string SLURL_SL_PREFIX			= "sl://";
const std::string SLURL_SECONDLIFE_PREFIX	= "secondlife://";
const std::string SLURL_SLURL_PREFIX		= "http://slurl.com/secondlife/";

const std::string SLURL_APP_TOKEN = "app/";

class LLURLDispatcherImpl
{
public:
	static bool isSLURL(const std::string& url);

	static bool isSLURLCommand(const std::string& url);

	static bool dispatch(const std::string& url);
		// returns true if handled

	static bool dispatchRightClick(const std::string& url);

private:
	static bool dispatchCore(const std::string& url, bool right_mouse);
		// handles both left and right click

	static bool dispatchHelp(const std::string& url, BOOL right_mouse);
		// Handles sl://app.floater.html.help by showing Help floater.
		// Returns true if handled.

	static bool dispatchApp(const std::string& url, BOOL right_mouse);
		// Handles secondlife://app/agent/<agent_id>/about and similar
		// by showing panel in Search floater.
		// Returns true if handled.

	static bool dispatchRegion(const std::string& url, BOOL right_mouse);
		// handles secondlife://Ahern/123/45/67/
		// Returns true if handled.

// *FIX: code in merge sl-search-8
//	static void regionHandleCallback(U64 handle, const std::string& url,
//		const LLUUID& snapshot_id, bool teleport);
//		// Called by LLWorldMap when a region name has been resolved to a
//		// location in-world, used by places-panel display.

	static bool matchPrefix(const std::string& url, const std::string& prefix);
	
	static std::string stripProtocol(const std::string& url);

	friend class LLTeleportHandler;
};

// static
bool LLURLDispatcherImpl::isSLURL(const std::string& url)
{
	if (matchPrefix(url, SLURL_SL_HELP_PREFIX)) return true;
	if (matchPrefix(url, SLURL_SL_PREFIX)) return true;
	if (matchPrefix(url, SLURL_SECONDLIFE_PREFIX)) return true;
	if (matchPrefix(url, SLURL_SLURL_PREFIX)) return true;
	return false;
}

// static
bool LLURLDispatcherImpl::isSLURLCommand(const std::string& url)
{
	if (matchPrefix(url, SLURL_SL_PREFIX + SLURL_APP_TOKEN)
		|| matchPrefix(url, SLURL_SECONDLIFE_PREFIX + SLURL_APP_TOKEN)
		|| matchPrefix(url, SLURL_SLURL_PREFIX + SLURL_APP_TOKEN) )
	{
		return true;
	}
	return false;
}

// static
bool LLURLDispatcherImpl::dispatchCore(const std::string& url, bool right_mouse)
{
	if (url.empty()) return false;
	if (dispatchHelp(url, right_mouse)) return true;
	if (dispatchApp(url, right_mouse)) return true;
	if (dispatchRegion(url, right_mouse)) return true;
	return false;
}

// static
bool LLURLDispatcherImpl::dispatch(const std::string& url)
{
	llinfos << "url: " << url << llendl;
	return dispatchCore(url, false);	// not right click
}

// static
bool LLURLDispatcherImpl::dispatchRightClick(const std::string& url)
{
	llinfos << "url: " << url << llendl;
	return dispatchCore(url, true);	// yes right click
}
// static
bool LLURLDispatcherImpl::dispatchHelp(const std::string& url, BOOL right_mouse)
{
	if (matchPrefix(url, SLURL_SL_HELP_PREFIX))
	{
		gViewerHtmlHelp.show();
		return true;
	}
	return false;
}

// static
bool LLURLDispatcherImpl::dispatchApp(const std::string& url, BOOL right_mouse)
{
	if (!isSLURL(url))
	{
		return false;
	}
	std::string s = stripProtocol(url);

	// At this point, "secondlife://app/foo/bar/baz/" should be left
	// as: 	"app/foo/bar/baz/"
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("/", "", boost::drop_empty_tokens);
	tokenizer tokens(s, sep);
	tokenizer::iterator it = tokens.begin();
	tokenizer::iterator end = tokens.end();

	// Build parameter list suitable for LLDispatcher dispatch
	if (it == end) return false;
	if (*it != "app") return false;
	++it;

	if (it == end) return false;
	std::string cmd = *it;
	++it;

	std::vector<std::string> params;
	for ( ; it != end; ++it)
	{
		params.push_back(*it);
	}

	bool handled = LLCommandDispatcher::dispatch(cmd, params);
	if (handled) return true;

	// Inform the user we can't handle this
	std::map<std::string, std::string> args;
	args["[SLURL]"] = url;
	gViewerWindow->alertXml("BadURL", args);
	return false;
}

// static
bool LLURLDispatcherImpl::dispatchRegion(const std::string& url, BOOL right_mouse)
{
	if (!isSLURL(url))
	{
		return false;
	}

	// Before we're logged in, need to update the startup screen
	// to tell the user where they are going.
	if (LLStartUp::getStartupState() < STATE_CLEANUP)
	{
		// Parse it and stash in globals, it will be dispatched in
		// STATE_CLEANUP.
		LLURLSimString::setString(url);
		// We're at the login screen, so make sure user can see
		// the login location box to know where they are going.
		LLPanelLogin::refreshLocation( true );
		return true;
	}

	std::string sim_string = stripProtocol(url);
	std::string region_name;
	S32 x = 128;
	S32 y = 128;
	S32 z = 0;
	LLURLSimString::parse(sim_string, &region_name, &x, &y, &z);
	if (gFloaterWorldMap)
	{
		llinfos << "Opening map to " << region_name << llendl;
		gFloaterWorldMap->trackURL( region_name.c_str(), x, y, z );
		LLFloaterWorldMap::show(NULL, TRUE);
	}
	return true;
}

// static
bool LLURLDispatcherImpl::matchPrefix(const std::string& url, const std::string& prefix)
{
	std::string test_prefix = url.substr(0, prefix.length());
	LLString::toLower(test_prefix);
	return test_prefix == prefix;
}

// static
std::string LLURLDispatcherImpl::stripProtocol(const std::string& url)
{
	std::string stripped = url;
	if (matchPrefix(stripped, SLURL_SL_HELP_PREFIX))
	{
		stripped.erase(0, SLURL_SL_HELP_PREFIX.length());
	}
	else if (matchPrefix(stripped, SLURL_SL_PREFIX))
	{
		stripped.erase(0, SLURL_SL_PREFIX.length());
	}
	else if (matchPrefix(stripped, SLURL_SECONDLIFE_PREFIX))
	{
		stripped.erase(0, SLURL_SECONDLIFE_PREFIX.length());
	}
	else if (matchPrefix(stripped, SLURL_SLURL_PREFIX))
	{
		stripped.erase(0, SLURL_SLURL_PREFIX.length());
	}
	return stripped;
}

// *FIX: code in merge sl-search-8
//
////---------------------------------------------------------------------------
//// Teleportation links are handled here because they are tightly coupled
//// to URL parsing and sim-fragment parsing
//class LLTeleportHandler : public LLCommandHandler
//{
//public:
//	LLTeleportHandler() : LLCommandHandler("teleport") { }
//	bool handle(const std::vector<std::string>& tokens)
//	{
//		// construct a "normal" SLURL, resolve the region to
//		// a global position, and teleport to it
//		if (tokens.size() < 1) return false;
//
//		// Region names may be %20 escaped.
//		std::string region_name = LLURLSimString::unescapeRegionName(tokens[0]);
//
//		// build secondlife://De%20Haro/123/45/67 for use in callback
//		std::string url = SLURL_SECONDLIFE_PREFIX;
//		for (size_t i = 0; i < tokens.size(); ++i)
//		{
//			url += tokens[i] + "/";
//		}
//		gWorldMap->sendNamedRegionRequest(region_name,
//			LLURLDispatcherImpl::regionHandleCallback,
//			url,
//			true);	// teleport
//		return true;
//	}
//};
//LLTeleportHandler gTeleportHandler;

//---------------------------------------------------------------------------

// static
bool LLURLDispatcher::isSLURL(const std::string& url)
{
	return LLURLDispatcherImpl::isSLURL(url);
}

// static
bool LLURLDispatcher::isSLURLCommand(const std::string& url)
{
	return LLURLDispatcherImpl::isSLURLCommand(url);
}

// static
bool LLURLDispatcher::dispatch(const std::string& url)
{
	return LLURLDispatcherImpl::dispatch(url);
}
// static
bool LLURLDispatcher::dispatchRightClick(const std::string& url)
{
	return LLURLDispatcherImpl::dispatchRightClick(url);
}

