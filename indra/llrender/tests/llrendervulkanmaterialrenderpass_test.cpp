/**
 * @file llrendervulkanmaterialrenderpass_test.cpp
 * @brief Tests for transactional Vulkan material render-pass ownership.
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

#include "llrendervulkanmaterialrenderpass.h"
#include "lltut.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
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

struct ProfileState
{
    ProfileState()
    {
        mFeatures.independentBlend                      = VK_TRUE;
        mProperties.limits.maxColorAttachments          = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
        mProperties.limits.maxFragmentOutputAttachments = LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT;
        for (std::size_t index = 0; index < LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT; ++index)
        {
            mFormats[index].optimalTilingFeatures =
                index < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT
                    ? VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
                    : VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
            mImages[index].maxExtent       = { 4096, 4096, 1 };
            mImages[index].maxMipLevels    = 1;
            mImages[index].maxArrayLayers  = 1;
            mImages[index].sampleCounts    = VK_SAMPLE_COUNT_1_BIT;
            mImages[index].maxResourceSize = 1ULL << 30;
        }
    }

    VkPhysicalDeviceFeatures                                                          mFeatures{};
    VkPhysicalDeviceProperties                                                        mProperties{};
    std::array<VkFormatProperties, LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT>      mFormats{};
    std::array<VkImageFormatProperties, LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT> mImages{};
    std::size_t                                                                       mFeaturesCount = 0;
    std::size_t                                                                       mFormatCount   = 0;
    std::size_t                                                                       mImageCount    = 0;
};

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceFeatures(VkPhysicalDevice physical_device, VkPhysicalDeviceFeatures* features) noexcept
{
    auto* state = reinterpret_cast<ProfileState*>(physical_device);
    if (state && features)
    {
        ++state->mFeaturesCount;
        *features = state->mFeatures;
    }
}

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceProperties(VkPhysicalDevice            physical_device,
                                                           VkPhysicalDeviceProperties* properties) noexcept
{
    auto* state = reinterpret_cast<ProfileState*>(physical_device);
    if (state && properties)
    {
        *properties = state->mProperties;
    }
}

VKAPI_ATTR void VKAPI_CALL fakeGetPhysicalDeviceFormatProperties(VkPhysicalDevice    physical_device, VkFormat,
                                                                 VkFormatProperties* properties) noexcept
{
    auto* state = reinterpret_cast<ProfileState*>(physical_device);
    if (!state || !properties || state->mFormatCount >= state->mFormats.size())
    {
        return;
    }
    *properties = state->mFormats[state->mFormatCount++];
}

VKAPI_ATTR VkResult VKAPI_CALL fakeGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physical_device, VkFormat, VkImageType,
                                                                          VkImageTiling, VkImageUsageFlags, VkImageCreateFlags,
                                                                          VkImageFormatProperties* properties) noexcept
{
    auto* state = reinterpret_cast<ProfileState*>(physical_device);
    if (!state || !properties || state->mImageCount >= state->mImages.size())
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *properties = state->mImages[state->mImageCount++];
    return VK_SUCCESS;
}

MaterialAttachmentResolutionResult resolveProfile(ProfileState& state, const LegacyNormSpecPipelineKey& key) noexcept
{
    MaterialAttachmentDevice device{ reinterpret_cast<VkPhysicalDevice>(&state),
                                     { fakeGetPhysicalDeviceFeatures, fakeGetPhysicalDeviceProperties,
                                       fakeGetPhysicalDeviceFormatProperties, fakeGetPhysicalDeviceImageFormatProperties } };
    return resolveLegacyNormSpecAttachmentProfile(device, key);
}

struct RenderPassCreateObservation
{
    VkDevice                                                                          mDevice          = VK_NULL_HANDLE;
    VkStructureType                                                                   mType            = VK_STRUCTURE_TYPE_MAX_ENUM;
    const void*                                                                       mNext            = nullptr;
    VkRenderPassCreateFlags                                                           mFlags           = 0;
    std::uint32_t                                                                     mAttachmentCount = 0;
    std::array<VkAttachmentDescription, LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT> mAttachments{};
    std::uint32_t                                                                     mSubpassCount    = 0;
    VkSubpassDescriptionFlags                                                         mSubpassFlags    = 0;
    VkPipelineBindPoint                                                               mBindPoint       = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    std::uint32_t                                                                     mInputCount      = 0;
    bool                                                                              mHasInputPointer = false;
    std::uint32_t                                                                     mColorCount      = 0;
    std::array<VkAttachmentReference, LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT>         mColorReferences{};
    bool                                                                              mHasResolvePointer = false;
    bool                                                                              mHasDepthReference = false;
    VkAttachmentReference                                                             mDepthReference{};
    std::uint32_t                                                                     mPreserveCount        = 0;
    bool                                                                              mHasPreservePointer   = false;
    std::uint32_t                                                                     mDependencyCount      = 0;
    bool                                                                              mHasDependencyPointer = false;
    const VkAllocationCallbacks*                                                      mAllocator            = nullptr;
};

struct RenderPassDestroyObservation
{
    VkDevice                     mDevice     = VK_NULL_HANDLE;
    VkRenderPass                 mRenderPass = VK_NULL_HANDLE;
    const VkAllocationCallbacks* mAllocator  = nullptr;
};

struct RenderPassState
{
    static constexpr std::size_t MAX_CALLS = 4;

    RenderPassState()
    {
        for (std::size_t index = 0; index < MAX_CALLS; ++index)
        {
            mOutputs[index] = fakeHandle<VkRenderPass>(0x3001U + index);
        }
    }

    std::array<VkResult, MAX_CALLS>                     mResults{};
    std::array<VkRenderPass, MAX_CALLS>                 mOutputs{};
    std::array<RenderPassCreateObservation, MAX_CALLS>  mCreates{};
    std::array<RenderPassDestroyObservation, MAX_CALLS> mDestroys{};
    std::size_t                                         mCreateCount  = 0;
    std::size_t                                         mDestroyCount = 0;
};

VKAPI_ATTR VkResult VKAPI_CALL fakeCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* info,
                                                    const VkAllocationCallbacks* allocator, VkRenderPass* output) noexcept
{
    auto* state = reinterpret_cast<RenderPassState*>(device);
    if (!state || !info || !output || state->mCreateCount >= RenderPassState::MAX_CALLS || info->attachmentCount > 5 ||
        (info->attachmentCount != 0 && !info->pAttachments) || info->subpassCount > 1 || (info->subpassCount != 0 && !info->pSubpasses))
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    const std::size_t index           = state->mCreateCount++;
    auto&             observation     = state->mCreates[index];
    observation.mDevice               = device;
    observation.mType                 = info->sType;
    observation.mNext                 = info->pNext;
    observation.mFlags                = info->flags;
    observation.mAttachmentCount      = info->attachmentCount;
    observation.mSubpassCount         = info->subpassCount;
    observation.mDependencyCount      = info->dependencyCount;
    observation.mHasDependencyPointer = info->pDependencies != nullptr;
    observation.mAllocator            = allocator;
    for (std::uint32_t attachment = 0; attachment < info->attachmentCount; ++attachment)
    {
        observation.mAttachments[attachment] = info->pAttachments[attachment];
    }

    if (info->subpassCount == 1)
    {
        const VkSubpassDescription& subpass = info->pSubpasses[0];
        if (subpass.colorAttachmentCount > LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT ||
            (subpass.colorAttachmentCount != 0 && !subpass.pColorAttachments))
        {
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        observation.mSubpassFlags       = subpass.flags;
        observation.mBindPoint          = subpass.pipelineBindPoint;
        observation.mInputCount         = subpass.inputAttachmentCount;
        observation.mHasInputPointer    = subpass.pInputAttachments != nullptr;
        observation.mColorCount         = subpass.colorAttachmentCount;
        observation.mHasResolvePointer  = subpass.pResolveAttachments != nullptr;
        observation.mHasDepthReference  = subpass.pDepthStencilAttachment != nullptr;
        observation.mPreserveCount      = subpass.preserveAttachmentCount;
        observation.mHasPreservePointer = subpass.pPreserveAttachments != nullptr;
        for (std::uint32_t color = 0; color < subpass.colorAttachmentCount; ++color)
        {
            observation.mColorReferences[color] = subpass.pColorAttachments[color];
        }
        if (subpass.pDepthStencilAttachment)
        {
            observation.mDepthReference = *subpass.pDepthStencilAttachment;
        }
    }

    *output = state->mOutputs[index];
    return state->mResults[index];
}

VKAPI_ATTR void VKAPI_CALL fakeDestroyRenderPass(VkDevice device, VkRenderPass render_pass, const VkAllocationCallbacks* allocator) noexcept
{
    auto* state = reinterpret_cast<RenderPassState*>(device);
    if (!state || state->mDestroyCount >= state->mDestroys.size())
    {
        return;
    }
    state->mDestroys[state->mDestroyCount++] = { device, render_pass, allocator };
}

MaterialRenderPassDevice fakeDevice(ProfileState& profile_state, RenderPassState& render_state) noexcept
{
    // This fake proves host-side create-info and ownership behavior only. It
    // cannot establish the public graphics-queue-family precondition.
    return { reinterpret_cast<VkPhysicalDevice>(&profile_state),
             reinterpret_cast<VkDevice>(&render_state),
             { fakeCreateRenderPass, fakeDestroyRenderPass } };
}

const MaterialRenderPassCreationError* creationError(const MaterialRenderPassCreationResult& result) noexcept
{
    return std::get_if<MaterialRenderPassCreationError>(&result);
}

LegacyNormSpecRenderPass* createdOwner(MaterialRenderPassCreationResult& result) noexcept
{
    auto* owner = std::get_if<std::unique_ptr<LegacyNormSpecRenderPass>>(&result);
    return owner ? owner->get() : nullptr;
}

const LegacyNormSpecAttachmentProfile* resolvedProfile(const MaterialAttachmentResolutionResult& result) noexcept
{
    return std::get_if<LegacyNormSpecAttachmentProfile>(&result);
}

void ensureCreationError(const char* message, const MaterialRenderPassCreationResult& result, MaterialRenderPassCreationCode code,
                         VkResult native_result = VK_SUCCESS)
{
    const auto* error = creationError(result);
    tut::ensure(message, error && error->mCode == code && error->mResult == native_result);
}

void ensureExactCreateInfo(const RenderPassCreateObservation& observation, const LegacyNormSpecAttachmentProfile& profile)
{
    tut::ensure("the top-level create info is exact",
                observation.mType == VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO && !observation.mNext && observation.mFlags == 0 &&
                    observation.mAttachmentCount == LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT && observation.mSubpassCount == 1 &&
                    observation.mDependencyCount == 0 && !observation.mHasDependencyPointer && !observation.mAllocator);

    for (std::size_t slot = 0; slot < LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT; ++slot)
    {
        const VkAttachmentDescription& attachment = observation.mAttachments[slot];
        tut::ensure("every color description consumes the resolved profile",
                    attachment.flags == 0 && attachment.format == profile.colors()[slot].mNativeFormat &&
                        attachment.samples == VK_SAMPLE_COUNT_1_BIT && attachment.loadOp == profile.colors()[slot].mRequiredLoadOp &&
                        attachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE && attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE &&
                        attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE &&
                        attachment.initialLayout == LegacyNormSpecRenderPass::colorAttachmentLayout() &&
                        attachment.finalLayout == LegacyNormSpecRenderPass::colorAttachmentLayout());
        tut::ensure("every color output has its ordered attachment reference",
                    observation.mColorReferences[slot].attachment == slot &&
                        observation.mColorReferences[slot].layout == LegacyNormSpecRenderPass::colorAttachmentLayout());
    }

    const VkAttachmentDescription& depth = observation.mAttachments[LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT];
    tut::ensure("the depth description consumes the resolved profile",
                depth.flags == 0 && depth.format == profile.depth().mNativeFormat && depth.samples == VK_SAMPLE_COUNT_1_BIT &&
                    depth.loadOp == profile.depth().mRequiredLoadOp && depth.storeOp == VK_ATTACHMENT_STORE_OP_STORE &&
                    depth.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE && depth.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE &&
                    depth.initialLayout == LegacyNormSpecRenderPass::depthAttachmentLayout() &&
                    depth.finalLayout == LegacyNormSpecRenderPass::depthAttachmentLayout());
    tut::ensure("one graphics subpass contains only four colors and depth",
                observation.mSubpassFlags == 0 && observation.mBindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS &&
                    observation.mInputCount == 0 && !observation.mHasInputPointer &&
                    observation.mColorCount == LEGACY_NORMSPEC_COLOR_ATTACHMENT_COUNT && !observation.mHasResolvePointer &&
                    observation.mHasDepthReference && observation.mDepthReference.attachment == 4 &&
                    observation.mDepthReference.layout == LegacyNormSpecRenderPass::depthAttachmentLayout() &&
                    observation.mPreserveCount == 0 && !observation.mHasPreservePointer);
}

} // namespace

namespace tut
{

struct render_vulkan_material_render_pass_test
{
};

using render_vulkan_material_render_pass_test_group  = test_group<render_vulkan_material_render_pass_test>;
using render_vulkan_material_render_pass_test_object = render_vulkan_material_render_pass_test_group::object;
render_vulkan_material_render_pass_test_group render_vulkan_material_render_pass_tests("render vulkan material render pass");

template<>
template<>
void render_vulkan_material_render_pass_test_object::test<1>()
{
    static_assert(LEGACY_NORMSPEC_RENDER_PASS_ATTACHMENT_COUNT == 5);
    static_assert(std::variant_size_v<MaterialRenderPassCreationResult> == 2);
    static_assert(std::is_same_v<std::variant_alternative_t<0, MaterialRenderPassCreationResult>, MaterialRenderPassCreationError>);
    static_assert(
        std::is_same_v<std::variant_alternative_t<1, MaterialRenderPassCreationResult>, std::unique_ptr<LegacyNormSpecRenderPass>>);
    static_assert(!std::is_copy_constructible_v<LegacyNormSpecRenderPass>);
    static_assert(!std::is_copy_assignable_v<LegacyNormSpecRenderPass>);
    static_assert(!std::is_move_constructible_v<LegacyNormSpecRenderPass>);
    static_assert(!std::is_move_assignable_v<LegacyNormSpecRenderPass>);
    static_assert(std::is_nothrow_destructible_v<LegacyNormSpecRenderPass>);
    static_assert(std::is_nothrow_copy_constructible_v<LegacyNormSpecAttachmentProfile>);
    static_assert(noexcept(createLegacyNormSpecRenderPass(std::declval<const MaterialRenderPassDevice&>(),
                                                          std::declval<const LegacyNormSpecAttachmentProfile&>())));
    static_assert(noexcept(std::declval<const LegacyNormSpecRenderPass&>().clearValues()));
}

template<>
template<>
void render_vulkan_material_render_pass_test_object::test<2>()
{
    ProfileState profile_state;
    auto         profile_result = resolveProfile(profile_state, legacyNormSpecModernHDRPipelineKey());
    const auto*  profile        = resolvedProfile(profile_result);
    ensure("the fixture profile resolves", profile != nullptr);

    RenderPassState          render_state;
    MaterialRenderPassDevice device = fakeDevice(profile_state, render_state);

    auto result = createLegacyNormSpecRenderPass({}, *profile);
    ensureCreationError("a null physical device is rejected", result, MaterialRenderPassCreationCode::InvalidPhysicalDevice);

    device.mDevice = VK_NULL_HANDLE;
    result         = createLegacyNormSpecRenderPass(device, *profile);
    ensureCreationError("a null logical device is rejected", result, MaterialRenderPassCreationCode::InvalidDevice);

    device                             = fakeDevice(profile_state, render_state);
    device.mDispatch.mCreateRenderPass = nullptr;
    result                             = createLegacyNormSpecRenderPass(device, *profile);
    ensureCreationError("missing create dispatch is rejected", result, MaterialRenderPassCreationCode::InvalidDispatch);

    device                              = fakeDevice(profile_state, render_state);
    device.mDispatch.mDestroyRenderPass = nullptr;
    result                              = createLegacyNormSpecRenderPass(device, *profile);
    ensureCreationError("missing destroy dispatch is rejected", result, MaterialRenderPassCreationCode::InvalidDispatch);

    device                 = fakeDevice(profile_state, render_state);
    device.mPhysicalDevice = fakeHandle<VkPhysicalDevice>(0x5eedU);
    result                 = createLegacyNormSpecRenderPass(device, *profile);
    ensureCreationError("a profile from another physical device is rejected", result,
                        MaterialRenderPassCreationCode::ProfilePhysicalDeviceMismatch);
    ensure_equals("all preflight failures happen before native creation", render_state.mCreateCount, std::size_t{ 0 });
}

template<>
template<>
void render_vulkan_material_render_pass_test_object::test<3>()
{
    ProfileState profile_state;
    auto         profile_result = resolveProfile(profile_state, legacyNormSpecModernHDRPipelineKey());
    const auto*  profile        = resolvedProfile(profile_result);
    ensure("the Modern HDR profile resolves", profile != nullptr);
    ensure_equals("the Modern HDR fixture queries physical features once", profile_state.mFeaturesCount, std::size_t{ 1 });

    RenderPassState render_state;
    auto            device = fakeDevice(profile_state, render_state);
    auto            result = createLegacyNormSpecRenderPass(device, *profile);
    auto*           owner  = createdOwner(result);
    ensure("one Modern HDR render pass is published", owner != nullptr);
    ensure_equals("native creation happens exactly once", render_state.mCreateCount, std::size_t{ 1 });
    ensureExactCreateInfo(render_state.mCreates[0], *profile);

    ensure("the owner retains both device provenances and its accepted handle",
           owner->createdOn(device.mDevice) && !owner->createdOn(fakeHandle<VkDevice>(0x5eedU)) &&
               owner->selectedFrom(device.mPhysicalDevice) && !owner->selectedFrom(fakeHandle<VkPhysicalDevice>(0x5eedU)) &&
               owner->renderPass() == render_state.mOutputs[0]);
    ensure("the owner exposes both no-transition attachment layouts",
           owner->colorAttachmentLayout() == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
               owner->depthAttachmentLayout() == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    ensure("the owner retains a distinct immutable copy of the complete profile",
           &owner->attachmentProfile() != profile && owner->attachmentProfile().targetProfile() == LegacyNormSpecTargetProfile::ModernHDR &&
               owner->attachmentProfile().deviceRequirements().independentBlendRequired() &&
               owner->attachmentProfile().colors()[3].mWriteMask ==
                   (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT) &&
               owner->attachmentProfile().colors()[3].mAlphaSemantic == MaterialAttachmentAlphaSemantic::ImplicitOneAfterClear);

    const auto clears = owner->clearValues();
    for (std::size_t slot = 0; slot < 3; ++slot)
    {
        ensure("the first three color clears remain transparent black",
               clears[slot].color.float32[0] == 0.f && clears[slot].color.float32[1] == 0.f && clears[slot].color.float32[2] == 0.f &&
                   clears[slot].color.float32[3] == 0.f);
    }
    ensure("the widened RGB target retains its alpha-one clear",
           clears[3].color.float32[0] == 0.f && clears[3].color.float32[1] == 0.f && clears[3].color.float32[2] == 0.f &&
               clears[3].color.float32[3] == 1.f);
    ensure("depth retains its one/zero clear", clears[4].depthStencil.depth == 1.f && clears[4].depthStencil.stencil == 0);

    profile_result = MaterialAttachmentResolutionError{};
    ensure("destroying the source value does not change the retained profile",
           owner->attachmentProfile().targetProfile() == LegacyNormSpecTargetProfile::ModernHDR &&
               owner->attachmentProfile().deviceRequirements().independentBlendRequired() &&
               owner->attachmentProfile().colors()[3].mNativeFormat == VK_FORMAT_R16G16B16A16_SFLOAT);

    std::get<std::unique_ptr<LegacyNormSpecRenderPass>>(result).reset();
    ensure_equals("the accepted pass is destroyed once", render_state.mDestroyCount, std::size_t{ 1 });
    ensure("destroy uses the creating device, accepted handle, and null allocator",
           render_state.mDestroys[0].mDevice == device.mDevice && render_state.mDestroys[0].mRenderPass == render_state.mOutputs[0] &&
               !render_state.mDestroys[0].mAllocator);
}

template<>
template<>
void render_vulkan_material_render_pass_test_object::test<4>()
{
    ProfileState profile_state;
    auto         profile_result = resolveProfile(profile_state, legacyNormSpecCompatibilityPipelineKey());
    const auto*  profile        = resolvedProfile(profile_result);
    ensure("the Compatibility profile resolves", profile != nullptr);
    ensure_equals("the Compatibility fixture queries physical features once", profile_state.mFeaturesCount, std::size_t{ 1 });

    RenderPassState render_state;
    auto            result = createLegacyNormSpecRenderPass(fakeDevice(profile_state, render_state), *profile);
    auto*           owner  = createdOwner(result);
    ensure("one Compatibility render pass is published", owner != nullptr);
    ensureExactCreateInfo(render_state.mCreates[0], *profile);

    const std::array<VkFormat, 4> expected{ VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32,
                                            VK_FORMAT_R8G8B8A8_UNORM };
    for (std::size_t slot = 0; slot < expected.size(); ++slot)
    {
        ensure("the Compatibility format order reaches native creation exactly",
               render_state.mCreates[0].mAttachments[slot].format == expected[slot]);
    }
    ensure("the retained Compatibility alpha contract stays coupled",
           owner->attachmentProfile().targetProfile() == LegacyNormSpecTargetProfile::Compatibility &&
               owner->attachmentProfile().deviceRequirements().independentBlendRequired() &&
               owner->attachmentProfile().colors()[3].mNativeFormat == VK_FORMAT_R8G8B8A8_UNORM &&
               owner->attachmentProfile().colors()[3].mWriteMask ==
                   (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT) &&
               owner->attachmentProfile().colors()[3].mAlphaSemantic == MaterialAttachmentAlphaSemantic::ImplicitOneAfterClear &&
               owner->clearValues()[3].color.float32[3] == 1.f);
}

template<>
template<>
void render_vulkan_material_render_pass_test_object::test<5>()
{
    ProfileState profile_state;
    auto         profile_result = resolveProfile(profile_state, legacyNormSpecModernHDRPipelineKey());
    const auto*  profile        = resolvedProfile(profile_result);
    ensure("the failure fixture profile resolves", profile != nullptr);

    RenderPassState render_state;
    render_state.mResults[0] = VK_ERROR_OUT_OF_HOST_MEMORY;
    render_state.mOutputs[0] = fakeHandle<VkRenderPass>(0xdeadU);
    auto result              = createLegacyNormSpecRenderPass(fakeDevice(profile_state, render_state), *profile);
    ensureCreationError("a failed create preserves its native result and ignores poisoned output", result,
                        MaterialRenderPassCreationCode::CreateFailure, VK_ERROR_OUT_OF_HOST_MEMORY);
    ensure_equals("failed output is never destroyed", render_state.mDestroyCount, std::size_t{ 0 });

    render_state.mResults[1] = VK_SUCCESS;
    render_state.mOutputs[1] = VK_NULL_HANDLE;
    result                   = createLegacyNormSpecRenderPass(fakeDevice(profile_state, render_state), *profile);
    ensureCreationError("success with a null render pass is rejected", result, MaterialRenderPassCreationCode::NullRenderPass);
    ensure_equals("null success is never destroyed", render_state.mDestroyCount, std::size_t{ 0 });
    ensure_equals("both native attempts were observed", render_state.mCreateCount, std::size_t{ 2 });
}

template<>
template<>
void render_vulkan_material_render_pass_test_object::test<6>()
{
    ProfileState profile_state;
    auto         profile_result = resolveProfile(profile_state, legacyNormSpecModernHDRPipelineKey());
    const auto*  profile        = resolvedProfile(profile_result);
    ensure("the repeated-handle fixture profile resolves", profile != nullptr);

    RenderPassState render_state;
    render_state.mOutputs[0] = fakeHandle<VkRenderPass>(0x4242U);
    render_state.mOutputs[1] = render_state.mOutputs[0];
    auto  device             = fakeDevice(profile_state, render_state);
    auto  first              = createLegacyNormSpecRenderPass(device, *profile);
    auto  second             = createLegacyNormSpecRenderPass(device, *profile);
    auto* first_owner        = createdOwner(first);
    auto* second_owner       = createdOwner(second);
    ensure("independent successful creates may expose equal opaque values",
           first_owner && second_owner && first_owner->renderPass() == second_owner->renderPass());

    std::get<std::unique_ptr<LegacyNormSpecRenderPass>>(first).reset();
    std::get<std::unique_ptr<LegacyNormSpecRenderPass>>(second).reset();
    ensure_equals("equal opaque values retain two destruction obligations", render_state.mDestroyCount, std::size_t{ 2 });
    ensure("both destroys retain the same accepted value and null allocator",
           render_state.mDestroys[0].mRenderPass == render_state.mOutputs[0] &&
               render_state.mDestroys[1].mRenderPass == render_state.mOutputs[1] && !render_state.mDestroys[0].mAllocator &&
               !render_state.mDestroys[1].mAllocator);
}

} // namespace tut
