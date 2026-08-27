/**
 * @file llrendervulkanmaterialrenderpass.h
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

#ifndef LL_LLRENDERVULKANMATERIALRENDERPASS_H
#define LL_LLRENDERVULKANMATERIALRENDERPASS_H

#include "llrendervulkanmaterialattachment.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>
#include <variant>

namespace LLRenderVulkanMaterial
{

inline constexpr std::uint32_t LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT + 1;

struct MaterialRenderPassDispatch
{
    PFN_vkCreateRenderPass  mCreateRenderPass  = nullptr;
    PFN_vkDestroyRenderPass mDestroyRenderPass = nullptr;
};

// Both handles and the implementation addressed by these callbacks remain
// owned by the caller. The caller guarantees a Vulkan 1.1 device created from
// mPhysicalDevice, with at least one graphics-capable queue family, and
// callbacks valid for mDevice. Raw handles cannot authenticate those
// relationships. Both handles remain valid until every returned owner has
// been destroyed. Host access is externally synchronized.
struct MaterialRenderPassDevice
{
    VkPhysicalDevice           mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice                   mDevice         = VK_NULL_HANDLE;
    MaterialRenderPassDispatch mDispatch;
};

enum class MaterialRenderPassCreationCode : std::uint8_t
{
    InvalidPhysicalDevice,
    InvalidDevice,
    InvalidDispatch,
    ProfilePhysicalDeviceMismatch,
    OwnerAllocationFailure,
    CreateFailure,
    NullRenderPass
};

struct MaterialRenderPassCreationError
{
    MaterialRenderPassCreationCode mCode   = MaterialRenderPassCreationCode::InvalidPhysicalDevice;
    VkResult                       mResult = VK_SUCCESS;

    friend constexpr bool operator==(const MaterialRenderPassCreationError&, const MaterialRenderPassCreationError&) = default;
};

// This owner describes one shared deferred pass generation. Its CLEAR load
// operations must not be interpreted as permission to begin one render pass
// per material draw. Borrowed handles and profile references expire with the
// owner. The caller must wait for all submitted native users before destroy.
class LegacyNormSpecRenderPass
{
public:
    ~LegacyNormSpecRenderPass() noexcept;

    LegacyNormSpecRenderPass(const LegacyNormSpecRenderPass&)            = delete;
    LegacyNormSpecRenderPass& operator=(const LegacyNormSpecRenderPass&) = delete;
    LegacyNormSpecRenderPass(LegacyNormSpecRenderPass&&)                 = delete;
    LegacyNormSpecRenderPass& operator=(LegacyNormSpecRenderPass&&)      = delete;

    VkRenderPass renderPass() const noexcept { return mRenderPass; }
    bool         createdOn(VkDevice device) const noexcept { return mDevice == device; }
    bool         selectedFrom(VkPhysicalDevice physical_device) const noexcept { return mAttachmentProfile.selectedFor(physical_device); }

    // Creation supplies no explicit subpass dependencies and performs no
    // automatic layout transitions. A future image-aware encoder must
    // transition the actual images into these attachment layouts before begin
    // and out to their consumer layouts afterward, including all required
    // synchronization and queue-family ownership transfers.
    static constexpr VkImageLayout colorAttachmentLayout() noexcept { return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; }
    static constexpr VkImageLayout depthAttachmentLayout() noexcept { return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; }

    const LegacyNormSpecAttachmentProfile& attachmentProfile() const noexcept { return mAttachmentProfile; }

    // VkRenderPass creation does not consume clear values. A later pass-begin
    // owner must use this complete ordered array over the full render area.
    std::array<VkClearValue, LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT> clearValues() const noexcept;

private:
    friend struct MaterialRenderPassFactory;

    LegacyNormSpecRenderPass(const MaterialRenderPassDevice& device, const LegacyNormSpecAttachmentProfile& profile) noexcept;

    VkDevice                        mDevice            = VK_NULL_HANDLE;
    PFN_vkDestroyRenderPass         mDestroyRenderPass = nullptr;
    VkRenderPass                    mRenderPass        = VK_NULL_HANDLE;
    LegacyNormSpecAttachmentProfile mAttachmentProfile;
};

using MaterialRenderPassCreationResult = std::variant<MaterialRenderPassCreationError, std::unique_ptr<LegacyNormSpecRenderPass>>;

MaterialRenderPassCreationResult createLegacyNormSpecRenderPass(const MaterialRenderPassDevice&        device,
                                                                const LegacyNormSpecAttachmentProfile& profile) noexcept;

} // namespace LLRenderVulkanMaterial

#endif // LL_LLRENDERVULKANMATERIALRENDERPASS_H
