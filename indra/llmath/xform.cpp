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
        // Rotate mWorldPosition by parent's world rotation. Uses the
        // glm-native form `quat * vec3`, which assumes the quaternion
        // is unit-length. Production avatar code always uses unit quats
        // (network packets ship normalized, angle-axis ctors normalize
        // implicitly, glTF spec requires unit). BUG-Q-002 (LL lenient
        // `vec *= quat` with |q|^2 scaling for non-unit input) is
        // documented in docs/quaternion_migration_bugs.md and is now
        // production-safe to ignore: the audit confirmed no production
        // path depends on the lenient |q|^2 scaling. The lenient LL
        // operators stay alive only for tests #10-#12 in
        // llquaternion_test.cpp until LLQuaternion is deleted in
        // Phase 4 of the quat migration. xform_test #7 was updated
        // to normalize its test quats so it agrees with this path.
        mWorldPosition = mParent->getWorldRotation() * mWorldPosition;
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
