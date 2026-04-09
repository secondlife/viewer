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

#pragma once

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "lljoint.h"
#include "llrefcount.h"

#include "glm/gtc/quaternion.hpp"
#include <glm/vec3.hpp>

//-----------------------------------------------------------------------------
// class LLJointState
//-----------------------------------------------------------------------------
class LLJointState : public LLRefCount
{
public:
    enum class BlendPhase
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
    glm::vec3       mPosition{0.f};  // position relative to parent joint
    glm::quat       mRotation{1.f, 0.f, 0.f, 0.f}; // joint rotation relative to parent joint (identity w,x,y,z)
    glm::vec3       mScale{0.f};     // scale relative to rotated frame
    LLJoint::JointPriority  mPriority;  // how important this joint state is relative to others
public:
    // Constructor
    LLJointState()
        : mUsage(0)
        , mJoint(NULL)
        , mWeight(0.f)
        , mPriority(LLJoint::USE_MOTION_PRIORITY)
    {}

    explicit LLJointState(LLJoint* joint)
        : mUsage(0)
        , mJoint(joint)
        , mWeight(0.f)
        , mPriority(LLJoint::USE_MOTION_PRIORITY)
    {}

    // joint that this state is applied to
    LLJoint* getJoint()             { return mJoint; }
    const LLJoint* getJoint() const { return mJoint; }
    bool setJoint( LLJoint *joint ) { mJoint = joint; return mJoint != NULL; }

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
    LLVector3 getPosition() const                { return LLVector3(mPosition); }
    void setPosition( const LLVector3& pos )    { llassert(mUsage & POS); mPosition = pos; }

    // get/set rotation
    // glm-quat migration (cluster #31): mirrors LLJoint::getRotation() —
    // returns const glm::quat& so callers reference the field directly with
    // no temporary materialization. Caller drift is absorbed by the
    // LLQuaternion(const glm::quat&) bridge ctor and the implicit operator
    // glm::quat() conversion.
    const glm::quat& getRotation() const         { return mRotation; }
    void setRotation( const glm::quat& rot )     { llassert(mUsage & ROT); mRotation = rot; }

    // get/set scale
    LLVector3 getScale() const                  { return LLVector3(mScale); }
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


