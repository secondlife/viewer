/**
 * @file llurldispatcher.cpp
 * @brief Central registry for all URL handlers
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llurldispatcher.h"

// viewer includes
#include "llagent.h"			// teleportViaLocation()
#include "llcommandhandler.h"
#include "llfloaterhelpbrowser.h"
#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterworldmap.h"
#include "llpanellogin.h"
#include "llregionhandle.h"
#include "llslurl.h"
#include "llstartup.h"			// gStartupState
#include "llweb.h"
#include "llworldmapmessage.h"
#include "llurldispatcherlistener.h"
#include "llviewernetwork.h"

// library includes
#include "llnotificationsutil.h"
#include "llsd.h"

static LLURLDispatcherListener sURLDispatcherListener;

class LLURLDispatcherImpl
{
public:
	static bool dispatch(const LLSLURL& slurl,
						 const std::string& nav_type,
						 LLMediaCtrl* web,
						 bool trusted_browser);
		// returns true if handled or explicitly blocked.

	static bool dispatchRightClick(const LLSLURL& slurl);

private:
	static bool dispatchCore(const LLSLURL& slurl, 
							 const std::string& nav_type,
							 bool right_mouse,
							 LLMediaCtrl* web,
							 bool trusted_browser);
		// handles both left and right click

	static bool dispatchHelp(const LLSLURL& slurl, bool right_mouse);
		// Handles sl://app.floater.html.help by showing Help floater.
		// Returns true if handled.

	static bool dispatchApp(const LLSLURL& slurl,
							const std::string& nav_type,
							bool right_mouse,
							LLMediaCtrl* web,
							bool trusted_browser);
		// Handles secondlife:///app/agent/<agent_id>/about and similar
		// by showing panel in Search floater.
		// Returns true if handled or explicitly blocked.

	static bool dispatchRegion(const LLSLURL& slurl, const std::string& nav_type, bool right_mouse);
		// handles secondlife://Ahern/123/45/67/
		// Returns true if handled.

	static void regionHandleCallback(U64 handle, const LLSLURL& slurl,
		const LLUUID& snapshot_id, bool teleport);
		// Called by LLWorldMap when a location has been resolved to a
	    // region name

	static void regionNameCallback(U64 handle, const LLSLURL& slurl,
		const LLUUID& snapshot_id, bool teleport);
		// Called by LLWorldMap when a region name has been resolved to a
		// location in-world, used by places-panel display.

	friend class LLTeleportHandler;
};

// static
bool LLURLDispatcherImpl::dispatchCore(const LLSLURL& slurl,
									   const std::string& nav_type,
									   bool right_mouse,
									   LLMediaCtrl* web,
									   bool trusted_browser)
{
	//if (dispatchHelp(slurl, right_mouse)) return true;
	switch(slurl.getType())
	{
		case LLSLURL::APP: 
			return dispatchApp(slurl, nav_type, right_mouse, web, trusted_browser);
		case LLSLURL::LOCATION:
			return dispatchRegion(slurl, nav_type, right_mouse);
		default:
			return false;
	}

	/*
	// Inform the user we can't handle this
	std::map<std::string, std::string> args;
	args["SLURL"] = slurl;
	r;
	*/
}

// static
bool LLURLDispatcherImpl::dispatch(const LLSLURL& slurl,
								   const std::string& nav_type,
								   LLMediaCtrl* web,
								   bool trusted_browser)
{
	const bool right_click = false;
	return dispatchCore(slurl, nav_type, right_click, web, trusted_browser);
}

// static
bool LLURLDispatcherImpl::dispatchRightClick(const LLSLURL& slurl)
{
	const bool right_click = true;
	LLMediaCtrl* web = NULL;
	const bool trusted_browser = false;
	return dispatchCore(slurl, "clicked", right_click, web, trusted_browser);
}

// static
bool LLURLDispatcherImpl::dispatchApp(const LLSLURL& slurl, 
									  const std::string& nav_type,
									  bool right_mouse,
									  LLMediaCtrl* web,
									  bool trusted_browser)
{
	llinfos << "cmd: " << slurl.getAppCmd() << " path: " << slurl.getAppPath() << " query: " << slurl.getAppQuery() << llendl;
	const LLSD& query_map = LLURI::queryMap(slurl.getAppQuery());
	bool handled = LLCommandDispatcher::dispatch(
			slurl.getAppCmd(), slurl.getAppPath(), query_map, web, nav_type, trusted_browser);

	// alert if we didn't handle this secondlife:///app/ SLURL
	// (but still return true because it is a valid app SLURL)
	if (! handled)
	{
		LLNotificationsUtil::add("UnsupportedCommandSLURL");
	}
	return true;
}

// static
bool LLURLDispatcherImpl::dispatchRegion(const LLSLURL& slurl, const std::string& nav_type, bool right_mouse)
{
	if(slurl.getType() != LLSLURL::LOCATION)
    {
		return false;
    }
	// Before we're logged in, need to update the startup screen
	// to tell the user where they are going.
	if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
	{
		// We're at the login screen, so make sure user can see
		// the login location box to know where they are going.
		
		LLPanelLogin::setLocation(slurl);
		return true;
	}

	// Request a region handle by name
	LLWorldMapMessage::getInstance()->sendNamedRegionRequest(slurl.getRegion(),
									  LLURLDispatcherImpl::regionNameCallback,
									  slurl.getSLURLString(),
									  LLUI::sSettingGroups["config"]->getBOOL("SLURLTeleportDirectly"));	// don't teleport
	return true;
}

/*static*/
void LLURLDispatcherImpl::regionNameCallback(U64 region_handle, const LLSLURL& slurl, const LLUUID& snapshot_id, bool teleport)
{
      
  if(slurl.getType() == LLSLURL::LOCATION)
    {        
      regionHandleCallback(region_handle, slurl, snapshot_id, teleport);
    }
}

/* static */
void LLURLDispatcherImpl::regionHandleCallback(U64 region_handle, const LLSLURL& slurl, const LLUUID& snapshot_id, bool teleport)
{

  // we can't teleport cross grid at this point
	if(   LLGridManager::getInstance()->getGrid(slurl.getGrid())
	   != LLGridManager::getInstance()->getGrid())
	{
		LLSD args;
		args["SLURL"] = slurl.getLocationString();
		args["CURRENT_GRID"] = LLGridManager::getInstance()->getGridLabel();
		std::string grid_label = 
			LLGridManager::getInstance()->getGridLabel(slurl.getGrid());
		
		if(!grid_label.empty())
		{
			args["GRID"] = grid_label;
		}
		else 
		{
			args["GRID"] = slurl.getGrid();
		}
		LLNotificationsUtil::add("CantTeleportToGrid", args);
		return;
	}
	
	LLVector3d global_pos = from_region_handle(region_handle);
	global_pos += LLVector3d(slurl.getPosition());
	
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

		LLFloaterSidePanelContainer::showPanel("places", key);
	}
}

//---------------------------------------------------------------------------
// Teleportation links are handled here because they are tightly coupled
// to SLURL parsing and sim-fragment parsing
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

		LLVector3 coords(128, 128, 0);
		if (tokens.size() <= 4)
		{
			coords = LLVector3(tokens[1].asReal(), 
							   tokens[2].asReal(), 
							   tokens[3].asReal());
		}
		
		// Region names may be %20 escaped.
		
		std::string region_name = LLURI::unescape(tokens[0]);

		LLWorldMapMessage::getInstance()->sendNamedRegionRequest(region_name,
			LLURLDispatcherImpl::regionHandleCallback,
			LLSLURL(region_name, coords).getSLURLString(),
			true);	// teleport
		return true;
	}
};
LLTeleportHandler gTeleportHandler;

//---------------------------------------------------------------------------

// static
bool LLURLDispatcher::dispatch(const std::string& slurl,
							   const std::string& nav_type,
							   LLMediaCtrl* web,
							   bool trusted_browser)
{
	return LLURLDispatcherImpl::dispatch(LLSLURL(slurl), nav_type, web, trusted_browser);
}

// static
bool LLURLDispatcher::dispatchRightClick(const std::string& slurl)
{
	return LLURLDispatcherImpl::dispatchRightClick(LLSLURL(slurl));
}

// static
bool LLURLDispatcher::dispatchFromTextEditor(const std::string& slurl)
{
	// *NOTE: Text editors are considered sources of trusted URLs
	// in order to make avatar profile links in chat history work.
	// While a malicious resident could chat an app SLURL, the
	// receiving resident will see it and must affirmatively
	// click on it.
	// *TODO: Make this trust model more refined.  JC
	const bool trusted_browser = true;
	LLMediaCtrl* web = NULL;
	return LLURLDispatcherImpl::dispatch(LLSLURL(slurl), "clicked", web, trusted_browser);
}


