/** 
 * @file lljointstate.h
 * @brief Implementation of LLJointState class.
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

#ifndef LL_LLJOINTSTATE_H
#define LL_LLJOINTSTATE_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "lljoint.h"
#include "llrefcount.h"

//-----------------------------------------------------------------------------
// class LLJointState
//-----------------------------------------------------------------------------
class LLJointState : public LLRefCount
{
public:
    enum BlendPhase
    {
        INACTIVE,
        EASE_IN,
        ACTIVE,
        EASE_OUT
    };
protected:
    // associated joint
    LLJoint *mJoint;

    // indicates which members are used
    U32     mUsage;

    // indicates weighted effect of this joint 
    F32     mWeight;

    // transformation members
    LLVector3       mPosition;  // position relative to parent joint
    LLQuaternion    mRotation;  // joint rotation relative to parent joint
    LLVector3       mScale;     // scale relative to rotated frame
    LLJoint::JointPriority  mPriority;  // how important this joint state is relative to others
public:
    // Constructor
    LLJointState()
        : mUsage(0)
        , mJoint(NULL)
        , mWeight(0.f)
        , mPriority(LLJoint::USE_MOTION_PRIORITY)
    {}

    LLJointState(LLJoint* joint)
        : mUsage(0)
        , mJoint(joint)
        , mWeight(0.f)
        , mPriority(LLJoint::USE_MOTION_PRIORITY)
    {}

    // joint that this state is applied to
    LLJoint* getJoint()             { return mJoint; }
    const LLJoint* getJoint() const { return mJoint; }
    BOOL setJoint( LLJoint *joint ) { mJoint = joint; return mJoint != NULL; }

    // transform type (bitwise flags can be combined)
    // Note that these are set automatically when various
    // member setPos/setRot/setScale functions are called.
    enum Usage
    {
        POS     = 1,
        ROT     = 2,
        SCALE   = 4,
    };
    U32 getUsage() const            { return mUsage; }
    void setUsage( U32 usage )      { mUsage = usage; }
    F32 getWeight() const           { return mWeight; }
    void setWeight( F32 weight )    { mWeight = weight; }

    // get/set position
    const LLVector3& getPosition() const        { return mPosition; }
    void setPosition( const LLVector3& pos )    { llassert(mUsage & POS); mPosition = pos; }

    // get/set rotation
    const LLQuaternion& getRotation() const     { return mRotation; }
    void setRotation( const LLQuaternion& rot ) { llassert(mUsage & ROT); mRotation = rot; }

    // get/set scale
    const LLVector3& getScale() const           { return mScale; }
    void setScale( const LLVector3& scale )     { llassert(mUsage & SCALE); mScale = scale; }

    // get/set priority
    LLJoint::JointPriority getPriority() const  { return mPriority; }
    void setPriority( LLJoint::JointPriority priority ) { mPriority = priority; }

protected:
    // Destructor
    virtual ~LLJointState()
    {
    }
    
};

#endif // LL_LLJOINTSTATE_H

