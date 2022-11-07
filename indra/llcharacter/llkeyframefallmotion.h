/** 
 * @file llkeyframefallmotion.h
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

#ifndef LL_LLKeyframeFallMotion_H
#define LL_LLKeyframeFallMotion_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llkeyframemotion.h"
#include "llcharacter.h"

//-----------------------------------------------------------------------------
// class LLKeyframeFallMotion
//-----------------------------------------------------------------------------
class LLKeyframeFallMotion :
    public LLKeyframeMotion
{
public:
    // Constructor
    LLKeyframeFallMotion(const LLUUID &id);

    // Destructor
    virtual ~LLKeyframeFallMotion();

public:
    //-------------------------------------------------------------------------
    // functions to support MotionController and MotionRegistry
    //-------------------------------------------------------------------------

    // static constructor
    // all subclasses must implement such a function and register it
    static LLMotion *create(const LLUUID &id) { return new LLKeyframeFallMotion(id); }

public:
    //-------------------------------------------------------------------------
    // animation callbacks to be implemented by subclasses
    //-------------------------------------------------------------------------
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);
    virtual BOOL onActivate();
    virtual F32 getEaseInDuration();
    virtual BOOL onUpdate(F32 activeTime, U8* joint_mask);

protected:
    //-------------------------------------------------------------------------
    // Member Data
    //-------------------------------------------------------------------------
    LLCharacter*    mCharacter;
    F32             mVelocityZ;
    LLPointer<LLJointState> mPelvisState;
    LLQuaternion    mRotationToGroundNormal;
};

#endif // LL_LLKeyframeFallMotion_H

