#pragma once

/**
 * @file animation.h
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

#include "accessor.h"

// LL GLTF Implementation
namespace LL
{
    namespace GLTF
    {
        class Asset;

        class Animation
        {
        public:
            class Sampler
            {
            public:
                std::vector<F32> mFrameTimes;

                F32 mMinTime = -FLT_MAX;
                F32 mMaxTime = FLT_MAX;

                S32 mInput = INVALID_INDEX;
                S32 mOutput = INVALID_INDEX;
                std::string mInterpolation;

                void allocateGLResources(Asset& asset);

                const Sampler& operator=(const tinygltf::AnimationSampler& src)
                {
                    mInput = src.input;
                    mOutput = src.output;
                    mInterpolation = src.interpolation;

                    return *this;
                }

                // get the frame index and time for the specified time
                // asset -- the asset to reference for Accessors
                // time -- the animation time to get the frame info for
                // frameIndex -- index of the closest frame that precedes the specified time
                // t - interpolant value between the frameIndex and the next frame
                void getFrameInfo(Asset& asset, F32 time, U32& frameIndex, F32& t);
            };

            class Channel
            {
            public:
                class Target
                {
                public:
                    S32 mNode = INVALID_INDEX;
                    std::string mPath;
                };

                S32 mSampler = INVALID_INDEX;
                Target mTarget;
                std::string mTargetPath;
                std::string mName;

                const Channel& operator=(const tinygltf::AnimationChannel& src)
                {
                    mSampler = src.sampler;

                    mTarget.mNode = src.target_node;
                    mTarget.mPath = src.target_path;

                    return *this;
                }

            };

            class RotationChannel : public Channel
            {
            public:
                std::vector<glh::quaternionf> mRotations;

                const RotationChannel& operator=(const tinygltf::AnimationChannel& src)
                {
                    Channel::operator=(src);
                    return *this;
                }

                // prepare data needed for rendering
                // asset -- asset to reference for Accessors
                // sampler -- Sampler associated with this channel
                void allocateGLResources(Asset& asset, Sampler& sampler);

                void apply(Asset& asset, Sampler& sampler, F32 time);
            };

            class TranslationChannel : public Channel
            {
            public:
                std::vector<glh::vec3f> mTranslations;

                const TranslationChannel& operator=(const tinygltf::AnimationChannel& src)
                {
                    Channel::operator=(src);
                    return *this;
                }

                // prepare data needed for rendering
                // asset -- asset to reference for Accessors
                // sampler -- Sampler associated with this channel
                void allocateGLResources(Asset& asset, Sampler& sampler);

                void apply(Asset& asset, Sampler& sampler, F32 time);
            };

            class ScaleChannel : public Channel
            {
            public:
                std::vector<glh::vec3f> mScales;

                const ScaleChannel& operator=(const tinygltf::AnimationChannel& src)
                {
                    Channel::operator=(src);
                    return *this;
                }

                // prepare data needed for rendering
                // asset -- asset to reference for Accessors
                // sampler -- Sampler associated with this channel
                void allocateGLResources(Asset& asset, Sampler& sampler);

                void apply(Asset& asset, Sampler& sampler, F32 time);
            };

            std::string mName;
            std::vector<Sampler> mSamplers;

            // min/max time values for all samplers combined
            F32 mMinTime = 0.f;
            F32 mMaxTime = 0.f;
            
            // current time of the animation
            F32 mTime = 0.f;

            std::vector<RotationChannel> mRotationChannels;
            std::vector<TranslationChannel> mTranslationChannels;
            std::vector<ScaleChannel> mScaleChannels;

            const Animation& operator=(const tinygltf::Animation& src);
            
            void allocateGLResources(Asset& asset);

            void update(Asset& asset, float dt);

            // apply this animation at the specified time
            void apply(Asset& asset, F32 time);
        };

    }
}
