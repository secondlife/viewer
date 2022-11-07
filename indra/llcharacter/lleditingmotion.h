/** 
 * @file lleditingmotion.h
 * @brief Implementation of LLEditingMotion class.
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

#ifndef LL_LLEDITINGMOTION_H
#define LL_LLEDITINGMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llmotion.h"
#include "lljointsolverrp3.h"
#include "v3dmath.h"

#define EDITING_EASEIN_DURATION 0.0f
#define EDITING_EASEOUT_DURATION 0.5f
#define EDITING_PRIORITY LLJoint::HIGH_PRIORITY
#define MIN_REQUIRED_PIXEL_AREA_EDITING 500.f

//-----------------------------------------------------------------------------
// class LLEditingMotion
//-----------------------------------------------------------------------------
LL_ALIGN_PREFIX(16)
class LLEditingMotion :
    public LLMotion
{
    LL_ALIGN_NEW
public:
    // Constructor
    LLEditingMotion(const LLUUID &id);

    // Destructor
    virtual ~LLEditingMotion();

public:
    //-------------------------------------------------------------------------
    // functions to support MotionController and MotionRegistry
    //-------------------------------------------------------------------------

    // static constructor
    // all subclasses must implement such a function and register it
    static LLMotion *create(const LLUUID &id) { return new LLEditingMotion(id); }

public:
    //-------------------------------------------------------------------------
    // animation callbacks to be implemented by subclasses
    //-------------------------------------------------------------------------

    // motions must specify whether or not they loop
    virtual BOOL getLoop() { return TRUE; }

    // motions must report their total duration
    virtual F32 getDuration() { return 0.0; }

    // motions must report their "ease in" duration
    virtual F32 getEaseInDuration() { return EDITING_EASEIN_DURATION; }

    // motions must report their "ease out" duration.
    virtual F32 getEaseOutDuration() { return EDITING_EASEOUT_DURATION; }

    // motions must report their priority
    virtual LLJoint::JointPriority getPriority() { return EDITING_PRIORITY; }

    virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

    // called to determine when a motion should be activated/deactivated based on avatar pixel coverage
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_EDITING; }

    // run-time (post constructor) initialization,
    // called after parameters have been set
    // must return true to indicate success and be available for activation
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);

    // called when a motion is activated
    // must return TRUE to indicate success, or else
    // it will be deactivated
    virtual BOOL onActivate();

    // called per time step
    // must return TRUE while it is active, and
    // must return FALSE when the motion is completed.
    virtual BOOL onUpdate(F32 time, U8* joint_mask);

    // called when a motion is deactivated
    virtual void onDeactivate();

public:
    //-------------------------------------------------------------------------
    // joint states to be animated
    //-------------------------------------------------------------------------
    LL_ALIGN_16(LLJoint             mParentJoint);
    LL_ALIGN_16(LLJoint             mShoulderJoint);
    LL_ALIGN_16(LLJoint             mElbowJoint);
    LL_ALIGN_16(LLJoint             mWristJoint);
    LL_ALIGN_16(LLJoint             mTarget);
    LLJointSolverRP3    mIKSolver;

    LLCharacter         *mCharacter;
    LLVector3           mWristOffset;

    LLPointer<LLJointState> mParentState;
    LLPointer<LLJointState> mShoulderState;
    LLPointer<LLJointState> mElbowState;
    LLPointer<LLJointState> mWristState;
    LLPointer<LLJointState> mTorsoState;

    static S32          sHandPose;
    static S32          sHandPosePriority;
    LLVector3           mLastSelectPt;
} LL_ALIGN_POSTFIX(16);

#endif // LL_LLKEYFRAMEMOTION_H

