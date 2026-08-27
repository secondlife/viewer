/**
 * @file llmaterialdiagnostic_test.cpp
 * @brief Tests for the indexed material fixture and artifact.
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

#include "linden_common.h"

#include "llmaterialdiagnostic.h"
#include "lltut.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace
{
using namespace LLRenderContract;

MaterialArtifact completeArtifact()
{
    MaterialArtifact artifact = makeMaterialArtifact();
    artifact.mGBuffer0RGBA8.reserve(MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT);
    artifact.mGBuffer1RGBA8.reserve(MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT);
    artifact.mGBuffer2RGBA16.reserve(MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT);
    for (std::size_t component = 0; component < MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT; ++component)
    {
        artifact.mGBuffer0RGBA8.push_back(materialUnorm8(static_cast<std::uint8_t>((component * 17 + 3) % 256)));
        artifact.mGBuffer1RGBA8.push_back(materialUnorm8(static_cast<std::uint8_t>((component * 29 + 101) % 256)));
        artifact.mGBuffer2RGBA16.push_back(materialUnorm16(static_cast<std::uint16_t>((component * 1237 + 4001) % 65536)));
    }
    artifact.mDepth24.reserve(MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT);
    for (std::size_t pixel = 0; pixel < MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT; ++pixel)
    {
        artifact.mDepth24.push_back(materialDepth24(static_cast<std::uint32_t>((pixel * 0x1f123U + 0x345678U) % 0x1000000U)));
    }
    return artifact;
}

std::filesystem::path temporaryArtifactPath()
{
    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() / ("llmaterialdiagnostic-" + std::to_string(suffix) + ".bin");
}

float vertexFloat(const MaterialFixture& fixture, std::size_t byte_offset)
{
    float value = 0.f;
    std::memcpy(&value, fixture.mVertexBytes.data() + byte_offset, sizeof(value));
    return value;
}

} // namespace

namespace tut
{

struct material_diagnostic_test
{
};

using material_diagnostic_test_group  = test_group<material_diagnostic_test>;
using material_diagnostic_test_object = material_diagnostic_test_group::object;
material_diagnostic_test_group material_diagnostic_tests("material diagnostic");

template<>
template<>
void material_diagnostic_test_object::test<1>()
{
    const MaterialFixture fixture = makeMaterialFixture();
    ensure("fixture is 8 by 8", fixture.mExtent.mWidth == 8 && fixture.mExtent.mHeight == 8);
    ensure("fixture rows start at the bottom", fixture.mRowOrigin == RowOrigin::BottomLeft);
    ensure("fixture owns the packed viewer vertex bytes", fixture.mVertexBytes.size() == 304);
    ensure("fixture starts with the asymmetric first position",
           vertexFloat(fixture, MATERIAL_POSITION_OFFSET) == -0.82f && vertexFloat(fixture, MATERIAL_POSITION_OFFSET + 4) == -0.74f);
    ensure("fixture owns two counter-clockwise indexed triangles", fixture.mIndices == std::array<std::uint16_t, 6>{ 0, 1, 2, 0, 2, 3 });
    ensure("all three texture chains have the exact mip payload size",
           fixture.mTextureRGBA8[0].size() == 84 && fixture.mTextureRGBA8[1].size() == 84 && fixture.mTextureRGBA8[2].size() == 84);
    ensure("texture inputs and mip levels are observably distinct",
           fixture.mTextureRGBA8[0][0] != fixture.mTextureRGBA8[1][0] &&
               fixture.mTextureRGBA8[0][MATERIAL_TEXTURE_MIP_BYTE_OFFSETS[1]] != fixture.mTextureRGBA8[0][0] &&
               fixture.mTextureRGBA8[2][MATERIAL_TEXTURE_MIP_BYTE_OFFSETS[2]] !=
                   fixture.mTextureRGBA8[1][MATERIAL_TEXTURE_MIP_BYTE_OFFSETS[2]]);
    ensure("fixture carries the complete material parameters",
           sizeof(fixture.mParameters) == 272 && fixture.mParameters.mEnvironmentIntensity == 0.625f &&
               fixture.mParameters.mEmissiveBrightness == 1.f && fixture.mParameters.mMirror == 1.f &&
               fixture.mParameters.mClipPlane[0] == 1.f && fixture.mParameters.mClipPlane[3] == 0.f);
    ensure("depth values are valid 24-bit storage codes",
           std::all_of(fixture.mDepth24.begin(), fixture.mDepth24.end(), [](std::uint32_t value) { return value <= 0xffffffU; }));
    ensure("three output sentinels differ",
           fixture.mGBuffer0SentinelRGBA8[0] != fixture.mGBuffer1SentinelRGBA8[0] &&
               fixture.mGBuffer2SentinelRGBA16[0] != fixture.mGBuffer0SentinelRGBA8[0]);
    ensure_equals("fixture drift requires an explicit fixture revision", materialFixtureFingerprint(),
                  std::uint64_t{ 0x4e52ab4e75b6748bULL });

    const MaterialCase diagnostic_case = makeMaterialCase();
    auto               decoded         = decodeMaterialFrame(diagnostic_case.mFrame);
    ensure("fixture has one canonical owned frame", decoded.has_value());
    ensure("case inputs agree with decoded parameters",
           decoded->mFrame == 1 && decoded->mParameters == fixture.mParameters &&
               diagnostic_case.mInputs.mParameters == fixture.mParameters);
}

template<>
template<>
void material_diagnostic_test_object::test<2>()
{
    const MaterialArtifact    artifact = completeArtifact();
    std::string               error;
    std::vector<std::uint8_t> first_encoding;
    std::vector<std::uint8_t> second_encoding;
    ensure("complete artifact validates", validateMaterialArtifact(artifact, &error));
    ensure("complete artifact encodes", encodeMaterialArtifact(artifact, first_encoding, &error));
    ensure("material artifact has its fixed byte count", first_encoding.size() == MATERIAL_ARTIFACT_BYTE_SIZE);
    ensure("material artifact encoding is deterministic",
           encodeMaterialArtifact(artifact, second_encoding, &error) && first_encoding == second_encoding);

    MaterialArtifact decoded;
    ensure("canonical material artifact decodes", decodeMaterialArtifact(first_encoding, decoded, &error));
    ensure("material artifact round trip preserves all planes", decoded == artifact);

    const std::filesystem::path path                  = temporaryArtifactPath();
    std::filesystem::path       predictable_temporary = path;
    predictable_temporary += ".tmp";
    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove_all(predictable_temporary, cleanup_error);
    ensure("predictable temporary sibling can already exist", std::filesystem::create_directory(predictable_temporary));
    ensure("writer publishes through a unique sibling", writeMaterialArtifact(path, artifact, &error));
    ensure("writer leaves the predictable sibling untouched",
           std::filesystem::exists(path) && std::filesystem::is_directory(predictable_temporary));
    bool              unique_temporary_remains = false;
    const std::string temporary_prefix         = path.filename().string() + ".tmp.";
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path.parent_path()))
    {
        unique_temporary_remains = unique_temporary_remains || entry.path().filename().string().starts_with(temporary_prefix);
    }
    ensure("published artifact has no unique temporary sibling", !unique_temporary_remains);

    MaterialArtifact from_file;
    ensure("published artifact reads back", readMaterialArtifact(path, from_file, &error));
    ensure("file round trip preserves every plane", from_file == artifact);
    ensure("atomic writer refuses to replace an existing artifact", !writeMaterialArtifact(path, artifact, &error));
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove_all(predictable_temporary, cleanup_error);
}

template<>
template<>
void material_diagnostic_test_object::test<3>()
{
    const MaterialArtifact    artifact = completeArtifact();
    std::string               error;
    std::vector<std::uint8_t> encoded;
    ensure("baseline material artifact encodes", encodeMaterialArtifact(artifact, encoded, &error));
    const std::vector<std::uint8_t> baseline = encoded;

    MaterialArtifact invalid = artifact;
    invalid.mDepth24.pop_back();
    ensure("missing depth values are rejected", !validateMaterialArtifact(invalid, &error));

    invalid                   = artifact;
    invalid.mGBuffer0RGBA8[0] = std::numeric_limits<float>::infinity();
    ensure("non-finite color values are rejected", !validateMaterialArtifact(invalid, &error));

    invalid                   = artifact;
    invalid.mGBuffer0RGBA8[0] = 0.5f;
    ensure("non-storage RGBA8 values are rejected", !validateMaterialArtifact(invalid, &error));

    invalid                    = artifact;
    invalid.mGBuffer2RGBA16[0] = 0.5f;
    ensure("non-storage RGBA16 values are rejected", !validateMaterialArtifact(invalid, &error));

    invalid             = artifact;
    invalid.mDepth24[0] = 0.5f;
    ensure("non-storage depth24 values are rejected", !validateMaterialArtifact(invalid, &error));

    MaterialArtifact          decoded;
    std::vector<std::uint8_t> corrupt = baseline;
    corrupt[0] ^= 0xffU;
    ensure("bad material artifact magic is rejected", !decodeMaterialArtifact(corrupt, decoded, &error));

    corrupt     = baseline;
    corrupt[11] = 2;
    ensure("unknown material artifact schema is rejected", !decodeMaterialArtifact(corrupt, decoded, &error));

    corrupt     = baseline;
    corrupt[47] = 2;
    ensure("non-canonical first plane id is rejected", !decodeMaterialArtifact(corrupt, decoded, &error));

    corrupt     = baseline;
    corrupt[60] = 0x7fU;
    corrupt[61] = 0x80U;
    corrupt[62] = 0;
    corrupt[63] = 0;
    ensure("non-finite wire values are rejected", !decodeMaterialArtifact(corrupt, decoded, &error));

    corrupt     = baseline;
    corrupt[60] = 0x80U;
    corrupt[61] = 0;
    corrupt[62] = 0;
    corrupt[63] = 0;
    ensure("negative zero is not a canonical wire value", !decodeMaterialArtifact(corrupt, decoded, &error));

    corrupt = baseline;
    corrupt.push_back(0);
    ensure("trailing material artifact bytes are rejected", !decodeMaterialArtifact(corrupt, decoded, &error));
}

template<>
template<>
void material_diagnostic_test_object::test<4>()
{
    MaterialArtifact reference;
    reference.mGBuffer0RGBA8.assign(MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT, 0.f);
    reference.mGBuffer1RGBA8.assign(MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT, 0.f);
    reference.mGBuffer2RGBA16.assign(MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT, 0.f);
    reference.mDepth24.assign(MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT, 0.f);

    MaterialArtifact candidate   = reference;
    candidate.mGBuffer0RGBA8[0]  = materialUnorm8(1);
    candidate.mGBuffer2RGBA16[0] = materialUnorm16(1);
    candidate.mDepth24[0]        = materialDepth24(1);

    MaterialComparisonStats stats = compareMaterialArtifacts(reference, candidate);
    ensure("one storage code per format mismatches", stats.mComparable && !stats.mMatch && stats.mMismatchCount == 3);
    ensure("comparison reports every color and depth component",
           stats.mComparedComponents == MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT * 3 + MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT);
    ensure("first one-code mismatch identifies its plane, pixel, and channel",
           stats.mFirstMismatchPlane == 1 && stats.mFirstMismatchPixel == 0 && stats.mFirstMismatchChannel == 0 &&
               stats.mFirstReference == 0.f && stats.mFirstCandidate == materialUnorm8(1) &&
               stats.mFirstTolerance == MATERIAL_RGBA8_TOLERANCE);

    candidate.mGBuffer0RGBA8[1]  = materialUnorm8(2);
    candidate.mGBuffer2RGBA16[1] = materialUnorm16(2);
    candidate.mDepth24[1]        = materialDepth24(2);
    stats                        = compareMaterialArtifacts(reference, candidate);
    ensure("all distinct storage codes mismatch", stats.mComparable && !stats.mMatch && stats.mMismatchCount == 6);
    ensure("first mismatch remains the first one-code difference",
           stats.mFirstMismatchPlane == 1 && stats.mFirstMismatchPixel == 0 && stats.mFirstMismatchChannel == 0 &&
               stats.mFirstReference == 0.f && stats.mFirstCandidate == materialUnorm8(1) &&
               stats.mFirstTolerance == MATERIAL_RGBA8_TOLERANCE);

    candidate.mGBuffer0RGBA8[0] = std::numeric_limits<float>::quiet_NaN();
    stats                       = compareMaterialArtifacts(reference, candidate);
    ensure("non-finite candidates fail comparison preflight", !stats.mComparable && !stats.mMatch && !stats.mError.empty());
}

} // namespace tut
