/** 
 * @file llkeyframemotionparam.h
 * @brief Implementation of LLKeframeMotionParam class.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLKEYFRAMEMOTIONPARAM_H
#define LL_LLKEYFRAMEMOTIONPARAM_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------

#include <string>

#include "llmotion.h"
#include "lljointstate.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llkeyframemotion.h"

//-----------------------------------------------------------------------------
// class LLKeyframeMotionParam
//-----------------------------------------------------------------------------
class LLKeyframeMotionParam :
    public LLMotion
{
public:
    // Constructor
    LLKeyframeMotionParam(const LLUUID &id);

    // Destructor
    virtual ~LLKeyframeMotionParam();

public:
    //-------------------------------------------------------------------------
    // functions to support MotionController and MotionRegistry
    //-------------------------------------------------------------------------

    // static constructor
    // all subclasses must implement such a function and register it
    static LLMotion *create(const LLUUID &id) { return new LLKeyframeMotionParam(id); }

public:
    //-------------------------------------------------------------------------
    // animation callbacks to be implemented by subclasses
    //-------------------------------------------------------------------------

    // motions must specify whether or not they loop
    virtual BOOL getLoop() {
        return TRUE;
    }

    // motions must report their total duration
    virtual F32 getDuration() { 
        return mDuration;
    }

    // motions must report their "ease in" duration
    virtual F32 getEaseInDuration() { 
        return mEaseInDuration;
    }

    // motions must report their "ease out" duration.
    virtual F32 getEaseOutDuration() { 
        return mEaseOutDuration;
    }

    // motions must report their priority
    virtual LLJoint::JointPriority getPriority() { 
        return mPriority;
    }

    virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

    // called to determine when a motion should be activated/deactivated based on avatar pixel coverage
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_KEYFRAME; }

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

    virtual LLPose* getPose() { return mPoseBlender.getBlendedPose();}

protected:
    //-------------------------------------------------------------------------
    // new functions defined by this subclass
    //-------------------------------------------------------------------------
    struct ParameterizedMotion
    {
        ParameterizedMotion(LLMotion* motion, F32 param) : mMotion(motion), mParam(param) {}
        LLMotion* mMotion;
        F32 mParam;
    };
    
    // add a motion and associated parameter triplet
    BOOL addKeyframeMotion(char *name, const LLUUID &id, char *param, F32 value);
    
    // set default motion for LOD and retrieving blend constants
    void setDefaultKeyframeMotion(char *);

    BOOL loadMotions();

protected:
    //-------------------------------------------------------------------------
    // Member Data
    //-------------------------------------------------------------------------

    struct compare_motions
    {
        bool operator() (const ParameterizedMotion& a, const ParameterizedMotion& b) const
        {
            if (a.mParam != b.mParam)
                return (a.mParam < b.mParam);
            else
                return a.mMotion < b.mMotion;
        }
    };
    
    typedef std::set < ParameterizedMotion, compare_motions > motion_list_t;
    typedef std::map <std::string, motion_list_t > motion_map_t;
    motion_map_t        mParameterizedMotions;
    LLMotion*           mDefaultKeyframeMotion;
    LLCharacter*        mCharacter;
    LLPoseBlender       mPoseBlender;

    F32                 mEaseInDuration;
    F32                 mEaseOutDuration;
    F32                 mDuration;
    LLJoint::JointPriority  mPriority;

    LLUUID              mTransactionID;
};

#endif // LL_LLKEYFRAMEMOTIONPARAM_H
