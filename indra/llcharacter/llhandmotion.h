/** 
 * @file llhandmotion.h
 * @brief Implementation of LLHandMotion class.
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

#ifndef LL_LLHANDMOTION_H
#define LL_LLHANDMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llmotion.h"
#include "lltimer.h"

#define MIN_REQUIRED_PIXEL_AREA_HAND 10000.f;

//-----------------------------------------------------------------------------
// class LLHandMotion
//-----------------------------------------------------------------------------
class LLHandMotion :
    public LLMotion
{
public:
    typedef enum e_hand_pose
    {
        HAND_POSE_SPREAD,
        HAND_POSE_RELAXED,
        HAND_POSE_POINT,
        HAND_POSE_FIST,
        HAND_POSE_RELAXED_L,
        HAND_POSE_POINT_L,
        HAND_POSE_FIST_L,
        HAND_POSE_RELAXED_R,
        HAND_POSE_POINT_R,
        HAND_POSE_FIST_R,
        HAND_POSE_SALUTE_R,
        HAND_POSE_TYPING,
        HAND_POSE_PEACE_R,
        HAND_POSE_PALM_R,
        NUM_HAND_POSES
    } eHandPose;

    // Constructor
    LLHandMotion(const LLUUID &id);

    // Destructor
    virtual ~LLHandMotion();

public:
    //-------------------------------------------------------------------------
    // functions to support MotionController and MotionRegistry
    //-------------------------------------------------------------------------

    // static constructor
    // all subclasses must implement such a function and register it
    static LLMotion *create(const LLUUID &id) { return new LLHandMotion(id); }

public:
    //-------------------------------------------------------------------------
    // animation callbacks to be implemented by subclasses
    //-------------------------------------------------------------------------

    // motions must specify whether or not they loop
    virtual BOOL getLoop() { return TRUE; }

    // motions must report their total duration
    virtual F32 getDuration() { return 0.0; }

    // motions must report their "ease in" duration
    virtual F32 getEaseInDuration() { return 0.0; }

    // motions must report their "ease out" duration.
    virtual F32 getEaseOutDuration() { return 0.0; }

    // called to determine when a motion should be activated/deactivated based on avatar pixel coverage
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_HAND; }

    // motions must report their priority
    virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }

    virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

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

    virtual BOOL canDeprecate() { return FALSE; }

    static std::string getHandPoseName(eHandPose pose);
    static eHandPose getHandPose(std::string posename);

public:
    //-------------------------------------------------------------------------
    // joint states to be animated
    //-------------------------------------------------------------------------

    LLCharacter         *mCharacter;

    F32                 mLastTime;
    eHandPose           mCurrentPose;
    eHandPose           mNewPose;
};
#endif // LL_LLHANDMOTION_H

