/**
 * @file llrendervulkantextureupload.cpp
 * @brief Vulkan replay of the canonical streamed texture-upload packet.
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

#include "llrendervulkantextureupload.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <optional>
#include <utility>

namespace LLRenderVulkanTextureUpload
{
namespace
{

constexpr VkImageUsageFlags STREAMED_IMAGE_USAGE =
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT;
constexpr VkImageUsageFlags OUTPUT_IMAGE_USAGE =
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
constexpr VkColorComponentFlags COLOR_WRITE_MASK =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
constexpr LLRenderContract::Extent2D RESIDENT_EXTENT{
    LLRenderContract::TEXTURE_UPLOAD_RESIDENT_WIDTH,
    LLRenderContract::TEXTURE_UPLOAD_RESIDENT_HEIGHT
};
constexpr LLRenderContract::Extent2D LOGICAL_EXTENT{
    LLRenderContract::TEXTURE_UPLOAD_LOGICAL_WIDTH,
    LLRenderContract::TEXTURE_UPLOAD_LOGICAL_HEIGHT
};
constexpr LLRenderContract::Extent2D OUTPUT_EXTENT{
    LLRenderContract::TEXTURE_UPLOAD_OUTPUT_WIDTH,
    LLRenderContract::TEXTURE_UPLOAD_OUTPUT_HEIGHT
};

bool nonzeroIdentity(const ShaderIdentityToken& identity)
{
    return std::any_of(identity.begin(), identity.end(),
                       [](std::uint8_t byte) { return byte != 0; });
}

bool sameExtent(LLRenderContract::Extent2D left, LLRenderContract::Extent2D right)
{
    return left.mWidth == right.mWidth && left.mHeight == right.mHeight;
}

bool identityComponents(const VkComponentMapping& components)
{
    return components.r == VK_COMPONENT_SWIZZLE_IDENTITY &&
           components.g == VK_COMPONENT_SWIZZLE_IDENTITY &&
           components.b == VK_COMPONENT_SWIZZLE_IDENTITY &&
           components.a == VK_COMPONENT_SWIZZLE_IDENTITY;
}

bool completeBuffer(const BufferBinding& binding)
{
    return binding.mBuffer != VK_NULL_HANDLE && binding.mMemory != VK_NULL_HANDLE &&
           binding.mOwnershipToken != 0 && binding.mMapped != nullptr && binding.mSize != 0 &&
           binding.mAllocationSize >= binding.mSize && binding.mMemoryOffset == 0 &&
           binding.mUsage != 0 && binding.mSharingMode != VK_SHARING_MODE_MAX_ENUM &&
           (binding.mMemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
}

bool completeImage(const ImageBinding& binding)
{
    return binding.mImage != VK_NULL_HANDLE && binding.mView != VK_NULL_HANDLE &&
           binding.mMemory != VK_NULL_HANDLE && binding.mOwnershipToken != 0 &&
           binding.mAllocationSize != 0 &&
           binding.mMemoryOffset == 0 && binding.mImageType != VK_IMAGE_TYPE_MAX_ENUM &&
           binding.mFormat != VK_FORMAT_UNDEFINED && binding.mResidentExtent.mWidth != 0 &&
           binding.mResidentExtent.mHeight != 0 && binding.mLogicalExtent.mWidth != 0 &&
           binding.mLogicalExtent.mHeight != 0 && binding.mMipLevels != 0 &&
           binding.mArrayLayers != 0 && binding.mSamples != VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM &&
           binding.mTiling != VK_IMAGE_TILING_MAX_ENUM && binding.mUsage != 0 &&
           binding.mSharingMode != VK_SHARING_MODE_MAX_ENUM && binding.mAspect != 0 &&
           binding.mViewType != VK_IMAGE_VIEW_TYPE_MAX_ENUM &&
           binding.mViewFormat != VK_FORMAT_UNDEFINED && binding.mViewRange.levelCount != 0 &&
           binding.mViewRange.layerCount != 0;
}

bool completePipeline(const PipelineBinding& binding)
{
    return !binding.mProgram.mName.empty() && binding.mPipeline != VK_NULL_HANDLE &&
           binding.mLayout != VK_NULL_HANDLE && binding.mRenderPass != VK_NULL_HANDLE &&
           binding.mFramebuffer != VK_NULL_HANDLE && binding.mDescriptorSet != VK_NULL_HANDLE &&
           binding.mOwnershipToken != 0 &&
           binding.mExtent.mWidth != 0 && binding.mExtent.mHeight != 0 &&
           binding.mColorView != VK_NULL_HANDLE && binding.mColorFormat != VK_FORMAT_UNDEFINED &&
           nonzeroIdentity(binding.mVertexShaderIdentity) &&
           nonzeroIdentity(binding.mFragmentShaderIdentity) &&
           !binding.mVertexEntryPoint.empty() && !binding.mFragmentEntryPoint.empty();
}

bool pristineLifecycle(const LifecycleLedger& ledger)
{
    return !ledger.mCompletionPending && ledger.mCompletionCount == 0 &&
           !ledger.mCompletedDestination && ledger.mCompletedRevision == 0 &&
           ledger.mCompletedFrame == 0 && ledger.mRetirementCount == 0 &&
           !ledger.mRetiredResource && ledger.mRetirementFrame == 0;
}

bool canonicalContext(const ExecutionContext& context)
{
    constexpr VkQueueFlags REQUIRED_QUEUE_FLAGS =
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
    return context.mDevice != VK_NULL_HANDLE && context.mCommandPool != VK_NULL_HANDLE &&
           context.mCommandBuffer != VK_NULL_HANDLE && context.mQueue != VK_NULL_HANDLE &&
           context.mOwnershipToken != 0 &&
           context.mQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
           (context.mQueueFamilyFlags & REQUIRED_QUEUE_FLAGS) == REQUIRED_QUEUE_FLAGS &&
           context.mQueueCount == 1 && context.mQueueIndex == 0 &&
           context.mCommandPoolQueueFamilyIndex == context.mQueueFamilyIndex &&
           (context.mCommandPoolFlags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) != 0 &&
           context.mCommandBufferLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY &&
           context.mRecordingAttemptCount && context.mSubmissionCount &&
           context.mRecordingAttemptCount != context.mSubmissionCount &&
           nonzeroIdentity(context.mRequiredVertexShaderIdentity) &&
           nonzeroIdentity(context.mRequiredFragmentShaderIdentity);
}

bool sameImageRange(const VkImageSubresourceRange& range, std::uint32_t mip_levels)
{
    return range.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT && range.baseMipLevel == 0 &&
           range.levelCount == mip_levels && range.baseArrayLayer == 0 &&
           range.layerCount == 1;
}

bool canonicalBuffer(const BufferBinding& binding, VkDeviceSize size,
                     VkBufferUsageFlags usage)
{
    return completeBuffer(binding) && binding.mSize == size &&
           binding.mAllocationSize >= size && binding.mCreateFlags == 0 &&
           binding.mUsage == usage && binding.mSharingMode == VK_SHARING_MODE_EXCLUSIVE;
}

bool canonicalImage(const ImageBinding& image, LLRenderContract::Extent2D resident_extent,
                    LLRenderContract::Extent2D logical_extent, std::uint32_t discard,
                    std::uint32_t mip_levels, VkImageUsageFlags usage)
{
    return completeImage(image) && image.mCreateFlags == 0 &&
           image.mImageType == VK_IMAGE_TYPE_2D && image.mFormat == VK_FORMAT_R8G8B8A8_UNORM &&
           sameExtent(image.mResidentExtent, resident_extent) &&
           sameExtent(image.mLogicalExtent, logical_extent) &&
           image.mResidentDiscard == discard && image.mMipLevels == mip_levels &&
           image.mArrayLayers == 1 && image.mSamples == VK_SAMPLE_COUNT_1_BIT &&
           image.mTiling == VK_IMAGE_TILING_OPTIMAL && image.mUsage == usage &&
           image.mSharingMode == VK_SHARING_MODE_EXCLUSIVE &&
           image.mAspect == VK_IMAGE_ASPECT_COLOR_BIT &&
           image.mLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
           image.mViewType == VK_IMAGE_VIEW_TYPE_2D &&
           image.mViewFormat == VK_FORMAT_R8G8B8A8_UNORM &&
           identityComponents(image.mViewComponents) &&
           sameImageRange(image.mViewRange, mip_levels);
}

bool canonicalSampler(const SamplerBinding& sampler)
{
    return sampler.mSampler != VK_NULL_HANDLE && sampler.mCreateFlags == 0 &&
           sampler.mMinFilter == VK_FILTER_LINEAR && sampler.mMagFilter == VK_FILTER_LINEAR &&
           sampler.mMipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR &&
           sampler.mAddressU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE &&
           sampler.mAddressV == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE &&
           sampler.mAddressW == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE &&
           sampler.mMipLodBias == 0.f && sampler.mAnisotropyEnable == VK_FALSE &&
           sampler.mMaxAnisotropy == 1.f && sampler.mCompareEnable == VK_FALSE &&
           sampler.mCompareOp == VK_COMPARE_OP_ALWAYS && sampler.mMinLod == 0.f &&
           sampler.mMaxLod == 2.f &&
           sampler.mBorderColor == VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK &&
           sampler.mUnnormalizedCoordinates == VK_FALSE;
}

bool canonicalStencilState(const VkStencilOpState& stencil)
{
    return stencil.failOp == VK_STENCIL_OP_KEEP && stencil.passOp == VK_STENCIL_OP_KEEP &&
           stencil.depthFailOp == VK_STENCIL_OP_KEEP &&
           stencil.compareOp == VK_COMPARE_OP_NEVER && stencil.compareMask == 0 &&
           stencil.writeMask == 0 && stencil.reference == 0;
}

bool canonicalPipeline(const PipelineBinding& pipeline, const ImageBinding& replacement,
                       const ImageBinding& output, const SamplerBinding& sampler,
                       const ExecutionContext& context)
{
    const SampledDescriptorBinding& descriptor = pipeline.mSampledDescriptor;
    const VertexBindingState& vertex_binding = pipeline.mVertexBinding;
    const VertexAttributeState& attribute = pipeline.mVertexAttribute;
    const RasterState& raster = pipeline.mRaster;
    const MultisampleState& multisample = pipeline.mMultisample;
    const DepthStencilState& depth = pipeline.mDepthStencil;
    const ColorTargetState& color = pipeline.mColorTarget;

    return pipeline.mProgram.mName == "contract.sample-texture" &&
           pipeline.mProgram.mVariant == 0 && sameExtent(pipeline.mExtent, OUTPUT_EXTENT) &&
           descriptor.mSet == 0 && descriptor.mBinding == 0 &&
           descriptor.mType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER &&
           descriptor.mStages == VK_SHADER_STAGE_FRAGMENT_BIT &&
           descriptor.mView == replacement.mView && descriptor.mSampler == sampler.mSampler &&
           descriptor.mLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
           pipeline.mColorView == output.mView &&
           pipeline.mColorFormat == VK_FORMAT_R8G8B8A8_UNORM &&
           pipeline.mColorSamples == VK_SAMPLE_COUNT_1_BIT &&
           pipeline.mColorLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE &&
           pipeline.mColorStoreOp == VK_ATTACHMENT_STORE_OP_STORE &&
           pipeline.mStencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE &&
           pipeline.mStencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE &&
           pipeline.mColorInitialLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
           pipeline.mColorFinalLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
           pipeline.mSubpassDependencyCount == 0 &&
           vertex_binding.mBinding == 0 && vertex_binding.mStride == 16 &&
           vertex_binding.mInputRate == VK_VERTEX_INPUT_RATE_VERTEX &&
           attribute.mLocation == 0 && attribute.mBinding == 0 &&
           attribute.mFormat == VK_FORMAT_R32G32B32_SFLOAT && attribute.mOffset == 0 &&
           raster.mTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST &&
           raster.mPrimitiveRestartEnable == VK_FALSE && raster.mDepthClampEnable == VK_FALSE &&
           raster.mRasterizerDiscardEnable == VK_FALSE &&
           raster.mPolygonMode == VK_POLYGON_MODE_FILL && raster.mCullMode == VK_CULL_MODE_NONE &&
           raster.mFrontFace == VK_FRONT_FACE_COUNTER_CLOCKWISE &&
           raster.mDepthBiasEnable == VK_FALSE && raster.mDepthBiasConstantFactor == 0.f &&
           raster.mDepthBiasClamp == 0.f && raster.mDepthBiasSlopeFactor == 0.f &&
           raster.mLineWidth == 1.f &&
           multisample.mRasterizationSamples == VK_SAMPLE_COUNT_1_BIT &&
           multisample.mSampleShadingEnable == VK_FALSE &&
           multisample.mMinSampleShading == 0.f &&
           multisample.mSampleMask == 0xffffffffu &&
           multisample.mAlphaToCoverageEnable == VK_FALSE &&
           multisample.mAlphaToOneEnable == VK_FALSE &&
           depth.mDepthTestEnable == VK_FALSE && depth.mDepthWriteEnable == VK_FALSE &&
           depth.mDepthCompareOp == VK_COMPARE_OP_LESS_OR_EQUAL &&
           depth.mDepthBoundsTestEnable == VK_FALSE && depth.mStencilTestEnable == VK_FALSE &&
           canonicalStencilState(depth.mFront) && canonicalStencilState(depth.mBack) &&
           depth.mMinDepthBounds == 0.f && depth.mMaxDepthBounds == 1.f &&
           color.mFormat == VK_FORMAT_R8G8B8A8_UNORM && color.mBlendEnable == VK_FALSE &&
           color.mSrcColorBlendFactor == VK_BLEND_FACTOR_ONE &&
           color.mDstColorBlendFactor == VK_BLEND_FACTOR_ZERO &&
           color.mColorBlendOp == VK_BLEND_OP_ADD &&
           color.mSrcAlphaBlendFactor == VK_BLEND_FACTOR_ONE &&
           color.mDstAlphaBlendFactor == VK_BLEND_FACTOR_ZERO &&
           color.mAlphaBlendOp == VK_BLEND_OP_ADD && color.mWriteMask == COLOR_WRITE_MASK &&
           pipeline.mLogicOpEnable == VK_FALSE && pipeline.mLogicOp == VK_LOGIC_OP_COPY &&
           pipeline.mBlendConstants == std::array<float, 4>{} &&
           pipeline.mDynamicViewport == VK_TRUE && pipeline.mDynamicScissor == VK_TRUE &&
           pipeline.mVertexShaderIdentity == context.mRequiredVertexShaderIdentity &&
           pipeline.mFragmentShaderIdentity == context.mRequiredFragmentShaderIdentity &&
           pipeline.mVertexEntryPoint == "main" && pipeline.mFragmentEntryPoint == "main";
}

bool distinctImageResources(const std::array<const ImageBinding*, 3>& images)
{
    for (std::size_t left = 0; left < images.size(); ++left)
    {
        for (std::size_t right = left + 1; right < images.size(); ++right)
        {
            if (images[left]->mImage == images[right]->mImage ||
                images[left]->mView == images[right]->mView ||
                images[left]->mMemory == images[right]->mMemory)
            {
                return false;
            }
        }
    }
    return true;
}

bool distinctBufferResources(const std::array<const BufferBinding*, 3>& buffers)
{
    for (std::size_t left = 0; left < buffers.size(); ++left)
    {
        for (std::size_t right = left + 1; right < buffers.size(); ++right)
        {
            if (buffers[left]->mBuffer == buffers[right]->mBuffer ||
                buffers[left]->mMemory == buffers[right]->mMemory ||
                buffers[left]->mMapped == buffers[right]->mMapped)
            {
                return false;
            }
        }
    }
    return true;
}

bool disjointMemory(const std::array<const BufferBinding*, 3>& buffers,
                    const std::array<const ImageBinding*, 3>& images)
{
    for (const BufferBinding* buffer : buffers)
    {
        for (const ImageBinding* image : images)
        {
            if (buffer->mMemory == image->mMemory)
            {
                return false;
            }
        }
    }
    return true;
}

std::array<std::uint8_t, SCREEN_TRIANGLE_BYTE_SIZE>
screenTriangleBytes(const LLRenderContract::TextureUploadFixture& fixture)
{
    std::array<std::uint8_t, SCREEN_TRIANGLE_BYTE_SIZE> bytes{};
    static_assert(sizeof(fixture.mScreenTriangle) == SCREEN_TRIANGLE_BYTE_SIZE);
    std::memcpy(bytes.data(), fixture.mScreenTriangle.data(), bytes.size());
    return bytes;
}

struct Prepared
{
    LLRenderContract::StreamingUploadInputs mInputs;
    LLRenderContract::TextureUploadFixture  mFixture;
    const BufferBinding*                    mScreen = nullptr;
    const ImageBinding*                     mOld = nullptr;
    const ImageBinding*                     mReplacement = nullptr;
    const ImageBinding*                     mOutput = nullptr;
    const SamplerBinding*                   mSampler = nullptr;
    const PipelineBinding*                  mPipeline = nullptr;
    const TransferResources*                mTransfer = nullptr;
    LifecycleLedger*                        mLifecycle = nullptr;
};

std::optional<Prepared> prepare(const LLRenderContract::FrameSnapshot& frame,
                                Registry& registry, const ExecutionContext& context,
                                std::string& error)
{
    auto inputs = LLRenderContract::decodeStreamingUploadFrame(frame);
    if (!inputs)
    {
        error = "packet is not the canonical streamed texture upload";
        return std::nullopt;
    }
    if (inputs->mFrame != LLRenderContract::TEXTURE_UPLOAD_DIAGNOSTIC_FRAME ||
        inputs->mHandles != LLRenderContract::StreamingUploadHandles{})
    {
        error = "packet frame or handles do not match the diagnostic case";
        return std::nullopt;
    }
    if (!canonicalContext(context))
    {
        error = "execution context is incomplete";
        return std::nullopt;
    }

    Prepared result;
    result.mInputs = std::move(*inputs);
    result.mFixture = LLRenderContract::makeTextureUploadFixture();
    const LLRenderContract::StreamingUploadHandles& handles = result.mInputs.mHandles;
    result.mScreen = registry.resolve(handles.mScreenTriangle);
    result.mOld = registry.resolveRegisteredImage(handles.mOldImage);
    result.mReplacement = registry.resolveRegisteredImage(handles.mReplacementImage);
    result.mOutput = registry.resolveOutput(handles.mOutput);
    result.mSampler = registry.resolve(handles.mSampler);
    result.mPipeline = registry.resolve(handles.mPipeline, frame.mPipelines.front().mProgram);
    result.mTransfer = registry.transferResources();
    result.mLifecycle = registry.lifecycle();

    if (!result.mScreen || !result.mOld || !result.mReplacement || !result.mOutput ||
        !result.mSampler || !result.mPipeline || !result.mTransfer || !result.mLifecycle)
    {
        error = "registry cannot resolve every exact resource generation";
        return std::nullopt;
    }
    if (result.mScreen->mOwnershipToken != context.mOwnershipToken ||
        result.mOld->mOwnershipToken != context.mOwnershipToken ||
        result.mReplacement->mOwnershipToken != context.mOwnershipToken ||
        result.mOutput->mOwnershipToken != context.mOwnershipToken ||
        result.mSampler->mOwnershipToken != context.mOwnershipToken ||
        result.mPipeline->mOwnershipToken != context.mOwnershipToken ||
        result.mTransfer->mStaging.mOwnershipToken != context.mOwnershipToken ||
        result.mTransfer->mReadback.mOwnershipToken != context.mOwnershipToken)
    {
        error = "native resources do not share the execution context owner";
        return std::nullopt;
    }
    if (result.mLifecycle->mCurrentImage != handles.mOldImage ||
        result.mLifecycle->mLastRevision != LLRenderContract::TEXTURE_UPLOAD_PRIOR_REVISION ||
        result.mInputs.mRevision <= result.mLifecycle->mLastRevision ||
        !pristineLifecycle(*result.mLifecycle) || !registry.isResolvable(handles.mOldImage) ||
        registry.isResolvable(handles.mReplacementImage))
    {
        error = "lifecycle ledger is not the unpublished replacement state";
        return std::nullopt;
    }

    const std::array<std::uint8_t, SCREEN_TRIANGLE_BYTE_SIZE> expected_screen =
        screenTriangleBytes(result.mFixture);
    if (!canonicalBuffer(*result.mScreen, SCREEN_TRIANGLE_BYTE_SIZE,
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) ||
        !result.mScreen->mHasFixtureBytes ||
        result.mScreen->mFixtureBytes != expected_screen ||
        std::memcmp(result.mScreen->mMapped, expected_screen.data(), expected_screen.size()) != 0)
    {
        error = "screen-triangle buffer metadata or bytes do not match the fixture";
        return std::nullopt;
    }
    if (!canonicalBuffer(result.mTransfer->mStaging, STAGING_BYTE_SIZE,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT) ||
        result.mTransfer->mStaging.mHasFixtureBytes ||
        !canonicalBuffer(result.mTransfer->mReadback, READBACK_BYTE_SIZE,
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT) ||
        result.mTransfer->mReadback.mHasFixtureBytes)
    {
        error = "staging or readback buffer metadata is not canonical";
        return std::nullopt;
    }

    if (!canonicalImage(*result.mOld, RESIDENT_EXTENT, LOGICAL_EXTENT,
                        LLRenderContract::TEXTURE_UPLOAD_RESIDENT_DISCARD,
                        LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS, STREAMED_IMAGE_USAGE) ||
        !result.mOld->mHasPreExecutionMipSnapshot ||
        result.mOld->mPreExecutionMipRGBA8 != result.mFixture.mOldMipRGBA8 ||
        !canonicalImage(*result.mReplacement, RESIDENT_EXTENT, LOGICAL_EXTENT,
                        LLRenderContract::TEXTURE_UPLOAD_RESIDENT_DISCARD,
                        LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS, STREAMED_IMAGE_USAGE) ||
        result.mReplacement->mHasPreExecutionMipSnapshot ||
        !canonicalImage(*result.mOutput, OUTPUT_EXTENT, OUTPUT_EXTENT, 0, 1,
                        OUTPUT_IMAGE_USAGE) || result.mOutput->mHasPreExecutionMipSnapshot)
    {
        error = "image metadata does not match the streamed upload";
        return std::nullopt;
    }
    const std::array<const ImageBinding*, 3> images{
        result.mOld, result.mReplacement, result.mOutput
    };
    const std::array<const BufferBinding*, 3> buffers{
        result.mScreen, &result.mTransfer->mStaging, &result.mTransfer->mReadback
    };
    if (!distinctImageResources(images) || !distinctBufferResources(buffers) ||
        !disjointMemory(buffers, images))
    {
        error = "diagnostic native resources alias";
        return std::nullopt;
    }
    if (!canonicalSampler(*result.mSampler))
    {
        error = "sampler metadata does not match the streamed upload";
        return std::nullopt;
    }
    if (!completePipeline(*result.mPipeline) ||
        !canonicalPipeline(*result.mPipeline, *result.mReplacement, *result.mOutput,
                           *result.mSampler, context))
    {
        error = "pipeline resources, state, or shader identity do not match";
        return std::nullopt;
    }
    return result;
}

VkImageMemoryBarrier imageBarrier(const ImageBinding& image, VkImageLayout old_layout,
                                  VkImageLayout new_layout, VkAccessFlags source_access,
                                  VkAccessFlags destination_access, std::uint32_t base_mip,
                                  std::uint32_t mip_count)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = source_access;
    barrier.dstAccessMask = destination_access;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.mImage;
    barrier.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT, base_mip, mip_count, 0, 1
    };
    return barrier;
}

VkBufferMemoryBarrier bufferBarrier(const BufferBinding& buffer,
                                    VkAccessFlags source_access,
                                    VkAccessFlags destination_access)
{
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = source_access;
    barrier.dstAccessMask = destination_access;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = buffer.mBuffer;
    barrier.offset = 0;
    barrier.size = buffer.mSize;
    return barrier;
}

VkBufferImageCopy imageCopy(VkDeviceSize offset, std::uint32_t mip,
                            std::uint32_t width, std::uint32_t height,
                            std::uint32_t destination_y = 0)
{
    VkBufferImageCopy copy{};
    copy.bufferOffset = offset;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mip, 0, 1 };
    copy.imageOffset = { 0, static_cast<std::int32_t>(destination_y), 0 };
    copy.imageExtent = { width, height, 1 };
    return copy;
}

VkImageBlit mipBlit(std::uint32_t source_mip, std::uint32_t source_width,
                    std::uint32_t source_height, std::uint32_t destination_width,
                    std::uint32_t destination_height)
{
    VkImageBlit blit{};
    blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, source_mip, 0, 1 };
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { static_cast<std::int32_t>(source_width),
                           static_cast<std::int32_t>(source_height), 1 };
    blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, source_mip + 1, 0, 1 };
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = { static_cast<std::int32_t>(destination_width),
                           static_cast<std::int32_t>(destination_height), 1 };
    return blit;
}

bool mappedRangeOperation(VkDevice device, const BufferBinding& buffer, bool flush,
                          std::string& error)
{
    if ((buffer.mMemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0)
    {
        return true;
    }
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = buffer.mMemory;
    range.offset = 0;
    range.size = VK_WHOLE_SIZE;
    const VkResult result = flush ? vkFlushMappedMemoryRanges(device, 1, &range)
                                  : vkInvalidateMappedMemoryRanges(device, 1, &range);
    if (result != VK_SUCCESS)
    {
        error = flush ? "vkFlushMappedMemoryRanges failed"
                      : "vkInvalidateMappedMemoryRanges failed";
        return false;
    }
    return true;
}

std::vector<std::uint8_t> normalizedSource(
    const LLRenderContract::StreamingUploadInputs& inputs)
{
    constexpr std::size_t tight_row =
        LLRenderContract::TEXTURE_UPLOAD_RESIDENT_WIDTH *
        LLRenderContract::TEXTURE_UPLOAD_CHANNELS;
    std::vector<std::uint8_t> normalized(
        tight_row * LLRenderContract::TEXTURE_UPLOAD_RESIDENT_HEIGHT);
    for (std::size_t source_y = 0;
         source_y < LLRenderContract::TEXTURE_UPLOAD_RESIDENT_HEIGHT; ++source_y)
    {
        const std::size_t destination_y =
            LLRenderContract::TEXTURE_UPLOAD_RESIDENT_HEIGHT - 1 - source_y;
        std::memcpy(normalized.data() + destination_y * tight_row,
                    inputs.mPixels.data() + source_y *
                        LLRenderContract::TEXTURE_UPLOAD_ROW_PITCH,
                    tight_row);
    }
    return normalized;
}

bool hasMultipleChangedTexels(const std::uint8_t* before, std::size_t size,
                              const std::vector<std::uint8_t>& after)
{
    if (after.size() != size || size < LLRenderContract::TEXTURE_UPLOAD_CHANNELS * 2 ||
        size % LLRenderContract::TEXTURE_UPLOAD_CHANNELS != 0)
    {
        return false;
    }
    std::optional<std::size_t> first_changed;
    for (std::size_t offset = 0; offset < size;
         offset += LLRenderContract::TEXTURE_UPLOAD_CHANNELS)
    {
        if (std::memcmp(before + offset, after.data() + offset,
                        LLRenderContract::TEXTURE_UPLOAD_CHANNELS) == 0)
        {
            continue;
        }
        if (!first_changed)
        {
            first_changed = offset;
            continue;
        }
        if (std::memcmp(after.data() + *first_changed, after.data() + offset,
                        LLRenderContract::TEXTURE_UPLOAD_CHANNELS) != 0)
        {
            return true;
        }
    }
    return false;
}

bool validateContent(const Prepared& prepared, const ExecutionResult& completed)
{
    if (completed.mMipRGBA8[0] != normalizedSource(prepared.mInputs) ||
        completed.mSampledRGBA8 != completed.mMipRGBA8[1])
    {
        return false;
    }
    for (std::size_t mip = 0; mip < LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS; ++mip)
    {
        const std::size_t offset = LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_OFFSETS[mip];
        const std::size_t size = LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_SIZES[mip];
        if (!hasMultipleChangedTexels(
                prepared.mFixture.mReplacementSentinelMipRGBA8.data() + offset,
                size, completed.mMipRGBA8[mip]))
        {
            return false;
        }
    }
    return hasMultipleChangedTexels(prepared.mFixture.mOutputSentinelRGBA8.data(),
                                    prepared.mFixture.mOutputSentinelRGBA8.size(),
                                    completed.mSampledRGBA8);
}

} // namespace

bool Registry::addScreenTriangle(LLRenderContract::BufferHandle handle,
                                 BufferBinding binding)
{
    if (!handle || mHasScreenTriangle || !completeBuffer(binding) ||
        (mHasTransferResources &&
         (binding.mBuffer == mTransferResources.mStaging.mBuffer ||
          binding.mBuffer == mTransferResources.mReadback.mBuffer ||
          binding.mMemory == mTransferResources.mStaging.mMemory ||
          binding.mMemory == mTransferResources.mReadback.mMemory)))
    {
        return false;
    }
    mScreenHandle = handle;
    mScreenTriangle = std::move(binding);
    mHasScreenTriangle = true;
    return true;
}

bool Registry::addImageGenerations(LLRenderContract::ImageHandle old_handle,
                                   ImageBinding old_image,
                                   LLRenderContract::ImageHandle replacement_handle,
                                   ImageBinding replacement_image)
{
    if (!old_handle || !replacement_handle || mHasImageGenerations ||
        !completeImage(old_image) || !completeImage(replacement_image) ||
        !old_image.mHasPreExecutionMipSnapshot ||
        replacement_image.mHasPreExecutionMipSnapshot ||
        old_handle.mIndex != replacement_handle.mIndex ||
        old_handle.mGeneration == std::numeric_limits<std::uint32_t>::max() ||
        replacement_handle.mGeneration != old_handle.mGeneration + 1 ||
        old_image.mImage == replacement_image.mImage || old_image.mView == replacement_image.mView ||
        old_image.mMemory == replacement_image.mMemory ||
        (mHasOutput && (mOutputHandle.mIndex == old_handle.mIndex ||
                        mOutput.mImage == old_image.mImage ||
                        mOutput.mImage == replacement_image.mImage ||
                        mOutput.mView == old_image.mView ||
                        mOutput.mView == replacement_image.mView ||
                        mOutput.mMemory == old_image.mMemory ||
                        mOutput.mMemory == replacement_image.mMemory)))
    {
        return false;
    }
    mOldHandle = old_handle;
    mOldImage = std::move(old_image);
    mReplacementHandle = replacement_handle;
    mReplacementImage = std::move(replacement_image);
    mHasImageGenerations = true;
    return true;
}

bool Registry::addOutput(LLRenderContract::ImageHandle handle, ImageBinding output)
{
    if (!handle || mHasOutput || !completeImage(output) ||
        output.mHasPreExecutionMipSnapshot ||
        (mHasImageGenerations &&
         (handle.mIndex == mOldHandle.mIndex || output.mImage == mOldImage.mImage ||
          output.mImage == mReplacementImage.mImage || output.mView == mOldImage.mView ||
          output.mView == mReplacementImage.mView || output.mMemory == mOldImage.mMemory ||
          output.mMemory == mReplacementImage.mMemory)))
    {
        return false;
    }
    mOutputHandle = handle;
    mOutput = std::move(output);
    mHasOutput = true;
    return true;
}

bool Registry::addSampler(LLRenderContract::SamplerHandle handle,
                          SamplerBinding sampler)
{
    if (!handle || mHasSampler || sampler.mSampler == VK_NULL_HANDLE ||
        sampler.mOwnershipToken == 0)
    {
        return false;
    }
    mSamplerHandle = handle;
    mSampler = std::move(sampler);
    mHasSampler = true;
    return true;
}

bool Registry::addPipeline(LLRenderContract::PipelineHandle handle,
                           PipelineBinding pipeline)
{
    if (!handle || mHasPipeline || !completePipeline(pipeline))
    {
        return false;
    }
    mPipelineHandle = handle;
    mPipeline = std::move(pipeline);
    mHasPipeline = true;
    return true;
}

bool Registry::addTransferResources(TransferResources resources)
{
    if (mHasTransferResources || !completeBuffer(resources.mStaging) ||
        !completeBuffer(resources.mReadback) ||
        resources.mStaging.mBuffer == resources.mReadback.mBuffer ||
        resources.mStaging.mMemory == resources.mReadback.mMemory ||
        resources.mStaging.mMapped == resources.mReadback.mMapped ||
        (mHasScreenTriangle &&
         (resources.mStaging.mBuffer == mScreenTriangle.mBuffer ||
          resources.mReadback.mBuffer == mScreenTriangle.mBuffer ||
          resources.mStaging.mMemory == mScreenTriangle.mMemory ||
          resources.mReadback.mMemory == mScreenTriangle.mMemory)))
    {
        return false;
    }
    mTransferResources = std::move(resources);
    mHasTransferResources = true;
    return true;
}

bool Registry::addLifecycle(LifecycleLedger* ledger)
{
    if (!ledger || mLifecycle || !pristineLifecycle(*ledger))
    {
        return false;
    }
    mLifecycle = ledger;
    return true;
}

const BufferBinding* Registry::resolve(LLRenderContract::BufferHandle handle) const
{
    return mHasScreenTriangle && handle == mScreenHandle ? &mScreenTriangle : nullptr;
}

const ImageBinding* Registry::resolveRegisteredImage(
    LLRenderContract::ImageHandle handle) const
{
    if (!mHasImageGenerations)
    {
        return nullptr;
    }
    if (handle == mOldHandle)
    {
        return &mOldImage;
    }
    return handle == mReplacementHandle ? &mReplacementImage : nullptr;
}

const ImageBinding* Registry::resolveOutput(LLRenderContract::ImageHandle handle) const
{
    return mHasOutput && handle == mOutputHandle ? &mOutput : nullptr;
}

const SamplerBinding* Registry::resolve(LLRenderContract::SamplerHandle handle) const
{
    return mHasSampler && handle == mSamplerHandle ? &mSampler : nullptr;
}

const PipelineBinding* Registry::resolve(
    LLRenderContract::PipelineHandle handle,
    const LLRenderContract::ShaderProgramKey& program) const
{
    return mHasPipeline && handle == mPipelineHandle &&
                   program.mName == mPipeline.mProgram.mName &&
                   program.mVariant == mPipeline.mProgram.mVariant
               ? &mPipeline
               : nullptr;
}

const TransferResources* Registry::transferResources() const
{
    return mHasTransferResources ? &mTransferResources : nullptr;
}

LifecycleLedger* Registry::lifecycle() const
{
    return mLifecycle;
}

bool Registry::isResolvable(LLRenderContract::ImageHandle handle) const
{
    return mLifecycle && mLifecycle->mCurrentImage == handle &&
           resolveRegisteredImage(handle) != nullptr;
}

bool execute(const LLRenderContract::FrameSnapshot& frame, Registry& registry,
             const ExecutionContext& context, ExecutionResult& result,
             std::string* error)
{
    std::string local_error;
    if (error)
    {
        error->clear();
    }
    const auto fail = [error, &local_error](const char* fallback)
    {
        if (local_error.empty())
        {
            local_error = fallback;
        }
        if (error)
        {
            *error = local_error;
        }
        return false;
    };

    const auto prepared = prepare(frame, registry, context, local_error);
    if (!prepared)
    {
        return fail("packet, registry, or context preflight failed");
    }

    const BufferBinding& staging = prepared->mTransfer->mStaging;
    const BufferBinding& readback = prepared->mTransfer->mReadback;
    // From here onward failures may leave private scratch or GPU resources
    // changed, but the borrowed lifecycle and caller artifact remain
    // unpublished. Vulkan failure/device-loss paths are not rollback-safe.
    std::memcpy(staging.mMapped, prepared->mInputs.mPixels.data(), STAGING_BYTE_SIZE);
    if (!mappedRangeOperation(context.mDevice, staging, true, local_error))
    {
        return fail("staging flush failed");
    }

    ++*context.mRecordingAttemptCount;
    VkResult vk_result = vkResetCommandBuffer(context.mCommandBuffer, 0);
    if (vk_result != VK_SUCCESS)
    {
        local_error = "vkResetCommandBuffer failed";
        return fail("command reset failed");
    }
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vk_result = vkBeginCommandBuffer(context.mCommandBuffer, &begin);
    if (vk_result != VK_SUCCESS)
    {
        local_error = "vkBeginCommandBuffer failed";
        return fail("command begin failed");
    }

    const VkBufferMemoryBarrier staging_barrier =
        bufferBarrier(staging, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    vkCmdPipelineBarrier(context.mCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                         &staging_barrier, 0, nullptr);

    const VkImageMemoryBarrier replacement_destination = imageBarrier(
        *prepared->mReplacement, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
        0, LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS);
    vkCmdPipelineBarrier(context.mCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                         1, &replacement_destination);

    const VkImageMemoryBarrier output_attachment = imageBarrier(
        *prepared->mOutput, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, 1);
    vkCmdPipelineBarrier(context.mCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &output_attachment);

    std::array<VkBufferImageCopy, LLRenderContract::TEXTURE_UPLOAD_RESIDENT_HEIGHT>
        upload_rows{};
    for (std::uint32_t row = 0;
         row < LLRenderContract::TEXTURE_UPLOAD_RESIDENT_HEIGHT; ++row)
    {
        upload_rows[row] = imageCopy(UPLOAD_ROW_SOURCE_OFFSETS[row], 0,
                                     LLRenderContract::TEXTURE_UPLOAD_RESIDENT_WIDTH,
                                     1, row);
    }
    vkCmdCopyBufferToImage(context.mCommandBuffer, staging.mBuffer,
                           prepared->mReplacement->mImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<std::uint32_t>(upload_rows.size()),
                           upload_rows.data());

    const VkImageMemoryBarrier mip_zero_source = imageBarrier(
        *prepared->mReplacement, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, 0, 1);
    vkCmdPipelineBarrier(context.mCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                         1, &mip_zero_source);
    const VkImageBlit first_blit = mipBlit(0, 8, 4, 4, 2);
    vkCmdBlitImage(context.mCommandBuffer, prepared->mReplacement->mImage,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   prepared->mReplacement->mImage,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &first_blit,
                   VK_FILTER_LINEAR);

    const VkImageMemoryBarrier mip_one_source = imageBarrier(
        *prepared->mReplacement, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, 1, 1);
    vkCmdPipelineBarrier(context.mCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                         1, &mip_one_source);
    const VkImageBlit second_blit = mipBlit(1, 4, 2, 2, 1);
    vkCmdBlitImage(context.mCommandBuffer, prepared->mReplacement->mImage,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   prepared->mReplacement->mImage,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &second_blit,
                   VK_FILTER_LINEAR);

    std::array<VkImageMemoryBarrier, 2> sampled_barriers{
        imageBarrier(*prepared->mReplacement,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, 0, 2),
        imageBarrier(*prepared->mReplacement,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, 2, 1)
    };
    vkCmdPipelineBarrier(context.mCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, static_cast<std::uint32_t>(sampled_barriers.size()),
                         sampled_barriers.data());

    VkRenderPassBeginInfo render_pass{};
    render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass.renderPass = prepared->mPipeline->mRenderPass;
    render_pass.framebuffer = prepared->mPipeline->mFramebuffer;
    render_pass.renderArea = { { 0, 0 },
                               { LLRenderContract::TEXTURE_UPLOAD_OUTPUT_WIDTH,
                                 LLRenderContract::TEXTURE_UPLOAD_OUTPUT_HEIGHT } };
    render_pass.clearValueCount = 0;
    render_pass.pClearValues = nullptr;
    vkCmdBeginRenderPass(context.mCommandBuffer, &render_pass,
                         VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{
        0.f, static_cast<float>(LLRenderContract::TEXTURE_UPLOAD_OUTPUT_HEIGHT),
        static_cast<float>(LLRenderContract::TEXTURE_UPLOAD_OUTPUT_WIDTH),
        -static_cast<float>(LLRenderContract::TEXTURE_UPLOAD_OUTPUT_HEIGHT), 0.f, 1.f
    };
    VkRect2D scissor{
        { 0, 0 },
        { LLRenderContract::TEXTURE_UPLOAD_OUTPUT_WIDTH,
          LLRenderContract::TEXTURE_UPLOAD_OUTPUT_HEIGHT }
    };
    vkCmdSetViewport(context.mCommandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(context.mCommandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(context.mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      prepared->mPipeline->mPipeline);
    vkCmdBindDescriptorSets(context.mCommandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            prepared->mPipeline->mLayout, 0, 1,
                            &prepared->mPipeline->mDescriptorSet, 0, nullptr);
    const VkBuffer vertex_buffer = prepared->mScreen->mBuffer;
    constexpr VkDeviceSize vertex_offset = 0;
    vkCmdBindVertexBuffers(context.mCommandBuffer, 0, 1, &vertex_buffer,
                           &vertex_offset);
    vkCmdDraw(context.mCommandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(context.mCommandBuffer);

    std::array<VkImageMemoryBarrier, 2> readback_sources{
        imageBarrier(*prepared->mReplacement,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                     0, LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS),
        imageBarrier(*prepared->mOutput,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_ACCESS_TRANSFER_READ_BIT, 0, 1)
    };
    vkCmdPipelineBarrier(context.mCommandBuffer,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                         static_cast<std::uint32_t>(readback_sources.size()),
                         readback_sources.data());

    std::array<VkBufferImageCopy, LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS>
        mip_copies{};
    std::uint32_t mip_width = LLRenderContract::TEXTURE_UPLOAD_RESIDENT_WIDTH;
    std::uint32_t mip_height = LLRenderContract::TEXTURE_UPLOAD_RESIDENT_HEIGHT;
    for (std::uint32_t mip = 0; mip < LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS;
         ++mip)
    {
        mip_copies[mip] = imageCopy(
            LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_OFFSETS[mip], mip,
            mip_width, mip_height);
        mip_width = std::max(1u, mip_width / 2);
        mip_height = std::max(1u, mip_height / 2);
    }
    vkCmdCopyImageToBuffer(context.mCommandBuffer,
                           prepared->mReplacement->mImage,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.mBuffer,
                           static_cast<std::uint32_t>(mip_copies.size()),
                           mip_copies.data());
    const VkBufferImageCopy output_copy = imageCopy(
        OUTPUT_READBACK_BYTE_OFFSET, 0,
        LLRenderContract::TEXTURE_UPLOAD_OUTPUT_WIDTH,
        LLRenderContract::TEXTURE_UPLOAD_OUTPUT_HEIGHT);
    vkCmdCopyImageToBuffer(context.mCommandBuffer, prepared->mOutput->mImage,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.mBuffer,
                           1, &output_copy);

    std::array<VkImageMemoryBarrier, 2> final_images{
        imageBarrier(*prepared->mReplacement,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                     0, LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS),
        imageBarrier(*prepared->mOutput,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, 0, 1)
    };
    const VkBufferMemoryBarrier host_readback = bufferBarrier(
        readback, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT);
    vkCmdPipelineBarrier(context.mCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                             VK_PIPELINE_STAGE_HOST_BIT,
                         0, 0, nullptr, 1, &host_readback,
                         static_cast<std::uint32_t>(final_images.size()),
                         final_images.data());

    vk_result = vkEndCommandBuffer(context.mCommandBuffer);
    if (vk_result != VK_SUCCESS)
    {
        local_error = "vkEndCommandBuffer failed";
        return fail("command end failed");
    }
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &context.mCommandBuffer;
    vk_result = vkQueueSubmit(context.mQueue, 1, &submit, VK_NULL_HANDLE);
    if (vk_result != VK_SUCCESS)
    {
        local_error = "vkQueueSubmit failed";
        return fail("queue submission failed");
    }
    ++*context.mSubmissionCount;
    vk_result = vkQueueWaitIdle(context.mQueue);
    if (vk_result != VK_SUCCESS)
    {
        local_error = "vkQueueWaitIdle failed";
        return fail("queue completion failed");
    }
    if (!mappedRangeOperation(context.mDevice, readback, false, local_error))
    {
        return fail("readback invalidation failed");
    }

    const auto* bytes = static_cast<const std::uint8_t*>(readback.mMapped);
    ExecutionResult completed = LLRenderContract::makeTextureUploadArtifact();
    completed.mPriorRevision = prepared->mLifecycle->mLastRevision;
    completed.mRevision = prepared->mInputs.mRevision;
    for (std::size_t mip = 0; mip < LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS; ++mip)
    {
        const std::size_t offset = LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_OFFSETS[mip];
        const std::size_t size = LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_SIZES[mip];
        completed.mMipRGBA8[mip].assign(bytes + offset, bytes + offset + size);
    }

    constexpr std::size_t output_row_size =
        LLRenderContract::TEXTURE_UPLOAD_OUTPUT_WIDTH *
        LLRenderContract::TEXTURE_UPLOAD_CHANNELS;
    completed.mSampledRGBA8.resize(LLRenderContract::TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT);
    for (std::size_t destination_row = 0;
         destination_row < LLRenderContract::TEXTURE_UPLOAD_OUTPUT_HEIGHT;
         ++destination_row)
    {
        const std::size_t source_row =
            LLRenderContract::TEXTURE_UPLOAD_OUTPUT_HEIGHT - 1 - destination_row;
        std::memcpy(completed.mSampledRGBA8.data() + destination_row * output_row_size,
                    bytes + OUTPUT_READBACK_BYTE_OFFSET + source_row * output_row_size,
                    output_row_size);
    }

    LifecycleLedger next = *prepared->mLifecycle;
    next.mCurrentImage = prepared->mInputs.mHandles.mReplacementImage;
    next.mLastRevision = prepared->mInputs.mRevision;
    next.mCompletionPending = false;
    ++next.mCompletionCount;
    next.mCompletedDestination = prepared->mInputs.mHandles.mReplacementImage;
    next.mCompletedRevision = prepared->mInputs.mRevision;
    next.mCompletedFrame = prepared->mInputs.mFrame;
    ++next.mRetirementCount;
    next.mRetiredResource = prepared->mInputs.mHandles.mOldImage;
    next.mRetirementFrame = prepared->mInputs.mFrame;

    completed.mCompletionCount = next.mCompletionCount;
    completed.mCompletedDestination = next.mCompletedDestination;
    completed.mCompletedRevision = next.mCompletedRevision;
    completed.mCompletedFrame = next.mCompletedFrame;
    completed.mRetirementCount = next.mRetirementCount;
    completed.mRetiredResource = next.mRetiredResource;
    completed.mRetirementFrame = next.mRetirementFrame;
    completed.mOldResolvableBefore = true;
    completed.mOldResolvableAfter = false;
    completed.mReplacementResolvableAfter = true;

    if (!LLRenderContract::validateTextureUploadArtifact(completed, &local_error) ||
        !validateContent(*prepared, completed))
    {
        if (local_error.empty())
        {
            local_error = "exact upload, mip, or sample content validation failed";
        }
        return fail("artifact validation failed");
    }

    // Publication is the only logical ownership mutation and follows command
    // completion, host visibility, exact readback validation, and artifact
    // validation. Preflight validated a queue-idle old-image snapshot and
    // disjoint image, view, and memory identities. The sole recorded command
    // never names that old generation, which proves its snapshot remains
    // unchanged without expanding the fixed 200-byte readback.
    *prepared->mLifecycle = next;
    result = std::move(completed);
    if (error)
    {
        error->clear();
    }
    return true;
}

} // namespace LLRenderVulkanTextureUpload
