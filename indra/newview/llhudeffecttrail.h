/** 
 * @file llhudeffecttrail.h
 * @brief LLHUDEffectSpiral class definition
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

#ifndef LL_LLHUDEFFECTTRAIL_H
#define LL_LLHUDEFFECTTRAIL_H

#include "llhudeffect.h"

#include "llframetimer.h"

#include "llinterp.h"
#include "llviewerpartsim.h"

class LLViewerObject;

const U32 NUM_TRAIL_POINTS = 40;


class LLHUDEffectSpiral : public LLHUDEffect
{
public:
    /*virtual*/ void markDead();
    /*virtual*/ void setTargetObject(LLViewerObject* objectp);
    void setVMag(F32 vmag) { mVMag = vmag; }
    void setVOffset(F32 offset) { mVOffset = offset; }
    void setInitialRadius(F32 radius) { mInitialRadius = radius; }
    void setFinalRadius(F32 radius) { mFinalRadius = radius; }
    void setScaleBase(F32 scale) { mScaleBase = scale; }
    void setScaleVar(F32 scale) { mScaleVar = scale; }
    void setSpinRate(F32 rate) { mSpinRate = rate; }
    void setFlickerRate(F32 rate) { mFlickerRate = rate; }

    // Start the effect playing locally.
    void triggerLocal();

    friend class LLHUDObject;
protected:
    LLHUDEffectSpiral(const U8 type);
    ~LLHUDEffectSpiral();

    /*virtual*/ void render();
    /*virtual*/ void renderForTimer();
    /*virtual*/ void packData(LLMessageSystem *mesgsys);
    /*virtual*/ void unpackData(LLMessageSystem *mesgsys, S32 blocknum);
private:
    /*
    void setupParticle(const S32 i, const F32 start_time);

    
    LLInterpExp<F32>                mRadius[NUM_TRAIL_POINTS];
    LLInterpLinear<LLVector3d>      mDistance[NUM_TRAIL_POINTS];
    LLInterpLinear<F32>             mScale[NUM_TRAIL_POINTS];
    F32                             mOffset[NUM_TRAIL_POINTS];
    */

    BOOL                            mbInit;
    LLPointer<LLViewerPartSource>   mPartSourcep;

    F32                             mKillTime;
    F32                             mVMag;
    F32                             mVOffset;
    F32                             mInitialRadius;
    F32                             mFinalRadius;
    F32                             mSpinRate;
    F32                             mFlickerRate;
    F32                             mScaleBase;
    F32                             mScaleVar;
    LLFrameTimer                    mTimer;
    LLInterpLinear<F32>             mFadeInterp;
};

#endif // LL_LLHUDEFFECTGLOW_H
