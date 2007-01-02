/** 
 * @file llhudeffecttrail.h
 * @brief LLHUDEffectSpiral class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	/*virtual*/ void packData(LLMessageSystem *mesgsys);
	/*virtual*/ void unpackData(LLMessageSystem *mesgsys, S32 blocknum);
private:
	/*
	void setupParticle(const S32 i, const F32 start_time);

	
	LLInterpExp<F32>				mRadius[NUM_TRAIL_POINTS];
	LLInterpLinear<LLVector3d>		mDistance[NUM_TRAIL_POINTS];
	LLInterpLinear<F32>				mScale[NUM_TRAIL_POINTS];
	F32								mOffset[NUM_TRAIL_POINTS];
	*/

	BOOL							mbInit;
	LLPointer<LLViewerPartSource>	mPartSourcep;

	F32								mKillTime;
	F32								mVMag;
	F32								mVOffset;
	F32								mInitialRadius;
	F32								mFinalRadius;
	F32								mSpinRate;
	F32								mFlickerRate;
	F32								mScaleBase;
	F32								mScaleVar;
	LLFrameTimer					mTimer;
	LLInterpLinear<F32>				mFadeInterp;
};

#endif // LL_LLHUDEFFECTGLOW_H
