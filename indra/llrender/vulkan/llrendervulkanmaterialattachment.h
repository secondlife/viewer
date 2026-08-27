/**
 * @file llrendervulkanmaterialattachment.h
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

#ifndef LL_LLRENDERVULKANMATERIALATTACHMENT_H
#define LL_LLRENDERVULKANMATERIALATTACHMENT_H

#include "lldrawpacketcontract.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

namespace LLRenderVulkanMaterial
{

inline constexpr std::uint32_t LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT = 4;

struct MaterialAttachmentDispatch
{
    PFN_vkGetPhysicalDeviceFeatures              mGetPhysicalDeviceFeatures              = nullptr;
    PFN_vkGetPhysicalDeviceProperties            mGetPhysicalDeviceProperties            = nullptr;
    PFN_vkGetPhysicalDeviceFormatProperties      mGetPhysicalDeviceFormatProperties      = nullptr;
    PFN_vkGetPhysicalDeviceImageFormatProperties mGetPhysicalDeviceImageFormatProperties = nullptr;
};

// The Vulkan 1.1-or-newer physical device and the implementation addressed by
// these callbacks remain owned by the caller. The resolved profile contains no
// native object and is valid only for this physical device. Feature resolution
// proves physical support, not what an existing logical device enabled.
struct MaterialAttachmentDevice
{
    VkPhysicalDevice           mPhysicalDevice = VK_NULL_HANDLE;
    MaterialAttachmentDispatch mDispatch;
};

enum class MaterialAttachmentKind : std::uint8_t
{
    Color,
    Depth
};

enum class MaterialAttachmentQuery : std::uint8_t
{
    PhysicalDeviceFeatures,
    PhysicalDeviceProperties,
    FormatProperties,
    ImageFormatProperties
};

enum class MaterialAttachmentFeature : std::uint8_t
{
    IndependentBlend
};

enum class MaterialAttachmentLimit : std::uint8_t
{
    ColorAttachments,
    FragmentOutputs
};

enum class MaterialAttachmentCapability : std::uint8_t
{
    MipLevels,
    ArrayLayers,
    ExtentWidth,
    ExtentHeight,
    ExtentDepth,
    SampleCountOne
};

enum class MaterialAttachmentAlphaSemantic : std::uint8_t
{
    Stored,
    ImplicitOneAfterClear
};

enum class MaterialAttachmentResolutionCode : std::uint8_t
{
    InvalidPhysicalDevice,
    InvalidDispatch,
    InvalidPipelineKey,
    UnsupportedTargetProfile,
    MissingDeviceFeature,
    InsufficientLimit,
    MissingFormatFeatures,
    ImageFormatQueryFailure,
    InsufficientImageCapability
};

struct MaterialAttachmentResolutionError
{
    MaterialAttachmentResolutionCode             mCode = MaterialAttachmentResolutionCode::InvalidPhysicalDevice;
    std::optional<MaterialAttachmentQuery>       mQuery;
    std::optional<MaterialAttachmentFeature>     mFeature;
    std::optional<MaterialAttachmentLimit>       mLimit;
    std::optional<MaterialAttachmentCapability>  mCapability;
    std::optional<MaterialAttachmentKind>        mAttachment;
    std::optional<std::uint32_t>                 mColorSlot;
    std::optional<LLRenderContract::PixelFormat> mLogicalFormat;
    VkFormat                                     mNativeFormat        = VK_FORMAT_UNDEFINED;
    VkFormatFeatureFlags                         mRequiredFeatures    = 0;
    VkFormatFeatureFlags                         mAvailableFeatures   = 0;
    std::uint32_t                                mRequiredLimit       = 0;
    std::uint32_t                                mAvailableLimit      = 0;
    std::uint64_t                                mRequiredCapability  = 0;
    std::uint64_t                                mAvailableCapability = 0;
    VkResult                                     mResult              = VK_SUCCESS;

    friend constexpr bool operator==(const MaterialAttachmentResolutionError&, const MaterialAttachmentResolutionError&) = default;
};

class MaterialAttachmentDeviceRequirements
{
public:
    MaterialAttachmentDeviceRequirements(const MaterialAttachmentDeviceRequirements&)            = default;
    MaterialAttachmentDeviceRequirements& operator=(const MaterialAttachmentDeviceRequirements&) = default;
    MaterialAttachmentDeviceRequirements(MaterialAttachmentDeviceRequirements&&)                 = default;
    MaterialAttachmentDeviceRequirements& operator=(MaterialAttachmentDeviceRequirements&&)      = default;

    bool independentBlendRequired() const noexcept { return mIndependentBlendRequired; }

    friend constexpr bool operator==(const MaterialAttachmentDeviceRequirements&, const MaterialAttachmentDeviceRequirements&) = default;

private:
    friend class LegacyNormSpecAttachmentProfile;

    constexpr MaterialAttachmentDeviceRequirements() noexcept = default;

    // A future logical-device owner must enable every required feature before
    // creating a device used with this profile.
    bool mIndependentBlendRequired = true;
};

struct MaterialColorAttachmentProfile
{
    LLRenderContract::PixelFormat   mLogicalFormat    = LLRenderContract::PixelFormat::RGBA8Unorm;
    VkFormat                        mNativeFormat     = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags               mUsage            = 0;
    VkFormatFeatureFlags            mRequiredFeatures = 0;
    VkColorComponentFlags           mWriteMask        = 0;
    VkAttachmentLoadOp              mRequiredLoadOp   = VK_ATTACHMENT_LOAD_OP_CLEAR;
    MaterialAttachmentAlphaSemantic mAlphaSemantic    = MaterialAttachmentAlphaSemantic::Stored;
    std::array<float, 4>            mClearColor{};
    VkImageFormatProperties         mCapabilities{};
};

struct MaterialDepthAttachmentProfile
{
    LLRenderContract::PixelFormat mLogicalFormat    = LLRenderContract::PixelFormat::Depth24Unorm;
    VkFormat                      mNativeFormat     = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags             mUsage            = 0;
    VkFormatFeatureFlags          mRequiredFeatures = 0;
    VkAttachmentLoadOp            mRequiredLoadOp   = VK_ATTACHMENT_LOAD_OP_CLEAR;
    float                         mClearDepth       = 1.f;
    std::uint32_t                 mClearStencil     = 0;
    VkImageFormatProperties       mCapabilities{};
};

// Only the resolver can construct or mutate this profile. The capability
// records are format/usage maxima, not a concrete allocation guarantee; a
// later image and framebuffer owner must validate its requested extent and
// resource size before publication. The RGB alpha-one semantic requires the
// recorded clear load operation, clear value, and write mask to be consumed
// together by the later render-pass and pipeline owners. deviceRequirements()
// returns a copy so callers cannot mutate the retained requirement.
class LegacyNormSpecAttachmentProfile
{
public:
    LegacyNormSpecAttachmentProfile(const LegacyNormSpecAttachmentProfile&)            = default;
    LegacyNormSpecAttachmentProfile& operator=(const LegacyNormSpecAttachmentProfile&) = default;
    LegacyNormSpecAttachmentProfile(LegacyNormSpecAttachmentProfile&&)                 = default;
    LegacyNormSpecAttachmentProfile& operator=(LegacyNormSpecAttachmentProfile&&)      = default;

    LLRenderContract::LegacyNormSpecTargetProfile targetProfile() const noexcept { return mTargetProfile; }
    MaterialAttachmentDeviceRequirements          deviceRequirements() const noexcept { return mDeviceRequirements; }
    const std::array<MaterialColorAttachmentProfile, LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT>& colors() const noexcept { return mColors; }
    const MaterialDepthAttachmentProfile&                                                     depth() const noexcept { return mDepth; }
    bool selectedFor(VkPhysicalDevice physical_device) const noexcept { return mPhysicalDevice == physical_device; }

private:
    friend struct MaterialAttachmentProfileFactory;

    LegacyNormSpecAttachmentProfile(LLRenderContract::LegacyNormSpecTargetProfile target_profile, VkPhysicalDevice physical_device) noexcept
        :
        mTargetProfile(target_profile),
        mPhysicalDevice(physical_device)
    {
    }

    LLRenderContract::LegacyNormSpecTargetProfile mTargetProfile  = LLRenderContract::LegacyNormSpecTargetProfile::ModernHDR;
    VkPhysicalDevice                              mPhysicalDevice = VK_NULL_HANDLE;
    MaterialAttachmentDeviceRequirements          mDeviceRequirements;
    std::array<MaterialColorAttachmentProfile, LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT> mColors{};
    MaterialDepthAttachmentProfile                                                     mDepth;
};

using MaterialAttachmentResolutionResult = std::variant<MaterialAttachmentResolutionError, LegacyNormSpecAttachmentProfile>;

MaterialAttachmentResolutionResult resolveLegacyNormSpecAttachmentProfile(
    const MaterialAttachmentDevice&                    device,
    const LLRenderContract::LegacyNormSpecPipelineKey& pipeline_key) noexcept;

} // namespace LLRenderVulkanMaterial

#endif // LL_LLRENDERVULKANMATERIALATTACHMENT_H
