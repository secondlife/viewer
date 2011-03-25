/**
 * @file   llagentlistener.cpp
 * @author Brad Kittenbrink
 * @date   2009-07-10
 * @brief  Implementation for llagentlistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llagentlistener.h"

#include "llagent.h"
#include "llcommandhandler.h"
#include "llslurl.h"
#include "llurldispatcher.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"

LLAgentListener::LLAgentListener(LLAgent &agent)
  : LLEventAPI("LLAgent",
               "LLAgent listener to (e.g.) teleport, sit, stand, etc."),
    mAgent(agent)
{
	add("requestTeleport",
        "Teleport: [\"regionname\"], [\"x\"], [\"y\"], [\"z\"]\n"
        "If [\"skip_confirmation\"] is true, use LLURLDispatcher rather than LLCommandDispatcher.",
        &LLAgentListener::requestTeleport);
	add("requestSit",
        "Ask to sit on the object specified in [\"obj_uuid\"]",
        &LLAgentListener::requestSit);
	add("requestStand",
        "Ask to stand up",
        &LLAgentListener::requestStand);
}

void LLAgentListener::requestTeleport(LLSD const & event_data) const
{
	if(event_data["skip_confirmation"].asBoolean())
	{
		LLSD params(LLSD::emptyArray());
		params.append(event_data["regionname"]);
		params.append(event_data["x"]);
		params.append(event_data["y"]);
		params.append(event_data["z"]);
		LLCommandDispatcher::dispatch("teleport", params, LLSD(), NULL, "clicked", true);
		// *TODO - lookup other LLCommandHandlers for "agent", "classified", "event", "group", "floater", "parcel", "login", login_refresh", "balance", "chat"
		// should we just compose LLCommandHandler and LLDispatchListener?
	}
	else
	{
		std::string url = LLSLURL(event_data["regionname"], 
								  LLVector3(event_data["x"].asReal(), 
											event_data["y"].asReal(), 
											event_data["z"].asReal())).getSLURLString();
		LLURLDispatcher::dispatch(url, "clicked", NULL, false);
	}
}

void LLAgentListener::requestSit(LLSD const & event_data) const
{
	//mAgent.getAvatarObject()->sitOnObject();
	// shamelessly ripped from llviewermenu.cpp:handle_sit_or_stand()
	// *TODO - find a permanent place to share this code properly.
	LLViewerObject *object = gObjectList.findObject(event_data["obj_uuid"]);

	if (object && object->getPCode() == LL_PCODE_VOLUME)
	{
		gMessageSystem->newMessageFast(_PREHASH_AgentRequestSit);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, mAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, mAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_TargetObject);
		gMessageSystem->addUUIDFast(_PREHASH_TargetID, object->mID);
		gMessageSystem->addVector3Fast(_PREHASH_Offset, LLVector3(0,0,0));

		object->getRegion()->sendReliableMessage();
	}
}

void LLAgentListener::requestStand(LLSD const & event_data) const
{
	mAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
}

