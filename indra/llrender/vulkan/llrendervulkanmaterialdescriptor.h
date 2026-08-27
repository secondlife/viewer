/**
 * @file llrendervulkanmaterialdescriptor.h
 * @brief Immutable populated Vulkan descriptors for the canonical material interface.
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

#ifndef LL_LLRENDERVULKANMATERIALDESCRIPTOR_H
#define LL_LLRENDERVULKANMATERIALDESCRIPTOR_H

#include "llrendervulkanmateriallayout.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace LLRenderVulkanMaterial
{

struct MaterialDescriptorDispatch
{
    PFN_vkCreateDescriptorPool   mCreateDescriptorPool   = nullptr;
    PFN_vkDestroyDescriptorPool  mDestroyDescriptorPool  = nullptr;
    PFN_vkAllocateDescriptorSets mAllocateDescriptorSets = nullptr;
    PFN_vkUpdateDescriptorSets   mUpdateDescriptorSets   = nullptr;
};

// The Vulkan 1.1-or-newer device, its implementation dispatch, and external
// host synchronization remain the caller's responsibility for the lifetime
// of every generation.
struct MaterialDescriptorDevice
{
    VkDevice                   mDevice = VK_NULL_HANDLE;
    MaterialDescriptorDispatch mDispatch;
};

struct MaterialUniformDescriptorResource
{
    VkBuffer     mBuffer = VK_NULL_HANDLE;
    VkDeviceSize mSize   = 0;
    VkDeviceSize mOffset = 0;

    friend constexpr bool operator==(const MaterialUniformDescriptorResource&, const MaterialUniformDescriptorResource&) = default;
};

struct MaterialSampledDescriptorResource
{
    VkSampler   mSampler = VK_NULL_HANDLE;
    VkImageView mView    = VK_NULL_HANDLE;

    friend constexpr bool operator==(const MaterialSampledDescriptorResource&, const MaterialSampledDescriptorResource&) = default;
};

struct MaterialDescriptorResources
{
    MaterialUniformDescriptorResource                mParameters;
    std::array<MaterialSampledDescriptorResource, 3> mSampledImages{};

    friend constexpr bool operator==(const MaterialDescriptorResources&, const MaterialDescriptorResources&) = default;
};

struct MaterialDescriptorSetPair
{
    VkDescriptorSet mParameters    = VK_NULL_HANDLE;
    VkDescriptorSet mSampledImages = VK_NULL_HANDLE;

    friend constexpr bool operator==(const MaterialDescriptorSetPair&, const MaterialDescriptorSetPair&) = default;
};

struct MaterialDescriptorBinding
{
    MaterialDescriptorSetPair   mSets;
    MaterialDescriptorResources mResources;

    friend constexpr bool operator==(const MaterialDescriptorBinding&, const MaterialDescriptorBinding&) = default;
};

enum class MaterialDescriptorCreationCode : std::uint8_t
{
    InvalidDevice,
    InvalidDispatch,
    LayoutDeviceMismatch,
    EmptyBatch,
    BatchTooLarge,
    InvalidUniformBuffer,
    InvalidUniformRange,
    InvalidSampledImage,
    InvalidSampler,
    OwnerAllocationFailure,
    PoolCreateFailure,
    NullPool,
    SetAllocationFailure,
    NullParameterSet,
    NullSampledImageSet
};

struct MaterialDescriptorCreationError
{
    MaterialDescriptorCreationCode mCode = MaterialDescriptorCreationCode::InvalidDevice;
    std::optional<std::size_t>     mTupleIndex;
    std::optional<std::uint32_t>   mSampledImageIndex;
    VkResult                       mResult = VK_SUCCESS;

    friend constexpr bool operator==(const MaterialDescriptorCreationError&, const MaterialDescriptorCreationError&) = default;
};

class LegacyNormSpecDescriptorGeneration
{
public:
    ~LegacyNormSpecDescriptorGeneration() noexcept;

    LegacyNormSpecDescriptorGeneration(const LegacyNormSpecDescriptorGeneration&)            = delete;
    LegacyNormSpecDescriptorGeneration& operator=(const LegacyNormSpecDescriptorGeneration&) = delete;
    LegacyNormSpecDescriptorGeneration(LegacyNormSpecDescriptorGeneration&&)                 = delete;
    LegacyNormSpecDescriptorGeneration& operator=(LegacyNormSpecDescriptorGeneration&&)      = delete;

    std::size_t size() const noexcept { return mBindings.size(); }

    // Returned bindings contain borrowed handles and copied metadata. The sets
    // expire with this generation; resources remain owned by the caller.
    std::optional<MaterialDescriptorBinding> binding(std::size_t index) const noexcept;

private:
    friend struct MaterialDescriptorFactory;

    LegacyNormSpecDescriptorGeneration(const MaterialDescriptorDevice& device, std::vector<MaterialDescriptorBinding>&& bindings) noexcept;

    VkDevice                               mDevice                = VK_NULL_HANDLE;
    PFN_vkDestroyDescriptorPool            mDestroyDescriptorPool = nullptr;
    VkDescriptorPool                       mPool                  = VK_NULL_HANDLE;
    std::vector<MaterialDescriptorBinding> mBindings;
};

using MaterialDescriptorCreationResult = std::variant<MaterialDescriptorCreationError, std::unique_ptr<LegacyNormSpecDescriptorGeneration>>;

bool validMaterialDescriptorGenerationCount(std::size_t count) noexcept;

// The layout owner conservatively outlives the returned generation. mSize is
// the buffer's creation size, and mOffset must satisfy the creating device's
// minUniformBufferOffsetAlignment. Every buffer, view, and sampler must belong
// to device and match the declared use. Sampled images must be in shader-read-
// only layout when consumed and use no YCbCr conversion. All resources remain
// alive through the last submitted command that consumes the generation;
// destruction is legal only after those commands complete.
MaterialDescriptorCreationResult createLegacyNormSpecDescriptorGeneration(
    const MaterialDescriptorDevice&                 device,
    const LegacyNormSpecPipelineLayout&             layout,
    const std::vector<MaterialDescriptorResources>& resources) noexcept;

} // namespace LLRenderVulkanMaterial

#endif // LL_LLRENDERVULKANMATERIALDESCRIPTOR_H
