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

#include <algorithm>
#include "llcamera.h"
#include "llcoordframe.h"

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

// 'view' is expected to be in radians
void LLFlycam::setView(F32 view)
{
    mView = std::clamp(view, MIN_FIELD_OF_VIEW, MAX_FIELD_OF_VIEW);
}


void LLFlycam::setLinearVelocity(const LLVector3& velocity)
{
    // Note: this math expects velocity components to be in range [-1.0, 1.0]
    mLinearVelocity = velocity;
}


void LLFlycam::setPitchRate(F32 pitch_rate)
{
    // Note: this math expects pitch_rate to be in range [-1.0, 1.0]
    constexpr F32 PITCH_RATE_FACTOR = 0.75f;
    mPitchRate = pitch_rate * PITCH_RATE_FACTOR;
}


void LLFlycam::setYawRate(F32 yaw_rate)
{
    // Note: this math expects yaw_rate to be in range [-1.0, 1.0]
    constexpr F32 YAW_RATE_FACTOR = 0.90f;
    mYawRate = yaw_rate * YAW_RATE_FACTOR;
}


void LLFlycam::setZoomRate(F32 zoom_rate)
{
    // Note: this math expects zoom_rate to be in range [-1.0, 1.0]
    constexpr F32 FULL_ZOOM_PERIOD = 5.0f; // seconds
    constexpr F32 ZOOM_RATE_FACTOR = (MAX_FIELD_OF_VIEW - MIN_FIELD_OF_VIEW) / FULL_ZOOM_PERIOD;
    mZoomRate = zoom_rate * ZOOM_RATE_FACTOR;
}


void LLFlycam::integrate(F32 delta_time)
{
    // cap delta_time to slow camera motion when framerates are low
    constexpr F32 MAX_DELTA_TIME = 0.2f;
    if (delta_time > MAX_DELTA_TIME)
    {
        delta_time = MAX_DELTA_TIME;
    }

    // Note: we modulate pitch and yaw rates by view ratio
    // to make pitch and yaw work better when zoomed in close
    F32 angle = delta_time * mPitchRate * (mView / DEFAULT_FIELD_OF_VIEW);
    bool needs_renormalization = false;
    if (fabsf(angle) > 0.0f)
    {
        LLQuaternion dQ;
        dQ.setAngleAxis(angle, 0.0f, 1.0f, 0.0f);
        mRotation = dQ * mRotation;
        needs_renormalization = true;
    }

    angle = delta_time * mYawRate * (mView / DEFAULT_FIELD_OF_VIEW);
    if (fabsf(angle) > 0.0f)
    {
        LLQuaternion dQ;
        dQ.setAngleAxis(angle, 0.0f, 0.0f, 1.0f);
        mRotation = mRotation * dQ;
        needs_renormalization = true;
    }

    if (mLinearVelocity.lengthSquared() > 0.0f)
    {
        mPosition += (delta_time * mLinearVelocity) * mRotation;
    }

    if (mZoomRate != 0.0f)
    {
        // Note: we subtract the delta because "positive" zoom (e.g. "zoom in")
        // produces smaller view angle
        mView = std::clamp(mView - delta_time * mZoomRate, MIN_FIELD_OF_VIEW, MAX_FIELD_OF_VIEW);
    }

    if (needs_renormalization)
    {
        mRotation.normalize();
    }
}
