/**
 * @file llrendervulkanmaterialpipelinecache.h
 * @brief Transactional cold Vulkan pipeline-cache ownership for the material prototype.
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

#ifndef LL_LLRENDERVULKANMATERIALPIPELINECACHE_H
#define LL_LLRENDERVULKANMATERIALPIPELINECACHE_H

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <variant>

namespace LLRenderVulkanMaterial
{

struct MaterialPipelineCacheDispatch
{
    PFN_vkCreatePipelineCache  mCreatePipelineCache  = nullptr;
    PFN_vkDestroyPipelineCache mDestroyPipelineCache = nullptr;
};

// The logical device and the implementation addressed by these callbacks
// remain owned by the caller and outlive every returned cache owner. The
// caller must prevent destruction while another host operation uses the cache.
struct MaterialPipelineCacheDevice
{
    VkDevice                      mDevice = VK_NULL_HANDLE;
    MaterialPipelineCacheDispatch mDispatch;
};

enum class MaterialPipelineCacheCreationCode : std::uint8_t
{
    InvalidDevice,
    InvalidDispatch,
    OwnerAllocationFailure,
    CreateFailure,
    NullPipelineCache
};

struct MaterialPipelineCacheCreationError
{
    MaterialPipelineCacheCreationCode mCode   = MaterialPipelineCacheCreationCode::InvalidDevice;
    VkResult                          mResult = VK_SUCCESS;

    friend constexpr bool operator==(const MaterialPipelineCacheCreationError&, const MaterialPipelineCacheCreationError&) = default;
};

// This owner creates a cold cache with Vulkan's default internally synchronized
// access for later pipeline-creation calls. Destruction and other host
// operations retain their external-synchronization requirements. The owner
// authenticates only the exact VkDevice used for creation. It does not prove
// physical-device capabilities, device features, queue creation, cache warmth,
// or a cache hit.
class MaterialPipelineCache
{
public:
    ~MaterialPipelineCache() noexcept;

    MaterialPipelineCache(const MaterialPipelineCache&)            = delete;
    MaterialPipelineCache& operator=(const MaterialPipelineCache&) = delete;
    MaterialPipelineCache(MaterialPipelineCache&&)                 = delete;
    MaterialPipelineCache& operator=(MaterialPipelineCache&&)      = delete;

    VkPipelineCache pipelineCache() const noexcept { return mPipelineCache; }
    bool            createdOn(VkDevice device) const noexcept { return mDevice == device; }

private:
    friend struct MaterialPipelineCacheFactory;

    explicit MaterialPipelineCache(const MaterialPipelineCacheDevice& device) noexcept;

    VkDevice                   mDevice               = VK_NULL_HANDLE;
    PFN_vkDestroyPipelineCache mDestroyPipelineCache = nullptr;
    VkPipelineCache            mPipelineCache        = VK_NULL_HANDLE;
};

using MaterialPipelineCacheCreationResult = std::variant<MaterialPipelineCacheCreationError, std::unique_ptr<MaterialPipelineCache>>;

MaterialPipelineCacheCreationResult createMaterialPipelineCache(const MaterialPipelineCacheDevice& device) noexcept;

} // namespace LLRenderVulkanMaterial

#endif // LL_LLRENDERVULKANMATERIALPIPELINECACHE_H
