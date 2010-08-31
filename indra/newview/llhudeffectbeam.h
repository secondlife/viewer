/** 
 * @file llhudeffectbeam.h
 * @brief LLHUDEffectBeam class definition
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
