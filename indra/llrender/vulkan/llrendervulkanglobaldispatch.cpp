/**
 * @file llrendervulkanglobaldispatch.cpp
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

#include "llrendervulkanglobaldispatch.h"

namespace LLRenderVulkan
{

struct VulkanGlobalDispatchGenerationFactory
{
    static VulkanGlobalDispatchGeneration create(PFN_vkGetInstanceProcAddr                  get_instance_proc_addr,
                                                 PFN_vkCreateInstance                       create_instance,
                                                 PFN_vkEnumerateInstanceExtensionProperties enumerate_instance_extension_properties,
                                                 PFN_vkEnumerateInstanceLayerProperties     enumerate_instance_layer_properties,
                                                 PFN_vkEnumerateInstanceVersion             enumerate_instance_version,
                                                 std::uint32_t                              loader_api_version) noexcept
    {
        return VulkanGlobalDispatchGeneration(get_instance_proc_addr, create_instance, enumerate_instance_extension_properties,
                                              enumerate_instance_layer_properties, enumerate_instance_version, loader_api_version);
    }
};

namespace
{

    VulkanGlobalDispatchResolutionError failure(VulkanGlobalDispatchResolutionCode code,
                                                std::optional<VulkanGlobalCommand> command               = std::nullopt,
                                                VkResult                           result                = VK_SUCCESS,
                                                std::uint32_t                      available_api_version = 0) noexcept
    {
        return { code, command, result, available_api_version };
    }

    template<typename Function>
    Function resolve(PFN_vkGetInstanceProcAddr get_instance_proc_addr, const char* name) noexcept
    {
        return reinterpret_cast<Function>(get_instance_proc_addr(VK_NULL_HANDLE, name));
    }

} // namespace

VulkanGlobalDispatchResolutionResult resolveVulkanGlobalDispatchGeneration(PFN_vkGetInstanceProcAddr get_instance_proc_addr) noexcept
{
    if (!get_instance_proc_addr)
    {
        return failure(VulkanGlobalDispatchResolutionCode::InvalidGetInstanceProcAddr);
    }

    const PFN_vkCreateInstance create_instance = resolve<PFN_vkCreateInstance>(get_instance_proc_addr, "vkCreateInstance");
    if (!create_instance)
    {
        return failure(VulkanGlobalDispatchResolutionCode::MissingRequiredCommand, VulkanGlobalCommand::CreateInstance);
    }

    const PFN_vkEnumerateInstanceExtensionProperties enumerate_instance_extension_properties =
        resolve<PFN_vkEnumerateInstanceExtensionProperties>(get_instance_proc_addr, "vkEnumerateInstanceExtensionProperties");
    if (!enumerate_instance_extension_properties)
    {
        return failure(VulkanGlobalDispatchResolutionCode::MissingRequiredCommand,
                       VulkanGlobalCommand::EnumerateInstanceExtensionProperties);
    }

    const PFN_vkEnumerateInstanceLayerProperties enumerate_instance_layer_properties =
        resolve<PFN_vkEnumerateInstanceLayerProperties>(get_instance_proc_addr, "vkEnumerateInstanceLayerProperties");
    if (!enumerate_instance_layer_properties)
    {
        return failure(VulkanGlobalDispatchResolutionCode::MissingRequiredCommand, VulkanGlobalCommand::EnumerateInstanceLayerProperties);
    }

    const PFN_vkEnumerateInstanceVersion enumerate_instance_version =
        resolve<PFN_vkEnumerateInstanceVersion>(get_instance_proc_addr, "vkEnumerateInstanceVersion");
    if (!enumerate_instance_version)
    {
        return failure(VulkanGlobalDispatchResolutionCode::InsufficientApiVersion, VulkanGlobalCommand::EnumerateInstanceVersion,
                       VK_SUCCESS, VK_API_VERSION_1_0);
    }

    std::uint32_t  loader_api_version = 0;
    const VkResult version_result     = enumerate_instance_version(&loader_api_version);
    if (version_result != VK_SUCCESS)
    {
        return failure(VulkanGlobalDispatchResolutionCode::VersionQueryFailure, VulkanGlobalCommand::EnumerateInstanceVersion,
                       version_result);
    }
    if (VK_API_VERSION_VARIANT(loader_api_version) != 0)
    {
        return failure(VulkanGlobalDispatchResolutionCode::UnsupportedApiVariant, VulkanGlobalCommand::EnumerateInstanceVersion, VK_SUCCESS,
                       loader_api_version);
    }
    if (loader_api_version < RENDERER_VULKAN_API_VERSION)
    {
        return failure(VulkanGlobalDispatchResolutionCode::InsufficientApiVersion, VulkanGlobalCommand::EnumerateInstanceVersion,
                       VK_SUCCESS, loader_api_version);
    }

    return VulkanGlobalDispatchGenerationFactory::create(get_instance_proc_addr, create_instance, enumerate_instance_extension_properties,
                                                         enumerate_instance_layer_properties, enumerate_instance_version,
                                                         loader_api_version);
}

} // namespace LLRenderVulkan
