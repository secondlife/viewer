/**
 * @file llrendervulkantonemap.cpp
 * @brief Vulkan replay of the canonical tonemap packet.
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

#include "llrendervulkantonemap.h"

#include <algorithm>
#include <cstring>
#include <optional>
#include <utility>

namespace LLRenderVulkanTonemap
{
namespace
{

template<typename Entry, typename Handle>
bool hasIndex(const std::vector<Entry>& entries, Handle handle)
{
    return std::any_of(entries.begin(), entries.end(),
                       [handle](const Entry& entry) { return entry.mHandle.mIndex == handle.mIndex; });
}

template<typename Entry, typename Handle, typename Binding>
bool addEntry(std::vector<Entry>& entries, Handle handle, Binding binding, bool complete)
{
    if (!handle || !complete || hasIndex(entries, handle))
    {
        return false;
    }
    entries.push_back({ handle, std::move(binding) });
    return true;
}

template<typename Entry, typename Handle>
const auto* resolveEntry(const std::vector<Entry>& entries, Handle handle)
{
    const auto found = std::find_if(entries.begin(), entries.end(),
                                    [handle](const Entry& entry) { return entry.mHandle == handle; });
    return found == entries.end() ? nullptr : &found->mBinding;
}

bool sameExtent(LLRenderContract::Extent2D left, LLRenderContract::Extent2D right)
{
    return left.mWidth == right.mWidth && left.mHeight == right.mHeight;
}

std::optional<VkFormat> vkFormat(LLRenderContract::PixelFormat format)
{
    switch (format)
    {
        case LLRenderContract::PixelFormat::RGBA8Unorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case LLRenderContract::PixelFormat::R16Float:
            return VK_FORMAT_R16_SFLOAT;
        case LLRenderContract::PixelFormat::RGBA16Float:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        default:
            return std::nullopt;
    }
}

struct Prepared
{
    LLRenderContract::TonemapInputs mInputs;
    const BufferBinding* mTriangle = nullptr;
    const ImageBinding* mScene = nullptr;
    const ImageBinding* mExposure = nullptr;
    const ImageBinding* mDestination = nullptr;
    const PipelineBinding* mPipeline = nullptr;
    const LLRenderContract::Draw* mDraw = nullptr;
};

std::optional<Prepared> prepare(const LLRenderContract::FrameSnapshot& frame,
                                const Registry& registry, const ExecutionContext& context,
                                std::string& error)
{
    const auto inputs = LLRenderContract::decodeTonemapFrame(frame);
    if (!inputs)
    {
        error = "packet is not the canonical tonemap frame";
        return std::nullopt;
    }
    if (context.mDevice == VK_NULL_HANDLE || context.mCommandBuffer == VK_NULL_HANDLE ||
        context.mQueue == VK_NULL_HANDLE || !context.mSubmissionCount)
    {
        error = "execution context is incomplete";
        return std::nullopt;
    }

    Prepared result;
    result.mInputs = *inputs;
    result.mTriangle = registry.resolve(inputs->mHandles.mScreenTriangle);
    result.mScene = registry.resolve(inputs->mHandles.mScene);
    result.mExposure = registry.resolve(inputs->mHandles.mExposure);
    result.mDestination = registry.resolve(inputs->mHandles.mDestination);
    result.mPipeline = registry.resolve(inputs->mHandles.mPipeline, frame.mPipelines.front().mProgram);
    result.mDraw = &std::get<LLRenderContract::Draw>(frame.mPasses.front().mDraws.front());

    const SamplerBinding* point = registry.resolve(inputs->mHandles.mPointSampler);
    const SamplerBinding* linear = registry.resolve(inputs->mHandles.mLinearSampler);
    if (!result.mTriangle || !result.mScene || !result.mExposure || !result.mDestination ||
        !result.mPipeline || !point || !linear)
    {
        error = "registry cannot resolve an exact live resource generation";
        return std::nullopt;
    }

    const auto destination_format = vkFormat(inputs->mDestinationFormat);
    if (!destination_format || result.mTriangle->mSize < 48 ||
        (result.mTriangle->mUsage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) == 0 ||
        result.mScene->mFormat != VK_FORMAT_R16G16B16A16_SFLOAT ||
        !sameExtent(result.mScene->mExtent, inputs->mSourceExtent) ||
        (result.mScene->mUsage & VK_IMAGE_USAGE_SAMPLED_BIT) == 0 ||
        result.mExposure->mFormat != VK_FORMAT_R16_SFLOAT ||
        !sameExtent(result.mExposure->mExtent, { 1, 1 }) ||
        (result.mExposure->mUsage & VK_IMAGE_USAGE_SAMPLED_BIT) == 0 ||
        result.mDestination->mFormat != *destination_format ||
        !sameExtent(result.mDestination->mExtent, inputs->mDestinationExtent) ||
        (result.mDestination->mUsage &
         (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)) !=
            (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) ||
        result.mScene->mImage == result.mDestination->mImage ||
        result.mExposure->mImage == result.mDestination->mImage)
    {
        error = "registry image or buffer metadata does not match the packet";
        return std::nullopt;
    }

    if (point->mMinFilter != LLRenderContract::Filter::Nearest ||
        point->mMagFilter != LLRenderContract::Filter::Nearest ||
        point->mAddressU != LLRenderContract::AddressMode::Mirror ||
        point->mAddressV != LLRenderContract::AddressMode::Mirror ||
        point->mMipFilter != LLRenderContract::MipFilter::Disabled ||
        point->mMaxAnisotropy != 1.f || point->mAnisotropyEnabled ||
        linear->mMinFilter != LLRenderContract::Filter::Linear ||
        linear->mMagFilter != LLRenderContract::Filter::Linear ||
        linear->mAddressU != LLRenderContract::AddressMode::Mirror ||
        linear->mAddressV != LLRenderContract::AddressMode::Mirror ||
        linear->mMipFilter != LLRenderContract::MipFilter::Disabled ||
        linear->mMaxAnisotropy != 1.f || linear->mAnisotropyEnabled)
    {
        error = "registry sampler metadata does not match the packet";
        return std::nullopt;
    }

    const PipelineBinding& pipeline = *result.mPipeline;
    if (pipeline.mDestinationFormat != inputs->mDestinationFormat ||
        !sameExtent(pipeline.mExtent, inputs->mDestinationExtent) ||
        pipeline.mSceneView != result.mScene->mView || pipeline.mExposureView != result.mExposure->mView ||
        pipeline.mDestinationView != result.mDestination->mView ||
        pipeline.mPointSampler != point->mSampler || pipeline.mLinearSampler != linear->mSampler ||
        pipeline.mDestinationFinalLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ||
        pipeline.mDescriptorBindings != std::vector<std::uint32_t>{ 0, 1 } ||
        pipeline.mVertexStride != 16 || pipeline.mPositionFormat != VK_FORMAT_R32G32B32_SFLOAT ||
        pipeline.mPositionOffset != 0 ||
        pipeline.mPushConstantSize != sizeof(LLRenderContract::TonemapParameters))
    {
        error = "pipeline layout metadata does not match the packet";
        return std::nullopt;
    }

    return result;
}

} // namespace

bool Registry::addBuffer(LLRenderContract::BufferHandle handle, BufferBinding binding)
{
    const bool complete = binding.mBuffer != VK_NULL_HANDLE && binding.mSize != 0;
    return addEntry(mBuffers, handle, std::move(binding), complete);
}

bool Registry::addImage(LLRenderContract::ImageHandle handle, ImageBinding binding)
{
    const bool complete = binding.mImage != VK_NULL_HANDLE && binding.mView != VK_NULL_HANDLE &&
                          binding.mFormat != VK_FORMAT_UNDEFINED && binding.mExtent.mWidth != 0 &&
                          binding.mExtent.mHeight != 0;
    return addEntry(mImages, handle, std::move(binding), complete);
}

bool Registry::addSampler(LLRenderContract::SamplerHandle handle, SamplerBinding binding)
{
    const bool complete = binding.mSampler != VK_NULL_HANDLE;
    return addEntry(mSamplers, handle, std::move(binding), complete);
}

bool Registry::addPipeline(LLRenderContract::PipelineHandle handle, PipelineBinding binding)
{
    const bool complete = !binding.mProgram.mName.empty() && binding.mPipeline != VK_NULL_HANDLE &&
                          binding.mLayout != VK_NULL_HANDLE && binding.mRenderPass != VK_NULL_HANDLE &&
                          binding.mFramebuffer != VK_NULL_HANDLE && binding.mDescriptorSet != VK_NULL_HANDLE;
    return addEntry(mPipelines, handle, std::move(binding), complete);
}

const BufferBinding* Registry::resolve(LLRenderContract::BufferHandle handle) const
{
    return resolveEntry(mBuffers, handle);
}

const ImageBinding* Registry::resolve(LLRenderContract::ImageHandle handle) const
{
    return resolveEntry(mImages, handle);
}

const SamplerBinding* Registry::resolve(LLRenderContract::SamplerHandle handle) const
{
    return resolveEntry(mSamplers, handle);
}

const PipelineBinding* Registry::resolve(LLRenderContract::PipelineHandle handle,
                                         const LLRenderContract::ShaderProgramKey& program) const
{
    const PipelineBinding* binding = resolveEntry(mPipelines, handle);
    if (!binding || binding->mProgram.mName != program.mName || binding->mProgram.mVariant != program.mVariant)
    {
        return nullptr;
    }
    return binding;
}

bool execute(const LLRenderContract::FrameSnapshot& frame, const Registry& registry,
             const ExecutionContext& context, std::string& error)
{
    const auto prepared = prepare(frame, registry, context, error);
    if (!prepared)
    {
        return false;
    }

    VkResult result = vkResetCommandBuffer(context.mCommandBuffer, 0);
    if (result != VK_SUCCESS)
    {
        error = "vkResetCommandBuffer failed";
        return false;
    }

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(context.mCommandBuffer, &begin);
    if (result != VK_SUCCESS)
    {
        error = "vkBeginCommandBuffer failed";
        return false;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = prepared->mDestination->mImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(context.mCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkRenderPassBeginInfo render_pass{};
    render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass.renderPass = prepared->mPipeline->mRenderPass;
    render_pass.framebuffer = prepared->mPipeline->mFramebuffer;
    render_pass.renderArea.extent = { prepared->mInputs.mDestinationExtent.mWidth,
                                      prepared->mInputs.mDestinationExtent.mHeight };
    vkCmdBeginRenderPass(context.mCommandBuffer, &render_pass, VK_SUBPASS_CONTENTS_INLINE);

    const LLRenderContract::Viewport& contract_viewport = frame.mPasses.front().mViewport;
    // Contract coordinates are bottom-left. A negative Vulkan viewport height
    // preserves that orientation without changing the shared shader math.
    VkViewport viewport{ contract_viewport.mX,
                         static_cast<float>(prepared->mInputs.mDestinationExtent.mHeight) -
                             contract_viewport.mY,
                         contract_viewport.mWidth, -contract_viewport.mHeight,
                         contract_viewport.mMinDepth, contract_viewport.mMaxDepth };
    const LLRenderContract::Scissor& contract_scissor = frame.mPasses.front().mScissor;
    VkRect2D scissor{ { static_cast<std::int32_t>(contract_scissor.mX),
                        static_cast<std::int32_t>(prepared->mInputs.mDestinationExtent.mHeight -
                                                  contract_scissor.mY - contract_scissor.mHeight) },
                      { contract_scissor.mWidth, contract_scissor.mHeight } };
    vkCmdSetViewport(context.mCommandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(context.mCommandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(context.mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prepared->mPipeline->mPipeline);
    vkCmdBindDescriptorSets(context.mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prepared->mPipeline->mLayout,
                            0, 1, &prepared->mPipeline->mDescriptorSet, 0, nullptr);

    const LLRenderContract::ByteRange& parameters = prepared->mDraw->mResources.mParameters.front().mBytes;
    vkCmdPushConstants(context.mCommandBuffer, prepared->mPipeline->mLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, static_cast<std::uint32_t>(parameters.mSize),
                       parameters.mStorage->data() + parameters.mOffset);
    const VkDeviceSize offset = prepared->mDraw->mResources.mVertexBuffers.front().mOffset;
    vkCmdBindVertexBuffers(context.mCommandBuffer, 0, 1, &prepared->mTriangle->mBuffer, &offset);
    vkCmdDraw(context.mCommandBuffer, prepared->mDraw->mVertexCount, prepared->mDraw->mInstanceCount,
              prepared->mDraw->mFirstVertex, prepared->mDraw->mFirstInstance);
    vkCmdEndRenderPass(context.mCommandBuffer);

    result = vkEndCommandBuffer(context.mCommandBuffer);
    if (result != VK_SUCCESS)
    {
        error = "vkEndCommandBuffer failed";
        return false;
    }

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &context.mCommandBuffer;
    result = vkQueueSubmit(context.mQueue, 1, &submit, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
    {
        error = "vkQueueSubmit failed";
        return false;
    }
    ++*context.mSubmissionCount;
    result = vkQueueWaitIdle(context.mQueue);
    if (result != VK_SUCCESS)
    {
        error = "vkQueueWaitIdle failed";
        return false;
    }

    error.clear();
    return true;
}

} // namespace LLRenderVulkanTonemap
