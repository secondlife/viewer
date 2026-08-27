/**
 * @file lltextureuploaddiagnostic.h
 * @brief Deterministic fixture and artifact for a streamed texture upload.
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

#ifndef LL_LLTEXTUREUPLOADDIAGNOSTIC_H
#define LL_LLTEXTUREUPLOADDIAGNOSTIC_H

#include "lltextureuploadcontract.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace LLRenderContract
{

inline constexpr std::uint32_t TEXTURE_UPLOAD_DIAGNOSTIC_FIXTURE_VERSION = 1;
inline constexpr std::uint32_t TEXTURE_UPLOAD_ARTIFACT_SCHEMA_VERSION    = 2;
inline constexpr std::size_t   TEXTURE_UPLOAD_ARTIFACT_BYTE_SIZE         = 480;
inline constexpr std::uint64_t TEXTURE_UPLOAD_DIAGNOSTIC_FRAME           = 1;

struct TextureUploadFixture
{
    Extent2D     mResidentExtent{ TEXTURE_UPLOAD_RESIDENT_WIDTH, TEXTURE_UPLOAD_RESIDENT_HEIGHT };
    Extent2D     mLogicalExtent{ TEXTURE_UPLOAD_LOGICAL_WIDTH, TEXTURE_UPLOAD_LOGICAL_HEIGHT };
    Extent2D     mOutputExtent{ TEXTURE_UPLOAD_OUTPUT_WIDTH, TEXTURE_UPLOAD_OUTPUT_HEIGHT };
    RowOrigin    mSourceRowOrigin = RowOrigin::TopLeft;
    std::uint32_t mSourceRowPitch = TEXTURE_UPLOAD_ROW_PITCH;
    std::uint64_t mPriorRevision  = TEXTURE_UPLOAD_PRIOR_REVISION;

    // Top-left row-major RGBA8 rows. Each row ends in four poison bytes.
    std::array<std::uint8_t, TEXTURE_UPLOAD_SOURCE_BYTE_COUNT> mSourceRGBA8{};

    // Three Float3 positions with a fourth padding word for the 16-byte stride.
    std::array<float, 12> mScreenTriangle{};

    // Bottom-left row-major RGBA8 mips at TEXTURE_UPLOAD_MIP_BYTE_OFFSETS.
    std::array<std::uint8_t, TEXTURE_UPLOAD_MIP_BYTE_COUNT> mOldMipRGBA8{};
    std::array<std::uint8_t, TEXTURE_UPLOAD_MIP_BYTE_COUNT> mReplacementSentinelMipRGBA8{};
    std::array<std::uint8_t, TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT> mOutputSentinelRGBA8{};
};

TextureUploadFixture makeTextureUploadFixture();
std::uint64_t         textureUploadFixtureFingerprint();

struct TextureUploadCase
{
    std::uint64_t         mPriorRevision = TEXTURE_UPLOAD_PRIOR_REVISION;
    StreamingUploadInputs mInputs;
    FrameSnapshot         mFrame;
};

TextureUploadCase makeTextureUploadCase();

struct TextureUploadArtifact
{
    std::uint64_t mPriorRevision = 0;
    std::uint64_t mRevision      = 0;

    // Bottom-left row-major RGBA8 bytes for the 8x4, 4x2, and 2x1 mips.
    std::array<std::vector<std::uint8_t>, TEXTURE_UPLOAD_MIP_LEVELS> mMipRGBA8;
    // Bottom-left row-major RGBA8 bytes for the 4x2 sampled output.
    std::vector<std::uint8_t> mSampledRGBA8;

    std::uint32_t mCompletionCount = 0;
    ImageHandle   mCompletedDestination{};
    std::uint64_t mCompletedRevision = 0;
    std::uint64_t mCompletedFrame    = 0;

    std::uint32_t mRetirementCount = 0;
    ImageHandle   mRetiredResource{};
    std::uint64_t mRetirementFrame = 0;

    bool mOldResolvableBefore        = false;
    bool mOldResolvableAfter         = false;
    bool mReplacementResolvableAfter = false;

    friend bool operator==(const TextureUploadArtifact&, const TextureUploadArtifact&) = default;
};

// Returns an unobserved artifact with empty pixel payloads and no lifetime evidence.
TextureUploadArtifact makeTextureUploadArtifact();

bool validateTextureUploadArtifact(const TextureUploadArtifact& artifact, std::string* error = nullptr);
bool encodeTextureUploadArtifact(const TextureUploadArtifact& artifact, std::vector<std::uint8_t>& encoded,
                                 std::string* error = nullptr);
bool decodeTextureUploadArtifact(const std::vector<std::uint8_t>& encoded, TextureUploadArtifact& artifact,
                                 std::string* error = nullptr);

// The destination must not exist. The writer reserves an unpredictable sibling
// file, writes it completely, then publishes it atomically without replacement.
// A successful publication may return a nonempty warning if sibling cleanup fails.
bool writeTextureUploadArtifact(const std::filesystem::path& destination, const TextureUploadArtifact& artifact,
                                std::string* error = nullptr);
bool readTextureUploadArtifact(const std::filesystem::path& source, TextureUploadArtifact& artifact,
                               std::string* error = nullptr);

struct TextureUploadComparisonStats
{
    bool        mComparable        = false;
    bool        mMatch             = false;
    std::size_t mComparedMipBytes  = 0;
    std::size_t mComparedSampleBytes = 0;
    std::size_t mMismatchCount     = 0;
    std::uint32_t mFirstMismatchPlane = 0;
    std::size_t mFirstMismatchByte = 0;
    std::uint8_t mFirstReference   = 0;
    std::uint8_t mFirstCandidate   = 0;
    std::string mError;
};

TextureUploadComparisonStats compareTextureUploadArtifacts(const TextureUploadArtifact& reference,
                                                            const TextureUploadArtifact& candidate);

} // namespace LLRenderContract

#endif // LL_LLTEXTUREUPLOADDIAGNOSTIC_H
