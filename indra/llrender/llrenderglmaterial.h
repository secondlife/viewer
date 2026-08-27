/**
 * @file llrenderglmaterial.h
 * @brief Narrow OpenGL registry and executor for the Stage 12 material draw.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the license only.
 * $/LicenseInfo$
 */

#ifndef LL_LLRENDERGLMATERIAL_H
#define LL_LLRENDERGLMATERIAL_H

#include "llmaterialcontract.h"
#include "stdtypes.h"
#include "llgltypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class LLGLSLShader;
class LLRenderTarget;
class LLVertexBuffer;

namespace LLRenderGLMaterial
{

struct SampledImage
{
    LLGLuint                      mTexture = 0;
    LLRenderContract::PixelFormat mFormat  = LLRenderContract::PixelFormat::RGBA8Unorm;
    LLRenderContract::Extent2D    mExtent;
    std::uint32_t                 mMipLevels = 0;

    friend constexpr bool operator==(const SampledImage& left, const SampledImage& right)
    {
        return left.mTexture == right.mTexture && left.mFormat == right.mFormat && left.mExtent.mWidth == right.mExtent.mWidth &&
               left.mExtent.mHeight == right.mExtent.mHeight && left.mMipLevels == right.mMipLevels;
    }
};

struct Sampler
{
    LLRenderContract::Filter      mMinFilter     = LLRenderContract::Filter::Linear;
    LLRenderContract::Filter      mMagFilter     = LLRenderContract::Filter::Linear;
    LLRenderContract::MipFilter   mMipFilter     = LLRenderContract::MipFilter::Linear;
    LLRenderContract::AddressMode mAddressU      = LLRenderContract::AddressMode::Repeat;
    LLRenderContract::AddressMode mAddressV      = LLRenderContract::AddressMode::Repeat;
    float                         mMaxAnisotropy = 8.f;

    friend constexpr bool operator==(const Sampler&, const Sampler&) = default;
};

enum class TargetAspect
{
    Color,
    Depth
};

struct TargetImage
{
    LLRenderTarget* mTarget     = nullptr;
    TargetAspect    mAspect     = TargetAspect::Color;
    std::uint32_t   mAttachment = 0;
};

// The registry borrows viewer objects for one synchronous replay. It owns no GL
// object and never dereferences a viewer pointer during registration/resolution.
class Registry
{
public:
    bool addVertexBuffer(LLRenderContract::BufferHandle vertex_handle, LLRenderContract::BufferHandle index_handle, LLVertexBuffer* buffer)
    {
        if (mBuffer.mBuffer || !vertex_handle || !index_handle || !buffer || vertex_handle.mIndex == index_handle.mIndex)
        {
            return false;
        }
        mBuffer = { vertex_handle, index_handle, buffer };
        return true;
    }

    bool addSampledImage(LLRenderContract::ImageHandle handle, SampledImage image)
    {
        if (!handle || image.mTexture == 0 || image.mExtent.mWidth == 0 || image.mExtent.mHeight == 0 || image.mMipLevels == 0 ||
            hasImageIndex(handle.mIndex) || hasTexture(image.mTexture))
        {
            return false;
        }
        mSampledImages.push_back({ handle, image });
        return true;
    }

    bool addRenderTarget(const std::array<LLRenderContract::ImageHandle, 3>& colors,
                         LLRenderContract::ImageHandle                       depth,
                         LLRenderTarget*                                     target)
    {
        if (mTarget.mTarget || !target || !depth)
        {
            return false;
        }

        std::array<std::uint32_t, 4> indices{ colors[0].mIndex, colors[1].mIndex, colors[2].mIndex, depth.mIndex };
        if (!colors[0] || !colors[1] || !colors[2] ||
            std::any_of(indices.begin(), indices.end(), [this](std::uint32_t index) { return hasImageIndex(index); }))
        {
            return false;
        }
        std::sort(indices.begin(), indices.end());
        if (std::adjacent_find(indices.begin(), indices.end()) != indices.end())
        {
            return false;
        }

        mTarget = { colors, depth, target };
        return true;
    }

    bool addSampler(LLRenderContract::SamplerHandle handle, Sampler sampler)
    {
        if (mSampler.mPresent || !handle || !std::isfinite(sampler.mMaxAnisotropy) || sampler.mMaxAnisotropy < 1.f)
        {
            return false;
        }
        mSampler = { handle, sampler, true };
        return true;
    }

    bool addPipeline(LLRenderContract::PipelineHandle handle, LLRenderContract::ShaderProgramKey program, LLGLSLShader* shader)
    {
        if (mPipeline.mShader || !handle || !shader || program.mName.empty())
        {
            return false;
        }
        mPipeline = { handle, std::move(program), shader };
        return true;
    }

    LLVertexBuffer* resolveVertexBuffer(LLRenderContract::BufferHandle vertex_handle, LLRenderContract::BufferHandle index_handle) const
    {
        return mBuffer.mVertexHandle == vertex_handle && mBuffer.mIndexHandle == index_handle ? mBuffer.mBuffer : nullptr;
    }

    LLVertexBuffer* resolve(LLRenderContract::BufferHandle handle) const
    {
        return mBuffer.mVertexHandle == handle || mBuffer.mIndexHandle == handle ? mBuffer.mBuffer : nullptr;
    }

    const SampledImage* resolveSampledImage(LLRenderContract::ImageHandle handle) const
    {
        const auto found = std::find_if(mSampledImages.begin(), mSampledImages.end(),
                                        [handle](const SampledImageEntry& entry) { return entry.mHandle == handle; });
        return found == mSampledImages.end() ? nullptr : &found->mImage;
    }

    TargetImage resolveTargetImage(LLRenderContract::ImageHandle handle) const
    {
        if (mTarget.mDepth == handle)
        {
            return { mTarget.mTarget, TargetAspect::Depth, 0 };
        }
        for (std::uint32_t attachment = 0; attachment < mTarget.mColors.size(); ++attachment)
        {
            if (mTarget.mColors[attachment] == handle)
            {
                return { mTarget.mTarget, TargetAspect::Color, attachment };
            }
        }
        return {};
    }

    const Sampler* resolve(LLRenderContract::SamplerHandle handle) const
    {
        return mSampler.mPresent && mSampler.mHandle == handle ? &mSampler.mSampler : nullptr;
    }

    LLGLSLShader* resolve(LLRenderContract::PipelineHandle handle, const LLRenderContract::ShaderProgramKey& program) const
    {
        return mPipeline.mHandle == handle && mPipeline.mProgram.mName == program.mName && mPipeline.mProgram.mVariant == program.mVariant
                   ? mPipeline.mShader
                   : nullptr;
    }

private:
    struct BufferEntry
    {
        LLRenderContract::BufferHandle mVertexHandle;
        LLRenderContract::BufferHandle mIndexHandle;
        LLVertexBuffer*                mBuffer = nullptr;
    };

    struct SampledImageEntry
    {
        LLRenderContract::ImageHandle mHandle;
        SampledImage                  mImage;
    };

    struct TargetEntry
    {
        std::array<LLRenderContract::ImageHandle, 3> mColors{};
        LLRenderContract::ImageHandle                mDepth;
        LLRenderTarget*                              mTarget = nullptr;
    };

    struct SamplerEntry
    {
        LLRenderContract::SamplerHandle mHandle;
        Sampler                         mSampler;
        bool                            mPresent = false;
    };

    struct PipelineEntry
    {
        LLRenderContract::PipelineHandle   mHandle;
        LLRenderContract::ShaderProgramKey mProgram;
        LLGLSLShader*                      mShader = nullptr;
    };

    bool hasImageIndex(std::uint32_t index) const
    {
        if (std::any_of(mSampledImages.begin(), mSampledImages.end(),
                        [index](const SampledImageEntry& entry) { return entry.mHandle.mIndex == index; }))
        {
            return true;
        }
        return std::any_of(mTarget.mColors.begin(), mTarget.mColors.end(),
                           [index](LLRenderContract::ImageHandle handle) { return handle.mIndex == index; }) ||
               mTarget.mDepth.mIndex == index;
    }

    bool hasTexture(LLGLuint texture) const
    {
        return std::any_of(mSampledImages.begin(), mSampledImages.end(),
                           [texture](const SampledImageEntry& entry) { return entry.mImage.mTexture == texture; });
    }

    BufferEntry                    mBuffer;
    std::vector<SampledImageEntry> mSampledImages;
    TargetEntry                    mTarget;
    SamplerEntry                   mSampler;
    PipelineEntry                  mPipeline;
};

// Decodes and validates the complete fixed packet and every live GL resource
// before binding or clearing its render target. Returns false without drawing
// when any explicit or implicit Stage 12 invariant is absent.
bool execute(const LLRenderContract::FrameSnapshot& frame, const Registry& registry);

} // namespace LLRenderGLMaterial

#endif // LL_LLRENDERGLMATERIAL_H
