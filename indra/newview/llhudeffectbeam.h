/** 
 * @file llhudeffectbeam.h
 * @brief LLHUDEffectBeam class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	/*virtual*/ void packData(LLMessageSystem *mesgsys);
	/*virtual*/ void unpackData(LLMessageSystem *mesgsys, S32 blocknum);
private:
	void setupParticle(const S32 i);

	
	F32 mKillTime;
	LLFrameTimer mTimer;
	LLInterpLinear<LLVector3d> mInterp[NUM_POINTS];
	LLInterpLinear<F32> mInterpFade[NUM_POINTS];
	LLInterpLinear<F32> mFadeInterp;
	LLVector3d	mTargetPos;
};

#endif // LL_LLHUDEFFECTBEAM_H
