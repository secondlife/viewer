/**
 * @file llflycam.h
 * @brief LLFlycam class header file
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

#pragma once

#include "llcamera.h"
#include "llquaternion.h"
#include "v3math.h"
#include "v3dmath.h"

class LLFlycam
{
public:

    LLFlycam() = default;

    // Note: position is in global (region-independent) coordinates, since it
    // must remain valid across the region-origin rebases that happen when the
    // agent crosses a region boundary while this transform is in use.
    void setTransform(const LLVector3d& position, const LLQuaternion& rotation);
    void getTransform(LLVector3d& position_out, LLQuaternion& rotation_out);

    void setView(F32 view);
    F32 getView() const { return mView; }

    void setLinearVelocity(const LLVector3& velocity);
    void setPitchRate(F32 pitch_rate);
    void setYawRate(F32 yaw_rate);
    void setRollRate(F32 roll_rate);
    void setZoomRate(F32 zoom_rate);

    // Orbit modifier: while engaged, position is re-derived every integrate()
    // from the camera's own (pitch/yaw-rotated) forward axis so the camera
    // stays 'focal_distance' meters from a focal point fixed in front of it
    // the moment orbit engages -- Pan/Truck/Tilt/Roll keep rotating the
    // camera exactly as they do outside orbit (see integrate()); only the
    // radial (dolly) rate is orbit-specific. Boom still translates the camera
    // directly (see integrate()), but doesn't reorient it or change its
    // distance from the focal point: the same translation is applied to the
    // focal point too, dragging it along with the camera.
    void setOrbitEngaged(bool engaged, F32 focal_distance);
    bool isOrbitEngaged() const { return mOrbitEngaged; }
    void setOrbitRadialRate(F32 radial_rate);

    // Begins a smooth transition from the current transform to
    // 'target_position'/'target_rotation', taking 'duration' seconds.  While
    // the transition is in progress, integrate() ignores the rates set by
    // setLinearVelocity()/setPitchRate()/etc. and instead lerps toward the
    // target; normal input-driven integration resumes once it completes.
    void startReset(const LLVector3d& target_position, const LLQuaternion& target_rotation, F32 duration);
    bool isResetting() const { return mResetTimeRemaining > 0.0f; }

    void integrate(F32 delta_time);

    // Applies one frame of already-tuned deltas directly to the transform,
    // bypassing the rate-based setPitchRate()/etc. + integrate() path. Used
    // by input sources (the legacy NDOF joystick) that compute their own
    // per-axis dead-zone/scale/feathering and only need this class to own
    // the resulting transform update, so their feel is preserved exactly.
    // 'local_delta' holds, in order: [0..2] local-frame translation delta
    // (x,y,z); [3..5] rotation delta (roll,pitch,yaw, as fed to
    // LLMatrix3(r,p,y)); [6] zoom delta added to the view angle (ignored if
    // 'direct_view' is set, in which case 'direct_view_value' replaces the
    // view angle outright).
    void applyFrameDelta(const F32 local_delta[7],
                          bool auto_level, F32 auto_level_fraction,
                          bool direct_view, F32 direct_view_value);

protected:
    LLVector3d mPosition;
    LLVector3 mLinearVelocity;
    LLQuaternion mRotation;
    F32 mPitchRate { 0.0f };
    F32 mYawRate { 0.0f };
    F32 mRollRate { 0.0f };
    F32 mZoomRate { 0.0f };
    F32 mView { DEFAULT_FIELD_OF_VIEW };

    // Orbit-in-progress state: see setOrbitEngaged()/setOrbitRadialRate() and
    // the orbit branch in integrate().
    bool mOrbitEngaged { false };
    LLVector3d mOrbitFocalPoint;
    F32 mOrbitRadialRate { 0.0f };

    // Reset-in-progress state: integrate() lerps mPosition/mRotation from
    // mResetStart* to mResetTarget* as mResetTimeRemaining counts down from
    // mResetDuration to zero.
    LLVector3d mResetStartPosition;
    LLQuaternion mResetStartRotation;
    LLVector3d mResetTargetPosition;
    LLQuaternion mResetTargetRotation;
    F32 mResetDuration { 0.0f };
    F32 mResetTimeRemaining { 0.0f };
};
