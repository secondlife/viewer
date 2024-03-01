/**
 * @file llflycam.cpp
 * @brief LLFlycam class implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "llflycam.h"

LLFlycam::LLFlycam()
{
}


void LLFlycam::setTransform(const LLVector3& position, const LLQuaternion& rotation)
{
    mPosition = position;
    mRotation = rotation;
    mRotation.normalize();
}

void LLFlycam::getTransform(LLVector3& position_out, LLQuaternion& rotation_out)
{
    position_out = mPosition;
    rotation_out = mRotation;
}


void LLFlycam::setLinearVelocity(const LLVector3& velocity)
{
    // TODO: cap input
    mLinearVelocity = velocity;
}


void LLFlycam::setPitchRate(F32 pitch_rate)
{
    // TODO: cap input
    mPitchRate = pitch_rate;
}


void LLFlycam::setYawRate(F32 yaw_rate)
{
    // TODO: cap input
    mYawRate = yaw_rate;
}


void LLFlycam::integrate(F32 delta_time)
{
    // cap delta_time to slow camera motion when framerates are low
    constexpr F32 MAX_DELTA_TIME = 0.2f;
    if (delta_time > MAX_DELTA_TIME)
    {
        delta_time = MAX_DELTA_TIME;
    }

    if (mLinearVelocity.lengthSquared() > 0.0f)
    {
        mPosition += delta_time * mLinearVelocity;
    }

    F32 angle = mPitchRate * delta_time;
    bool needs_renormalization = false;
    if (fabsf(angle) > 0.0f)
    {
        LLQuaternion dQ;
        dQ.setAngleAxis(angle, 0.0f, 1.0f, 0.0f);
        mRotation = dQ * mRotation;
        needs_renormalization = true;
    }

    angle = mYawRate * delta_time;
    if (fabsf(angle) > 0.0f)
    {
        LLQuaternion dQ;
        dQ.setAngleAxis(angle, 0.0f, 0.0f, 1.0f);
        mRotation = dQ * mRotation;
        needs_renormalization = true;
    }

    if (needs_renormalization)
    {
        mRotation.normalize();
    }


    /*
    // from llviewerjoystick.cpp
    LLMatrix3 mat(sFlycamRotation);
    LLViewerCamera::getInstance()->setView(sFlycamZoom);
    LLViewerCamera::getInstance()->setOrigin(sFlycamPosition);
    LLViewerCamera::getInstance()->mXAxis = LLVector3(mat.mMatrix[0]);
    LLViewerCamera::getInstance()->mYAxis = LLVector3(mat.mMatrix[1]);
    LLViewerCamera::getInstance()->mZAxis = LLVector3(mat.mMatrix[2]);
    */
}
