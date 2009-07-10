/**
 * @file   llagentlistener.cpp
 * @author Brad Kittenbrink
 * @date   2009-07-10
 * @brief  Implementation for llagentlistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
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
  : LLDispatchListener("LLAgent", "op"),
    mAgent(agent)
{
	add("requestTeleport", &LLAgentListener::requestTeleport);
	add("requestSit", &LLAgentListener::requestSit);
	add("requestStand", &LLAgentListener::requestStand);
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
		LLCommandDispatcher::dispatch("teleport", params, LLSD(), NULL, true);
		// *TODO - lookup other LLCommandHandlers for "agent", "classified", "event", "group", "floater", "objectim", "parcel", "login", login_refresh", "balance", "chat"
		// should we just compose LLCommandHandler and LLDispatchListener?
	}
	else
	{
		std::string url = LLSLURL::buildSLURL(event_data["regionname"].asString(), event_data["x"].asReal(), event_data["y"].asReal(), event_data["z"].asReal());
		LLURLDispatcher::dispatch(url, NULL, false);
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

