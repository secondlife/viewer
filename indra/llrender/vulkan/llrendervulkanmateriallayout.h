/**
 * @file llrendervulkanmateriallayout.h
 * @brief Transactional Vulkan layouts for the canonical material interface.
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

#ifndef LL_LLRENDERVULKANMATERIALLAYOUT_H
#define LL_LLRENDERVULKANMATERIALLAYOUT_H

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <variant>

namespace LLRenderVulkanMaterial
{

struct MaterialLayoutDispatch
{
    PFN_vkCreateDescriptorSetLayout  mCreateDescriptorSetLayout  = nullptr;
    PFN_vkDestroyDescriptorSetLayout mDestroyDescriptorSetLayout = nullptr;
    PFN_vkCreatePipelineLayout       mCreatePipelineLayout       = nullptr;
    PFN_vkDestroyPipelineLayout      mDestroyPipelineLayout      = nullptr;
};

// mDevice belongs to a Vulkan 1.1-or-newer logical device. The logical device
// and the implementation addressed by these function pointers must remain
// valid until every returned layout owner has been destroyed. Callers
// externally synchronize host access to the device and layouts.
struct MaterialLayoutDevice
{
    VkDevice               mDevice = VK_NULL_HANDLE;
    MaterialLayoutDispatch mDispatch;
};

enum class MaterialLayoutObject : std::uint8_t
{
    ParameterSetLayout,
    SampledImageSetLayout,
    PipelineLayout
};

enum class MaterialLayoutCreationCode : std::uint8_t
{
    InvalidDevice,
    InvalidDispatch,
    OwnerAllocationFailure,
    CreateFailure,
    NullHandle
};

struct MaterialLayoutCreationError
{
    MaterialLayoutCreationCode          mCode = MaterialLayoutCreationCode::InvalidDevice;
    std::optional<MaterialLayoutObject> mObject;
    VkResult                            mResult = VK_SUCCESS;

    friend constexpr bool operator==(const MaterialLayoutCreationError&, const MaterialLayoutCreationError&) = default;
};

class LegacyNormSpecPipelineLayout
{
public:
    ~LegacyNormSpecPipelineLayout() noexcept;

    LegacyNormSpecPipelineLayout(const LegacyNormSpecPipelineLayout&)            = delete;
    LegacyNormSpecPipelineLayout& operator=(const LegacyNormSpecPipelineLayout&) = delete;
    LegacyNormSpecPipelineLayout(LegacyNormSpecPipelineLayout&&)                 = delete;
    LegacyNormSpecPipelineLayout& operator=(LegacyNormSpecPipelineLayout&&)      = delete;

    // These handles are borrowed from this owner and expire with it. Callers
    // must not destroy them. The ordered array is returned by value so it
    // cannot leave a reference into a destroyed owner.
    VkDescriptorSetLayout                parameterSetLayout() const noexcept { return mDescriptorSetLayouts[0]; }
    VkDescriptorSetLayout                sampledImageSetLayout() const noexcept { return mDescriptorSetLayouts[1]; }
    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts() const noexcept { return mDescriptorSetLayouts; }
    VkPipelineLayout                     pipelineLayout() const noexcept { return mPipelineLayout; }
    bool                                 createdOn(VkDevice device) const noexcept { return mDevice == device; }

private:
    friend struct MaterialLayoutFactory;

    explicit LegacyNormSpecPipelineLayout(const MaterialLayoutDevice& device) noexcept;

    VkDevice                             mDevice                     = VK_NULL_HANDLE;
    PFN_vkDestroyDescriptorSetLayout     mDestroyDescriptorSetLayout = nullptr;
    PFN_vkDestroyPipelineLayout          mDestroyPipelineLayout      = nullptr;
    std::array<VkDescriptorSetLayout, 2> mDescriptorSetLayouts{};
    VkPipelineLayout                     mPipelineLayout = VK_NULL_HANDLE;
};

using MaterialLayoutCreationResult = std::variant<MaterialLayoutCreationError, std::unique_ptr<LegacyNormSpecPipelineLayout>>;

MaterialLayoutCreationResult createLegacyNormSpecPipelineLayout(const MaterialLayoutDevice& device) noexcept;

} // namespace LLRenderVulkanMaterial

#endif // LL_LLRENDERVULKANMATERIALLAYOUT_H
