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
using namespace boost::json;

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

    for (auto& channel : mTranslationChannels)
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

    for (auto& channel : mTranslationChannels)
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


const Animation::Sampler& Animation::Sampler::operator=(const tinygltf::AnimationSampler& src)
{
    mInput = src.input;
    mOutput = src.output;
    mInterpolation = src.interpolation;

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

const Animation::Channel& Animation::Channel::operator=(const tinygltf::AnimationChannel& src)
{
    mSampler = src.sampler;

    mTarget.mNode = src.target_node;
    mTarget.mPath = src.target_path;

    return *this;
}


void Animation::Sampler::getFrameInfo(Asset& asset, F32 time, U32& frameIndex, F32& t)
{
    LL_PROFILE_ZONE_SCOPED;

    if (time < mMinTime)
    {
        frameIndex = 0;
        t = 0.0f;
        return;
    }

    if (mFrameTimes.size() > 1)
    {
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
    else
    {
        frameIndex = 0;
        t = 0.0f;
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

    Node& node = asset.mNodes[mTarget.mNode];

    sampler.getFrameInfo(asset, time, frameIndex, t);

    if (sampler.mFrameTimes.size() == 1)
    {
        node.setRotation(mRotations[0]);
    }
    else
    {
        // interpolate
        quat qf = glm::slerp(mRotations[frameIndex], mRotations[frameIndex + 1], t);

        qf = glm::normalize(qf);

        node.setRotation(qf);
    }
}

void Animation::TranslationChannel::allocateGLResources(Asset& asset, Animation::Sampler& sampler)
{
    Accessor& accessor = asset.mAccessors[sampler.mOutput];

    copy(asset, accessor, mTranslations);
}

void Animation::TranslationChannel::apply(Asset& asset, Sampler& sampler, F32 time)
{
    U32 frameIndex;
    F32 t;

    Node& node = asset.mNodes[mTarget.mNode];

    sampler.getFrameInfo(asset, time, frameIndex, t);

    if (sampler.mFrameTimes.size() == 1)
    {
        node.setTranslation(mTranslations[0]);
    }
    else
    {
        // interpolate
        const vec3& v0 = mTranslations[frameIndex];
        const vec3& v1 = mTranslations[frameIndex + 1];

        vec3 vf = v0 + t * (v1 - v0);

        node.setTranslation(vf);
    }
}

void Animation::ScaleChannel::allocateGLResources(Asset& asset, Animation::Sampler& sampler)
{
    Accessor& accessor = asset.mAccessors[sampler.mOutput];

    copy(asset, accessor, mScales);
}

void Animation::ScaleChannel::apply(Asset& asset, Sampler& sampler, F32 time)
{
    U32 frameIndex;
    F32 t;

    Node& node = asset.mNodes[mTarget.mNode];

    sampler.getFrameInfo(asset, time, frameIndex, t);

    if (sampler.mFrameTimes.size() == 1)
    {
        node.setScale(mScales[0]);
    }
    else
    {
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

const Animation& Animation::operator=(const tinygltf::Animation& src)
{
    mName = src.name;

    mSamplers.resize(src.samplers.size());
    for (U32 i = 0; i < src.samplers.size(); ++i)
    {
        mSamplers[i] = src.samplers[i];
    }

    for (U32 i = 0; i < src.channels.size(); ++i)
    {
        if (src.channels[i].target_path == "rotation")
        {
            mRotationChannels.push_back(RotationChannel());
            mRotationChannels.back() = src.channels[i];
        }

        if (src.channels[i].target_path == "translation")
        {
            mTranslationChannels.push_back(TranslationChannel());
            mTranslationChannels.back() = src.channels[i];
        }

        if (src.channels[i].target_path == "scale")
        {
            mScaleChannels.push_back(ScaleChannel());
            mScaleChannels.back() = src.channels[i];
        }
    }

    return *this;
}

void Skin::allocateGLResources(Asset& asset)
{
    if (mInverseBindMatrices != INVALID_INDEX)
    {
        Accessor& accessor = asset.mAccessors[mInverseBindMatrices];
        copy(asset, accessor, mInverseBindMatricesData);
    }
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

const Skin& Skin::operator=(const tinygltf::Skin& src)
{
    mName = src.name;
    mSkeleton = src.skeleton;
    mInverseBindMatrices = src.inverseBindMatrices;
    mJoints = src.joints;

    return *this;
}

void Skin::serialize(object& obj) const
{
    write(mInverseBindMatrices, "inverseBindMatrices", obj, INVALID_INDEX);
    write(mJoints, "joints", obj);
    write(mName, "name", obj);
    write(mSkeleton, "skeleton", obj, INVALID_INDEX);
}
