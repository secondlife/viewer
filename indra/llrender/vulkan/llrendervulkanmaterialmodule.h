/**
 * @file llrendervulkanmaterialmodule.h
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

#ifndef LL_LLRENDERVULKANMATERIALMODULE_H
#define LL_LLRENDERVULKANMATERIALMODULE_H

#include "llvulkanmaterialpublication.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <variant>

namespace LLRenderVulkanMaterial
{

struct ShaderModuleDispatch
{
    PFN_vkCreateShaderModule  mCreateShaderModule  = nullptr;
    PFN_vkDestroyShaderModule mDestroyShaderModule = nullptr;
};

// mDevice belongs to a Vulkan 1.1-or-newer logical device. The logical device
// and the implementation addressed by these function pointers must remain
// valid until every module generation has been destroyed. Callers externally
// synchronize host access to the device and modules.
struct ShaderModuleDevice
{
    VkDevice             mDevice = VK_NULL_HANDLE;
    ShaderModuleDispatch mDispatch;
};

enum class ShaderModuleCreationCode : std::uint8_t
{
    InvalidDevice,
    InvalidDispatch,
    InvalidLease,
    OwnerAllocationFailure,
    CreateFailure,
    NullModule
};

struct ShaderModuleCreationError
{
    ShaderModuleCreationCode                     mCode = ShaderModuleCreationCode::InvalidDevice;
    std::optional<LLRenderContract::ShaderStage> mStage;
    VkResult                                     mResult = VK_SUCCESS;

    friend constexpr bool operator==(const ShaderModuleCreationError&, const ShaderModuleCreationError&) = default;
};

class ShaderModuleGeneration
{
public:
    ~ShaderModuleGeneration() noexcept;

    ShaderModuleGeneration(const ShaderModuleGeneration&)            = delete;
    ShaderModuleGeneration& operator=(const ShaderModuleGeneration&) = delete;
    ShaderModuleGeneration(ShaderModuleGeneration&&)                 = delete;
    ShaderModuleGeneration& operator=(ShaderModuleGeneration&&)      = delete;

    LLRenderContract::ShaderHandle               handle() const noexcept { return mHandle; }
    const LLRenderContract::LoadedShaderProgram& program() const noexcept { return *mProgram; }
    VkShaderModule                               vertexModule() const noexcept { return mVertexModule; }
    VkShaderModule                               fragmentModule() const noexcept { return mFragmentModule; }

private:
    friend struct ShaderModuleGenerationFactory;

    ShaderModuleGeneration(const ShaderModuleDevice& device, LLRenderContract::ShaderHandle handle,
                           std::shared_ptr<const LLRenderContract::LoadedShaderProgram> program) noexcept;

    VkDevice                                                     mDevice              = VK_NULL_HANDLE;
    PFN_vkDestroyShaderModule                                    mDestroyShaderModule = nullptr;
    LLRenderContract::ShaderHandle                               mHandle;
    std::shared_ptr<const LLRenderContract::LoadedShaderProgram> mProgram;
    VkShaderModule                                               mVertexModule   = VK_NULL_HANDLE;
    VkShaderModule                                               mFragmentModule = VK_NULL_HANDLE;
};

using ShaderModuleCreationResult = std::variant<ShaderModuleCreationError, std::unique_ptr<ShaderModuleGeneration>>;

// The aggregate lease cannot authenticate artifact provenance or its
// publication owner. Its program must originate from the trusted Stage 20
// packaged artifacts after the build-time SPIR-V validation chain, and the
// lease must come from the matching live LegacyNormSpecShaderPublication.
// Callers invoke this factory before the lease's frame completes. The bounded
// runtime validator rechecks shape and limits, not full SPIR-V validity. The
// frame admits this synchronous creation attempt; it is deliberately not
// retained as a shader-module lifetime or cache key.
ShaderModuleCreationResult createLegacyNormSpecShaderModules(const ShaderModuleDevice&                      device,
                                                             const LLRenderContract::ShaderGenerationLease& lease) noexcept;

} // namespace LLRenderVulkanMaterial

#endif // LL_LLRENDERVULKANMATERIALMODULE_H
