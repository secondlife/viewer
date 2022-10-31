/** 
 * @file llkeyframestandmotion.h
 * @brief Implementation of LLKeyframeStandMotion class.
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

#ifndef LL_LLKEYFRAMESTANDMOTION_H
#define LL_LLKEYFRAMESTANDMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llkeyframemotion.h"
#include "lljointsolverrp3.h"


//-----------------------------------------------------------------------------
// class LLKeyframeStandMotion
//-----------------------------------------------------------------------------
LL_ALIGN_PREFIX(16)
class LLKeyframeStandMotion :
    public LLKeyframeMotion
{
    LL_ALIGN_NEW
public:
    // Constructor
    LLKeyframeStandMotion(const LLUUID &id);

    // Destructor
    virtual ~LLKeyframeStandMotion();

public:
    //-------------------------------------------------------------------------
    // functions to support MotionController and MotionRegistry
    //-------------------------------------------------------------------------

    // static constructor
    // all subclasses must implement such a function and register it
    static LLMotion *create(const LLUUID &id) { return new LLKeyframeStandMotion(id); }

public:
    //-------------------------------------------------------------------------
    // animation callbacks to be implemented by subclasses
    //-------------------------------------------------------------------------
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);
    virtual BOOL onActivate();
    void    onDeactivate();
    virtual BOOL onUpdate(F32 time, U8* joint_mask);

public:
    //-------------------------------------------------------------------------
    // Member Data
    //-------------------------------------------------------------------------
    LLJoint             mPelvisJoint;

    LLJoint             mHipLeftJoint;
    LLJoint             mKneeLeftJoint;
    LLJoint             mAnkleLeftJoint;
    LLJoint             mTargetLeft;

    LLJoint             mHipRightJoint;
    LLJoint             mKneeRightJoint;
    LLJoint             mAnkleRightJoint;
    LLJoint             mTargetRight;

    LLCharacter *mCharacter;

    BOOL                mFlipFeet;

    LLPointer<LLJointState> mPelvisState;

    LLPointer<LLJointState> mHipLeftState;
    LLPointer<LLJointState> mKneeLeftState;
    LLPointer<LLJointState> mAnkleLeftState;

    LLPointer<LLJointState> mHipRightState;
    LLPointer<LLJointState> mKneeRightState;
    LLPointer<LLJointState> mAnkleRightState;

    LLJointSolverRP3    mIKLeft;
    LLJointSolverRP3    mIKRight;

    LLVector3           mPositionLeft;
    LLVector3           mPositionRight;
    LLVector3           mNormalLeft;
    LLVector3           mNormalRight;
    LLQuaternion        mRotationLeft;
    LLQuaternion        mRotationRight;

    LLQuaternion        mLastGoodPelvisRotation;
    LLVector3           mLastGoodPosition;
    BOOL                mTrackAnkles;

    S32                 mFrameNum;
} LL_ALIGN_POSTFIX(16);

#endif // LL_LLKEYFRAMESTANDMOTION_H

