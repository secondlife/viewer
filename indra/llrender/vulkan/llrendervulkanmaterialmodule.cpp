/**
 * @file llrendervulkanmaterialmodule.cpp
 * @brief Transactional Vulkan shader modules for one published material generation.
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

#include "llrendervulkanmaterialmodule.h"

#include <cstddef>
#include <new>
#include <utility>

namespace LLRenderVulkanMaterial
{
namespace
{

    constexpr std::uint32_t LEGACY_NORMSPEC_SHADER_INDEX = 1;

    ShaderModuleCreationError failure(ShaderModuleCreationCode                     code,
                                      std::optional<LLRenderContract::ShaderStage> stage  = std::nullopt,
                                      VkResult                                     result = VK_SUCCESS) noexcept
    {
        return { code, stage, result };
    }

    VkShaderModuleCreateInfo createInfo(const LLRenderContract::LoadedShaderModule& module) noexcept
    {
        VkShaderModuleCreateInfo info{};
        info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = module.mWords.size() * sizeof(std::uint32_t);
        info.pCode    = module.mWords.data();
        return info;
    }

} // namespace

struct ShaderModuleGenerationFactory
{
    static std::unique_ptr<ShaderModuleGeneration> allocate(const ShaderModuleDevice&                      device,
                                                            const LLRenderContract::ShaderGenerationLease& lease) noexcept
    {
        return std::unique_ptr<ShaderModuleGeneration>(new (std::nothrow) ShaderModuleGeneration(device, lease.mHandle, lease.mProgram));
    }

    static VkShaderModule& vertex(ShaderModuleGeneration& generation) noexcept { return generation.mVertexModule; }

    static VkShaderModule& fragment(ShaderModuleGeneration& generation) noexcept { return generation.mFragmentModule; }
};

ShaderModuleGeneration::ShaderModuleGeneration(const ShaderModuleDevice& device, LLRenderContract::ShaderHandle handle,
                                               std::shared_ptr<const LLRenderContract::LoadedShaderProgram> program) noexcept :
    mDevice(device.mDevice),
    mDestroyShaderModule(device.mDispatch.mDestroyShaderModule),
    mHandle(handle),
    mProgram(std::move(program))
{
}

ShaderModuleGeneration::~ShaderModuleGeneration() noexcept
{
    if (mFragmentModule != VK_NULL_HANDLE)
    {
        mDestroyShaderModule(mDevice, mFragmentModule, nullptr);
    }
    if (mVertexModule != VK_NULL_HANDLE)
    {
        mDestroyShaderModule(mDevice, mVertexModule, nullptr);
    }
}

ShaderModuleCreationResult createLegacyNormSpecShaderModules(const ShaderModuleDevice&                      device,
                                                             const LLRenderContract::ShaderGenerationLease& lease) noexcept
{
    if (device.mDevice == VK_NULL_HANDLE)
    {
        return failure(ShaderModuleCreationCode::InvalidDevice);
    }
    if (!device.mDispatch.mCreateShaderModule || !device.mDispatch.mDestroyShaderModule)
    {
        return failure(ShaderModuleCreationCode::InvalidDispatch);
    }
    if (!lease.mHandle || lease.mHandle.mIndex != LEGACY_NORMSPEC_SHADER_INDEX || lease.mFrame == 0 || !lease.mProgram ||
        !LLRenderContract::validLegacyNormSpecProductionShaderProgram(*lease.mProgram))
    {
        return failure(ShaderModuleCreationCode::InvalidLease);
    }

    std::unique_ptr<ShaderModuleGeneration> generation = ShaderModuleGenerationFactory::allocate(device, lease);
    if (!generation)
    {
        return failure(ShaderModuleCreationCode::OwnerAllocationFailure);
    }

    const VkShaderModuleCreateInfo vertex_info   = createInfo(lease.mProgram->mVertex);
    VkShaderModule                 vertex        = VK_NULL_HANDLE;
    const VkResult                 vertex_result = device.mDispatch.mCreateShaderModule(device.mDevice, &vertex_info, nullptr, &vertex);
    if (vertex_result != VK_SUCCESS)
    {
        generation.reset();
        return failure(ShaderModuleCreationCode::CreateFailure, LLRenderContract::ShaderStage::Vertex, vertex_result);
    }
    if (vertex == VK_NULL_HANDLE)
    {
        generation.reset();
        return failure(ShaderModuleCreationCode::NullModule, LLRenderContract::ShaderStage::Vertex);
    }
    ShaderModuleGenerationFactory::vertex(*generation) = vertex;

    const VkShaderModuleCreateInfo fragment_info = createInfo(lease.mProgram->mFragment);
    VkShaderModule                 fragment      = VK_NULL_HANDLE;
    const VkResult fragment_result               = device.mDispatch.mCreateShaderModule(device.mDevice, &fragment_info, nullptr, &fragment);
    if (fragment_result != VK_SUCCESS)
    {
        generation.reset();
        return failure(ShaderModuleCreationCode::CreateFailure, LLRenderContract::ShaderStage::Fragment, fragment_result);
    }
    if (fragment == VK_NULL_HANDLE)
    {
        generation.reset();
        return failure(ShaderModuleCreationCode::NullModule, LLRenderContract::ShaderStage::Fragment);
    }
    ShaderModuleGenerationFactory::fragment(*generation) = fragment;

    return std::move(generation);
}

} // namespace LLRenderVulkanMaterial
