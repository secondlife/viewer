/**
 * @file llrendervulkanmaterialattachment_test.cpp
 * @brief Tests for portable Vulkan material attachment profiles.
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

#include "linden_common.h"

#include "llrendervulkanmaterialattachment.h"
#include "lltut.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace
{
using namespace LLRenderContract;
using namespace LLRenderVulkanMaterial;

template<typename Handle>
Handle fakeHandle(std::uintptr_t value) noexcept
{
    if constexpr (std::is_pointer_v<Handle>)
    {
        return reinterpret_cast<Handle>(value);
    }
    else
    {
        return static_cast<Handle>(value);
    }
}

struct ImageQueryObservation
{
    VkPhysicalDevice   mPhysicalDevice = VK_NULL_HANDLE;
    VkFormat           mFormat         = VK_FORMAT_UNDEFINED;
    VkImageType        mType           = VK_IMAGE_TYPE_MAX_ENUM;
    VkImageTiling      mTiling         = VK_IMAGE_TILING_MAX_ENUM;
    VkImageUsageFlags  mUsage          = 0;
    VkImageCreateFlags mFlags          = 0;
};

struct FakeState
{
    static constexpr std::size_t QUERY_COUNT    = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT + 1;
    static constexpr std::size_t CALLBACK_COUNT = 2 + 2 * QUERY_COUNT;

    FakeState()
    {
        mFeatures.independentBlend                      = VK_TRUE;
        mProperties.limits.maxColorAttachments          = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
        mProperties.limits.maxFragmentOutputAttachments = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
        for (std::size_t index = 0; index < QUERY_COUNT; ++index)
        {
            mFormatOutputs[index].optimalTilingFeatures =
                index < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT
                    ? VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
                    : VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
            mImageOutputs[index].maxExtent       = { 8192, 4096, 1 };
            mImageOutputs[index].maxMipLevels    = 12;
            mImageOutputs[index].maxArrayLayers  = 1;
            mImageOutputs[index].sampleCounts    = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_4_BIT;
            mImageOutputs[index].maxResourceSize = 1ULL << 32;
        }
    }

    VkPhysicalDeviceFeatures                            mFeatures{};
    VkPhysicalDeviceProperties                          mProperties{};
    std::array<VkFormatProperties, QUERY_COUNT>         mFormatOutputs{};
    std::array<VkResult, QUERY_COUNT>                   mImageResults{};
    std::array<VkImageFormatProperties, QUERY_COUNT>    mImageOutputs{};
    std::array<VkFormat, QUERY_COUNT>                   mFormatQueries{};
    std::array<ImageQueryObservation, QUERY_COUNT>      mImageQueries{};
    std::array<MaterialAttachmentQuery, CALLBACK_COUNT> mCallbackOrder{};
    VkPhysicalDevice                                    mFeaturesPhysicalDevice = VK_NULL_HANDLE;
    std::size_t                                         mFeaturesCallCount      = 0;
    std::size_t                                         mPropertiesCallCount    = 0;
    std::size_t                                         mFormatCallCount        = 0;
    std::size_t                                         mImageCallCount         = 0;
    std::size_t                                         mCallbackCount          = 0;
    bool                                                mOverflow               = false;
};

void recordCallback(FakeState& state, MaterialAttachmentQuery query) noexcept
{
    if (state.mCallbackCount >= state.mCallbackOrder.size())
    {
        state.mOverflow = true;
        return;
    }
    state.mCallbackOrder[state.mCallbackCount++] = query;
}

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceFeatures(VkPhysicalDevice physical_device, VkPhysicalDeviceFeatures* features) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(physical_device);
    if (!state || !features)
    {
        return;
    }
    recordCallback(*state, MaterialAttachmentQuery::PhysicalDeviceFeatures);
    state->mFeaturesPhysicalDevice = physical_device;
    ++state->mFeaturesCallCount;
    *features = state->mFeatures;
}

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceProperties(VkPhysicalDevice            physical_device,
                                                           VkPhysicalDeviceProperties* properties) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(physical_device);
    if (!state || !properties)
    {
        return;
    }
    recordCallback(*state, MaterialAttachmentQuery::PhysicalDeviceProperties);
    ++state->mPropertiesCallCount;
    *properties = state->mProperties;
}

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceFormatProperties(VkPhysicalDevice physical_device, VkFormat format,
                                                                 VkFormatProperties* properties) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(physical_device);
    if (!state || !properties || state->mFormatCallCount >= FakeState::QUERY_COUNT)
    {
        if (state)
        {
            state->mOverflow = true;
        }
        return;
    }
    recordCallback(*state, MaterialAttachmentQuery::FormatProperties);
    const std::size_t index      = state->mFormatCallCount++;
    state->mFormatQueries[index] = format;
    *properties                  = state->mFormatOutputs[index];
}

VKAPI_ATTR VkResult VKAPI_CALL fakeGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physical_device, VkFormat format,
                                                                          VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage,
                                                                          VkImageCreateFlags       flags,
                                                                          VkImageFormatProperties* properties) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(physical_device);
    if (!state || !properties || state->mImageCallCount >= FakeState::QUERY_COUNT)
    {
        if (state)
        {
            state->mOverflow = true;
        }
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    recordCallback(*state, MaterialAttachmentQuery::ImageFormatProperties);
    const std::size_t index     = state->mImageCallCount++;
    state->mImageQueries[index] = { physical_device, format, type, tiling, usage, flags };
    *properties                 = state->mImageOutputs[index];
    return state->mImageResults[index];
}

MaterialAttachmentDevice fakeDevice(FakeState& state) noexcept
{
    return { reinterpret_cast<VkPhysicalDevice>(&state),
             { fakeGetPhysicalDeviceFeatures, fakeGetPhysicalDeviceProperties, fakeGetPhysicalDeviceFormatProperties,
               fakeGetPhysicalDeviceImageFormatProperties } };
}

const MaterialAttachmentResolutionError* resolutionError(const MaterialAttachmentResolutionResult& result) noexcept
{
    return std::get_if<MaterialAttachmentResolutionError>(&result);
}

const LegacyNormSpecAttachmentProfile* resolvedProfile(const MaterialAttachmentResolutionResult& result) noexcept
{
    return std::get_if<LegacyNormSpecAttachmentProfile>(&result);
}

void ensureInputError(const char* message, const MaterialAttachmentResolutionResult& result, MaterialAttachmentResolutionCode code)
{
    const auto* error = resolutionError(result);
    tut::ensure(message,
                error && error->mCode == code && !error->mQuery && !error->mFeature && !error->mLimit && !error->mCapability &&
                    !error->mAttachment && !error->mColorSlot && !error->mLogicalFormat && error->mNativeFormat == VK_FORMAT_UNDEFINED &&
                    error->mRequiredFeatures == 0 && error->mAvailableFeatures == 0 && error->mRequiredLimit == 0 &&
                    error->mAvailableLimit == 0 && error->mRequiredCapability == 0 && error->mAvailableCapability == 0 &&
                    error->mResult == VK_SUCCESS);
}

std::size_t callbackCount(const FakeState& state) noexcept
{
    return state.mFeaturesCallCount + state.mPropertiesCallCount + state.mFormatCallCount + state.mImageCallCount;
}

bool sameCapabilities(const VkImageFormatProperties& left, const VkImageFormatProperties& right) noexcept
{
    return left.maxExtent.width == right.maxExtent.width && left.maxExtent.height == right.maxExtent.height &&
           left.maxExtent.depth == right.maxExtent.depth && left.maxMipLevels == right.maxMipLevels &&
           left.maxArrayLayers == right.maxArrayLayers && left.sampleCounts == right.sampleCounts &&
           left.maxResourceSize == right.maxResourceSize;
}

} // namespace

namespace tut
{

struct render_vulkan_material_attachment_test
{
};

using render_vulkan_material_attachment_test_group  = test_group<render_vulkan_material_attachment_test>;
using render_vulkan_material_attachment_test_object = render_vulkan_material_attachment_test_group::object;
render_vulkan_material_attachment_test_group render_vulkan_material_attachment_tests("render vulkan material attachment");

template<>
template<>
void render_vulkan_material_attachment_test_object::test<1>()
{
    static_assert(LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT == 4);
    static_assert(std::variant_size_v<MaterialAttachmentResolutionResult> == 2);
    static_assert(std::is_same_v<std::variant_alternative_t<0, MaterialAttachmentResolutionResult>, MaterialAttachmentResolutionError>);
    static_assert(std::is_same_v<std::variant_alternative_t<1, MaterialAttachmentResolutionResult>, LegacyNormSpecAttachmentProfile>);
    static_assert(!std::is_aggregate_v<LegacyNormSpecAttachmentProfile>);
    static_assert(!std::is_default_constructible_v<LegacyNormSpecAttachmentProfile>);
    static_assert(!std::is_aggregate_v<MaterialAttachmentDeviceRequirements>);
    static_assert(!std::is_default_constructible_v<MaterialAttachmentDeviceRequirements>);
    static_assert(std::is_trivially_copyable_v<MaterialAttachmentDeviceRequirements>);
    static_assert(std::is_nothrow_move_constructible_v<MaterialAttachmentResolutionResult>);
    static_assert(noexcept(resolveLegacyNormSpecAttachmentProfile(std::declval<const MaterialAttachmentDevice&>(),
                                                                  std::declval<const LegacyNormSpecPipelineKey&>())));
}

template<>
template<>
void render_vulkan_material_attachment_test_object::test<2>()
{
    FakeState state;
    auto      result = resolveLegacyNormSpecAttachmentProfile({}, legacyNormSpecModernHDRPipelineKey());
    ensureInputError("a null physical device is rejected", result, MaterialAttachmentResolutionCode::InvalidPhysicalDevice);

    MaterialAttachmentDevice device             = fakeDevice(state);
    device.mDispatch.mGetPhysicalDeviceFeatures = nullptr;
    result                                      = resolveLegacyNormSpecAttachmentProfile(device, legacyNormSpecModernHDRPipelineKey());
    ensureInputError("missing physical-device features is rejected", result, MaterialAttachmentResolutionCode::InvalidDispatch);

    device                                        = fakeDevice(state);
    device.mDispatch.mGetPhysicalDeviceProperties = nullptr;
    result                                        = resolveLegacyNormSpecAttachmentProfile(device, legacyNormSpecModernHDRPipelineKey());
    ensureInputError("missing physical-device properties is rejected", result, MaterialAttachmentResolutionCode::InvalidDispatch);

    device                                              = fakeDevice(state);
    device.mDispatch.mGetPhysicalDeviceFormatProperties = nullptr;
    result = resolveLegacyNormSpecAttachmentProfile(device, legacyNormSpecModernHDRPipelineKey());
    ensureInputError("missing format properties is rejected", result, MaterialAttachmentResolutionCode::InvalidDispatch);

    device                                                   = fakeDevice(state);
    device.mDispatch.mGetPhysicalDeviceImageFormatProperties = nullptr;
    result = resolveLegacyNormSpecAttachmentProfile(device, legacyNormSpecModernHDRPipelineKey());
    ensureInputError("missing image-format properties is rejected", result, MaterialAttachmentResolutionCode::InvalidDispatch);

    LegacyNormSpecPipelineKey malformed = legacyNormSpecModernHDRPipelineKey();
    malformed.mSamples                  = 2;
    result                              = resolveLegacyNormSpecAttachmentProfile(fakeDevice(state), malformed);
    ensureInputError("a malformed production key is rejected", result, MaterialAttachmentResolutionCode::InvalidPipelineKey);

    result = resolveLegacyNormSpecAttachmentProfile(fakeDevice(state), legacyNormSpecDiagnosticPipelineKey());
    ensureInputError("the diagnostic profile is rejected", result, MaterialAttachmentResolutionCode::UnsupportedTargetProfile);
    ensure_equals("all input rejection happens before callbacks", callbackCount(state), std::size_t{ 0 });
}

template<>
template<>
void render_vulkan_material_attachment_test_object::test<3>()
{
    FakeState   state;
    const auto  key     = legacyNormSpecModernHDRPipelineKey();
    const auto  device  = fakeDevice(state);
    const auto  result  = resolveLegacyNormSpecAttachmentProfile(device, key);
    const auto* profile = resolvedProfile(result);
    ensure("the Modern HDR profile resolves", profile != nullptr);

    const std::array<VkFormat, 4> expected_formats{ VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_UNORM,
                                                    VK_FORMAT_R16G16B16A16_SFLOAT };
    const VkImageUsageFlags       expected_usage    = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    const VkFormatFeatureFlags    expected_features = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    const VkColorComponentFlags   rgba =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    const VkColorComponentFlags rgb = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

    ensure("the result retains canonical identity and physical-device provenance",
           profile->targetProfile() == LegacyNormSpecTargetProfile::ModernHDR && profile->selectedFor(device.mPhysicalDevice) &&
               !profile->selectedFor(fakeHandle<VkPhysicalDevice>(0x5eedU)));
    ensure("the supported profile retains the required logical-device feature", profile->deviceRequirements().independentBlendRequired());
    for (std::size_t slot = 0; slot < expected_formats.size(); ++slot)
    {
        const auto& color = profile->colors()[slot];
        ensure("every Modern HDR logical target maps exactly",
               color.mLogicalFormat == key.mColorTargets[slot].mFormat && color.mNativeFormat == expected_formats[slot] &&
                   color.mUsage == expected_usage && color.mRequiredFeatures == expected_features &&
                   sameCapabilities(color.mCapabilities, state.mImageOutputs[slot]));
        ensure("the first three targets preserve RGBA writes and zero clears",
               slot == 3 || (color.mWriteMask == rgba && color.mRequiredLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR &&
                             color.mAlphaSemantic == MaterialAttachmentAlphaSemantic::Stored &&
                             color.mClearColor == std::array<float, 4>{ 0.f, 0.f, 0.f, 0.f }));
    }
    ensure("the widened RGB16-float target preserves implicit alpha one",
           profile->colors()[3].mWriteMask == rgb && profile->colors()[3].mRequiredLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR &&
               profile->colors()[3].mAlphaSemantic == MaterialAttachmentAlphaSemantic::ImplicitOneAfterClear &&
               profile->colors()[3].mClearColor == std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f });
    ensure("logical depth24 widens to sampled D32 float",
           profile->depth().mLogicalFormat == PixelFormat::Depth24Unorm && profile->depth().mNativeFormat == VK_FORMAT_D32_SFLOAT &&
               profile->depth().mUsage == (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) &&
               profile->depth().mRequiredFeatures ==
                   (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
               profile->depth().mRequiredLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR && profile->depth().mClearDepth == 1.f &&
               profile->depth().mClearStencil == 0 && sameCapabilities(profile->depth().mCapabilities, state.mImageOutputs[4]));

    ensure_equals("physical features are queried once", state.mFeaturesCallCount, std::size_t{ 1 });
    ensure("physical features are queried from the profile's exact device", state.mFeaturesPhysicalDevice == device.mPhysicalDevice);
    ensure_equals("physical limits are queried once", state.mPropertiesCallCount, std::size_t{ 1 });
    ensure_equals("all five formats are queried", state.mFormatCallCount, FakeState::QUERY_COUNT);
    ensure_equals("all five exact image roles are queried", state.mImageCallCount, FakeState::QUERY_COUNT);
    ensure("the fake query recorder did not overflow", !state.mOverflow);
    ensure_equals("the complete query order is recorded", state.mCallbackCount, FakeState::CALLBACK_COUNT);
    ensure("features precede physical properties",
           state.mCallbackOrder[0] == MaterialAttachmentQuery::PhysicalDeviceFeatures &&
               state.mCallbackOrder[1] == MaterialAttachmentQuery::PhysicalDeviceProperties);
    for (std::size_t index = 0; index < FakeState::QUERY_COUNT; ++index)
    {
        const auto& query = state.mImageQueries[index];
        const bool  color = index < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
        ensure("format and image queries retain exact order", state.mFormatQueries[index] == query.mFormat);
        ensure("each ordered role queries format support before image capabilities",
               state.mCallbackOrder[2 + 2 * index] == MaterialAttachmentQuery::FormatProperties &&
                   state.mCallbackOrder[3 + 2 * index] == MaterialAttachmentQuery::ImageFormatProperties);
        ensure("the queried format is the one published for that attachment",
               query.mFormat == (color ? profile->colors()[index].mNativeFormat : profile->depth().mNativeFormat));
        ensure("every image query is exact 2D optimal tiling with no flags",
               query.mPhysicalDevice == device.mPhysicalDevice && query.mType == VK_IMAGE_TYPE_2D &&
                   query.mTiling == VK_IMAGE_TILING_OPTIMAL && query.mFlags == 0 &&
                   query.mUsage == (color ? expected_usage : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
        ensure("no nonportable three-channel Vulkan image format is queried",
               query.mFormat != VK_FORMAT_R8G8B8_UNORM && query.mFormat != VK_FORMAT_R16G16B16_SFLOAT);
    }
}

template<>
template<>
void render_vulkan_material_attachment_test_object::test<4>()
{
    FakeState   state;
    const auto  key     = legacyNormSpecCompatibilityPipelineKey();
    const auto  device  = fakeDevice(state);
    const auto  result  = resolveLegacyNormSpecAttachmentProfile(device, key);
    const auto* profile = resolvedProfile(result);
    ensure("the compatibility profile resolves", profile != nullptr);

    const std::array<VkFormat, 4> expected_formats{ VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32,
                                                    VK_FORMAT_R8G8B8A8_UNORM };
    ensure("the result retains compatibility identity and the required logical-device feature",
           profile->targetProfile() == LegacyNormSpecTargetProfile::Compatibility &&
               profile->deviceRequirements().independentBlendRequired());
    for (std::size_t slot = 0; slot < expected_formats.size(); ++slot)
    {
        ensure("every compatibility target maps exactly",
               profile->colors()[slot].mLogicalFormat == key.mColorTargets[slot].mFormat &&
                   profile->colors()[slot].mNativeFormat == expected_formats[slot] &&
                   state.mFormatQueries[slot] == expected_formats[slot] && state.mImageQueries[slot].mFormat == expected_formats[slot]);
    }
    ensure("RGB8 widens to RGBA8 with explicit RGB writes and alpha-one clear",
           profile->colors()[3].mWriteMask == (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT) &&
               profile->colors()[3].mRequiredLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR &&
               profile->colors()[3].mAlphaSemantic == MaterialAttachmentAlphaSemantic::ImplicitOneAfterClear &&
               profile->colors()[3].mClearColor == std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f });
    ensure("compatibility selection never queries a three-channel Vulkan image",
           state.mFormatQueries[3] != VK_FORMAT_R8G8B8_UNORM && state.mFormatQueries[3] != VK_FORMAT_R16G16B16_SFLOAT);
    ensure_equals("compatibility queries physical feature support once", state.mFeaturesCallCount, std::size_t{ 1 });
    ensure("compatibility queries the exact physical device before every later callback",
           state.mFeaturesPhysicalDevice == device.mPhysicalDevice && state.mCallbackCount == FakeState::CALLBACK_COUNT &&
               state.mCallbackOrder[0] == MaterialAttachmentQuery::PhysicalDeviceFeatures &&
               state.mCallbackOrder[1] == MaterialAttachmentQuery::PhysicalDeviceProperties);
    ensure_equals("compatibility still proves all five roles", state.mImageCallCount, FakeState::QUERY_COUNT);
}

template<>
template<>
void render_vulkan_material_attachment_test_object::test<5>()
{
    FakeState color_state;
    color_state.mProperties.limits.maxColorAttachments = 3;
    auto        result = resolveLegacyNormSpecAttachmentProfile(fakeDevice(color_state), legacyNormSpecModernHDRPipelineKey());
    const auto* error  = resolutionError(result);
    ensure("an insufficient color-attachment limit retains exact context",
           error && error->mCode == MaterialAttachmentResolutionCode::InsufficientLimit &&
               error->mQuery == MaterialAttachmentQuery::PhysicalDeviceProperties &&
               error->mLimit == MaterialAttachmentLimit::ColorAttachments && error->mRequiredLimit == 4 && error->mAvailableLimit == 3 &&
               !error->mAttachment);
    ensure_equals("a color limit failure stops after features and physical properties", callbackCount(color_state), std::size_t{ 2 });

    FakeState output_state;
    output_state.mProperties.limits.maxFragmentOutputAttachments = 3;
    result = resolveLegacyNormSpecAttachmentProfile(fakeDevice(output_state), legacyNormSpecCompatibilityPipelineKey());
    error  = resolutionError(result);
    ensure("an insufficient fragment-output limit retains exact context",
           error && error->mCode == MaterialAttachmentResolutionCode::InsufficientLimit &&
               error->mQuery == MaterialAttachmentQuery::PhysicalDeviceProperties &&
               error->mLimit == MaterialAttachmentLimit::FragmentOutputs && error->mRequiredLimit == 4 && error->mAvailableLimit == 3 &&
               !error->mAttachment);
    ensure_equals("an output limit failure stops after features and physical properties", callbackCount(output_state), std::size_t{ 2 });
}

template<>
template<>
void render_vulkan_material_attachment_test_object::test<6>()
{
    const auto key = legacyNormSpecModernHDRPipelineKey();
    for (std::size_t query = 0; query < FakeState::QUERY_COUNT; ++query)
    {
        const bool                                color = query < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
        const std::array<VkFormatFeatureFlags, 2> required_bits =
            color ? std::array<VkFormatFeatureFlags, 2>{ VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT }
                  : std::array<VkFormatFeatureFlags, 2>{ VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                         VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT };
        for (VkFormatFeatureFlags missing : required_bits)
        {
            FakeState state;
            state.mFormatOutputs[query].optimalTilingFeatures &= ~missing;
            state.mFormatOutputs[query].linearTilingFeatures |= missing;
            state.mFormatOutputs[query].bufferFeatures |= missing;
            const auto  result = resolveLegacyNormSpecAttachmentProfile(fakeDevice(state), key);
            const auto* error  = resolutionError(result);
            ensure("every missing optimal-tiling feature fails with attachment context",
                   error && error->mCode == MaterialAttachmentResolutionCode::MissingFormatFeatures &&
                       error->mQuery == MaterialAttachmentQuery::FormatProperties &&
                       error->mAttachment == (color ? MaterialAttachmentKind::Color : MaterialAttachmentKind::Depth) &&
                       error->mColorSlot == (color ? std::optional<std::uint32_t>{ static_cast<std::uint32_t>(query) } : std::nullopt) &&
                       error->mLogicalFormat == (color ? key.mColorTargets[query].mFormat : PixelFormat::Depth24Unorm) &&
                       error->mNativeFormat == state.mFormatQueries[query] && (error->mRequiredFeatures & missing) != 0 &&
                       (error->mAvailableFeatures & missing) == 0 && error->mResult == VK_SUCCESS);
            ensure_equals("a feature failure stops at its format query", state.mFormatCallCount, query + 1);
            ensure_equals("a feature failure does not issue its image query", state.mImageCallCount, query);
        }
    }
}

template<>
template<>
void render_vulkan_material_attachment_test_object::test<7>()
{
    const auto key = legacyNormSpecModernHDRPipelineKey();
    for (std::size_t query = 0; query < FakeState::QUERY_COUNT; ++query)
    {
        FakeState state;
        state.mImageResults[query]              = VK_ERROR_FORMAT_NOT_SUPPORTED;
        state.mImageOutputs[query].maxMipLevels = 99;
        state.mImageOutputs[query].sampleCounts = VK_SAMPLE_COUNT_1_BIT;
        const auto  result                      = resolveLegacyNormSpecAttachmentProfile(fakeDevice(state), key);
        const auto* error                       = resolutionError(result);
        const bool  color                       = query < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
        ensure("every image-format failure preserves its native result and attachment context",
               error && error->mCode == MaterialAttachmentResolutionCode::ImageFormatQueryFailure &&
                   error->mQuery == MaterialAttachmentQuery::ImageFormatProperties &&
                   error->mAttachment == (color ? MaterialAttachmentKind::Color : MaterialAttachmentKind::Depth) &&
                   error->mColorSlot == (color ? std::optional<std::uint32_t>{ static_cast<std::uint32_t>(query) } : std::nullopt) &&
                   error->mLogicalFormat == (color ? key.mColorTargets[query].mFormat : PixelFormat::Depth24Unorm) &&
                   error->mNativeFormat == state.mImageQueries[query].mFormat && error->mResult == VK_ERROR_FORMAT_NOT_SUPPORTED &&
                   !error->mCapability);
        ensure_equals("an image-format failure stops at its format query", state.mFormatCallCount, query + 1);
        ensure_equals("an image-format failure stops at its image query", state.mImageCallCount, query + 1);
        ensure("a failed Modern HDR format does not trigger compatibility fallback",
               state.mImageCallCount == query + 1 && !resolvedProfile(result));
    }
}

template<>
template<>
void render_vulkan_material_attachment_test_object::test<8>()
{
    const auto                                        key = legacyNormSpecModernHDRPipelineKey();
    const std::array<MaterialAttachmentCapability, 6> expected{
        MaterialAttachmentCapability::MipLevels,    MaterialAttachmentCapability::ArrayLayers, MaterialAttachmentCapability::ExtentWidth,
        MaterialAttachmentCapability::ExtentHeight, MaterialAttachmentCapability::ExtentDepth, MaterialAttachmentCapability::SampleCountOne
    };

    for (std::size_t case_index = 0; case_index < expected.size(); ++case_index)
    {
        FakeState state;
        auto&     capability = state.mImageOutputs[0];
        switch (case_index)
        {
            case 0:
                capability.maxMipLevels = 0;
                break;
            case 1:
                capability.maxArrayLayers = 0;
                break;
            case 2:
                capability.maxExtent.width = 0;
                break;
            case 3:
                capability.maxExtent.height = 0;
                break;
            case 4:
                capability.maxExtent.depth = 0;
                break;
            case 5:
                capability.sampleCounts = VK_SAMPLE_COUNT_4_BIT;
                break;
            default:
                break;
        }

        const auto          result    = resolveLegacyNormSpecAttachmentProfile(fakeDevice(state), key);
        const auto*         error     = resolutionError(result);
        const std::uint64_t available = case_index == 5 ? VK_SAMPLE_COUNT_4_BIT : 0;
        ensure("each insufficient image capability fails with the exact dimension",
               error && error->mCode == MaterialAttachmentResolutionCode::InsufficientImageCapability &&
                   error->mQuery == MaterialAttachmentQuery::ImageFormatProperties && error->mCapability == expected[case_index] &&
                   error->mAttachment == MaterialAttachmentKind::Color && error->mColorSlot == 0 && error->mRequiredCapability == 1 &&
                   error->mAvailableCapability == available && error->mResult == VK_SUCCESS);
        ensure_equals("first-color capability failure stops immediately", state.mImageCallCount, std::size_t{ 1 });
    }

    FakeState depth_state;
    depth_state.mImageOutputs[4].sampleCounts = VK_SAMPLE_COUNT_4_BIT;
    const auto  result                        = resolveLegacyNormSpecAttachmentProfile(fakeDevice(depth_state), key);
    const auto* error                         = resolutionError(result);
    ensure("depth capability failure retains depth context without a color slot",
           error && error->mCode == MaterialAttachmentResolutionCode::InsufficientImageCapability &&
               error->mCapability == MaterialAttachmentCapability::SampleCountOne && error->mAttachment == MaterialAttachmentKind::Depth &&
               !error->mColorSlot && error->mLogicalFormat == PixelFormat::Depth24Unorm && error->mNativeFormat == VK_FORMAT_D32_SFLOAT &&
               error->mRequiredCapability == VK_SAMPLE_COUNT_1_BIT && error->mAvailableCapability == VK_SAMPLE_COUNT_4_BIT);
    ensure_equals("depth capability failure follows four complete color queries", depth_state.mImageCallCount, FakeState::QUERY_COUNT);
}

template<>
template<>
void render_vulkan_material_attachment_test_object::test<9>()
{
    FakeState state;
    state.mFeatures.independentBlend = VK_FALSE;

    const auto  result = resolveLegacyNormSpecAttachmentProfile(fakeDevice(state), legacyNormSpecModernHDRPipelineKey());
    const auto* error  = resolutionError(result);
    ensure("missing independent-blend support retains exact typed context",
           error && error->mCode == MaterialAttachmentResolutionCode::MissingDeviceFeature &&
               error->mQuery == MaterialAttachmentQuery::PhysicalDeviceFeatures &&
               error->mFeature == MaterialAttachmentFeature::IndependentBlend && !error->mLimit && !error->mCapability &&
               !error->mAttachment && !error->mColorSlot && !error->mLogicalFormat && error->mNativeFormat == VK_FORMAT_UNDEFINED &&
               error->mRequiredFeatures == 0 && error->mAvailableFeatures == 0 && error->mRequiredLimit == 0 &&
               error->mAvailableLimit == 0 && error->mRequiredCapability == 0 && error->mAvailableCapability == 0 &&
               error->mResult == VK_SUCCESS && !resolvedProfile(result));
    ensure_equals("unsupported hardware performs exactly one feature query", state.mFeaturesCallCount, std::size_t{ 1 });
    ensure("unsupported hardware queries the supplied physical device",
           state.mFeaturesPhysicalDevice == reinterpret_cast<VkPhysicalDevice>(&state));
    ensure_equals("unsupported hardware performs no properties query", state.mPropertiesCallCount, std::size_t{ 0 });
    ensure_equals("unsupported hardware performs no format query", state.mFormatCallCount, std::size_t{ 0 });
    ensure_equals("unsupported hardware performs no image-format query", state.mImageCallCount, std::size_t{ 0 });
    ensure_equals("feature rejection records one callback", state.mCallbackCount, std::size_t{ 1 });
    ensure("the sole callback has physical-feature context",
           state.mCallbackOrder[0] == MaterialAttachmentQuery::PhysicalDeviceFeatures && !state.mOverflow);
}

} // namespace tut
