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
#include "llagent.h"
#include "llagentcamera.h"
#include "llphysicsmotion.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"

static LLAgentIORegistrar AgentIOReg;
static LLAgentIOModule* AgentIOModule;

void processAgentIOGetRequest(const LLSD& data)
{
    // Agent GET requests are processed here.
    // Expected data format:
    // data = 'command'
    // data = {command:get, data:[thing_one, thing_two, ...]}

    if (!data.has("data"))
    {
        LL_WARNS("AgentIO") << "malformed GET: map no 'data' key" << LL_ENDL;
        return;
    }
    
    const LLSD& payload = data["data"];
    if (!payload.isArray())
    {
        LL_WARNS("AgentIO") << "malformed GET: 'get' data not array" << LL_ENDL;
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

void processAgentIOSetRequest(const LLSD& data)
{
    // Agent SET requests are processed here.
    // Expected data format:
    // data = 'command'
    // data = {command:set, data:[thing_one, thing_two, ...]}

    if (!data.has("data"))
    {
        LL_WARNS("AgentIO") << "malformed SET: map no 'data' key" << LL_ENDL;
        return;
    }
    
    const LLSD& payload = data["data"];
    if (!payload.isArray())
    {
        LL_WARNS("AgentIO") << "malformed SET: 'data' not array" << LL_ENDL;
        return;
    }

    for (auto itr = payload.beginArray(); itr != payload.endArray(); ++itr)
    {
        std::string key = itr->asString();
        if ( *itr == "c" || *itr == "camera" )
        {
            AgentIOModule->setCamera(data["data"]);
        }
    }
}

void LLAgentIOModule::setCamera(const LLSD& data)
{
    //Camera and target position are expected as
    //world-space coordinates in the viewer-standard
    //coordinate frame.
    if (data.has("camera") && data.has("target"))
    {
        LLUUID target_id = LLUUID::null;
        
        if (data.has("target_id"))
        {
            target_id = data["target_id"].asUUID();
        }
        
        gAgentCamera.setCameraPosAndFocusGlobal(
            ll_vector3d_from_sd ( data["camera"] ),
            ll_vector3d_from_sd ( data["target"] ),
            target_id);
    }
}

void LLAgentIOModule::sendAgentOrientation()
{
    /*Find the agent's position and rotation in world and send it out*/
    LLVector3 pos = LLVector3(gAgent.getPositionGlobal());
    LLQuaternion rot = gAgent.getFrameAgent().getQuaternion();
    
    LLSD dat = LLSD::emptyMap();
    dat["position"] = ll_sd_from_vector3(pos);
    dat["rotation"] = ll_sd_from_quaternion(rot);

    sendCommand("agent_orientation", dat);
}

void LLAgentIOModule::sendCameraOrientation()
{
    /*Send camera position and facing relative the agent's
     position and facing.*/
    LLVector3 camera_pos= gAgentCamera.getCurrentCameraOffset();
    LLVector3 target_pos = LLVector3(gAgentCamera.getCurrentFocusOffset());
    
    LLSD dat = LLSD::emptyMap();
    dat["camera"] = ll_sd_from_vector3(camera_pos);
    dat["target"] = ll_sd_from_vector3(target_pos);

    sendCommand("viewer_camera", dat);
}

void LLAgentIOModule::sendLookAt()
{
    //Send what the agent is looking at to the leap module.
    //If the agent isn't looking at anything, sends empty map
    //Otherwise, sends the direction relative the avatar's
    //facing and the distance to the look target.
    LLPhysicsMotionController::ptr_t motion(std::static_pointer_cast<LLPhysicsMotionController>(gAgentAvatarp->findMotion(ANIM_AGENT_PHYSICS_MOTION)));
    
    if (!motion)
    {
        LL_WARNS("AgentIO") << "Agent has no physics motion" << LL_ENDL;
        return;
    }
        
    LLVector3* targetPos = (LLVector3*)motion->getCharacter()->getAnimationData("LookAtPoint");

    LLSD dat = LLSD::emptyMap();

    if (targetPos)
    {
        LLVector3 headLookAt = *targetPos;
        
        dat["direction"] = ll_sd_from_vector3(headLookAt);
        dat["distance"] = (F32)headLookAt.normVec();
    }

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
		"A plugin module has requested information from the viewer\n"
		"Requested data may be a simple string.  EX:\n"
		"  camera\n"
		"  look_at\n"
		"  position\n"
		"Or a key and dict"
		"Response will be a set issued to the plugin module. EX:\n"
		"  camera_id: <integer>\n"
		"  skeleton: <llsd>\n"
		"multiple items may be requested in a single get",
		&processAgentIOGetRequest);
    add("set",
        "A plugin module wishes to set agent data in the viewer",
        &processAgentIOSetRequest);

	//Example of defining viewer-internal API endpoints for this event handler if we wanted the viewer to trigger an update.
    /*mPlugin = LLEventPumps::instance().obtain("look_at").listen(
                    "LLAgentIOModule",
                    [](const LLSD& unused)
                   {
                        LLAgentIOModule::instance().sendLookAt();
                        return false;
                    });*/
}
