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

LLXform::~LLXform() = default;

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
    return const_cast<LLXform*>(root);
}

bool LLXform::isRoot() const
{
    return (!mParent);
}

bool LLXform::isRootEdit() const
{
    return (!mParent);
}

LLXformMatrix::~LLXformMatrix() = default;

void LLXformMatrix::update()
{
    if (mParent)
    {
        mWorldPosition = mPosition;
        if (mParent->getScaleChildOffset())
        {
            mWorldPosition *= mParent->getScale();
        }
        // Rotate by parent's world rotation: bridge through LLVector3 for the
        // LLVector3 *= LLQuaternion operator, then assign back.
        {
            LLVector3 tmp(mWorldPosition);
            tmp *= mParent->getWorldRotation();
            mWorldPosition = glm::vec3(tmp.mV[VX], tmp.mV[VY], tmp.mV[VZ]);
        }
        mWorldPosition += mParent->getWorldPosition();
        mWorldRotation = mRotation * mParent->getWorldRotation();
    }
    else
    {
        mWorldPosition = mPosition;
        mWorldRotation = mRotation;
    }
}

void LLXformMatrix::updateMatrix(bool update_bounds)
{
    update();

    mWorldMatrix.initAll(mScale, mWorldRotation, mWorldPosition);

    if (update_bounds && (mChanged & MOVED))
    {
        mMin.x = mMax.x = mWorldMatrix.mMatrix[3][0];
        mMin.y = mMax.y = mWorldMatrix.mMatrix[3][1];
        mMin.z = mMax.z = mWorldMatrix.mMatrix[3][2];

        F32 f0 = (fabs(mWorldMatrix.mMatrix[0][0])+fabs(mWorldMatrix.mMatrix[1][0])+fabs(mWorldMatrix.mMatrix[2][0])) * 0.5f;
        F32 f1 = (fabs(mWorldMatrix.mMatrix[0][1])+fabs(mWorldMatrix.mMatrix[1][1])+fabs(mWorldMatrix.mMatrix[2][1])) * 0.5f;
        F32 f2 = (fabs(mWorldMatrix.mMatrix[0][2])+fabs(mWorldMatrix.mMatrix[1][2])+fabs(mWorldMatrix.mMatrix[2][2])) * 0.5f;

        mMin.x -= f0;
        mMin.y -= f1;
        mMin.z -= f2;

        mMax.x += f0;
        mMax.y += f1;
        mMax.z += f2;
    }
}

void LLXformMatrix::getMinMax(glm::vec3& min, glm::vec3& max) const
{
    min = mMin;
    max = mMax;
}
