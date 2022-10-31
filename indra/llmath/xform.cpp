/** 
 * @file xform.cpp
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

#include "linden_common.h"

#include "xform.h"

LLXform::LLXform()
{
    init();
}

LLXform::~LLXform()
{
}

// Link optimization - don't inline these LL_WARNS()
void LLXform::warn(const char* const msg)
{
    LL_WARNS() << msg << LL_ENDL;
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
