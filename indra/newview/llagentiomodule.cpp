/**
 * @file llagentiomodule.cpp
 * @brief Implementation of LLAgentIOModule class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llviewerprecompiledheaders.h"

#include "llagentiomodule.h"

#include <boost/bind.hpp>

#include "httpcommon.h"

#include "llagent.h"
#include "llevents.h"
#include "llleap.h"
#include "llsdutil.h"
#include "llsdutil_math.h"

static LLAgentIORegistrar AgentIOReg;
static LLAgentIOModule* AgentIOModule;

void processAgentIOGetRequest(const LLSD& data)
{
    // Agent GET requests are processed here.
    // Expected data format:
    // data = 'command'
    // data = {command:get, get:[thing_one, thing_two, ...]}
    // data = {command:get, g:[thing_one, thing_two, ...]}

    // always check for short format first...
    LL_INFOS("SPATTERS") << "Got to here.  Sweet!" << LL_ENDL;

    if (!data.has("data"))
    {
        LL_WARNS("AgentIO") << "malformed GET: map no 'get' key" << LL_ENDL;
        return;
    }
    
    const LLSD& payload = data["data"];
    if (!payload.isArray())
    {
        LL_WARNS("AgentIO") << "malformed GET: 'get' value not array" << LL_ENDL;
        return;
    }

    for (auto itr = payload.beginArray(); itr != payload.endArray(); ++itr)
    {
        std::string key = itr->asString();
        if ( *itr == "c" || *itr == "camera" )
        {
            //TODO:  Return camera position and target
            //LLAgentIOModule::instance().getCameraNumber_(data);
        }
        else if ( *itr == "l" || *itr == "look_at" )
        {
            //TODO:  Return lookat target ...  What to do if no target?
            AgentIOModule->sendLookAt();
        }
        else if ( *itr == "a" || *itr == "agent" )
        {
            //TODO:  Return avatar position and rotation in world space.
        }
        else if ( *itr == "t" || *itr == "touch_target" )
        {
            //TODO:  Return touch-target or a none type answer.
        }
    }
}

void LLAgentIOModule::sendLookAt()
{
    LLSD dat = LLSD::emptyMap();
    LLVector3 direction(1.0f, 0.3f, 0.4f);
    dat["SPATTERS"]="it me";
    dat["direction"]=ll_sd_from_vector3(direction);
    sendCommand("look_at", dat);
}

LLAgentIOModule::LLAgentIOModule(const LL::LazyEventAPIParams& params) :
    LLEventAPI(params),
    mLeapModule()
{
    //NOTE: This debug line ensures the module gets linked.
    LL_DEBUGS("LeapAgentIO") << "Initialized." << LL_ENDL;
    AgentIOModule = this;
}


void LLAgentIOModule::setLeapModule(std::weak_ptr<LLLeap> mod, const std::string & module_name)
{
    mLeapModule = mod;
    mModuleName = module_name;
};


LLAgentIOModule::module_ptr_t LLAgentIOModule::getLeapModule() const
{
    // lock it
    return mLeapModule.lock();
}

void LLAgentIOModule::sendCommand(const std::string& command, const LLSD& args) const
{
    /*module_ptr_t mod = getLeapModule();
    if (mod)
    {*/
        LLSD data;
        data["command"] = command;
        // args is optional
        if (args.isDefined())
        {
            data["args"] = args;
        }
        LL_DEBUGS("AgentIO") << "Posting " << command << " to Leap module" << LL_ENDL;
        LLEventPumps::instance().post("agentio.controller", data);
/*    }
    else
    {
        LL_WARNS("AgentIO") << "AgentIO module not loaded, dropping " << command << " command" << LL_ENDL;
    }
 */
}

LLAgentIORegistrar::LLAgentIORegistrar() :
	LazyEventAPI("agentio",
		"Integrate external agentio control module",
		// dispatch incoming events on "command" key
		"command")
{
	//This section defines the external API targets for this event handler, created with the add routine.
	add("get",
		"Puppetry plugin module has requested information from the viewer\n"
		"Requested data may be a simple string.  EX:\n"
		"  camera\n"
		"  look_at\n"
		"  position\n"
		"  touch_target\n"
		"Or a key and dict"
		"Response will be a set issued to the plugin module. EX:\n"
		"  camera_id: <integer>\n"
		"  skeleton: <llsd>\n"
		"multiple items may be requested in a single get",
		&processAgentIOGetRequest);

	//This function defines viewer-internal API endpoints for this event handler.
    /*mPlugin = LLEventPumps::instance().obtain("look_at").listen(
                    "LLAgentIOModule",
                    [](const LLSD& unused)
                   {
                        LLAgentIOModule::instance().sendLookAt();
                        return false;
                    });*/
}
