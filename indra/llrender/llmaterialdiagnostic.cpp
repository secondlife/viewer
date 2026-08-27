/**
 * @file llmaterialdiagnostic.cpp
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

#include "llmaterialdiagnostic.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>
#include <utility>

namespace LLRenderContract
{
namespace
{

    static_assert(sizeof(float) == sizeof(std::uint32_t), "material artifacts require 32-bit floats");
    static_assert(std::numeric_limits<float>::is_iec559, "material artifacts require IEEE 754 floats");

    constexpr std::array<std::uint8_t, 8> ARTIFACT_MAGIC{ 'L', 'L', 'M', 'A', 'T', 'E', 'R', 'L' };
    constexpr std::uint32_t               BOTTOM_LEFT_WIRE_VALUE       = 1;
    constexpr std::uint32_t               GBUFFER0_PLANE               = 1;
    constexpr std::uint32_t               GBUFFER1_PLANE               = 2;
    constexpr std::uint32_t               GBUFFER2_PLANE               = 3;
    constexpr std::uint32_t               DEPTH_PLANE                  = 4;
    constexpr std::uint32_t               RGBA8_WIRE_VALUE             = 1;
    constexpr std::uint32_t               RGBA16_WIRE_VALUE            = 2;
    constexpr std::uint32_t               DEPTH24_WIRE_VALUE           = 3;
    constexpr std::size_t                 ARTIFACT_HEADER_SIZE         = 44;
    constexpr std::size_t                 ARTIFACT_PLANE_METADATA_SIZE = 16;
    constexpr std::size_t                 ARTIFACT_PLANE_COUNT         = 4;
    constexpr std::size_t                 ARTIFACT_COMPONENT_COUNT =
        MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT * 3 + MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT;
    constexpr std::size_t ARTIFACT_SIZE =
        ARTIFACT_HEADER_SIZE + ARTIFACT_PLANE_COUNT * ARTIFACT_PLANE_METADATA_SIZE + ARTIFACT_COMPONENT_COUNT * sizeof(float);
    static_assert(ARTIFACT_SIZE == MATERIAL_ARTIFACT_BYTE_SIZE);

    struct PlaneSpec
    {
        std::uint32_t mId          = 0;
        std::uint32_t mFormat      = 0;
        std::uint32_t mChannels    = 0;
        std::size_t   mComponents  = 0;
        std::uint32_t mMaximumCode = 0;
        float         mTolerance   = 0.f;
        const char*   mName        = nullptr;
    };

    constexpr std::array<PlaneSpec, ARTIFACT_PLANE_COUNT> PLANES{
        PlaneSpec{ GBUFFER0_PLANE, RGBA8_WIRE_VALUE, 4, MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT, 0xffU, MATERIAL_RGBA8_TOLERANCE,
                   "G-buffer 0" },
        PlaneSpec{ GBUFFER1_PLANE, RGBA8_WIRE_VALUE, 4, MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT, 0xffU, MATERIAL_RGBA8_TOLERANCE,
                   "G-buffer 1" },
        PlaneSpec{ GBUFFER2_PLANE, RGBA16_WIRE_VALUE, 4, MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT, 0xffffU, MATERIAL_RGBA16_TOLERANCE,
                   "G-buffer 2" },
        PlaneSpec{ DEPTH_PLANE, DEPTH24_WIRE_VALUE, 1, MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT, 0xffffffU, MATERIAL_DEPTH24_TOLERANCE,
                   "depth" }
    };

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

    const std::vector<float>& plane(const MaterialArtifact& artifact, std::size_t index)
    {
        switch (index)
        {
            case 0:
                return artifact.mGBuffer0RGBA8;
            case 1:
                return artifact.mGBuffer1RGBA8;
            case 2:
                return artifact.mGBuffer2RGBA16;
            default:
                return artifact.mDepth24;
        }
    }

    std::vector<float>& plane(MaterialArtifact& artifact, std::size_t index)
    {
        return const_cast<std::vector<float>&>(plane(static_cast<const MaterialArtifact&>(artifact), index));
    }

    float normalizedCode(std::uint32_t code, std::uint32_t maximum) noexcept
    {
        if (code > maximum)
        {
            return std::numeric_limits<float>::quiet_NaN();
        }
        return static_cast<float>(code) / static_cast<float>(maximum);
    }

    bool canonicalValue(float value, std::uint32_t maximum_code)
    {
        if (!std::isfinite(value) || value < 0.f || value > 1.f)
        {
            return false;
        }
        const auto code = static_cast<std::uint32_t>(std::llround(static_cast<double>(value) * maximum_code));
        return code <= maximum_code && value == normalizedCode(code, maximum_code);
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
            value = (static_cast<std::uint32_t>(mBytes[mOffset]) << 24) | (static_cast<std::uint32_t>(mBytes[mOffset + 1]) << 16) |
                    (static_cast<std::uint32_t>(mBytes[mOffset + 2]) << 8) | static_cast<std::uint32_t>(mBytes[mOffset + 3]);
            mOffset += sizeof(value);
            return true;
        }

        bool readU64(std::uint64_t& value)
        {
            std::uint32_t high = 0;
            std::uint32_t low  = 0;
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
        std::size_t                      mOffset = 0;
    };

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

    template<std::size_t Size>
    void hashFloats(std::uint64_t& hash, const std::array<float, Size>& values)
    {
        for (float value : values)
        {
            hashU32(hash, floatBits(value));
        }
    }

    void hashParameters(std::uint64_t& hash, const MaterialParameters& parameters)
    {
        hashFloats(hash, parameters.mModelviewMatrix);
        hashFloats(hash, parameters.mModelviewProjectionMatrix);
        hashFloats(hash, parameters.mNormalMatrix);
        hashFloats(hash, parameters.mTextureMatrix0);
        hashFloats(hash, parameters.mSpecularColor);
        hashFloats(hash, parameters.mClipPlane);
        hashU32(hash, floatBits(parameters.mEnvironmentIntensity));
        hashU32(hash, floatBits(parameters.mEmissiveBrightness));
        hashU32(hash, floatBits(parameters.mMirror));
    }

    template<std::size_t Size>
    void copyFloats(std::array<std::uint8_t, MATERIAL_VERTEX_BUFFER_SIZE>& destination, std::size_t offset,
                    const std::array<float, Size>& source)
    {
        std::memcpy(destination.data() + offset, source.data(), sizeof(source));
    }

    template<std::size_t Size>
    void identity(std::array<float, Size>& matrix, std::size_t dimension)
    {
        for (std::size_t diagonal = 0; diagonal < dimension; ++diagonal)
        {
            matrix[diagonal * dimension + diagonal] = 1.f;
        }
    }

    std::filesystem::path temporaryArtifactPath(const std::filesystem::path& destination)
    {
        static std::atomic<std::uint64_t> serial{ 0 };
        const auto                        now      = std::chrono::steady_clock::now().time_since_epoch().count();
        const std::uint64_t               sequence = serial.fetch_add(1, std::memory_order_relaxed);
        const std::uint64_t               token =
            static_cast<std::uint64_t>(now) ^ (sequence + 0x9e3779b97f4a7c15ULL + (sequence << 6) + (sequence >> 2));
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
        open_error      = _wfopen_s(&file, path.c_str(), L"wbx");
        return file;
#else
        errno           = 0;
        std::FILE* file = std::fopen(path.c_str(), "wbx");
        open_error      = errno;
        return file;
#endif
    }

} // namespace

float materialUnorm8(std::uint8_t code) noexcept
{
    return normalizedCode(code, 0xffU);
}

float materialUnorm16(std::uint16_t code) noexcept
{
    return normalizedCode(code, 0xffffU);
}

float materialDepth24(std::uint32_t code) noexcept
{
    return normalizedCode(code, 0xffffffU);
}

MaterialFixture makeMaterialFixture()
{
    MaterialFixture fixture;

    constexpr std::array<float, 16>        POSITIONS{ -0.82f, -0.74f, 0.18f,  1.f, 0.78f,  -0.61f, 0.42f, 1.f,
                                               0.63f,  0.81f,  -0.12f, 1.f, -0.71f, 0.57f,  0.31f, 1.f };
    constexpr std::array<float, 16>        NORMALS{ 0.10f, 0.20f,  0.97f, 0.f, -0.18f, 0.12f,  0.98f, 0.f,
                                             0.24f, -0.08f, 0.96f, 0.f, -0.12f, -0.22f, 0.97f, 0.f };
    constexpr std::array<float, 8>         TEXCOORD0{ 0.15f, 0.05f, 2.85f, 0.25f, 2.45f, 2.95f, -0.35f, 2.40f };
    constexpr std::array<float, 8>         TEXCOORD1{ 0.35f, 0.20f, 3.75f, 0.55f, 2.90f, 3.60f, -0.60f, 2.75f };
    constexpr std::array<float, 8>         TEXCOORD2{ 0.05f, 0.45f, 2.20f, -0.15f, 3.20f, 2.30f, -0.25f, 3.10f };
    constexpr std::array<std::uint8_t, 16> COLORS{ 241, 109, 53, 229, 67, 223, 137, 197, 151, 79, 239, 173, 211, 187, 41, 251 };
    constexpr std::array<float, 16>        TANGENTS{ 0.98f, 0.05f, 0.18f,  1.f, 0.94f, -0.21f, 0.26f,  -1.f,
                                              0.91f, 0.31f, -0.19f, 1.f, 0.96f, -0.11f, -0.24f, -1.f };

    copyFloats(fixture.mVertexBytes, MATERIAL_POSITION_OFFSET, POSITIONS);
    copyFloats(fixture.mVertexBytes, MATERIAL_NORMAL_OFFSET, NORMALS);
    copyFloats(fixture.mVertexBytes, MATERIAL_TEXCOORD0_OFFSET, TEXCOORD0);
    copyFloats(fixture.mVertexBytes, MATERIAL_TEXCOORD1_OFFSET, TEXCOORD1);
    copyFloats(fixture.mVertexBytes, MATERIAL_TEXCOORD2_OFFSET, TEXCOORD2);
    std::copy(COLORS.begin(), COLORS.end(), fixture.mVertexBytes.begin() + MATERIAL_COLOR_OFFSET);
    copyFloats(fixture.mVertexBytes, MATERIAL_TANGENT_OFFSET, TANGENTS);
    fixture.mIndices = MATERIAL_INDICES;

    for (std::size_t texture = 0; texture < fixture.mTextureRGBA8.size(); ++texture)
    {
        for (std::size_t mip = 0; mip < MATERIAL_TEXTURE_MIP_LEVELS; ++mip)
        {
            const std::size_t first = MATERIAL_TEXTURE_MIP_BYTE_OFFSETS[mip];
            const std::size_t size  = MATERIAL_TEXTURE_MIP_BYTE_SIZES[mip];
            for (std::size_t component = 0; component < size; ++component)
            {
                const std::size_t texel   = component / MATERIAL_DIAGNOSTIC_CHANNELS;
                const std::size_t channel = component % MATERIAL_DIAGNOSTIC_CHANNELS;
                fixture.mTextureRGBA8[texture][first + component] =
                    static_cast<std::uint8_t>(1 + (texture * 71 + mip * 43 + texel * 29 + channel * 53 + 17) % 255);
            }
        }
    }

    identity(fixture.mParameters.mModelviewMatrix, 4);
    identity(fixture.mParameters.mModelviewProjectionMatrix, 4);
    identity(fixture.mParameters.mNormalMatrix, 3);
    identity(fixture.mParameters.mTextureMatrix0, 4);
    fixture.mParameters.mSpecularColor = { 0.31f, 0.57f, 0.83f, 0.68f };
    // Mirror clipping at view-space x=0 keeps the right half and discards the
    // left half of the asymmetric quad.
    fixture.mParameters.mClipPlane            = { 1.f, 0.f, 0.f, 0.f };
    fixture.mParameters.mEnvironmentIntensity = 0.625f;
    fixture.mParameters.mEmissiveBrightness   = 1.f;
    fixture.mParameters.mMirror               = 1.f;

    for (std::size_t pixel = 0; pixel < MATERIAL_DIAGNOSTIC_PIXEL_COUNT; ++pixel)
    {
        const std::size_t   x    = pixel % MATERIAL_FRAME_WIDTH;
        const std::size_t   y    = pixel / MATERIAL_FRAME_WIDTH;
        const std::uint32_t base = (x + y) % 2 == 0 ? 0xd00000U : 0x280000U;
        fixture.mDepth24[pixel]  = static_cast<std::uint32_t>(base + (pixel * 0x1f123U + x * 0x207U) % 0x18000U);
        for (std::size_t channel = 0; channel < MATERIAL_DIAGNOSTIC_CHANNELS; ++channel)
        {
            const std::size_t component                = pixel * MATERIAL_DIAGNOSTIC_CHANNELS + channel;
            fixture.mGBuffer0SentinelRGBA8[component]  = static_cast<std::uint8_t>(1 + (pixel * 19 + channel * 47 + 13) % 255);
            fixture.mGBuffer1SentinelRGBA8[component]  = static_cast<std::uint8_t>(1 + (pixel * 31 + channel * 37 + 101) % 255);
            fixture.mGBuffer2SentinelRGBA16[component] = static_cast<std::uint16_t>(1 + (pixel * 1237 + channel * 7919 + 4001) % 65535);
        }
    }
    return fixture;
}

std::uint64_t materialFixtureFingerprint()
{
    constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    const MaterialFixture   fixture          = makeMaterialFixture();
    std::uint64_t           hash             = FNV_OFFSET_BASIS;
    hashU32(hash, MATERIAL_DIAGNOSTIC_FIXTURE_VERSION);
    hashU32(hash, fixture.mExtent.mWidth);
    hashU32(hash, fixture.mExtent.mHeight);
    hashU32(hash, BOTTOM_LEFT_WIRE_VALUE);
    for (std::uint8_t value : fixture.mVertexBytes)
        hashByte(hash, value);
    for (std::uint16_t value : fixture.mIndices)
        hashU16(hash, value);
    for (const auto& texture : fixture.mTextureRGBA8)
    {
        for (std::uint8_t value : texture)
            hashByte(hash, value);
    }
    hashParameters(hash, fixture.mParameters);
    for (std::uint32_t value : fixture.mDepth24)
        hashU32(hash, value);
    for (std::uint8_t value : fixture.mGBuffer0SentinelRGBA8)
        hashByte(hash, value);
    for (std::uint8_t value : fixture.mGBuffer1SentinelRGBA8)
        hashByte(hash, value);
    for (std::uint16_t value : fixture.mGBuffer2SentinelRGBA16)
        hashU16(hash, value);
    return hash;
}

MaterialCase makeMaterialCase()
{
    MaterialCase          result;
    const MaterialFixture fixture = makeMaterialFixture();
    result.mInputs.mFrame         = 1;
    result.mInputs.mParameters    = fixture.mParameters;
    auto frame                    = buildMaterialFrame(result.mInputs);
    if (!frame)
    {
        std::terminate();
    }
    result.mFrame = std::move(*frame);
    return result;
}

MaterialArtifact makeMaterialArtifact()
{
    return {};
}

bool validateMaterialArtifact(const MaterialArtifact& artifact, std::string* error)
{
    clearError(error);
    for (std::size_t plane_index = 0; plane_index < PLANES.size(); ++plane_index)
    {
        const PlaneSpec&          spec   = PLANES[plane_index];
        const std::vector<float>& values = plane(artifact, plane_index);
        if (values.size() != spec.mComponents)
        {
            std::ostringstream message;
            message << spec.mName << " has " << values.size() << " components, expected " << spec.mComponents;
            return fail(error, message.str());
        }
        for (std::size_t component = 0; component < values.size(); ++component)
        {
            if (!canonicalValue(values[component], spec.mMaximumCode))
            {
                std::ostringstream message;
                message << spec.mName << " component " << component << " is not a finite value representable by its storage format";
                return fail(error, message.str());
            }
        }
    }
    return true;
}

bool encodeMaterialArtifact(const MaterialArtifact& artifact, std::vector<std::uint8_t>& encoded, std::string* error)
{
    clearError(error);
    if (!validateMaterialArtifact(artifact, error))
    {
        return false;
    }

    std::vector<std::uint8_t> result;
    result.reserve(ARTIFACT_SIZE);
    result.insert(result.end(), ARTIFACT_MAGIC.begin(), ARTIFACT_MAGIC.end());
    appendU32(result, MATERIAL_ARTIFACT_SCHEMA_VERSION);
    appendU32(result, MATERIAL_DIAGNOSTIC_FIXTURE_VERSION);
    appendU64(result, materialFixtureFingerprint());
    appendU32(result, MATERIAL_FRAME_WIDTH);
    appendU32(result, MATERIAL_FRAME_HEIGHT);
    appendU32(result, MATERIAL_DIAGNOSTIC_CHANNELS);
    appendU32(result, BOTTOM_LEFT_WIRE_VALUE);
    appendU32(result, static_cast<std::uint32_t>(PLANES.size()));

    for (std::size_t plane_index = 0; plane_index < PLANES.size(); ++plane_index)
    {
        const PlaneSpec&          spec   = PLANES[plane_index];
        const std::vector<float>& values = plane(artifact, plane_index);
        appendU32(result, spec.mId);
        appendU32(result, spec.mFormat);
        appendU32(result, spec.mChannels);
        appendU32(result, static_cast<std::uint32_t>(spec.mComponents));
        for (float value : values)
        {
            appendFloat(result, value);
        }
    }

    if (result.size() != ARTIFACT_SIZE)
    {
        return fail(error, "artifact encoder produced an unexpected byte count");
    }
    encoded = std::move(result);
    return true;
}

bool decodeMaterialArtifact(const std::vector<std::uint8_t>& encoded, MaterialArtifact& artifact, std::string* error)
{
    clearError(error);
    if (encoded.size() != ARTIFACT_SIZE)
    {
        std::ostringstream message;
        message << "artifact has " << encoded.size() << " bytes, expected " << ARTIFACT_SIZE;
        return fail(error, message.str());
    }

    ArtifactReader                                  reader(encoded);
    std::array<std::uint8_t, ARTIFACT_MAGIC.size()> magic{};
    if (!reader.readBytes(magic.data(), magic.size()) || magic != ARTIFACT_MAGIC)
    {
        return fail(error, "artifact magic is invalid");
    }
    if (!expectedU32(reader, MATERIAL_ARTIFACT_SCHEMA_VERSION, "schema version", error) ||
        !expectedU32(reader, MATERIAL_DIAGNOSTIC_FIXTURE_VERSION, "fixture version", error) ||
        !expectedU64(reader, materialFixtureFingerprint(), "fixture fingerprint", error) ||
        !expectedU32(reader, MATERIAL_FRAME_WIDTH, "width", error) || !expectedU32(reader, MATERIAL_FRAME_HEIGHT, "height", error) ||
        !expectedU32(reader, MATERIAL_DIAGNOSTIC_CHANNELS, "color channel count", error) ||
        !expectedU32(reader, BOTTOM_LEFT_WIRE_VALUE, "row origin", error) ||
        !expectedU32(reader, static_cast<std::uint32_t>(PLANES.size()), "plane count", error))
    {
        return false;
    }

    MaterialArtifact result = makeMaterialArtifact();
    for (std::size_t plane_index = 0; plane_index < PLANES.size(); ++plane_index)
    {
        const PlaneSpec&  spec   = PLANES[plane_index];
        const std::string prefix = std::string(spec.mName) + ' ';
        if (!expectedU32(reader, spec.mId, prefix + "plane id", error) || !expectedU32(reader, spec.mFormat, prefix + "format", error) ||
            !expectedU32(reader, spec.mChannels, prefix + "channel count", error) ||
            !expectedU32(reader, static_cast<std::uint32_t>(spec.mComponents), prefix + "component count", error))
        {
            return false;
        }

        std::vector<float>& values = plane(result, plane_index);
        values.reserve(spec.mComponents);
        for (std::size_t component = 0; component < spec.mComponents; ++component)
        {
            std::uint32_t bits = 0;
            if (!reader.readU32(bits))
            {
                return fail(error, prefix + "payload is truncated");
            }
            if (bits == 0x80000000U)
            {
                return fail(error, prefix + "payload contains non-canonical negative zero");
            }
            const float value = bitsToFloat(bits);
            if (!canonicalValue(value, spec.mMaximumCode))
            {
                std::ostringstream message;
                message << prefix << "component " << component << " is not a finite value representable by its storage format";
                return fail(error, message.str());
            }
            values.push_back(value);
        }
    }

    if (reader.offset() != encoded.size())
    {
        return fail(error, "artifact has trailing data");
    }
    artifact = std::move(result);
    return true;
}

bool writeMaterialArtifact(const std::filesystem::path& destination, const MaterialArtifact& artifact, std::string* error)
{
    clearError(error);
    if (destination.empty() || destination.filename().empty())
    {
        return fail(error, "artifact destination must name a file");
    }

    std::vector<std::uint8_t> encoded;
    if (!encodeMaterialArtifact(artifact, encoded, error))
    {
        return false;
    }

    std::filesystem::path temporary;
    std::FILE*            output     = nullptr;
    int                   open_error = 0;
    for (std::size_t attempt = 0; attempt < 64 && !output; ++attempt)
    {
        temporary = temporaryArtifactPath(destination);
        output    = openExclusive(temporary, open_error);
        if (!output && open_error != EEXIST)
        {
            return fail(error, "cannot create artifact temporary file: " + std::error_code(open_error, std::generic_category()).message());
        }
    }
    if (!output)
    {
        return fail(error, "cannot reserve a unique artifact temporary file");
    }

    const bool wrote   = std::fwrite(encoded.data(), 1, encoded.size(), output) == encoded.size();
    const bool flushed = wrote && std::fflush(output) == 0;
    const bool closed  = std::fclose(output) == 0;
    if (!wrote || !flushed || !closed)
    {
        std::error_code cleanup_error;
        std::filesystem::remove(temporary, cleanup_error);
        return fail(error, "cannot write artifact temporary file: " + temporary.string());
    }

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
        return fail(error, "artifact published but its temporary link could not be removed: " + file_error.message());
    }
    return true;
}

bool readMaterialArtifact(const std::filesystem::path& source, MaterialArtifact& artifact, std::string* error)
{
    clearError(error);
    std::error_code      file_error;
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
    return decodeMaterialArtifact(encoded, artifact, error);
}

MaterialComparisonStats compareMaterialArtifacts(const MaterialArtifact& reference, const MaterialArtifact& candidate)
{
    MaterialComparisonStats stats;
    std::string             validation_error;
    if (!validateMaterialArtifact(reference, &validation_error))
    {
        stats.mError = "reference " + validation_error;
        return stats;
    }
    if (!validateMaterialArtifact(candidate, &validation_error))
    {
        stats.mError = "candidate " + validation_error;
        return stats;
    }

    stats.mComparable = true;
    for (std::size_t plane_index = 0; plane_index < PLANES.size(); ++plane_index)
    {
        const PlaneSpec&          spec             = PLANES[plane_index];
        const std::vector<float>& reference_values = plane(reference, plane_index);
        const std::vector<float>& candidate_values = plane(candidate, plane_index);
        for (std::size_t component = 0; component < reference_values.size(); ++component)
        {
            const float  reference_value = reference_values[component];
            const float  candidate_value = candidate_values[component];
            const double delta           = std::fabs(static_cast<double>(reference_value) - candidate_value);
            stats.mMaximumAbsoluteError  = std::max(stats.mMaximumAbsoluteError, delta);
            ++stats.mComparedComponents;
            if (delta <= spec.mTolerance)
            {
                continue;
            }

            ++stats.mMismatchCount;
            if (stats.mFirstMismatchPlane == 0)
            {
                stats.mFirstMismatchPlane   = spec.mId;
                stats.mFirstMismatchPixel   = component / spec.mChannels;
                stats.mFirstMismatchChannel = static_cast<std::uint32_t>(component % spec.mChannels);
                stats.mFirstReference       = reference_value;
                stats.mFirstCandidate       = candidate_value;
                stats.mFirstTolerance       = spec.mTolerance;
            }
        }
    }
    stats.mMatch = stats.mMismatchCount == 0;
    return stats;
}

} // namespace LLRenderContract
