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
#include "llvoavatar.h"
#include "llcommandhandler.h"
#include "llslurl.h"
#include "llurldispatcher.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "lltoolgrab.h"
#include "llhudeffectlookat.h"
#include "llagentcamera.h"

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
		"[\"obj_uuid\"]: id of object to sit on, use this or [\"position\"] to indicate the sit target"
		"[\"position\"]: region position {x, y, z} where to find closest object to sit on",
        &LLAgentListener::requestSit);
    add("requestStand",
        "Ask to stand up",
        &LLAgentListener::requestStand);
    add("requestTouch",
		"[\"obj_uuid\"]: id of object to touch, use this or [\"position\"] to indicate the object to touch"
		"[\"position\"]: region position {x, y, z} where to find closest object to touch"
		"[\"face\"]: optional object face number to touch[Default: 0]",
        &LLAgentListener::requestTouch);
    add("resetAxes",
        "Set the agent to a fixed orientation (optionally specify [\"lookat\"] = array of [x, y, z])",
        &LLAgentListener::resetAxes);
    add("getAxes",
        "Obsolete - use getPosition instead\n"
        "Send information about the agent's orientation on [\"reply\"]:\n"
        "[\"euler\"]: map of {roll, pitch, yaw}\n"
        "[\"quat\"]:  array of [x, y, z, w] quaternion values",
        &LLAgentListener::getAxes,
        LLSDMap("reply", LLSD()));
    add("getPosition",
        "Send information about the agent's position and orientation on [\"reply\"]:\n"
        "[\"region\"]: array of region {x, y, z} position\n"
        "[\"global\"]: array of global {x, y, z} position\n"
        "[\"euler\"]: map of {roll, pitch, yaw}\n"
        "[\"quat\"]:  array of [x, y, z, w] quaternion values",
        &LLAgentListener::getPosition,
        LLSDMap("reply", LLSD()));
    add("startAutoPilot",
        "Start the autopilot system using the following parameters:\n"
        "[\"target_global\"]: array of target global {x, y, z} position\n"
        "[\"stop_distance\"]: target maxiumum distance from target [default: autopilot guess]\n"
        "[\"target_rotation\"]: array of [x, y, z, w] quaternion values [default: no target]\n"
        "[\"rotation_threshold\"]: target maximum angle from target facing rotation [default: 0.03 radians]\n"
        "[\"behavior_name\"]: name of the autopilot behavior [default: \"\"]"
        "[\"allow_flying\"]: allow flying during autopilot [default: True]",
        //"[\"callback_pump\"]: pump to send success/failure and callback data to [default: none]\n"
        //"[\"callback_data\"]: data to send back during a callback [default: none]",
        &LLAgentListener::startAutoPilot);
    add("getAutoPilot",
        "Send information about current state of the autopilot system to [\"reply\"]:\n"
        "[\"enabled\"]: boolean indicating whether or not autopilot is enabled\n"
        "[\"target_global\"]: array of target global {x, y, z} position\n"
        "[\"leader_id\"]: uuid of target autopilot is following\n"
        "[\"stop_distance\"]: target maximum distance from target\n"
        "[\"target_distance\"]: last known distance from target\n"
        "[\"use_rotation\"]: boolean indicating if autopilot has a target facing rotation\n"
        "[\"target_facing\"]: array of {x, y} target direction to face\n"
        "[\"rotation_threshold\"]: target maximum angle from target facing rotation\n"
        "[\"behavior_name\"]: name of the autopilot behavior",
        &LLAgentListener::getAutoPilot,
        LLSDMap("reply", LLSD()));
    add("startFollowPilot",
		"[\"leader_id\"]: uuid of target to follow using the autopilot system (optional with avatar_name)\n"
		"[\"avatar_name\"]: avatar name to follow using the autopilot system (optional with leader_id)\n"
        "[\"allow_flying\"]: allow flying during autopilot [default: True]\n"
        "[\"stop_distance\"]: target maxiumum distance from target [default: autopilot guess]",
        &LLAgentListener::startFollowPilot);
    add("setAutoPilotTarget",
        "Update target for currently running autopilot:\n"
        "[\"target_global\"]: array of target global {x, y, z} position",
        &LLAgentListener::setAutoPilotTarget);
    add("stopAutoPilot",
        "Stop the autopilot system:\n"
        "[\"user_cancel\"] indicates whether or not to act as though user canceled autopilot [default: false]",
        &LLAgentListener::stopAutoPilot);
    add("lookAt",
		"[\"type\"]: number to indicate the lookAt type, 0 to clear\n"
		"[\"obj_uuid\"]: id of object to look at, use this or [\"position\"] to indicate the target\n"
		"[\"position\"]: region position {x, y, z} where to find closest object or avatar to look at",
        &LLAgentListener::lookAt);
    add("getGroups",
        "Send information about the agent's groups on [\"reply\"]:\n"
        "[\"groups\"]: array of group information\n"
        "[\"id\"]: group id\n"
        "[\"name\"]: group name\n"
        "[\"insignia\"]: group insignia texture id\n"
        "[\"notices\"]: boolean indicating if this user accepts notices from this group\n"
        "[\"display\"]: boolean indicating if this group is listed in the user's profile\n"
        "[\"contrib\"]: user's land contribution to this group\n",
        &LLAgentListener::getGroups,
        LLSDMap("reply", LLSD()));
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

	LLViewerObject *object = NULL;
	if (event_data.has("obj_uuid"))
	{
		object = gObjectList.findObject(event_data["obj_uuid"]);
	}
	else if (event_data.has("position"))
	{
		LLVector3 target_position = ll_vector3_from_sd(event_data["position"]);
		object = findObjectClosestTo(target_position);
	}

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
	else
	{
		llwarns << "LLAgent requestSit could not find the sit target: " 
			<< event_data << llendl;
	}
}

void LLAgentListener::requestStand(LLSD const & event_data) const
{
    mAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
}


LLViewerObject * LLAgentListener::findObjectClosestTo( const LLVector3 & position ) const
{
	LLViewerObject *object = NULL;

	// Find the object closest to that position
	F32 min_distance = 10000.0f;		// Start big
	S32 num_objects = gObjectList.getNumObjects();
	S32 cur_index = 0;
	while (cur_index < num_objects)
	{
		LLViewerObject * cur_object = gObjectList.getObject(cur_index++);
		if (cur_object)
		{	// Calculate distance from the target position
			LLVector3 target_diff = cur_object->getPositionRegion() - position;
			F32 distance_to_target = target_diff.length();
			if (distance_to_target < min_distance)
			{	// Found an object closer
				min_distance = distance_to_target;
				object = cur_object;
			}
		}
	}

	return object;
}


void LLAgentListener::requestTouch(LLSD const & event_data) const
{
	LLViewerObject *object = NULL;
	
	if (event_data.has("obj_uuid"))
	{
		object = gObjectList.findObject(event_data["obj_uuid"]);
	}
	else if (event_data.has("position"))
	{
		LLVector3 target_position = ll_vector3_from_sd(event_data["position"]);
		object = findObjectClosestTo(target_position);
	}

	S32 face = 0;
	if (event_data.has("face"))
	{
		face = event_data["face"].asInteger();
	}

    if (object && object->getPCode() == LL_PCODE_VOLUME)
    {
		// Fake enough pick info to get it to (hopefully) work
		LLPickInfo pick;
		pick.mObjectFace = face;

		/*
		These values are sent to the simulator, but face seems to be easiest to use

		pick.mUVCoords	 "UVCoord"
		pick.mSTCoords	"STCoord"	
		pick.mObjectFace	"FaceIndex"
		pick.mIntersection	"Position"
		pick.mNormal	"Normal"
		pick.mBinormal	"Binormal"
		*/

		// A touch is a sketchy message sequence ... send a grab, immediately
		// followed by un-grabbing, crossing fingers and hoping packets arrive in
		// the correct order
		send_ObjectGrab_message(object, pick, LLVector3::zero);
		send_ObjectDeGrab_message(object, pick);
    }
	else
	{
		llwarns << "LLAgent requestTouch could not find the touch target " 
			<< event_data["obj_uuid"].asUUID() << llendl;
	}
}


void LLAgentListener::resetAxes(const LLSD& event_data) const
{
    if (event_data.has("lookat"))
    {
        mAgent.resetAxes(ll_vector3_from_sd(event_data["lookat"]));
    }
    else
    {
        // no "lookat", default call
        mAgent.resetAxes();
    }
}

void LLAgentListener::getAxes(const LLSD& event_data) const
{
    LLQuaternion quat(mAgent.getQuat());
    F32 roll, pitch, yaw;
    quat.getEulerAngles(&roll, &pitch, &yaw);
    // The official query API for LLQuaternion's [x, y, z, w] values is its
    // public member mQ...
	LLSD reply = LLSD::emptyMap();
	reply["quat"] = llsd_copy_array(boost::begin(quat.mQ), boost::end(quat.mQ));
	reply["euler"] = LLSD::emptyMap();
	reply["euler"]["roll"] = roll;
	reply["euler"]["pitch"] = pitch;
	reply["euler"]["yaw"] = yaw;
    sendReply(reply, event_data);
}

void LLAgentListener::getPosition(const LLSD& event_data) const
{
    F32 roll, pitch, yaw;
    LLQuaternion quat(mAgent.getQuat());
    quat.getEulerAngles(&roll, &pitch, &yaw);

	LLSD reply = LLSD::emptyMap();
	reply["quat"] = llsd_copy_array(boost::begin(quat.mQ), boost::end(quat.mQ));
	reply["euler"] = LLSD::emptyMap();
	reply["euler"]["roll"] = roll;
	reply["euler"]["pitch"] = pitch;
	reply["euler"]["yaw"] = yaw;
    reply["region"] = ll_sd_from_vector3(mAgent.getPositionAgent());
    reply["global"] = ll_sd_from_vector3d(mAgent.getPositionGlobal());

	sendReply(reply, event_data);
}


void LLAgentListener::startAutoPilot(LLSD const & event_data)
{
    LLQuaternion target_rotation_value;
    LLQuaternion* target_rotation = NULL;
    if (event_data.has("target_rotation"))
    {
        target_rotation_value = ll_quaternion_from_sd(event_data["target_rotation"]);
        target_rotation = &target_rotation_value;
    }
    // *TODO: Use callback_pump and callback_data
    F32 rotation_threshold = 0.03f;
    if (event_data.has("rotation_threshold"))
    {
        rotation_threshold = event_data["rotation_threshold"].asReal();
    }
	
	BOOL allow_flying = TRUE;
	if (event_data.has("allow_flying"))
	{
		allow_flying = (BOOL) event_data["allow_flying"].asBoolean();
		mAgent.setFlying(allow_flying);
	}

	F32 stop_distance = 0.f;
	if (event_data.has("stop_distance"))
	{
		stop_distance = event_data["stop_distance"].asReal();
	}

	// Clear follow target, this is doing a path
	mFollowTarget.setNull();

    mAgent.startAutoPilotGlobal(ll_vector3d_from_sd(event_data["target_global"]),
                                event_data["behavior_name"],
                                target_rotation,
                                NULL, NULL,
                                stop_distance,
                                rotation_threshold,
								allow_flying);
}

void LLAgentListener::getAutoPilot(const LLSD& event_data) const
{
	LLSD reply = LLSD::emptyMap();
	
	LLSD::Boolean enabled = mAgent.getAutoPilot();
	reply["enabled"] = enabled;
	
	reply["target_global"] = ll_sd_from_vector3d(mAgent.getAutoPilotTargetGlobal());
	
	reply["leader_id"] = mAgent.getAutoPilotLeaderID();
	
	reply["stop_distance"] = mAgent.getAutoPilotStopDistance();

	reply["target_distance"] = mAgent.getAutoPilotTargetDist();
	if (!enabled &&
		mFollowTarget.notNull())
	{	// Get an actual distance from the target object we were following
		LLViewerObject * target = gObjectList.findObject(mFollowTarget);
		if (target)
		{	// Found the target AV, return the actual distance to them as well as their ID
			LLVector3 difference = target->getPositionRegion() - mAgent.getPositionAgent();
			reply["target_distance"] = difference.length();
			reply["leader_id"] = mFollowTarget;
		}
	}

	reply["use_rotation"] = (LLSD::Boolean) mAgent.getAutoPilotUseRotation();
	reply["target_facing"] = ll_sd_from_vector3(mAgent.getAutoPilotTargetFacing());
	reply["rotation_threshold"] = mAgent.getAutoPilotRotationThreshold();
	reply["behavior_name"] = mAgent.getAutoPilotBehaviorName();
	reply["fly"] = (LLSD::Boolean) mAgent.getFlying();

	sendReply(reply, event_data);
}

void LLAgentListener::startFollowPilot(LLSD const & event_data)
{
	LLUUID target_id;

	BOOL allow_flying = TRUE;
	if (event_data.has("allow_flying"))
	{
		allow_flying = (BOOL) event_data["allow_flying"].asBoolean();
	}

	if (event_data.has("leader_id"))
	{
		target_id = event_data["leader_id"];
	}
	else if (event_data.has("avatar_name"))
	{	// Find the avatar with matching name
		std::string target_name = event_data["avatar_name"].asString();

		if (target_name.length() > 0)
		{
			S32 num_objects = gObjectList.getNumObjects();
			S32 cur_index = 0;
			while (cur_index < num_objects)
			{
				LLViewerObject * cur_object = gObjectList.getObject(cur_index++);
				if (cur_object &&
					cur_object->asAvatar() &&
					cur_object->asAvatar()->getFullname() == target_name)
				{	// Found avatar with matching name, extract id and break out of loop
					target_id = cur_object->getID();
					break;
				}
			}
		}
	}

	F32 stop_distance = 0.f;
	if (event_data.has("stop_distance"))
	{
		stop_distance = event_data["stop_distance"].asReal();
	}

	if (target_id.notNull())
	{
		mAgent.setFlying(allow_flying);
		mFollowTarget = target_id;	// Save follow target so we can report distance later

	    mAgent.startFollowPilot(target_id, allow_flying, stop_distance);
	}
}

void LLAgentListener::setAutoPilotTarget(LLSD const & event_data) const
{
	if (event_data.has("target_global"))
	{
		LLVector3d target_global(ll_vector3d_from_sd(event_data["target_global"]));
		mAgent.setAutoPilotTargetGlobal(target_global);
	}
}

void LLAgentListener::stopAutoPilot(LLSD const & event_data) const
{
	BOOL user_cancel = FALSE;
	if (event_data.has("user_cancel"))
	{
		user_cancel = event_data["user_cancel"].asBoolean();
	}
    mAgent.stopAutoPilot(user_cancel);
}

void LLAgentListener::lookAt(LLSD const & event_data) const
{
	LLViewerObject *object = NULL;
	if (event_data.has("obj_uuid"))
	{
		object = gObjectList.findObject(event_data["obj_uuid"]);
	}
	else if (event_data.has("position"))
	{
		LLVector3 target_position = ll_vector3_from_sd(event_data["position"]);
		object = findObjectClosestTo(target_position);
	}

	S32 look_at_type = (S32) LOOKAT_TARGET_NONE;
	if (event_data.has("type"))
	{
		look_at_type = event_data["type"].asInteger();
	}
	if (look_at_type >= (S32) LOOKAT_TARGET_NONE &&
		look_at_type < (S32) LOOKAT_NUM_TARGETS)
	{
		gAgentCamera.setLookAt((ELookAtType) look_at_type, object);
	}
}

void LLAgentListener::getGroups(const LLSD& event) const
{
    LLSD reply(LLSD::emptyArray());
    for (LLDynamicArray<LLGroupData>::const_iterator
             gi(mAgent.mGroups.begin()), gend(mAgent.mGroups.end());
         gi != gend; ++gi)
    {
        reply.append(LLSDMap
                     ("id", gi->mID)
                     ("name", gi->mName)
                     ("insignia", gi->mInsigniaID)
                     ("notices", bool(gi->mAcceptNotices))
                     ("display", bool(gi->mListInProfile))
                     ("contrib", gi->mContribution));
    }
    sendReply(LLSDMap("groups", reply), event);
}
