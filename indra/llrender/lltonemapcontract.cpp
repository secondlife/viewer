/**
 * @file lltonemapcontract.cpp
 * @brief Pure builder and decoder for the viewer tonemap pass.
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

#include "lltonemapcontract.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

namespace LLRenderContract
{
namespace
{

constexpr const char* TONEMAP_PROGRAM = "deferred.tonemap";
constexpr const char* TONEMAP_PASS = "tonemap";

bool sameExtent(Extent2D left, Extent2D right)
{
    return left.mWidth == right.mWidth && left.mHeight == right.mHeight;
}

bool sameRange(const ImageSubresourceRange& left, const ImageSubresourceRange& right)
{
    return left.mBaseMipLevel == right.mBaseMipLevel && left.mMipLevelCount == right.mMipLevelCount &&
           left.mBaseArrayLayer == right.mBaseArrayLayer && left.mArrayLayerCount == right.mArrayLayerCount;
}

bool sameSubresource(const ImageSubresource& left, const ImageSubresource& right)
{
    return left == right;
}

bool finiteParameters(const TonemapParameters& parameters)
{
    return std::isfinite(parameters.mExposure) && parameters.mExposure >= 0.5f && parameters.mExposure <= 4.f &&
           std::isfinite(parameters.mTonemapMix) && std::isfinite(parameters.mGamma) && parameters.mGamma >= 0.f;
}

bool validOutputFormat(PixelFormat format)
{
    return format == PixelFormat::RGBA8Unorm || format == PixelFormat::RGBA16Float;
}

bool validInputs(const TonemapInputs& inputs)
{
    return inputs.mFrame != 0 && inputs.mHandles.mScreenTriangle && inputs.mHandles.mScene && inputs.mHandles.mExposure &&
           inputs.mHandles.mDestination && inputs.mHandles.mPointSampler && inputs.mHandles.mLinearSampler && inputs.mHandles.mPipeline &&
           inputs.mHandles.mPass && inputs.mSourceExtent.mWidth != 0 && inputs.mSourceExtent.mHeight != 0 &&
           inputs.mDestinationExtent.mWidth != 0 && inputs.mDestinationExtent.mHeight != 0 &&
           validOutputFormat(inputs.mDestinationFormat) && validTonemapVariant(inputs.mVariant) && finiteParameters(inputs.mParameters);
}

ByteRange parameterBytes(const TonemapParameters& parameters)
{
    static_assert(std::is_trivially_copyable_v<TonemapParameters>);
    auto storage = std::make_shared<std::vector<std::uint8_t>>(sizeof(parameters));
    std::memcpy(storage->data(), &parameters, sizeof(parameters));
    return { std::move(storage), 0, sizeof(parameters) };
}

ImageResource image(ImageHandle handle, Extent2D extent, PixelFormat format)
{
    ImageResource result;
    result.mHandle = handle;
    result.mExtent = extent;
    result.mFormat = format;
    return result;
}

SamplerResource sampler(SamplerHandle handle, Filter filter)
{
    SamplerResource result;
    result.mHandle = handle;
    result.mMinFilter = filter;
    result.mMagFilter = filter;
    result.mMipFilter = MipFilter::Disabled;
    result.mAddressU = AddressMode::Mirror;
    result.mAddressV = AddressMode::Mirror;
    return result;
}

bool canonicalSampler(const SamplerResource& sampler_resource, SamplerHandle handle, Filter filter)
{
    return sampler_resource.mHandle == handle && sampler_resource.mMinFilter == filter && sampler_resource.mMagFilter == filter &&
           sampler_resource.mMipFilter == MipFilter::Disabled && sampler_resource.mAddressU == AddressMode::Mirror &&
           sampler_resource.mAddressV == AddressMode::Mirror && sampler_resource.mMaxAnisotropy == 1.f &&
           sampler_resource.mLifetime == ResourceLifetime::Persistent;
}

bool canonicalImage(const ImageResource& image_resource, ImageHandle handle, Extent2D extent, PixelFormat format)
{
    return image_resource.mHandle == handle && sameExtent(image_resource.mExtent, extent) && image_resource.mMipLevels == 1 &&
           image_resource.mArrayLayers == 1 && image_resource.mSamples == 1 && image_resource.mFormat == format &&
           image_resource.mLifetime == ResourceLifetime::Persistent;
}

bool canonicalSampledAccess(const ImageAccess& access, ImageHandle image_handle)
{
    return access.mImage == image_handle && sameRange(access.mRange, {}) && access.mKind == ImageAccessKind::SampledRead &&
           access.mBefore == ImageState::ShaderRead && access.mDuring == ImageState::ShaderRead && access.mAfter == ImageState::ShaderRead;
}

bool canonicalDestinationAccess(const ImageAccess& access, ImageHandle image_handle)
{
    return access.mImage == image_handle && sameRange(access.mRange, {}) && access.mKind == ImageAccessKind::ColorAttachmentWrite &&
           access.mBefore == ImageState::Undefined && access.mDuring == ImageState::ColorAttachment && access.mAfter == ImageState::ShaderRead;
}

bool canonicalSampledBinding(const SampledImageBinding& binding, std::uint32_t slot, ImageHandle image_handle, SamplerHandle sampler_handle)
{
    return binding.mBinding == slot && binding.mImage == image_handle && sameRange(binding.mRange, {}) &&
           binding.mSampler == sampler_handle;
}

}

bool validTonemapVariant(TonemapVariant variant) noexcept
{
    switch (variant)
    {
        case TonemapVariant::Deferred:
        case TonemapVariant::NoPost:
        case TonemapVariant::GammaCorrect:
        case TonemapVariant::NoPostGammaCorrect:
        case TonemapVariant::LegacyGammaCorrect:
        case TonemapVariant::NoPostLegacyGammaCorrect:
            return true;
    }
    return false;
}

std::optional<FrameSnapshot> buildTonemapFrame(const TonemapInputs& inputs)
{
    if (!validInputs(inputs))
    {
        return std::nullopt;
    }

    const TonemapHandles& handles = inputs.mHandles;

    FrameSnapshot frame;
    frame.mFrame = inputs.mFrame;
    frame.mBuffers.push_back({ handles.mScreenTriangle, 48, ResourceLifetime::Persistent });
    frame.mImages = { image(handles.mScene, inputs.mSourceExtent, PixelFormat::RGBA16Float),
                      image(handles.mExposure, { 1, 1 }, PixelFormat::R16Float),
                      image(handles.mDestination, inputs.mDestinationExtent, inputs.mDestinationFormat) };
    frame.mSamplers = { sampler(handles.mPointSampler, Filter::Nearest), sampler(handles.mLinearSampler, Filter::Linear) };

    PipelineResource pipeline;
    pipeline.mHandle = handles.mPipeline;
    pipeline.mProgram = { TONEMAP_PROGRAM, static_cast<std::uint64_t>(inputs.mVariant) };
    pipeline.mCullMode = CullMode::Disabled;
    pipeline.mDepthTestEnabled = false;
    pipeline.mDepthWriteEnabled = false;
    pipeline.mDepthCompare = CompareOp::LessOrEqual;
    pipeline.mColorTargets.push_back({ inputs.mDestinationFormat, false, 0xf });
    pipeline.mVertexBindings.push_back({ 0, 16 });
    pipeline.mVertexAttributes.push_back({ VertexSemantic::Position, VertexFormat::Float3, 0, 0 });
    pipeline.mSampledImageBindings = { 0, 1 };
    pipeline.mParameterBindings.push_back({ 0, sizeof(TonemapParameters) });
    frame.mPipelines.push_back(std::move(pipeline));

    RenderPass pass;
    pass.mId = handles.mPass;
    pass.mLabel = TONEMAP_PASS;
    pass.mExtent = inputs.mDestinationExtent;
    pass.mViewport = { 0.f, 0.f, static_cast<float>(inputs.mDestinationExtent.mWidth),
                       static_cast<float>(inputs.mDestinationExtent.mHeight), 0.f, 1.f };
    pass.mScissor = { 0, 0, inputs.mDestinationExtent.mWidth, inputs.mDestinationExtent.mHeight };
    pass.mBufferAccesses.push_back({ handles.mScreenTriangle, BufferAccessKind::VertexRead });
    pass.mImageAccesses = {
        { handles.mScene, {}, ImageAccessKind::SampledRead, ImageState::ShaderRead, ImageState::ShaderRead, ImageState::ShaderRead },
        { handles.mExposure, {}, ImageAccessKind::SampledRead, ImageState::ShaderRead, ImageState::ShaderRead, ImageState::ShaderRead },
        { handles.mDestination, {}, ImageAccessKind::ColorAttachmentWrite, ImageState::Undefined,
          ImageState::ColorAttachment, ImageState::ShaderRead }
    };
    pass.mColorAttachments.push_back({ handles.mDestination, {}, LoadOp::DontCare, StoreOp::Store, {} });

    Draw draw;
    draw.mResources.mPipeline = handles.mPipeline;
    draw.mResources.mVertexBuffers.push_back({ 0, handles.mScreenTriangle, 0 });
    draw.mResources.mSampledImages = {
        { 0, handles.mScene, {}, handles.mPointSampler },
        { 1, handles.mExposure, {}, handles.mLinearSampler }
    };
    draw.mResources.mParameters.push_back({ 0, parameterBytes(inputs.mParameters) });
    draw.mVertexCount = 3;
    pass.mDraws.emplace_back(std::move(draw));
    frame.mPasses.push_back(std::move(pass));

    return frame;
}

std::optional<TonemapInputs> decodeTonemapFrame(const FrameSnapshot& frame)
{
    if (!static_cast<bool>(validate(frame)) || !frame.mUploads.empty() || !frame.mReleases.empty() || frame.mBuffers.size() != 1 ||
        frame.mImages.size() != 3 || frame.mSamplers.size() != 2 || frame.mPipelines.size() != 1 || frame.mPasses.size() != 1)
    {
        return std::nullopt;
    }

    const BufferResource& buffer = frame.mBuffers.front();
    const ImageResource& scene = frame.mImages[0];
    const ImageResource& exposure = frame.mImages[1];
    const ImageResource& destination = frame.mImages[2];
    const SamplerResource& point_sampler = frame.mSamplers[0];
    const SamplerResource& linear_sampler = frame.mSamplers[1];
    const PipelineResource& pipeline = frame.mPipelines.front();
    const RenderPass& pass = frame.mPasses.front();

    TonemapInputs result;
    result.mFrame = frame.mFrame;
    result.mHandles = { buffer.mHandle, scene.mHandle, exposure.mHandle, destination.mHandle,
                        point_sampler.mHandle, linear_sampler.mHandle, pipeline.mHandle, pass.mId };
    result.mSourceExtent = scene.mExtent;
    result.mDestinationExtent = destination.mExtent;
    result.mDestinationFormat = destination.mFormat;
    result.mVariant = static_cast<TonemapVariant>(pipeline.mProgram.mVariant);

    if (buffer.mSize != 48 || buffer.mLifetime != ResourceLifetime::Persistent ||
        !canonicalImage(scene, result.mHandles.mScene, result.mSourceExtent, PixelFormat::RGBA16Float) ||
        !canonicalImage(exposure, result.mHandles.mExposure, { 1, 1 }, PixelFormat::R16Float) ||
        !canonicalImage(destination, result.mHandles.mDestination, result.mDestinationExtent, result.mDestinationFormat) ||
        !validOutputFormat(result.mDestinationFormat) || !canonicalSampler(point_sampler, result.mHandles.mPointSampler, Filter::Nearest) ||
        !canonicalSampler(linear_sampler, result.mHandles.mLinearSampler, Filter::Linear) || !validTonemapVariant(result.mVariant))
    {
        return std::nullopt;
    }

    if (pipeline.mProgram.mName != TONEMAP_PROGRAM || pipeline.mTopology != PrimitiveTopology::TriangleList ||
        pipeline.mCullMode != CullMode::Disabled || pipeline.mFrontFace != FrontFace::CounterClockwise || pipeline.mDepthTestEnabled ||
        pipeline.mDepthWriteEnabled || pipeline.mDepthCompare != CompareOp::LessOrEqual || pipeline.mSamples != 1 || pipeline.mDepthFormat ||
        pipeline.mLifetime != ResourceLifetime::Persistent || pipeline.mColorTargets.size() != 1 ||
        pipeline.mColorTargets[0].mFormat != result.mDestinationFormat || pipeline.mColorTargets[0].mBlendEnabled ||
        pipeline.mColorTargets[0].mWriteMask != 0xf || pipeline.mVertexBindings.size() != 1 ||
        pipeline.mVertexBindings[0].mBinding != 0 || pipeline.mVertexBindings[0].mStride != 16 ||
        pipeline.mVertexAttributes.size() != 1 || pipeline.mVertexAttributes[0].mSemantic != VertexSemantic::Position ||
        pipeline.mVertexAttributes[0].mFormat != VertexFormat::Float3 || pipeline.mVertexAttributes[0].mBinding != 0 ||
        pipeline.mVertexAttributes[0].mOffset != 0 || pipeline.mSampledImageBindings != std::vector<std::uint32_t>{ 0, 1 } ||
        pipeline.mParameterBindings.size() != 1 || pipeline.mParameterBindings[0].mBinding != 0 ||
        pipeline.mParameterBindings[0].mSize != sizeof(TonemapParameters))
    {
        return std::nullopt;
    }

    if (pass.mId != result.mHandles.mPass || pass.mLabel != TONEMAP_PASS || !sameExtent(pass.mExtent, result.mDestinationExtent) ||
        pass.mViewport.mX != 0.f || pass.mViewport.mY != 0.f || pass.mViewport.mWidth != result.mDestinationExtent.mWidth ||
        pass.mViewport.mHeight != result.mDestinationExtent.mHeight || pass.mViewport.mMinDepth != 0.f || pass.mViewport.mMaxDepth != 1.f ||
        pass.mScissor.mX != 0 || pass.mScissor.mY != 0 || pass.mScissor.mWidth != result.mDestinationExtent.mWidth ||
        pass.mScissor.mHeight != result.mDestinationExtent.mHeight || !pass.mDependencies.empty() || pass.mBufferAccesses.size() != 1 ||
        pass.mBufferAccesses[0].mBuffer != result.mHandles.mScreenTriangle ||
        pass.mBufferAccesses[0].mKind != BufferAccessKind::VertexRead || pass.mImageAccesses.size() != 3 ||
        !canonicalSampledAccess(pass.mImageAccesses[0], result.mHandles.mScene) ||
        !canonicalSampledAccess(pass.mImageAccesses[1], result.mHandles.mExposure) ||
        !canonicalDestinationAccess(pass.mImageAccesses[2], result.mHandles.mDestination) || pass.mColorAttachments.size() != 1 ||
        pass.mColorAttachments[0].mImage != result.mHandles.mDestination || !sameSubresource(pass.mColorAttachments[0].mSubresource, {}) ||
        pass.mColorAttachments[0].mLoad != LoadOp::DontCare || pass.mColorAttachments[0].mStore != StoreOp::Store ||
        pass.mDepthAttachment || pass.mDraws.size() != 1 || !std::holds_alternative<Draw>(pass.mDraws.front()))
    {
        return std::nullopt;
    }

    const Draw& draw = std::get<Draw>(pass.mDraws.front());
    if (draw.mResources.mPipeline != result.mHandles.mPipeline || draw.mResources.mVertexBuffers.size() != 1 ||
        draw.mResources.mVertexBuffers[0].mBinding != 0 || draw.mResources.mVertexBuffers[0].mBuffer != result.mHandles.mScreenTriangle ||
        draw.mResources.mVertexBuffers[0].mOffset != 0 || draw.mResources.mSampledImages.size() != 2 ||
        !canonicalSampledBinding(draw.mResources.mSampledImages[0], 0, result.mHandles.mScene, result.mHandles.mPointSampler) ||
        !canonicalSampledBinding(draw.mResources.mSampledImages[1], 1, result.mHandles.mExposure, result.mHandles.mLinearSampler) ||
        draw.mResources.mParameters.size() != 1 || draw.mResources.mParameters[0].mBinding != 0 ||
        draw.mResources.mParameters[0].mBytes.mSize != sizeof(TonemapParameters) || draw.mFirstVertex != 0 || draw.mVertexCount != 3 ||
        draw.mFirstInstance != 0 || draw.mInstanceCount != 1)
    {
        return std::nullopt;
    }

    const ByteRange& bytes = draw.mResources.mParameters[0].mBytes;
    std::memcpy(&result.mParameters, bytes.mStorage->data() + bytes.mOffset, sizeof(result.mParameters));
    if (!finiteParameters(result.mParameters))
    {
        return std::nullopt;
    }

    return result;
}

}
