/**
 * @file llkeyframewalkmotion.h
 * @brief Implementation of LLKeframeWalkMotion class.
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

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llkeyframemotion.h"
#include "llcharacter.h"
#include "v3dmath.h"

#define MIN_REQUIRED_PIXEL_AREA_WALK_ADJUST (20.f)
#define MIN_REQUIRED_PIXEL_AREA_FLY_ADJUST (20.f)

//-----------------------------------------------------------------------------
// class LLKeyframeWalkMotion
//-----------------------------------------------------------------------------
class LLKeyframeWalkMotion :
    public LLKeyframeMotion
{
    friend class LLWalkAdjustMotion;
public:
    // Constructor
    explicit LLKeyframeWalkMotion(const LLUUID &id);

    // Destructor
    ~LLKeyframeWalkMotion() override;

public:
    //-------------------------------------------------------------------------
    // functions to support MotionController and MotionRegistry
    //-------------------------------------------------------------------------

    // static constructor
    // all subclasses must implement such a function and register it
    static LLMotion *create(const LLUUID &id) { return new LLKeyframeWalkMotion(id); }

public:
    //-------------------------------------------------------------------------
    // animation callbacks to be implemented by subclasses
    //-------------------------------------------------------------------------
    LLMotionInitStatus onInitialize(LLCharacter *character) override;
    bool onActivate() override;
    void onDeactivate() override;
    bool onUpdate(F32 time, U8* joint_mask) override;

public:
    //-------------------------------------------------------------------------
    // Member Data
    //-------------------------------------------------------------------------
    LLCharacter *mCharacter;
    F32         mCyclePhase;
    F32         mRealTimeLast;
    F32         mAdjTimeLast;
    S32         mDownFoot;
};

class LLWalkAdjustMotion : public LLMotion
{
public:
    // Constructor
    explicit LLWalkAdjustMotion(const LLUUID &id);

public:
    //-------------------------------------------------------------------------
    // functions to support MotionController and MotionRegistry
    //-------------------------------------------------------------------------

    // static constructor
    // all subclasses must implement such a function and register it
    static LLMotion *create(const LLUUID &id) { return new LLWalkAdjustMotion(id); }

public:
    //-------------------------------------------------------------------------
    // animation callbacks to be implemented by subclasses
    //-------------------------------------------------------------------------
    LLMotionInitStatus onInitialize(LLCharacter *character) override;
    bool onActivate() override;
    void onDeactivate() override;
    bool onUpdate(F32 time, U8* joint_mask) override;
    LLJoint::JointPriority getPriority() override {return LLJoint::HIGH_PRIORITY;}
    bool getLoop() override { return true; }
    F32 getDuration() override { return 0.f; }
    F32 getEaseInDuration() override { return 0.f; }
    F32 getEaseOutDuration() override { return 0.f; }
    F32 getMinPixelArea() override { return MIN_REQUIRED_PIXEL_AREA_WALK_ADJUST; }
    LLMotionBlendType getBlendType() override { return LLMotionBlendType::ADDITIVE_BLEND; }

public:
    //-------------------------------------------------------------------------
    // Member Data
    //-------------------------------------------------------------------------
    LLCharacter     *mCharacter = nullptr;
    LLJoint*        mLeftAnkleJoint = nullptr;
    LLJoint*        mRightAnkleJoint = nullptr;
    LLPointer<LLJointState> mPelvisState;
    LLJoint*        mPelvisJoint = nullptr;
    LLVector3d      mLastLeftFootGlobalPos;
    LLVector3d      mLastRightFootGlobalPos;
    F32             mLastTime;
    F32             mAdjustedSpeed;
    F32             mAnimSpeed;
    F32             mRelativeDir;
    LLVector3       mPelvisOffset;
    F32             mAnkleOffset;
};

class LLFlyAdjustMotion : public LLMotion
{
public:
    // Constructor
    explicit LLFlyAdjustMotion(const LLUUID &id);

public:
    //-------------------------------------------------------------------------
    // functions to support MotionController and MotionRegistry
    //-------------------------------------------------------------------------

    // static constructor
    // all subclasses must implement such a function and register it
    static LLMotion *create(const LLUUID &id) { return new LLFlyAdjustMotion(id); }

public:
    //-------------------------------------------------------------------------
    // animation callbacks to be implemented by subclasses
    //-------------------------------------------------------------------------
    LLMotionInitStatus onInitialize(LLCharacter *character) override;
    bool onActivate() override;
    void onDeactivate() override {};
    bool onUpdate(F32 time, U8* joint_mask) override;
    LLJoint::JointPriority getPriority() override {return LLJoint::HIGHER_PRIORITY;}
    bool getLoop() override { return true; }
    F32 getDuration() override { return 0.f; }
    F32 getEaseInDuration() override { return 0.f; }
    F32 getEaseOutDuration() override { return 0.f; }
    F32 getMinPixelArea() override { return MIN_REQUIRED_PIXEL_AREA_FLY_ADJUST; }
    LLMotionBlendType getBlendType() override { return LLMotionBlendType::ADDITIVE_BLEND; }

protected:
    //-------------------------------------------------------------------------
    // Member Data
    //-------------------------------------------------------------------------
    LLCharacter     *mCharacter = nullptr;
    LLPointer<LLJointState> mPelvisState;
    F32             mRoll;
};


