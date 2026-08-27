/**
 * @file llrendervulkanmaterialcapability.cpp
 * @brief Physical-device capability closure for the canonical Vulkan material pipeline.
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

#include "llrendervulkanmaterialcapability.h"

#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>

namespace LLRenderVulkanMaterial
{
static_assert(std::is_nothrow_copy_constructible_v<LegacyNormSpecAttachmentProfile>);

struct MaterialPipelineCapabilityProfileFactory
{
    static LegacyNormSpecPipelineCapabilityProfile create(
        VkPhysicalDevice                                                                     physical_device,
        const LegacyNormSpecAttachmentProfile&                                               attachment_profile,
        const std::array<MaterialVertexInputCapability, LEGACY_NORMSPEC_VERTEX_INPUT_COUNT>& vertex_inputs,
        bool                                                                                 portability_subset_advertised,
        std::uint32_t                                                                        api_version,
        std::uint32_t min_vertex_input_binding_stride_alignment) noexcept
    {
        return LegacyNormSpecPipelineCapabilityProfile(physical_device, attachment_profile, vertex_inputs, portability_subset_advertised,
                                                       api_version, min_vertex_input_binding_stride_alignment);
    }
};

namespace
{
    using namespace LLRenderContract;

    inline constexpr char          PORTABILITY_SUBSET_EXTENSION[] = "VK_KHR_portability_subset";
    inline constexpr std::uint32_t ENUMERATION_ATTEMPTS           = 3;
    inline constexpr std::array<MaterialVertexInputCapability, LEGACY_NORMSPEC_VERTEX_INPUT_COUNT> CANONICAL_VERTEX_INPUTS{
        MaterialVertexInputCapability{ VertexSemantic::Position, VertexFormat::Float3, 0, 0, 0, 16, VK_FORMAT_R32G32B32_SFLOAT },
        MaterialVertexInputCapability{ VertexSemantic::Normal, VertexFormat::Float3, 1, 1, 0, 16, VK_FORMAT_R32G32B32_SFLOAT },
        MaterialVertexInputCapability{ VertexSemantic::TexCoord0, VertexFormat::Float2, 2, 2, 0, 8, VK_FORMAT_R32G32_SFLOAT },
        MaterialVertexInputCapability{ VertexSemantic::Color, VertexFormat::UNorm8x4, 3, 3, 0, 4, VK_FORMAT_R8G8B8A8_UNORM },
        MaterialVertexInputCapability{ VertexSemantic::Tangent, VertexFormat::Float4, 4, 4, 0, 16, VK_FORMAT_R32G32B32A32_SFLOAT },
        MaterialVertexInputCapability{ VertexSemantic::TexCoord1, VertexFormat::Float2, 5, 5, 0, 8, VK_FORMAT_R32G32_SFLOAT },
        MaterialVertexInputCapability{ VertexSemantic::TexCoord2, VertexFormat::Float2, 6, 6, 0, 8, VK_FORMAT_R32G32_SFLOAT }
    };

    MaterialPipelineCapabilityResolutionError failure(MaterialPipelineCapabilityResolutionCode code) noexcept
    {
        MaterialPipelineCapabilityResolutionError error;
        error.mCode = code;
        return error;
    }

    MaterialPipelineCapabilityResolutionError queryFailure(MaterialPipelineCapabilityResolutionCode code,
                                                           MaterialPipelineCapabilityQuery          query,
                                                           VkResult                                 result = VK_SUCCESS) noexcept
    {
        auto error    = failure(code);
        error.mQuery  = query;
        error.mResult = result;
        return error;
    }

    template<typename Value>
    std::optional<MaterialPipelineCapabilityResolutionError> allocationSizeFailure(std::uint32_t                   count,
                                                                                   MaterialPipelineCapabilityQuery query) noexcept
    {
        constexpr std::size_t MAX_COUNT = std::numeric_limits<std::size_t>::max() / sizeof(Value);
        if (static_cast<std::uint64_t>(count) <= static_cast<std::uint64_t>(MAX_COUNT))
        {
            return std::nullopt;
        }

        auto error            = queryFailure(MaterialPipelineCapabilityResolutionCode::EnumerationCountExceeded, query);
        error.mRequiredValue  = MAX_COUNT;
        error.mAvailableValue = count;
        return error;
    }

    std::optional<MaterialPipelineCapabilityResolutionError> requireGraphicsQueue(const MaterialPipelineCapabilityDevice& device) noexcept
    {
        std::uint32_t count = 0;
        device.mDispatch.mGetPhysicalDeviceQueueFamilyProperties(device.mPhysicalDevice, &count, nullptr);
        if (count == 0)
        {
            return queryFailure(MaterialPipelineCapabilityResolutionCode::MissingGraphicsQueueFamily,
                                MaterialPipelineCapabilityQuery::QueueFamilyProperties);
        }
        if (auto error = allocationSizeFailure<VkQueueFamilyProperties>(count, MaterialPipelineCapabilityQuery::QueueFamilyProperties))
        {
            return error;
        }

        std::unique_ptr<VkQueueFamilyProperties[]> properties(new (std::nothrow) VkQueueFamilyProperties[count]);
        if (!properties)
        {
            return queryFailure(MaterialPipelineCapabilityResolutionCode::ScratchAllocationFailure,
                                MaterialPipelineCapabilityQuery::QueueFamilyProperties);
        }

        const std::uint32_t capacity = count;
        device.mDispatch.mGetPhysicalDeviceQueueFamilyProperties(device.mPhysicalDevice, &count, properties.get());
        if (count > capacity)
        {
            auto error            = queryFailure(MaterialPipelineCapabilityResolutionCode::InvalidEnumerationOutput,
                                                 MaterialPipelineCapabilityQuery::QueueFamilyProperties);
            error.mRequiredValue  = capacity;
            error.mAvailableValue = count;
            return error;
        }

        for (std::uint32_t index = 0; index < count; ++index)
        {
            if (properties[index].queueCount != 0 && (properties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
            {
                return std::nullopt;
            }
        }
        return queryFailure(MaterialPipelineCapabilityResolutionCode::MissingGraphicsQueueFamily,
                            MaterialPipelineCapabilityQuery::QueueFamilyProperties);
    }

    struct DeviceExtensionResolution
    {
        bool                                      mPortabilitySubsetAdvertised = false;
        MaterialPipelineCapabilityResolutionError mError;
        bool                                      mSucceeded = false;
    };

    DeviceExtensionResolution resolveDeviceExtensions(const MaterialPipelineCapabilityDevice& device) noexcept
    {
        VkResult last_result = VK_INCOMPLETE;
        for (std::uint32_t attempt = 1; attempt <= ENUMERATION_ATTEMPTS; ++attempt)
        {
            std::uint32_t  count = 0;
            const VkResult count_result =
                device.mDispatch.mEnumerateDeviceExtensionProperties(device.mPhysicalDevice, nullptr, &count, nullptr);
            if (count_result == VK_INCOMPLETE)
            {
                last_result = count_result;
                continue;
            }
            if (count_result != VK_SUCCESS)
            {
                DeviceExtensionResolution resolution;
                resolution.mError                     = queryFailure(MaterialPipelineCapabilityResolutionCode::EnumerationFailure,
                                                                     MaterialPipelineCapabilityQuery::DeviceExtensionProperties, count_result);
                resolution.mError.mEnumerationAttempt = attempt;
                return resolution;
            }
            if (auto error =
                    allocationSizeFailure<VkExtensionProperties>(count, MaterialPipelineCapabilityQuery::DeviceExtensionProperties))
            {
                DeviceExtensionResolution resolution;
                resolution.mError                     = *error;
                resolution.mError.mEnumerationAttempt = attempt;
                return resolution;
            }
            if (count == 0)
            {
                DeviceExtensionResolution resolution;
                resolution.mSucceeded = true;
                return resolution;
            }

            std::unique_ptr<VkExtensionProperties[]> properties(new (std::nothrow) VkExtensionProperties[count]);
            if (!properties)
            {
                DeviceExtensionResolution resolution;
                resolution.mError                     = queryFailure(MaterialPipelineCapabilityResolutionCode::ScratchAllocationFailure,
                                                                     MaterialPipelineCapabilityQuery::DeviceExtensionProperties);
                resolution.mError.mAvailableValue     = count;
                resolution.mError.mEnumerationAttempt = attempt;
                return resolution;
            }

            const std::uint32_t capacity = count;
            const VkResult      list_result =
                device.mDispatch.mEnumerateDeviceExtensionProperties(device.mPhysicalDevice, nullptr, &count, properties.get());
            if (list_result != VK_SUCCESS && list_result != VK_INCOMPLETE)
            {
                DeviceExtensionResolution resolution;
                resolution.mError                     = queryFailure(MaterialPipelineCapabilityResolutionCode::EnumerationFailure,
                                                                     MaterialPipelineCapabilityQuery::DeviceExtensionProperties, list_result);
                resolution.mError.mEnumerationAttempt = attempt;
                return resolution;
            }
            if (count > capacity)
            {
                DeviceExtensionResolution resolution;
                resolution.mError                     = queryFailure(MaterialPipelineCapabilityResolutionCode::InvalidEnumerationOutput,
                                                                     MaterialPipelineCapabilityQuery::DeviceExtensionProperties, list_result);
                resolution.mError.mRequiredValue      = capacity;
                resolution.mError.mAvailableValue     = count;
                resolution.mError.mEnumerationAttempt = attempt;
                return resolution;
            }
            if (list_result == VK_INCOMPLETE)
            {
                last_result = list_result;
                continue;
            }

            DeviceExtensionResolution resolution;
            for (std::uint32_t index = 0; index < count; ++index)
            {
                if (std::strncmp(properties[index].extensionName, PORTABILITY_SUBSET_EXTENSION, sizeof(PORTABILITY_SUBSET_EXTENSION)) == 0)
                {
                    resolution.mPortabilitySubsetAdvertised = true;
                    break;
                }
            }
            resolution.mSucceeded = true;
            return resolution;
        }

        DeviceExtensionResolution resolution;
        resolution.mError                     = queryFailure(MaterialPipelineCapabilityResolutionCode::EnumerationIncomplete,
                                                             MaterialPipelineCapabilityQuery::DeviceExtensionProperties, last_result);
        resolution.mError.mEnumerationAttempt = ENUMERATION_ATTEMPTS;
        return resolution;
    }

    std::optional<MaterialPipelineCapabilityResolutionError> resolvePortabilityAlignment(
        const MaterialPipelineCapabilityDevice&                                              device,
        const std::array<MaterialVertexInputCapability, LEGACY_NORMSPEC_VERTEX_INPUT_COUNT>& vertex_inputs,
        bool                                                                                 portability_subset_advertised,
        std::uint32_t&                                                                       alignment) noexcept
    {
        alignment = 1;
        if (!portability_subset_advertised)
        {
            return std::nullopt;
        }

        VkPhysicalDevicePortabilitySubsetPropertiesKHR portability_properties{};
        portability_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 properties{};
        properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        properties.pNext = &portability_properties;
        device.mDispatch.mGetPhysicalDeviceProperties2(device.mPhysicalDevice, &properties);

        alignment = portability_properties.minVertexInputBindingStrideAlignment;
        if (alignment == 0 || (alignment & (alignment - 1)) != 0)
        {
            auto error            = queryFailure(MaterialPipelineCapabilityResolutionCode::InvalidPortabilityAlignment,
                                                 MaterialPipelineCapabilityQuery::PhysicalDeviceProperties2);
            error.mRequiredValue  = 1;
            error.mAvailableValue = alignment;
            return error;
        }

        for (const auto& input : vertex_inputs)
        {
            if (input.mStride < alignment || input.mStride % alignment != 0)
            {
                auto error            = queryFailure(MaterialPipelineCapabilityResolutionCode::IncompatiblePortabilityStride,
                                                     MaterialPipelineCapabilityQuery::PhysicalDeviceProperties2);
                error.mVertexBinding  = input.mBinding;
                error.mRequiredValue  = alignment;
                error.mAvailableValue = input.mStride;
                return error;
            }
        }
        return std::nullopt;
    }

} // namespace

MaterialPipelineCapabilityResolutionResult resolveLegacyNormSpecPipelineCapabilityProfile(
    const MaterialPipelineCapabilityDevice& device,
    const LegacyNormSpecAttachmentProfile&  attachment_profile) noexcept
{
    if (device.mPhysicalDevice == VK_NULL_HANDLE)
    {
        return failure(MaterialPipelineCapabilityResolutionCode::InvalidPhysicalDevice);
    }
    if (!device.mDispatch.mGetPhysicalDeviceProperties || !device.mDispatch.mGetPhysicalDeviceQueueFamilyProperties ||
        !device.mDispatch.mEnumerateDeviceExtensionProperties || !device.mDispatch.mGetPhysicalDeviceProperties2)
    {
        return failure(MaterialPipelineCapabilityResolutionCode::InvalidDispatch);
    }
    if (!attachment_profile.selectedFor(device.mPhysicalDevice))
    {
        return failure(MaterialPipelineCapabilityResolutionCode::AttachmentProfilePhysicalDeviceMismatch);
    }
    VkPhysicalDeviceProperties properties{};
    device.mDispatch.mGetPhysicalDeviceProperties(device.mPhysicalDevice, &properties);
    if (VK_API_VERSION_VARIANT(properties.apiVersion) != 0)
    {
        auto error            = queryFailure(MaterialPipelineCapabilityResolutionCode::UnsupportedApiVariant,
                                             MaterialPipelineCapabilityQuery::PhysicalDeviceProperties);
        error.mRequiredValue  = 0;
        error.mAvailableValue = VK_API_VERSION_VARIANT(properties.apiVersion);
        return error;
    }
    if (properties.apiVersion < VK_API_VERSION_1_1)
    {
        auto error            = queryFailure(MaterialPipelineCapabilityResolutionCode::InsufficientApiVersion,
                                             MaterialPipelineCapabilityQuery::PhysicalDeviceProperties);
        error.mRequiredValue  = VK_API_VERSION_1_1;
        error.mAvailableValue = properties.apiVersion;
        return error;
    }
    auto vertex_inputs = CANONICAL_VERTEX_INPUTS;
    if (auto error = requireGraphicsQueue(device))
    {
        return *error;
    }

    const DeviceExtensionResolution extensions = resolveDeviceExtensions(device);
    if (!extensions.mSucceeded)
    {
        return extensions.mError;
    }

    std::uint32_t alignment = 1;
    if (auto error = resolvePortabilityAlignment(device, vertex_inputs, extensions.mPortabilitySubsetAdvertised, alignment))
    {
        return *error;
    }

    return MaterialPipelineCapabilityProfileFactory::create(device.mPhysicalDevice, attachment_profile, vertex_inputs,
                                                            extensions.mPortabilitySubsetAdvertised, properties.apiVersion, alignment);
}

} // namespace LLRenderVulkanMaterial
