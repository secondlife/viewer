/**
 * @file lltextureuploaddiagnostic_test.cpp
 * @brief Tests for the streamed texture upload diagnostic boundary.
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

#include "lltextureuploaddiagnostic.h"
#include "lltut.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
using namespace LLRenderContract;

TextureUploadArtifact completeArtifact()
{
    TextureUploadArtifact artifact = makeTextureUploadArtifact();
    artifact.mPriorRevision               = TEXTURE_UPLOAD_PRIOR_REVISION;
    artifact.mRevision                    = TEXTURE_UPLOAD_REVISION;
    artifact.mCompletionCount             = 1;
    artifact.mCompletedDestination        = ImageHandle{ 11, 2 };
    artifact.mCompletedRevision           = TEXTURE_UPLOAD_REVISION;
    artifact.mCompletedFrame              = TEXTURE_UPLOAD_DIAGNOSTIC_FRAME;
    artifact.mRetirementCount             = 1;
    artifact.mRetiredResource             = ImageHandle{ 11, 1 };
    artifact.mRetirementFrame             = TEXTURE_UPLOAD_DIAGNOSTIC_FRAME;
    artifact.mOldResolvableBefore         = true;
    artifact.mOldResolvableAfter          = false;
    artifact.mReplacementResolvableAfter = true;
    for (std::size_t mip = 0; mip < artifact.mMipRGBA8.size(); ++mip)
    {
        artifact.mMipRGBA8[mip].reserve(TEXTURE_UPLOAD_MIP_BYTE_SIZES[mip]);
        for (std::size_t byte = 0; byte < TEXTURE_UPLOAD_MIP_BYTE_SIZES[mip]; ++byte)
        {
            artifact.mMipRGBA8[mip].push_back(
                static_cast<std::uint8_t>(1 + (mip * 83 + byte * 17 + 31) % 251));
        }
    }
    artifact.mSampledRGBA8.reserve(TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT);
    for (std::size_t byte = 0; byte < TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT; ++byte)
    {
        artifact.mSampledRGBA8.push_back(static_cast<std::uint8_t>(1 + (byte * 43 + 109) % 251));
    }
    return artifact;
}

void refreshArtifactChecksum(std::vector<std::uint8_t>& bytes)
{
    constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr std::uint64_t FNV_PRIME        = 1099511628211ULL;
    const std::size_t       checksum_offset  = bytes.size() - sizeof(std::uint64_t);
    std::uint64_t           checksum         = FNV_OFFSET_BASIS;
    for (std::size_t index = 0; index < checksum_offset; ++index)
    {
        checksum ^= bytes[index];
        checksum *= FNV_PRIME;
    }
    for (std::size_t byte = 0; byte < sizeof(checksum); ++byte)
    {
        bytes[checksum_offset + byte] = static_cast<std::uint8_t>(checksum >> ((7 - byte) * 8));
    }
}

std::filesystem::path temporaryArtifactPath()
{
    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("lltextureuploaddiagnostic-" + std::to_string(suffix) + ".bin");
}

std::uint8_t sourceComponent(const TextureUploadFixture& fixture, std::size_t top_row, std::size_t x,
                             std::size_t channel)
{
    return fixture.mSourceRGBA8[top_row * TEXTURE_UPLOAD_ROW_PITCH + x * TEXTURE_UPLOAD_CHANNELS + channel];
}

bool sameExtent(Extent2D extent, std::uint32_t width, std::uint32_t height)
{
    return extent.mWidth == width && extent.mHeight == height;
}

} // namespace

namespace tut
{

struct texture_upload_diagnostic_test
{
};

using texture_upload_diagnostic_test_group  = test_group<texture_upload_diagnostic_test>;
using texture_upload_diagnostic_test_object = texture_upload_diagnostic_test_group::object;
texture_upload_diagnostic_test_group texture_upload_diagnostic_tests("texture upload diagnostic");

template<>
template<>
void texture_upload_diagnostic_test_object::test<1>()
{
    const TextureUploadFixture fixture = makeTextureUploadFixture();
    ensure("resident, logical, and sample extents are asymmetric",
           sameExtent(fixture.mResidentExtent, 8, 4) && sameExtent(fixture.mLogicalExtent, 32, 16) &&
               sameExtent(fixture.mOutputExtent, 4, 2));
    ensure("source has padded top-left rows",
           fixture.mSourceRowOrigin == RowOrigin::TopLeft && fixture.mSourceRowPitch == 36 &&
               fixture.mSourceRGBA8.size() == 144);
    ensure("source corners and channels are observably distinct",
           sourceComponent(fixture, 0, 0, 0) != sourceComponent(fixture, 0, 7, 0) &&
               sourceComponent(fixture, 0, 0, 0) != sourceComponent(fixture, 3, 0, 0) &&
               sourceComponent(fixture, 0, 0, 0) != sourceComponent(fixture, 0, 0, 1));

    bool poison_is_exact = true;
    bool averages_are_integral = true;
    for (std::size_t top_row = 0; top_row < TEXTURE_UPLOAD_RESIDENT_HEIGHT; ++top_row)
    {
        const std::size_t row_start = top_row * TEXTURE_UPLOAD_ROW_PITCH;
        for (std::size_t padding = 0; padding < 4; ++padding)
        {
            poison_is_exact = poison_is_exact &&
                              fixture.mSourceRGBA8[row_start + 32 + padding] == 0xf0U + top_row * 4 + padding;
        }
    }
    for (std::size_t top_row = 0; top_row < TEXTURE_UPLOAD_RESIDENT_HEIGHT; top_row += 2)
    {
        for (std::size_t x = 0; x < TEXTURE_UPLOAD_RESIDENT_WIDTH; x += 2)
        {
            for (std::size_t channel = 0; channel < TEXTURE_UPLOAD_CHANNELS; ++channel)
            {
                const unsigned sum = sourceComponent(fixture, top_row, x, channel) +
                                     sourceComponent(fixture, top_row, x + 1, channel) +
                                     sourceComponent(fixture, top_row + 1, x, channel) +
                                     sourceComponent(fixture, top_row + 1, x + 1, channel);
                averages_are_integral = averages_are_integral && sum % 4 == 0;
            }
        }
    }
    ensure("every row ends in four distinct poison bytes", poison_is_exact);
    ensure("first generated mip uses exact integer 2 by 2 averages", averages_are_integral);
    ensure("screen triangle has the exact 16-byte vertex stride",
           fixture.mScreenTriangle == std::array<float, 12>{ -1.f, 1.f, 0.f, 0.f,
                                                             -1.f, -3.f, 0.f, 0.f,
                                                             3.f, 1.f, 0.f, 0.f });
    ensure("old, replacement sentinel, and output sentinel storage is complete",
           fixture.mOldMipRGBA8.size() == 168 && fixture.mReplacementSentinelMipRGBA8.size() == 168 &&
               fixture.mOutputSentinelRGBA8.size() == 32);
    ensure("initial generations and outputs are observably distinct",
           fixture.mOldMipRGBA8[0] != fixture.mReplacementSentinelMipRGBA8[0] &&
               fixture.mReplacementSentinelMipRGBA8[0] != fixture.mSourceRGBA8[0] &&
               fixture.mOutputSentinelRGBA8[0] != fixture.mReplacementSentinelMipRGBA8[0]);

    ensure_equals("fixture drift requires an explicit fixture revision", textureUploadFixtureFingerprint(),
                  std::uint64_t{ 0x7f76518103e7019eULL });

    TextureUploadCase diagnostic_case = makeTextureUploadCase();
    auto              decoded         = decodeStreamingUploadFrame(diagnostic_case.mFrame);
    ensure("fixture produces one canonical owned frame", decoded.has_value());
    ensure("case freezes the consecutive revisions and discard geometry",
           diagnostic_case.mPriorRevision == 22 && decoded->mRevision == 23 && decoded->mResidentDiscard == 2 &&
               sameExtent(decoded->mLogicalExtent, 32, 16));
    ensure("case owns the entire padded source",
           decoded->mPixels == std::vector<std::uint8_t>(fixture.mSourceRGBA8.begin(), fixture.mSourceRGBA8.end()));
    diagnostic_case.mInputs.mPixels[0] ^= 0xffU;
    auto decoded_again = decodeStreamingUploadFrame(diagnostic_case.mFrame);
    ensure("mutating case inputs cannot mutate frame upload storage",
           decoded_again && decoded_again->mPixels[0] == fixture.mSourceRGBA8[0]);
}

template<>
template<>
void texture_upload_diagnostic_test_object::test<2>()
{
    const TextureUploadArtifact unobserved = makeTextureUploadArtifact();
    ensure("new artifacts contain no implicit upload evidence",
           unobserved.mPriorRevision == 0 && unobserved.mRevision == 0 && unobserved.mCompletionCount == 0 &&
               unobserved.mCompletedDestination == ImageHandle{} && unobserved.mCompletedRevision == 0 &&
               unobserved.mCompletedFrame == 0 && unobserved.mRetirementCount == 0 &&
               unobserved.mRetiredResource == ImageHandle{} && unobserved.mRetirementFrame == 0 &&
               !unobserved.mOldResolvableBefore && !unobserved.mOldResolvableAfter &&
               !unobserved.mReplacementResolvableAfter);
    const TextureUploadArtifact artifact = completeArtifact();
    std::string                 error;
    std::vector<std::uint8_t>   first_encoding;
    std::vector<std::uint8_t>   second_encoding;
    ensure("complete texture upload artifact validates", validateTextureUploadArtifact(artifact, &error));
    ensure("complete texture upload artifact encodes", encodeTextureUploadArtifact(artifact, first_encoding, &error));
    ensure("artifact schema has a fixed byte count", first_encoding.size() == TEXTURE_UPLOAD_ARTIFACT_BYTE_SIZE);
    ensure("artifact encoding is deterministic",
           encodeTextureUploadArtifact(artifact, second_encoding, &error) && first_encoding == second_encoding);

    TextureUploadArtifact decoded;
    ensure("canonical artifact decodes", decodeTextureUploadArtifact(first_encoding, decoded, &error));
    ensure("artifact round trip preserves pixels and lifecycle evidence", decoded == artifact);

    const std::filesystem::path path                  = temporaryArtifactPath();
    std::filesystem::path       predictable_temporary = path;
    predictable_temporary += ".tmp";
    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove_all(predictable_temporary, cleanup_error);
    ensure("predictable temporary sibling can already exist", std::filesystem::create_directory(predictable_temporary));
    ensure("writer publishes through an unpredictable sibling", writeTextureUploadArtifact(path, artifact, &error));
    ensure("writer leaves the predictable sibling untouched",
           std::filesystem::exists(path) && std::filesystem::is_directory(predictable_temporary));
    bool              unique_temporary_remains = false;
    const std::string temporary_prefix         = path.filename().string() + ".tmp.";
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path.parent_path()))
    {
        unique_temporary_remains = unique_temporary_remains || entry.path().filename().string().starts_with(temporary_prefix);
    }
    ensure("published artifact has no unique temporary sibling", !unique_temporary_remains);

    TextureUploadArtifact from_file;
    ensure("published artifact reads back", readTextureUploadArtifact(path, from_file, &error));
    ensure("file round trip preserves exact bytes and evidence", from_file == artifact);
    TextureUploadArtifact replacement = artifact;
    replacement.mSampledRGBA8[0] ^= 1U;
    ensure("atomic writer refuses to replace an existing artifact", !writeTextureUploadArtifact(path, replacement, &error));
    ensure("failed replacement leaves the published artifact unchanged",
           readTextureUploadArtifact(path, from_file, &error) && from_file == artifact);
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove_all(predictable_temporary, cleanup_error);
}

template<>
template<>
void texture_upload_diagnostic_test_object::test<3>()
{
    const TextureUploadArtifact artifact = completeArtifact();
    std::string                 error;
    std::vector<std::uint8_t>   encoded;
    ensure("baseline texture upload artifact encodes", encodeTextureUploadArtifact(artifact, encoded, &error));
    const std::vector<std::uint8_t> baseline = encoded;

    TextureUploadArtifact invalid = artifact;
    invalid.mMipRGBA8[1].pop_back();
    ensure("short generated mip is rejected", !validateTextureUploadArtifact(invalid, &error));

    invalid = artifact;
    invalid.mSampledRGBA8.pop_back();
    ensure("short sampled output is rejected", !validateTextureUploadArtifact(invalid, &error));

    invalid                   = artifact;
    invalid.mCompletionCount = 2;
    ensure("duplicate completion evidence is rejected", !validateTextureUploadArtifact(invalid, &error));

    invalid                    = artifact;
    invalid.mCompletedRevision = TEXTURE_UPLOAD_PRIOR_REVISION;
    ensure("completion for a stale revision is rejected", !validateTextureUploadArtifact(invalid, &error));

    invalid                   = artifact;
    invalid.mRetirementCount = 0;
    ensure("missing retirement evidence is rejected", !validateTextureUploadArtifact(invalid, &error));

    invalid                              = artifact;
    invalid.mRetiredResource.mGeneration = 2;
    ensure("retiring the replacement generation is rejected", !validateTextureUploadArtifact(invalid, &error));

    invalid                    = artifact;
    ++invalid.mRetirementFrame;
    ensure("late retirement evidence is rejected", !validateTextureUploadArtifact(invalid, &error));

    invalid                              = artifact;
    invalid.mOldResolvableAfter         = true;
    invalid.mReplacementResolvableAfter = false;
    ensure("non-canonical post-state evidence is rejected", !validateTextureUploadArtifact(invalid, &error));

    TextureUploadArtifact    decoded;
    std::vector<std::uint8_t> corrupt = baseline;
    corrupt[0] ^= 0xffU;
    refreshArtifactChecksum(corrupt);
    ensure("bad artifact magic is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt     = baseline;
    corrupt[11] = 3;
    refreshArtifactChecksum(corrupt);
    ensure("unknown artifact schema is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt     = baseline;
    corrupt[23] ^= 1U;
    refreshArtifactChecksum(corrupt);
    ensure("wrong fixture fingerprint is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt     = baseline;
    corrupt[47] = TEXTURE_UPLOAD_PRIOR_REVISION;
    refreshArtifactChecksum(corrupt);
    ensure("stale wire revision is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt      = baseline;
    corrupt[119] = 2;
    refreshArtifactChecksum(corrupt);
    ensure("wrong source row origin is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt      = baseline;
    corrupt[123] = 1;
    refreshArtifactChecksum(corrupt);
    ensure("wrong artifact row origin is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt      = baseline;
    corrupt[127] = 32;
    refreshArtifactChecksum(corrupt);
    ensure("tight wire row pitch is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt      = baseline;
    corrupt[131] = 0;
    refreshArtifactChecksum(corrupt);
    ensure("disabled wire mip generation is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt      = baseline;
    corrupt[167] = 2;
    refreshArtifactChecksum(corrupt);
    ensure("duplicate wire completion evidence is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt      = baseline;
    corrupt[215] = 0;
    refreshArtifactChecksum(corrupt);
    ensure("missing old-before state is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt      = baseline;
    corrupt[231] = 7;
    refreshArtifactChecksum(corrupt);
    ensure("wrong base mip width is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt      = baseline;
    corrupt[239] = 127;
    refreshArtifactChecksum(corrupt);
    ensure("wrong base mip byte count is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    const std::size_t checksum_offset = baseline.size() - sizeof(std::uint64_t);
    const std::size_t sample_offset   = checksum_offset - TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT;
    corrupt = baseline;
    corrupt[sample_offset - 1] ^= 1U;
    ensure("mip payload corruption is rejected by its checksum", !decodeTextureUploadArtifact(corrupt, decoded, &error));
    ensure_equals("mip corruption reports the integrity failure", error, std::string("artifact checksum is invalid"));

    corrupt = baseline;
    corrupt[sample_offset] ^= 1U;
    ensure("sample payload corruption is rejected by its checksum", !decodeTextureUploadArtifact(corrupt, decoded, &error));
    ensure_equals("sample corruption reports the integrity failure", error, std::string("artifact checksum is invalid"));

    corrupt = baseline;
    corrupt[checksum_offset] ^= 1U;
    ensure("checksum corruption is rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));
    ensure_equals("checksum corruption reports the integrity failure", error, std::string("artifact checksum is invalid"));

    corrupt = baseline;
    corrupt.pop_back();
    ensure("truncated artifacts are rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));

    corrupt = baseline;
    corrupt.push_back(0);
    ensure("trailing artifact bytes are rejected", !decodeTextureUploadArtifact(corrupt, decoded, &error));
}

template<>
template<>
void texture_upload_diagnostic_test_object::test<4>()
{
    const TextureUploadArtifact reference = completeArtifact();
    TextureUploadArtifact       candidate = reference;

    TextureUploadComparisonStats stats = compareTextureUploadArtifacts(reference, candidate);
    ensure("identical artifacts match exactly", stats.mComparable && stats.mMatch && stats.mMismatchCount == 0);
    ensure("comparison reports every mip and sample byte",
           stats.mComparedMipBytes == 168 && stats.mComparedSampleBytes == 32);

    candidate.mMipRGBA8[1][3] ^= 1U;
    candidate.mSampledRGBA8[5] ^= 1U;
    stats = compareTextureUploadArtifacts(reference, candidate);
    ensure("every distinct byte mismatches at zero tolerance",
           stats.mComparable && !stats.mMatch && stats.mMismatchCount == 2);
    ensure("first mismatch identifies the mip, byte, and exact codes",
           stats.mFirstMismatchPlane == 2 && stats.mFirstMismatchByte == 3 &&
               stats.mFirstReference == reference.mMipRGBA8[1][3] &&
               stats.mFirstCandidate == candidate.mMipRGBA8[1][3]);

    candidate.mMipRGBA8[0].pop_back();
    stats = compareTextureUploadArtifacts(reference, candidate);
    ensure("invalid candidates fail comparison preflight", !stats.mComparable && !stats.mMatch && !stats.mError.empty());
}

} // namespace tut
