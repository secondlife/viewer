/**
 * @file llpuppetmodule.cpp
 * @brief Implementation of LLPuppetModule class.
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

#include "llpuppetmodule.h"

#include <boost/bind.hpp>

#include "httpcommon.h"

#include "llagent.h"
#include "llevents.h"
#include "llheadrotmotion.h"
#include "llleap.h"
#include "llpuppetmotion.h"
#include "llsdutil.h"
#include "llviewercontrol.h"    // gSavedSettings
#include "llviewerobjectlist.h" //gObjectList
#include "llviewerregion.h"
#include "llvoavatarself.h"


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
static const std::string current_camera_setting("puppetry_current_camera");
static const std::string puppetry_parts_setting("puppetry_enabled_parts");

// static
void processLeapData(const LLSD& data)
{
    // Data arrives through a LEAP child process which is specified via
    // option to the secondlife viewer.  It is most likely started via the UI.
    // It also can be started on the command line like this:
    //
    //   '--leap "python /path/to/script.py"'
    //
    // The viewer will launch the script which is expected to write messages
    // to its stdout.  Each message is converted into a proper LLEvent on the
    // "puppetry" LLEventPump, and an LLEventListener submits their 'data'
    // payloads to this method.
    // Note: don't confuse the more general LLEvent class with the specific
    // LLPuppetJointEvent and LLPuppetEvent classes: they do not share ancestry.
    //
    // Expected data format:
    //   data = {'command':'move','joint_name':{'param_name':[r1.23,r4.56,r7.89]}, ...}
    // Where:
    //   joint_name = e.g. mWristLeft
    //   param_name = rot | pos | scale | eff
    //   param value = array of 3 floats [x,y,z]

    if (!data.isMap())
    {
        LL_WARNS("Puppet") << "Received invalid non-map data" << LL_ENDL;
        return;
    }

    LLVOAvatar* voa = static_cast<LLVOAvatar*>(gObjectList.findObject(gAgentID));
    if (!voa)
    {
        LL_WARNS("Puppet") << "No avatar object found for self" << LL_ENDL;
        return;
    }
    LLMotion::ptr_t motion(gAgentAvatarp->findMotion(ANIM_AGENT_PUPPET_MOTION));
    if (!motion)
    {
        LL_WARNS("Puppet") << "No puppet motion found on self" << LL_ENDL;
        return;
    }

    LL_DEBUGS("LLLeapData") << "puppet data: " << data << LL_ENDL;

    LLVector3 v;
    for (LLSD::map_const_iterator joint_itr = data.beginMap();
            joint_itr != data.endMap();
            ++joint_itr)
    {
        const std::string& joint_name = joint_itr->first;
        if (joint_name == "time")
        {   // Actually shouldn't get 'time', but it's added when writing data to a file.
            continue;       // Ignore it if it sneaks in here (TBD - is it useful downstream?)
        }

        LLJoint* joint = voa->getJoint(joint_name);
        if (!joint)
        {
            continue;
        }
        if (joint_name == "mHead")
        {   // If the head is animated, stop looking at the mouse
            LLPuppetModule::instance().disableHeadMotion();
        }
        const LLSD& params = joint_itr->second;
        if (!params.isMap())
        {
            continue;
        }

        // Record that we've seen this joint name
        LLPuppetModule::instance().addActiveJoint(joint_name);

        LLPuppetJointEvent joint_event;
        joint_event.setJointID(joint->getJointNum());
        for (LLSD::map_const_iterator param_itr = params.beginMap();
                param_itr != params.endMap();
                ++param_itr)
        {
            const LLSD& value = param_itr->second;
            const std::string& param_name = param_itr->first;
            if (!value.isArray() || value.size() != 3)
            {
                if (param_name == "no_constraint" && value.asBoolean())
                {
                    joint_event.disableConstraint();
                }
                continue;
            }
            v.mV[VX] = value.get(0).asReal();
            v.mV[VY] = value.get(1).asReal();
            v.mV[VZ] = value.get(2).asReal();

            if (param_name == "rot" || param_name == "local_rot")
            {
                // Packed quaternions have the imaginary part (e.g. xyz)
                LLQuaternion q;
                // copy the imaginary part
                memcpy(q.mQ, v.mV, 3 * sizeof(F32));
                // compute the real part
                F32 imaginary_length_squared = q.mQ[VX] * q.mQ[VX] + q.mQ[VY] * q.mQ[VY] + q.mQ[VZ] * q.mQ[VZ];
                if (imaginary_length_squared > 1.0f)
                {
                    F32 imaginary_length = sqrtf(imaginary_length_squared);
                    q.mQ[VX] /= imaginary_length;
                    q.mQ[VY] /= imaginary_length;
                    q.mQ[VZ] /= imaginary_length;
                    q.mQ[VW] = 0.0f;
                }
                else
                {
                    q.mQ[VW] = sqrtf(1.0f - imaginary_length_squared);
                }
                LLPuppetJointEvent::E_REFERENCE_FRAME ref_frame = param_name == "local_rot" ?
                    LLPuppetJointEvent::PARENT_FRAME : LLPuppetJointEvent::ROOT_FRAME;
                joint_event.setRotation(q, ref_frame);
            }
            else if (param_name == "pos")
            {
                joint_event.setPosition(v);
            }
            else if (param_name == "scale")
            {
                joint_event.setScale(v);
            }
            // LEGACY support for "eff" -- please use "pos" instead
            else if (param_name == "eff")
            {
                joint_event.setPosition(v);
            }
        }
        if (!joint_event.isEmpty())
        {
            if (!motion->isActive())
            {
                gAgentAvatarp->startMotion(ANIM_AGENT_PUPPET_MOTION);
            }
            std::static_pointer_cast<LLPuppetMotion>(motion)->addExpressionEvent(joint_event);
        }
    }
}

LLPuppetModule::LLPuppetModule() :
    LLEventAPI("puppetry",
               "Integrate external puppetry control module",
               // dispatch incoming events on "command" key
               "command"),
    mLeapModule(),
    mPlayServerEcho(false)
{
    gSavedSettings.declareS32(current_camera_setting, 0, "Camera device number, 0, 1, 2 etc");
    gSavedSettings.declareS32(puppetry_parts_setting, PPM_ALL, "Enabled puppetry body parts mask");

    add("move",
        "Send puppet movement data\n"
        "Expected data format:\n"
        "  {'joint_name':{'param_name':[r1.23,r4.56,r7.89]}, ...}\n"
        "Where:\n"
        "  joint_name = e.g. mWristLeft\n"
        "  param_name = rot | pos | scale | eff\n"
        "  param value = array of 3 floats [x,y,z]\n"
        "No response is generated - invalid data blob is ignored",
        &processLeapData);
    add("get_camera",
        "Request camera number: returns [\"camera_id\"]",
        &LLPuppetModule::getCameraNumber_);
    add("set_camera",
        "Set camera number [\"camera_id\"]",
        &LLPuppetModule::setCameraNumber_,
        [this](){ return this; },   // instance getter
        llsd::array("camera_id"));  // argument names
    add("send_skeleton",
        "Request skeleton data: returns dict",
        &LLPuppetModule::send_skeleton);

    mPlugin = LLEventPumps::instance().obtain("SkeletonUpdate").listen(
                    "LLPuppetModule",
                    [](const LLSD& unused)
                    {
                        LLPuppetModule::instance().send_skeleton();
                        return false;
                    });
}


void LLPuppetModule::setLeapModule(std::weak_ptr<LLLeap> mod, const std::string & module_name)
{
    mLeapModule = mod;
    mModuleName = module_name;
    mActiveJoints.clear();      // Make sure data is cleared
    if (gAgentAvatarp->getPuppetMotion())
        gAgentAvatarp->getPuppetMotion()->clearAll();
};


LLPuppetModule::puppet_module_ptr_t LLPuppetModule::getLeapModule() const
{
    // lock it
    return mLeapModule.lock();
}


bool LLPuppetModule::havePuppetModule() const
{
    puppet_module_ptr_t mod = getLeapModule();
    return (bool)(mod);
}

void LLPuppetModule::disableHeadMotion() const
{
    LLMotion::ptr_t motion(gAgentAvatarp->findMotion(ANIM_AGENT_HEAD_ROT));
    LLHeadRotMotion* head_rot_motion = static_cast<LLHeadRotMotion*>(motion.get());
    if (head_rot_motion)
    {
        head_rot_motion->disable();
    }
}

void LLPuppetModule::enableHeadMotion() const
{
    LLMotion::ptr_t motion(gAgentAvatarp->findMotion(ANIM_AGENT_HEAD_ROT));
    LLHeadRotMotion* head_rot_motion = static_cast<LLHeadRotMotion*>(motion.get());
    if (head_rot_motion)
    {
        head_rot_motion->enable();
    }
}

void LLPuppetModule::clearLeapModule()
{
    if (isAgentAvatarValid())
    {
        LL_INFOS("Puppet") << "Sending 'stop' command to Leap module" << LL_ENDL;
        sendCommand("stop");
        enableHeadMotion();
        mActiveJoints.clear();
        bool immediate = false;
        gAgentAvatarp->stopMotion(ANIM_AGENT_PUPPET_MOTION, immediate);
    }
}

void LLPuppetModule::sendCommand(const std::string& command, const LLSD& args) const
{
    puppet_module_ptr_t mod = getLeapModule();
    if (mod)
    {
        LLSD data;
        data["command"] = command;
        // args is optional
        if (args.isDefined())
        {
            data["args"] = args;
        }
        LL_DEBUGS("Puppet") << "Posting " << command << " to Leap module" << LL_ENDL;
        LLEventPumps::instance().post("puppetry.controller", data);
    }
    else
    {
        LL_WARNS("Puppet") << "Puppet module not loaded, dropping " << command << " command" << LL_ENDL;
    }
}


void LLPuppetModule::setCameraNumber(S32 num)
{
    setCameraNumber_(num);
    // For a C++ caller, also send the new camera number to the LEAP module.
    sendCameraNumber();
}

void LLPuppetModule::setCameraNumber_(S32 num)
{
    gSavedSettings.setS32(current_camera_setting, num);
    LL_INFOS("Puppet") << "Camera number set to " << num << LL_ENDL;
}

S32 LLPuppetModule::getCameraNumber() const
{
    return gSavedSettings.getS32(current_camera_setting);
};

void LLPuppetModule::getCameraNumber_(const LLSD& request) const
{
    // Response sends a reply on destruction.
    Response response(llsd::map("camera_id", getCameraNumber()), request);
}

void LLPuppetModule::sendCameraNumber()
{
    sendCommand("set_camera", llsd::map("camera_id", getCameraNumber()));
}

void LLPuppetModule::send_skeleton(const LLSD& sd)  //Named per nat's request
{
    LLMotion::ptr_t motion(gAgentAvatarp->findMotion(ANIM_AGENT_PUPPET_MOTION));
    if (!motion)
    {
        LL_WARNS("Puppet") << "No puppet motion found on self" << LL_ENDL;
        return;
    }

    LLSD skeleton_sd = std::static_pointer_cast<LLPuppetMotion>(motion)->getSkeletonData();
    sendCommand("set_skeleton", skeleton_sd);
}

void LLPuppetModule::sendEnabledParts()
{
    sendCommand("enable_parts", llsd::map("parts_mask", getEnabledPart()));
}

void LLPuppetModule::setEnabledPart(S32 part_num, bool enable)
{   // Enable puppetry on body part - head, face, left / right hands
    S32 cur_setting = gSavedSettings.getS32(puppetry_parts_setting) & PPM_ALL;
    part_num = part_num & PPM_ALL;
    if (enable)
    {   // set bit
        cur_setting = cur_setting | part_num;
    }
    else
    {   // clear bit
        cur_setting = cur_setting & ~part_num;
    }

    gSavedSettings.setS32(puppetry_parts_setting, cur_setting);
    LL_INFOS("Puppet") << "Puppetry enabled parts mask now " << cur_setting << LL_ENDL;

    sendEnabledParts();     // Send to module
}

S32  LLPuppetModule::getEnabledPart(S32 mask /* = PPM_ALL */) const
{
    return (gSavedSettings.getS32(puppetry_parts_setting) & mask);
}

void LLPuppetModule::addActiveJoint(const std::string & joint_name)
{
    mActiveJoints[joint_name] = LLFrameTimer::getTotalSeconds();
}

bool LLPuppetModule::isActiveJoint(const std::string & joint_name)
{
    active_joint_map_t::iterator iter = mActiveJoints.find(joint_name);
    if (iter != mActiveJoints.end())
    {
        F64 age = LLFrameTimer::getTotalSeconds() - iter->second;
        const F64 PUPPET_SHOW_BONE_AGE = 3.0;
        if (age < PUPPET_SHOW_BONE_AGE)
        {   // It's recently active
            return true;
        }
        // Delete old data and return not found
        mActiveJoints.erase(iter);
    }
    return false;   // Not found
}

void LLPuppetModule::setEcho(bool play_server_echo)
{
    setPuppetryOptions(LLSDMap("echo_back", play_server_echo));
}

void LLPuppetModule::setSending(bool sending)
{
    setPuppetryOptions(LLSDMap("transmit", sending));
}

void LLPuppetModule::setReceiving(bool receiving)
{
    setPuppetryOptions(LLSDMap("receive", receiving));
}

void LLPuppetModule::setRange(F32 range)
{
    setPuppetryOptions(LLSDMap("range", range));
}

void LLPuppetModule::setPuppetryOptions(LLSD options)
{
    std::string cap;
    LLViewerRegion *currentRegion = gAgent.getRegion();
    if (currentRegion)
    {
        cap = currentRegion->getCapability("Puppetry");
    }

    if (!cap.empty())
    {   // Start up coroutine to set puppetry options.
        if (options.has("echo_back") && options["echo_back"].asBoolean())
        {   // echo implies both transmit and receive.
            options["transmit"] = true;
            options["receive"] = true;
        }

        LLCoros::instance().launch("SetPuppetryOptionsCoro",
            [cap, options]() { LLPuppetModule::SetPuppetryOptionsCoro(cap, options); });
    }
    else
    {
        LL_WARNS("Puppet") << "Unable to get Puppeteer cap to set echo status" << LL_ENDL;
    }
}

void LLPuppetModule::parsePuppetryResponse(LLSD response)
{
    mPlayServerEcho = response["echo_back"].asBoolean();
    mIsSending = response["transmit"].asBoolean();
    mIsReceiving = response["receive"].asBoolean();
    mRange = response["range"].asReal();

    /*TODO Mute list and subscribe*/
    LL_INFOS("Puppet") << "Settings puppetry values from server: echo " << (mPlayServerEcho ? "on" : "off") <<
        ", transmit is " << (mIsSending ? "on" : "off") << 
        ", receiving is " << (mIsReceiving? "on" : "off") << 
        ", receiving range is " << mRange << "m" << LL_ENDL;
}

// Set echo on or off
void LLPuppetModule::SetPuppetryOptionsCoro(const std::string& capurl, LLSD options)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("SetPuppetryOptionsCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    httpOpts->setFollowRedirects(true);

    LLCore::HttpStatus status;
    LLSD result;
    S32 retry_count(0);
    do
    {
        LLSD dataToPost = LLSD::emptyMap();
        if (options.has("echo_back"))
            dataToPost["echo_back"] = options["echo_back"].asBoolean();
        if (options.has("transmit"))
            dataToPost["transmit"] = options["transmit"].asBoolean();
        if (options.has("receive"))
            dataToPost["receive"] = options["receive"].asBoolean();
        if (options.has("range"))
            dataToPost["range"] = options["range"].asReal();

        result = httpAdapter->postAndSuspend(httpRequest, capurl, dataToPost);

        LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        if (status.getType() == HTTP_NOT_FOUND)
        {   // There seems to be a case at first login where the simulator is slow getting all of the caps
            // connected for the agent.  It has given us back the cap URL but returns a 404 when we try and
            // hit it.  Pause, take a breath and give it another shot.
            if (++retry_count >= 3)
            {
                LL_WARNS("Puppet") << "Failed to set puppetry echo status after 3 retries." << LL_ENDL;
                return;
            }
            llcoro::suspendUntilTimeout(0.25);
            continue;
        }
        else if (!status)
        {
            LL_WARNS("Puppet") << "Failed to set puppetry echo status with " << status.getMessage() << " body: " << result << LL_ENDL;
            return;
        }

        break;
    } while (true);

    LLPuppetModule::instance().parsePuppetryResponse(result);
}
