/**
 * @file llmaterialcontract.cpp
 * @brief Pure builder and decoder for one indexed legacy material draw.
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

#include "llmaterialcontract.h"

#include <algorithm>
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

    constexpr const char*           MATERIAL_PROGRAM = "deferred.material.normspec";
    constexpr const char*           MATERIAL_PASS    = "indexed material";
    constexpr Extent2D              MATERIAL_FRAME_EXTENT{ MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT };
    constexpr Extent2D              MATERIAL_TEXTURE_EXTENT{ MATERIAL_TEXTURE_WIDTH, MATERIAL_TEXTURE_HEIGHT };
    constexpr ImageSubresourceRange MATERIAL_TEXTURE_RANGE{ 0, MATERIAL_TEXTURE_MIP_LEVELS, 0, 1 };

    bool sameExtent(Extent2D left, Extent2D right)
    {
        return left.mWidth == right.mWidth && left.mHeight == right.mHeight;
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

    template<std::size_t Size>
    bool finite(const std::array<float, Size>& values)
    {
        return std::all_of(values.begin(), values.end(), [](float value) { return std::isfinite(value); });
    }

    template<std::size_t Size>
    bool unitRange(const std::array<float, Size>& values)
    {
        return std::all_of(values.begin(), values.end(), [](float value) { return value >= 0.f && value <= 1.f; });
    }

    template<typename HandleType, std::size_t Size>
    bool distinctIndices(const std::array<HandleType, Size>& handles)
    {
        std::array<std::uint32_t, Size> indices{};
        std::transform(handles.begin(), handles.end(), indices.begin(), [](HandleType handle) { return handle.mIndex; });
        std::sort(indices.begin(), indices.end());
        return std::adjacent_find(indices.begin(), indices.end()) == indices.end();
    }

    bool validHandles(const MaterialHandles& handles)
    {
        const std::array buffers{ handles.mVertexBuffer, handles.mIndexBuffer };
        const std::array images{ handles.mDiffuse,  handles.mNormal,   handles.mSpecular, handles.mGBuffer0,
                                 handles.mGBuffer1, handles.mGBuffer2, handles.mDepth };
        return handles.mVertexBuffer && handles.mIndexBuffer && handles.mDiffuse && handles.mNormal && handles.mSpecular &&
               handles.mGBuffer0 && handles.mGBuffer1 && handles.mGBuffer2 && handles.mDepth && handles.mSampler && handles.mPipeline &&
               handles.mPass && distinctIndices(buffers) && distinctIndices(images);
    }

    ByteRange parameterBytes(const MaterialParameters& parameters)
    {
        static_assert(std::is_trivially_copyable_v<MaterialParameters>);
        auto storage = std::make_shared<std::vector<std::uint8_t>>(sizeof(parameters));
        std::memcpy(storage->data(), &parameters, sizeof(parameters));
        return { std::move(storage), 0, sizeof(parameters) };
    }

    ImageResource image(ImageHandle handle, Extent2D extent, PixelFormat format, std::uint32_t mip_levels = 1)
    {
        ImageResource result;
        result.mHandle    = handle;
        result.mExtent    = extent;
        result.mMipLevels = mip_levels;
        result.mFormat    = format;
        return result;
    }

    bool canonicalImage(const ImageResource& resource, ImageHandle handle, Extent2D extent, PixelFormat format,
                        std::uint32_t mip_levels = 1)
    {
        return resource.mHandle == handle && sameExtent(resource.mExtent, extent) && resource.mMipLevels == mip_levels &&
               resource.mArrayLayers == 1 && resource.mSamples == 1 && resource.mFormat == format &&
               resource.mLifetime == ResourceLifetime::Persistent;
    }

    SamplerResource sampler(SamplerHandle handle)
    {
        SamplerResource result;
        result.mHandle        = handle;
        result.mMinFilter     = Filter::Linear;
        result.mMagFilter     = Filter::Linear;
        result.mMipFilter     = MipFilter::Linear;
        result.mAddressU      = AddressMode::Repeat;
        result.mAddressV      = AddressMode::Repeat;
        result.mMaxAnisotropy = 8.f;
        return result;
    }

    bool canonicalSampler(const SamplerResource& resource, SamplerHandle handle)
    {
        return resource.mHandle == handle && resource.mMinFilter == Filter::Linear && resource.mMagFilter == Filter::Linear &&
               resource.mMipFilter == MipFilter::Linear && resource.mAddressU == AddressMode::Repeat &&
               resource.mAddressV == AddressMode::Repeat && resource.mMaxAnisotropy == 8.f &&
               resource.mLifetime == ResourceLifetime::Persistent;
    }

    bool canonicalSampledAccess(const ImageAccess& access, ImageHandle handle)
    {
        return access.mImage == handle && sameRange(access.mRange, MATERIAL_TEXTURE_RANGE) &&
               access.mKind == ImageAccessKind::SampledRead && access.mBefore == ImageState::ShaderRead &&
               access.mDuring == ImageState::ShaderRead && access.mAfter == ImageState::ShaderRead;
    }

    bool canonicalColorAccess(const ImageAccess& access, ImageHandle handle)
    {
        return access.mImage == handle && sameRange(access.mRange, {}) && access.mKind == ImageAccessKind::ColorAttachmentWrite &&
               access.mBefore == ImageState::Undefined && access.mDuring == ImageState::ColorAttachment &&
               access.mAfter == ImageState::ShaderRead;
    }

    bool canonicalDepthAccess(const ImageAccess& access, ImageHandle handle)
    {
        return access.mImage == handle && sameRange(access.mRange, {}) && access.mKind == ImageAccessKind::DepthAttachmentReadWrite &&
               access.mBefore == ImageState::DepthAttachment && access.mDuring == ImageState::DepthAttachment &&
               access.mAfter == ImageState::DepthAttachment;
    }

    bool canonicalColorAttachment(const ColorAttachment& attachment, ImageHandle handle)
    {
        return attachment.mImage == handle && attachment.mSubresource == ImageSubresource{} && attachment.mLoad == LoadOp::Clear &&
               attachment.mStore == StoreOp::Store && zeroClear(attachment.mClear);
    }

    bool canonicalSampledBinding(const SampledImageBinding& binding, std::uint32_t slot, ImageHandle image_handle,
                                 SamplerHandle sampler_handle)
    {
        return binding.mBinding == slot && binding.mImage == image_handle && sameRange(binding.mRange, MATERIAL_TEXTURE_RANGE) &&
               binding.mSampler == sampler_handle;
    }

    bool canonicalVertexBinding(const VertexBufferBinding& binding, std::uint32_t slot, BufferHandle buffer, std::uint64_t offset)
    {
        return binding.mBinding == slot && binding.mBuffer == buffer && binding.mOffset == offset;
    }

    bool canonicalTarget(const ColorTargetState& target, PixelFormat format)
    {
        return target.mFormat == format && !target.mBlendEnabled && target.mWriteMask == 0xf;
    }

    bool canonicalVertexLayout(const PipelineResource& pipeline)
    {
        if (pipeline.mVertexBindings.size() != 7 || pipeline.mVertexAttributes.size() != 7)
        {
            return false;
        }

        constexpr std::array<std::uint32_t, 7>  STRIDES{ 16, 16, 8, 4, 16, 8, 8 };
        constexpr std::array<VertexSemantic, 7> SEMANTICS{ VertexSemantic::Position, VertexSemantic::Normal,  VertexSemantic::TexCoord0,
                                                           VertexSemantic::Color,    VertexSemantic::Tangent, VertexSemantic::TexCoord1,
                                                           VertexSemantic::TexCoord2 };
        constexpr std::array<VertexFormat, 7>   FORMATS{ VertexFormat::Float3,   VertexFormat::Float3, VertexFormat::Float2,
                                                       VertexFormat::UNorm8x4, VertexFormat::Float4, VertexFormat::Float2,
                                                       VertexFormat::Float2 };

        for (std::size_t offset = 0; offset < STRIDES.size(); ++offset)
        {
            const VertexBindingLayout& binding   = pipeline.mVertexBindings[offset];
            const VertexAttribute&     attribute = pipeline.mVertexAttributes[offset];
            if (binding.mBinding != offset || binding.mStride != STRIDES[offset] || attribute.mSemantic != SEMANTICS[offset] ||
                attribute.mFormat != FORMATS[offset] || attribute.mBinding != offset || attribute.mOffset != 0)
            {
                return false;
            }
        }
        return true;
    }

} // namespace

bool validMaterialParameters(const MaterialParameters& parameters) noexcept
{
    return finite(parameters.mModelviewMatrix) && finite(parameters.mModelviewProjectionMatrix) && finite(parameters.mNormalMatrix) &&
           finite(parameters.mTextureMatrix0) && unitRange(parameters.mSpecularColor) && finite(parameters.mClipPlane) &&
           parameters.mEnvironmentIntensity >= 0.f && parameters.mEnvironmentIntensity <= 1.f &&
           (parameters.mEmissiveBrightness == 0.f || parameters.mEmissiveBrightness == 1.f) &&
           (parameters.mMirror == 0.f || parameters.mMirror == 1.f);
}

std::optional<FrameSnapshot> buildMaterialFrame(const MaterialInputs& inputs)
{
    if (inputs.mFrame == 0 || !validHandles(inputs.mHandles) || !validMaterialParameters(inputs.mParameters))
    {
        return std::nullopt;
    }

    const MaterialHandles& handles = inputs.mHandles;
    FrameSnapshot          frame;
    frame.mFrame   = inputs.mFrame;
    frame.mBuffers = { { handles.mVertexBuffer, MATERIAL_VERTEX_BUFFER_SIZE, ResourceLifetime::Persistent },
                       { handles.mIndexBuffer, MATERIAL_INDEX_BUFFER_SIZE, ResourceLifetime::Persistent } };
    frame.mImages  = { image(handles.mDiffuse, MATERIAL_TEXTURE_EXTENT, PixelFormat::RGBA8Unorm, MATERIAL_TEXTURE_MIP_LEVELS),
                       image(handles.mNormal, MATERIAL_TEXTURE_EXTENT, PixelFormat::RGBA8Unorm, MATERIAL_TEXTURE_MIP_LEVELS),
                       image(handles.mSpecular, MATERIAL_TEXTURE_EXTENT, PixelFormat::RGBA8Unorm, MATERIAL_TEXTURE_MIP_LEVELS),
                       image(handles.mGBuffer0, MATERIAL_FRAME_EXTENT, PixelFormat::RGBA8Unorm),
                       image(handles.mGBuffer1, MATERIAL_FRAME_EXTENT, PixelFormat::RGBA8Unorm),
                       image(handles.mGBuffer2, MATERIAL_FRAME_EXTENT, PixelFormat::RGBA16Unorm),
                       image(handles.mDepth, MATERIAL_FRAME_EXTENT, PixelFormat::Depth24Unorm) };
    frame.mSamplers.push_back(sampler(handles.mSampler));

    PipelineResource pipeline;
    pipeline.mHandle            = handles.mPipeline;
    pipeline.mProgram           = { MATERIAL_PROGRAM, 0 };
    pipeline.mCullMode          = CullMode::Back;
    pipeline.mFrontFace         = FrontFace::CounterClockwise;
    pipeline.mDepthTestEnabled  = true;
    pipeline.mDepthWriteEnabled = true;
    pipeline.mDepthCompare      = CompareOp::LessOrEqual;
    pipeline.mColorTargets      = { { PixelFormat::RGBA8Unorm, false, 0xf },
                                    { PixelFormat::RGBA8Unorm, false, 0xf },
                                    { PixelFormat::RGBA16Unorm, false, 0xf } };
    pipeline.mDepthFormat       = PixelFormat::Depth24Unorm;
    pipeline.mVertexBindings    = { { 0, 16 }, { 1, 16 }, { 2, 8 }, { 3, 4 }, { 4, 16 }, { 5, 8 }, { 6, 8 } };
    pipeline.mVertexAttributes  = {
        { VertexSemantic::Position, VertexFormat::Float3, 0, 0 },  { VertexSemantic::Normal, VertexFormat::Float3, 1, 0 },
        { VertexSemantic::TexCoord0, VertexFormat::Float2, 2, 0 }, { VertexSemantic::Color, VertexFormat::UNorm8x4, 3, 0 },
        { VertexSemantic::Tangent, VertexFormat::Float4, 4, 0 },   { VertexSemantic::TexCoord1, VertexFormat::Float2, 5, 0 },
        { VertexSemantic::TexCoord2, VertexFormat::Float2, 6, 0 }
    };
    pipeline.mSampledImageBindings = { 0, 1, 2 };
    pipeline.mParameterBindings.push_back({ 0, sizeof(MaterialParameters) });
    frame.mPipelines.push_back(std::move(pipeline));

    RenderPass pass;
    pass.mId               = handles.mPass;
    pass.mLabel            = MATERIAL_PASS;
    pass.mExtent           = MATERIAL_FRAME_EXTENT;
    pass.mViewport         = { 0.f, 0.f, static_cast<float>(MATERIAL_FRAME_WIDTH), static_cast<float>(MATERIAL_FRAME_HEIGHT), 0.f, 1.f };
    pass.mScissor          = { 0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT };
    pass.mBufferAccesses   = { { handles.mVertexBuffer, BufferAccessKind::VertexRead },
                               { handles.mIndexBuffer, BufferAccessKind::IndexRead } };
    pass.mImageAccesses    = { { handles.mDiffuse, MATERIAL_TEXTURE_RANGE, ImageAccessKind::SampledRead, ImageState::ShaderRead,
                                 ImageState::ShaderRead, ImageState::ShaderRead },
                               { handles.mNormal, MATERIAL_TEXTURE_RANGE, ImageAccessKind::SampledRead, ImageState::ShaderRead,
                                 ImageState::ShaderRead, ImageState::ShaderRead },
                               { handles.mSpecular, MATERIAL_TEXTURE_RANGE, ImageAccessKind::SampledRead, ImageState::ShaderRead,
                                 ImageState::ShaderRead, ImageState::ShaderRead },
                               { handles.mGBuffer0,
                                 {},
                                 ImageAccessKind::ColorAttachmentWrite,
                                 ImageState::Undefined,
                                 ImageState::ColorAttachment,
                                 ImageState::ShaderRead },
                               { handles.mGBuffer1,
                                 {},
                                 ImageAccessKind::ColorAttachmentWrite,
                                 ImageState::Undefined,
                                 ImageState::ColorAttachment,
                                 ImageState::ShaderRead },
                               { handles.mGBuffer2,
                                 {},
                                 ImageAccessKind::ColorAttachmentWrite,
                                 ImageState::Undefined,
                                 ImageState::ColorAttachment,
                                 ImageState::ShaderRead },
                               { handles.mDepth,
                                 {},
                                 ImageAccessKind::DepthAttachmentReadWrite,
                                 ImageState::DepthAttachment,
                                 ImageState::DepthAttachment,
                                 ImageState::DepthAttachment } };
    pass.mColorAttachments = { { handles.mGBuffer0, {}, LoadOp::Clear, StoreOp::Store, {} },
                               { handles.mGBuffer1, {}, LoadOp::Clear, StoreOp::Store, {} },
                               { handles.mGBuffer2, {}, LoadOp::Clear, StoreOp::Store, {} } };
    pass.mDepthAttachment  = DepthAttachment{ handles.mDepth, {}, LoadOp::Load, StoreOp::Store, 1.f };

    DrawIndexed draw;
    draw.mResources.mPipeline      = handles.mPipeline;
    draw.mResources.mVertexBuffers = {
        { 0, handles.mVertexBuffer, MATERIAL_POSITION_OFFSET },  { 1, handles.mVertexBuffer, MATERIAL_NORMAL_OFFSET },
        { 2, handles.mVertexBuffer, MATERIAL_TEXCOORD0_OFFSET }, { 3, handles.mVertexBuffer, MATERIAL_COLOR_OFFSET },
        { 4, handles.mVertexBuffer, MATERIAL_TANGENT_OFFSET },   { 5, handles.mVertexBuffer, MATERIAL_TEXCOORD1_OFFSET },
        { 6, handles.mVertexBuffer, MATERIAL_TEXCOORD2_OFFSET }
    };
    draw.mResources.mSampledImages = { { 0, handles.mDiffuse, MATERIAL_TEXTURE_RANGE, handles.mSampler },
                                       { 1, handles.mNormal, MATERIAL_TEXTURE_RANGE, handles.mSampler },
                                       { 2, handles.mSpecular, MATERIAL_TEXTURE_RANGE, handles.mSampler } };
    draw.mResources.mParameters.push_back({ 0, parameterBytes(inputs.mParameters) });
    draw.mIndexBuffer = { handles.mIndexBuffer, 0, IndexType::UInt16 };
    draw.mIndexCount  = 6;
    draw.mMinVertex   = 0;
    draw.mMaxVertex   = 3;
    pass.mDraws.emplace_back(std::move(draw));
    frame.mPasses.push_back(std::move(pass));

    if (!static_cast<bool>(validate(frame)))
    {
        return std::nullopt;
    }
    return frame;
}

std::optional<MaterialInputs> decodeMaterialFrame(const FrameSnapshot& frame)
{
    if (!static_cast<bool>(validate(frame)) || !frame.mUploads.empty() || !frame.mReleases.empty() || frame.mBuffers.size() != 2 ||
        frame.mImages.size() != 7 || frame.mSamplers.size() != 1 || frame.mPipelines.size() != 1 || frame.mPasses.size() != 1)
    {
        return std::nullopt;
    }

    const BufferResource&   vertex_buffer    = frame.mBuffers[0];
    const BufferResource&   index_buffer     = frame.mBuffers[1];
    const ImageResource&    diffuse          = frame.mImages[0];
    const ImageResource&    normal           = frame.mImages[1];
    const ImageResource&    specular         = frame.mImages[2];
    const ImageResource&    gbuffer0         = frame.mImages[3];
    const ImageResource&    gbuffer1         = frame.mImages[4];
    const ImageResource&    gbuffer2         = frame.mImages[5];
    const ImageResource&    depth            = frame.mImages[6];
    const SamplerResource&  material_sampler = frame.mSamplers.front();
    const PipelineResource& pipeline         = frame.mPipelines.front();
    const RenderPass&       pass             = frame.mPasses.front();

    MaterialInputs result;
    result.mFrame   = frame.mFrame;
    result.mHandles = { vertex_buffer.mHandle, index_buffer.mHandle,     diffuse.mHandle,  normal.mHandle,
                        specular.mHandle,      gbuffer0.mHandle,         gbuffer1.mHandle, gbuffer2.mHandle,
                        depth.mHandle,         material_sampler.mHandle, pipeline.mHandle, pass.mId };

    if (!validHandles(result.mHandles) || vertex_buffer.mSize != MATERIAL_VERTEX_BUFFER_SIZE ||
        vertex_buffer.mLifetime != ResourceLifetime::Persistent || index_buffer.mSize != MATERIAL_INDEX_BUFFER_SIZE ||
        index_buffer.mLifetime != ResourceLifetime::Persistent ||
        !canonicalImage(diffuse, result.mHandles.mDiffuse, MATERIAL_TEXTURE_EXTENT, PixelFormat::RGBA8Unorm, MATERIAL_TEXTURE_MIP_LEVELS) ||
        !canonicalImage(normal, result.mHandles.mNormal, MATERIAL_TEXTURE_EXTENT, PixelFormat::RGBA8Unorm, MATERIAL_TEXTURE_MIP_LEVELS) ||
        !canonicalImage(specular, result.mHandles.mSpecular, MATERIAL_TEXTURE_EXTENT, PixelFormat::RGBA8Unorm,
                        MATERIAL_TEXTURE_MIP_LEVELS) ||
        !canonicalImage(gbuffer0, result.mHandles.mGBuffer0, MATERIAL_FRAME_EXTENT, PixelFormat::RGBA8Unorm) ||
        !canonicalImage(gbuffer1, result.mHandles.mGBuffer1, MATERIAL_FRAME_EXTENT, PixelFormat::RGBA8Unorm) ||
        !canonicalImage(gbuffer2, result.mHandles.mGBuffer2, MATERIAL_FRAME_EXTENT, PixelFormat::RGBA16Unorm) ||
        !canonicalImage(depth, result.mHandles.mDepth, MATERIAL_FRAME_EXTENT, PixelFormat::Depth24Unorm) ||
        !canonicalSampler(material_sampler, result.mHandles.mSampler))
    {
        return std::nullopt;
    }

    if (pipeline.mProgram.mName != MATERIAL_PROGRAM || pipeline.mProgram.mVariant != 0 ||
        pipeline.mTopology != PrimitiveTopology::TriangleList || pipeline.mCullMode != CullMode::Back ||
        pipeline.mFrontFace != FrontFace::CounterClockwise || !pipeline.mDepthTestEnabled || !pipeline.mDepthWriteEnabled ||
        pipeline.mDepthCompare != CompareOp::LessOrEqual || pipeline.mSamples != 1 || pipeline.mDepthFormat != PixelFormat::Depth24Unorm ||
        pipeline.mLifetime != ResourceLifetime::Persistent || pipeline.mColorTargets.size() != 3 ||
        !canonicalTarget(pipeline.mColorTargets[0], PixelFormat::RGBA8Unorm) ||
        !canonicalTarget(pipeline.mColorTargets[1], PixelFormat::RGBA8Unorm) ||
        !canonicalTarget(pipeline.mColorTargets[2], PixelFormat::RGBA16Unorm) || !canonicalVertexLayout(pipeline) ||
        pipeline.mSampledImageBindings != std::vector<std::uint32_t>{ 0, 1, 2 } || pipeline.mParameterBindings.size() != 1 ||
        pipeline.mParameterBindings[0].mBinding != 0 || pipeline.mParameterBindings[0].mSize != sizeof(MaterialParameters))
    {
        return std::nullopt;
    }

    if (pass.mId != result.mHandles.mPass || pass.mLabel != MATERIAL_PASS || !sameExtent(pass.mExtent, MATERIAL_FRAME_EXTENT) ||
        pass.mViewport.mX != 0.f || pass.mViewport.mY != 0.f || pass.mViewport.mWidth != MATERIAL_FRAME_WIDTH ||
        pass.mViewport.mHeight != MATERIAL_FRAME_HEIGHT || pass.mViewport.mMinDepth != 0.f || pass.mViewport.mMaxDepth != 1.f ||
        pass.mScissor.mX != 0 || pass.mScissor.mY != 0 || pass.mScissor.mWidth != MATERIAL_FRAME_WIDTH ||
        pass.mScissor.mHeight != MATERIAL_FRAME_HEIGHT || !pass.mDependencies.empty() || pass.mBufferAccesses.size() != 2 ||
        pass.mBufferAccesses[0].mBuffer != result.mHandles.mVertexBuffer || pass.mBufferAccesses[0].mKind != BufferAccessKind::VertexRead ||
        pass.mBufferAccesses[1].mBuffer != result.mHandles.mIndexBuffer || pass.mBufferAccesses[1].mKind != BufferAccessKind::IndexRead ||
        pass.mImageAccesses.size() != 7 || !canonicalSampledAccess(pass.mImageAccesses[0], result.mHandles.mDiffuse) ||
        !canonicalSampledAccess(pass.mImageAccesses[1], result.mHandles.mNormal) ||
        !canonicalSampledAccess(pass.mImageAccesses[2], result.mHandles.mSpecular) ||
        !canonicalColorAccess(pass.mImageAccesses[3], result.mHandles.mGBuffer0) ||
        !canonicalColorAccess(pass.mImageAccesses[4], result.mHandles.mGBuffer1) ||
        !canonicalColorAccess(pass.mImageAccesses[5], result.mHandles.mGBuffer2) ||
        !canonicalDepthAccess(pass.mImageAccesses[6], result.mHandles.mDepth) || pass.mColorAttachments.size() != 3 ||
        !canonicalColorAttachment(pass.mColorAttachments[0], result.mHandles.mGBuffer0) ||
        !canonicalColorAttachment(pass.mColorAttachments[1], result.mHandles.mGBuffer1) ||
        !canonicalColorAttachment(pass.mColorAttachments[2], result.mHandles.mGBuffer2) || !pass.mDepthAttachment ||
        pass.mDepthAttachment->mImage != result.mHandles.mDepth || pass.mDepthAttachment->mSubresource != ImageSubresource{} ||
        pass.mDepthAttachment->mLoad != LoadOp::Load || pass.mDepthAttachment->mStore != StoreOp::Store ||
        pass.mDepthAttachment->mClearDepth != 1.f || pass.mDraws.size() != 1 || !std::holds_alternative<DrawIndexed>(pass.mDraws.front()))
    {
        return std::nullopt;
    }

    const DrawIndexed& draw = std::get<DrawIndexed>(pass.mDraws.front());
    if (draw.mResources.mPipeline != result.mHandles.mPipeline || draw.mResources.mVertexBuffers.size() != 7 ||
        !canonicalVertexBinding(draw.mResources.mVertexBuffers[0], 0, result.mHandles.mVertexBuffer, MATERIAL_POSITION_OFFSET) ||
        !canonicalVertexBinding(draw.mResources.mVertexBuffers[1], 1, result.mHandles.mVertexBuffer, MATERIAL_NORMAL_OFFSET) ||
        !canonicalVertexBinding(draw.mResources.mVertexBuffers[2], 2, result.mHandles.mVertexBuffer, MATERIAL_TEXCOORD0_OFFSET) ||
        !canonicalVertexBinding(draw.mResources.mVertexBuffers[3], 3, result.mHandles.mVertexBuffer, MATERIAL_COLOR_OFFSET) ||
        !canonicalVertexBinding(draw.mResources.mVertexBuffers[4], 4, result.mHandles.mVertexBuffer, MATERIAL_TANGENT_OFFSET) ||
        !canonicalVertexBinding(draw.mResources.mVertexBuffers[5], 5, result.mHandles.mVertexBuffer, MATERIAL_TEXCOORD1_OFFSET) ||
        !canonicalVertexBinding(draw.mResources.mVertexBuffers[6], 6, result.mHandles.mVertexBuffer, MATERIAL_TEXCOORD2_OFFSET) ||
        draw.mResources.mSampledImages.size() != 3 ||
        !canonicalSampledBinding(draw.mResources.mSampledImages[0], 0, result.mHandles.mDiffuse, result.mHandles.mSampler) ||
        !canonicalSampledBinding(draw.mResources.mSampledImages[1], 1, result.mHandles.mNormal, result.mHandles.mSampler) ||
        !canonicalSampledBinding(draw.mResources.mSampledImages[2], 2, result.mHandles.mSpecular, result.mHandles.mSampler) ||
        draw.mResources.mParameters.size() != 1 || draw.mResources.mParameters[0].mBinding != 0 ||
        draw.mResources.mParameters[0].mBytes.mSize != sizeof(MaterialParameters) ||
        draw.mIndexBuffer.mBuffer != result.mHandles.mIndexBuffer || draw.mIndexBuffer.mOffset != 0 ||
        draw.mIndexBuffer.mType != IndexType::UInt16 || draw.mFirstIndex != 0 || draw.mIndexCount != 6 || draw.mBaseVertex != 0 ||
        draw.mMinVertex != 0 || draw.mMaxVertex != 3 || draw.mFirstInstance != 0 || draw.mInstanceCount != 1)
    {
        return std::nullopt;
    }

    const ByteRange& bytes = draw.mResources.mParameters[0].mBytes;
    if (bytes.mOffset != 0 || bytes.mStorage->size() != sizeof(MaterialParameters))
    {
        return std::nullopt;
    }
    std::memcpy(&result.mParameters, bytes.mStorage->data() + bytes.mOffset, sizeof(result.mParameters));
    if (!validMaterialParameters(result.mParameters))
    {
        return std::nullopt;
    }
    return result;
}

} // namespace LLRenderContract
