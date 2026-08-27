/**
 * @file lltextureuploadcontract.cpp
 * @brief Pure builder and decoder for one streamed texture replacement.
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

#include "lltextureuploadcontract.h"

#include <limits>
#include <memory>
#include <variant>

namespace LLRenderContract
{
namespace
{

constexpr const char*           TEXTURE_UPLOAD_PROGRAM = "contract.sample-texture";
constexpr const char*           TEXTURE_UPLOAD_PASS    = "sample streamed image";
constexpr Extent2D              RESIDENT_EXTENT{ TEXTURE_UPLOAD_RESIDENT_WIDTH, TEXTURE_UPLOAD_RESIDENT_HEIGHT };
constexpr Extent2D              LOGICAL_EXTENT{ TEXTURE_UPLOAD_LOGICAL_WIDTH, TEXTURE_UPLOAD_LOGICAL_HEIGHT };
constexpr Extent2D              OUTPUT_EXTENT{ TEXTURE_UPLOAD_OUTPUT_WIDTH, TEXTURE_UPLOAD_OUTPUT_HEIGHT };
constexpr ImageSubresourceRange REPLACEMENT_RANGE{ 0, TEXTURE_UPLOAD_MIP_LEVELS, 0, 1 };

bool sameExtent(Extent2D left, Extent2D right)
{
    return left.mWidth == right.mWidth && left.mHeight == right.mHeight;
}

bool sameOffset(Offset2D left, Offset2D right)
{
    return left.mX == right.mX && left.mY == right.mY;
}

bool sameRange(const ImageSubresourceRange& left, const ImageSubresourceRange& right)
{
    return left.mBaseMipLevel == right.mBaseMipLevel && left.mMipLevelCount == right.mMipLevelCount &&
           left.mBaseArrayLayer == right.mBaseArrayLayer && left.mArrayLayerCount == right.mArrayLayerCount;
}

bool zeroClear(const ClearColor& clear)
{
    return clear.mRed == 0.f && clear.mGreen == 0.f && clear.mBlue == 0.f && clear.mAlpha == 0.f;
}

bool validHandles(const StreamingUploadHandles& handles)
{
    return handles.mScreenTriangle && handles.mOldImage && handles.mReplacementImage && handles.mOutput && handles.mSampler &&
           handles.mPipeline && handles.mPass && handles.mOldImage.mIndex == handles.mReplacementImage.mIndex &&
           handles.mOldImage.mGeneration != std::numeric_limits<std::uint32_t>::max() &&
           handles.mReplacementImage.mGeneration == handles.mOldImage.mGeneration + 1 &&
           handles.mOutput.mIndex != handles.mOldImage.mIndex;
}

bool canonicalRequest(const StreamingUploadInputs& inputs)
{
    return inputs.mFrame != 0 && validHandles(inputs.mHandles) && inputs.mRevision == TEXTURE_UPLOAD_REVISION &&
           inputs.mSubresource == ImageSubresource{} && sameOffset(inputs.mOffset, {}) && sameExtent(inputs.mExtent, RESIDENT_EXTENT) &&
           sameExtent(inputs.mLogicalExtent, LOGICAL_EXTENT) && inputs.mResidentDiscard == TEXTURE_UPLOAD_RESIDENT_DISCARD &&
           inputs.mSourceFormat == PixelFormat::RGBA8Unorm && inputs.mRowPitch == TEXTURE_UPLOAD_ROW_PITCH &&
           inputs.mRowOrigin == RowOrigin::TopLeft && inputs.mMipGeneration == MipGeneration::GenerateRemaining &&
           inputs.mPixels.size() == TEXTURE_UPLOAD_SOURCE_BYTE_COUNT && inputs.mBefore == ImageState::Undefined &&
           inputs.mDuring == ImageState::TransferDestination && inputs.mAfter == ImageState::ShaderRead;
}

ImageResource image(ImageHandle handle, Extent2D extent, std::uint32_t mip_levels, ResourceLifetime lifetime)
{
    ImageResource result;
    result.mHandle    = handle;
    result.mExtent    = extent;
    result.mMipLevels = mip_levels;
    result.mFormat    = PixelFormat::RGBA8Unorm;
    result.mLifetime  = lifetime;
    return result;
}

bool canonicalImage(const ImageResource& resource, ImageHandle handle, Extent2D extent, std::uint32_t mip_levels,
                    ResourceLifetime lifetime)
{
    return resource.mHandle == handle && sameExtent(resource.mExtent, extent) && resource.mMipLevels == mip_levels &&
           resource.mArrayLayers == 1 && resource.mSamples == 1 && resource.mFormat == PixelFormat::RGBA8Unorm &&
           resource.mLifetime == lifetime;
}

bool canonicalSampledAccess(const ImageAccess& access, ImageHandle handle)
{
    return access.mImage == handle && sameRange(access.mRange, REPLACEMENT_RANGE) &&
           access.mKind == ImageAccessKind::SampledRead && access.mBefore == ImageState::ShaderRead &&
           access.mDuring == ImageState::ShaderRead && access.mAfter == ImageState::ShaderRead;
}

bool canonicalOutputAccess(const ImageAccess& access, ImageHandle handle)
{
    return access.mImage == handle && sameRange(access.mRange, {}) && access.mKind == ImageAccessKind::ColorAttachmentWrite &&
           access.mBefore == ImageState::Undefined && access.mDuring == ImageState::ColorAttachment &&
           access.mAfter == ImageState::ShaderRead;
}

ByteRange ownedPixels(const std::vector<std::uint8_t>& pixels)
{
    auto storage = std::make_shared<std::vector<std::uint8_t>>(pixels);
    return { std::move(storage), 0, pixels.size() };
}

} // namespace

std::optional<FrameSnapshot> buildStreamingUploadFrame(const StreamingUploadInputs& inputs)
{
    if (!canonicalRequest(inputs))
    {
        return std::nullopt;
    }

    const StreamingUploadHandles& handles = inputs.mHandles;
    FrameSnapshot                  frame;
    frame.mFrame   = inputs.mFrame;
    frame.mBuffers = { { handles.mScreenTriangle, 48, ResourceLifetime::Persistent } };
    frame.mImages  = { image(handles.mOldImage, RESIDENT_EXTENT, TEXTURE_UPLOAD_MIP_LEVELS, ResourceLifetime::Persistent),
                       image(handles.mReplacementImage, RESIDENT_EXTENT, TEXTURE_UPLOAD_MIP_LEVELS,
                             ResourceLifetime::Persistent),
                       image(handles.mOutput, OUTPUT_EXTENT, 1, ResourceLifetime::External) };

    SamplerResource sampler;
    sampler.mHandle     = handles.mSampler;
    sampler.mMinFilter  = Filter::Linear;
    sampler.mMagFilter  = Filter::Linear;
    sampler.mMipFilter  = MipFilter::Linear;
    sampler.mAddressU   = AddressMode::Clamp;
    sampler.mAddressV   = AddressMode::Clamp;
    frame.mSamplers.push_back(sampler);

    PipelineResource pipeline;
    pipeline.mHandle        = handles.mPipeline;
    pipeline.mProgram       = { TEXTURE_UPLOAD_PROGRAM, 0 };
    pipeline.mCullMode      = CullMode::Disabled;
    pipeline.mDepthCompare  = CompareOp::LessOrEqual;
    pipeline.mColorTargets  = { { PixelFormat::RGBA8Unorm, false, 0xf } };
    pipeline.mVertexBindings.push_back({ 0, 16 });
    pipeline.mVertexAttributes.push_back({ VertexSemantic::Position, VertexFormat::Float3, 0, 0 });
    pipeline.mSampledImageBindings = { 0 };
    frame.mPipelines.push_back(std::move(pipeline));

    TextureUpload upload;
    upload.mDestination     = handles.mReplacementImage;
    upload.mRevision        = inputs.mRevision;
    upload.mSubresource     = inputs.mSubresource;
    upload.mOffset          = inputs.mOffset;
    upload.mExtent          = inputs.mExtent;
    upload.mLogicalExtent   = inputs.mLogicalExtent;
    upload.mResidentDiscard = inputs.mResidentDiscard;
    upload.mSourceFormat    = inputs.mSourceFormat;
    upload.mRowPitch        = inputs.mRowPitch;
    upload.mRowOrigin       = inputs.mRowOrigin;
    upload.mMipGeneration   = inputs.mMipGeneration;
    upload.mPixels          = ownedPixels(inputs.mPixels);
    upload.mBefore          = inputs.mBefore;
    upload.mDuring          = inputs.mDuring;
    upload.mAfter           = inputs.mAfter;
    frame.mUploads.push_back(std::move(upload));

    RenderPass pass;
    pass.mId       = handles.mPass;
    pass.mLabel    = TEXTURE_UPLOAD_PASS;
    pass.mExtent   = OUTPUT_EXTENT;
    pass.mViewport = { 0.f, 0.f, static_cast<float>(TEXTURE_UPLOAD_OUTPUT_WIDTH),
                       static_cast<float>(TEXTURE_UPLOAD_OUTPUT_HEIGHT), 0.f, 1.f };
    pass.mScissor = { 0, 0, TEXTURE_UPLOAD_OUTPUT_WIDTH, TEXTURE_UPLOAD_OUTPUT_HEIGHT };
    pass.mBufferAccesses.push_back({ handles.mScreenTriangle, BufferAccessKind::VertexRead });
    pass.mImageAccesses = {
        { handles.mReplacementImage, REPLACEMENT_RANGE, ImageAccessKind::SampledRead, ImageState::ShaderRead,
          ImageState::ShaderRead, ImageState::ShaderRead },
        { handles.mOutput, {}, ImageAccessKind::ColorAttachmentWrite, ImageState::Undefined, ImageState::ColorAttachment,
          ImageState::ShaderRead }
    };
    pass.mColorAttachments.push_back({ handles.mOutput, {}, LoadOp::DontCare, StoreOp::Store, {} });

    Draw draw;
    draw.mResources.mPipeline = handles.mPipeline;
    draw.mResources.mVertexBuffers.push_back({ 0, handles.mScreenTriangle, 0 });
    draw.mResources.mSampledImages.push_back({ 0, handles.mReplacementImage, REPLACEMENT_RANGE, handles.mSampler });
    draw.mVertexCount = 3;
    pass.mDraws.emplace_back(std::move(draw));
    frame.mPasses.push_back(std::move(pass));
    frame.mReleases.push_back({ ResourceHandle{ handles.mOldImage }, inputs.mFrame });

    if (!static_cast<bool>(validate(frame)))
    {
        return std::nullopt;
    }
    return frame;
}

std::optional<StreamingUploadInputs> decodeStreamingUploadFrame(const FrameSnapshot& frame)
{
    if (!static_cast<bool>(validate(frame)) || frame.mBuffers.size() != 1 || frame.mImages.size() != 3 ||
        frame.mSamplers.size() != 1 || frame.mPipelines.size() != 1 || frame.mUploads.size() != 1 ||
        frame.mPasses.size() != 1 || frame.mReleases.size() != 1)
    {
        return std::nullopt;
    }

    const BufferResource&   screen      = frame.mBuffers[0];
    const ImageResource&    old_image   = frame.mImages[0];
    const ImageResource&    replacement = frame.mImages[1];
    const ImageResource&    output      = frame.mImages[2];
    const SamplerResource&  sampler     = frame.mSamplers[0];
    const PipelineResource& pipeline    = frame.mPipelines[0];
    const TextureUpload&    upload      = frame.mUploads[0];
    const RenderPass&       pass        = frame.mPasses[0];
    const ReleaseAfterFrame& release    = frame.mReleases[0];

    StreamingUploadInputs result;
    result.mFrame   = frame.mFrame;
    result.mHandles = { screen.mHandle, old_image.mHandle, replacement.mHandle, output.mHandle, sampler.mHandle,
                        pipeline.mHandle, pass.mId };
    result.mRevision        = upload.mRevision;
    result.mSubresource     = upload.mSubresource;
    result.mOffset          = upload.mOffset;
    result.mExtent          = upload.mExtent;
    result.mLogicalExtent   = upload.mLogicalExtent;
    result.mResidentDiscard = upload.mResidentDiscard;
    result.mSourceFormat    = upload.mSourceFormat;
    result.mRowPitch        = upload.mRowPitch;
    result.mRowOrigin       = upload.mRowOrigin;
    result.mMipGeneration   = upload.mMipGeneration;
    result.mBefore          = upload.mBefore;
    result.mDuring          = upload.mDuring;
    result.mAfter           = upload.mAfter;

    if (!validHandles(result.mHandles) || screen.mSize != 48 || screen.mLifetime != ResourceLifetime::Persistent ||
        !canonicalImage(old_image, result.mHandles.mOldImage, RESIDENT_EXTENT, TEXTURE_UPLOAD_MIP_LEVELS,
                        ResourceLifetime::Persistent) ||
        !canonicalImage(replacement, result.mHandles.mReplacementImage, RESIDENT_EXTENT, TEXTURE_UPLOAD_MIP_LEVELS,
                        ResourceLifetime::Persistent) ||
        !canonicalImage(output, result.mHandles.mOutput, OUTPUT_EXTENT, 1, ResourceLifetime::External))
    {
        return std::nullopt;
    }

    if (sampler.mHandle != result.mHandles.mSampler || sampler.mMinFilter != Filter::Linear ||
        sampler.mMagFilter != Filter::Linear || sampler.mMipFilter != MipFilter::Linear ||
        sampler.mAddressU != AddressMode::Clamp || sampler.mAddressV != AddressMode::Clamp || sampler.mMaxAnisotropy != 1.f ||
        sampler.mLifetime != ResourceLifetime::Persistent)
    {
        return std::nullopt;
    }

    if (pipeline.mProgram.mName != TEXTURE_UPLOAD_PROGRAM || pipeline.mProgram.mVariant != 0 ||
        pipeline.mTopology != PrimitiveTopology::TriangleList || pipeline.mCullMode != CullMode::Disabled ||
        pipeline.mFrontFace != FrontFace::CounterClockwise || pipeline.mDepthTestEnabled || pipeline.mDepthWriteEnabled ||
        pipeline.mDepthCompare != CompareOp::LessOrEqual || pipeline.mSamples != 1 || pipeline.mDepthFormat ||
        pipeline.mLifetime != ResourceLifetime::Persistent || pipeline.mColorTargets.size() != 1 ||
        pipeline.mColorTargets[0].mFormat != PixelFormat::RGBA8Unorm || pipeline.mColorTargets[0].mBlendEnabled ||
        pipeline.mColorTargets[0].mWriteMask != 0xf || pipeline.mVertexBindings.size() != 1 ||
        pipeline.mVertexBindings[0].mBinding != 0 || pipeline.mVertexBindings[0].mStride != 16 ||
        pipeline.mVertexAttributes.size() != 1 || pipeline.mVertexAttributes[0].mSemantic != VertexSemantic::Position ||
        pipeline.mVertexAttributes[0].mFormat != VertexFormat::Float3 || pipeline.mVertexAttributes[0].mBinding != 0 ||
        pipeline.mVertexAttributes[0].mOffset != 0 || pipeline.mSampledImageBindings != std::vector<std::uint32_t>{ 0 } ||
        !pipeline.mParameterBindings.empty())
    {
        return std::nullopt;
    }

    if (upload.mDestination != result.mHandles.mReplacementImage || !upload.mPixels.mStorage || upload.mPixels.mOffset != 0 ||
        upload.mPixels.mSize != TEXTURE_UPLOAD_SOURCE_BYTE_COUNT ||
        upload.mPixels.mStorage->size() != TEXTURE_UPLOAD_SOURCE_BYTE_COUNT)
    {
        return std::nullopt;
    }
    result.mPixels.assign(upload.mPixels.mStorage->begin(), upload.mPixels.mStorage->end());
    if (!canonicalRequest(result))
    {
        return std::nullopt;
    }

    if (pass.mId != result.mHandles.mPass || pass.mLabel != TEXTURE_UPLOAD_PASS || !sameExtent(pass.mExtent, OUTPUT_EXTENT) ||
        pass.mViewport.mX != 0.f || pass.mViewport.mY != 0.f || pass.mViewport.mWidth != TEXTURE_UPLOAD_OUTPUT_WIDTH ||
        pass.mViewport.mHeight != TEXTURE_UPLOAD_OUTPUT_HEIGHT || pass.mViewport.mMinDepth != 0.f || pass.mViewport.mMaxDepth != 1.f ||
        pass.mScissor.mX != 0 || pass.mScissor.mY != 0 || pass.mScissor.mWidth != TEXTURE_UPLOAD_OUTPUT_WIDTH ||
        pass.mScissor.mHeight != TEXTURE_UPLOAD_OUTPUT_HEIGHT || !pass.mDependencies.empty() || pass.mBufferAccesses.size() != 1 ||
        pass.mBufferAccesses[0].mBuffer != result.mHandles.mScreenTriangle ||
        pass.mBufferAccesses[0].mKind != BufferAccessKind::VertexRead || pass.mImageAccesses.size() != 2 ||
        !canonicalSampledAccess(pass.mImageAccesses[0], result.mHandles.mReplacementImage) ||
        !canonicalOutputAccess(pass.mImageAccesses[1], result.mHandles.mOutput) || pass.mColorAttachments.size() != 1 ||
        pass.mColorAttachments[0].mImage != result.mHandles.mOutput || pass.mColorAttachments[0].mSubresource != ImageSubresource{} ||
        pass.mColorAttachments[0].mLoad != LoadOp::DontCare || pass.mColorAttachments[0].mStore != StoreOp::Store ||
        !zeroClear(pass.mColorAttachments[0].mClear) || pass.mDepthAttachment || pass.mDraws.size() != 1 ||
        !std::holds_alternative<Draw>(pass.mDraws[0]))
    {
        return std::nullopt;
    }

    const Draw& draw = std::get<Draw>(pass.mDraws[0]);
    if (draw.mResources.mPipeline != result.mHandles.mPipeline || draw.mResources.mVertexBuffers.size() != 1 ||
        draw.mResources.mVertexBuffers[0].mBinding != 0 ||
        draw.mResources.mVertexBuffers[0].mBuffer != result.mHandles.mScreenTriangle ||
        draw.mResources.mVertexBuffers[0].mOffset != 0 || draw.mResources.mSampledImages.size() != 1 ||
        draw.mResources.mSampledImages[0].mBinding != 0 ||
        draw.mResources.mSampledImages[0].mImage != result.mHandles.mReplacementImage ||
        !sameRange(draw.mResources.mSampledImages[0].mRange, REPLACEMENT_RANGE) ||
        draw.mResources.mSampledImages[0].mSampler != result.mHandles.mSampler || !draw.mResources.mParameters.empty() ||
        draw.mFirstVertex != 0 || draw.mVertexCount != 3 || draw.mFirstInstance != 0 || draw.mInstanceCount != 1)
    {
        return std::nullopt;
    }

    if (!std::holds_alternative<ImageHandle>(release.mResource) ||
        std::get<ImageHandle>(release.mResource) != result.mHandles.mOldImage || release.mFrame != result.mFrame)
    {
        return std::nullopt;
    }

    return result;
}

} // namespace LLRenderContract
