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
#include "m3math.h"

void LLFlycam::setTransform(const LLVector3d& position, const LLQuaternion& rotation)
{
    mPosition = position;
    mRotation = rotation;
    mRotation.normalize();
}

void LLFlycam::getTransform(LLVector3d& position_out, LLQuaternion& rotation_out)
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


void LLFlycam::setRollRate(F32 roll_rate)
{
    // Note: this math expects roll_rate to be in range [-1.0, 1.0]
    constexpr F32 ROLL_RATE_FACTOR = 0.90f;
    mRollRate = roll_rate * ROLL_RATE_FACTOR;
}


void LLFlycam::setZoomRate(F32 zoom_rate)
{
    // Note: this math expects zoom_rate to be in range [-1.0, 1.0]
    constexpr F32 FULL_ZOOM_PERIOD = 5.0f; // seconds
    constexpr F32 ZOOM_RATE_FACTOR = (MAX_FIELD_OF_VIEW - MIN_FIELD_OF_VIEW) / FULL_ZOOM_PERIOD;
    mZoomRate = zoom_rate * ZOOM_RATE_FACTOR;
}


void LLFlycam::setOrbitEngaged(bool engaged, F32 focal_distance)
{
    if (engaged && !mOrbitEngaged)
    {
        // Rising edge: fix the focal point 'focal_distance' meters in front of
        // the camera's current forward axis. It stays put in world space for
        // as long as orbit stays engaged, even as Pan/Truck/Tilt/Roll rotate
        // the camera around it (see integrate()).
        LLVector3 forward(LLMatrix3(mRotation).getFwdRow());
        mOrbitFocalPoint = mPosition + LLVector3d(forward) * (F64)focal_distance;
    }
    mOrbitEngaged = engaged;
}


void LLFlycam::setOrbitRadialRate(F32 radial_rate)
{
    // Note: this math expects radial_rate to be in range [-1.0, 1.0]. The
    // resulting rate is a FRACTION of the current distance-to-focal-point per
    // second (not a fixed speed), so approach slows near the focal point and
    // retreat speeds up far away from it.
    constexpr F32 ORBIT_RADIAL_RATE_FACTOR = 1.0f; // fraction of distance/sec at full deflection; may be tuned later
    mOrbitRadialRate = radial_rate * ORBIT_RADIAL_RATE_FACTOR;
}


void LLFlycam::startReset(const LLVector3d& target_position, const LLQuaternion& target_rotation, F32 duration)
{
    mResetStartPosition = mPosition;
    mResetStartRotation = mRotation;
    mResetTargetPosition = target_position;
    mResetTargetRotation = target_rotation;
    mResetDuration = std::max(duration, 0.001f);
    mResetTimeRemaining = mResetDuration;

    // Drop any live rates so leftover input doesn't fight the lerp, or get
    // applied unexpectedly the moment the reset completes.
    mLinearVelocity.clear();
    mPitchRate = mYawRate = mRollRate = mZoomRate = 0.0f;
}


void LLFlycam::integrate(F32 delta_time)
{
    // cap delta_time to slow camera motion when framerates are low
    constexpr F32 MAX_DELTA_TIME = 0.2f;
    if (delta_time > MAX_DELTA_TIME)
    {
        delta_time = MAX_DELTA_TIME;
    }

    if (mResetTimeRemaining > 0.0f)
    {
        // Reset in progress: lerp toward the target transform and ignore
        // whatever rates setLinearVelocity()/setPitchRate()/etc. set this
        // frame -- flycam input has no effect until the lerp completes.
        mResetTimeRemaining = std::max(mResetTimeRemaining - delta_time, 0.0f);
        F32 fraction = 1.0f - (mResetTimeRemaining / mResetDuration);
        mPosition = lerp(mResetStartPosition, mResetTargetPosition, fraction);
        mRotation = nlerp(fraction, mResetStartRotation, mResetTargetRotation);
        return;
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

    // Roll about the camera's forward (local X) axis.  Like pitch this is a
    // body-frame rotation, so it pre-multiplies mRotation.
    angle = delta_time * mRollRate * (mView / DEFAULT_FIELD_OF_VIEW);
    if (fabsf(angle) > 0.0f)
    {
        LLQuaternion dQ;
        dQ.setAngleAxis(angle, 1.0f, 0.0f, 0.0f);
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

    if (mOrbitEngaged)
    {
        // Re-derive position from the just-rotated forward axis, so the
        // camera stays (radially adjusted) 'distance' meters from the fixed
        // focal point, always facing it.  Pan/Truck (yaw, above) and Tilt
        // (pitch, above) therefore sweep/swing the camera around the focal
        // point as a side effect of simply rotating the camera; only the
        // radius itself is orbit-specific here.
        constexpr F32 MIN_ORBIT_DISTANCE = 0.1f; // meters
        F32 distance = (F32)(mPosition - mOrbitFocalPoint).length();
        distance = std::max(distance * (1.0f + delta_time * mOrbitRadialRate), MIN_ORBIT_DISTANCE);
        // 'forward' points from the camera toward the focal point, so the
        // camera itself sits behind the focal point along that axis.
        LLVector3 forward(LLMatrix3(mRotation).getFwdRow());
        mPosition = mOrbitFocalPoint - LLVector3d(forward) * (F64)distance;
    }

    if (mLinearVelocity.lengthSquared() > 0.0f)
    {
        // Boom (local Z) still translates the camera directly here, on top of
        // the orbit reposition above, regardless of mOrbitEngaged -- Truck/
        // Dolly's contributions are zeroed out by LLAgentCamera::updateFlycam()
        // while orbiting, since they're redirected into yaw/radial rate instead.
        LLVector3d translation((delta_time * mLinearVelocity) * mRotation);
        mPosition += translation;
        if (mOrbitEngaged)
        {
            // Boom doesn't reorient the camera or change its distance from
            // the focal point -- drag the focal point along with it instead,
            // so the next frame's orbit reposition still measures 'distance'
            // from wherever boom just moved it to.
            mOrbitFocalPoint += translation;
        }
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


void LLFlycam::applyFrameDelta(const F32 local_delta[7],
                                bool auto_level, F32 auto_level_fraction,
                                bool direct_view, F32 direct_view_value)
{
    mPosition += LLVector3d(local_delta[VX], local_delta[VY], local_delta[VZ]) * mRotation;

    LLMatrix3 rot_mat(local_delta[3], local_delta[4], local_delta[5]);
    mRotation = LLQuaternion(rot_mat) * mRotation;

    if (auto_level)
    {
        LLMatrix3 level(mRotation);

        LLVector3 x = LLVector3(level.mMatrix[0]);
        LLVector3 y = LLVector3(level.mMatrix[1]);
        LLVector3 z = LLVector3(level.mMatrix[2]);

        y.mV[2] = 0.f;
        y.normVec();

        level.setRows(x, y, z);
        level.orthogonalize();

        LLQuaternion quat(level);
        mRotation = nlerp(auto_level_fraction, mRotation, quat);
    }

    if (direct_view)
    {
        setView(direct_view_value);
    }
    else
    {
        setView(mView + local_delta[6]);
    }
}
