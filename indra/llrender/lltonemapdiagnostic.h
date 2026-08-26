/**
 * @file lltonemapdiagnostic.h
 * @brief Shared fixture and artifact format for cross-process tonemap checks.
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

#ifndef LL_LLTONEMAPDIAGNOSTIC_H
#define LL_LLTONEMAPDIAGNOSTIC_H

#include "lltonemapcontract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace LLRenderContract
{

inline constexpr std::uint32_t TONEMAP_DIAGNOSTIC_WIDTH = 8;
inline constexpr std::uint32_t TONEMAP_DIAGNOSTIC_HEIGHT = 8;
inline constexpr std::uint32_t TONEMAP_DIAGNOSTIC_CHANNELS = 4;
inline constexpr std::size_t TONEMAP_DIAGNOSTIC_PIXEL_COUNT =
    static_cast<std::size_t>(TONEMAP_DIAGNOSTIC_WIDTH) * TONEMAP_DIAGNOSTIC_HEIGHT;
inline constexpr std::size_t TONEMAP_DIAGNOSTIC_COMPONENT_COUNT =
    TONEMAP_DIAGNOSTIC_PIXEL_COUNT * TONEMAP_DIAGNOSTIC_CHANNELS;
inline constexpr std::size_t TONEMAP_DIAGNOSTIC_CASE_COUNT = 24;

inline constexpr std::uint32_t TONEMAP_DIAGNOSTIC_FIXTURE_VERSION = 1;
inline constexpr std::uint32_t TONEMAP_ARTIFACT_SCHEMA_VERSION = 1;

inline constexpr float TONEMAP_RGBA8_TOLERANCE = 1.f / 255.f;
inline constexpr float TONEMAP_RGBA16F_TOLERANCE = 2.f / 1024.f;

// These conversions use IEEE 754 round-to-nearest-even semantics.
std::uint16_t floatToHalfBits(float value) noexcept;
float halfBitsToFloat(std::uint16_t bits) noexcept;

struct TonemapFixture
{
    Extent2D mExtent{ TONEMAP_DIAGNOSTIC_WIDTH, TONEMAP_DIAGNOSTIC_HEIGHT };
    RowOrigin mRowOrigin = RowOrigin::BottomLeft;
    // Native uint16_t words containing IEEE binary16 bits, in bottom-left
    // row-major RGBA order.
    std::array<std::uint16_t, TONEMAP_DIAGNOSTIC_COMPONENT_COUNT> mSceneRGBA16F{};
    std::uint16_t mExposureR16F = 0;

    // Three Float3 vertices with a fourth padding word for the contract's
    // 16-byte vertex stride.
    std::array<float, 12> mScreenTriangle{};
};

static_assert(sizeof(std::array<float, 12>) == 48, "the diagnostic screen triangle must occupy 48 bytes");

struct TonemapCaseKey
{
    std::uint32_t mIndex = 0;
    PixelFormat mDestinationFormat = PixelFormat::RGBA8Unorm;
    TonemapVariant mVariant = TonemapVariant::Deferred;
    std::uint32_t mTonemapType = 0;

    friend constexpr bool operator==(const TonemapCaseKey&, const TonemapCaseKey&) = default;
};

struct TonemapCase
{
    TonemapCaseKey mKey;
    TonemapInputs mInputs;
    FrameSnapshot mFrame;
};

using TonemapCases = std::array<TonemapCase, TONEMAP_DIAGNOSTIC_CASE_COUNT>;

TonemapFixture makeTonemapFixture();
std::uint64_t tonemapFixtureFingerprint();
TonemapCases makeTonemapCases();

struct TonemapArtifactCase
{
    TonemapCaseKey mKey;
    // Bottom-left row-major RGBA values, one float per component.
    std::vector<float> mPixels;

    friend bool operator==(const TonemapArtifactCase&, const TonemapArtifactCase&) = default;
};

struct TonemapArtifact
{
    std::vector<TonemapArtifactCase> mCases;

    friend bool operator==(const TonemapArtifact&, const TonemapArtifact&) = default;
};

// Returns the canonical case metadata with empty pixel arrays.
TonemapArtifact makeTonemapArtifact();

bool validateTonemapArtifact(const TonemapArtifact& artifact, std::string* error = nullptr);
// The wire format uses big-endian integer and IEEE float words. Its fixed
// schema records the fixture identity and all canonical case metadata.
bool encodeTonemapArtifact(const TonemapArtifact& artifact,
                           std::vector<std::uint8_t>& encoded,
                           std::string* error = nullptr);
bool decodeTonemapArtifact(const std::vector<std::uint8_t>& encoded,
                           TonemapArtifact& artifact,
                           std::string* error = nullptr);

// The destination must not exist. The writer creates a unique sibling file
// exclusively, then publishes the complete bytes atomically without replacement.
bool writeTonemapArtifact(const std::filesystem::path& destination,
                          const TonemapArtifact& artifact,
                          std::string* error = nullptr);
bool readTonemapArtifact(const std::filesystem::path& source,
                         TonemapArtifact& artifact,
                         std::string* error = nullptr);

struct TonemapComparisonStats
{
    bool mComparable = false;
    bool mMatch = false;
    std::size_t mComparedCases = 0;
    std::size_t mComparedComponents = 0;
    std::size_t mMismatchCount = 0;
    double mMaximumAbsoluteError = 0.0;

    std::uint32_t mFirstMismatchCase = 0;
    std::size_t mFirstMismatchPixel = 0;
    std::uint32_t mFirstMismatchChannel = 0;
    float mFirstReference = 0.f;
    float mFirstCandidate = 0.f;
    float mFirstTolerance = 0.f;

    std::string mError;
};

float tonemapComparisonTolerance(PixelFormat format) noexcept;
TonemapComparisonStats compareTonemapArtifacts(const TonemapArtifact& reference,
                                                const TonemapArtifact& candidate);

}

#endif
