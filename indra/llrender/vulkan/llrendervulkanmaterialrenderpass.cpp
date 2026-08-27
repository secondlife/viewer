/**
 * @file llrendervulkanmaterialrenderpass.cpp
 * @brief Transactional Vulkan render-pass ownership for the canonical material pass.
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

#include "llrendervulkanmaterialrenderpass.h"

#include <new>
#include <type_traits>

namespace LLRenderVulkanMaterial
{
static_assert(std::is_nothrow_copy_constructible_v<LegacyNormSpecAttachmentProfile>);

namespace
{

    MaterialRenderPassCreationError failure(MaterialRenderPassCreationCode code, VkResult result = VK_SUCCESS) noexcept
    {
        return { code, result };
    }

    VkAttachmentDescription colorDescription(const MaterialColorAttachmentProfile& color) noexcept
    {
        VkAttachmentDescription description{};
        description.format         = color.mNativeFormat;
        description.samples        = VK_SAMPLE_COUNT_1_BIT;
        description.loadOp         = color.mRequiredLoadOp;
        description.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.initialLayout  = LegacyNormSpecRenderPass::colorAttachmentLayout();
        description.finalLayout    = LegacyNormSpecRenderPass::colorAttachmentLayout();
        return description;
    }

    VkAttachmentDescription depthDescription(const MaterialDepthAttachmentProfile& depth) noexcept
    {
        VkAttachmentDescription description{};
        description.format         = depth.mNativeFormat;
        description.samples        = VK_SAMPLE_COUNT_1_BIT;
        description.loadOp         = depth.mRequiredLoadOp;
        description.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.initialLayout  = LegacyNormSpecRenderPass::depthAttachmentLayout();
        description.finalLayout    = LegacyNormSpecRenderPass::depthAttachmentLayout();
        return description;
    }

} // namespace

struct MaterialRenderPassFactory
{
    static std::unique_ptr<LegacyNormSpecRenderPass> allocate(const MaterialRenderPassDevice&        device,
                                                              const LegacyNormSpecAttachmentProfile& profile) noexcept
    {
        return std::unique_ptr<LegacyNormSpecRenderPass>(new (std::nothrow) LegacyNormSpecRenderPass(device, profile));
    }

    static VkRenderPass& renderPass(LegacyNormSpecRenderPass& owner) noexcept { return owner.mRenderPass; }
};

LegacyNormSpecRenderPass::LegacyNormSpecRenderPass(const MaterialRenderPassDevice&        device,
                                                   const LegacyNormSpecAttachmentProfile& profile) noexcept :
    mDevice(device.mDevice),
    mDestroyRenderPass(device.mDispatch.mDestroyRenderPass),
    mAttachmentProfile(profile)
{
}

LegacyNormSpecRenderPass::~LegacyNormSpecRenderPass() noexcept
{
    if (mRenderPass != VK_NULL_HANDLE)
    {
        mDestroyRenderPass(mDevice, mRenderPass, nullptr);
    }
}

std::array<VkClearValue, LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT> LegacyNormSpecRenderPass::clearValues() const noexcept
{
    std::array<VkClearValue, LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT> values{};
    for (std::size_t slot = 0; slot < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT; ++slot)
    {
        for (std::size_t component = 0; component < mAttachmentProfile.colors()[slot].mClearColor.size(); ++component)
        {
            values[slot].color.float32[component] = mAttachmentProfile.colors()[slot].mClearColor[component];
        }
    }
    values[LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT].depthStencil.depth   = mAttachmentProfile.depth().mClearDepth;
    values[LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT].depthStencil.stencil = mAttachmentProfile.depth().mClearStencil;
    return values;
}

MaterialRenderPassCreationResult createLegacyNormSpecRenderPass(const MaterialRenderPassDevice&        device,
                                                                const LegacyNormSpecAttachmentProfile& profile) noexcept
{
    if (device.mPhysicalDevice == VK_NULL_HANDLE)
    {
        return failure(MaterialRenderPassCreationCode::InvalidPhysicalDevice);
    }
    if (device.mDevice == VK_NULL_HANDLE)
    {
        return failure(MaterialRenderPassCreationCode::InvalidDevice);
    }
    if (!device.mDispatch.mCreateRenderPass || !device.mDispatch.mDestroyRenderPass)
    {
        return failure(MaterialRenderPassCreationCode::InvalidDispatch);
    }
    if (!profile.selectedFor(device.mPhysicalDevice))
    {
        return failure(MaterialRenderPassCreationCode::ProfilePhysicalDeviceMismatch);
    }

    std::array<VkAttachmentDescription, LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT> attachments{};
    std::array<VkAttachmentReference, LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT>         color_references{};
    for (std::uint32_t slot = 0; slot < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT; ++slot)
    {
        attachments[slot]                 = colorDescription(profile.colors()[slot]);
        color_references[slot].attachment = slot;
        color_references[slot].layout     = LegacyNormSpecRenderPass::colorAttachmentLayout();
    }
    attachments[LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT] = depthDescription(profile.depth());

    VkAttachmentReference depth_reference{};
    depth_reference.attachment = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
    depth_reference.layout     = LegacyNormSpecRenderPass::depthAttachmentLayout();

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
    subpass.pColorAttachments       = color_references.data();
    subpass.pDepthStencilAttachment = &depth_reference;

    VkRenderPassCreateInfo create_info{};
    create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT;
    create_info.pAttachments    = attachments.data();
    create_info.subpassCount    = 1;
    create_info.pSubpasses      = &subpass;

    std::unique_ptr<LegacyNormSpecRenderPass> owner = MaterialRenderPassFactory::allocate(device, profile);
    if (!owner)
    {
        return failure(MaterialRenderPassCreationCode::OwnerAllocationFailure);
    }

    VkRenderPass   render_pass = VK_NULL_HANDLE;
    const VkResult result      = device.mDispatch.mCreateRenderPass(device.mDevice, &create_info, nullptr, &render_pass);
    if (result != VK_SUCCESS)
    {
        return failure(MaterialRenderPassCreationCode::CreateFailure, result);
    }
    if (render_pass == VK_NULL_HANDLE)
    {
        return failure(MaterialRenderPassCreationCode::NullRenderPass);
    }

    MaterialRenderPassFactory::renderPass(*owner) = render_pass;
    return owner;
}

} // namespace LLRenderVulkanMaterial
