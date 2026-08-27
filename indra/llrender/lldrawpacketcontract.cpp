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
#include <initializer_list>
#include <iterator>
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

    bool sameColorTargets(const std::vector<ColorTargetState>& targets, std::initializer_list<PixelFormat> formats) noexcept
    {
        if (targets.size() != formats.size())
        {
            return false;
        }

        return std::equal(targets.begin(), targets.end(), formats.begin(), [](const ColorTargetState& target, PixelFormat format)
                          { return sameColorTarget(target, { format, false, 0xf }); });
    }

    LegacyNormSpecPipelineKey pipelineKey(LegacyNormSpecShaderVariant variant, LegacyNormSpecTargetProfile profile,
                                          std::initializer_list<PixelFormat> formats)
    {
        LegacyNormSpecPipelineKey key;
        key.mProgram           = variant == LEGACY_NORMSPEC_DIAGNOSTIC_SHADER_VARIANT ? legacyNormSpecDiagnosticProgramKey()
                                                                                      : legacyNormSpecProductionProgramKey();
        key.mShaderVariant     = variant;
        key.mTargetProfile     = profile;
        key.mVertexLayout      = DrawVertexLayout::LegacyMaterialNormSpec;
        key.mTopology          = PrimitiveTopology::TriangleList;
        key.mCullMode          = CullMode::Back;
        key.mFrontFace         = FrontFace::CounterClockwise;
        key.mDepthTestEnabled  = true;
        key.mDepthWriteEnabled = true;
        key.mDepthCompare      = CompareOp::LessOrEqual;
        key.mSamples           = 1;
        key.mColorTargets.reserve(formats.size());
        std::transform(formats.begin(), formats.end(), std::back_inserter(key.mColorTargets),
                       [](PixelFormat format) { return ColorTargetState{ format, false, 0xf }; });
        key.mDepthFormat = PixelFormat::Depth24Unorm;
        return key;
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

bool validLegacyNormSpecShaderVariant(LegacyNormSpecShaderVariant variant) noexcept
{
    switch (variant.mEmissive)
    {
        case LegacyNormSpecEmissive::Disabled:
        case LegacyNormSpecEmissive::Enabled:
            break;
        default:
            return false;
    }

    switch (variant.mShadowAssembly)
    {
        case ShadowAssembly::Disabled:
        case ShadowAssembly::Sun:
        case ShadowAssembly::SunAndSpot:
            return true;
    }
    return false;
}

std::optional<std::uint64_t> encodeLegacyNormSpecShaderVariant(LegacyNormSpecShaderVariant variant) noexcept
{
    if (!validLegacyNormSpecShaderVariant(variant))
    {
        return std::nullopt;
    }

    return static_cast<std::uint64_t>(variant.mEmissive) | (static_cast<std::uint64_t>(variant.mShadowAssembly) << 1);
}

std::optional<LegacyNormSpecShaderVariant> decodeLegacyNormSpecShaderVariant(std::uint64_t encoded) noexcept
{
    constexpr std::uint64_t KNOWN_BITS = 0x7;
    if ((encoded & ~KNOWN_BITS) != 0)
    {
        return std::nullopt;
    }

    const auto variant = LegacyNormSpecShaderVariant{ static_cast<LegacyNormSpecEmissive>(encoded & 0x1),
                                                      static_cast<ShadowAssembly>((encoded >> 1) & 0x3) };
    if (!validLegacyNormSpecShaderVariant(variant))
    {
        return std::nullopt;
    }
    return variant;
}

ShaderProgramKey legacyNormSpecDiagnosticProgramKey()
{
    return { LEGACY_NORMSPEC_PIPELINE_NAME, LEGACY_NORMSPEC_DIAGNOSTIC_VARIANT };
}

ShaderProgramKey legacyNormSpecProductionProgramKey()
{
    return { LEGACY_NORMSPEC_PIPELINE_NAME, LEGACY_NORMSPEC_PRODUCTION_VARIANT };
}

LegacyNormSpecPipelineKey legacyNormSpecDiagnosticPipelineKey()
{
    return pipelineKey(LEGACY_NORMSPEC_DIAGNOSTIC_SHADER_VARIANT, LegacyNormSpecTargetProfile::DiagnosticThreeTarget,
                       { PixelFormat::RGBA8Unorm, PixelFormat::RGBA8Unorm, PixelFormat::RGBA16Unorm });
}

LegacyNormSpecPipelineKey legacyNormSpecModernHDRPipelineKey()
{
    return pipelineKey(LEGACY_NORMSPEC_PRODUCTION_SHADER_VARIANT, LegacyNormSpecTargetProfile::ModernHDR,
                       { PixelFormat::RGBA8Unorm, PixelFormat::RGBA8Unorm, PixelFormat::RGBA16Unorm, PixelFormat::RGB16Float });
}

LegacyNormSpecPipelineKey legacyNormSpecCompatibilityPipelineKey()
{
    return pipelineKey(LEGACY_NORMSPEC_PRODUCTION_SHADER_VARIANT, LegacyNormSpecTargetProfile::Compatibility,
                       { PixelFormat::RGBA8Unorm, PixelFormat::RGBA8Unorm, PixelFormat::RGB10A2Unorm, PixelFormat::RGB8Unorm });
}

bool validLegacyNormSpecPipelineKey(const LegacyNormSpecPipelineKey& key) noexcept
{
    const auto decoded_variant = decodeLegacyNormSpecShaderVariant(key.mProgram.mVariant);
    if (key.mProgram.mName != LEGACY_NORMSPEC_PIPELINE_NAME || !decoded_variant || *decoded_variant != key.mShaderVariant ||
        key.mVertexLayout != DrawVertexLayout::LegacyMaterialNormSpec || key.mTopology != PrimitiveTopology::TriangleList ||
        key.mCullMode != CullMode::Back || key.mFrontFace != FrontFace::CounterClockwise || !key.mDepthTestEnabled ||
        !key.mDepthWriteEnabled || key.mDepthCompare != CompareOp::LessOrEqual || key.mSamples != 1 ||
        key.mDepthFormat != PixelFormat::Depth24Unorm)
    {
        return false;
    }

    switch (key.mTargetProfile)
    {
        case LegacyNormSpecTargetProfile::DiagnosticThreeTarget:
            return key.mShaderVariant == LEGACY_NORMSPEC_DIAGNOSTIC_SHADER_VARIANT &&
                   sameColorTargets(key.mColorTargets, { PixelFormat::RGBA8Unorm, PixelFormat::RGBA8Unorm, PixelFormat::RGBA16Unorm });
        case LegacyNormSpecTargetProfile::ModernHDR:
            return key.mShaderVariant == LEGACY_NORMSPEC_PRODUCTION_SHADER_VARIANT &&
                   sameColorTargets(key.mColorTargets, { PixelFormat::RGBA8Unorm, PixelFormat::RGBA8Unorm, PixelFormat::RGBA16Unorm,
                                                         PixelFormat::RGB16Float });
        case LegacyNormSpecTargetProfile::Compatibility:
            return key.mShaderVariant == LEGACY_NORMSPEC_PRODUCTION_SHADER_VARIANT &&
                   sameColorTargets(key.mColorTargets, { PixelFormat::RGBA8Unorm, PixelFormat::RGBA8Unorm, PixelFormat::RGB10A2Unorm,
                                                         PixelFormat::RGB8Unorm });
    }
    return false;
}

bool operator==(const LegacyNormSpecPipelineKey& left, const LegacyNormSpecPipelineKey& right)
{
    return left.mProgram.mName == right.mProgram.mName && left.mProgram.mVariant == right.mProgram.mVariant &&
           left.mShaderVariant == right.mShaderVariant && left.mTargetProfile == right.mTargetProfile &&
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
           validDescriptor(packet.mDescriptors.mSpecular) && validLegacyNormSpecPipelineKey(packet.mPipelineKey) &&
           validIndexType(packet.mIndexType) && validRanges(packet) && validConstants(packet);
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
