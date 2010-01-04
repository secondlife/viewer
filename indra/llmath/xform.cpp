/** 
 * @file xform.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "xform.h"

LLXform::LLXform()
{
	init();
}

LLXform::~LLXform()
{
}

// Link optimization - don't inline these llwarns
void LLXform::warn(const char* const msg)
{
	llwarns << msg << llendl;
}

LLXform* LLXform::getRoot() const
{
	const LLXform* root = this;
	while(root->mParent)
	{
		root = root->mParent;
	}
	return (LLXform*)root;
}

BOOL LLXform::isRoot() const
{
	return (!mParent);
}

BOOL LLXform::isRootEdit() const
{
	return (!mParent);
}

LLXformMatrix::~LLXformMatrix()
{
}

void LLXformMatrix::update()
{
	if (mParent) 
	{
		mWorldPosition = mPosition;
		if (mParent->getScaleChildOffset())
		{
			mWorldPosition.scaleVec(mParent->getScale());
		}
		mWorldPosition *= mParent->getWorldRotation();
		mWorldPosition += mParent->getWorldPosition();
		mWorldRotation = mRotation * mParent->getWorldRotation();
	}
	else
	{
		mWorldPosition = mPosition;
		mWorldRotation = mRotation;
	}
}

void LLXformMatrix::updateMatrix(BOOL update_bounds)
{
	update();

	mWorldMatrix.initAll(mScale, mWorldRotation, mWorldPosition);

	if (update_bounds && (mChanged & MOVED))
	{
		mMin.mV[0] = mMax.mV[0] = mWorldMatrix.mMatrix[3][0];
		mMin.mV[1] = mMax.mV[1] = mWorldMatrix.mMatrix[3][1];
		mMin.mV[2] = mMax.mV[2] = mWorldMatrix.mMatrix[3][2];

		F32 f0 = (fabs(mWorldMatrix.mMatrix[0][0])+fabs(mWorldMatrix.mMatrix[1][0])+fabs(mWorldMatrix.mMatrix[2][0])) * 0.5f;
		F32 f1 = (fabs(mWorldMatrix.mMatrix[0][1])+fabs(mWorldMatrix.mMatrix[1][1])+fabs(mWorldMatrix.mMatrix[2][1])) * 0.5f;
		F32 f2 = (fabs(mWorldMatrix.mMatrix[0][2])+fabs(mWorldMatrix.mMatrix[1][2])+fabs(mWorldMatrix.mMatrix[2][2])) * 0.5f;

		mMin.mV[0] -= f0; 
		mMin.mV[1] -= f1; 
		mMin.mV[2] -= f2; 

		mMax.mV[0] += f0; 
		mMax.mV[1] += f1; 
		mMax.mV[2] += f2; 
	}
}

void LLXformMatrix::getMinMax(LLVector3& min, LLVector3& max) const
{
	min = mMin;
	max = mMax;
}
