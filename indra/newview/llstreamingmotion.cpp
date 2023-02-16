/**
 * @file llstreamingmotion.cpp
 * @brief LLStreamingMotion is an LLMotion which plays animation stream from server
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#include <set>
#include <limits>

#include "llagent.h"
#include "llagentcamera.h"
#include "llagentdata.h"
#include "llcharacter.h"
#include "lldatapacker.h"
#include "llframetimer.h"
#include "llstreamingmotion.h"
#include "llpuppetmodule.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "v3dmath.h"
#include "llsdutil_math.h"        //For ll_sd_from_vector3

#include "llvoavatarself.h"

#define ENABLE_RIGHT_CONSTRAINTS

// BEGIN_HACK : hard-coded joint_ids
//constexpr U16 PELVIS_ID = 0;
constexpr U16 TORSO_ID = 3;
constexpr U16 CHEST_ID = 6;
constexpr U16 NECK_ID = 7;
constexpr U16 HEAD_ID = 8;
constexpr S16 COLLAR_LEFT_ID = 58;
constexpr S16 SHOULDER_LEFT_ID = 59;
constexpr S16 ELBOW_LEFT_ID = 60;
constexpr S16 WRIST_LEFT_ID = 61;

constexpr S16 HAND_MIDDLE_LEFT_1_ID = 62;
constexpr S16 HAND_MIDDLE_LEFT_2_ID = 63;
constexpr S16 HAND_MIDDLE_LEFT_3_ID = 64;
constexpr S16 HAND_INDEX_LEFT_1_ID = 65;
constexpr S16 HAND_INDEX_LEFT_2_ID = 66;
constexpr S16 HAND_INDEX_LEFT_3_ID = 67;
constexpr S16 HAND_RING_LEFT_1_ID = 68;
constexpr S16 HAND_RING_LEFT_2_ID = 69;
constexpr S16 HAND_RING_LEFT_3_ID = 70;
constexpr S16 HAND_PINKY_LEFT_1_ID = 71;
constexpr S16 HAND_PINKY_LEFT_2_ID = 72;
constexpr S16 HAND_PINKY_LEFT_3_ID = 73;
constexpr S16 HAND_THUMB_LEFT_1_ID = 74;
constexpr S16 HAND_THUMB_LEFT_2_ID = 75;
constexpr S16 HAND_THUMB_LEFT_3_ID = 76;

#ifdef ENABLE_RIGHT_CONSTRAINTS
constexpr S16 COLLAR_RIGHT_ID = 77;
constexpr S16 SHOULDER_RIGHT_ID = 78;
constexpr S16 ELBOW_RIGHT_ID = 79;
constexpr S16 WRIST_RIGHT_ID = 80;
constexpr S16 HAND_MIDDLE_RIGHT_1_ID = 81;
constexpr S16 HAND_MIDDLE_RIGHT_2_ID = 82;
constexpr S16 HAND_MIDDLE_RIGHT_3_ID = 83;
constexpr S16 HAND_INDEX_RIGHT_1_ID = 84;
constexpr S16 HAND_INDEX_RIGHT_2_ID = 85;
constexpr S16 HAND_INDEX_RIGHT_3_ID = 86;
constexpr S16 HAND_RING_RIGHT_1_ID = 87;
constexpr S16 HAND_RING_RIGHT_2_ID = 88;
constexpr S16 HAND_RING_RIGHT_3_ID = 89;
constexpr S16 HAND_PINKY_RIGHT_1_ID = 90;
constexpr S16 HAND_PINKY_RIGHT_2_ID = 91;
constexpr S16 HAND_PINKY_RIGHT_3_ID = 92;
constexpr S16 HAND_THUMB_RIGHT_1_ID = 93;
constexpr S16 HAND_THUMB_RIGHT_2_ID = 94;
constexpr S16 HAND_THUMB_RIGHT_3_ID = 95;
#endif //ENABLE_RIGHT_CONSTRAINTS

namespace
{
    constexpr U32 PUPPET_MAX_MSG_BYTES = 255;  // This is the largest possible size event
}

bool gIsSendingAnimationStream = false;

void LLStreamingMotion::SetIsSendingAnimationStream(bool is_sending)
{
    gIsSendingAnimationStream = is_sending;
}

bool LLStreamingMotion::GetIsSendingAnimationStream()
{
    return gIsSendingAnimationStream;
}

void LLStreamingMotion::DelayedEventQueue::addEvent(
        Timestamp remote_timestamp,
        Timestamp local_timestamp,
        const LLPuppetJointEvent& event)
{
    if (mLastRemoteTimestamp != -1)
    {
        // dynamically measure mEventPeriod and mEventJitter
        constexpr F32 DEL = 0.1f;
        Timestamp this_period = remote_timestamp - mLastRemoteTimestamp;
        mEventJitter = (1.0f - DEL) * mEventJitter + DEL * std::abs(mEventPeriod - (F32)this_period);

        // mEventPeriod is a running average of the period between events
        mEventPeriod = (1.0f - DEL) * mEventPeriod + DEL * this_period;
    }
    mLastRemoteTimestamp = remote_timestamp;

    // we push event into the future so we have something to interpolate toward
    // while we wait for the next
    Timestamp delayed_timestamp = local_timestamp + (Timestamp)(mEventPeriod + mEventJitter);
    mQueue.push_back({delayed_timestamp, event});
}

LLStreamingMotion::LLStreamingMotion(const LLUUID &id) :
    LLMotion(id)
{
    mName = "streaming_motion";
    mRemoteToLocalClockOffset = std::numeric_limits<F32>::min();
}

void LLStreamingMotion::collectJoints(LLJoint* joint)
{
    // rather than maintain a map<name, LLJointState>
    // we use a vector<JointState> so joints can be looked up by index
    // which means we must collect ALL the bone joints
    if (!joint->isBone() || !joint->getParent())
    {
        return;
    }

    LLPointer<LLJointState> joint_state = new LLJointState(joint);
    S16 joint_id = joint->getJointNum();
    mJointStates[joint_id] = joint_state;

    // Recurse through the children of this joint and add them to our joint control list
    for (LLJoint::joints_t::iterator itr = joint->mChildren.begin();
         itr != joint->mChildren.end(); ++itr)
    {
        collectJoints(*itr);
    }
}

// override
bool LLStreamingMotion::needsUpdate() const
{
    return !mEventQueues.empty() || LLMotion::needsUpdate();
}

F32 LLStreamingMotion::getEaseInDuration()
{
    return 1.0f;
}

F32 LLStreamingMotion::getEaseOutDuration()
{
    return 1.0f;
}

LLJoint::JointPriority LLStreamingMotion::getPriority()
{
    // Note: getPriority() is only used to delegate motion-wide priority
    // to LLJointStates added to mPose in LLMotion::addJointState()...
    // when they have LLJoint::USE_MOTION_PRIORITY.
    return LLJoint::PUPPET_PRIORITY;
}

LLMotion::LLMotionInitStatus LLStreamingMotion::onInitialize(LLCharacter *character)
{
    assert(character);
    constexpr size_t NUM_JOINTS = 133;
    mJointStates.resize(NUM_JOINTS);
    LLJoint* root_joint = character->getJoint("mPelvis");
    assert(root_joint);
    assert(root_joint->isBone());
    collectJoints(root_joint);
    return STATUS_SUCCESS;
}

void LLStreamingMotion::applyEvent(const LLPuppetJointEvent& event, Timestamp now)
{
    S16 joint_id = event.getJointID();
    if (joint_id > 0 && joint_id < mJointStates.size())
    {
        LLPointer<LLJointState> joint_state = mJointStates[joint_id];
        if (!joint_state)
        {
            return;
        }
        U8 flags = event.getMask();
        joint_state->setUsage((U32)(flags & LLIK::MASK_JOINT_STATE_USAGE));
        if (flags & LLIK::CONFIG_FLAG_LOCAL_POS)
        {
            // we expect received position to be scaled correctly
            // so it can be applied without modification
            joint_state->setPosition(event.getPosition());
        }
        if (flags & LLIK::CONFIG_FLAG_LOCAL_ROT)
        {
            joint_state->setRotation(event.getRotation());
        }
        if (flags & LLIK::CONFIG_FLAG_LOCAL_SCALE)
        {
            joint_state->setScale(event.getScale());
        }
        addJointState(joint_state);
    }
}

void LLStreamingMotion::updateFromQueues(Timestamp now)
{
    // We walk the queue looking for the two bounding events: the last previous
    // and the next pending: we will interpolate between them.  If we don't
    // find bounding events we'll use whatever we've got.
    auto queue_itr = mEventQueues.begin();
    while (queue_itr != mEventQueues.end())
    {
        auto& queue = queue_itr->second.getEventQueue();
        auto event_itr = queue.begin();
        while (event_itr != queue.end())
        {
            Timestamp timestamp = event_itr->first;
            const LLPuppetJointEvent& event = event_itr->second;
            if (timestamp > now)
            {
                // first available event is in the future
                // we have no choice but to apply what we have
                applyEvent(event, now);
                break;
            }

            // event is in the past --> check next event
            ++event_itr;
            if (event_itr == queue.end())
            {
                // we're at the end of the queue
                constexpr Timestamp STALE_QUEUE_DURATION = 3 * MSEC_PER_SEC;
                if (timestamp < now - STALE_QUEUE_DURATION)
                {
                    // this queue is stale
                    // the "remembered pose" will be purged elewhere
                    queue.clear();
                }
                else
                {
                    // presumeably we already interpolated close to this event
                    // but just in case we didn't quite reach it yet: apply
                    applyEvent(event, now);
                }
                break;
            }

            Timestamp next_timestamp = event_itr->first;
            if (next_timestamp < now)
            {
                // event is stale --> drop it
                queue.pop_front();
                event_itr = queue.begin();
                continue;
            }

            // next event is in the future
            // which means we've found the two events that straddle 'now'
            // --> create an interpolated event and apply that
            F32 del = (F32)(now - timestamp) / (F32)(next_timestamp - timestamp);
            LLPuppetJointEvent interpolated_event;
            const LLPuppetJointEvent& next_event = event_itr->second;
            interpolated_event.interpolate(del, event, next_event);
            applyEvent(interpolated_event, now);
            break;
        }
        if (queue.empty())
        {
            queue_itr = mEventQueues.erase(queue_itr);
        }
        else
        {
            ++queue_itr;
        }
    }
}

void LLStreamingMotion::queueEvent(const LLPuppetEvent& puppet_event)
{
    // adjust the timestamp for local clock
    // and push into the future to allow interpolation
    Timestamp remote_timestamp = puppet_event.getTimestamp();
    Timestamp now = (S32)(LLFrameTimer::getElapsedSeconds() * MSEC_PER_SEC);
    Timestamp clock_skew = now - remote_timestamp;
    if (std::numeric_limits<F32>::min() == mRemoteToLocalClockOffset)
    {
        mRemoteToLocalClockOffset = (F32)(clock_skew);
    }
    else
    {
        // compute a running average
        constexpr F32 DEL = 0.05f;
        mRemoteToLocalClockOffset = (1.0 - DEL) * mRemoteToLocalClockOffset + DEL * clock_skew;
    }
    Timestamp local_timestamp = remote_timestamp + (Timestamp)(mRemoteToLocalClockOffset);

    // split puppet_event into joint-specific streams
    for (const auto& joint_event : puppet_event.mJointEvents)
    {
        S16 joint_id = joint_event.getJointID();
        if (joint_id > 0 && joint_id < mJointStates.size() && mJointStates[joint_id])
        {
            DelayedEventQueue& queue = mEventQueues[joint_id];
            queue.addEvent(remote_timestamp, local_timestamp, joint_event);
        }
    }
}

BOOL LLStreamingMotion::onUpdate(F32 time, U8* joint_mask)
{
    if (mJointStates.empty())
    {
        return FALSE;
    }

    Timestamp now = (S32)(LLFrameTimer::getElapsedSeconds() * MSEC_PER_SEC);
    updateFromQueues(now);

    // We must return TRUE else LLMotionController will stop and purge this motion.
    //
    // TODO?: figure out when to return FALSE so that LLMotionController can
    // reduce its idle load.
    return TRUE;
}

BOOL LLStreamingMotion::onActivate()
{
    // LLMotionController calls this when it adds this motion
    // to its active list.  As of 2022.04.21 the return value
    // is never checked.
    return TRUE;
}

void LLStreamingMotion::onDeactivate()
{
    // LLMotionController calls this when it removes
    // this motion from its active list.
    mPose.removeAllJointStates();
    for (auto& joint_state: mJointStates)
    {
        joint_state->setUsage(0);
    }
}

void LLStreamingMotion::unpackEvents(LLMessageSystem *mesgsys,int blocknum)
{
    std::array<U8, PUPPET_MAX_MSG_BYTES> puppet_pack_buffer;

    LLDataPackerBinaryBuffer dataPacker( puppet_pack_buffer.data(), PUPPET_MAX_MSG_BYTES );
    dataPacker.reset();

    S32 data_size = mesgsys->getSizeFast(_PREHASH_PhysicalAvatarEventList, blocknum, _PREHASH_TypeData);
    mesgsys->getBinaryDataFast(_PREHASH_PhysicalAvatarEventList, _PREHASH_TypeData, puppet_pack_buffer.data(), data_size , blocknum,PUPPET_MAX_MSG_BYTES);

    LLPuppetEvent event;
    if (event.unpack(dataPacker))
    {
        queueEvent(event);
    }
    else
    {
        LL_WARNS_ONCE("Puppet") << "Reject invalid animation data" << LL_ENDL;
    }
}

