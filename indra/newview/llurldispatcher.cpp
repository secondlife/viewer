/**
 * @file llurldispatcher.cpp
 * @brief Central registry for all URL handlers
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"

#include "llurldispatcher.h"

// viewer includes
#include "llagent.h"			// teleportViaLocation()
#include "llcommandhandler.h"
#include "llfloaterhelpbrowser.h"
#include "llfloaterreg.h"
#include "llfloaterurldisplay.h"
#include "llfloaterworldmap.h"
#include "llpanellogin.h"
#include "llregionhandle.h"
#include "llsidetray.h"
#include "llslurl.h"
#include "llstartup.h"			// gStartupState
#include "llurlsimstring.h"
#include "llweb.h"
#include "llworldmapmessage.h"
#include "llurldispatcherlistener.h"

// library includes
#include "llnotificationsutil.h"
#include "llsd.h"

static LLURLDispatcherListener sURLDispatcherListener;

class LLURLDispatcherImpl
{
public:
	static bool dispatch(const std::string& url,
						 LLMediaCtrl* web,
						 bool trusted_browser);
		// returns true if handled or explicitly blocked.

	static bool dispatchRightClick(const std::string& url);

private:
	static bool dispatchCore(const std::string& url, 
							 bool right_mouse,
							 LLMediaCtrl* web,
							 bool trusted_browser);
		// handles both left and right click

	static bool dispatchHelp(const std::string& url, bool right_mouse);
		// Handles sl://app.floater.html.help by showing Help floater.
		// Returns true if handled.

	static bool dispatchApp(const std::string& url,
							bool right_mouse,
							LLMediaCtrl* web,
							bool trusted_browser);
		// Handles secondlife:///app/agent/<agent_id>/about and similar
		// by showing panel in Search floater.
		// Returns true if handled or explicitly blocked.

	static bool dispatchRegion(const std::string& url, bool right_mouse);
		// handles secondlife://Ahern/123/45/67/
		// Returns true if handled.

	static void regionHandleCallback(U64 handle, const std::string& url,
		const LLUUID& snapshot_id, bool teleport);
		// Called by LLWorldMap when a location has been resolved to a
	    // region name

	static void regionNameCallback(U64 handle, const std::string& url,
		const LLUUID& snapshot_id, bool teleport);
		// Called by LLWorldMap when a region name has been resolved to a
		// location in-world, used by places-panel display.

	friend class LLTeleportHandler;
};

// static
bool LLURLDispatcherImpl::dispatchCore(const std::string& url,
									   bool right_mouse,
									   LLMediaCtrl* web,
									   bool trusted_browser)
{
	if (url.empty()) return false;
	//if (dispatchHelp(url, right_mouse)) return true;
	if (dispatchApp(url, right_mouse, web, trusted_browser)) return true;
	if (dispatchRegion(url, right_mouse)) return true;

	/*
	// Inform the user we can't handle this
	std::map<std::string, std::string> args;
	args["SLURL"] = url;
	r;
	*/
	
	return false;
}

// static
bool LLURLDispatcherImpl::dispatch(const std::string& url,
								   LLMediaCtrl* web,
								   bool trusted_browser)
{
	llinfos << "url: " << url << llendl;
	const bool right_click = false;
	return dispatchCore(url, right_click, web, trusted_browser);
}

// static
bool LLURLDispatcherImpl::dispatchRightClick(const std::string& url)
{
	llinfos << "url: " << url << llendl;
	const bool right_click = true;
	LLMediaCtrl* web = NULL;
	const bool trusted_browser = false;
	return dispatchCore(url, right_click, web, trusted_browser);
}

// static
bool LLURLDispatcherImpl::dispatchApp(const std::string& url, 
									  bool right_mouse,
									  LLMediaCtrl* web,
									  bool trusted_browser)
{
	// ensure the URL is in the secondlife:///app/ format
	if (!LLSLURL::isSLURLCommand(url))
	{
		return false;
	}

	LLURI uri(url);
	LLSD pathArray = uri.pathArray();
	pathArray.erase(0); // erase "app"
	std::string cmd = pathArray.get(0);
	pathArray.erase(0); // erase "cmd"
	bool handled = LLCommandDispatcher::dispatch(
			cmd, pathArray, uri.queryMap(), web, trusted_browser);

	// alert if we didn't handle this secondlife:///app/ SLURL
	// (but still return true because it is a valid app SLURL)
	if (! handled)
	{
		LLNotificationsUtil::add("UnsupportedCommandSLURL");
	}
	return true;
}

// static
bool LLURLDispatcherImpl::dispatchRegion(const std::string& url, bool right_mouse)
{
	if (!LLSLURL::isSLURL(url))
	{
		return false;
	}

	// Before we're logged in, need to update the startup screen
	// to tell the user where they are going.
	if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
	{
		// Parse it and stash in globals, it will be dispatched in
		// STATE_CLEANUP.
		LLURLSimString::setString(url);
		// We're at the login screen, so make sure user can see
		// the login location box to know where they are going.
		
		LLPanelLogin::refreshLocation( true );
		return true;
	}

	std::string sim_string = LLSLURL::stripProtocol(url);
	std::string region_name;
	S32 x = 128;
	S32 y = 128;
	S32 z = 0;
	LLURLSimString::parse(sim_string, &region_name, &x, &y, &z);

	// LLFloaterURLDisplay functionality moved to LLPanelPlaces in Side Tray.
	//LLFloaterURLDisplay* url_displayp = LLFloaterReg::getTypedInstance<LLFloaterURLDisplay>("preview_url",LLSD());
	//if(url_displayp) url_displayp->setName(region_name);

	// Request a region handle by name
	LLWorldMapMessage::getInstance()->sendNamedRegionRequest(region_name,
									  LLURLDispatcherImpl::regionNameCallback,
									  url,
									  false);	// don't teleport
	return true;
}

/*static*/
void LLURLDispatcherImpl::regionNameCallback(U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport)
{
	std::string sim_string = LLSLURL::stripProtocol(url);
	std::string region_name;
	S32 x = 128;
	S32 y = 128;
	S32 z = 0;
	LLURLSimString::parse(sim_string, &region_name, &x, &y, &z);

	LLVector3 local_pos;
	local_pos.mV[VX] = (F32)x;
	local_pos.mV[VY] = (F32)y;
	local_pos.mV[VZ] = (F32)z;

	
	// determine whether the point is in this region
	if ((x >= 0) && (x < REGION_WIDTH_UNITS) &&
		(y >= 0) && (y < REGION_WIDTH_UNITS))
	{
		// if so, we're done
		regionHandleCallback(region_handle, url, snapshot_id, teleport);
	}

	else
	{
		// otherwise find the new region from the location
		
		// add the position to get the new region
		LLVector3d global_pos = from_region_handle(region_handle) + LLVector3d(local_pos);

		U64 new_region_handle = to_region_handle(global_pos);
		LLWorldMapMessage::getInstance()->sendHandleRegionRequest(new_region_handle,
										   LLURLDispatcherImpl::regionHandleCallback,
										   url, teleport);
	}
}

/* static */
void LLURLDispatcherImpl::regionHandleCallback(U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport)
{
	std::string sim_string = LLSLURL::stripProtocol(url);
	std::string region_name;
	S32 x = 128;
	S32 y = 128;
	S32 z = 0;
	LLURLSimString::parse(sim_string, &region_name, &x, &y, &z);

	// remap x and y to local coordinates
	S32 local_x = x % REGION_WIDTH_UNITS;
	S32 local_y = y % REGION_WIDTH_UNITS;
	if (local_x < 0)
		local_x += REGION_WIDTH_UNITS;
	if (local_y < 0)
		local_y += REGION_WIDTH_UNITS;
	
	LLVector3 local_pos;
	local_pos.mV[VX] = (F32)local_x;
	local_pos.mV[VY] = (F32)local_y;
	local_pos.mV[VZ] = (F32)z;

	LLVector3d global_pos = from_region_handle(region_handle);
	global_pos += LLVector3d(local_pos);
	
	if (teleport)
	{	
		gAgent.teleportViaLocation(global_pos);
		LLFloaterWorldMap* instance = LLFloaterWorldMap::getInstance();
		if(instance)
		{
			instance->trackLocation(global_pos);
		}
	}
	else
	{
		LLSD key;
		key["type"] = "remote_place";
		key["x"] = global_pos.mdV[VX];
		key["y"] = global_pos.mdV[VY];
		key["z"] = global_pos.mdV[VZ];

		LLSideTray::getInstance()->showPanel("panel_places", key);

		// LLFloaterURLDisplay functionality moved to LLPanelPlaces in Side Tray.

//		// display informational floater, allow user to click teleport btn
//		LLFloaterURLDisplay* url_displayp = LLFloaterReg::getTypedInstance<LLFloaterURLDisplay>("preview_url",LLSD());
//		if(url_displayp)
//		{
//			url_displayp->displayParcelInfo(region_handle, local_pos);
//			if(snapshot_id.notNull())
//			{
//				url_displayp->setSnapshotDisplay(snapshot_id);
//			}
//			std::string locationString = llformat("%s %d, %d, %d", region_name.c_str(), x, y, z);
//			url_displayp->setLocationString(locationString);
//		}
	}
}

//---------------------------------------------------------------------------
// Teleportation links are handled here because they are tightly coupled
// to URL parsing and sim-fragment parsing
class LLTeleportHandler : public LLCommandHandler
{
public:
	// Teleport requests *must* come from a trusted browser
	// inside the app, otherwise a malicious web page could
	// cause a constant teleport loop.  JC
	LLTeleportHandler() : LLCommandHandler("teleport", UNTRUSTED_BLOCK) { }

	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		// construct a "normal" SLURL, resolve the region to
		// a global position, and teleport to it
		if (tokens.size() < 1) return false;

		// Region names may be %20 escaped.
		std::string region_name = LLURLSimString::unescapeRegionName(tokens[0]);

		// build secondlife://De%20Haro/123/45/67 for use in callback
		std::string url = LLSLURL::PREFIX_SECONDLIFE;
		for (int i = 0; i < tokens.size(); ++i)
		{
			url += tokens[i].asString() + "/";
		}
		LLWorldMapMessage::getInstance()->sendNamedRegionRequest(region_name,
			LLURLDispatcherImpl::regionHandleCallback,
			url,
			true);	// teleport
		return true;
	}
};
LLTeleportHandler gTeleportHandler;

//---------------------------------------------------------------------------

// static
bool LLURLDispatcher::dispatch(const std::string& url,
							   LLMediaCtrl* web,
							   bool trusted_browser)
{
	return LLURLDispatcherImpl::dispatch(url, web, trusted_browser);
}

// static
bool LLURLDispatcher::dispatchRightClick(const std::string& url)
{
	return LLURLDispatcherImpl::dispatchRightClick(url);
}

// static
bool LLURLDispatcher::dispatchFromTextEditor(const std::string& url)
{
	// *NOTE: Text editors are considered sources of trusted URLs
	// in order to make avatar profile links in chat history work.
	// While a malicious resident could chat an app SLURL, the
	// receiving resident will see it and must affirmatively
	// click on it.
	// *TODO: Make this trust model more refined.  JC
	const bool trusted_browser = true;
	LLMediaCtrl* web = NULL;
	return LLURLDispatcherImpl::dispatch(url, web, trusted_browser);
}
