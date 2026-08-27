/**
 * @file llmaterialcontract.h
 * @brief Backend-neutral description of one indexed legacy material draw.
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

#ifndef LL_LLMATERIALCONTRACT_H
#define LL_LLMATERIALCONTRACT_H

#include "llrendercontract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace LLRenderContract
{

inline constexpr std::uint32_t                MATERIAL_FRAME_WIDTH        = 8;
inline constexpr std::uint32_t                MATERIAL_FRAME_HEIGHT       = 8;
inline constexpr std::uint32_t                MATERIAL_TEXTURE_WIDTH      = 4;
inline constexpr std::uint32_t                MATERIAL_TEXTURE_HEIGHT     = 4;
inline constexpr std::uint32_t                MATERIAL_TEXTURE_MIP_LEVELS = 3;
inline constexpr std::uint64_t                MATERIAL_VERTEX_BUFFER_SIZE = 304;
inline constexpr std::uint64_t                MATERIAL_INDEX_BUFFER_SIZE  = 12;
inline constexpr std::array<std::uint16_t, 6> MATERIAL_INDICES{ 0, 1, 2, 0, 2, 3 };

inline constexpr std::uint64_t MATERIAL_POSITION_OFFSET  = 0;
inline constexpr std::uint64_t MATERIAL_NORMAL_OFFSET    = 64;
inline constexpr std::uint64_t MATERIAL_TEXCOORD0_OFFSET = 128;
inline constexpr std::uint64_t MATERIAL_TEXCOORD1_OFFSET = 160;
inline constexpr std::uint64_t MATERIAL_TEXCOORD2_OFFSET = 192;
inline constexpr std::uint64_t MATERIAL_COLOR_OFFSET     = 224;
inline constexpr std::uint64_t MATERIAL_TANGENT_OFFSET   = 240;

// This is the complete state consumed by the selected non-rigged material
// shader. The old 160-byte validation blob was only a placeholder and omitted
// shader-visible transforms and clipping state.
struct MaterialParameters
{
    std::array<float, 16> mModelviewMatrix{};
    std::array<float, 16> mModelviewProjectionMatrix{};
    std::array<float, 9>  mNormalMatrix{};
    std::array<float, 16> mTextureMatrix0{};
    std::array<float, 4>  mSpecularColor{};
    std::array<float, 4>  mClipPlane{};
    float                 mEnvironmentIntensity = 0.f;
    float                 mEmissiveBrightness   = 0.f;
    float                 mMirror               = 0.f;

    friend constexpr bool operator==(const MaterialParameters&, const MaterialParameters&) = default;
};

static_assert(sizeof(MaterialParameters) == 272, "material parameters must remain a 68-word packet");
static_assert(offsetof(MaterialParameters, mModelviewMatrix) == 0);
static_assert(offsetof(MaterialParameters, mModelviewProjectionMatrix) == 64);
static_assert(offsetof(MaterialParameters, mNormalMatrix) == 128);
static_assert(offsetof(MaterialParameters, mTextureMatrix0) == 164);
static_assert(offsetof(MaterialParameters, mSpecularColor) == 228);
static_assert(offsetof(MaterialParameters, mClipPlane) == 244);
static_assert(offsetof(MaterialParameters, mEnvironmentIntensity) == 260);
static_assert(offsetof(MaterialParameters, mEmissiveBrightness) == 264);
static_assert(offsetof(MaterialParameters, mMirror) == 268);

struct MaterialHandles
{
    BufferHandle   mVertexBuffer{ 1, 1 };
    BufferHandle   mIndexBuffer{ 2, 1 };
    ImageHandle    mDiffuse{ 1, 1 };
    ImageHandle    mNormal{ 2, 1 };
    ImageHandle    mSpecular{ 3, 1 };
    ImageHandle    mGBuffer0{ 4, 1 };
    ImageHandle    mGBuffer1{ 5, 1 };
    ImageHandle    mGBuffer2{ 6, 1 };
    ImageHandle    mDepth{ 7, 1 };
    SamplerHandle  mSampler{ 1, 1 };
    PipelineHandle mPipeline{ 1, 1 };
    PassId         mPass{ 1 };

    friend constexpr bool operator==(const MaterialHandles&, const MaterialHandles&) = default;
};

struct MaterialInputs
{
    std::uint64_t      mFrame = 0;
    MaterialHandles    mHandles;
    MaterialParameters mParameters;

    friend constexpr bool operator==(const MaterialInputs&, const MaterialInputs&) = default;
};

bool validMaterialParameters(const MaterialParameters& parameters) noexcept;

// Returns no packet when policy supplied an invalid handle or parameter value.
std::optional<FrameSnapshot> buildMaterialFrame(const MaterialInputs& inputs);

// Accepts only the fixed Stage 12 material packet and returns owned values.
std::optional<MaterialInputs> decodeMaterialFrame(const FrameSnapshot& frame);

} // namespace LLRenderContract

#endif
