/**
 * @file   llgesturelistener.cpp
 * @author Dave Simmons
 * @date   2011-03-28
 * @brief  Implementation for LLGestureListener.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llgesturelistener.h"
#include "llgesturemgr.h"
#include "llmultigesture.h"


LLGestureListener::LLGestureListener()
  : LLEventAPI("LLGesture",
               "LLGesture listener interface to control gestures")
{
    add("getActiveGestures",
        "Return information about the agent's available gestures [\"reply\"]:\n"
        "[\"gestures\"]: a dictionary with UUID strings as keys\n"
		"  and the following dict values for each entry:\n"
		"     [\"name\"]: name of the gesture, may be empty\n"
		"     [\"trigger\"]: trigger string used to invoke via user chat, may be empty\n"
		"     [\"playing\"]: true or false indicating the playing state",
        &LLGestureListener::getActiveGestures,
        LLSDMap("reply", LLSD()));
	add("isGesturePlaying",
		"[\"id\"]: UUID of the gesture to query.  Returns True or False in [\"playing\"] value of the result",
        &LLGestureListener::isGesturePlaying);
	add("startGesture",
		"[\"id\"]: UUID of the gesture to start playing",
        &LLGestureListener::startGesture);
	add("stopGesture",
		"[\"id\"]: UUID of the gesture to stop",
        &LLGestureListener::stopGesture);
}


// "getActiveGestures" command
void LLGestureListener::getActiveGestures(const LLSD& event_data) const
{
	LLSD reply = LLSD::emptyMap();
	LLSD gesture_map = LLSD::emptyMap();

	const LLGestureMgr::item_map_t& active_gestures = LLGestureMgr::instance().getActiveGestures();

	// Scan active gesture map and get all the names
	LLGestureMgr::item_map_t::const_iterator it;
	for (it = active_gestures.begin(); it != active_gestures.end(); ++it)
	{
		LLMultiGesture* gesture = (*it).second;
		if (gesture)
		{	// Add an entry to the result map with the LLUUID as key with a map containing data
			LLSD info = LLSD::emptyMap();
			info["name"] = (LLSD::String) gesture->mName;
			info["trigger"] = (LLSD::String) gesture->mTrigger;
			info["playing"] = (LLSD::Boolean) gesture->mPlaying;

			gesture_map[(*it).first.asString()] = info;
		}
	}

	reply["gestures"] = gesture_map;
	sendReply(reply, event_data);
}



// "isGesturePlaying" command
void LLGestureListener::isGesturePlaying(const LLSD& event_data) const
{
	bool is_playing = false;
	if (event_data.has("id"))
	{
		LLUUID gesture_id = event_data["id"].asUUID();
		if (gesture_id.notNull())
		{
			is_playing = LLGestureMgr::instance().isGesturePlaying(gesture_id);
		}
		else
		{
			llwarns << "isGesturePlaying did not find a gesture object for " << gesture_id << llendl;
		}
	}
	else
	{
		llwarns << "isGesturePlaying didn't have 'id' value passed in" << llendl;
	}

	LLSD reply = LLSD::emptyMap();
	reply["playing"] = (LLSD::Boolean) is_playing;
	sendReply(reply, event_data);
}


// "startGesture" command
void LLGestureListener::startGesture(LLSD const & event_data) const
{
	startOrStopGesture(event_data, true);
}


// "stopGesture" command
void LLGestureListener::stopGesture(LLSD const & event_data) const
{
	startOrStopGesture(event_data, false);
}


// Real code for "startGesture" or "stopGesture"
void LLGestureListener::startOrStopGesture(LLSD const & event_data, bool start) const
{
	if (event_data.has("id"))
	{
		LLUUID gesture_id = event_data["id"].asUUID();
		if (gesture_id.notNull())
		{
			if (start)
			{
				LLGestureMgr::instance().playGesture(gesture_id);
			}
			else
			{
				LLGestureMgr::instance().stopGesture(gesture_id);
			}
		}
		else
		{
			llwarns << "startOrStopGesture did not find a gesture object for " << gesture_id << llendl;
		}
	}
	else
	{
		llwarns << "startOrStopGesture didn't have 'id' value passed in" << llendl;
	}
}

