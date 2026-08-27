/**
 * @file llrendervulkanglobaldispatch.h
 * @brief Loader-independent validation of Vulkan global command dispatch.
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

#ifndef LL_LLRENDERVULKANGLOBALDISPATCH_H
#define LL_LLRENDERVULKANGLOBALDISPATCH_H

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <variant>

namespace LLRenderVulkan
{

inline constexpr std::uint32_t RENDERER_VULKAN_API_VERSION = VK_API_VERSION_1_1;

enum class VulkanGlobalCommand : std::uint8_t
{
    CreateInstance,
    EnumerateInstanceExtensionProperties,
    EnumerateInstanceLayerProperties,
    EnumerateInstanceVersion
};

enum class VulkanGlobalDispatchResolutionCode : std::uint8_t
{
    InvalidGetInstanceProcAddr,
    MissingRequiredCommand,
    VersionQueryFailure,
    UnsupportedApiVariant,
    InsufficientApiVersion
};

struct VulkanGlobalDispatchResolutionError
{
    VulkanGlobalDispatchResolutionCode mCode = VulkanGlobalDispatchResolutionCode::InvalidGetInstanceProcAddr;
    std::optional<VulkanGlobalCommand> mCommand;
    VkResult                           mResult              = VK_SUCCESS;
    std::uint32_t                      mAvailableApiVersion = 0;

    friend constexpr bool operator==(const VulkanGlobalDispatchResolutionError&, const VulkanGlobalDispatchResolutionError&) = default;
};

// The resolver and its loader implementation remain owned by the caller and
// must outlive every copy of this immutable generation. This value owns no
// dynamic library, Vulkan object, extension data, or unload policy.
class VulkanGlobalDispatchGeneration
{
public:
    VulkanGlobalDispatchGeneration(const VulkanGlobalDispatchGeneration&) noexcept = default;
    VulkanGlobalDispatchGeneration(VulkanGlobalDispatchGeneration&&) noexcept      = default;

    VulkanGlobalDispatchGeneration& operator=(const VulkanGlobalDispatchGeneration&) = delete;
    VulkanGlobalDispatchGeneration& operator=(VulkanGlobalDispatchGeneration&&)      = delete;

    PFN_vkGetInstanceProcAddr                  getInstanceProcAddr() const noexcept { return mGetInstanceProcAddr; }
    PFN_vkCreateInstance                       createInstance() const noexcept { return mCreateInstance; }
    PFN_vkEnumerateInstanceExtensionProperties enumerateInstanceExtensionProperties() const noexcept
    {
        return mEnumerateInstanceExtensionProperties;
    }
    PFN_vkEnumerateInstanceLayerProperties enumerateInstanceLayerProperties() const noexcept { return mEnumerateInstanceLayerProperties; }
    PFN_vkEnumerateInstanceVersion         enumerateInstanceVersion() const noexcept { return mEnumerateInstanceVersion; }
    std::uint32_t                          loaderApiVersion() const noexcept { return mLoaderApiVersion; }

private:
    friend struct VulkanGlobalDispatchGenerationFactory;

    constexpr VulkanGlobalDispatchGeneration(PFN_vkGetInstanceProcAddr                  get_instance_proc_addr,
                                             PFN_vkCreateInstance                       create_instance,
                                             PFN_vkEnumerateInstanceExtensionProperties enumerate_instance_extension_properties,
                                             PFN_vkEnumerateInstanceLayerProperties     enumerate_instance_layer_properties,
                                             PFN_vkEnumerateInstanceVersion             enumerate_instance_version,
                                             std::uint32_t                              loader_api_version) noexcept :
        mGetInstanceProcAddr(get_instance_proc_addr),
        mCreateInstance(create_instance),
        mEnumerateInstanceExtensionProperties(enumerate_instance_extension_properties),
        mEnumerateInstanceLayerProperties(enumerate_instance_layer_properties),
        mEnumerateInstanceVersion(enumerate_instance_version),
        mLoaderApiVersion(loader_api_version)
    {
    }

    const PFN_vkGetInstanceProcAddr                  mGetInstanceProcAddr;
    const PFN_vkCreateInstance                       mCreateInstance;
    const PFN_vkEnumerateInstanceExtensionProperties mEnumerateInstanceExtensionProperties;
    const PFN_vkEnumerateInstanceLayerProperties     mEnumerateInstanceLayerProperties;
    const PFN_vkEnumerateInstanceVersion             mEnumerateInstanceVersion;
    const std::uint32_t                              mLoaderApiVersion;
};

using VulkanGlobalDispatchResolutionResult = std::variant<VulkanGlobalDispatchResolutionError, VulkanGlobalDispatchGeneration>;

VulkanGlobalDispatchResolutionResult resolveVulkanGlobalDispatchGeneration(PFN_vkGetInstanceProcAddr get_instance_proc_addr) noexcept;

} // namespace LLRenderVulkan

#endif // LL_LLRENDERVULKANGLOBALDISPATCH_H
