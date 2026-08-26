/**
 * @file lltonemapdiagnostic.cpp
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

#include "lltonemapdiagnostic.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>
#include <utility>

namespace LLRenderContract
{
namespace
{

static_assert(sizeof(float) == sizeof(std::uint32_t), "tonemap artifacts require 32-bit floats");
static_assert(std::numeric_limits<float>::is_iec559, "tonemap artifacts require IEEE 754 floats");

constexpr std::array<std::uint8_t, 8> ARTIFACT_MAGIC{ 'L', 'L', 'T', 'O', 'N', 'E', 'M', 'P' };
constexpr std::uint32_t BOTTOM_LEFT_WIRE_VALUE = 1;
constexpr std::uint32_t RGBA8_WIRE_VALUE = 1;
constexpr std::uint32_t RGBA16F_WIRE_VALUE = 2;
constexpr std::size_t ARTIFACT_HEADER_SIZE = 44;
constexpr std::size_t ARTIFACT_CASE_METADATA_SIZE = 44;
constexpr std::size_t ARTIFACT_CASE_SIZE =
    ARTIFACT_CASE_METADATA_SIZE + TONEMAP_DIAGNOSTIC_COMPONENT_COUNT * sizeof(float);
constexpr std::size_t ARTIFACT_SIZE =
    ARTIFACT_HEADER_SIZE + TONEMAP_DIAGNOSTIC_CASE_COUNT * ARTIFACT_CASE_SIZE;

constexpr std::array<TonemapVariant, 6> TONEMAP_VARIANTS{
    TonemapVariant::Deferred,
    TonemapVariant::NoPost,
    TonemapVariant::GammaCorrect,
    TonemapVariant::NoPostGammaCorrect,
    TonemapVariant::LegacyGammaCorrect,
    TonemapVariant::NoPostLegacyGammaCorrect
};

constexpr std::array<PixelFormat, 2> TONEMAP_FORMATS{
    PixelFormat::RGBA8Unorm,
    PixelFormat::RGBA16Float
};

constexpr TonemapParameters TONEMAP_PARAMETERS{ 1.25f, 0.65f, 0, 1.8f };

std::uint32_t floatBits(float value) noexcept
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

float bitsToFloat(std::uint32_t bits) noexcept
{
    float value = 0.f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void clearError(std::string* error)
{
    if (error)
    {
        error->clear();
    }
}

bool fail(std::string* error, std::string message)
{
    if (error)
    {
        *error = std::move(message);
    }
    return false;
}

std::string caseField(std::uint32_t case_index, const char* field)
{
    std::ostringstream out;
    out << "case " << case_index << ' ' << field;
    return out.str();
}

TonemapInputs canonicalInputs(std::size_t offset)
{
    const std::size_t cases_per_format = TONEMAP_VARIANTS.size() * 2;
    const std::size_t format_index = offset / cases_per_format;
    const std::size_t within_format = offset % cases_per_format;
    const std::size_t variant_index = within_format / 2;
    const std::uint32_t tonemap_type = static_cast<std::uint32_t>(within_format % 2);

    TonemapInputs inputs;
    inputs.mFrame = offset + 1;
    inputs.mSourceExtent = { TONEMAP_DIAGNOSTIC_WIDTH, TONEMAP_DIAGNOSTIC_HEIGHT };
    inputs.mDestinationExtent = inputs.mSourceExtent;
    inputs.mDestinationFormat = TONEMAP_FORMATS[format_index];
    inputs.mVariant = TONEMAP_VARIANTS[variant_index];
    inputs.mParameters = TONEMAP_PARAMETERS;
    inputs.mParameters.mTonemapType = tonemap_type;
    return inputs;
}

TonemapCaseKey canonicalKey(std::size_t offset)
{
    const TonemapInputs inputs = canonicalInputs(offset);
    return { static_cast<std::uint32_t>(offset + 1), inputs.mDestinationFormat, inputs.mVariant,
             inputs.mParameters.mTonemapType };
}

bool sameKey(const TonemapCaseKey& left, const TonemapCaseKey& right)
{
    return left == right;
}

std::uint32_t formatWireValue(PixelFormat format)
{
    return format == PixelFormat::RGBA8Unorm ? RGBA8_WIRE_VALUE : RGBA16F_WIRE_VALUE;
}

bool canonicalPixel(float value, PixelFormat format)
{
    if (!std::isfinite(value))
    {
        return false;
    }

    if (format == PixelFormat::RGBA8Unorm)
    {
        if (value < 0.f || value > 1.f)
        {
            return false;
        }
        const int code = static_cast<int>(std::lround(value * 255.f));
        return value == static_cast<float>(code) / 255.f;
    }

    return format == PixelFormat::RGBA16Float && halfBitsToFloat(floatToHalfBits(value)) == value;
}

void appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value >> 24));
    bytes.push_back(static_cast<std::uint8_t>(value >> 16));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    bytes.push_back(static_cast<std::uint8_t>(value));
}

void appendU64(std::vector<std::uint8_t>& bytes, std::uint64_t value)
{
    appendU32(bytes, static_cast<std::uint32_t>(value >> 32));
    appendU32(bytes, static_cast<std::uint32_t>(value));
}

void appendFloat(std::vector<std::uint8_t>& bytes, float value)
{
    // Positive zero is the one canonical wire representation of zero.
    appendU32(bytes, floatBits(value == 0.f ? 0.f : value));
}

class ArtifactReader
{
public:
    explicit ArtifactReader(const std::vector<std::uint8_t>& bytes) : mBytes(bytes) {}

    bool readU32(std::uint32_t& value)
    {
        if (mOffset > mBytes.size() || mBytes.size() - mOffset < sizeof(value))
        {
            return false;
        }
        value = (static_cast<std::uint32_t>(mBytes[mOffset]) << 24) |
                (static_cast<std::uint32_t>(mBytes[mOffset + 1]) << 16) |
                (static_cast<std::uint32_t>(mBytes[mOffset + 2]) << 8) |
                static_cast<std::uint32_t>(mBytes[mOffset + 3]);
        mOffset += sizeof(value);
        return true;
    }

    bool readU64(std::uint64_t& value)
    {
        std::uint32_t high = 0;
        std::uint32_t low = 0;
        if (!readU32(high) || !readU32(low))
        {
            return false;
        }
        value = (static_cast<std::uint64_t>(high) << 32) | low;
        return true;
    }

    bool readBytes(std::uint8_t* destination, std::size_t size)
    {
        if (mOffset > mBytes.size() || mBytes.size() - mOffset < size)
        {
            return false;
        }
        std::copy_n(mBytes.data() + mOffset, size, destination);
        mOffset += size;
        return true;
    }

    std::size_t offset() const noexcept { return mOffset; }

private:
    const std::vector<std::uint8_t>& mBytes;
    std::size_t mOffset = 0;
};

void hashByte(std::uint64_t& hash, std::uint8_t value)
{
    constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
    hash ^= value;
    hash *= FNV_PRIME;
}

void hashU16(std::uint64_t& hash, std::uint16_t value)
{
    hashByte(hash, static_cast<std::uint8_t>(value >> 8));
    hashByte(hash, static_cast<std::uint8_t>(value));
}

void hashU32(std::uint64_t& hash, std::uint32_t value)
{
    hashByte(hash, static_cast<std::uint8_t>(value >> 24));
    hashByte(hash, static_cast<std::uint8_t>(value >> 16));
    hashByte(hash, static_cast<std::uint8_t>(value >> 8));
    hashByte(hash, static_cast<std::uint8_t>(value));
}

bool expectedU32(ArtifactReader& reader, std::uint32_t expected, const std::string& field, std::string* error)
{
    std::uint32_t actual = 0;
    if (!reader.readU32(actual))
    {
        return fail(error, "artifact ends before " + field);
    }
    if (actual != expected)
    {
        std::ostringstream message;
        message << field << " is " << actual << ", expected " << expected;
        return fail(error, message.str());
    }
    return true;
}

bool expectedU64(ArtifactReader& reader, std::uint64_t expected, const std::string& field, std::string* error)
{
    std::uint64_t actual = 0;
    if (!reader.readU64(actual))
    {
        return fail(error, "artifact ends before " + field);
    }
    if (actual != expected)
    {
        std::ostringstream message;
        message << field << " is " << actual << ", expected " << expected;
        return fail(error, message.str());
    }
    return true;
}

bool expectedFloat(ArtifactReader& reader, float expected, const std::string& field, std::string* error)
{
    return expectedU32(reader, floatBits(expected), field, error);
}

std::filesystem::path temporaryArtifactPath(const std::filesystem::path& destination)
{
    static std::atomic<std::uint64_t> serial{ 0 };
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::uint64_t sequence = serial.fetch_add(1, std::memory_order_relaxed);
    const std::uint64_t token = static_cast<std::uint64_t>(now) ^
                                (sequence + 0x9e3779b97f4a7c15ULL + (sequence << 6) + (sequence >> 2));
    std::ostringstream suffix;
    suffix << ".tmp." << std::hex << token;
    std::filesystem::path temporary = destination;
    temporary += suffix.str();
    return temporary;
}

std::FILE* openExclusive(const std::filesystem::path& path, int& open_error)
{
#if defined(_WIN32)
    std::FILE* file = nullptr;
    open_error = _wfopen_s(&file, path.c_str(), L"wbx");
    return file;
#else
    errno = 0;
    std::FILE* file = std::fopen(path.c_str(), "wbx");
    open_error = errno;
    return file;
#endif
}

}

std::uint16_t floatToHalfBits(float value) noexcept
{
    const std::uint32_t bits = floatBits(value);
    const std::uint16_t sign = static_cast<std::uint16_t>((bits >> 16) & 0x8000U);
    const std::uint32_t exponent = (bits >> 23) & 0xffU;
    const std::uint32_t significand = bits & 0x7fffffU;

    if (exponent == 0xffU)
    {
        if (significand == 0)
        {
            return static_cast<std::uint16_t>(sign | 0x7c00U);
        }
        std::uint16_t payload = static_cast<std::uint16_t>(significand >> 13);
        payload = static_cast<std::uint16_t>(payload | 0x0200U);
        return static_cast<std::uint16_t>(sign | 0x7c00U | payload);
    }

    const int half_exponent = static_cast<int>(exponent) - 127 + 15;
    if (half_exponent >= 31)
    {
        return static_cast<std::uint16_t>(sign | 0x7c00U);
    }

    if (half_exponent <= 0)
    {
        if (half_exponent < -10)
        {
            return sign;
        }

        const std::uint32_t normalized = significand | 0x800000U;
        const unsigned shift = static_cast<unsigned>(14 - half_exponent);
        std::uint32_t rounded = normalized >> shift;
        const std::uint32_t remainder = normalized & ((1U << shift) - 1U);
        const std::uint32_t halfway = 1U << (shift - 1U);
        if (remainder > halfway || (remainder == halfway && (rounded & 1U) != 0))
        {
            ++rounded;
        }
        return static_cast<std::uint16_t>(sign | rounded);
    }

    std::uint32_t rounded = (static_cast<std::uint32_t>(half_exponent) << 10) | (significand >> 13);
    const std::uint32_t remainder = significand & 0x1fffU;
    if (remainder > 0x1000U || (remainder == 0x1000U && (rounded & 1U) != 0))
    {
        ++rounded;
    }
    return static_cast<std::uint16_t>(sign | rounded);
}

float halfBitsToFloat(std::uint16_t bits) noexcept
{
    const std::uint32_t sign = static_cast<std::uint32_t>(bits & 0x8000U) << 16;
    std::uint32_t exponent = (bits >> 10) & 0x1fU;
    std::uint32_t significand = bits & 0x03ffU;
    std::uint32_t result = 0;

    if (exponent == 0)
    {
        if (significand == 0)
        {
            result = sign;
        }
        else
        {
            exponent = 113;
            while ((significand & 0x0400U) == 0)
            {
                significand <<= 1;
                --exponent;
            }
            significand &= 0x03ffU;
            result = sign | (exponent << 23) | (significand << 13);
        }
    }
    else if (exponent == 0x1fU)
    {
        result = sign | 0x7f800000U | (significand << 13);
    }
    else
    {
        result = sign | ((exponent + 112U) << 23) | (significand << 13);
    }
    return bitsToFloat(result);
}

TonemapFixture makeTonemapFixture()
{
    constexpr std::array<float, 8> LEVELS{ 0.f, 0.04f, 0.08f, 0.2f, 0.76f, 1.f, 2.f, 8.f };

    TonemapFixture fixture;
    fixture.mScreenTriangle = {
        -1.f,  1.f, 0.f, 0.f,
        -1.f, -3.f, 0.f, 0.f,
         3.f,  1.f, 0.f, 0.f
    };

    for (std::size_t pixel = 0; pixel < TONEMAP_DIAGNOSTIC_PIXEL_COUNT; ++pixel)
    {
        fixture.mSceneRGBA16F[pixel * 4] = floatToHalfBits(LEVELS[pixel % LEVELS.size()]);
        fixture.mSceneRGBA16F[pixel * 4 + 1] = floatToHalfBits(LEVELS[(pixel * 3 + 1) % LEVELS.size()] * 0.75f);
        fixture.mSceneRGBA16F[pixel * 4 + 2] = floatToHalfBits(LEVELS[(pixel * 5 + 2) % LEVELS.size()] * 1.25f);
        fixture.mSceneRGBA16F[pixel * 4 + 3] = floatToHalfBits(static_cast<float>(pixel % 7) / 6.f);
    }
    fixture.mExposureR16F = floatToHalfBits(0.85f);
    return fixture;
}

std::uint64_t tonemapFixtureFingerprint()
{
    constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    const TonemapFixture fixture = makeTonemapFixture();
    std::uint64_t hash = FNV_OFFSET_BASIS;
    hashU32(hash, TONEMAP_DIAGNOSTIC_FIXTURE_VERSION);
    hashU32(hash, fixture.mExtent.mWidth);
    hashU32(hash, fixture.mExtent.mHeight);
    hashU32(hash, BOTTOM_LEFT_WIRE_VALUE);
    for (float component : fixture.mScreenTriangle)
    {
        hashU32(hash, floatBits(component));
    }
    for (std::uint16_t component : fixture.mSceneRGBA16F)
    {
        hashU16(hash, component);
    }
    hashU16(hash, fixture.mExposureR16F);
    return hash;
}

TonemapCases makeTonemapCases()
{
    TonemapCases cases{};
    for (std::size_t offset = 0; offset < cases.size(); ++offset)
    {
        TonemapCase& diagnostic_case = cases[offset];
        diagnostic_case.mKey = canonicalKey(offset);
        diagnostic_case.mInputs = canonicalInputs(offset);
        auto frame = buildTonemapFrame(diagnostic_case.mInputs);
        if (!frame)
        {
            std::terminate();
        }
        diagnostic_case.mFrame = std::move(*frame);
    }
    return cases;
}

TonemapArtifact makeTonemapArtifact()
{
    TonemapArtifact artifact;
    artifact.mCases.reserve(TONEMAP_DIAGNOSTIC_CASE_COUNT);
    for (std::size_t offset = 0; offset < TONEMAP_DIAGNOSTIC_CASE_COUNT; ++offset)
    {
        artifact.mCases.push_back({ canonicalKey(offset), {} });
    }
    return artifact;
}

bool validateTonemapArtifact(const TonemapArtifact& artifact, std::string* error)
{
    clearError(error);
    if (artifact.mCases.size() != TONEMAP_DIAGNOSTIC_CASE_COUNT)
    {
        std::ostringstream message;
        message << "artifact has " << artifact.mCases.size() << " cases, expected " << TONEMAP_DIAGNOSTIC_CASE_COUNT;
        return fail(error, message.str());
    }

    for (std::size_t offset = 0; offset < artifact.mCases.size(); ++offset)
    {
        const TonemapArtifactCase& artifact_case = artifact.mCases[offset];
        const TonemapCaseKey expected = canonicalKey(offset);
        if (!sameKey(artifact_case.mKey, expected))
        {
            return fail(error, caseField(expected.mIndex, "metadata is not canonical"));
        }
        if (artifact_case.mPixels.size() != TONEMAP_DIAGNOSTIC_COMPONENT_COUNT)
        {
            std::ostringstream message;
            message << "case " << expected.mIndex << " has " << artifact_case.mPixels.size()
                    << " components, expected " << TONEMAP_DIAGNOSTIC_COMPONENT_COUNT;
            return fail(error, message.str());
        }
        for (std::size_t component = 0; component < artifact_case.mPixels.size(); ++component)
        {
            if (!canonicalPixel(artifact_case.mPixels[component], expected.mDestinationFormat))
            {
                std::ostringstream message;
                message << "case " << expected.mIndex << " component " << component
                        << " is not a finite value representable by its destination format";
                return fail(error, message.str());
            }
        }
    }
    return true;
}

bool encodeTonemapArtifact(const TonemapArtifact& artifact,
                           std::vector<std::uint8_t>& encoded,
                           std::string* error)
{
    clearError(error);
    if (!validateTonemapArtifact(artifact, error))
    {
        return false;
    }

    std::vector<std::uint8_t> result;
    result.reserve(ARTIFACT_SIZE);
    result.insert(result.end(), ARTIFACT_MAGIC.begin(), ARTIFACT_MAGIC.end());
    appendU32(result, TONEMAP_ARTIFACT_SCHEMA_VERSION);
    appendU32(result, TONEMAP_DIAGNOSTIC_FIXTURE_VERSION);
    appendU64(result, tonemapFixtureFingerprint());
    appendU32(result, TONEMAP_DIAGNOSTIC_WIDTH);
    appendU32(result, TONEMAP_DIAGNOSTIC_HEIGHT);
    appendU32(result, TONEMAP_DIAGNOSTIC_CHANNELS);
    appendU32(result, BOTTOM_LEFT_WIRE_VALUE);
    appendU32(result, static_cast<std::uint32_t>(TONEMAP_DIAGNOSTIC_CASE_COUNT));

    for (std::size_t offset = 0; offset < artifact.mCases.size(); ++offset)
    {
        const TonemapArtifactCase& artifact_case = artifact.mCases[offset];
        const TonemapInputs inputs = canonicalInputs(offset);
        appendU32(result, artifact_case.mKey.mIndex);
        appendU64(result, inputs.mFrame);
        appendU32(result, formatWireValue(inputs.mDestinationFormat));
        appendU64(result, static_cast<std::uint64_t>(inputs.mVariant));
        appendFloat(result, inputs.mParameters.mExposure);
        appendFloat(result, inputs.mParameters.mTonemapMix);
        appendU32(result, inputs.mParameters.mTonemapType);
        appendFloat(result, inputs.mParameters.mGamma);
        appendU32(result, static_cast<std::uint32_t>(artifact_case.mPixels.size()));
        for (float pixel : artifact_case.mPixels)
        {
            appendFloat(result, pixel);
        }
    }

    if (result.size() != ARTIFACT_SIZE)
    {
        return fail(error, "artifact encoder produced an unexpected byte count");
    }
    encoded = std::move(result);
    return true;
}

bool decodeTonemapArtifact(const std::vector<std::uint8_t>& encoded,
                           TonemapArtifact& artifact,
                           std::string* error)
{
    clearError(error);
    if (encoded.size() != ARTIFACT_SIZE)
    {
        std::ostringstream message;
        message << "artifact has " << encoded.size() << " bytes, expected " << ARTIFACT_SIZE;
        return fail(error, message.str());
    }

    ArtifactReader reader(encoded);
    std::array<std::uint8_t, ARTIFACT_MAGIC.size()> magic{};
    if (!reader.readBytes(magic.data(), magic.size()) || magic != ARTIFACT_MAGIC)
    {
        return fail(error, "artifact magic is invalid");
    }
    if (!expectedU32(reader, TONEMAP_ARTIFACT_SCHEMA_VERSION, "schema version", error) ||
        !expectedU32(reader, TONEMAP_DIAGNOSTIC_FIXTURE_VERSION, "fixture version", error) ||
        !expectedU64(reader, tonemapFixtureFingerprint(), "fixture fingerprint", error) ||
        !expectedU32(reader, TONEMAP_DIAGNOSTIC_WIDTH, "width", error) ||
        !expectedU32(reader, TONEMAP_DIAGNOSTIC_HEIGHT, "height", error) ||
        !expectedU32(reader, TONEMAP_DIAGNOSTIC_CHANNELS, "channel count", error) ||
        !expectedU32(reader, BOTTOM_LEFT_WIRE_VALUE, "row origin", error) ||
        !expectedU32(reader, static_cast<std::uint32_t>(TONEMAP_DIAGNOSTIC_CASE_COUNT), "case count", error))
    {
        return false;
    }

    TonemapArtifact result = makeTonemapArtifact();
    for (std::size_t offset = 0; offset < result.mCases.size(); ++offset)
    {
        const TonemapInputs inputs = canonicalInputs(offset);
        const TonemapCaseKey key = canonicalKey(offset);
        const std::string prefix = "case " + std::to_string(key.mIndex) + ' ';
        if (!expectedU32(reader, key.mIndex, prefix + "index", error) ||
            !expectedU64(reader, inputs.mFrame, prefix + "frame", error) ||
            !expectedU32(reader, formatWireValue(inputs.mDestinationFormat), prefix + "format", error) ||
            !expectedU64(reader, static_cast<std::uint64_t>(inputs.mVariant), prefix + "variant", error) ||
            !expectedFloat(reader, inputs.mParameters.mExposure, prefix + "exposure", error) ||
            !expectedFloat(reader, inputs.mParameters.mTonemapMix, prefix + "tonemap mix", error) ||
            !expectedU32(reader, inputs.mParameters.mTonemapType, prefix + "tonemap type", error) ||
            !expectedFloat(reader, inputs.mParameters.mGamma, prefix + "gamma", error) ||
            !expectedU32(reader, static_cast<std::uint32_t>(TONEMAP_DIAGNOSTIC_COMPONENT_COUNT), prefix + "component count", error))
        {
            return false;
        }

        std::vector<float>& pixels = result.mCases[offset].mPixels;
        pixels.reserve(TONEMAP_DIAGNOSTIC_COMPONENT_COUNT);
        for (std::size_t component = 0; component < TONEMAP_DIAGNOSTIC_COMPONENT_COUNT; ++component)
        {
            std::uint32_t bits = 0;
            if (!reader.readU32(bits))
            {
                return fail(error, prefix + "pixel payload is truncated");
            }
            if (bits == 0x80000000U)
            {
                return fail(error, prefix + "pixel payload contains non-canonical negative zero");
            }
            const float value = bitsToFloat(bits);
            if (!canonicalPixel(value, inputs.mDestinationFormat))
            {
                std::ostringstream message;
                message << prefix << "component " << component
                        << " is not a finite value representable by its destination format";
                return fail(error, message.str());
            }
            pixels.push_back(value);
        }
    }

    if (reader.offset() != encoded.size())
    {
        return fail(error, "artifact has trailing data");
    }
    artifact = std::move(result);
    return true;
}

bool writeTonemapArtifact(const std::filesystem::path& destination,
                          const TonemapArtifact& artifact,
                          std::string* error)
{
    clearError(error);
    if (destination.empty() || destination.filename().empty())
    {
        return fail(error, "artifact destination must name a file");
    }

    std::vector<std::uint8_t> encoded;
    if (!encodeTonemapArtifact(artifact, encoded, error))
    {
        return false;
    }

    std::filesystem::path temporary;
    std::FILE* output = nullptr;
    int open_error = 0;
    for (std::size_t attempt = 0; attempt < 64 && !output; ++attempt)
    {
        temporary = temporaryArtifactPath(destination);
        output = openExclusive(temporary, open_error);
        if (!output && open_error != EEXIST)
        {
            return fail(error, "cannot create artifact temporary file: " +
                                   std::error_code(open_error, std::generic_category()).message());
        }
    }
    if (!output)
    {
        return fail(error, "cannot reserve a unique artifact temporary file");
    }

    const bool wrote = std::fwrite(encoded.data(), 1, encoded.size(), output) == encoded.size();
    const bool flushed = wrote && std::fflush(output) == 0;
    const bool closed = std::fclose(output) == 0;
    if (!wrote || !flushed || !closed)
    {
        std::error_code cleanup_error;
        std::filesystem::remove(temporary, cleanup_error);
        return fail(error, "cannot write artifact temporary file: " + temporary.string());
    }

    // A same-directory hard link publishes the fully written bytes atomically
    // and fails if another writer created the destination first.
    std::error_code file_error;
    std::filesystem::create_hard_link(temporary, destination, file_error);
    if (file_error)
    {
        std::error_code cleanup_error;
        std::filesystem::remove(temporary, cleanup_error);
        return fail(error, "cannot publish artifact: " + file_error.message());
    }
    std::filesystem::remove(temporary, file_error);
    if (file_error)
    {
        return fail(error, "artifact published but its temporary link could not be removed: " +
                               file_error.message());
    }
    return true;
}

bool readTonemapArtifact(const std::filesystem::path& source,
                         TonemapArtifact& artifact,
                         std::string* error)
{
    clearError(error);
    std::error_code file_error;
    const std::uintmax_t size = std::filesystem::file_size(source, file_error);
    if (file_error)
    {
        return fail(error, "cannot inspect artifact file: " + file_error.message());
    }
    if (size != ARTIFACT_SIZE)
    {
        std::ostringstream message;
        message << "artifact file has " << size << " bytes, expected " << ARTIFACT_SIZE;
        return fail(error, message.str());
    }

    std::ifstream input(source, std::ios::binary | std::ios::in);
    if (!input)
    {
        return fail(error, "cannot open artifact file: " + source.string());
    }
    std::vector<std::uint8_t> encoded(static_cast<std::size_t>(size));
    input.read(reinterpret_cast<char*>(encoded.data()), static_cast<std::streamsize>(encoded.size()));
    if (!input || input.peek() != std::ifstream::traits_type::eof())
    {
        return fail(error, "cannot read complete artifact file: " + source.string());
    }
    return decodeTonemapArtifact(encoded, artifact, error);
}

float tonemapComparisonTolerance(PixelFormat format) noexcept
{
    switch (format)
    {
        case PixelFormat::RGBA8Unorm:
            return TONEMAP_RGBA8_TOLERANCE;
        case PixelFormat::RGBA16Float:
            return TONEMAP_RGBA16F_TOLERANCE;
        default:
            return 0.f;
    }
}

TonemapComparisonStats compareTonemapArtifacts(const TonemapArtifact& reference,
                                                const TonemapArtifact& candidate)
{
    TonemapComparisonStats stats;
    std::string validation_error;
    if (!validateTonemapArtifact(reference, &validation_error))
    {
        stats.mError = "reference " + validation_error;
        return stats;
    }
    if (!validateTonemapArtifact(candidate, &validation_error))
    {
        stats.mError = "candidate " + validation_error;
        return stats;
    }

    stats.mComparable = true;
    stats.mComparedCases = reference.mCases.size();
    for (std::size_t case_offset = 0; case_offset < reference.mCases.size(); ++case_offset)
    {
        const TonemapArtifactCase& reference_case = reference.mCases[case_offset];
        const TonemapArtifactCase& candidate_case = candidate.mCases[case_offset];
        const float tolerance = tonemapComparisonTolerance(reference_case.mKey.mDestinationFormat);
        for (std::size_t component = 0; component < reference_case.mPixels.size(); ++component)
        {
            const float reference_value = reference_case.mPixels[component];
            const float candidate_value = candidate_case.mPixels[component];
            const double delta = std::fabs(static_cast<double>(reference_value) - candidate_value);
            stats.mMaximumAbsoluteError = std::max(stats.mMaximumAbsoluteError, delta);
            ++stats.mComparedComponents;
            if (delta <= tolerance)
            {
                continue;
            }

            ++stats.mMismatchCount;
            if (stats.mFirstMismatchCase == 0)
            {
                stats.mFirstMismatchCase = reference_case.mKey.mIndex;
                stats.mFirstMismatchPixel = component / TONEMAP_DIAGNOSTIC_CHANNELS;
                stats.mFirstMismatchChannel = static_cast<std::uint32_t>(component % TONEMAP_DIAGNOSTIC_CHANNELS);
                stats.mFirstReference = reference_value;
                stats.mFirstCandidate = candidate_value;
                stats.mFirstTolerance = tolerance;
            }
        }
    }
    stats.mMatch = stats.mMismatchCount == 0;
    return stats;
}

}
