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
#include "llagentcamera.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llvoavatar.h"
#include "llcommandhandler.h"
#include "llinventorymodel.h"
#include "llslurl.h"
#include "llurldispatcher.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "lltoolgrab.h"
#include "llhudeffectlookat.h"
#include "llviewercamera.h"
#include "resultset.h"
#include <functional>

static const F64 PLAY_ANIM_THROTTLE_PERIOD = 1.f;

LLAgentListener::LLAgentListener(LLAgent &agent)
  : LLEventAPI("LLAgent",
               "LLAgent listener to (e.g.) teleport, sit, stand, etc."),
    mPlayAnimThrottle("playAnimation", &LLAgentListener::playAnimation_, this, PLAY_ANIM_THROTTLE_PERIOD),
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
        "[\"stop_distance\"]: maximum stop distance from target [default: autopilot guess]\n"
        "[\"target_rotation\"]: array of [x, y, z, w] quaternion values [default: no target]\n"
        "[\"rotation_threshold\"]: target maximum angle from target facing rotation [default: 0.03 radians]\n"
        "[\"behavior_name\"]: name of the autopilot behavior [default: \"\"]\n"
        "[\"allow_flying\"]: allow flying during autopilot [default: True]\n"
        "event with [\"success\"] flag is sent to 'LLAutopilot' event pump, when auto pilot is terminated",
        &LLAgentListener::startAutoPilot,
        llsd::map("target_global", LLSD()));
    add("getAutoPilot",
        "Send information about current state of the autopilot system to [\"reply\"]:\n"
        "[\"enabled\"]: boolean indicating whether or not autopilot is enabled\n"
        "[\"target_global\"]: array of target global {x, y, z} position\n"
        "[\"leader_id\"]: uuid of target autopilot is following\n"
        "[\"stop_distance\"]: maximum stop distance from target\n"
        "[\"target_distance\"]: last known distance from target\n"
        "[\"use_rotation\"]: boolean indicating if autopilot has a target facing rotation\n"
        "[\"target_facing\"]: array of {x, y} target direction to face\n"
        "[\"rotation_threshold\"]: target maximum angle from target facing rotation\n"
        "[\"behavior_name\"]: name of the autopilot behavior",
        &LLAgentListener::getAutoPilot,
        llsd::map("reply", LLSD()));
    add("startFollowPilot",
        "[\"leader_id\"]: uuid of target to follow using the autopilot system (optional with avatar_name)\n"
        "[\"avatar_name\"]: avatar name to follow using the autopilot system (optional with leader_id)\n"
        "[\"allow_flying\"]: allow flying during autopilot [default: True]\n"
        "[\"stop_distance\"]: maximum stop distance from target [default: autopilot guess]",
        &LLAgentListener::startFollowPilot,
        llsd::map("reply", LLSD()));
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
    //camera params are similar to LSL, see https://wiki.secondlife.com/wiki/LlSetCameraParams
    add("setCameraParams",
        "Set Follow camera params, and then activate it:\n"
        "[\"camera_pos\"]: vector3, camera position in region coordinates\n"
        "[\"focus_pos\"]: vector3, what the camera is aimed at (in region coordinates)\n"
        "[\"focus_offset\"]: vector3, adjusts the camera focus position relative to the target, default is (1, 0, 0)\n"
        "[\"distance\"]: float (meters), distance the camera wants to be from its target, default is 3\n"
        "[\"focus_threshold\"]: float (meters), sets the radius of a sphere around the camera's target position within which its focus is not affected by target motion, default is 1\n"
        "[\"camera_threshold\"]: float (meters), sets the radius of a sphere around the camera's ideal position within which it is not affected by target motion, default is 1\n"
        "[\"focus_lag\"]: float (seconds), how much the camera lags as it tries to aim towards the target, default is 0.1\n"
        "[\"camera_lag\"]: float (seconds), how much the camera lags as it tries to move towards its 'ideal' position, default is 0.1\n"
        "[\"camera_pitch\"]: float (degrees), adjusts the angular amount that the camera aims straight ahead vs. straight down, maintaining the same distance, default is 0\n"
        "[\"behindness_angle\"]: float (degrees), sets the angle in degrees within which the camera is not constrained by changes in target rotation, default is 10\n"
        "[\"behindness_lag\"]: float (seconds), sets how strongly the camera is forced to stay behind the target if outside of behindness angle, default is 0\n"
        "[\"camera_locked\"]: bool, locks the camera position so it will not move\n"
        "[\"focus_locked\"]: bool, locks the camera focus so it will not move",
        &LLAgentListener::setFollowCamParams);
    add("setFollowCamActive",
        "Turns on or off scripted control of the camera using boolean [\"active\"]",
        &LLAgentListener::setFollowCamActive,
        llsd::map("active", LLSD()));
    add("removeCameraParams",
        "Reset Follow camera params",
        &LLAgentListener::removeFollowCamParams);

    add("playAnimation",
        "Play [\"item_id\"] animation locally (by default) or [\"inworld\"] (when set to true)",
        &LLAgentListener::playAnimation,
        llsd::map("item_id", LLSD(), "reply", LLSD()));
    add("stopAnimation",
        "Stop playing [\"item_id\"] animation",
        &LLAgentListener::stopAnimation,
        llsd::map("item_id", LLSD(), "reply", LLSD()));
    add("getAnimationInfo",
        "Return information about [\"item_id\"] animation",
        &LLAgentListener::getAnimationInfo,
        llsd::map("item_id", LLSD(), "reply", LLSD()));

    add("getID",
        "Return your own avatar ID",
        &LLAgentListener::getID,
        llsd::map("reply", LLSD()));

    add("getNearbyAvatarsList",
        "Return result set key [\"result\"] for nearby avatars in a range of [\"dist\"]\n"
        "if [\"dist\"] is not specified, 'RenderFarClip' setting is used\n"
        "reply contains \"result\" table with \"id\", \"name\", \"global_pos\", \"region_pos\", \"region_id\" fields",
        &LLAgentListener::getNearbyAvatarsList,
        llsd::map("reply", LLSD()));

    add("getNearbyObjectsList",
        "Return result set key [\"result\"] for nearby objects in a range of [\"dist\"]\n"
        "if [\"dist\"] is not specified, 'RenderFarClip' setting is used\n"
        "reply contains \"result\" table with \"id\", \"global_pos\", \"region_pos\", \"region_id\" fields",
        &LLAgentListener::getNearbyObjectsList,
        llsd::map("reply", LLSD()));

    add("getAgentScreenPos",
        "Return screen position of the [\"avatar_id\"] avatar or own avatar if not specified\n"
        "reply contains \"x\", \"y\" coordinates and \"onscreen\" flag to indicate if it's actually in within the current window\n"
        "avatar render position is used as the point",
        &LLAgentListener::getAgentScreenPos,
        llsd::map("reply", LLSD()));
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
        LLCommandDispatcher::dispatch("teleport", params, LLSD(), LLGridManager::getInstance()->getGrid(), NULL, LLCommandHandler::NAV_TYPE_CLICKED, true);
        // *TODO - lookup other LLCommandHandlers for "agent", "classified", "event", "group", "floater", "parcel", "login", login_refresh", "balance", "chat"
        // should we just compose LLCommandHandler and LLDispatchListener?
    }
    else
    {
        std::string url = LLSLURL(event_data["regionname"],
                                  LLVector3((F32)event_data["x"].asReal(),
                                            (F32)event_data["y"].asReal(),
                                            (F32)event_data["z"].asReal())).getSLURLString();
        LLURLDispatcher::dispatch(url, LLCommandHandler::NAV_TYPE_CLICKED, NULL, false);
    }
}

void LLAgentListener::requestSit(LLSD const & event_data) const
{
    //mAgent.getAvatarObject()->sitOnObject();
    // shamelessly ripped from llviewermenu.cpp:handle_sit_or_stand()
    // *TODO - find a permanent place to share this code properly.
    Response response(LLSD(), event_data);
    LLViewerObject *object = NULL;
    if (event_data.has("obj_uuid"))
    {
        object = gObjectList.findObject(event_data["obj_uuid"]);
    }
    else if (event_data.has("position"))
    {
        LLVector3 target_position = ll_vector3_from_sd(event_data["position"]);
        object = findObjectClosestTo(target_position, true);
    }
    else
    {
        //just sit on the ground
        mAgent.setControlFlags(AGENT_CONTROL_SIT_ON_GROUND);
        return;
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
        response.error("requestSit could not find the sit target");
    }
}

void LLAgentListener::requestStand(LLSD const & event_data) const
{
    mAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
}


LLViewerObject * LLAgentListener::findObjectClosestTo(const LLVector3 & position, bool sit_target) const
{
    LLViewerObject *object = NULL;

    // Find the object closest to that position
    F32 min_distance = 10000.0f;        // Start big
    S32 num_objects = gObjectList.getNumObjects();
    S32 cur_index = 0;
    while (cur_index < num_objects)
    {
        LLViewerObject * cur_object = gObjectList.getObject(cur_index++);
        if (cur_object && !cur_object->isAttachment())
        {
            if(sit_target && (cur_object->getPCode() != LL_PCODE_VOLUME))
            {
                continue;
            }
            // Calculate distance from the target position
            LLVector3 target_diff = cur_object->getPositionRegion() - position;
            F32 distance_to_target = target_diff.length();
            if (distance_to_target < min_distance)
            {   // Found an object closer
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

        pick.mUVCoords   "UVCoord"
        pick.mSTCoords  "STCoord"
        pick.mObjectFace    "FaceIndex"
        pick.mIntersection  "Position"
        pick.mNormal    "Normal"
        pick.mBinormal  "Binormal"
        */

        // A touch is a sketchy message sequence ... send a grab, immediately
        // followed by un-grabbing, crossing fingers and hoping packets arrive in
        // the correct order
        send_ObjectGrab_message(object, pick, LLVector3::zero);
        send_ObjectDeGrab_message(object, pick);
    }
    else
    {
        LL_WARNS() << "LLAgent requestTouch could not find the touch target "
            << event_data["obj_uuid"].asUUID() << LL_ENDL;
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
    LLQuaternion* target_rotation = NULL;
    if (event_data.has("target_rotation"))
    {
        LLQuaternion target_rotation_value = ll_quaternion_from_sd(event_data["target_rotation"]);
        target_rotation = &target_rotation_value;
    }

    F32 rotation_threshold = 0.03f;
    if (event_data.has("rotation_threshold"))
    {
        rotation_threshold = (F32)event_data["rotation_threshold"].asReal();
    }

    bool allow_flying = true;
    if (event_data.has("allow_flying"))
    {
        allow_flying = (bool) event_data["allow_flying"].asBoolean();
        mAgent.setFlying(allow_flying);
    }

    F32 stop_distance = 0.f;
    if (event_data.has("stop_distance"))
    {
        stop_distance = (F32)event_data["stop_distance"].asReal();
    }

    std::string behavior_name = LLCoros::getName();
    if (event_data.has("behavior_name"))
    {
        behavior_name = event_data["behavior_name"].asString();
    }

    // Clear follow target, this is doing a path
    mFollowTarget.setNull();

    auto finish_cb = [](bool success, void*)
    {
        LLEventPumps::instance().obtain("LLAutopilot").post(llsd::map("success", success));
    };

    mAgent.startAutoPilotGlobal(ll_vector3d_from_sd(event_data["target_global"]),
                                behavior_name,
                                target_rotation,
                                finish_cb, NULL,
                                stop_distance,
                                rotation_threshold,
                                allow_flying);
}

void LLAgentListener::getAutoPilot(const LLSD& event_data) const
{
    Response reply(LLSD(), event_data);

    LLSD::Boolean enabled = mAgent.getAutoPilot();
    reply["enabled"] = enabled;

    reply["target_global"] = ll_sd_from_vector3d(mAgent.getAutoPilotTargetGlobal());

    reply["leader_id"] = mAgent.getAutoPilotLeaderID();

    reply["stop_distance"] = mAgent.getAutoPilotStopDistance();

    reply["target_distance"] = mAgent.getAutoPilotTargetDist();
    if (!enabled &&
        mFollowTarget.notNull())
    {   // Get an actual distance from the target object we were following
        LLViewerObject * target = gObjectList.findObject(mFollowTarget);
        if (target)
        {   // Found the target AV, return the actual distance to them as well as their ID
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
}

void LLAgentListener::startFollowPilot(LLSD const & event_data)
{
    Response response(LLSD(), event_data);
    LLUUID target_id;

    bool allow_flying = true;
    if (event_data.has("allow_flying"))
    {
        allow_flying = (bool) event_data["allow_flying"].asBoolean();
    }

    if (event_data.has("leader_id"))
    {
        target_id = event_data["leader_id"];
    }
    else if (event_data.has("avatar_name"))
    {   // Find the avatar with matching name
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
                {   // Found avatar with matching name, extract id and break out of loop
                    target_id = cur_object->getID();
                    break;
                }
            }
        }
    }
    else
    {
        return response.error("'leader_id' or 'avatar_name' should be specified");
    }

    F32 stop_distance = 0.f;
    if (event_data.has("stop_distance"))
    {
        stop_distance = (F32)event_data["stop_distance"].asReal();
    }

    if (!gObjectList.findObject(target_id))
    {
        std::string target_info = event_data.has("leader_id") ? event_data["leader_id"] : event_data["avatar_name"];
        return response.error(stringize("Target ", std::quoted(target_info), " was not found"));
    }

    mAgent.setFlying(allow_flying);
    mFollowTarget = target_id;  // Save follow target so we can report distance later

    mAgent.startFollowPilot(target_id, allow_flying, stop_distance);
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
    bool user_cancel = false;
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
    for (std::vector<LLGroupData>::const_iterator
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

/*----------------------------- camera control -----------------------------*/
// specialize LLSDParam to support (const LLVector3&) arguments -- this
// wouldn't even be necessary except that the relevant LLVector3 constructor
// is explicitly explicit
template <>
class LLSDParam<const LLVector3&>: public LLSDParamBase
{
public:
    LLSDParam(const LLSD& value): value(LLVector3(value)) {}

    operator const LLVector3&() const { return value; }

private:
    LLVector3 value;
};

// accept any of a number of similar LLFollowCamMgr methods with different
// argument types, and return a wrapper lambda that accepts LLSD and converts
// to the target argument type
template <typename T>
auto wrap(void (LLFollowCamMgr::*method)(const LLUUID& source, T arg))
{
    return [method](LLFollowCamMgr& followcam, const LLUUID& source, const LLSD& arg)
    { (followcam.*method)(source, LLSDParam<T>(arg)); };
}

// table of supported LLFollowCamMgr methods,
// with the corresponding setFollowCamParams() argument keys
static std::pair<std::string, std::function<void(LLFollowCamMgr&, const LLUUID&, const LLSD&)>>
cam_params[] =
{
    { "camera_pos",       wrap(&LLFollowCamMgr::setPosition) },
    { "focus_pos",        wrap(&LLFollowCamMgr::setFocus) },
    { "focus_offset",     wrap(&LLFollowCamMgr::setFocusOffset) },
    { "camera_locked",    wrap(&LLFollowCamMgr::setPositionLocked) },
    { "focus_locked",     wrap(&LLFollowCamMgr::setFocusLocked) },
    { "distance",         wrap(&LLFollowCamMgr::setDistance) },
    { "focus_threshold",  wrap(&LLFollowCamMgr::setFocusThreshold) },
    { "camera_threshold", wrap(&LLFollowCamMgr::setPositionThreshold) },
    { "focus_lag",        wrap(&LLFollowCamMgr::setFocusLag) },
    { "camera_lag",       wrap(&LLFollowCamMgr::setPositionLag) },
    { "camera_pitch",     wrap(&LLFollowCamMgr::setPitch) },
    { "behindness_lag",   wrap(&LLFollowCamMgr::setBehindnessLag) },
    { "behindness_angle", wrap(&LLFollowCamMgr::setBehindnessAngle) },
};

void LLAgentListener::setFollowCamParams(const LLSD& event) const
{
    auto& followcam{ LLFollowCamMgr::instance() };
    for (const auto& pair : cam_params)
    {
        if (event.has(pair.first))
        {
            pair.second(followcam, gAgentID, event[pair.first]);
        }
    }
    followcam.setCameraActive(gAgentID, true);
}

void LLAgentListener::setFollowCamActive(LLSD const & event) const
{
    LLFollowCamMgr::getInstance()->setCameraActive(gAgentID, event["active"]);
}

void LLAgentListener::removeFollowCamParams(LLSD const & event) const
{
    LLFollowCamMgr::getInstance()->removeFollowCamParams(gAgentID);
}

LLViewerInventoryItem* get_anim_item(LLEventAPI::Response &response, const LLSD &event_data)
{
    LLViewerInventoryItem* item = gInventory.getItem(event_data["item_id"].asUUID());
    if (!item || (item->getInventoryType() != LLInventoryType::IT_ANIMATION))
    {
        response.error(stringize("Animation item ", std::quoted(event_data["item_id"].asString()), " was not found"));
        return NULL;
    }
    return item;
}

void LLAgentListener::playAnimation(LLSD const &event_data)
{
    Response response(LLSD(), event_data);
    if (LLViewerInventoryItem* item = get_anim_item(response, event_data))
    {
        mPlayAnimThrottle(item->getAssetUUID(), event_data["inworld"].asBoolean());
    }
}

void LLAgentListener::playAnimation_(const LLUUID& asset_id, const bool inworld)
{
    if (inworld)
    {
        mAgent.sendAnimationRequest(asset_id, ANIM_REQUEST_START);
    }
    else
    {
        gAgentAvatarp->startMotion(asset_id);
    }
}

void LLAgentListener::stopAnimation(LLSD const &event_data)
{
    Response response(LLSD(), event_data);
    if (LLViewerInventoryItem* item = get_anim_item(response, event_data))
    {
        gAgentAvatarp->stopMotion(item->getAssetUUID());
        mAgent.sendAnimationRequest(item->getAssetUUID(), ANIM_REQUEST_STOP);
    }
}

void LLAgentListener::getAnimationInfo(LLSD const &event_data)
{
    Response response(LLSD(), event_data);
    if (LLViewerInventoryItem* item = get_anim_item(response, event_data))
    {
        // if motion exists, will return existing one
        LLMotion* motion = gAgentAvatarp->createMotion(item->getAssetUUID());
        response["anim_info"] = llsd::map("duration", motion->getDuration(),
                                               "is_loop", motion->getLoop(),
                                               "num_joints", motion->getNumJointMotions(),
                                               "asset_id", item->getAssetUUID(),
                                               "priority", motion->getPriority());
    }
}

void LLAgentListener::getID(LLSD const& event_data)
{
    Response response(llsd::map("id", gAgentID), event_data);
}

struct AvResultSet : public LL::VectorResultSet<LLVOAvatar*>
{
    AvResultSet() : super("nearby_avatars") {}
    LLSD getSingleFrom(LLVOAvatar* const& av) const override
    {
        LLAvatarName av_name;
        LLAvatarNameCache::get(av->getID(), &av_name);
        LLVector3 region_pos = av->getCharacterPosition();
        return llsd::map("id", av->getID(),
                         "global_pos", ll_sd_from_vector3d(av->getPosGlobalFromAgent(region_pos)),
                         "region_pos", ll_sd_from_vector3(region_pos),
                         "name", av_name.getUserName(),
                         "region_id", av->getRegion()->getRegionID());
    }
};

struct ObjResultSet : public LL::VectorResultSet<LLViewerObject*>
{
    ObjResultSet() : super("nearby_objects") {}
    LLSD getSingleFrom(LLViewerObject* const& obj) const override
    {
        return llsd::map("id", obj->getID(),
                         "global_pos", ll_sd_from_vector3d(obj->getPositionGlobal()),
                         "region_pos", ll_sd_from_vector3(obj->getPositionRegion()),
                         "region_id", obj->getRegion()->getRegionID());
    }
};


F32 get_search_radius(LLSD const& event_data)
{
    static LLCachedControl<F32> render_far_clip(gSavedSettings, "RenderFarClip", 64);
    F32 dist = render_far_clip;
    if (event_data.has("dist"))
    {
        dist = llclamp((F32)event_data["dist"].asReal(), 1, 512);
    }
   return dist * dist;
}

void LLAgentListener::getNearbyAvatarsList(LLSD const& event_data)
{
    Response response(LLSD(), event_data);
    auto avresult = new AvResultSet;

    F32 radius = get_search_radius(event_data);
    LLVector3d agent_pos = gAgent.getPositionGlobal();
    for (LLCharacter* character : LLCharacter::sInstances)
    {
        LLVOAvatar* avatar = (LLVOAvatar*)character;
        if (!avatar->isDead() && !avatar->isControlAvatar() && !avatar->isSelf())
        {
            if ((dist_vec_squared(avatar->getPositionGlobal(), agent_pos) <= radius))
            {
                avresult->mVector.push_back(avatar);
            }
        }
    }
    response["result"] = avresult->getKeyLength();
}

void LLAgentListener::getNearbyObjectsList(LLSD const& event_data)
{
    Response response(LLSD(), event_data);
    auto objresult = new ObjResultSet;

    F32 radius = get_search_radius(event_data);
    S32 num_objects = gObjectList.getNumObjects();
    LLVector3d agent_pos = gAgent.getPositionGlobal();
    for (S32 i = 0; i < num_objects; ++i)
    {
        LLViewerObject* object = gObjectList.getObject(i);
        if (object && object->getVolume() && !object->isAttachment())
        {
            if ((dist_vec_squared(object->getPositionGlobal(), agent_pos) <= radius))
            {
                objresult->mVector.push_back(object);
            }
        }
    }
    response["result"] = objresult->getKeyLength();
}

void LLAgentListener::getAgentScreenPos(LLSD const& event_data)
{
    Response response(LLSD(), event_data);
    LLVector3 render_pos;
    if (event_data.has("avatar_id") && (event_data["avatar_id"].asUUID() != gAgentID))
    {
        LLUUID avatar_id(event_data["avatar_id"]);
        for (LLCharacter* character : LLCharacter::sInstances)
        {
            LLVOAvatar* avatar = (LLVOAvatar*)character;
            if (!avatar->isDead() && (avatar->getID() == avatar_id))
            {
                render_pos = avatar->getRenderPosition();
                break;
            }
        }
    }
    else if (gAgentAvatarp.notNull() && gAgentAvatarp->isValid())
    {
        render_pos = gAgentAvatarp->getRenderPosition();
    }
    LLCoordGL screen_pos;
    response["onscreen"] = LLViewerCamera::getInstance()->projectPosAgentToScreen(render_pos, screen_pos, false);
    response["x"] = screen_pos.mX;
    response["y"] = screen_pos.mY;
}
