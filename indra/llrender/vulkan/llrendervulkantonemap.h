/**
 * @file llrendervulkantonemap.h
 * @brief Narrow Vulkan registry and executor for tonemap packets.
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

#ifndef LL_LLRENDERVULKANTONEMAP_H
#define LL_LLRENDERVULKANTONEMAP_H

#include "lltonemapcontract.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

namespace LLRenderVulkanTonemap
{

struct BufferBinding
{
    VkBuffer           mBuffer = VK_NULL_HANDLE;
    VkDeviceSize       mSize   = 0;
    VkBufferUsageFlags mUsage  = 0;
};

struct ImageBinding
{
    VkImage            mImage = VK_NULL_HANDLE;
    VkImageView        mView  = VK_NULL_HANDLE;
    VkFormat           mFormat = VK_FORMAT_UNDEFINED;
    LLRenderContract::Extent2D mExtent;
    VkImageUsageFlags  mUsage = 0;
};

struct SamplerBinding
{
    VkSampler                  mSampler = VK_NULL_HANDLE;
    LLRenderContract::Filter   mMinFilter = LLRenderContract::Filter::Linear;
    LLRenderContract::Filter   mMagFilter = LLRenderContract::Filter::Linear;
    LLRenderContract::AddressMode mAddressU = LLRenderContract::AddressMode::Clamp;
    LLRenderContract::AddressMode mAddressV = LLRenderContract::AddressMode::Clamp;
    LLRenderContract::MipFilter mMipFilter = LLRenderContract::MipFilter::Disabled;
    float                       mMaxAnisotropy = 1.f;
    bool                        mAnisotropyEnabled = false;
};

struct PipelineBinding
{
    LLRenderContract::ShaderProgramKey mProgram;
    LLRenderContract::PixelFormat      mDestinationFormat = LLRenderContract::PixelFormat::RGBA8Unorm;
    LLRenderContract::Extent2D         mExtent;
    VkPipeline                         mPipeline = VK_NULL_HANDLE;
    VkPipelineLayout                   mLayout = VK_NULL_HANDLE;
    VkRenderPass                       mRenderPass = VK_NULL_HANDLE;
    VkFramebuffer                      mFramebuffer = VK_NULL_HANDLE;
    VkDescriptorSet                    mDescriptorSet = VK_NULL_HANDLE;
    VkImageView                        mSceneView = VK_NULL_HANDLE;
    VkImageView                        mExposureView = VK_NULL_HANDLE;
    VkImageView                        mDestinationView = VK_NULL_HANDLE;
    VkSampler                          mPointSampler = VK_NULL_HANDLE;
    VkSampler                          mLinearSampler = VK_NULL_HANDLE;
    VkImageLayout                      mDestinationFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    std::vector<std::uint32_t>         mDescriptorBindings;
    std::uint32_t                      mVertexStride = 0;
    VkFormat                           mPositionFormat = VK_FORMAT_UNDEFINED;
    std::uint32_t                      mPositionOffset = 0;
    std::uint32_t                      mPushConstantSize = 0;
};

// The registry borrows objects owned by one synchronous offscreen run.
class Registry
{
public:
    bool addBuffer(LLRenderContract::BufferHandle handle, BufferBinding binding);
    bool addImage(LLRenderContract::ImageHandle handle, ImageBinding binding);
    bool addSampler(LLRenderContract::SamplerHandle handle, SamplerBinding binding);
    bool addPipeline(LLRenderContract::PipelineHandle handle, PipelineBinding binding);

    const BufferBinding* resolve(LLRenderContract::BufferHandle handle) const;
    const ImageBinding* resolve(LLRenderContract::ImageHandle handle) const;
    const SamplerBinding* resolve(LLRenderContract::SamplerHandle handle) const;
    const PipelineBinding* resolve(LLRenderContract::PipelineHandle handle,
                                   const LLRenderContract::ShaderProgramKey& program) const;

private:
    template<typename HandleType, typename BindingType>
    struct Entry
    {
        HandleType  mHandle;
        BindingType mBinding;
    };

    std::vector<Entry<LLRenderContract::BufferHandle, BufferBinding>> mBuffers;
    std::vector<Entry<LLRenderContract::ImageHandle, ImageBinding>> mImages;
    std::vector<Entry<LLRenderContract::SamplerHandle, SamplerBinding>> mSamplers;
    std::vector<Entry<LLRenderContract::PipelineHandle, PipelineBinding>> mPipelines;
};

struct ExecutionContext
{
    VkDevice          mDevice = VK_NULL_HANDLE;
    VkCommandBuffer   mCommandBuffer = VK_NULL_HANDLE;
    VkQueue           mQueue = VK_NULL_HANDLE;
    std::uint64_t*    mSubmissionCount = nullptr;
};

// Performs all packet, handle, and physical-resource checks before beginning
// the command buffer or changing the destination image.
bool execute(const LLRenderContract::FrameSnapshot& frame, const Registry& registry,
             const ExecutionContext& context, std::string& error);

} // namespace LLRenderVulkanTonemap

#endif // LL_LLRENDERVULKANTONEMAP_H
