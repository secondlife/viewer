/**
 * @file llpuppetmotion.h
 * @brief Implementation of LLPuppetMotion class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#pragma once
#ifndef LL_LLPUPPETMOTION_H
#define LL_LLPUPPETMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include <deque>
#include <vector>
#include "llmotion.h"
#include "llframetimer.h"
#include "m3math.h"
#include "v3dmath.h"
#include "lldatapacker.h"
#include "llik.h"
#include "llpuppetevent.h"

constexpr F32 MIN_REQUIRED_PIXEL_AREA_PUPPET = 500.f;
constexpr S32 DISTANT_FUTURE_TIMESTAMP = std::numeric_limits<S32>::max();

class LLViewerRegion;
class LLVOAvatar;

class LLPuppetMotion :
    public LLMotion
{
public:
    using state_map_t = std::map <S16, LLPointer<LLJointState>>;
    using jointid_vec_t = std::vector<S16>;
    using update_deq_t = std::deque<LLPuppetEvent>;
    using joint_events_t = std::vector<LLPuppetJointEvent>;
    using ptr_t = std::shared_ptr<LLPuppetMotion>;
    using Timestamp = S32;

public:
    class JointStateExpiry
    {
    public:
        JointStateExpiry() {}
        JointStateExpiry(LLPointer<LLJointState> state, Timestamp expiry): mState(state), mExpiry(expiry) {}
        LLPointer<LLJointState> mState;
        Timestamp mExpiry = DISTANT_FUTURE_TIMESTAMP;
    };

    using EventQueue = std::deque< std::pair<Timestamp, LLPuppetJointEvent> >;
    class DelayedEventQueue
    {
    public:
        void addEvent(Timestamp remote_timestamp, Timestamp local_timestamp, const LLPuppetJointEvent& event);
        EventQueue& getEventQueue() { return mQueue; }
    private:
        EventQueue mQueue;
        Timestamp mLastRemoteTimestamp = -1; // msec
        // EventPeriod and Jitter are dynamically updated
        // but we start with these optimistic guesses
        F32 mEventPeriod = 100.0f; // msec
        F32 mEventJitter = 50.0f; // msec
    };
    using EventQueues = std::map<S16, DelayedEventQueue>;

    // Constructor
    LLPuppetMotion(const LLUUID &id);

    bool needsUpdate() const override;

    // Destructor
    virtual ~LLPuppetMotion() {}

    void collectJoints(LLJoint* joint);
    void addExpressionEvent(const LLPuppetJointEvent& event);
    void queueOutgoingEvent(const LLPuppetEvent& event);
    void unpackEvents(LLMessageSystem *mesgsys,int blocknum);

    void setAvatar(LLVOAvatar* avatar);
    void clearAll();

public:
    //-------------------------------------------------------------------------
    // functions to support MotionController and MotionRegistry
    //-------------------------------------------------------------------------
    // static constructor
    // all subclasses must implement such a function and register it
    static LLMotion::ptr_t create(const LLUUID &id) { return std::make_shared<LLPuppetMotion>(id); }

    static void RequestPuppetryStatus(LLViewerRegion *regionp);
    static void RequestPuppetryStatusCoro(const std::string& capurl);

    static void SetPuppetryEnabled(bool enabled, size_t event_size);

    static bool GetPuppetryEnabled() { return sIsPuppetryEnabled; }
    static size_t GetPuppetryEventMax() { return sPuppeteerEventMaxSize; }

public:
    //-------------------------------------------------------------------------
    // animation callbacks to be implemented by subclasses
    //-------------------------------------------------------------------------

    // motions must specify whether or not they loop
    virtual BOOL getLoop() override { return FALSE; }

    // motions must report their total duration
    virtual F32 getDuration() override { return 0.0f; }

    // motions must report their "ease in" duration
    virtual F32 getEaseInDuration() override;

    // motions must report their "ease out" duration.
    virtual F32 getEaseOutDuration() override;

    // motions must report their priority
    virtual LLJoint::JointPriority getPriority() override;
    void setPriority(LLJoint::JointPriority priority) { mMotionPriority = priority; }

    virtual LLMotionBlendType getBlendType() override { return NORMAL_BLEND; }

    // called to determine when a motion should be activated/deactivated based on avatar pixel coverage
    virtual F32 getMinPixelArea() override { return MIN_REQUIRED_PIXEL_AREA_PUPPET; }

    // run-time (post constructor) initialization,
    // called after parameters have been set
    // must return true to indicate success and be available for activation
    virtual LLMotionInitStatus onInitialize(LLCharacter *character) override;

    virtual BOOL onActivate() override;

    // called per time step
    // must return TRUE while it is active, and
    // must return FALSE when the motion is completed.
    virtual BOOL onUpdate(F32 time, U8* joint_mask) override;

    virtual void onDeactivate() override;

    BOOL canDeprecate() override { return FALSE; }
    void addJointToSkeletonData(LLSD& skeleton_sd, LLJoint* joint, const LLVector3& parent_rel_pos, const LLVector3& tip_rel_end_pos);
    LLSD getSkeletonData();
    void reconfigureJoints();

private:
    void measureArmSpan();
    void queueEvent(const LLPuppetEvent& event);
    void applyEvent(const LLPuppetJointEvent& event, U64 now, LLIK::Solver::joint_config_map_t& targets);
    void applyBroadcastEvent(const LLPuppetJointEvent& event, Timestamp now, bool local_puppetry);
    void packEvents();
    void pumpOutgoingEvents();
    void solveIKAndHarvestResults(const LLIK::Solver::joint_config_map_t& configs, Timestamp now);
    void updateFromExpression(Timestamp now);
    void updateFromBroadcast(Timestamp now);
    void rememberPosedJoint(S16 joint_id, LLPointer<LLJointState> joint_state, Timestamp now);

private:
    state_map_t             mJointStates;      // joints known to IK
    EventQueues             mEventQueues;
    std::map<S16, JointStateExpiry> mJointStateExpiries;        // recently animated joints and their expiries
    LLCharacter*            mCharacter;
    update_deq_t            mOutgoingEvents;   // LLPuppetEvents to broadcast.
    std::vector< LLPointer<LLJointState> > mJointsToRemoveFromPose;
    LLFrameTimer            mBroadcastTimer;   // When to broadcast events.
    LLFrameTimer            mPlaybackTimer;    // Playback what was broadcast
    joint_events_t          mExpressionEvents;
    LLIK::Solver            mIKSolver;
    LLVOAvatar*             mAvatar = nullptr;
    Timestamp               mNextJointStateExpiry = DISTANT_FUTURE_TIMESTAMP;
    F32                     mRemoteToLocalClockOffset; // msec
    F32                     mArmSpan = 2.0f;
    bool                    mIsSelf = false;

    LLJoint::JointPriority  mMotionPriority = LLJoint::PUPPET_PRIORITY;

    static bool sIsPuppetryEnabled;  // Is puppetry enabled on the simulator.
    static size_t sPuppeteerEventMaxSize;  // Simulator reported maximum size for a puppetry event.
};

#endif
