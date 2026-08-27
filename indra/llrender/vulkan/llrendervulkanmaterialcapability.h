/**
 * @file llrendervulkanmaterialcapability.h
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

#ifndef LL_LLRENDERVULKANMATERIALCAPABILITY_H
#define LL_LLRENDERVULKANMATERIALCAPABILITY_H

#include "llrendervulkanmaterialattachment.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <optional>
#include <variant>

namespace LLRenderVulkanMaterial
{

inline constexpr std::uint32_t LEGACY_NORMSPEC_VERTEX_INPUT_COUNT = 7;

struct MaterialPipelineCapabilityDispatch
{
    PFN_vkGetPhysicalDeviceProperties            mGetPhysicalDeviceProperties            = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties mGetPhysicalDeviceQueueFamilyProperties = nullptr;
    PFN_vkEnumerateDeviceExtensionProperties     mEnumerateDeviceExtensionProperties     = nullptr;
    PFN_vkGetPhysicalDeviceProperties2           mGetPhysicalDeviceProperties2           = nullptr;
};

// The physical device and the implementation addressed by these callbacks
// remain owned by the caller. This service performs queries only and creates no
// native object.
struct MaterialPipelineCapabilityDevice
{
    VkPhysicalDevice                   mPhysicalDevice = VK_NULL_HANDLE;
    MaterialPipelineCapabilityDispatch mDispatch;
};

enum class MaterialPipelineCapabilityQuery : std::uint8_t
{
    PhysicalDeviceProperties,
    QueueFamilyProperties,
    DeviceExtensionProperties,
    PhysicalDeviceProperties2
};

enum class MaterialPipelineCapabilityResolutionCode : std::uint8_t
{
    InvalidPhysicalDevice,
    InvalidDispatch,
    AttachmentProfilePhysicalDeviceMismatch,
    UnsupportedApiVariant,
    InsufficientApiVersion,
    MissingGraphicsQueueFamily,
    EnumerationFailure,
    EnumerationIncomplete,
    EnumerationCountExceeded,
    InvalidEnumerationOutput,
    ScratchAllocationFailure,
    InvalidPortabilityAlignment,
    IncompatiblePortabilityStride
};

struct MaterialPipelineCapabilityResolutionError
{
    MaterialPipelineCapabilityResolutionCode       mCode = MaterialPipelineCapabilityResolutionCode::InvalidPhysicalDevice;
    std::optional<MaterialPipelineCapabilityQuery> mQuery;
    std::optional<std::uint32_t>                   mVertexBinding;
    std::uint64_t                                  mRequiredValue      = 0;
    std::uint64_t                                  mAvailableValue     = 0;
    std::uint32_t                                  mEnumerationAttempt = 0;
    VkResult                                       mResult             = VK_SUCCESS;

    friend constexpr bool operator==(const MaterialPipelineCapabilityResolutionError&,
                                     const MaterialPipelineCapabilityResolutionError&) = default;
};

struct MaterialVertexInputCapability
{
    LLRenderContract::VertexSemantic mSemantic      = LLRenderContract::VertexSemantic::Position;
    LLRenderContract::VertexFormat   mLogicalFormat = LLRenderContract::VertexFormat::Float3;
    std::uint32_t                    mLocation      = 0;
    std::uint32_t                    mBinding       = 0;
    std::uint32_t                    mOffset        = 0;
    std::uint32_t                    mStride        = 0;
    VkFormat                         mNativeFormat  = VK_FORMAT_UNDEFINED;

    friend constexpr bool operator==(const MaterialVertexInputCapability&, const MaterialVertexInputCapability&) = default;
};

// These are obligations for a future logical-device transaction, not evidence
// that an arbitrary VkDevice enabled them. The future owner of the exact
// VkDeviceCreateInfo must authenticate feature, extension, and queue creation.
class MaterialPipelineLogicalDeviceRequirements
{
public:
    MaterialPipelineLogicalDeviceRequirements(const MaterialPipelineLogicalDeviceRequirements&)            = default;
    MaterialPipelineLogicalDeviceRequirements& operator=(const MaterialPipelineLogicalDeviceRequirements&) = default;
    MaterialPipelineLogicalDeviceRequirements(MaterialPipelineLogicalDeviceRequirements&&)                 = default;
    MaterialPipelineLogicalDeviceRequirements& operator=(MaterialPipelineLogicalDeviceRequirements&&)      = default;

    bool independentBlendRequired() const noexcept { return mIndependentBlendRequired; }
    bool graphicsQueueRequired() const noexcept { return mGraphicsQueueRequired; }
    bool portabilitySubsetExtensionRequired() const noexcept { return mPortabilitySubsetExtensionRequired; }

    friend constexpr bool operator==(const MaterialPipelineLogicalDeviceRequirements&,
                                     const MaterialPipelineLogicalDeviceRequirements&) = default;

private:
    friend class LegacyNormSpecPipelineCapabilityProfile;

    constexpr MaterialPipelineLogicalDeviceRequirements(bool independent_blend_required,
                                                        bool portability_subset_extension_required) noexcept :
        mIndependentBlendRequired(independent_blend_required),
        mPortabilitySubsetExtensionRequired(portability_subset_extension_required)
    {
    }

    bool mIndependentBlendRequired           = true;
    bool mGraphicsQueueRequired              = true;
    bool mPortabilitySubsetExtensionRequired = false;
};

// Only the resolver can construct this physical-device-specific value. It
// retains the exact attachment profile with which the remaining production
// pipeline requirements were proven. No VkDevice enablement is inferred.
class LegacyNormSpecPipelineCapabilityProfile
{
public:
    LegacyNormSpecPipelineCapabilityProfile(const LegacyNormSpecPipelineCapabilityProfile&)            = default;
    LegacyNormSpecPipelineCapabilityProfile& operator=(const LegacyNormSpecPipelineCapabilityProfile&) = default;
    LegacyNormSpecPipelineCapabilityProfile(LegacyNormSpecPipelineCapabilityProfile&&)                 = default;
    LegacyNormSpecPipelineCapabilityProfile& operator=(LegacyNormSpecPipelineCapabilityProfile&&)      = default;

    const LegacyNormSpecAttachmentProfile& attachmentProfile() const noexcept { return mAttachmentProfile; }
    const std::array<MaterialVertexInputCapability, LEGACY_NORMSPEC_VERTEX_INPUT_COUNT>& vertexInputs() const noexcept
    {
        return mVertexInputs;
    }
    MaterialPipelineLogicalDeviceRequirements     logicalDeviceRequirements() const noexcept { return mLogicalDeviceRequirements; }
    LLRenderContract::LegacyNormSpecTargetProfile targetProfile() const noexcept { return mAttachmentProfile.targetProfile(); }
    bool          selectedFor(VkPhysicalDevice physical_device) const noexcept { return mPhysicalDevice == physical_device; }
    bool          portabilitySubsetAdvertised() const noexcept { return mLogicalDeviceRequirements.portabilitySubsetExtensionRequired(); }
    std::uint32_t apiVersion() const noexcept { return mApiVersion; }
    std::uint32_t minVertexInputBindingStrideAlignment() const noexcept { return mMinVertexInputBindingStrideAlignment; }

private:
    friend struct MaterialPipelineCapabilityProfileFactory;

    LegacyNormSpecPipelineCapabilityProfile(
        VkPhysicalDevice                                                                     physical_device,
        const LegacyNormSpecAttachmentProfile&                                               attachment_profile,
        const std::array<MaterialVertexInputCapability, LEGACY_NORMSPEC_VERTEX_INPUT_COUNT>& vertex_inputs,
        bool                                                                                 portability_subset_advertised,
        std::uint32_t                                                                        api_version,
        std::uint32_t min_vertex_input_binding_stride_alignment) noexcept :
        mPhysicalDevice(physical_device),
        mAttachmentProfile(attachment_profile),
        mVertexInputs(vertex_inputs),
        mLogicalDeviceRequirements(attachment_profile.deviceRequirements().independentBlendRequired(), portability_subset_advertised),
        mApiVersion(api_version),
        mMinVertexInputBindingStrideAlignment(min_vertex_input_binding_stride_alignment)
    {
    }

    VkPhysicalDevice                                                              mPhysicalDevice = VK_NULL_HANDLE;
    LegacyNormSpecAttachmentProfile                                               mAttachmentProfile;
    std::array<MaterialVertexInputCapability, LEGACY_NORMSPEC_VERTEX_INPUT_COUNT> mVertexInputs{};
    MaterialPipelineLogicalDeviceRequirements                                     mLogicalDeviceRequirements;
    std::uint32_t                                                                 mApiVersion                           = 0;
    std::uint32_t                                                                 mMinVertexInputBindingStrideAlignment = 1;
};

using MaterialPipelineCapabilityResolutionResult =
    std::variant<MaterialPipelineCapabilityResolutionError, LegacyNormSpecPipelineCapabilityProfile>;

MaterialPipelineCapabilityResolutionResult resolveLegacyNormSpecPipelineCapabilityProfile(
    const MaterialPipelineCapabilityDevice& device,
    const LegacyNormSpecAttachmentProfile&  attachment_profile) noexcept;

} // namespace LLRenderVulkanMaterial

#endif // LL_LLRENDERVULKANMATERIALCAPABILITY_H
