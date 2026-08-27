/**
 * @file llmaterialdiagnostic.h
 * @brief Deterministic fixture and artifact for the indexed material draw.
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

#ifndef LL_LLMATERIALDIAGNOSTIC_H
#define LL_LLMATERIALDIAGNOSTIC_H

#include "llmaterialcontract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace LLRenderContract
{

inline constexpr std::uint32_t MATERIAL_DIAGNOSTIC_CHANNELS    = 4;
inline constexpr std::size_t   MATERIAL_DIAGNOSTIC_PIXEL_COUNT = static_cast<std::size_t>(MATERIAL_FRAME_WIDTH) * MATERIAL_FRAME_HEIGHT;
inline constexpr std::size_t   MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT = MATERIAL_DIAGNOSTIC_PIXEL_COUNT * MATERIAL_DIAGNOSTIC_CHANNELS;
inline constexpr std::size_t   MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT = MATERIAL_DIAGNOSTIC_PIXEL_COUNT;

inline constexpr std::size_t MATERIAL_TEXTURE_COUNT           = 3;
inline constexpr std::size_t MATERIAL_TEXTURE_TEXEL_COUNT     = 16 + 4 + 1;
inline constexpr std::size_t MATERIAL_TEXTURE_COMPONENT_COUNT = MATERIAL_TEXTURE_TEXEL_COUNT * MATERIAL_DIAGNOSTIC_CHANNELS;
inline constexpr std::array<std::size_t, MATERIAL_TEXTURE_MIP_LEVELS> MATERIAL_TEXTURE_MIP_BYTE_OFFSETS{ 0, 64, 80 };
inline constexpr std::array<std::size_t, MATERIAL_TEXTURE_MIP_LEVELS> MATERIAL_TEXTURE_MIP_BYTE_SIZES{ 64, 16, 4 };

inline constexpr std::uint32_t MATERIAL_DIAGNOSTIC_FIXTURE_VERSION = 1;
inline constexpr std::uint32_t MATERIAL_ARTIFACT_SCHEMA_VERSION    = 1;
inline constexpr std::size_t   MATERIAL_ARTIFACT_BYTE_SIZE         = 3436;

inline constexpr float MATERIAL_RGBA8_TOLERANCE   = 0.f;
inline constexpr float MATERIAL_RGBA16_TOLERANCE  = 0.f;
inline constexpr float MATERIAL_DEPTH24_TOLERANCE = 0.f;

struct MaterialFixture
{
    Extent2D  mExtent{ MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT };
    RowOrigin mRowOrigin = RowOrigin::BottomLeft;

    // Native viewer planar layout. Float arrays and color bytes occupy the
    // offsets published by llmaterialcontract.h.
    std::array<std::uint8_t, MATERIAL_VERTEX_BUFFER_SIZE> mVertexBytes{};
    std::array<std::uint16_t, 6>                          mIndices{};

    // Diffuse, normal, and specular. Each flat array contains the 4x4, 2x2,
    // and 1x1 RGBA8 mip levels in that order.
    std::array<std::array<std::uint8_t, MATERIAL_TEXTURE_COMPONENT_COUNT>, MATERIAL_TEXTURE_COUNT> mTextureRGBA8{};

    MaterialParameters mParameters;

    // The depth codes are the loaded contents. Color sentinels make failed
    // preflight observable before the canonical color clears run.
    std::array<std::uint32_t, MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT> mDepth24{};
    std::array<std::uint8_t, MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT>  mGBuffer0SentinelRGBA8{};
    std::array<std::uint8_t, MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT>  mGBuffer1SentinelRGBA8{};
    std::array<std::uint16_t, MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT> mGBuffer2SentinelRGBA16{};
};

static_assert(sizeof(std::array<std::uint8_t, MATERIAL_VERTEX_BUFFER_SIZE>) == MATERIAL_VERTEX_BUFFER_SIZE);
static_assert(sizeof(std::array<std::uint16_t, 6>) == MATERIAL_INDEX_BUFFER_SIZE);

MaterialFixture makeMaterialFixture();
std::uint64_t   materialFixtureFingerprint();

struct MaterialCase
{
    MaterialInputs mInputs;
    FrameSnapshot  mFrame;
};

MaterialCase makeMaterialCase();

float materialUnorm8(std::uint8_t code) noexcept;
float materialUnorm16(std::uint16_t code) noexcept;
float materialDepth24(std::uint32_t code) noexcept;

struct MaterialArtifact
{
    // Bottom-left row-major values. The three color vectors are RGBA; depth
    // has one component per pixel.
    std::vector<float> mGBuffer0RGBA8;
    std::vector<float> mGBuffer1RGBA8;
    std::vector<float> mGBuffer2RGBA16;
    std::vector<float> mDepth24;

    friend bool operator==(const MaterialArtifact&, const MaterialArtifact&) = default;
};

MaterialArtifact makeMaterialArtifact();
bool             validateMaterialArtifact(const MaterialArtifact& artifact, std::string* error = nullptr);
bool             encodeMaterialArtifact(const MaterialArtifact& artifact, std::vector<std::uint8_t>& encoded, std::string* error = nullptr);
bool             decodeMaterialArtifact(const std::vector<std::uint8_t>& encoded, MaterialArtifact& artifact, std::string* error = nullptr);

// The destination must not exist. The writer reserves an unpredictable sibling
// file, writes it completely, then publishes it atomically without replacement.
bool writeMaterialArtifact(const std::filesystem::path& destination, const MaterialArtifact& artifact, std::string* error = nullptr);
bool readMaterialArtifact(const std::filesystem::path& source, MaterialArtifact& artifact, std::string* error = nullptr);

struct MaterialComparisonStats
{
    bool          mComparable           = false;
    bool          mMatch                = false;
    std::size_t   mComparedComponents   = 0;
    std::size_t   mMismatchCount        = 0;
    double        mMaximumAbsoluteError = 0.0;
    std::uint32_t mFirstMismatchPlane   = 0;
    std::size_t   mFirstMismatchPixel   = 0;
    std::uint32_t mFirstMismatchChannel = 0;
    float         mFirstReference       = 0.f;
    float         mFirstCandidate       = 0.f;
    float         mFirstTolerance       = 0.f;
    std::string   mError;
};

MaterialComparisonStats compareMaterialArtifacts(const MaterialArtifact& reference, const MaterialArtifact& candidate);

} // namespace LLRenderContract

#endif
