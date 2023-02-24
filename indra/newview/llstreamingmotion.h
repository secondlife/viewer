/**
 * @file llstreamingmotion.h
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

#pragma once

//#include <deque>
//#include <vector>
#include "llmotion.h"
#include "m3math.h"
#include "v3dmath.h"
#include "llpuppetevent.h"

constexpr F32 MIN_REQUIRED_PIXEL_AREA_STREAMING = 100.f;

class LLCharacter;

class LLStreamingMotion :
    public LLMotion
{
public:

    //using state_map_t = std::map <S16, LLPointer<LLJointState>>;
    using state_vector_t = std::vector< LLPointer<LLJointState> >;
    //using jointid_vec_t = std::vector<S16>;
    //using update_deq_t = std::deque<LLPuppetEvent>;
    //using joint_events_t = std::vector<LLPuppetJointEvent>;
    using ptr_t = std::shared_ptr<LLStreamingMotion>;
    using Timestamp = S32;

public:
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
    LLStreamingMotion(const LLUUID &id);

    void collectJoints(LLJoint* joint);
    bool needsUpdate() const override;

    // Destructor
    virtual ~LLStreamingMotion() {}

    void unpackEvents(LLMessageSystem *mesgsys,int blocknum);

public:
    // LLMotion subclasses must implement create() factory method
    static LLMotion::ptr_t create(const LLUUID &id) { return std::make_shared<LLStreamingMotion>(id); }

public:
    // required overrides

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

    virtual LLMotionBlendType getBlendType() override { return NORMAL_BLEND; }

    // called to determine when a motion should be activated/deactivated based on avatar pixel coverage
    virtual F32 getMinPixelArea() override { return MIN_REQUIRED_PIXEL_AREA_STREAMING; }

    // run-time (post constructor) initialization,
    // called after parameters have been set
    // must return true to indicate success and be available for activation
    virtual LLMotionInitStatus onInitialize(LLCharacter* character) override;

    virtual BOOL onActivate() override;

    // called per time step
    // must return TRUE while it is active, and
    // must return FALSE when the motion is completed.
    virtual BOOL onUpdate(F32 time, U8* joint_mask) override;

    virtual void onDeactivate() override;

    BOOL canDeprecate() override { return FALSE; }

private:
    void queueEvent(const LLPuppetEvent& event);
    void applyEvent(const LLPuppetJointEvent& event, Timestamp now);

    void updateFromQueues(Timestamp now);

private:
    state_vector_t mJointStates;
    EventQueues mEventQueues;
    LLCharacter* mCharacter;
    F32 mRemoteToLocalClockOffset; // msec
};

