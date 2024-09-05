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
#include "../llskinningutil.h"

using namespace LL::GLTF;
using namespace boost::json;

bool Animation::prep(Asset& asset)
{
    if (!mSamplers.empty())
    {
        mMinTime = FLT_MAX;
        mMaxTime = -FLT_MAX;
        for (auto& sampler : mSamplers)
        {
            if (!sampler.prep(asset))
            {
                return false;
            }
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
        if (!channel.prep(asset, mSamplers[channel.mSampler]))
        {
            return false;
        }
    }

    for (auto& channel : mTranslationChannels)
    {
        if (!channel.prep(asset, mSamplers[channel.mSampler]))
        {
            return false;
        }
    }

    for (auto& channel : mScaleChannels)
    {
        if (!channel.prep(asset, mSamplers[channel.mSampler]))
        {
            return false;
        }
    }

    return true;
}

void Animation::update(Asset& asset, F32 dt)
{
    mTime += dt;

    apply(asset, mTime);
}

void Animation::apply(Asset& asset, float time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_GLTF;

    // convert time to animation loop time
    time = fmod(time, mMaxTime - mMinTime) + mMinTime;

    // apply each channel
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_GLTF("gltfanim - rotation");

        for (auto& channel : mRotationChannels)
        {
            channel.apply(asset, mSamplers[channel.mSampler], time);
        }
    }

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_GLTF("gltfanim - translation");

        for (auto& channel : mTranslationChannels)
        {
            channel.apply(asset, mSamplers[channel.mSampler], time);
        }
    }

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_GLTF("gltfanim - scale");

        for (auto& channel : mScaleChannels)
        {
            channel.apply(asset, mSamplers[channel.mSampler], time);
        }
    }
};

bool Animation::Sampler::prep(Asset& asset)
{
    Accessor& accessor = asset.mAccessors[mInput];
    mMinTime = (F32)accessor.mMin[0];
    mMaxTime = (F32)accessor.mMax[0];

    mFrameTimes.resize(accessor.mCount);

    LLStrider<F32> frame_times = mFrameTimes.data();
    copy(asset, accessor, frame_times);

    return true;
}


void Animation::Sampler::serialize(object& obj) const
{
    write(mInput, "input", obj, INVALID_INDEX);
    write(mOutput, "output", obj, INVALID_INDEX);
    write(mInterpolation, "interpolation", obj, std::string("LINEAR"));
    write(mMinTime, "min_time", obj);
    write(mMaxTime, "max_time", obj);
}

const Animation::Sampler& Animation::Sampler::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "input", mInput);
        copy(src, "output", mOutput);
        copy(src, "interpolation", mInterpolation);
        copy(src, "min_time", mMinTime);
        copy(src, "max_time", mMaxTime);
    }
    return *this;
}

bool Animation::Channel::Target::operator==(const Channel::Target& rhs) const
{
    return mNode == rhs.mNode && mPath == rhs.mPath;
}

bool Animation::Channel::Target::operator!=(const Channel::Target& rhs) const
{
    return !(*this == rhs);
}

void Animation::Channel::Target::serialize(object& obj) const
{
    write(mNode, "node", obj, INVALID_INDEX);
    write(mPath, "path", obj);
}

const Animation::Channel::Target& Animation::Channel::Target::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "node", mNode);
        copy(src, "path", mPath);
    }
    return *this;
}

void Animation::Channel::serialize(object& obj) const
{
    write(mSampler, "sampler", obj, INVALID_INDEX);
    write(mTarget, "target", obj);
}

const Animation::Channel& Animation::Channel::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "sampler", mSampler);
        copy(src, "target", mTarget);
    }
    return *this;
}

void Animation::Sampler::getFrameInfo(Asset& asset, F32 time, U32& frameIndex, F32& t)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_GLTF;
    llassert(mFrameTimes.size() > 1); // if there is only one frame, there is no need to interpolate

    if (time < mMinTime)
    {
        frameIndex = 0;
        t = 0.0f;
        return;
    }

    frameIndex = U32(mFrameTimes.size()) - 2;
    t = 1.f;

    if (time > mMaxTime)
    {
        return;
    }

    if (time < mLastFrameTime)
    {
        mLastFrameIndex = 0;
    }

    mLastFrameTime = time;

    U32 idx = mLastFrameIndex;

    for (U32 i = idx; i < (U32)mFrameTimes.size() - 1; i++)
    {
        if (time >= mFrameTimes[i] && time < mFrameTimes[i + 1])
        {
            frameIndex = i;
            t = (time - mFrameTimes[i]) / (mFrameTimes[i + 1] - mFrameTimes[i]);
            mLastFrameIndex = frameIndex;
            return;
        }
    }
}

bool Animation::RotationChannel::prep(Asset& asset, Animation::Sampler& sampler)
{
    Accessor& accessor = asset.mAccessors[sampler.mOutput];

    copy(asset, accessor, mRotations);

    return true;
}

void Animation::RotationChannel::apply(Asset& asset, Sampler& sampler, F32 time)
{
    U32 frameIndex;
    F32 t;

    Node& node = asset.mNodes[mTarget.mNode];

    if (sampler.mFrameTimes.size() < 2)
    {
        node.setRotation(mRotations[0]);
    }
    else
    {
        sampler.getFrameInfo(asset, time, frameIndex, t);

        // interpolate
        quat qf = glm::slerp(mRotations[frameIndex], mRotations[frameIndex + 1], t);

        qf = glm::normalize(qf);

        node.setRotation(qf);
    }
}

bool Animation::TranslationChannel::prep(Asset& asset, Animation::Sampler& sampler)
{
    Accessor& accessor = asset.mAccessors[sampler.mOutput];

    copy(asset, accessor, mTranslations);

    return true;
}

void Animation::TranslationChannel::apply(Asset& asset, Sampler& sampler, F32 time)
{
    U32 frameIndex;
    F32 t;

    Node& node = asset.mNodes[mTarget.mNode];

    if (sampler.mFrameTimes.size() < 2)
    {
        node.setTranslation(mTranslations[0]);
    }
    else
    {
        sampler.getFrameInfo(asset, time, frameIndex, t);

        // interpolate
        const vec3& v0 = mTranslations[frameIndex];
        const vec3& v1 = mTranslations[frameIndex + 1];

        vec3 vf = v0 + t * (v1 - v0);

        node.setTranslation(vf);
    }
}

bool Animation::ScaleChannel::prep(Asset& asset, Animation::Sampler& sampler)
{
    Accessor& accessor = asset.mAccessors[sampler.mOutput];

    copy(asset, accessor, mScales);

    return true;
}

void Animation::ScaleChannel::apply(Asset& asset, Sampler& sampler, F32 time)
{
    U32 frameIndex;
    F32 t;

    Node& node = asset.mNodes[mTarget.mNode];

    if (sampler.mFrameTimes.size() < 2)
    {
        node.setScale(mScales[0]);
    }
    else
    {
        sampler.getFrameInfo(asset, time, frameIndex, t);

        // interpolate
        const vec3& v0 = mScales[frameIndex];
        const vec3& v1 = mScales[frameIndex + 1];

        vec3 vf = v0 + t * (v1 - v0);

        node.setScale(vf);
    }
}

void Animation::serialize(object& obj) const
{
    write(mName, "name", obj);
    write(mSamplers, "samplers", obj);

    std::vector<Channel> channels;
    channels.insert(channels.end(), mRotationChannels.begin(), mRotationChannels.end());
    channels.insert(channels.end(), mTranslationChannels.begin(), mTranslationChannels.end());
    channels.insert(channels.end(), mScaleChannels.begin(), mScaleChannels.end());

    write(channels, "channels", obj);
}

const Animation& Animation::operator=(const Value& src)
{
    if (src.is_object())
    {
        const object& obj = src.as_object();

        copy(obj, "name", mName);
        copy(obj, "samplers", mSamplers);

        // make a temporory copy of generic channels
        std::vector<Channel> channels;
        copy(obj, "channels", channels);

        // break up into channel specific implementations
        for (auto& channel: channels)
        {
            if (channel.mTarget.mPath == "rotation")
            {
                mRotationChannels.push_back(channel);
            }
            else if (channel.mTarget.mPath == "translation")
            {
                mTranslationChannels.push_back(channel);
            }
            else if (channel.mTarget.mPath == "scale")
            {
                mScaleChannels.push_back(channel);
            }
        }
    }
    return *this;
}

Skin::~Skin()
{
    if (mUBO)
    {
        glDeleteBuffers(1, &mUBO);
    }
}

void Skin::uploadMatrixPalette(Asset& asset)
{
    // prepare matrix palette
    LL_PROFILE_ZONE_SCOPED_CATEGORY_GLTF;

    U32 max_joints = LLSkinningUtil::getMaxGLTFJointCount();

    if (mUBO == 0)
    {
        glGenBuffers(1, &mUBO);
    }

    size_t joint_count = llmin<size_t>(max_joints, mJoints.size());

    std::vector<mat4> t_mp;

    t_mp.resize(joint_count);

    for (U32 i = 0; i < joint_count; ++i)
    {
        Node& joint = asset.mNodes[mJoints[i]];
        // build matrix palette in asset space
        t_mp[i] = joint.mAssetMatrix * mInverseBindMatricesData[i];
    }

    std::vector<F32> glmp;

    glmp.resize(joint_count * 12);

    F32* mp = glmp.data();

    for (U32 i = 0; i < joint_count; ++i)
    {
        F32* m = glm::value_ptr(t_mp[i]);

        U32 idx = i * 12;

        mp[idx + 0] = m[0];
        mp[idx + 1] = m[1];
        mp[idx + 2] = m[2];
        mp[idx + 3] = m[12];

        mp[idx + 4] = m[4];
        mp[idx + 5] = m[5];
        mp[idx + 6] = m[6];
        mp[idx + 7] = m[13];

        mp[idx + 8] = m[8];
        mp[idx + 9] = m[9];
        mp[idx + 10] = m[10];
        mp[idx + 11] = m[14];
    }

    glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
    glBufferData(GL_UNIFORM_BUFFER, glmp.size() * sizeof(F32), glmp.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

bool Skin::prep(Asset& asset)
{
    if (mInverseBindMatrices != INVALID_INDEX)
    {
        Accessor& accessor = asset.mAccessors[mInverseBindMatrices];
        copy(asset, accessor, mInverseBindMatricesData);
    }

    return true;
}

const Skin& Skin::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "name", mName);
        copy(src, "skeleton", mSkeleton);
        copy(src, "inverseBindMatrices", mInverseBindMatrices);
        copy(src, "joints", mJoints);
    }
    return *this;
}

void Skin::serialize(object& obj) const
{
    write(mInverseBindMatrices, "inverseBindMatrices", obj, INVALID_INDEX);
    write(mJoints, "joints", obj);
    write(mName, "name", obj);
    write(mSkeleton, "skeleton", obj, INVALID_INDEX);
}
