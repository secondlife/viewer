/** 
 * @file llhudeffectbeam.h
 * @brief LLHUDEffectBeam class definition
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

#ifndef LL_LLHUDEFFECTBEAM_H
#define LL_LLHUDEFFECTBEAM_H

#include "llhudeffect.h"

#include "llframetimer.h"

#include "llinterp.h"

class LLViewerObject;

const S32 NUM_POINTS = 5;

class LLHUDEffectBeam : public LLHUDEffect
{
public:
    /*virtual*/ void setSourceObject(LLViewerObject *objp);

    // A beam can have either a target object or a target position
    void setTargetObject(LLViewerObject *objp);
    void setTargetPos(const LLVector3d &target_pos_global);

    friend class LLHUDObject;
protected:
    LLHUDEffectBeam(const U8 type);
    ~LLHUDEffectBeam();

    /*virtual*/ void render();
    /*virtual*/ void renderForTimer();
    /*virtual*/ void packData(LLMessageSystem *mesgsys);
    /*virtual*/ void unpackData(LLMessageSystem *mesgsys, S32 blocknum);
private:
    void setupParticle(const S32 i);

    
    F32 mKillTime;
    LLFrameTimer mTimer;
    LLInterpLinear<LLVector3d> mInterp[NUM_POINTS];
    LLInterpLinear<F32> mInterpFade[NUM_POINTS];
    LLInterpLinear<F32> mFadeInterp;
    LLVector3d  mTargetPos;
};

#endif // LL_LLHUDEFFECTBEAM_H
