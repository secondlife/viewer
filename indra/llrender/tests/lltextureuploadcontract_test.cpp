/**
 * @file lltextureuploadcontract_test.cpp
 * @brief Tests for the backend-neutral streaming-upload packet.
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

#include "lltextureuploadcontract.h"
#include "lltut.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

namespace
{
using namespace LLRenderContract;

StreamingUploadInputs inputs()
{
    StreamingUploadInputs result;
    result.mFrame = 7;
    result.mPixels.reserve(TEXTURE_UPLOAD_SOURCE_BYTE_COUNT);
    for (std::size_t byte = 0; byte < TEXTURE_UPLOAD_SOURCE_BYTE_COUNT; ++byte)
    {
        result.mPixels.push_back(static_cast<std::uint8_t>((byte * 37 + 19) % 256));
    }
    return result;
}

FrameSnapshot frame()
{
    return *buildStreamingUploadFrame(inputs());
}

} // namespace

namespace tut
{

struct texture_upload_contract_test
{
};

using texture_upload_contract_test_group  = test_group<texture_upload_contract_test>;
using texture_upload_contract_test_object = texture_upload_contract_test_group::object;
texture_upload_contract_test_group texture_upload_contract_tests("texture upload contract");

template<>
template<>
void texture_upload_contract_test_object::test<1>()
{
    StreamingUploadInputs original = inputs();
    const std::uint8_t     first    = original.mPixels.front();
    auto                   built    = buildStreamingUploadFrame(original);
    ensure("canonical streaming upload builds", built.has_value());
    original.mPixels.front() ^= 0xffU;

    auto decoded = decodeStreamingUploadFrame(*built);
    ensure("canonical streaming upload decodes", decoded.has_value());
    ensure("builder owns source pixels independently of the caller",
           decoded->mPixels.front() == first && decoded->mPixels != original.mPixels);
    decoded->mPixels.front() ^= 0xffU;
    auto decoded_again = decodeStreamingUploadFrame(*built);
    ensure("decoder returns its own pixel vector", decoded_again && decoded_again->mPixels.front() == first);
    ensure("request preserves padded top-left rows",
           decoded_again->mRowOrigin == RowOrigin::TopLeft && decoded_again->mRowPitch == TEXTURE_UPLOAD_ROW_PITCH &&
               decoded_again->mPixels.size() == TEXTURE_UPLOAD_SOURCE_BYTE_COUNT);
    ensure("old and replacement generations share one logical index",
           decoded_again->mHandles.mOldImage.mIndex == decoded_again->mHandles.mReplacementImage.mIndex &&
               decoded_again->mHandles.mReplacementImage.mGeneration == decoded_again->mHandles.mOldImage.mGeneration + 1);
    ensure("only the old generation retires after this frame",
           built->mReleases.size() == 1 && std::holds_alternative<ImageHandle>(built->mReleases[0].mResource) &&
               std::get<ImageHandle>(built->mReleases[0].mResource) == decoded_again->mHandles.mOldImage &&
               built->mReleases[0].mFrame == built->mFrame);
}

template<>
template<>
void texture_upload_contract_test_object::test<2>()
{
    StreamingUploadInputs value = inputs();
    value.mFrame                 = 0;
    ensure("zero frame is rejected", !buildStreamingUploadFrame(value));

    value                                           = inputs();
    value.mHandles.mReplacementImage.mGeneration += 1;
    ensure("non-consecutive replacement generation is rejected", !buildStreamingUploadFrame(value));

    value                                  = inputs();
    value.mHandles.mOutput.mIndex          = value.mHandles.mOldImage.mIndex;
    value.mHandles.mOutput.mGeneration    += 5;
    ensure("output cannot alias the streamed image index", !buildStreamingUploadFrame(value));

    value           = inputs();
    value.mRevision = TEXTURE_UPLOAD_REVISION - 1;
    ensure("the frozen revision is rejected when changed", !buildStreamingUploadFrame(value));

    value                              = inputs();
    value.mSubresource.mMipLevel       = 1;
    ensure("only base mip upload is accepted", !buildStreamingUploadFrame(value));

    value             = inputs();
    value.mOffset.mX  = 1;
    ensure("partial destination offsets are outside this slice", !buildStreamingUploadFrame(value));

    value                 = inputs();
    value.mExtent.mWidth -= 1;
    ensure("partial upload extents are outside this slice", !buildStreamingUploadFrame(value));

    value                          = inputs();
    value.mLogicalExtent.mHeight >>= 1;
    ensure("logical extent is frozen", !buildStreamingUploadFrame(value));

    value                  = inputs();
    value.mResidentDiscard = 1;
    ensure("resident discard is frozen", !buildStreamingUploadFrame(value));

    value               = inputs();
    value.mSourceFormat = PixelFormat::RGBA8Srgb;
    ensure("source format is frozen", !buildStreamingUploadFrame(value));

    value             = inputs();
    value.mRowPitch  -= 1;
    ensure("padded row pitch is frozen", !buildStreamingUploadFrame(value));

    value            = inputs();
    value.mRowOrigin = RowOrigin::BottomLeft;
    ensure("source row origin is frozen", !buildStreamingUploadFrame(value));

    value                = inputs();
    value.mMipGeneration = MipGeneration::Disabled;
    ensure("remaining mips must be generated", !buildStreamingUploadFrame(value));

    value = inputs();
    value.mPixels.pop_back();
    ensure("short owned pixel range is rejected", !buildStreamingUploadFrame(value));

    value         = inputs();
    value.mBefore = ImageState::ShaderRead;
    ensure("before state is frozen", !buildStreamingUploadFrame(value));

    value         = inputs();
    value.mDuring = ImageState::ColorAttachment;
    ensure("during state is frozen", !buildStreamingUploadFrame(value));

    value        = inputs();
    value.mAfter = ImageState::ColorAttachment;
    ensure("after state is frozen", !buildStreamingUploadFrame(value));
}

template<>
template<>
void texture_upload_contract_test_object::test<3>()
{
    FrameSnapshot packet = frame();
    std::swap(packet.mImages[0], packet.mImages[1]);
    ensure("resource declaration order is canonical", !decodeStreamingUploadFrame(packet));

    packet = frame();
    packet.mImages[1].mMipLevels = 2;
    ensure("replacement mip count is canonical", !decodeStreamingUploadFrame(packet));

    packet = frame();
    packet.mImages[2].mLifetime = ResourceLifetime::Persistent;
    ensure("sample target ownership is canonical", !decodeStreamingUploadFrame(packet));

    packet = frame();
    packet.mSamplers[0].mAddressU = AddressMode::Repeat;
    ensure("sampler address state is canonical", !decodeStreamingUploadFrame(packet));

    packet = frame();
    packet.mPipelines[0].mProgram.mName = "other";
    ensure("program identity is canonical", !decodeStreamingUploadFrame(packet));

    packet = frame();
    packet.mPasses[0].mViewport.mWidth -= 1.f;
    ensure("sample viewport is canonical", !decodeStreamingUploadFrame(packet));

    packet = frame();
    std::get<Draw>(packet.mPasses[0].mDraws[0]).mVertexCount = 4;
    ensure("sample draw shape is canonical", !decodeStreamingUploadFrame(packet));

    packet = frame();
    packet.mReleases.clear();
    ensure("old generation release is required", !decodeStreamingUploadFrame(packet));

    packet = frame();
    packet.mReleases[0].mResource = packet.mImages[1].mHandle;
    ensure("replacement generation cannot be released", !decodeStreamingUploadFrame(packet));

    packet = frame();
    ++packet.mReleases[0].mFrame;
    ensure("release frame is exact", !decodeStreamingUploadFrame(packet));
}

template<>
template<>
void texture_upload_contract_test_object::test<4>()
{
    FrameSnapshot packet = frame();
    packet.mUploads[0].mPixels.mStorage.reset();
    ensure("unowned upload storage is rejected", !decodeStreamingUploadFrame(packet));

    packet                              = frame();
    packet.mUploads[0].mPixels.mOffset = 1;
    packet.mUploads[0].mPixels.mSize  -= 1;
    ensure("non-canonical byte range offset is rejected", !decodeStreamingUploadFrame(packet));

    packet = frame();
    auto larger = std::make_shared<std::vector<std::uint8_t>>(TEXTURE_UPLOAD_SOURCE_BYTE_COUNT + 1, 0);
    packet.mUploads[0].mPixels = { larger, 0, TEXTURE_UPLOAD_SOURCE_BYTE_COUNT };
    ensure("extra caller storage cannot hide outside the packet range", !decodeStreamingUploadFrame(packet));

    packet = frame();
    packet.mUploads[0].mPixels.mSize -= 1;
    ensure("short packet range is rejected", !decodeStreamingUploadFrame(packet));
}

} // namespace tut
