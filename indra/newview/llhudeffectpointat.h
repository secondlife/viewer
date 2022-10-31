/** 
 * @file llhudeffectpointat.h
 * @brief LLHUDEffectPointAt class definition
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

#ifndef LL_LLHUDEFFECTPOINTAT_H
#define LL_LLHUDEFFECTPOINTAT_H

#include "llframetimer.h"
#include "llhudeffect.h"

class LLViewerObject;
class LLVOAvatar;

typedef enum e_pointat_type
{
    POINTAT_TARGET_NONE,
    POINTAT_TARGET_SELECT,
    POINTAT_TARGET_GRAB,
    POINTAT_TARGET_CLEAR,
    POINTAT_NUM_TARGETS
} EPointAtType;

class LLHUDEffectPointAt : public LLHUDEffect
{
public:
    friend class LLHUDObject;

    /*virtual*/ void markDead();
    /*virtual*/ void setSourceObject(LLViewerObject* objectp);

    BOOL setPointAt(EPointAtType target_type, LLViewerObject *object, LLVector3 position);
    void clearPointAtTarget();

    EPointAtType getPointAtType() { return mTargetType; }
    const LLVector3& getPointAtPosAgent() { return mTargetPos; }
    const LLVector3d getPointAtPosGlobal();
protected:
    LLHUDEffectPointAt(const U8 type);
    ~LLHUDEffectPointAt();

    /*virtual*/ void render();
    /*virtual*/ void packData(LLMessageSystem *mesgsys);
    /*virtual*/ void unpackData(LLMessageSystem *mesgsys, S32 blocknum);

    // lookat behavior has either target position or target object with offset
    void setTargetObjectAndOffset(LLViewerObject *objp, LLVector3d offset);
    void setTargetPosGlobal(const LLVector3d &target_pos_global);
    bool calcTargetPosition();
    void update();
public:
    static BOOL sDebugPointAt;
private:
    EPointAtType                mTargetType;
    LLVector3d                  mTargetOffsetGlobal;
    LLVector3                   mLastSentOffsetGlobal;
    F32                         mKillTime;
    LLFrameTimer                mTimer;
    LLVector3                   mTargetPos;
    F32                         mLastSendTime;
};

#endif // LL_LLHUDEFFECTPOINTAT_H
