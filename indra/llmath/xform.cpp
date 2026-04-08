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
        // Rotate mWorldPosition by parent's world rotation. We bridge
        // through LLVector3 *= LLQuaternion deliberately, NOT
        // glm::quat * glm::vec3 directly. Reason:
        //
        // LL's `vec *= quat` uses the full `q * v * conj(q)` formula,
        // which scales the result by |q|^2 for non-unit input quats.
        // glm's `quat * vec3` uses the optimized formula that assumes
        // unit-length quat and produces a different result for non-unit
        // input.
        //
        // Production avatar code uses unit quats, so the two paths agree
        // in practice. But the unit test xform_test #7 (the LLXform
        // unit-test net's compose test, lines 207-243) uses non-unit
        // quats LLQuaternion(1,2,3,4) and LLQuaternion(5,6,7,8) and
        // pins LL's lenient behavior bit-for-bit. Switching to the glm
        // form would break that test.
        //
        // Per the bug-preservation doctrine in the quat migration cheat
        // sheet, this is preserved-as-is. Track in
        // sl_dev/docs/quaternion_migration_bugs.md as BUG-Q-002 (LL
        // lenient vec*quat for non-unit input — likely a design choice,
        // not a bug, but worth documenting). Revisit if production code
        // is ever found that depends on the |q|^2 scaling.
        {
            LLVector3 tmp(mWorldPosition);
            tmp *= LLQuaternion(mParent->getWorldRotation());
            mWorldPosition = glm::vec3(tmp.mV[VX], tmp.mV[VY], tmp.mV[VZ]);
        }
        mWorldPosition += mParent->getWorldPosition();
        // CRITICAL OPERAND-ORDER FLIP (cluster #17):
        //   LL form:  mWorldRotation = mRotation * mParent->getWorldRotation()
        //     LL semantics: 'apply mRotation first, then parent's world rotation'
        //   glm form: mWorldRotation = mParent->getWorldRotation() * mRotation
        //     glm semantics: rightmost is applied first (= apply mRotation,
        //     then parent's world rotation) -- same total rotation, REVERSED
        //     operand order. The unit test net (test 7 + test 12) pins this
        //     via direct quaternion comparison so any miss surfaces immediately.
        mWorldRotation = mParent->getWorldRotation() * mRotation;
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
