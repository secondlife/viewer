/** 
 * @file llhudeffecttrail.h
 * @brief LLHUDEffectSpiral class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
