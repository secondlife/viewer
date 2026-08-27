/**
 * @file lldrawpacketcontract.h
 * @brief Owned API-neutral packet for one prepared legacy material draw.
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

#ifndef LL_LLDRAWPACKETCONTRACT_H
#define LL_LLDRAWPACKETCONTRACT_H

#include "llrendercontract.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace LLRenderContract
{

inline constexpr char LEGACY_NORMSPEC_PIPELINE_NAME[] = "deferred.material.normspec";

using DrawMatrix4 = std::array<float, 16>;

inline constexpr DrawMatrix4 DRAW_IDENTITY_MATRIX4{ 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };

struct DrawPacketHandles
{
    BufferHandle   mVertexBuffer;
    BufferHandle   mIndexBuffer;
    PipelineHandle mPipeline;

    friend constexpr bool operator==(const DrawPacketHandles&, const DrawPacketHandles&) = default;
};

struct DrawTextureInput
{
    ImageHandle           mImage;
    SamplerHandle         mSampler;
    ImageSubresourceRange mRange;

    friend constexpr bool operator==(const DrawTextureInput&, const DrawTextureInput&) = default;
};

struct LegacyNormSpecDescriptors
{
    DrawTextureInput mDiffuse;
    DrawTextureInput mNormal;
    DrawTextureInput mSpecular;

    friend constexpr bool operator==(const LegacyNormSpecDescriptors&, const LegacyNormSpecDescriptors&) = default;
};

enum class DrawVertexLayout : std::uint8_t
{
    LegacyMaterialNormSpec
};

enum class LegacyNormSpecEmissive : std::uint8_t
{
    Disabled = 0,
    Enabled  = 1
};

enum class ShadowAssembly : std::uint8_t
{
    Disabled   = 0,
    Sun        = 1,
    SunAndSpot = 2
};

struct LegacyNormSpecShaderVariant
{
    LegacyNormSpecEmissive mEmissive       = LegacyNormSpecEmissive::Disabled;
    ShadowAssembly         mShadowAssembly = ShadowAssembly::Disabled;

    friend constexpr bool operator==(const LegacyNormSpecShaderVariant&, const LegacyNormSpecShaderVariant&) = default;
};

inline constexpr LegacyNormSpecShaderVariant LEGACY_NORMSPEC_DIAGNOSTIC_SHADER_VARIANT{ LegacyNormSpecEmissive::Disabled,
                                                                                        ShadowAssembly::Disabled };
inline constexpr LegacyNormSpecShaderVariant LEGACY_NORMSPEC_PRODUCTION_SHADER_VARIANT{ LegacyNormSpecEmissive::Enabled,
                                                                                        ShadowAssembly::SunAndSpot };
inline constexpr std::uint64_t               LEGACY_NORMSPEC_DIAGNOSTIC_VARIANT =
    static_cast<std::uint64_t>(LegacyNormSpecEmissive::Disabled) | (static_cast<std::uint64_t>(ShadowAssembly::Disabled) << 1);
inline constexpr std::uint64_t LEGACY_NORMSPEC_PRODUCTION_VARIANT =
    static_cast<std::uint64_t>(LegacyNormSpecEmissive::Enabled) | (static_cast<std::uint64_t>(ShadowAssembly::SunAndSpot) << 1);

enum class LegacyNormSpecTargetProfile : std::uint8_t
{
    DiagnosticThreeTarget,
    ModernHDR,
    Compatibility
};

struct LegacyNormSpecPipelineKey
{
    ShaderProgramKey              mProgram{ LEGACY_NORMSPEC_PIPELINE_NAME, LEGACY_NORMSPEC_DIAGNOSTIC_VARIANT };
    LegacyNormSpecShaderVariant   mShaderVariant     = LEGACY_NORMSPEC_DIAGNOSTIC_SHADER_VARIANT;
    LegacyNormSpecTargetProfile   mTargetProfile     = LegacyNormSpecTargetProfile::DiagnosticThreeTarget;
    DrawVertexLayout              mVertexLayout      = DrawVertexLayout::LegacyMaterialNormSpec;
    PrimitiveTopology             mTopology          = PrimitiveTopology::TriangleList;
    CullMode                      mCullMode          = CullMode::Back;
    FrontFace                     mFrontFace         = FrontFace::CounterClockwise;
    bool                          mDepthTestEnabled  = false;
    bool                          mDepthWriteEnabled = false;
    CompareOp                     mDepthCompare      = CompareOp::LessOrEqual;
    std::uint32_t                 mSamples           = 1;
    std::vector<ColorTargetState> mColorTargets;
    std::optional<PixelFormat>    mDepthFormat;

    friend bool operator==(const LegacyNormSpecPipelineKey& left, const LegacyNormSpecPipelineKey& right);
    friend bool operator!=(const LegacyNormSpecPipelineKey& left, const LegacyNormSpecPipelineKey& right) { return !(left == right); }
};

struct LegacyNormSpecDrawInputs
{
    std::uint64_t             mFrame = 0;
    PassId                    mPass;
    DrawPacketHandles         mHandles;
    LegacyNormSpecDescriptors mDescriptors;
    LegacyNormSpecPipelineKey mPipelineKey;
    IndexType                 mIndexType            = IndexType::UInt16;
    std::uint32_t             mFirstIndex           = 0;
    std::uint32_t             mIndexCount           = 0;
    std::uint32_t             mMinVertex            = 0;
    std::uint32_t             mMaxVertex            = 0;
    DrawMatrix4               mModelMatrix          = DRAW_IDENTITY_MATRIX4;
    DrawMatrix4               mDiffuseTextureMatrix = DRAW_IDENTITY_MATRIX4;
    std::array<float, 4>      mSpecularRGBA{ 1.f, 1.f, 1.f, 0.5f };
    float                     mEnvironmentIntensity = 0.f;
    float                     mAlphaCutoff          = 0.5f;
    float                     mEmissiveBrightness   = 0.f;
};

struct LegacyNormSpecDrawPacket
{
    std::uint64_t             mFrame = 0;
    PassId                    mPass;
    DrawPacketHandles         mHandles;
    LegacyNormSpecDescriptors mDescriptors;
    LegacyNormSpecPipelineKey mPipelineKey;
    IndexType                 mIndexType            = IndexType::UInt16;
    std::uint32_t             mFirstIndex           = 0;
    std::uint32_t             mIndexCount           = 0;
    std::uint32_t             mMinVertex            = 0;
    std::uint32_t             mMaxVertex            = 0;
    DrawMatrix4               mModelMatrix          = DRAW_IDENTITY_MATRIX4;
    DrawMatrix4               mDiffuseTextureMatrix = DRAW_IDENTITY_MATRIX4;
    std::array<float, 4>      mSpecularRGBA{ 1.f, 1.f, 1.f, 0.5f };
    float                     mEnvironmentIntensity = 0.f;
    float                     mAlphaCutoff          = 0.5f;
    float                     mEmissiveBrightness   = 0.f;

    friend bool operator==(const LegacyNormSpecDrawPacket& left, const LegacyNormSpecDrawPacket& right);
    friend bool operator!=(const LegacyNormSpecDrawPacket& left, const LegacyNormSpecDrawPacket& right) { return !(left == right); }
};

bool validLegacyNormSpecShaderVariant(LegacyNormSpecShaderVariant variant) noexcept;

std::optional<std::uint64_t>               encodeLegacyNormSpecShaderVariant(LegacyNormSpecShaderVariant variant) noexcept;
std::optional<LegacyNormSpecShaderVariant> decodeLegacyNormSpecShaderVariant(std::uint64_t encoded) noexcept;

ShaderProgramKey legacyNormSpecDiagnosticProgramKey();
ShaderProgramKey legacyNormSpecProductionProgramKey();

LegacyNormSpecPipelineKey legacyNormSpecDiagnosticPipelineKey();
LegacyNormSpecPipelineKey legacyNormSpecModernHDRPipelineKey();
LegacyNormSpecPipelineKey legacyNormSpecCompatibilityPipelineKey();

bool validLegacyNormSpecPipelineKey(const LegacyNormSpecPipelineKey& key) noexcept;

bool validLegacyNormSpecDrawPacket(const LegacyNormSpecDrawPacket& packet) noexcept;

std::optional<LegacyNormSpecDrawPacket> buildLegacyNormSpecDrawPacket(const LegacyNormSpecDrawInputs& inputs);

} // namespace LLRenderContract

#endif // LL_LLDRAWPACKETCONTRACT_H
