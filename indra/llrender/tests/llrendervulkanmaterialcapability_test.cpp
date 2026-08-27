/**
 * @file llrendervulkanmaterialcapability_test.cpp
 * @brief Tests for Vulkan material pipeline physical-device capability closure.
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

#include "llrendervulkanmaterialcapability.h"
#include "llshadermanifest.h"
#include "lltut.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
#include <variant>

namespace
{
using namespace LLRenderContract;
using namespace LLRenderVulkanMaterial;

constexpr std::size_t ATTACHMENT_QUERY_COUNT = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT + 1;
constexpr std::size_t MAX_QUEUE_FAMILIES     = 4;
constexpr std::size_t MAX_EXTENSION_STEPS    = 8;
constexpr std::size_t MAX_EXTENSIONS         = 3;
constexpr std::size_t MAX_PIPELINE_CALLBACKS = 16;

struct ExtensionStep
{
    VkResult      mResult      = VK_SUCCESS;
    std::uint32_t mOutputCount = 0;
};

struct FakePhysicalDevice
{
    FakePhysicalDevice()
    {
        mFeatures.independentBlend                      = VK_TRUE;
        mProperties.apiVersion                          = VK_API_VERSION_1_1;
        mProperties.limits.maxColorAttachments          = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
        mProperties.limits.maxFragmentOutputAttachments = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
        for (std::size_t index = 0; index < ATTACHMENT_QUERY_COUNT; ++index)
        {
            mFormatProperties[index].optimalTilingFeatures =
                index < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT
                    ? VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
                    : VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
            mImageProperties[index].maxExtent       = { 4096, 2048, 1 };
            mImageProperties[index].maxMipLevels    = 1;
            mImageProperties[index].maxArrayLayers  = 1;
            mImageProperties[index].sampleCounts    = VK_SAMPLE_COUNT_1_BIT;
            mImageProperties[index].maxResourceSize = 1ULL << 30;
        }

        mQueueFirstCount                = 1;
        mQueueSecondCount               = 1;
        mQueueFamilies[0].queueFlags    = VK_QUEUE_GRAPHICS_BIT;
        mQueueFamilies[0].queueCount    = 1;
        mExtensionSteps[0].mOutputCount = 0;
        mExtensionStepCount             = 1;
        mPortabilityStrideAlignment     = 1;
    }

    VkPhysicalDeviceFeatures                                    mFeatures{};
    VkPhysicalDeviceProperties                                  mProperties{};
    std::array<VkFormatProperties, ATTACHMENT_QUERY_COUNT>      mFormatProperties{};
    std::array<VkImageFormatProperties, ATTACHMENT_QUERY_COUNT> mImageProperties{};
    std::size_t                                                 mFormatCalls = 0;
    std::size_t                                                 mImageCalls  = 0;

    std::uint32_t                                           mQueueFirstCount  = 0;
    std::uint32_t                                           mQueueSecondCount = 0;
    std::array<VkQueueFamilyProperties, MAX_QUEUE_FAMILIES> mQueueFamilies{};
    std::array<ExtensionStep, MAX_EXTENSION_STEPS>          mExtensionSteps{};
    std::size_t                                             mExtensionStepCount = 0;
    std::array<VkExtensionProperties, MAX_EXTENSIONS>       mExtensions{};
    std::size_t                                             mExtensionCount             = 0;
    std::uint32_t                                           mPortabilityStrideAlignment = 1;

    std::size_t                                                         mPropertyCalls         = 0;
    std::size_t                                                         mQueueCalls            = 0;
    std::size_t                                                         mExtensionCalls        = 0;
    std::size_t                                                         mProperties2Calls      = 0;
    VkPhysicalDevice                                                    mLastPropertyDevice    = VK_NULL_HANDLE;
    VkPhysicalDevice                                                    mLastQueueDevice       = VK_NULL_HANDLE;
    VkPhysicalDevice                                                    mLastExtensionDevice   = VK_NULL_HANDLE;
    VkPhysicalDevice                                                    mLastProperties2Device = VK_NULL_HANDLE;
    bool                                                                mExtensionLayerWasNull = true;
    bool                                                                mProperties2ChainValid = true;
    bool                                                                mOverflow              = false;
    std::array<MaterialPipelineCapabilityQuery, MAX_PIPELINE_CALLBACKS> mPipelineOrder{};
    std::size_t                                                         mPipelineOrderCount = 0;
};

void recordPipelineCallback(FakePhysicalDevice& state, MaterialPipelineCapabilityQuery query) noexcept
{
    if (state.mPipelineOrderCount >= state.mPipelineOrder.size())
    {
        state.mOverflow = true;
        return;
    }
    state.mPipelineOrder[state.mPipelineOrderCount++] = query;
}

void resetPipelineObservations(FakePhysicalDevice& state) noexcept
{
    state.mPropertyCalls         = 0;
    state.mQueueCalls            = 0;
    state.mExtensionCalls        = 0;
    state.mProperties2Calls      = 0;
    state.mLastPropertyDevice    = VK_NULL_HANDLE;
    state.mLastQueueDevice       = VK_NULL_HANDLE;
    state.mLastExtensionDevice   = VK_NULL_HANDLE;
    state.mLastProperties2Device = VK_NULL_HANDLE;
    state.mExtensionLayerWasNull = true;
    state.mProperties2ChainValid = true;
    state.mOverflow              = false;
    state.mPipelineOrderCount    = 0;
    state.mPipelineOrder.fill(MaterialPipelineCapabilityQuery::PhysicalDeviceProperties);
}

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceFeatures(VkPhysicalDevice physical_device, VkPhysicalDeviceFeatures* features) noexcept
{
    auto* state = reinterpret_cast<FakePhysicalDevice*>(physical_device);
    if (state && features)
    {
        *features = state->mFeatures;
    }
}

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceProperties(VkPhysicalDevice            physical_device,
                                                           VkPhysicalDeviceProperties* properties) noexcept
{
    auto* state = reinterpret_cast<FakePhysicalDevice*>(physical_device);
    if (state && properties)
    {
        ++state->mPropertyCalls;
        state->mLastPropertyDevice = physical_device;
        recordPipelineCallback(*state, MaterialPipelineCapabilityQuery::PhysicalDeviceProperties);
        *properties = state->mProperties;
    }
}

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceFormatProperties(VkPhysicalDevice    physical_device, VkFormat,
                                                                 VkFormatProperties* properties) noexcept
{
    auto* state = reinterpret_cast<FakePhysicalDevice*>(physical_device);
    if (!state || !properties || state->mFormatCalls >= state->mFormatProperties.size())
    {
        if (state)
        {
            state->mOverflow = true;
        }
        return;
    }
    *properties = state->mFormatProperties[state->mFormatCalls++];
}

VKAPI_ATTR VkResult VKAPI_CALL fakeGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physical_device,
                                                                          VkFormat,
                                                                          VkImageType,
                                                                          VkImageTiling,
                                                                          VkImageUsageFlags,
                                                                          VkImageCreateFlags,
                                                                          VkImageFormatProperties* properties) noexcept
{
    auto* state = reinterpret_cast<FakePhysicalDevice*>(physical_device);
    if (!state || !properties || state->mImageCalls >= state->mImageProperties.size())
    {
        if (state)
        {
            state->mOverflow = true;
        }
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *properties = state->mImageProperties[state->mImageCalls++];
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice         physical_device,
                                                                      std::uint32_t*           count,
                                                                      VkQueueFamilyProperties* properties) noexcept
{
    auto* state = reinterpret_cast<FakePhysicalDevice*>(physical_device);
    if (!state || !count)
    {
        return;
    }
    ++state->mQueueCalls;
    state->mLastQueueDevice = physical_device;
    recordPipelineCallback(*state, MaterialPipelineCapabilityQuery::QueueFamilyProperties);
    if (!properties)
    {
        *count = state->mQueueFirstCount;
        return;
    }

    const std::uint32_t capacity = *count;
    const std::uint32_t copied = std::min({ capacity, state->mQueueSecondCount, static_cast<std::uint32_t>(state->mQueueFamilies.size()) });
    for (std::uint32_t index = 0; index < copied; ++index)
    {
        properties[index] = state->mQueueFamilies[index];
    }
    *count = state->mQueueSecondCount;
}

VKAPI_ATTR VkResult VKAPI_CALL fakeEnumerateDeviceExtensionProperties(VkPhysicalDevice       physical_device,
                                                                      const char*            layer_name,
                                                                      std::uint32_t*         count,
                                                                      VkExtensionProperties* properties) noexcept
{
    auto* state = reinterpret_cast<FakePhysicalDevice*>(physical_device);
    if (!state || !count || state->mExtensionCalls >= state->mExtensionStepCount)
    {
        if (state)
        {
            state->mOverflow = true;
        }
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    state->mLastExtensionDevice   = physical_device;
    state->mExtensionLayerWasNull = state->mExtensionLayerWasNull && layer_name == nullptr;
    recordPipelineCallback(*state, MaterialPipelineCapabilityQuery::DeviceExtensionProperties);

    const ExtensionStep step     = state->mExtensionSteps[state->mExtensionCalls++];
    const std::uint32_t capacity = properties ? *count : 0;
    if (properties)
    {
        const std::uint32_t copied = std::min({ capacity, step.mOutputCount, static_cast<std::uint32_t>(state->mExtensionCount) });
        for (std::uint32_t index = 0; index < copied; ++index)
        {
            properties[index] = state->mExtensions[index];
        }
    }
    *count = step.mOutputCount;
    return step.mResult;
}

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceProperties2(VkPhysicalDevice             physical_device,
                                                            VkPhysicalDeviceProperties2* properties) noexcept
{
    auto* state = reinterpret_cast<FakePhysicalDevice*>(physical_device);
    if (!state || !properties)
    {
        return;
    }
    ++state->mProperties2Calls;
    state->mLastProperties2Device = physical_device;
    recordPipelineCallback(*state, MaterialPipelineCapabilityQuery::PhysicalDeviceProperties2);

    const bool outer_valid = properties->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 && properties->pNext;
    auto*      portability = static_cast<VkPhysicalDevicePortabilitySubsetPropertiesKHR*>(properties->pNext);
    const bool inner_valid = outer_valid && portability->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_PROPERTIES_KHR &&
                             portability->pNext == nullptr;
    state->mProperties2ChainValid = state->mProperties2ChainValid && inner_valid;
    properties->properties        = state->mProperties;
    if (inner_valid)
    {
        portability->minVertexInputBindingStrideAlignment = state->mPortabilityStrideAlignment;
    }
}

MaterialAttachmentDevice attachmentDevice(FakePhysicalDevice& state) noexcept
{
    return { reinterpret_cast<VkPhysicalDevice>(&state),
             { fakeGetPhysicalDeviceFeatures, fakeGetPhysicalDeviceProperties, fakeGetPhysicalDeviceFormatProperties,
               fakeGetPhysicalDeviceImageFormatProperties } };
}

MaterialPipelineCapabilityDevice capabilityDevice(FakePhysicalDevice& state) noexcept
{
    return { reinterpret_cast<VkPhysicalDevice>(&state),
             { fakeGetPhysicalDeviceProperties, fakeGetPhysicalDeviceQueueFamilyProperties, fakeEnumerateDeviceExtensionProperties,
               fakeGetPhysicalDeviceProperties2 } };
}

MaterialAttachmentResolutionResult resolveAttachment(FakePhysicalDevice& state, const LegacyNormSpecPipelineKey& key) noexcept
{
    return resolveLegacyNormSpecAttachmentProfile(attachmentDevice(state), key);
}

const LegacyNormSpecAttachmentProfile* attachmentProfile(const MaterialAttachmentResolutionResult& result) noexcept
{
    return std::get_if<LegacyNormSpecAttachmentProfile>(&result);
}

const MaterialPipelineCapabilityResolutionError* capabilityError(const MaterialPipelineCapabilityResolutionResult& result) noexcept
{
    return std::get_if<MaterialPipelineCapabilityResolutionError>(&result);
}

const LegacyNormSpecPipelineCapabilityProfile* capabilityProfile(const MaterialPipelineCapabilityResolutionResult& result) noexcept
{
    return std::get_if<LegacyNormSpecPipelineCapabilityProfile>(&result);
}

void configureExtensions(FakePhysicalDevice& state,
                         std::initializer_list<ExtensionStep>
                                                            steps,
                         std::initializer_list<const char*> names = {})
{
    state.mExtensionSteps.fill({});
    state.mExtensionStepCount = steps.size();
    std::copy(steps.begin(), steps.end(), state.mExtensionSteps.begin());
    state.mExtensions.fill({});
    state.mExtensionCount = names.size();
    std::size_t index     = 0;
    for (const char* name : names)
    {
        std::strncpy(state.mExtensions[index++].extensionName, name, VK_MAX_EXTENSION_NAME_SIZE - 1);
    }
}

std::size_t pipelineCallbackCount(const FakePhysicalDevice& state) noexcept
{
    return state.mPropertyCalls + state.mQueueCalls + state.mExtensionCalls + state.mProperties2Calls;
}

void ensureBareError(const char*                                       message,
                     const MaterialPipelineCapabilityResolutionResult& result,
                     MaterialPipelineCapabilityResolutionCode          code)
{
    const auto* error = capabilityError(result);
    tut::ensure(message,
                error && error->mCode == code && !error->mQuery && !error->mVertexBinding && error->mRequiredValue == 0 &&
                    error->mAvailableValue == 0 && error->mEnumerationAttempt == 0 && error->mResult == VK_SUCCESS);
}

bool sameImageProperties(const VkImageFormatProperties& left, const VkImageFormatProperties& right) noexcept
{
    return left.maxExtent.width == right.maxExtent.width && left.maxExtent.height == right.maxExtent.height &&
           left.maxExtent.depth == right.maxExtent.depth && left.maxMipLevels == right.maxMipLevels &&
           left.maxArrayLayers == right.maxArrayLayers && left.sampleCounts == right.sampleCounts &&
           left.maxResourceSize == right.maxResourceSize;
}

bool sameAttachmentProfile(const LegacyNormSpecAttachmentProfile& left,
                           const LegacyNormSpecAttachmentProfile& right,
                           VkPhysicalDevice                       physical_device) noexcept
{
    if (left.targetProfile() != right.targetProfile() || !left.selectedFor(physical_device) || !right.selectedFor(physical_device) ||
        left.deviceRequirements() != right.deviceRequirements())
    {
        return false;
    }
    for (std::size_t slot = 0; slot < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT; ++slot)
    {
        const auto& l = left.colors()[slot];
        const auto& r = right.colors()[slot];
        if (l.mLogicalFormat != r.mLogicalFormat || l.mNativeFormat != r.mNativeFormat || l.mUsage != r.mUsage ||
            l.mRequiredFeatures != r.mRequiredFeatures || l.mWriteMask != r.mWriteMask || l.mRequiredLoadOp != r.mRequiredLoadOp ||
            l.mAlphaSemantic != r.mAlphaSemantic || l.mClearColor != r.mClearColor ||
            !sameImageProperties(l.mCapabilities, r.mCapabilities))
        {
            return false;
        }
    }
    const auto& l = left.depth();
    const auto& r = right.depth();
    return l.mLogicalFormat == r.mLogicalFormat && l.mNativeFormat == r.mNativeFormat && l.mUsage == r.mUsage &&
           l.mRequiredFeatures == r.mRequiredFeatures && l.mRequiredLoadOp == r.mRequiredLoadOp && l.mClearDepth == r.mClearDepth &&
           l.mClearStencil == r.mClearStencil && sameImageProperties(l.mCapabilities, r.mCapabilities);
}

std::uint32_t componentCount(ShaderValueType type) noexcept
{
    switch (type)
    {
        case ShaderValueType::Float:
            return 1;
        case ShaderValueType::Float2:
            return 2;
        case ShaderValueType::Float3:
            return 3;
        case ShaderValueType::Float4:
            return 4;
    }
    return 0;
}

} // namespace

namespace tut
{

struct render_vulkan_material_capability_test
{
};

using render_vulkan_material_capability_test_group  = test_group<render_vulkan_material_capability_test>;
using render_vulkan_material_capability_test_object = render_vulkan_material_capability_test_group::object;
render_vulkan_material_capability_test_group render_vulkan_material_capability_tests("render vulkan material capability");

template<>
template<>
void render_vulkan_material_capability_test_object::test<1>()
{
    static_assert(LEGACY_NORMSPEC_VERTEX_INPUT_COUNT == 7);
    static_assert(std::variant_size_v<MaterialPipelineCapabilityResolutionResult> == 2);
    static_assert(std::is_same_v<std::variant_alternative_t<0, MaterialPipelineCapabilityResolutionResult>,
                                 MaterialPipelineCapabilityResolutionError>);
    static_assert(
        std::is_same_v<std::variant_alternative_t<1, MaterialPipelineCapabilityResolutionResult>, LegacyNormSpecPipelineCapabilityProfile>);
    static_assert(!std::is_aggregate_v<LegacyNormSpecPipelineCapabilityProfile>);
    static_assert(!std::is_default_constructible_v<LegacyNormSpecPipelineCapabilityProfile>);
    static_assert(!std::is_aggregate_v<MaterialPipelineLogicalDeviceRequirements>);
    static_assert(!std::is_default_constructible_v<MaterialPipelineLogicalDeviceRequirements>);
    static_assert(!std::is_constructible_v<MaterialPipelineLogicalDeviceRequirements, bool, bool>);
    static_assert(std::is_trivially_copyable_v<MaterialPipelineLogicalDeviceRequirements>);
    static_assert(std::is_nothrow_move_constructible_v<MaterialPipelineCapabilityResolutionResult>);
    static_assert(std::is_same_v<decltype(std::declval<const LegacyNormSpecPipelineCapabilityProfile&>().attachmentProfile()),
                                 const LegacyNormSpecAttachmentProfile&>);
    static_assert(std::is_same_v<decltype(std::declval<const LegacyNormSpecPipelineCapabilityProfile&>().vertexInputs()),
                                 const std::array<MaterialVertexInputCapability, LEGACY_NORMSPEC_VERTEX_INPUT_COUNT>&>);
    static_assert(noexcept(resolveLegacyNormSpecPipelineCapabilityProfile(std::declval<const MaterialPipelineCapabilityDevice&>(),
                                                                          std::declval<const LegacyNormSpecAttachmentProfile&>())));
}

template<>
template<>
void render_vulkan_material_capability_test_object::test<2>()
{
    FakePhysicalDevice profile_state;
    auto               attachment_result = resolveAttachment(profile_state, legacyNormSpecModernHDRPipelineKey());
    const auto*        attachment        = attachmentProfile(attachment_result);
    ensure("the preflight fixture resolves a real attachment profile", attachment != nullptr);
    resetPipelineObservations(profile_state);

    auto device            = capabilityDevice(profile_state);
    device.mPhysicalDevice = VK_NULL_HANDLE;
    auto result            = resolveLegacyNormSpecPipelineCapabilityProfile(device, *attachment);
    ensureBareError("a null physical device is rejected", result, MaterialPipelineCapabilityResolutionCode::InvalidPhysicalDevice);

    device                                        = capabilityDevice(profile_state);
    device.mDispatch.mGetPhysicalDeviceProperties = nullptr;
    result                                        = resolveLegacyNormSpecPipelineCapabilityProfile(device, *attachment);
    ensureBareError("missing physical-device properties is rejected", result, MaterialPipelineCapabilityResolutionCode::InvalidDispatch);

    device                                                   = capabilityDevice(profile_state);
    device.mDispatch.mGetPhysicalDeviceQueueFamilyProperties = nullptr;
    result                                                   = resolveLegacyNormSpecPipelineCapabilityProfile(device, *attachment);
    ensureBareError("missing queue-family properties is rejected", result, MaterialPipelineCapabilityResolutionCode::InvalidDispatch);

    device                                               = capabilityDevice(profile_state);
    device.mDispatch.mEnumerateDeviceExtensionProperties = nullptr;
    result                                               = resolveLegacyNormSpecPipelineCapabilityProfile(device, *attachment);
    ensureBareError("missing device-extension enumeration is rejected", result, MaterialPipelineCapabilityResolutionCode::InvalidDispatch);

    device                                         = capabilityDevice(profile_state);
    device.mDispatch.mGetPhysicalDeviceProperties2 = nullptr;
    result                                         = resolveLegacyNormSpecPipelineCapabilityProfile(device, *attachment);
    ensureBareError("missing physical-device properties2 is rejected", result, MaterialPipelineCapabilityResolutionCode::InvalidDispatch);
    ensure_equals("all null and dispatch rejection precedes callbacks", pipelineCallbackCount(profile_state), std::size_t{ 0 });

    FakePhysicalDevice other_state;
    result = resolveLegacyNormSpecPipelineCapabilityProfile(capabilityDevice(other_state), *attachment);
    ensureBareError("an attachment profile from another physical device is rejected", result,
                    MaterialPipelineCapabilityResolutionCode::AttachmentProfilePhysicalDeviceMismatch);
    ensure_equals("provenance mismatch precedes callbacks", pipelineCallbackCount(other_state), std::size_t{ 0 });
}

template<>
template<>
void render_vulkan_material_capability_test_object::test<3>()
{
    FakePhysicalDevice state;
    state.mProperties.apiVersion  = VK_API_VERSION_1_0;
    auto        attachment_result = resolveAttachment(state, legacyNormSpecCompatibilityPipelineKey());
    const auto* attachment        = attachmentProfile(attachment_result);
    ensure("the API-floor fixture resolves its attachment profile", attachment != nullptr);
    resetPipelineObservations(state);

    const auto  result = resolveLegacyNormSpecPipelineCapabilityProfile(capabilityDevice(state), *attachment);
    const auto* error  = capabilityError(result);
    ensure("Vulkan 1.0 is rejected with the exact API floor",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::InsufficientApiVersion &&
               error->mQuery == MaterialPipelineCapabilityQuery::PhysicalDeviceProperties && error->mRequiredValue == VK_API_VERSION_1_1 &&
               error->mAvailableValue == VK_API_VERSION_1_0 && !error->mVertexBinding && error->mResult == VK_SUCCESS);
    ensure_equals("API rejection performs only the properties query", pipelineCallbackCount(state), std::size_t{ 1 });

    FakePhysicalDevice variant_state;
    variant_state.mProperties.apiVersion = VK_MAKE_API_VERSION(1, 1, 1, 0);
    attachment_result                    = resolveAttachment(variant_state, legacyNormSpecModernHDRPipelineKey());
    attachment                           = attachmentProfile(attachment_result);
    ensure("the API-variant fixture resolves its attachment profile", attachment != nullptr);
    resetPipelineObservations(variant_state);
    const auto variant_result = resolveLegacyNormSpecPipelineCapabilityProfile(capabilityDevice(variant_state), *attachment);
    error                     = capabilityError(variant_result);
    ensure("a nonzero API variant is rejected before numeric version comparison",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::UnsupportedApiVariant &&
               error->mQuery == MaterialPipelineCapabilityQuery::PhysicalDeviceProperties && error->mRequiredValue == 0 &&
               error->mAvailableValue == 1 && !error->mVertexBinding && error->mResult == VK_SUCCESS);
    ensure_equals("API-variant rejection performs only the properties query", pipelineCallbackCount(variant_state), std::size_t{ 1 });
}

template<>
template<>
void render_vulkan_material_capability_test_object::test<4>()
{
    const std::array<LegacyNormSpecPipelineKey, 2> keys{ legacyNormSpecModernHDRPipelineKey(), legacyNormSpecCompatibilityPipelineKey() };
    const std::array<MaterialVertexInputCapability, LEGACY_NORMSPEC_VERTEX_INPUT_COUNT> expected_inputs{
        MaterialVertexInputCapability{ VertexSemantic::Position, VertexFormat::Float3, 0, 0, 0, 16, VK_FORMAT_R32G32B32_SFLOAT },
        MaterialVertexInputCapability{ VertexSemantic::Normal, VertexFormat::Float3, 1, 1, 0, 16, VK_FORMAT_R32G32B32_SFLOAT },
        MaterialVertexInputCapability{ VertexSemantic::TexCoord0, VertexFormat::Float2, 2, 2, 0, 8, VK_FORMAT_R32G32_SFLOAT },
        MaterialVertexInputCapability{ VertexSemantic::Color, VertexFormat::UNorm8x4, 3, 3, 0, 4, VK_FORMAT_R8G8B8A8_UNORM },
        MaterialVertexInputCapability{ VertexSemantic::Tangent, VertexFormat::Float4, 4, 4, 0, 16, VK_FORMAT_R32G32B32A32_SFLOAT },
        MaterialVertexInputCapability{ VertexSemantic::TexCoord1, VertexFormat::Float2, 5, 5, 0, 8, VK_FORMAT_R32G32_SFLOAT },
        MaterialVertexInputCapability{ VertexSemantic::TexCoord2, VertexFormat::Float2, 6, 6, 0, 8, VK_FORMAT_R32G32_SFLOAT }
    };
    constexpr std::uint32_t accepted_api = VK_MAKE_API_VERSION(0, 1, 2, 203);

    for (const auto& key : keys)
    {
        FakePhysicalDevice state;
        state.mProperties.apiVersion  = accepted_api;
        auto        attachment_result = resolveAttachment(state, key);
        const auto* attachment        = attachmentProfile(attachment_result);
        ensure("both production attachment profiles resolve", attachment != nullptr);
        resetPipelineObservations(state);

        const auto  result  = resolveLegacyNormSpecPipelineCapabilityProfile(capabilityDevice(state), *attachment);
        const auto* profile = capabilityProfile(result);
        ensure("both production pipeline capability profiles resolve", profile != nullptr);
        ensure("the successful profile preserves physical-device and target provenance",
               profile->selectedFor(reinterpret_cast<VkPhysicalDevice>(&state)) &&
                   !profile->selectedFor(reinterpret_cast<VkPhysicalDevice>(std::uintptr_t{ 0x5eed })) &&
                   profile->targetProfile() == key.mTargetProfile);
        ensure("the exact attachment profile is retained immutably",
               &profile->attachmentProfile() != attachment &&
                   sameAttachmentProfile(profile->attachmentProfile(), *attachment, reinterpret_cast<VkPhysicalDevice>(&state)));
        ensure("the exact API and neutral portability state are retained",
               profile->apiVersion() == accepted_api && !profile->portabilitySubsetAdvertised() &&
                   profile->minVertexInputBindingStrideAlignment() == 1);
        const auto requirements = profile->logicalDeviceRequirements();
        ensure("all successful logical-device obligations are explicit",
               requirements.independentBlendRequired() && requirements.graphicsQueueRequired() &&
                   !requirements.portabilitySubsetExtensionRequired());
        ensure("all canonical vertex records are retained exactly", profile->vertexInputs() == expected_inputs);

        const auto manifest = legacyNormSpecShaderManifest(key, ShaderBackend::Vulkan);
        ensure("the successful profile has the exact production shader manifest",
               manifest && validLegacyNormSpecProductionShaderManifest(*manifest) &&
                   manifest->mVertexInputs.size() == profile->vertexInputs().size());
        for (std::size_t index = 0; index < profile->vertexInputs().size(); ++index)
        {
            const auto& input    = profile->vertexInputs()[index];
            const auto& declared = manifest->mVertexInputs[index];
            ensure("each retained vertex record agrees with the shader manifest",
                   input.mSemantic == declared.mSemantic && input.mLogicalFormat == declared.mFormat &&
                       input.mLocation == declared.mLocation && input.mBinding == declared.mBinding && input.mOffset == 0 &&
                       input.mStride == declared.mStride);
        }

        std::uint32_t interstage_components = 0;
        for (const auto& variable : manifest->mInterstageVariables)
        {
            interstage_components += componentCount(variable.mType);
        }
        std::uint32_t descriptor_sets = 0;
        for (const auto& image : manifest->mSampledImages)
        {
            descriptor_sets = std::max(descriptor_sets, image.mSet + 1);
        }
        if (manifest->mParameterBlock)
        {
            descriptor_sets = std::max(descriptor_sets, manifest->mParameterBlock->mSet + 1);
        }
        ensure("the fixed contract fits Vulkan 1.1 core vertex and interstage floors",
               manifest->mVertexInputs.size() <= 16 && interstage_components == 20 && interstage_components <= 64 &&
                   expected_inputs.back().mBinding < 16 && expected_inputs[0].mStride <= 2048);
        ensure("the fixed contract fits Vulkan 1.1 descriptor and uniform floors",
               descriptor_sets == 2 && descriptor_sets <= 4 && manifest->mSampledImages.size() == 3 &&
                   manifest->mSampledImages.size() <= 16 && manifest->mParameterBlock && manifest->mParameterBlock->mByteSize == 272 &&
                   manifest->mParameterBlock->mByteSize <= 16384);
        ensure("the fixed contract fits Vulkan 1.1 output and fixed-function floors",
               manifest->mLogicalFragmentOutputs.size() == 4 && manifest->mLogicalFragmentOutputs.size() <= 4 &&
                   manifest->mPushConstantRanges.empty());

        ensure("the successful query transaction uses the exact physical device and null extension layer",
               state.mLastPropertyDevice == reinterpret_cast<VkPhysicalDevice>(&state) &&
                   state.mLastQueueDevice == reinterpret_cast<VkPhysicalDevice>(&state) &&
                   state.mLastExtensionDevice == reinterpret_cast<VkPhysicalDevice>(&state) && state.mExtensionLayerWasNull &&
                   state.mProperties2Calls == 0 && !state.mOverflow);
        ensure("the non-portability query order is fixed",
               state.mPipelineOrderCount == 4 && state.mPipelineOrder[0] == MaterialPipelineCapabilityQuery::PhysicalDeviceProperties &&
                   state.mPipelineOrder[1] == MaterialPipelineCapabilityQuery::QueueFamilyProperties &&
                   state.mPipelineOrder[2] == MaterialPipelineCapabilityQuery::QueueFamilyProperties &&
                   state.mPipelineOrder[3] == MaterialPipelineCapabilityQuery::DeviceExtensionProperties);
    }
}

template<>
template<>
void render_vulkan_material_capability_test_object::test<5>()
{
    auto resolve_queue_case = [](FakePhysicalDevice& state)
    {
        auto        attachment_result = resolveAttachment(state, legacyNormSpecModernHDRPipelineKey());
        const auto* attachment        = attachmentProfile(attachment_result);
        tut::ensure("the queue fixture resolves an attachment profile", attachment != nullptr);
        resetPipelineObservations(state);
        return resolveLegacyNormSpecPipelineCapabilityProfile(capabilityDevice(state), *attachment);
    };

    FakePhysicalDevice empty;
    empty.mQueueFirstCount = 0;
    auto        result     = resolve_queue_case(empty);
    const auto* error      = capabilityError(result);
    ensure("zero queue families fail closed",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::MissingGraphicsQueueFamily &&
               error->mQuery == MaterialPipelineCapabilityQuery::QueueFamilyProperties);
    ensure_equals("zero queue families need only the count call", empty.mQueueCalls, std::size_t{ 1 });

    FakePhysicalDevice compute;
    compute.mQueueFamilies[0].queueFlags = VK_QUEUE_COMPUTE_BIT;
    result                               = resolve_queue_case(compute);
    error                                = capabilityError(result);
    ensure("a non-empty compute-only family is insufficient",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::MissingGraphicsQueueFamily &&
               error->mQuery == MaterialPipelineCapabilityQuery::QueueFamilyProperties);

    FakePhysicalDevice zero_count;
    zero_count.mQueueFamilies[0].queueFlags = VK_QUEUE_GRAPHICS_BIT;
    zero_count.mQueueFamilies[0].queueCount = 0;
    result                                  = resolve_queue_case(zero_count);
    error                                   = capabilityError(result);
    ensure("a graphics flag with no queues is insufficient",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::MissingGraphicsQueueFamily &&
               error->mQuery == MaterialPipelineCapabilityQuery::QueueFamilyProperties);

    FakePhysicalDevice later_valid;
    later_valid.mQueueFirstCount             = 3;
    later_valid.mQueueSecondCount            = 3;
    later_valid.mQueueFamilies[0].queueFlags = VK_QUEUE_COMPUTE_BIT;
    later_valid.mQueueFamilies[0].queueCount = 1;
    later_valid.mQueueFamilies[1].queueFlags = VK_QUEUE_GRAPHICS_BIT;
    later_valid.mQueueFamilies[1].queueCount = 0;
    later_valid.mQueueFamilies[2].queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
    later_valid.mQueueFamilies[2].queueCount = 2;
    result                                   = resolve_queue_case(later_valid);
    ensure("a later non-empty graphics family is accepted", capabilityProfile(result) != nullptr);

    FakePhysicalDevice invalid_output;
    invalid_output.mQueueFirstCount  = 1;
    invalid_output.mQueueSecondCount = 2;
    result                           = resolve_queue_case(invalid_output);
    error                            = capabilityError(result);
    ensure("a second queue count above capacity is rejected with exact bounds",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::InvalidEnumerationOutput &&
               error->mQuery == MaterialPipelineCapabilityQuery::QueueFamilyProperties && error->mRequiredValue == 1 &&
               error->mAvailableValue == 2);
}

template<>
template<>
void render_vulkan_material_capability_test_object::test<6>()
{
    auto resolve_extension_case = [](FakePhysicalDevice& state)
    {
        auto        attachment_result = resolveAttachment(state, legacyNormSpecCompatibilityPipelineKey());
        const auto* attachment        = attachmentProfile(attachment_result);
        tut::ensure("the extension fixture resolves an attachment profile", attachment != nullptr);
        resetPipelineObservations(state);
        return resolveLegacyNormSpecPipelineCapabilityProfile(capabilityDevice(state), *attachment);
    };

    FakePhysicalDevice count_failure;
    configureExtensions(count_failure, { { VK_ERROR_INITIALIZATION_FAILED, 0 } });
    auto        result = resolve_extension_case(count_failure);
    const auto* error  = capabilityError(result);
    ensure("a count query failure preserves result and attempt",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::EnumerationFailure &&
               error->mQuery == MaterialPipelineCapabilityQuery::DeviceExtensionProperties &&
               error->mResult == VK_ERROR_INITIALIZATION_FAILED && error->mEnumerationAttempt == 1);

    FakePhysicalDevice list_failure;
    configureExtensions(list_failure, { { VK_SUCCESS, 1 }, { VK_ERROR_OUT_OF_HOST_MEMORY, 1 } }, { "VK_EXT_fake" });
    result = resolve_extension_case(list_failure);
    error  = capabilityError(result);
    ensure("a list query failure preserves result and attempt",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::EnumerationFailure &&
               error->mQuery == MaterialPipelineCapabilityQuery::DeviceExtensionProperties &&
               error->mResult == VK_ERROR_OUT_OF_HOST_MEMORY && error->mEnumerationAttempt == 1);

    FakePhysicalDevice failed_list_output;
    configureExtensions(failed_list_output, { { VK_SUCCESS, 1 }, { VK_ERROR_INITIALIZATION_FAILED, 2 } }, { "VK_EXT_fake" });
    result = resolve_extension_case(failed_list_output);
    error  = capabilityError(result);
    ensure("a failed list does not interpret its undefined returned count",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::EnumerationFailure &&
               error->mQuery == MaterialPipelineCapabilityQuery::DeviceExtensionProperties &&
               error->mResult == VK_ERROR_INITIALIZATION_FAILED && error->mEnumerationAttempt == 1 && error->mRequiredValue == 0 &&
               error->mAvailableValue == 0);

    FakePhysicalDevice invalid_output;
    configureExtensions(invalid_output, { { VK_SUCCESS, 1 }, { VK_SUCCESS, 2 } }, { "VK_EXT_fake" });
    result = resolve_extension_case(invalid_output);
    error  = capabilityError(result);
    ensure("an extension list count above capacity is rejected with exact bounds",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::InvalidEnumerationOutput &&
               error->mQuery == MaterialPipelineCapabilityQuery::DeviceExtensionProperties && error->mRequiredValue == 1 &&
               error->mAvailableValue == 2 && error->mEnumerationAttempt == 1 && error->mResult == VK_SUCCESS);
}

template<>
template<>
void render_vulkan_material_capability_test_object::test<7>()
{
    auto resolve_extension_case = [](FakePhysicalDevice& state)
    {
        auto        attachment_result = resolveAttachment(state, legacyNormSpecModernHDRPipelineKey());
        const auto* attachment        = attachmentProfile(attachment_result);
        tut::ensure("the retry fixture resolves an attachment profile", attachment != nullptr);
        resetPipelineObservations(state);
        return resolveLegacyNormSpecPipelineCapabilityProfile(capabilityDevice(state), *attachment);
    };

    FakePhysicalDevice count_retry;
    configureExtensions(count_retry, { { VK_INCOMPLETE, 0 }, { VK_SUCCESS, 0 } });
    auto result = resolve_extension_case(count_retry);
    ensure("an incomplete count transaction retries and succeeds", capabilityProfile(result) != nullptr);
    ensure_equals("count retry consumed exactly two attempts", count_retry.mExtensionCalls, std::size_t{ 2 });

    FakePhysicalDevice list_retry;
    configureExtensions(list_retry, { { VK_SUCCESS, 1 }, { VK_INCOMPLETE, 1 }, { VK_SUCCESS, 1 }, { VK_SUCCESS, 1 } }, { "VK_EXT_fake" });
    result = resolve_extension_case(list_retry);
    ensure("an incomplete list retries the complete transaction", capabilityProfile(result) != nullptr);
    ensure_equals("list retry consumed two complete transactions", list_retry.mExtensionCalls, std::size_t{ 4 });

    FakePhysicalDevice incomplete_oversized;
    configureExtensions(incomplete_oversized, { { VK_SUCCESS, 1 }, { VK_INCOMPLETE, 2 } }, { "VK_EXT_fake" });
    result            = resolve_extension_case(incomplete_oversized);
    const auto* error = capabilityError(result);
    ensure("an incomplete list still rejects a returned count above capacity",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::InvalidEnumerationOutput &&
               error->mQuery == MaterialPipelineCapabilityQuery::DeviceExtensionProperties && error->mResult == VK_INCOMPLETE &&
               error->mRequiredValue == 1 && error->mAvailableValue == 2 && error->mEnumerationAttempt == 1);

    FakePhysicalDevice exhausted;
    configureExtensions(exhausted, { { VK_INCOMPLETE, 0 }, { VK_INCOMPLETE, 0 }, { VK_INCOMPLETE, 0 } });
    result = resolve_extension_case(exhausted);
    error  = capabilityError(result);
    ensure("three incomplete attempts fail closed with exact retry context",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::EnumerationIncomplete &&
               error->mQuery == MaterialPipelineCapabilityQuery::DeviceExtensionProperties && error->mResult == VK_INCOMPLETE &&
               error->mEnumerationAttempt == 3);
    ensure_equals("the retry bound is exact", exhausted.mExtensionCalls, std::size_t{ 3 });
}

template<>
template<>
void render_vulkan_material_capability_test_object::test<8>()
{
    auto resolve_named_case = [](FakePhysicalDevice& state)
    {
        auto        attachment_result = resolveAttachment(state, legacyNormSpecCompatibilityPipelineKey());
        const auto* attachment        = attachmentProfile(attachment_result);
        tut::ensure("the extension-name fixture resolves an attachment profile", attachment != nullptr);
        resetPipelineObservations(state);
        return resolveLegacyNormSpecPipelineCapabilityProfile(capabilityDevice(state), *attachment);
    };

    FakePhysicalDevice decoy;
    configureExtensions(decoy, { { VK_SUCCESS, 1 }, { VK_SUCCESS, 1 } }, { "VK_KHR_portability_subset.extra" });
    auto        result  = resolve_named_case(decoy);
    const auto* profile = capabilityProfile(result);
    ensure("a prefix extension name is not mistaken for portability subset",
           profile && !profile->portabilitySubsetAdvertised() &&
               !profile->logicalDeviceRequirements().portabilitySubsetExtensionRequired() &&
               profile->minVertexInputBindingStrideAlignment() == 1 && decoy.mProperties2Calls == 0);

    FakePhysicalDevice exact;
    exact.mPortabilityStrideAlignment = 4;
    configureExtensions(exact, { { VK_SUCCESS, 1 }, { VK_SUCCESS, 1 } }, { "VK_KHR_portability_subset" });
    result  = resolve_named_case(exact);
    profile = capabilityProfile(result);
    ensure("the exact extension name publishes the logical-device obligation",
           profile && profile->portabilitySubsetAdvertised() && profile->logicalDeviceRequirements().portabilitySubsetExtensionRequired() &&
               profile->minVertexInputBindingStrideAlignment() == 4);
    ensure("portability properties use the exact properties2 chain and physical device",
           exact.mProperties2Calls == 1 && exact.mProperties2ChainValid &&
               exact.mLastProperties2Device == reinterpret_cast<VkPhysicalDevice>(&exact));
    ensure("portability properties are queried last",
           exact.mPipelineOrderCount == 6 && exact.mPipelineOrder[5] == MaterialPipelineCapabilityQuery::PhysicalDeviceProperties2 &&
               !exact.mOverflow);
}

template<>
template<>
void render_vulkan_material_capability_test_object::test<9>()
{
    auto resolve_alignment = [](FakePhysicalDevice& state)
    {
        configureExtensions(state, { { VK_SUCCESS, 1 }, { VK_SUCCESS, 1 } }, { "VK_KHR_portability_subset" });
        auto        attachment_result = resolveAttachment(state, legacyNormSpecModernHDRPipelineKey());
        const auto* attachment        = attachmentProfile(attachment_result);
        tut::ensure("the alignment fixture resolves an attachment profile", attachment != nullptr);
        resetPipelineObservations(state);
        return resolveLegacyNormSpecPipelineCapabilityProfile(capabilityDevice(state), *attachment);
    };

    for (const std::uint32_t alignment : std::array<std::uint32_t, 3>{ 1, 2, 4 })
    {
        FakePhysicalDevice state;
        state.mPortabilityStrideAlignment = alignment;
        const auto  result                = resolve_alignment(state);
        const auto* profile               = capabilityProfile(result);
        ensure("alignments one, two, and four accept every canonical stride",
               profile && profile->minVertexInputBindingStrideAlignment() == alignment && profile->portabilitySubsetAdvertised() &&
                   state.mProperties2ChainValid);
    }

    for (const std::uint32_t alignment : std::array<std::uint32_t, 2>{ 0, 3 })
    {
        FakePhysicalDevice state;
        state.mPortabilityStrideAlignment = alignment;
        const auto  result                = resolve_alignment(state);
        const auto* error                 = capabilityError(result);
        ensure("zero and non-power-of-two alignments are rejected",
               error && error->mCode == MaterialPipelineCapabilityResolutionCode::InvalidPortabilityAlignment &&
                   error->mQuery == MaterialPipelineCapabilityQuery::PhysicalDeviceProperties2 && !error->mVertexBinding &&
                   error->mRequiredValue == 1 && error->mAvailableValue == alignment);
    }

    FakePhysicalDevice too_wide;
    too_wide.mPortabilityStrideAlignment = 8;
    const auto  result                   = resolve_alignment(too_wide);
    const auto* error                    = capabilityError(result);
    ensure("alignment eight identifies the first incompatible four-byte color binding",
           error && error->mCode == MaterialPipelineCapabilityResolutionCode::IncompatiblePortabilityStride &&
               error->mQuery == MaterialPipelineCapabilityQuery::PhysicalDeviceProperties2 && error->mVertexBinding == 3 &&
               error->mRequiredValue == 8 && error->mAvailableValue == 4);
}

} // namespace tut
