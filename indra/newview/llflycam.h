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

class LLFlycam
{
public:

    LLFlycam();

    void setTransform(const LLVector3& position, const LLQuaternion& rotation);
    void getTransform(LLVector3& position_out, LLQuaternion& rotation_out);

    void setView(F32 view);
    F32 getView() const { return mView; }

    void setLinearVelocity(const LLVector3& velocity);
    void setPitchRate(F32 pitch_rate);
    void setYawRate(F32 yaw_rate);
    void setZoomRate(F32 zoom_rate);

    void integrate(F32 delta_time);

protected:
    LLVector3 mPosition;
    LLVector3 mLinearVelocity;
    LLQuaternion mRotation;
    F32 mPitchRate { 0.0f };
    F32 mYawRate { 0.0f };
    F32 mZoomRate { 0.0f };
    F32 mView { DEFAULT_FIELD_OF_VIEW };
};
