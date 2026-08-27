/**
 * @file lldrawpacketcontract.cpp
 * @brief Builder and validation for one prepared legacy material draw.
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

#include "lldrawpacketcontract.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace LLRenderContract
{
namespace
{

    bool validIndexType(IndexType type) noexcept
    {
        switch (type)
        {
            case IndexType::UInt16:
            case IndexType::UInt32:
                return true;
        }
        return false;
    }

    bool validSubresourceRange(const ImageSubresourceRange& range) noexcept
    {
        return range.mMipLevelCount != 0 && range.mArrayLayerCount != 0 &&
               range.mBaseMipLevel <= std::numeric_limits<std::uint32_t>::max() - range.mMipLevelCount &&
               range.mBaseArrayLayer <= std::numeric_limits<std::uint32_t>::max() - range.mArrayLayerCount;
    }

    bool validDescriptor(const DrawTextureInput& descriptor) noexcept
    {
        return static_cast<bool>(descriptor.mImage) && static_cast<bool>(descriptor.mSampler) && validSubresourceRange(descriptor.mRange);
    }

    bool sameColorTarget(const ColorTargetState& left, const ColorTargetState& right) noexcept
    {
        return left.mFormat == right.mFormat && left.mBlendEnabled == right.mBlendEnabled && left.mWriteMask == right.mWriteMask;
    }

    bool validPipelineKey(const LegacyNormSpecPipelineKey& key) noexcept
    {
        return key.mProgram.mName == LEGACY_NORMSPEC_PIPELINE_NAME && key.mProgram.mVariant == LEGACY_NORMSPEC_PIPELINE_VARIANT &&
               key.mVertexLayout == DrawVertexLayout::LegacyMaterialNormSpec && key.mTopology == PrimitiveTopology::TriangleList &&
               key.mCullMode == CullMode::Back && key.mFrontFace == FrontFace::CounterClockwise && key.mDepthTestEnabled &&
               key.mDepthWriteEnabled && key.mDepthCompare == CompareOp::LessOrEqual && key.mSamples == 1 &&
               key.mColorTargets.size() == 3 && sameColorTarget(key.mColorTargets[0], { PixelFormat::RGBA8Unorm, false, 0xf }) &&
               sameColorTarget(key.mColorTargets[1], { PixelFormat::RGBA8Unorm, false, 0xf }) &&
               sameColorTarget(key.mColorTargets[2], { PixelFormat::RGBA16Unorm, false, 0xf }) &&
               key.mDepthFormat == PixelFormat::Depth24Unorm;
    }

    bool finite(const DrawMatrix4& matrix) noexcept
    {
        return std::all_of(matrix.begin(), matrix.end(), [](float value) { return std::isfinite(value); });
    }

    bool unitRange(float value) noexcept
    {
        return std::isfinite(value) && value >= 0.f && value <= 1.f;
    }

    bool validHandles(const DrawPacketHandles& handles) noexcept
    {
        return static_cast<bool>(handles.mVertexBuffer) && static_cast<bool>(handles.mIndexBuffer) &&
               handles.mVertexBuffer != handles.mIndexBuffer && static_cast<bool>(handles.mPipeline);
    }

    bool validRanges(const LegacyNormSpecDrawPacket& packet) noexcept
    {
        return packet.mIndexCount != 0 && packet.mFirstIndex <= std::numeric_limits<std::uint32_t>::max() - packet.mIndexCount &&
               packet.mMinVertex <= packet.mMaxVertex;
    }

    bool validConstants(const LegacyNormSpecDrawPacket& packet) noexcept
    {
        return finite(packet.mModelMatrix) && finite(packet.mDiffuseTextureMatrix) &&
               std::all_of(packet.mSpecularRGBA.begin(), packet.mSpecularRGBA.end(), unitRange) &&
               unitRange(packet.mEnvironmentIntensity) && unitRange(packet.mAlphaCutoff) &&
               (packet.mEmissiveBrightness == 0.f || packet.mEmissiveBrightness == 1.f);
    }

} // namespace

ShaderProgramKey legacyNormSpecProgramKey()
{
    return { LEGACY_NORMSPEC_PIPELINE_NAME, LEGACY_NORMSPEC_PIPELINE_VARIANT };
}

LegacyNormSpecPipelineKey legacyNormSpecPipelineKey()
{
    LegacyNormSpecPipelineKey key;
    key.mProgram           = legacyNormSpecProgramKey();
    key.mVertexLayout      = DrawVertexLayout::LegacyMaterialNormSpec;
    key.mTopology          = PrimitiveTopology::TriangleList;
    key.mCullMode          = CullMode::Back;
    key.mFrontFace         = FrontFace::CounterClockwise;
    key.mDepthTestEnabled  = true;
    key.mDepthWriteEnabled = true;
    key.mDepthCompare      = CompareOp::LessOrEqual;
    key.mSamples           = 1;
    key.mColorTargets      = { { PixelFormat::RGBA8Unorm, false, 0xf },
                               { PixelFormat::RGBA8Unorm, false, 0xf },
                               { PixelFormat::RGBA16Unorm, false, 0xf } };
    key.mDepthFormat       = PixelFormat::Depth24Unorm;
    return key;
}

bool operator==(const LegacyNormSpecPipelineKey& left, const LegacyNormSpecPipelineKey& right)
{
    return left.mProgram.mName == right.mProgram.mName && left.mProgram.mVariant == right.mProgram.mVariant &&
           left.mVertexLayout == right.mVertexLayout && left.mTopology == right.mTopology && left.mCullMode == right.mCullMode &&
           left.mFrontFace == right.mFrontFace && left.mDepthTestEnabled == right.mDepthTestEnabled &&
           left.mDepthWriteEnabled == right.mDepthWriteEnabled && left.mDepthCompare == right.mDepthCompare &&
           left.mSamples == right.mSamples && left.mDepthFormat == right.mDepthFormat &&
           left.mColorTargets.size() == right.mColorTargets.size() &&
           std::equal(left.mColorTargets.begin(), left.mColorTargets.end(), right.mColorTargets.begin(), sameColorTarget);
}

bool operator==(const LegacyNormSpecDrawPacket& left, const LegacyNormSpecDrawPacket& right)
{
    return left.mFrame == right.mFrame && left.mPass == right.mPass && left.mHandles == right.mHandles &&
           left.mDescriptors == right.mDescriptors && left.mPipelineKey == right.mPipelineKey && left.mIndexType == right.mIndexType &&
           left.mFirstIndex == right.mFirstIndex && left.mIndexCount == right.mIndexCount && left.mMinVertex == right.mMinVertex &&
           left.mMaxVertex == right.mMaxVertex && left.mModelMatrix == right.mModelMatrix &&
           left.mDiffuseTextureMatrix == right.mDiffuseTextureMatrix && left.mSpecularRGBA == right.mSpecularRGBA &&
           left.mEnvironmentIntensity == right.mEnvironmentIntensity && left.mAlphaCutoff == right.mAlphaCutoff &&
           left.mEmissiveBrightness == right.mEmissiveBrightness;
}

bool validLegacyNormSpecDrawPacket(const LegacyNormSpecDrawPacket& packet) noexcept
{
    return packet.mFrame != 0 && static_cast<bool>(packet.mPass) && validHandles(packet.mHandles) &&
           validDescriptor(packet.mDescriptors.mDiffuse) && validDescriptor(packet.mDescriptors.mNormal) &&
           validDescriptor(packet.mDescriptors.mSpecular) && validPipelineKey(packet.mPipelineKey) && validIndexType(packet.mIndexType) &&
           validRanges(packet) && validConstants(packet);
}

std::optional<LegacyNormSpecDrawPacket> buildLegacyNormSpecDrawPacket(const LegacyNormSpecDrawInputs& inputs)
{
    LegacyNormSpecDrawPacket packet;
    packet.mFrame                = inputs.mFrame;
    packet.mPass                 = inputs.mPass;
    packet.mHandles              = inputs.mHandles;
    packet.mDescriptors          = inputs.mDescriptors;
    packet.mPipelineKey          = inputs.mPipelineKey;
    packet.mIndexType            = inputs.mIndexType;
    packet.mFirstIndex           = inputs.mFirstIndex;
    packet.mIndexCount           = inputs.mIndexCount;
    packet.mMinVertex            = inputs.mMinVertex;
    packet.mMaxVertex            = inputs.mMaxVertex;
    packet.mModelMatrix          = inputs.mModelMatrix;
    packet.mDiffuseTextureMatrix = inputs.mDiffuseTextureMatrix;
    packet.mSpecularRGBA         = inputs.mSpecularRGBA;
    packet.mEnvironmentIntensity = inputs.mEnvironmentIntensity;
    packet.mAlphaCutoff          = inputs.mAlphaCutoff;
    packet.mEmissiveBrightness   = inputs.mEmissiveBrightness;

    if (!validLegacyNormSpecDrawPacket(packet))
    {
        return std::nullopt;
    }
    return packet;
}

} // namespace LLRenderContract
