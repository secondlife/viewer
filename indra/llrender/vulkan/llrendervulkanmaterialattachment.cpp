/**
 * @file llrendervulkanmaterialattachment.cpp
 * @brief Portable Vulkan attachment profiles for the canonical material pass.
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

#include "llrendervulkanmaterialattachment.h"

namespace LLRenderVulkanMaterial
{

struct MaterialAttachmentProfileFactory
{
    static LegacyNormSpecAttachmentProfile create(LLRenderContract::LegacyNormSpecTargetProfile target_profile,
                                                  VkPhysicalDevice                              physical_device) noexcept
    {
        return LegacyNormSpecAttachmentProfile(target_profile, physical_device);
    }

    static MaterialColorAttachmentProfile& color(LegacyNormSpecAttachmentProfile& profile, std::uint32_t slot) noexcept
    {
        return profile.mColors[slot];
    }

    static MaterialDepthAttachmentProfile& depth(LegacyNormSpecAttachmentProfile& profile) noexcept { return profile.mDepth; }
};

namespace
{
    using namespace LLRenderContract;

    inline constexpr VkImageUsageFlags    COLOR_USAGE    = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    inline constexpr VkImageUsageFlags    DEPTH_USAGE    = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    inline constexpr VkFormatFeatureFlags COLOR_FEATURES = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    inline constexpr VkFormatFeatureFlags DEPTH_FEATURES =
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    inline constexpr VkColorComponentFlags RGBA_WRITE_MASK =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    inline constexpr VkColorComponentFlags RGB_WRITE_MASK = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

    inline constexpr std::array<VkFormat, LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT> MODERN_HDR_FORMATS{
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT
    };

    inline constexpr std::array<VkFormat, LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT> COMPATIBILITY_FORMATS{
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_FORMAT_R8G8B8A8_UNORM
    };

    MaterialAttachmentResolutionError failure(MaterialAttachmentResolutionCode code) noexcept
    {
        MaterialAttachmentResolutionError error;
        error.mCode = code;
        return error;
    }

    MaterialAttachmentResolutionError limitFailure(MaterialAttachmentLimit limit, std::uint32_t available) noexcept
    {
        MaterialAttachmentResolutionError error = failure(MaterialAttachmentResolutionCode::InsufficientLimit);
        error.mQuery                            = MaterialAttachmentQuery::PhysicalDeviceProperties;
        error.mLimit                            = limit;
        error.mRequiredLimit                    = static_cast<std::uint32_t>(LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT);
        error.mAvailableLimit                   = available;
        return error;
    }

    MaterialAttachmentResolutionError deviceFeatureFailure(MaterialAttachmentFeature feature) noexcept
    {
        MaterialAttachmentResolutionError error = failure(MaterialAttachmentResolutionCode::MissingDeviceFeature);
        error.mQuery                            = MaterialAttachmentQuery::PhysicalDeviceFeatures;
        error.mFeature                          = feature;
        return error;
    }

    MaterialAttachmentResolutionError attachmentFailure(MaterialAttachmentResolutionCode code, MaterialAttachmentKind kind,
                                                        std::optional<std::uint32_t> color_slot, PixelFormat logical_format,
                                                        VkFormat native_format, VkFormatFeatureFlags required,
                                                        VkFormatFeatureFlags available) noexcept
    {
        MaterialAttachmentResolutionError error = failure(code);
        error.mAttachment                       = kind;
        error.mColorSlot                        = color_slot;
        error.mLogicalFormat                    = logical_format;
        error.mNativeFormat                     = native_format;
        error.mRequiredFeatures                 = required;
        error.mAvailableFeatures                = available;
        return error;
    }

    std::optional<MaterialAttachmentResolutionError> resolveCapabilities(const MaterialAttachmentDevice& device,
                                                                         MaterialAttachmentKind          kind,
                                                                         std::optional<std::uint32_t>    color_slot,
                                                                         PixelFormat logical_format, VkFormat native_format,
                                                                         VkImageUsageFlags usage, VkFormatFeatureFlags required_features,
                                                                         VkImageFormatProperties& resolved) noexcept
    {
        VkFormatProperties format_properties{};
        device.mDispatch.mGetPhysicalDeviceFormatProperties(device.mPhysicalDevice, native_format, &format_properties);
        const VkFormatFeatureFlags available_features = format_properties.optimalTilingFeatures;
        if ((available_features & required_features) != required_features)
        {
            auto error   = attachmentFailure(MaterialAttachmentResolutionCode::MissingFormatFeatures, kind, color_slot, logical_format,
                                             native_format, required_features, available_features);
            error.mQuery = MaterialAttachmentQuery::FormatProperties;
            return error;
        }

        VkImageFormatProperties image_properties{};
        const VkResult          result = device.mDispatch.mGetPhysicalDeviceImageFormatProperties(
            device.mPhysicalDevice, native_format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &image_properties);
        if (result != VK_SUCCESS)
        {
            auto error    = attachmentFailure(MaterialAttachmentResolutionCode::ImageFormatQueryFailure, kind, color_slot, logical_format,
                                              native_format, required_features, available_features);
            error.mQuery  = MaterialAttachmentQuery::ImageFormatProperties;
            error.mResult = result;
            return error;
        }

        auto insufficient = [&](MaterialAttachmentCapability capability, std::uint64_t available)
        {
            auto error = attachmentFailure(MaterialAttachmentResolutionCode::InsufficientImageCapability, kind, color_slot, logical_format,
                                           native_format, required_features, available_features);
            error.mQuery               = MaterialAttachmentQuery::ImageFormatProperties;
            error.mCapability          = capability;
            error.mRequiredCapability  = 1;
            error.mAvailableCapability = available;
            return std::optional<MaterialAttachmentResolutionError>{ error };
        };

        if (image_properties.maxMipLevels < 1)
        {
            return insufficient(MaterialAttachmentCapability::MipLevels, image_properties.maxMipLevels);
        }
        if (image_properties.maxArrayLayers < 1)
        {
            return insufficient(MaterialAttachmentCapability::ArrayLayers, image_properties.maxArrayLayers);
        }
        if (image_properties.maxExtent.width < 1)
        {
            return insufficient(MaterialAttachmentCapability::ExtentWidth, image_properties.maxExtent.width);
        }
        if (image_properties.maxExtent.height < 1)
        {
            return insufficient(MaterialAttachmentCapability::ExtentHeight, image_properties.maxExtent.height);
        }
        if (image_properties.maxExtent.depth < 1)
        {
            return insufficient(MaterialAttachmentCapability::ExtentDepth, image_properties.maxExtent.depth);
        }
        if ((image_properties.sampleCounts & VK_SAMPLE_COUNT_1_BIT) == 0)
        {
            return insufficient(MaterialAttachmentCapability::SampleCountOne, image_properties.sampleCounts);
        }

        resolved = image_properties;
        return std::nullopt;
    }

} // namespace

MaterialAttachmentResolutionResult resolveLegacyNormSpecAttachmentProfile(const MaterialAttachmentDevice&  device,
                                                                          const LegacyNormSpecPipelineKey& pipeline_key) noexcept
{
    if (device.mPhysicalDevice == VK_NULL_HANDLE)
    {
        return failure(MaterialAttachmentResolutionCode::InvalidPhysicalDevice);
    }
    if (!device.mDispatch.mGetPhysicalDeviceFeatures || !device.mDispatch.mGetPhysicalDeviceProperties ||
        !device.mDispatch.mGetPhysicalDeviceFormatProperties || !device.mDispatch.mGetPhysicalDeviceImageFormatProperties)
    {
        return failure(MaterialAttachmentResolutionCode::InvalidDispatch);
    }
    if (!validLegacyNormSpecPipelineKey(pipeline_key))
    {
        return failure(MaterialAttachmentResolutionCode::InvalidPipelineKey);
    }
    if (pipeline_key.mTargetProfile != LegacyNormSpecTargetProfile::ModernHDR &&
        pipeline_key.mTargetProfile != LegacyNormSpecTargetProfile::Compatibility)
    {
        return failure(MaterialAttachmentResolutionCode::UnsupportedTargetProfile);
    }

    VkPhysicalDeviceFeatures features{};
    device.mDispatch.mGetPhysicalDeviceFeatures(device.mPhysicalDevice, &features);
    if (features.independentBlend != VK_TRUE)
    {
        return deviceFeatureFailure(MaterialAttachmentFeature::IndependentBlend);
    }

    VkPhysicalDeviceProperties properties{};
    device.mDispatch.mGetPhysicalDeviceProperties(device.mPhysicalDevice, &properties);
    if (properties.limits.maxColorAttachments < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT)
    {
        return limitFailure(MaterialAttachmentLimit::ColorAttachments, properties.limits.maxColorAttachments);
    }
    if (properties.limits.maxFragmentOutputAttachments < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT)
    {
        return limitFailure(MaterialAttachmentLimit::FragmentOutputs, properties.limits.maxFragmentOutputAttachments);
    }

    const auto& native_formats =
        pipeline_key.mTargetProfile == LegacyNormSpecTargetProfile::ModernHDR ? MODERN_HDR_FORMATS : COMPATIBILITY_FORMATS;

    LegacyNormSpecAttachmentProfile profile = MaterialAttachmentProfileFactory::create(pipeline_key.mTargetProfile, device.mPhysicalDevice);
    for (std::uint32_t slot = 0; slot < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT; ++slot)
    {
        MaterialColorAttachmentProfile& color = MaterialAttachmentProfileFactory::color(profile, slot);
        color.mLogicalFormat                  = pipeline_key.mColorTargets[slot].mFormat;
        color.mNativeFormat                   = native_formats[slot];
        color.mUsage                          = COLOR_USAGE;
        color.mRequiredFeatures               = COLOR_FEATURES;
        color.mWriteMask                      = slot == 3 ? RGB_WRITE_MASK : RGBA_WRITE_MASK;
        color.mAlphaSemantic = slot == 3 ? MaterialAttachmentAlphaSemantic::ImplicitOneAfterClear : MaterialAttachmentAlphaSemantic::Stored;
        color.mClearColor    = slot == 3 ? std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f } : std::array<float, 4>{ 0.f, 0.f, 0.f, 0.f };

        if (auto error = resolveCapabilities(device, MaterialAttachmentKind::Color, slot, color.mLogicalFormat, color.mNativeFormat,
                                             color.mUsage, color.mRequiredFeatures, color.mCapabilities))
        {
            return *error;
        }
    }

    MaterialDepthAttachmentProfile& depth = MaterialAttachmentProfileFactory::depth(profile);
    depth.mLogicalFormat                  = *pipeline_key.mDepthFormat;
    depth.mNativeFormat                   = VK_FORMAT_D32_SFLOAT;
    depth.mUsage                          = DEPTH_USAGE;
    depth.mRequiredFeatures               = DEPTH_FEATURES;
    if (auto error = resolveCapabilities(device, MaterialAttachmentKind::Depth, std::nullopt, depth.mLogicalFormat, depth.mNativeFormat,
                                         depth.mUsage, depth.mRequiredFeatures, depth.mCapabilities))
    {
        return *error;
    }

    return profile;
}

} // namespace LLRenderVulkanMaterial
