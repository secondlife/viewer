/**
 * @file lltextureuploaddiagnostic.cpp
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

#include "lltextureuploaddiagnostic.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace LLRenderContract
{
namespace
{

constexpr std::array<std::uint8_t, 8> ARTIFACT_MAGIC{ 'L', 'L', 'U', 'P', 'L', 'O', 'A', 'D' };
constexpr std::uint32_t RGBA8_WIRE_VALUE          = 1;
constexpr std::uint32_t TOP_LEFT_WIRE_VALUE       = 1;
constexpr std::uint32_t BOTTOM_LEFT_WIRE_VALUE    = 2;
constexpr std::uint32_t GENERATE_MIPS_WIRE_VALUE  = 1;
constexpr std::uint32_t UNDEFINED_WIRE_VALUE      = 1;
constexpr std::uint32_t TRANSFER_DEST_WIRE_VALUE  = 2;
constexpr std::uint32_t SHADER_READ_WIRE_VALUE    = 3;
constexpr std::array<std::uint32_t, TEXTURE_UPLOAD_MIP_LEVELS> MIP_WIDTHS{ 8, 4, 2 };
constexpr std::array<std::uint32_t, TEXTURE_UPLOAD_MIP_LEVELS> MIP_HEIGHTS{ 4, 2, 1 };
constexpr std::size_t ARTIFACT_CONTENT_SIZE = 472;
constexpr std::size_t ARTIFACT_SIZE         = ARTIFACT_CONTENT_SIZE + sizeof(std::uint64_t);

static_assert(ARTIFACT_SIZE == TEXTURE_UPLOAD_ARTIFACT_BYTE_SIZE);
static_assert(TEXTURE_UPLOAD_MIP_BYTE_OFFSETS[2] + TEXTURE_UPLOAD_MIP_BYTE_SIZES[2] == TEXTURE_UPLOAD_MIP_BYTE_COUNT);

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

void hashU32(std::uint64_t& hash, std::uint32_t value)
{
    hashByte(hash, static_cast<std::uint8_t>(value >> 24));
    hashByte(hash, static_cast<std::uint8_t>(value >> 16));
    hashByte(hash, static_cast<std::uint8_t>(value >> 8));
    hashByte(hash, static_cast<std::uint8_t>(value));
}

void hashU64(std::uint64_t& hash, std::uint64_t value)
{
    hashU32(hash, static_cast<std::uint32_t>(value >> 32));
    hashU32(hash, static_cast<std::uint32_t>(value));
}

std::uint64_t artifactChecksum(const std::uint8_t* bytes, std::size_t size)
{
    constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    std::uint64_t           hash             = FNV_OFFSET_BASIS;
    for (std::size_t index = 0; index < size; ++index)
    {
        hashByte(hash, bytes[index]);
    }
    return hash;
}

std::uint32_t floatBits(float value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
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

TextureUploadFixture makeTextureUploadFixture()
{
    TextureUploadFixture fixture;
    fixture.mScreenTriangle = { -1.f, 1.f, 0.f, 0.f, -1.f, -3.f, 0.f, 0.f, 3.f, 1.f, 0.f, 0.f };

    for (std::size_t top_row = 0; top_row < TEXTURE_UPLOAD_RESIDENT_HEIGHT; ++top_row)
    {
        const std::size_t row_start = top_row * TEXTURE_UPLOAD_ROW_PITCH;
        for (std::size_t x = 0; x < TEXTURE_UPLOAD_RESIDENT_WIDTH; ++x)
        {
            for (std::size_t channel = 0; channel < TEXTURE_UPLOAD_CHANNELS; ++channel)
            {
                fixture.mSourceRGBA8[row_start + x * TEXTURE_UPLOAD_CHANNELS + channel] =
                    static_cast<std::uint8_t>(5 + channel * 64 + top_row * 8 + x * 2);
            }
        }
        for (std::size_t padding = TEXTURE_UPLOAD_RESIDENT_WIDTH * TEXTURE_UPLOAD_CHANNELS;
             padding < TEXTURE_UPLOAD_ROW_PITCH; ++padding)
        {
            fixture.mSourceRGBA8[row_start + padding] =
                static_cast<std::uint8_t>(0xf0U + top_row * 4 + padding -
                                          TEXTURE_UPLOAD_RESIDENT_WIDTH * TEXTURE_UPLOAD_CHANNELS);
        }
    }

    for (std::size_t mip = 0; mip < TEXTURE_UPLOAD_MIP_LEVELS; ++mip)
    {
        const std::size_t begin = TEXTURE_UPLOAD_MIP_BYTE_OFFSETS[mip];
        const std::size_t size  = TEXTURE_UPLOAD_MIP_BYTE_SIZES[mip];
        for (std::size_t component = 0; component < size; ++component)
        {
            fixture.mOldMipRGBA8[begin + component] =
                static_cast<std::uint8_t>(1 + (mip * 73 + component * 31 + 17) % 251);
            fixture.mReplacementSentinelMipRGBA8[begin + component] =
                static_cast<std::uint8_t>(1 + (mip * 41 + component * 19 + 137) % 251);
        }
    }
    for (std::size_t component = 0; component < fixture.mOutputSentinelRGBA8.size(); ++component)
    {
        fixture.mOutputSentinelRGBA8[component] = static_cast<std::uint8_t>(1 + (component * 43 + 89) % 251);
    }
    return fixture;
}

std::uint64_t textureUploadFixtureFingerprint()
{
    constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    const TextureUploadFixture fixture       = makeTextureUploadFixture();
    std::uint64_t              hash          = FNV_OFFSET_BASIS;
    hashU32(hash, TEXTURE_UPLOAD_DIAGNOSTIC_FIXTURE_VERSION);
    hashU32(hash, fixture.mResidentExtent.mWidth);
    hashU32(hash, fixture.mResidentExtent.mHeight);
    hashU32(hash, fixture.mLogicalExtent.mWidth);
    hashU32(hash, fixture.mLogicalExtent.mHeight);
    hashU32(hash, fixture.mOutputExtent.mWidth);
    hashU32(hash, fixture.mOutputExtent.mHeight);
    hashU32(hash, TOP_LEFT_WIRE_VALUE);
    hashU32(hash, fixture.mSourceRowPitch);
    hashU64(hash, fixture.mPriorRevision);
    for (std::uint8_t value : fixture.mSourceRGBA8)
        hashByte(hash, value);
    for (float value : fixture.mScreenTriangle)
        hashU32(hash, floatBits(value));
    for (std::uint8_t value : fixture.mOldMipRGBA8)
        hashByte(hash, value);
    for (std::uint8_t value : fixture.mReplacementSentinelMipRGBA8)
        hashByte(hash, value);
    for (std::uint8_t value : fixture.mOutputSentinelRGBA8)
        hashByte(hash, value);
    return hash;
}

TextureUploadCase makeTextureUploadCase()
{
    TextureUploadCase          result;
    const TextureUploadFixture fixture = makeTextureUploadFixture();
    result.mPriorRevision              = fixture.mPriorRevision;
    result.mInputs.mFrame              = TEXTURE_UPLOAD_DIAGNOSTIC_FRAME;
    result.mInputs.mPixels.assign(fixture.mSourceRGBA8.begin(), fixture.mSourceRGBA8.end());
    auto frame = buildStreamingUploadFrame(result.mInputs);
    if (!frame)
    {
        std::terminate();
    }
    result.mFrame = std::move(*frame);
    return result;
}

TextureUploadArtifact makeTextureUploadArtifact()
{
    return {};
}

bool validateTextureUploadArtifact(const TextureUploadArtifact& artifact, std::string* error)
{
    clearError(error);
    if (artifact.mPriorRevision != TEXTURE_UPLOAD_PRIOR_REVISION || artifact.mRevision != TEXTURE_UPLOAD_REVISION ||
        artifact.mCompletionCount != 1 || artifact.mCompletedDestination != ImageHandle{ 11, 2 } ||
        artifact.mCompletedRevision != TEXTURE_UPLOAD_REVISION || artifact.mCompletedFrame != TEXTURE_UPLOAD_DIAGNOSTIC_FRAME ||
        artifact.mRetirementCount != 1 || artifact.mRetiredResource != ImageHandle{ 11, 1 } ||
        artifact.mRetirementFrame != artifact.mCompletedFrame || !artifact.mOldResolvableBefore || artifact.mOldResolvableAfter ||
        !artifact.mReplacementResolvableAfter)
    {
        return fail(error, "artifact revision, completion, retirement, or post-state metadata is not canonical");
    }
    for (std::size_t mip = 0; mip < artifact.mMipRGBA8.size(); ++mip)
    {
        if (artifact.mMipRGBA8[mip].size() != TEXTURE_UPLOAD_MIP_BYTE_SIZES[mip])
        {
            std::ostringstream message;
            message << "mip " << mip << " has " << artifact.mMipRGBA8[mip].size() << " bytes, expected "
                    << TEXTURE_UPLOAD_MIP_BYTE_SIZES[mip];
            return fail(error, message.str());
        }
    }
    if (artifact.mSampledRGBA8.size() != TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT)
    {
        std::ostringstream message;
        message << "sampled output has " << artifact.mSampledRGBA8.size() << " bytes, expected "
                << TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT;
        return fail(error, message.str());
    }
    return true;
}

bool encodeTextureUploadArtifact(const TextureUploadArtifact& artifact, std::vector<std::uint8_t>& encoded, std::string* error)
{
    clearError(error);
    if (!validateTextureUploadArtifact(artifact, error))
    {
        return false;
    }

    std::vector<std::uint8_t> result;
    result.reserve(ARTIFACT_SIZE);
    result.insert(result.end(), ARTIFACT_MAGIC.begin(), ARTIFACT_MAGIC.end());
    appendU32(result, TEXTURE_UPLOAD_ARTIFACT_SCHEMA_VERSION);
    appendU32(result, TEXTURE_UPLOAD_DIAGNOSTIC_FIXTURE_VERSION);
    appendU64(result, textureUploadFixtureFingerprint());
    appendU64(result, artifact.mCompletedFrame);
    appendU64(result, artifact.mPriorRevision);
    appendU64(result, artifact.mRevision);
    appendU32(result, 11);
    appendU32(result, 1);
    appendU32(result, 11);
    appendU32(result, 2);
    appendU32(result, 0);
    appendU32(result, 0);
    appendU32(result, 0);
    appendU32(result, 0);
    appendU32(result, TEXTURE_UPLOAD_RESIDENT_WIDTH);
    appendU32(result, TEXTURE_UPLOAD_RESIDENT_HEIGHT);
    appendU32(result, TEXTURE_UPLOAD_RESIDENT_WIDTH);
    appendU32(result, TEXTURE_UPLOAD_RESIDENT_HEIGHT);
    appendU32(result, TEXTURE_UPLOAD_LOGICAL_WIDTH);
    appendU32(result, TEXTURE_UPLOAD_LOGICAL_HEIGHT);
    appendU32(result, TEXTURE_UPLOAD_RESIDENT_DISCARD);
    appendU32(result, RGBA8_WIRE_VALUE);
    appendU32(result, RGBA8_WIRE_VALUE);
    appendU32(result, TOP_LEFT_WIRE_VALUE);
    appendU32(result, BOTTOM_LEFT_WIRE_VALUE);
    appendU32(result, TEXTURE_UPLOAD_ROW_PITCH);
    appendU32(result, GENERATE_MIPS_WIRE_VALUE);
    appendU32(result, UNDEFINED_WIRE_VALUE);
    appendU32(result, TRANSFER_DEST_WIRE_VALUE);
    appendU32(result, SHADER_READ_WIRE_VALUE);
    appendU32(result, TEXTURE_UPLOAD_MIP_LEVELS);
    appendU32(result, TEXTURE_UPLOAD_OUTPUT_WIDTH);
    appendU32(result, TEXTURE_UPLOAD_OUTPUT_HEIGHT);
    appendU32(result, RGBA8_WIRE_VALUE);
    appendU32(result, TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT);
    appendU32(result, artifact.mCompletionCount);
    appendU32(result, artifact.mCompletedDestination.mIndex);
    appendU32(result, artifact.mCompletedDestination.mGeneration);
    appendU64(result, artifact.mCompletedRevision);
    appendU64(result, artifact.mCompletedFrame);
    appendU32(result, artifact.mRetirementCount);
    appendU32(result, artifact.mRetiredResource.mIndex);
    appendU32(result, artifact.mRetiredResource.mGeneration);
    appendU64(result, artifact.mRetirementFrame);
    appendU32(result, artifact.mOldResolvableBefore ? 1 : 0);
    appendU32(result, artifact.mOldResolvableAfter ? 1 : 0);
    appendU32(result, artifact.mReplacementResolvableAfter ? 1 : 0);

    for (std::size_t mip = 0; mip < TEXTURE_UPLOAD_MIP_LEVELS; ++mip)
    {
        appendU32(result, static_cast<std::uint32_t>(mip));
        appendU32(result, MIP_WIDTHS[mip]);
        appendU32(result, MIP_HEIGHTS[mip]);
        appendU32(result, static_cast<std::uint32_t>(artifact.mMipRGBA8[mip].size()));
        result.insert(result.end(), artifact.mMipRGBA8[mip].begin(), artifact.mMipRGBA8[mip].end());
    }
    result.insert(result.end(), artifact.mSampledRGBA8.begin(), artifact.mSampledRGBA8.end());

    if (result.size() != ARTIFACT_CONTENT_SIZE)
    {
        return fail(error, "artifact encoder produced an unexpected byte count");
    }
    appendU64(result, artifactChecksum(result.data(), result.size()));
    if (result.size() != ARTIFACT_SIZE)
    {
        return fail(error, "artifact encoder produced an unexpected checksum byte count");
    }
    encoded = std::move(result);
    return true;
}

bool decodeTextureUploadArtifact(const std::vector<std::uint8_t>& encoded, TextureUploadArtifact& artifact, std::string* error)
{
    clearError(error);
    if (encoded.size() != ARTIFACT_SIZE)
    {
        std::ostringstream message;
        message << "artifact has " << encoded.size() << " bytes, expected " << ARTIFACT_SIZE;
        return fail(error, message.str());
    }

    ArtifactReader checksum_reader(encoded);
    std::array<std::uint8_t, ARTIFACT_CONTENT_SIZE> content{};
    std::uint64_t                                   stored_checksum = 0;
    if (!checksum_reader.readBytes(content.data(), content.size()) || !checksum_reader.readU64(stored_checksum))
    {
        return fail(error, "artifact checksum is truncated");
    }
    const std::uint64_t computed_checksum = artifactChecksum(content.data(), content.size());
    if (stored_checksum != computed_checksum)
    {
        return fail(error, "artifact checksum is invalid");
    }

    ArtifactReader                                  reader(encoded);
    std::array<std::uint8_t, ARTIFACT_MAGIC.size()> magic{};
    if (!reader.readBytes(magic.data(), magic.size()) || magic != ARTIFACT_MAGIC)
    {
        return fail(error, "artifact magic is invalid");
    }

    if (!expectedU32(reader, TEXTURE_UPLOAD_ARTIFACT_SCHEMA_VERSION, "schema version", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_DIAGNOSTIC_FIXTURE_VERSION, "fixture version", error) ||
        !expectedU64(reader, textureUploadFixtureFingerprint(), "fixture fingerprint", error) ||
        !expectedU64(reader, TEXTURE_UPLOAD_DIAGNOSTIC_FRAME, "frame", error) ||
        !expectedU64(reader, TEXTURE_UPLOAD_PRIOR_REVISION, "prior revision", error) ||
        !expectedU64(reader, TEXTURE_UPLOAD_REVISION, "revision", error) ||
        !expectedU32(reader, 11, "old image index", error) || !expectedU32(reader, 1, "old image generation", error) ||
        !expectedU32(reader, 11, "replacement image index", error) ||
        !expectedU32(reader, 2, "replacement image generation", error) ||
        !expectedU32(reader, 0, "base mip", error) || !expectedU32(reader, 0, "array layer", error) ||
        !expectedU32(reader, 0, "upload x offset", error) || !expectedU32(reader, 0, "upload y offset", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_RESIDENT_WIDTH, "upload width", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_RESIDENT_HEIGHT, "upload height", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_RESIDENT_WIDTH, "resident width", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_RESIDENT_HEIGHT, "resident height", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_LOGICAL_WIDTH, "logical width", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_LOGICAL_HEIGHT, "logical height", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_RESIDENT_DISCARD, "resident discard", error) ||
        !expectedU32(reader, RGBA8_WIRE_VALUE, "source format", error) ||
        !expectedU32(reader, RGBA8_WIRE_VALUE, "destination format", error) ||
        !expectedU32(reader, TOP_LEFT_WIRE_VALUE, "source row origin", error) ||
        !expectedU32(reader, BOTTOM_LEFT_WIRE_VALUE, "artifact row origin", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_ROW_PITCH, "source row pitch", error) ||
        !expectedU32(reader, GENERATE_MIPS_WIRE_VALUE, "mip generation", error) ||
        !expectedU32(reader, UNDEFINED_WIRE_VALUE, "before state", error) ||
        !expectedU32(reader, TRANSFER_DEST_WIRE_VALUE, "during state", error) ||
        !expectedU32(reader, SHADER_READ_WIRE_VALUE, "after state", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_MIP_LEVELS, "mip count", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_OUTPUT_WIDTH, "sample width", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_OUTPUT_HEIGHT, "sample height", error) ||
        !expectedU32(reader, RGBA8_WIRE_VALUE, "sample format", error) ||
        !expectedU32(reader, TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT, "sample byte count", error) ||
        !expectedU32(reader, 1, "completion count", error) || !expectedU32(reader, 11, "completed image index", error) ||
        !expectedU32(reader, 2, "completed image generation", error) ||
        !expectedU64(reader, TEXTURE_UPLOAD_REVISION, "completed revision", error) ||
        !expectedU64(reader, TEXTURE_UPLOAD_DIAGNOSTIC_FRAME, "completed frame", error) ||
        !expectedU32(reader, 1, "retirement count", error) || !expectedU32(reader, 11, "retired image index", error) ||
        !expectedU32(reader, 1, "retired image generation", error) ||
        !expectedU64(reader, TEXTURE_UPLOAD_DIAGNOSTIC_FRAME, "retirement frame", error) ||
        !expectedU32(reader, 1, "old resolvable before", error) || !expectedU32(reader, 0, "old resolvable after", error) ||
        !expectedU32(reader, 1, "replacement resolvable after", error))
    {
        return false;
    }

    TextureUploadArtifact result        = makeTextureUploadArtifact();
    result.mPriorRevision               = TEXTURE_UPLOAD_PRIOR_REVISION;
    result.mRevision                    = TEXTURE_UPLOAD_REVISION;
    result.mCompletionCount             = 1;
    result.mCompletedDestination        = ImageHandle{ 11, 2 };
    result.mCompletedRevision           = TEXTURE_UPLOAD_REVISION;
    result.mCompletedFrame              = TEXTURE_UPLOAD_DIAGNOSTIC_FRAME;
    result.mRetirementCount             = 1;
    result.mRetiredResource             = ImageHandle{ 11, 1 };
    result.mRetirementFrame             = TEXTURE_UPLOAD_DIAGNOSTIC_FRAME;
    result.mOldResolvableBefore         = true;
    result.mOldResolvableAfter          = false;
    result.mReplacementResolvableAfter = true;
    for (std::size_t mip = 0; mip < TEXTURE_UPLOAD_MIP_LEVELS; ++mip)
    {
        const std::string prefix = "mip " + std::to_string(mip) + ' ';
        if (!expectedU32(reader, static_cast<std::uint32_t>(mip), prefix + "level", error) ||
            !expectedU32(reader, MIP_WIDTHS[mip], prefix + "width", error) ||
            !expectedU32(reader, MIP_HEIGHTS[mip], prefix + "height", error) ||
            !expectedU32(reader, static_cast<std::uint32_t>(TEXTURE_UPLOAD_MIP_BYTE_SIZES[mip]), prefix + "byte count", error))
        {
            return false;
        }
        result.mMipRGBA8[mip].resize(TEXTURE_UPLOAD_MIP_BYTE_SIZES[mip]);
        if (!reader.readBytes(result.mMipRGBA8[mip].data(), result.mMipRGBA8[mip].size()))
        {
            return fail(error, prefix + "payload is truncated");
        }
    }
    result.mSampledRGBA8.resize(TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT);
    if (!reader.readBytes(result.mSampledRGBA8.data(), result.mSampledRGBA8.size()))
    {
        return fail(error, "sampled payload is truncated");
    }
    std::uint64_t decoded_checksum = 0;
    if (!reader.readU64(decoded_checksum) || decoded_checksum != stored_checksum)
    {
        return fail(error, "artifact checksum is inconsistent");
    }
    if (reader.offset() != encoded.size())
    {
        return fail(error, "artifact has trailing data");
    }
    if (!validateTextureUploadArtifact(result, error))
    {
        return false;
    }
    artifact = std::move(result);
    return true;
}

bool writeTextureUploadArtifact(const std::filesystem::path& destination, const TextureUploadArtifact& artifact, std::string* error)
{
    clearError(error);
    if (destination.empty() || destination.filename().empty())
    {
        return fail(error, "artifact destination must name a file");
    }

    std::vector<std::uint8_t> encoded;
    if (!encodeTextureUploadArtifact(artifact, encoded, error))
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
            return fail(error, "cannot create artifact temporary file: " +
                                   std::error_code(open_error, std::generic_category()).message());
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
        if (error)
        {
            *error = "artifact published but its temporary link could not be removed: " + file_error.message();
        }
        return true;
    }
    return true;
}

bool readTextureUploadArtifact(const std::filesystem::path& source, TextureUploadArtifact& artifact, std::string* error)
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
    return decodeTextureUploadArtifact(encoded, artifact, error);
}

TextureUploadComparisonStats compareTextureUploadArtifacts(const TextureUploadArtifact& reference,
                                                            const TextureUploadArtifact& candidate)
{
    TextureUploadComparisonStats stats;
    std::string                  validation_error;
    if (!validateTextureUploadArtifact(reference, &validation_error))
    {
        stats.mError = "reference " + validation_error;
        return stats;
    }
    if (!validateTextureUploadArtifact(candidate, &validation_error))
    {
        stats.mError = "candidate " + validation_error;
        return stats;
    }

    stats.mComparable = true;
    auto compare_plane = [&stats](const std::vector<std::uint8_t>& left, const std::vector<std::uint8_t>& right,
                                  std::uint32_t plane)
    {
        for (std::size_t byte = 0; byte < left.size(); ++byte)
        {
            if (left[byte] == right[byte])
            {
                continue;
            }
            ++stats.mMismatchCount;
            if (stats.mFirstMismatchPlane == 0)
            {
                stats.mFirstMismatchPlane = plane;
                stats.mFirstMismatchByte  = byte;
                stats.mFirstReference     = left[byte];
                stats.mFirstCandidate     = right[byte];
            }
        }
    };

    for (std::size_t mip = 0; mip < TEXTURE_UPLOAD_MIP_LEVELS; ++mip)
    {
        compare_plane(reference.mMipRGBA8[mip], candidate.mMipRGBA8[mip], static_cast<std::uint32_t>(mip + 1));
        stats.mComparedMipBytes += reference.mMipRGBA8[mip].size();
    }
    compare_plane(reference.mSampledRGBA8, candidate.mSampledRGBA8, TEXTURE_UPLOAD_MIP_LEVELS + 1);
    stats.mComparedSampleBytes = reference.mSampledRGBA8.size();
    stats.mMatch               = stats.mMismatchCount == 0;
    return stats;
}

} // namespace LLRenderContract
