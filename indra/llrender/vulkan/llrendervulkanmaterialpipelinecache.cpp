/**
 * @file llrendervulkanmaterialpipelinecache.cpp
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

#include "llrendervulkanmaterialpipelinecache.h"

#include <new>
#include <utility>

namespace LLRenderVulkanMaterial
{
namespace
{

    MaterialPipelineCacheCreationError failure(MaterialPipelineCacheCreationCode code, VkResult result = VK_SUCCESS) noexcept
    {
        return { code, result };
    }

} // namespace

struct MaterialPipelineCacheFactory
{
    static std::unique_ptr<MaterialPipelineCache> allocate(const MaterialPipelineCacheDevice& device) noexcept
    {
        return std::unique_ptr<MaterialPipelineCache>(new (std::nothrow) MaterialPipelineCache(device));
    }

    static VkPipelineCache& cache(MaterialPipelineCache& owner) noexcept { return owner.mPipelineCache; }
};

MaterialPipelineCache::MaterialPipelineCache(const MaterialPipelineCacheDevice& device) noexcept :
    mDevice(device.mDevice),
    mDestroyPipelineCache(device.mDispatch.mDestroyPipelineCache)
{
}

MaterialPipelineCache::~MaterialPipelineCache() noexcept
{
    if (mPipelineCache != VK_NULL_HANDLE)
    {
        mDestroyPipelineCache(mDevice, mPipelineCache, nullptr);
    }
}

MaterialPipelineCacheCreationResult createMaterialPipelineCache(const MaterialPipelineCacheDevice& device) noexcept
{
    if (device.mDevice == VK_NULL_HANDLE)
    {
        return failure(MaterialPipelineCacheCreationCode::InvalidDevice);
    }
    if (!device.mDispatch.mCreatePipelineCache || !device.mDispatch.mDestroyPipelineCache)
    {
        return failure(MaterialPipelineCacheCreationCode::InvalidDispatch);
    }

    std::unique_ptr<MaterialPipelineCache> owner = MaterialPipelineCacheFactory::allocate(device);
    if (!owner)
    {
        return failure(MaterialPipelineCacheCreationCode::OwnerAllocationFailure);
    }

    VkPipelineCacheCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkPipelineCache cache  = VK_NULL_HANDLE;
    const VkResult  result = device.mDispatch.mCreatePipelineCache(device.mDevice, &create_info, nullptr, &cache);
    if (result != VK_SUCCESS)
    {
        return failure(MaterialPipelineCacheCreationCode::CreateFailure, result);
    }
    if (cache == VK_NULL_HANDLE)
    {
        return failure(MaterialPipelineCacheCreationCode::NullPipelineCache);
    }

    MaterialPipelineCacheFactory::cache(*owner) = cache;
    return std::move(owner);
}

} // namespace LLRenderVulkanMaterial
