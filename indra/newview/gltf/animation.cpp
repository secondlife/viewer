/**
 * @file animation.cpp
 * @brief LL GLTF Animation Implementation
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

#include "../llviewerprecompiledheaders.h"

#include "asset.h"
#include "buffer_util.h"

using namespace LL::GLTF;

void Animation::allocateGLResources(Asset& asset)
{
    if (!mSamplers.empty())
    {
        mMinTime = FLT_MAX;
        mMaxTime = -FLT_MAX;
        for (auto& sampler : mSamplers)
        {
            sampler.allocateGLResources(asset);
            mMinTime = llmin(sampler.mMinTime, mMinTime);
            mMaxTime = llmax(sampler.mMaxTime, mMaxTime);
        }
    }
    else
    {
        mMinTime = mMaxTime = 0.f;
    }

    for (auto& channel : mRotationChannels)
    {
        channel.allocateGLResources(asset, mSamplers[channel.mSampler]);
    }
}

void Animation::update(Asset& asset, F32 dt)
{
    mTime += dt;

    apply(asset, mTime);
}

void Animation::apply(Asset& asset, float time)
{
    // convert time to animation loop time
    time = fmod(time, mMaxTime - mMinTime) + mMinTime;

    // apply each channel
    for (auto& channel : mRotationChannels)
    {
        channel.apply(asset, mSamplers[channel.mSampler], time);
    }
};


void Animation::Sampler::allocateGLResources(Asset& asset)
{
    Accessor& accessor = asset.mAccessors[mInput];
    mMinTime = accessor.mMin[0];
    mMaxTime = accessor.mMax[0];

    mFrameTimes.resize(accessor.mCount);

    LLStrider<F32> frame_times = mFrameTimes.data();
    copy(asset, accessor, frame_times);

    asset.updateTransforms();
}

void Animation::Sampler::getFrameInfo(Asset& asset, F32 time, U32& frameIndex, F32& t)
{
    if (time < mMinTime)
    {
        frameIndex = 0;
        t = 0.0f;
        return;
    }

    if (time > mMaxTime)
    {
        frameIndex = mFrameTimes.size() - 2;
        t = 1.0f;
        return;
    }

    frameIndex = mFrameTimes.size() - 2;
    t = 1.f;

    for (U32 i = 0; i < mFrameTimes.size() - 1; i++)
    {
        if (time >= mFrameTimes[i] && time < mFrameTimes[i + 1])
        {
            frameIndex = i;
            t = (time - mFrameTimes[i]) / (mFrameTimes[i + 1] - mFrameTimes[i]);
            return;
        }
    }
}

void Animation::RotationChannel::allocateGLResources(Asset& asset, Animation::Sampler& sampler)
{
    Accessor& accessor = asset.mAccessors[sampler.mOutput];

    copy(asset, accessor, mRotations);
}

void Animation::RotationChannel::apply(Asset& asset, Sampler& sampler, F32 time)
{
    U32 frameIndex;
    F32 t;

    sampler.getFrameInfo(asset, time, frameIndex, t);

    // interpolate
    LLQuaternion q0(mRotations[frameIndex].get_value());
    LLQuaternion q1(mRotations[frameIndex + 1].get_value());

    LLQuaternion qf = slerp(t, q0, q1);

    qf.normalize();

    Node& node = asset.mNodes[mTarget.mNode];

    node.setRotation(glh::quaternionf(qf.mQ));
}

