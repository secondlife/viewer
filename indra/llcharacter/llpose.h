/**
 * @file llpose.h
 * @brief Implementation of LLPose class.
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

#include "lljointstate.h"
#include "lljoint.h"
#include "llpointer.h"

#include <array>
#include <map>
#include <string>


//-----------------------------------------------------------------------------
// class LLPose
//-----------------------------------------------------------------------------
class LLPose
{
    friend class LLPoseBlender;
protected:
    using joint_map = std::map<std::string, LLPointer<LLJointState> >;
    using joint_map_iterator = joint_map::iterator;
    using joint_map_value_type = joint_map::value_type;

    joint_map                   mJointMap;
    F32                         mWeight;
    joint_map_iterator          mListIter = {};
public:
    // Iterate through jointStates
    LLJointState* getFirstJointState();
    LLJointState* getNextJointState();
    LLJointState* findJointState(LLJoint *joint);
    LLJointState* findJointState(const std::string &name);
public:
    // Constructor
    LLPose() : mWeight(0.f) {}
    // Destructor
    ~LLPose();
    // add a joint state in this pose
    bool addJointState(const LLPointer<LLJointState>& jointState);
    // remove a joint state from this pose
    bool removeJointState(const LLPointer<LLJointState>& jointState);
    // removes all joint states from this pose
    bool removeAllJointStates();
    // set weight for all joint states in this pose
    void setWeight(F32 weight);
    // get weight for this pose
    F32 getWeight() const;
    // returns number of joint states stored in this pose
    S32 getNumJointStates() const;
};

const S32 JSB_NUM_JOINT_STATES = 6;

LL_ALIGN_PREFIX(16)
class LLJointStateBlender
{
    LL_ALIGN_NEW
protected:
    std::array<LLPointer<LLJointState>, JSB_NUM_JOINT_STATES> mJointStates;
    std::array<S32, JSB_NUM_JOINT_STATES>             mPriorities;
    std::array<bool, JSB_NUM_JOINT_STATES>            mAdditiveBlends;
public:
    LLJointStateBlender();
    ~LLJointStateBlender();
    void blendJointStates(bool apply_now = true);
    bool addJointState(const LLPointer<LLJointState>& joint_state, S32 priority, bool additive_blend);
    void interpolate(F32 u);
    void clear();
    void resetCachedJoint();

public:
    LL_ALIGN_16(LLJoint mJointCache);
} LL_ALIGN_POSTFIX(16);

class LLMotion;

class LLPoseBlender
{
protected:
    using blender_list_t = std::list<LLJointStateBlender*>;
    using blender_map_t = std::map<LLJoint*,LLJointStateBlender*>;
    blender_map_t mJointStateBlenderPool;
    blender_list_t mActiveBlenders;

    S32         mNextPoseSlot;
    LLPose      mBlendedPose;
public:
    // Constructor
    LLPoseBlender();
    // Destructor
    ~LLPoseBlender();

    // request motion joint states to be added to pose blender joint state records
    bool addMotion(LLMotion* motion);

    // blend all joint states and apply to skeleton
    void blendAndApply();

    // removes all joint state blenders from last time
    void clearBlenders();

    // blend all joint states and cache results
    void blendAndCache(bool reset_cached_joints);

    // interpolate all joints towards cached values
    void interpolate(F32 u);

    LLPose* getBlendedPose() { return &mBlendedPose; }
};


