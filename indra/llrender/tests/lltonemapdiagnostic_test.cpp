/**
 * @file lltonemapdiagnostic_test.cpp
 * @brief Tests for the cross-process tonemap diagnostic boundary.
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

#include "lltonemapdiagnostic.h"
#include "lltut.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace
{
using namespace LLRenderContract;

TonemapArtifact completeArtifact(float value = 0.f)
{
    TonemapArtifact artifact = makeTonemapArtifact();
    for (TonemapArtifactCase& artifact_case : artifact.mCases)
    {
        artifact_case.mPixels.assign(TONEMAP_DIAGNOSTIC_COMPONENT_COUNT, value);
    }
    return artifact;
}

std::filesystem::path temporaryArtifactPath()
{
    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("lltonemapdiagnostic-" + std::to_string(suffix) + ".bin");
}

}

namespace tut
{

struct tonemap_diagnostic_test
{
};

using tonemap_diagnostic_test_group = test_group<tonemap_diagnostic_test>;
using tonemap_diagnostic_test_object = tonemap_diagnostic_test_group::object;
tonemap_diagnostic_test_group tonemap_diagnostic_tests("tonemap diagnostic");

template<>
template<>
void tonemap_diagnostic_test_object::test<1>()
{
    const TonemapFixture fixture = makeTonemapFixture();
    ensure("fixture is 8 by 8", fixture.mExtent.mWidth == 8 && fixture.mExtent.mHeight == 8);
    ensure("fixture rows start at the bottom", fixture.mRowOrigin == RowOrigin::BottomLeft);
    ensure("fixture has one RGBA half value per source component",
           fixture.mSceneRGBA16F.size() == TONEMAP_DIAGNOSTIC_COMPONENT_COUNT);
    ensure("screen triangle is exactly 48 bytes", sizeof(fixture.mScreenTriangle) == 48);
    ensure("screen triangle includes deterministic stride padding",
           fixture.mScreenTriangle == std::array<float, 12>{ -1.f, 1.f, 0.f, 0.f,
                                                            -1.f, -3.f, 0.f, 0.f,
                                                            3.f, 1.f, 0.f, 0.f });

    ensure("known half values use IEEE encodings", floatToHalfBits(1.f) == 0x3c00U &&
                                                     floatToHalfBits(-2.f) == 0xc000U &&
                                                     halfBitsToFloat(0x0001U) == std::ldexp(1.f, -24));
    bool finite_half_values_round_trip = true;
    for (std::uint32_t bits = 0; bits <= std::numeric_limits<std::uint16_t>::max(); ++bits)
    {
        if ((bits & 0x7c00U) != 0x7c00U &&
            floatToHalfBits(halfBitsToFloat(static_cast<std::uint16_t>(bits))) != bits)
        {
            finite_half_values_round_trip = false;
            break;
        }
    }
    ensure("all finite half encodings round trip", finite_half_values_round_trip);
    ensure("half conversion rounds ties to even",
           floatToHalfBits(1.f + std::ldexp(1.f, -11)) == 0x3c00U &&
           floatToHalfBits(1.f + 3.f * std::ldexp(1.f, -11)) == 0x3c02U);
    ensure("source starts with the Stage 10 sequence",
           fixture.mSceneRGBA16F[0] == floatToHalfBits(0.f) &&
           fixture.mSceneRGBA16F[1] == floatToHalfBits(0.04f * 0.75f) &&
           fixture.mSceneRGBA16F[2] == floatToHalfBits(0.08f * 1.25f) &&
           fixture.mSceneRGBA16F[3] == floatToHalfBits(0.f));
    ensure("exposure is the Stage 10 value", fixture.mExposureR16F == 0x3acdU);
    ensure_equals("fixture fingerprint changes only with an explicit fixture revision",
                  tonemapFixtureFingerprint(), std::uint64_t{ 0xb5b53dcc766cd299ULL });

    const TonemapCases cases = makeTonemapCases();
    ensure("fixture has the canonical case count", cases.size() == 24);
    for (std::size_t offset = 0; offset < cases.size(); ++offset)
    {
        const TonemapCase& diagnostic_case = cases[offset];
        ensure("case indices and frame numbers are one based",
               diagnostic_case.mKey.mIndex == offset + 1 && diagnostic_case.mInputs.mFrame == offset + 1 &&
               diagnostic_case.mFrame.mFrame == offset + 1);
        auto decoded = decodeTonemapFrame(diagnostic_case.mFrame);
        ensure("each case owns a canonical frame", decoded.has_value());
        ensure("case metadata agrees with its frame",
               decoded->mDestinationFormat == diagnostic_case.mKey.mDestinationFormat &&
               decoded->mVariant == diagnostic_case.mKey.mVariant &&
               decoded->mParameters.mTonemapType == diagnostic_case.mKey.mTonemapType);
    }
    ensure("formats are the outer case dimension",
           cases.front().mKey.mDestinationFormat == PixelFormat::RGBA8Unorm &&
           cases[11].mKey.mDestinationFormat == PixelFormat::RGBA8Unorm &&
           cases[12].mKey.mDestinationFormat == PixelFormat::RGBA16Float &&
           cases.back().mKey.mDestinationFormat == PixelFormat::RGBA16Float);
}

template<>
template<>
void tonemap_diagnostic_test_object::test<2>()
{
    TonemapArtifact artifact = makeTonemapArtifact();
    for (TonemapArtifactCase& artifact_case : artifact.mCases)
    {
        artifact_case.mPixels.reserve(TONEMAP_DIAGNOSTIC_COMPONENT_COUNT);
        for (std::size_t component = 0; component < TONEMAP_DIAGNOSTIC_COMPONENT_COUNT; ++component)
        {
            if (artifact_case.mKey.mDestinationFormat == PixelFormat::RGBA8Unorm)
            {
                const std::uint32_t code = static_cast<std::uint32_t>((artifact_case.mKey.mIndex + component) % 256);
                artifact_case.mPixels.push_back(static_cast<float>(code) / 255.f);
            }
            else
            {
                const int numerator = static_cast<int>(component % 33) - 16;
                artifact_case.mPixels.push_back(halfBitsToFloat(floatToHalfBits(static_cast<float>(numerator) / 16.f)));
            }
        }
    }

    std::string error;
    std::vector<std::uint8_t> first_encoding;
    std::vector<std::uint8_t> second_encoding;
    ensure("complete artifact validates", validateTonemapArtifact(artifact, &error));
    ensure("complete artifact encodes", encodeTonemapArtifact(artifact, first_encoding, &error));
    ensure("the schema has a fixed byte count", first_encoding.size() == 25676);
    ensure("encoding is deterministic", encodeTonemapArtifact(artifact, second_encoding, &error) &&
                                             first_encoding == second_encoding);

    TonemapArtifact decoded;
    ensure("canonical artifact decodes", decodeTonemapArtifact(first_encoding, decoded, &error));
    ensure("artifact round trip preserves metadata and pixels", decoded == artifact);

    const std::filesystem::path path = temporaryArtifactPath();
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove_all(temporary, cleanup_error);
    ensure("fixed temporary sibling can already exist", std::filesystem::create_directory(temporary));
    ensure("artifact publishes through a unique sibling temporary file", writeTonemapArtifact(path, artifact, &error));
    ensure("writer does not touch a predictable temporary sibling",
           std::filesystem::exists(path) && std::filesystem::is_directory(temporary));
    bool unique_temporary_remains = false;
    const std::string temporary_prefix = path.filename().string() + ".tmp.";
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path.parent_path()))
    {
        unique_temporary_remains = unique_temporary_remains ||
                                   entry.path().filename().string().starts_with(temporary_prefix);
    }
    ensure("published artifact has no unique temporary sibling", !unique_temporary_remains);
    TonemapArtifact from_file;
    ensure("published artifact reads back", readTonemapArtifact(path, from_file, &error));
    ensure("file round trip preserves the artifact", from_file == artifact);
    ensure("atomic writer refuses to replace an existing artifact", !writeTonemapArtifact(path, artifact, &error));
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove_all(temporary, cleanup_error);
}

template<>
template<>
void tonemap_diagnostic_test_object::test<3>()
{
    TonemapArtifact artifact = completeArtifact();
    std::string error;
    std::vector<std::uint8_t> encoded;
    ensure("baseline artifact encodes", encodeTonemapArtifact(artifact, encoded, &error));
    const std::vector<std::uint8_t> baseline = encoded;

    TonemapArtifact invalid = artifact;
    invalid.mCases.pop_back();
    ensure("missing cases are rejected", !validateTonemapArtifact(invalid, &error));

    invalid = artifact;
    invalid.mCases[0].mKey.mIndex = 2;
    ensure("out of order metadata is rejected", !encodeTonemapArtifact(invalid, encoded, &error));

    invalid = artifact;
    invalid.mCases[0].mPixels[0] = std::numeric_limits<float>::infinity();
    ensure("non-finite pixels are rejected before encoding", !validateTonemapArtifact(invalid, &error));

    invalid = artifact;
    invalid.mCases[0].mPixels[0] = 0.5f;
    ensure("values not representable by RGBA8 are rejected", !validateTonemapArtifact(invalid, &error));

    TonemapArtifact decoded;
    std::vector<std::uint8_t> corrupt = baseline;
    corrupt[0] ^= 0xffU;
    ensure("bad magic is rejected", !decodeTonemapArtifact(corrupt, decoded, &error));

    corrupt = baseline;
    corrupt[11] = 2;
    ensure("unknown schema version is rejected", !decodeTonemapArtifact(corrupt, decoded, &error));

    corrupt = baseline;
    corrupt[47] = 2;
    ensure("non-canonical case index is rejected", !decodeTonemapArtifact(corrupt, decoded, &error));

    corrupt = baseline;
    corrupt[88] = 0x7fU;
    corrupt[89] = 0x80U;
    corrupt[90] = 0;
    corrupt[91] = 0;
    ensure("non-finite wire pixels are rejected", !decodeTonemapArtifact(corrupt, decoded, &error));

    corrupt = baseline;
    corrupt.push_back(0);
    ensure("trailing bytes are rejected", !decodeTonemapArtifact(corrupt, decoded, &error));
}

template<>
template<>
void tonemap_diagnostic_test_object::test<4>()
{
    const TonemapArtifact reference = completeArtifact();
    TonemapArtifact candidate = reference;
    candidate.mCases[0].mPixels[0] = 1.f / 255.f;
    candidate.mCases[12].mPixels[0] = 2.f / 1024.f;

    TonemapComparisonStats stats = compareTonemapArtifacts(reference, candidate);
    ensure("one RGBA8 code and two half steps are within tolerance",
           stats.mComparable && stats.mMatch && stats.mMismatchCount == 0);
    ensure("comparison reports its full scope",
           stats.mComparedCases == TONEMAP_DIAGNOSTIC_CASE_COUNT &&
           stats.mComparedComponents == TONEMAP_DIAGNOSTIC_CASE_COUNT * TONEMAP_DIAGNOSTIC_COMPONENT_COUNT);

    candidate.mCases[0].mPixels[1] = 2.f / 255.f;
    candidate.mCases[12].mPixels[1] = 3.f / 1024.f;
    stats = compareTonemapArtifacts(reference, candidate);
    ensure("values beyond each format tolerance mismatch",
           stats.mComparable && !stats.mMatch && stats.mMismatchCount == 2);
    ensure("first mismatch identifies case, pixel, and channel",
           stats.mFirstMismatchCase == 1 && stats.mFirstMismatchPixel == 0 && stats.mFirstMismatchChannel == 1 &&
           stats.mFirstReference == 0.f && stats.mFirstCandidate == 2.f / 255.f &&
           stats.mFirstTolerance == TONEMAP_RGBA8_TOLERANCE);
    ensure("comparison records the maximum absolute error",
           stats.mMaximumAbsoluteError == static_cast<double>(2.f / 255.f));

    candidate.mCases[0].mPixels[0] = std::numeric_limits<float>::quiet_NaN();
    stats = compareTonemapArtifacts(reference, candidate);
    ensure("non-finite candidates fail preflight", !stats.mComparable && !stats.mMatch && !stats.mError.empty());
}

}
