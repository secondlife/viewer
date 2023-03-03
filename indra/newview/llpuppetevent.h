/** 
 * @file llpuppetevent.h
 * @brief Implementation of LLPuppetEvent class.
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
#ifndef LL_LLPUPPETEVENT_H
#define LL_LLPUPPETEVENT_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include <deque>
#include <vector>
#include "lldatapacker.h"
#include "llframetimer.h"
#include "llik.h"
#include "llquaternion.h"
#include "v3math.h"
#include "llframetimer.h"

class LLPuppetJointEvent
{
    //Information about an expression event that we want to broadcast
public:
    enum E_REFERENCE_FRAME
    {
        ROOT_FRAME = 0,
        PARENT_FRAME = 1
    };

public:
    // BOOKMARK: give LLPuppetJointEvent an mReferenceFrame?
    LLPuppetJointEvent() {}

    void fromJoint(LLJoint * jointp, LLJoint * pelvisp, E_REFERENCE_FRAME frame);

    void setReferenceFrame(E_REFERENCE_FRAME frame) { mRefFrame = frame; }
    void setRotation(const LLQuaternion& rotation);
    void setPosition(const LLVector3& position);
    void setScale(const LLVector3& scale);
    void setJointID(S32 id);
    void setChainLimit(U8 limit);
    void disableConstraint() { mMask |= LLIK::CONFIG_FLAG_DISABLE_CONSTRAINT; }
    void forceMask(U8 mask) { mMask = mask;}

    S16 getJointID() const { return mJointID; }
    LLQuaternion getRotation() const { return mRotation; }
    LLVector3 getPosition() const { return mPosition; }
    LLVector3 getScale() const { return mScale; }
    E_REFERENCE_FRAME getReferenceFrame( ) const { return mRefFrame; }
    U8 getChainLimit() const { return mChainLimit; }
    bool hasChainLimit() const { return (mChainLimit != 0); }

    size_t getSize() const;
    size_t pack(U8* wptr);     // non-const:  will quantize values
    size_t unpack(U8* wptr);

    void interpolate(F32 del, const LLPuppetJointEvent& A, const LLPuppetJointEvent& B);

    bool isEmpty() const { return mMask == 0; }

    U8 getMask() const { return mMask; }

private:
    E_REFERENCE_FRAME mRefFrame = ROOT_FRAME;
    LLQuaternion mRotation;
    LLVector3 mPosition;
    LLVector3 mScale;
    U16 mJointID = -1;
    U8 mChainLimit = 0;
    U8 mMask = 0x0;
};

class LLPuppetEvent
{
    //An event is snapshot at mTimestamp (msec from start)
    //with 1 or more joints that have moved or rotated
    //These snapshots along with the time delta are used to
    //reconstruct the animation on the receiving clients.
public:
    typedef std::deque<LLPuppetJointEvent> joint_deq_t;

public:
    LLPuppetEvent() {}
    void addJointEvent(const LLPuppetJointEvent& joint_event);
    bool pack(LLDataPackerBinaryBuffer& buffer, S32& out_num_joints);
    bool unpack(LLDataPackerBinaryBuffer& mesgsys);

    // for outbound LLPuppetEvents
    void updateTimestamp() { mTimestamp = (S32)(LLFrameTimer::getElapsedSeconds() * MSEC_PER_SEC); }

    // for inbound LLPuppetEvents we compute a localized timestamp and slam it
    void setTimestamp(S32 timestamp) { mTimestamp = timestamp; }

    S32 getTimestamp() const { return mTimestamp; }
    U32 getNumJoints() const { return mJointEvents.size(); }
    S32 getMinEventSize() const;

public:
    joint_deq_t mJointEvents;

private:
    S32 mTimestamp = 0; // msec
};

/* Puppet Strings: Control events for puppetry, Generally sent from the simulator, but 
* could in theory be adapted to arrive from other sources. These generates LLPuppetJointEvents
* that are then fed into the IK system.
*/
class LLPuppetControl
{
public:
    // Animation control flags as sent from the simulator. 
    static constexpr U32 PUPPET_POS_LOC     = 0b00000000000000000000000000000001; // position is valid and relative
    static constexpr U32 PUPPET_POS_ABS     = 0b00000000000000000000000000000010; // position is valid and absolute
    static constexpr U32 PUPPET_ROT_LOC     = 0b00000000000000000000000000000100; // rotation is valid and relative
    static constexpr U32 PUPPET_ROT_ABS     = 0b00000000000000000000000000001000; // rotation is valid and absolute
    static constexpr U32 PUPPET_IGNORE_IK   = 0b00000000000000000000000000010000; // Ignore IK constraints
    static constexpr U32 PUPPET_EASEIN      = 0b00000000000000000000000000100000; // Ease in is valid
    static constexpr U32 PUPPET_HOLD        = 0b00000000000000000000000001000000; // Hold time is valid
    static constexpr U32 PUPPET_EASEOUT     = 0b00000000000000000000000010000000; // Ease out is  valid.
    static constexpr U32 PUPPET_CHAIN_LEN   = 0b00000000000000000000000100000000; // Chain length was provide in the message.
    static constexpr U32 PUPPET_POS_ATTCH   = 0b00000000000000000000001000000000;
    static constexpr U32 PUPPET_POS_TARGET  = 0b00000000000000000000010000000000;
    static constexpr U32 PUPPET_ROT_ATTCH   = 0b00000000000000000010000000000000;
    static constexpr U32 PUPPET_ROT_TARGET  = 0b00000000000000000100000000000000;
    static constexpr U32 PUPPET_TARGET_AVI  = 0b00000000000000100000000000000000;

    static constexpr U32 PUPPET_POSITION = (PUPPET_POS_LOC | PUPPET_POS_ABS | PUPPET_POS_ATTCH | PUPPET_POS_TARGET);
    static constexpr U32 PUPPET_ROTATION = (PUPPET_ROT_LOC | PUPPET_ROT_ABS | PUPPET_ROT_ATTCH | PUPPET_ROT_TARGET);
    static constexpr U32 PUPPET_LOCAL =    (PUPPET_POS_LOC | PUPPET_ROT_LOC);
    static constexpr U32 PUPPET_ABSOLUTE = (PUPPET_POS_ABS | PUPPET_ROT_ABS);

    enum PhaseID
    {
        EASEIN = 0,
        HOLD = 1,
        EASEOUT = 2,
        DONE
    };
    inline friend PhaseID &operator++(PhaseID &phase) 
    {
        if (phase == DONE)
            return phase;
        phase = static_cast<PhaseID>(static_cast<unsigned int>(phase)+1);
        return phase;
    }

    LLPuppetControl() {}
    LLPuppetControl(U8 attachment_point);
    LLPuppetControl(const LLPuppetControl &) = default;

    void            setFlags(U32 sim_flags);
    void            setEaseIn(F32 now, F32 time, U8 method);
    void            setHold(F32 now, F32 time, bool clear_prev);
    void            setEaseOut(F32 now, F32 time, U8 method, bool clear_prev);
    void            setTrackingAttachmentPnt(U8 tracking_attch);

    PhaseID         getPhaseID() const;
    bool            generateEventAt(F32 now, LLPuppetJointEvent &event_out);
    bool            updateTargetEvent();

    S32             mAttachmentPoint    { 0 };
    S32             mTargetAttachment   { 0 };
    U32             mFlags              { 0 };
    U32             mControlFlags       { 0 };
    LLVector3       mTargetPosition;
    LLQuaternion    mTargetRotation;
    U8              mChainLength        { 0 };

private:
    struct PhaseDef
    {
        F32 mDuration { 0.0f };
        U32 mMethod   { 0 };

        void reset() { mDuration = 0.0f; mMethod = 0; }
    };

    void                    setAnimationPhase(PhaseID phase, F32 time);
    PhaseID                 advanceAnimationPhase(F32 now);

    PhaseID                 mCurentPhase { DONE };
    std::array<PhaseDef, 3> mPhaseDef;
    F32                     mPhaseStartTime { 0.0f };

    LLPuppetJointEvent      mEventTarget;

    LLJoint *               mAttachmentJoint{ nullptr };
    LLJoint *               mParentJoint    { nullptr };
    LLJoint *               mPelvis         { nullptr };
    LLJoint *               mTrackingAttach { nullptr };
};

#endif
