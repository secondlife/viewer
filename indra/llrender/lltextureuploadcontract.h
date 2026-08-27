/**
 * @file lltextureuploadcontract.h
 * @brief Backend-neutral description of one streamed texture replacement.
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

#ifndef LL_LLTEXTUREUPLOADCONTRACT_H
#define LL_LLTEXTUREUPLOADCONTRACT_H

#include "llrendercontract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace LLRenderContract
{

inline constexpr std::uint32_t TEXTURE_UPLOAD_RESIDENT_WIDTH  = 8;
inline constexpr std::uint32_t TEXTURE_UPLOAD_RESIDENT_HEIGHT = 4;
inline constexpr std::uint32_t TEXTURE_UPLOAD_LOGICAL_WIDTH   = 32;
inline constexpr std::uint32_t TEXTURE_UPLOAD_LOGICAL_HEIGHT  = 16;
inline constexpr std::uint32_t TEXTURE_UPLOAD_OUTPUT_WIDTH    = 4;
inline constexpr std::uint32_t TEXTURE_UPLOAD_OUTPUT_HEIGHT   = 2;
inline constexpr std::uint32_t TEXTURE_UPLOAD_MIP_LEVELS      = 3;
inline constexpr std::uint32_t TEXTURE_UPLOAD_CHANNELS        = 4;
inline constexpr std::uint32_t TEXTURE_UPLOAD_RESIDENT_DISCARD = 2;
inline constexpr std::uint32_t TEXTURE_UPLOAD_ROW_PITCH        = 36;

inline constexpr std::size_t TEXTURE_UPLOAD_SOURCE_BYTE_COUNT =
    static_cast<std::size_t>(TEXTURE_UPLOAD_ROW_PITCH) * TEXTURE_UPLOAD_RESIDENT_HEIGHT;
inline constexpr std::array<std::size_t, TEXTURE_UPLOAD_MIP_LEVELS> TEXTURE_UPLOAD_MIP_BYTE_OFFSETS{ 0, 128, 160 };
inline constexpr std::array<std::size_t, TEXTURE_UPLOAD_MIP_LEVELS> TEXTURE_UPLOAD_MIP_BYTE_SIZES{ 128, 32, 8 };
inline constexpr std::size_t TEXTURE_UPLOAD_MIP_BYTE_COUNT = 168;
inline constexpr std::size_t TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT =
    static_cast<std::size_t>(TEXTURE_UPLOAD_OUTPUT_WIDTH) * TEXTURE_UPLOAD_OUTPUT_HEIGHT * TEXTURE_UPLOAD_CHANNELS;

inline constexpr std::uint64_t TEXTURE_UPLOAD_PRIOR_REVISION = 22;
inline constexpr std::uint64_t TEXTURE_UPLOAD_REVISION       = 23;

struct StreamingUploadHandles
{
    BufferHandle   mScreenTriangle{ 1, 1 };
    ImageHandle    mOldImage{ 11, 1 };
    ImageHandle    mReplacementImage{ 11, 2 };
    ImageHandle    mOutput{ 12, 1 };
    SamplerHandle  mSampler{ 1, 1 };
    PipelineHandle mPipeline{ 1, 1 };
    PassId         mPass{ 1 };

    friend constexpr bool operator==(const StreamingUploadHandles&, const StreamingUploadHandles&) = default;
};

struct StreamingUploadInputs
{
    std::uint64_t         mFrame = 0;
    StreamingUploadHandles mHandles;
    std::uint64_t         mRevision = TEXTURE_UPLOAD_REVISION;
    ImageSubresource      mSubresource;
    Offset2D              mOffset;
    Extent2D              mExtent{ TEXTURE_UPLOAD_RESIDENT_WIDTH, TEXTURE_UPLOAD_RESIDENT_HEIGHT };
    Extent2D              mLogicalExtent{ TEXTURE_UPLOAD_LOGICAL_WIDTH, TEXTURE_UPLOAD_LOGICAL_HEIGHT };
    std::uint32_t         mResidentDiscard = TEXTURE_UPLOAD_RESIDENT_DISCARD;
    PixelFormat           mSourceFormat    = PixelFormat::RGBA8Unorm;
    std::uint32_t         mRowPitch        = TEXTURE_UPLOAD_ROW_PITCH;
    RowOrigin             mRowOrigin       = RowOrigin::TopLeft;
    MipGeneration         mMipGeneration   = MipGeneration::GenerateRemaining;
    std::vector<std::uint8_t> mPixels;
    ImageState            mBefore = ImageState::Undefined;
    ImageState            mDuring = ImageState::TransferDestination;
    ImageState            mAfter  = ImageState::ShaderRead;
};

// Copies the caller's pixels into storage owned by the returned frame.
std::optional<FrameSnapshot> buildStreamingUploadFrame(const StreamingUploadInputs& inputs);

// Accepts only the fixed streaming-upload packet shape and returns owned pixels.
std::optional<StreamingUploadInputs> decodeStreamingUploadFrame(const FrameSnapshot& frame);

} // namespace LLRenderContract

#endif // LL_LLTEXTUREUPLOADCONTRACT_H
