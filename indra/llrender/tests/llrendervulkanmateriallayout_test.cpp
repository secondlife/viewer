/**
 * @file llrendervulkanmateriallayout_test.cpp
 * @brief Tests for transactional Vulkan material descriptor and pipeline layouts.
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

#include "llrendervulkanmateriallayout.h"
#include "llshadermanifest.h"
#include "lltut.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
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

VkShaderStageFlags vulkanStages(const ShaderStageVisibility& visibility) noexcept
{
    VkShaderStageFlags flags = 0;
    if (visibility.mVertex)
    {
        flags |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (visibility.mFragment)
    {
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    return flags;
}

enum class EventKind : std::uint8_t
{
    CreateDescriptorSetLayout,
    CreatePipelineLayout,
    DestroyPipelineLayout,
    DestroyDescriptorSetLayout
};

struct Event
{
    EventKind                    mKind             = EventKind::CreateDescriptorSetLayout;
    VkDevice                     mDevice           = VK_NULL_HANDLE;
    VkDescriptorSetLayout        mDescriptorLayout = VK_NULL_HANDLE;
    VkPipelineLayout             mPipelineLayout   = VK_NULL_HANDLE;
    const VkAllocationCallbacks* mAllocator        = nullptr;
};

struct DescriptorCreateObservation
{
    VkDevice                                    mDevice = VK_NULL_HANDLE;
    VkStructureType                             mType   = VK_STRUCTURE_TYPE_MAX_ENUM;
    const void*                                 mNext   = nullptr;
    VkDescriptorSetLayoutCreateFlags            mFlags  = 0;
    std::array<VkDescriptorSetLayoutBinding, 3> mBindings{};
    std::array<bool, 3>                         mHasImmutableSamplers{};
    std::uint32_t                               mBindingCount = 0;
    const VkAllocationCallbacks*                mAllocator    = nullptr;
};

struct PipelineCreateObservation
{
    VkDevice                             mDevice = VK_NULL_HANDLE;
    VkStructureType                      mType   = VK_STRUCTURE_TYPE_MAX_ENUM;
    const void*                          mNext   = nullptr;
    VkPipelineLayoutCreateFlags          mFlags  = 0;
    std::array<VkDescriptorSetLayout, 2> mSetLayouts{};
    std::uint32_t                        mSetLayoutCount         = 0;
    std::uint32_t                        mPushConstantRangeCount = 0;
    const VkPushConstantRange*           mPushConstantRanges     = nullptr;
    const VkAllocationCallbacks*         mAllocator              = nullptr;
};

struct FakeState
{
    static constexpr std::size_t MAX_DESCRIPTOR_CALLS = 8;
    static constexpr std::size_t MAX_PIPELINE_CALLS   = 4;
    static constexpr std::size_t MAX_EVENTS           = 24;

    FakeState()
    {
        for (std::size_t index = 0; index < MAX_DESCRIPTOR_CALLS; ++index)
        {
            mDescriptorOutputs[index] = fakeHandle<VkDescriptorSetLayout>(0x1001U + index);
        }
        for (std::size_t index = 0; index < MAX_PIPELINE_CALLS; ++index)
        {
            mPipelineOutputs[index] = fakeHandle<VkPipelineLayout>(0x2001U + index);
        }
    }

    std::array<VkResult, MAX_DESCRIPTOR_CALLS>                    mDescriptorResults{};
    std::array<VkDescriptorSetLayout, MAX_DESCRIPTOR_CALLS>       mDescriptorOutputs{};
    std::array<DescriptorCreateObservation, MAX_DESCRIPTOR_CALLS> mDescriptorCreates{};
    std::array<VkResult, MAX_PIPELINE_CALLS>                      mPipelineResults{};
    std::array<VkPipelineLayout, MAX_PIPELINE_CALLS>              mPipelineOutputs{};
    std::array<PipelineCreateObservation, MAX_PIPELINE_CALLS>     mPipelineCreates{};
    std::array<Event, MAX_EVENTS>                                 mEvents{};
    std::size_t                                                   mDescriptorCreateCount  = 0;
    std::size_t                                                   mDescriptorDestroyCount = 0;
    std::size_t                                                   mPipelineCreateCount    = 0;
    std::size_t                                                   mPipelineDestroyCount   = 0;
    std::size_t                                                   mEventCount             = 0;
};

VKAPI_ATTR VkResult VKAPI_CALL fakeCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* info,
                                                             const VkAllocationCallbacks* allocator, VkDescriptorSetLayout* output) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || !info || !output || state->mDescriptorCreateCount >= FakeState::MAX_DESCRIPTOR_CALLS || info->bindingCount > 3 ||
        (info->bindingCount != 0 && !info->pBindings) || state->mEventCount >= FakeState::MAX_EVENTS)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    const std::size_t index       = state->mDescriptorCreateCount++;
    auto&             observation = state->mDescriptorCreates[index];
    observation.mDevice           = device;
    observation.mType             = info->sType;
    observation.mNext             = info->pNext;
    observation.mFlags            = info->flags;
    observation.mBindingCount     = info->bindingCount;
    observation.mAllocator        = allocator;
    for (std::uint32_t binding = 0; binding < info->bindingCount; ++binding)
    {
        observation.mBindings[binding]                    = info->pBindings[binding];
        observation.mHasImmutableSamplers[binding]        = info->pBindings[binding].pImmutableSamplers != nullptr;
        observation.mBindings[binding].pImmutableSamplers = nullptr;
    }

    *output                              = state->mDescriptorOutputs[index];
    state->mEvents[state->mEventCount++] = { EventKind::CreateDescriptorSetLayout, device, *output, VK_NULL_HANDLE, allocator };
    return state->mDescriptorResults[index];
}

VKAPI_ATTR void VKAPI_CALL fakeDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout,
                                                          const VkAllocationCallbacks* allocator) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || state->mEventCount >= FakeState::MAX_EVENTS)
    {
        return;
    }
    ++state->mDescriptorDestroyCount;
    state->mEvents[state->mEventCount++] = { EventKind::DestroyDescriptorSetLayout, device, layout, VK_NULL_HANDLE, allocator };
}

VKAPI_ATTR VkResult VKAPI_CALL fakeCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* info,
                                                        const VkAllocationCallbacks* allocator, VkPipelineLayout* output) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || !info || !output || state->mPipelineCreateCount >= FakeState::MAX_PIPELINE_CALLS || info->setLayoutCount > 2 ||
        (info->setLayoutCount != 0 && !info->pSetLayouts) || state->mEventCount >= FakeState::MAX_EVENTS)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    const std::size_t index             = state->mPipelineCreateCount++;
    auto&             observation       = state->mPipelineCreates[index];
    observation.mDevice                 = device;
    observation.mType                   = info->sType;
    observation.mNext                   = info->pNext;
    observation.mFlags                  = info->flags;
    observation.mSetLayoutCount         = info->setLayoutCount;
    observation.mPushConstantRangeCount = info->pushConstantRangeCount;
    observation.mPushConstantRanges     = info->pPushConstantRanges;
    observation.mAllocator              = allocator;
    for (std::uint32_t set = 0; set < info->setLayoutCount; ++set)
    {
        observation.mSetLayouts[set] = info->pSetLayouts[set];
    }

    *output                              = state->mPipelineOutputs[index];
    state->mEvents[state->mEventCount++] = { EventKind::CreatePipelineLayout, device, VK_NULL_HANDLE, *output, allocator };
    return state->mPipelineResults[index];
}

VKAPI_ATTR void VKAPI_CALL fakeDestroyPipelineLayout(VkDevice device, VkPipelineLayout layout,
                                                     const VkAllocationCallbacks* allocator) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || state->mEventCount >= FakeState::MAX_EVENTS)
    {
        return;
    }
    ++state->mPipelineDestroyCount;
    state->mEvents[state->mEventCount++] = { EventKind::DestroyPipelineLayout, device, VK_NULL_HANDLE, layout, allocator };
}

MaterialLayoutDevice fakeDevice(FakeState& state) noexcept
{
    return { reinterpret_cast<VkDevice>(&state),
             { fakeCreateDescriptorSetLayout, fakeDestroyDescriptorSetLayout, fakeCreatePipelineLayout, fakeDestroyPipelineLayout } };
}

const MaterialLayoutCreationError* creationError(const MaterialLayoutCreationResult& result) noexcept
{
    return std::get_if<MaterialLayoutCreationError>(&result);
}

std::unique_ptr<LegacyNormSpecPipelineLayout>* createdLayout(MaterialLayoutCreationResult& result) noexcept
{
    return std::get_if<std::unique_ptr<LegacyNormSpecPipelineLayout>>(&result);
}

void ensureCreationError(const char* message, const MaterialLayoutCreationResult& result, MaterialLayoutCreationCode code,
                         std::optional<MaterialLayoutObject> object, VkResult native_result)
{
    const auto* error = creationError(result);
    tut::ensure(message, error && error->mCode == code && error->mObject == object && error->mResult == native_result);
}

} // namespace

namespace tut
{

struct render_vulkan_material_layout_test
{
};

using render_vulkan_material_layout_test_group  = test_group<render_vulkan_material_layout_test>;
using render_vulkan_material_layout_test_object = render_vulkan_material_layout_test_group::object;
render_vulkan_material_layout_test_group render_vulkan_material_layout_tests("render vulkan material layout");

template<>
template<>
void render_vulkan_material_layout_test_object::test<1>()
{
    static_assert(!std::is_copy_constructible_v<LegacyNormSpecPipelineLayout>);
    static_assert(!std::is_copy_assignable_v<LegacyNormSpecPipelineLayout>);
    static_assert(!std::is_move_constructible_v<LegacyNormSpecPipelineLayout>);
    static_assert(!std::is_move_assignable_v<LegacyNormSpecPipelineLayout>);
    static_assert(std::is_nothrow_destructible_v<LegacyNormSpecPipelineLayout>);
    static_assert(noexcept(createLegacyNormSpecPipelineLayout(std::declval<const MaterialLayoutDevice&>())));
    static_assert(std::variant_size_v<MaterialLayoutCreationResult> == 2);
    static_assert(std::is_same_v<std::variant_alternative_t<0, MaterialLayoutCreationResult>, MaterialLayoutCreationError>);
    static_assert(
        std::is_same_v<std::variant_alternative_t<1, MaterialLayoutCreationResult>, std::unique_ptr<LegacyNormSpecPipelineLayout>>);
}

template<>
template<>
void render_vulkan_material_layout_test_object::test<2>()
{
    FakeState state;

    auto result = createLegacyNormSpecPipelineLayout({});
    ensureCreationError("a null device is rejected", result, MaterialLayoutCreationCode::InvalidDevice, std::nullopt, VK_SUCCESS);

    MaterialLayoutDevice device                 = fakeDevice(state);
    device.mDispatch.mCreateDescriptorSetLayout = nullptr;
    result                                      = createLegacyNormSpecPipelineLayout(device);
    ensureCreationError("missing descriptor creation is rejected", result, MaterialLayoutCreationCode::InvalidDispatch, std::nullopt,
                        VK_SUCCESS);

    device                                       = fakeDevice(state);
    device.mDispatch.mDestroyDescriptorSetLayout = nullptr;
    result                                       = createLegacyNormSpecPipelineLayout(device);
    ensureCreationError("missing descriptor destruction is rejected", result, MaterialLayoutCreationCode::InvalidDispatch, std::nullopt,
                        VK_SUCCESS);

    device                                 = fakeDevice(state);
    device.mDispatch.mCreatePipelineLayout = nullptr;
    result                                 = createLegacyNormSpecPipelineLayout(device);
    ensureCreationError("missing pipeline creation is rejected", result, MaterialLayoutCreationCode::InvalidDispatch, std::nullopt,
                        VK_SUCCESS);

    device                                  = fakeDevice(state);
    device.mDispatch.mDestroyPipelineLayout = nullptr;
    result                                  = createLegacyNormSpecPipelineLayout(device);
    ensureCreationError("missing pipeline destruction is rejected", result, MaterialLayoutCreationCode::InvalidDispatch, std::nullopt,
                        VK_SUCCESS);

    ensure_equals("preflight rejection makes no native call", state.mEventCount, std::size_t{ 0 });
}

template<>
template<>
void render_vulkan_material_layout_test_object::test<3>()
{
    FakeState                  state;
    const MaterialLayoutDevice device = fakeDevice(state);
    auto                       result = createLegacyNormSpecPipelineLayout(device);
    auto*                      owner  = createdLayout(result);
    ensure("the canonical layouts are created", owner && *owner);

    ensure_equals("two descriptor layouts are created", state.mDescriptorCreateCount, std::size_t{ 2 });
    ensure_equals("one pipeline layout is created", state.mPipelineCreateCount, std::size_t{ 1 });
    ensure_equals("creation does not destroy live layouts", state.mDescriptorDestroyCount + state.mPipelineDestroyCount, std::size_t{ 0 });

    const auto manifest = legacyNormSpecShaderManifest(legacyNormSpecModernHDRPipelineKey(), ShaderBackend::Vulkan);
    ensure("the canonical production manifest resolves", manifest && validLegacyNormSpecProductionShaderManifest(*manifest));
    ensure("the production manifest has one parameter block and three sampled images",
           manifest->mParameterBlock && manifest->mSampledImages.size() == 3 && manifest->mPushConstantRanges.empty());

    const DescriptorCreateObservation& parameters = state.mDescriptorCreates[0];
    ensure("parameter set create info is canonical",
           parameters.mDevice == device.mDevice && parameters.mType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO &&
               parameters.mNext == nullptr && parameters.mFlags == 0 && parameters.mBindingCount == 1 && parameters.mAllocator == nullptr);
    const VkDescriptorSetLayoutBinding& parameter_binding = parameters.mBindings[0];
    ensure("set zero follows the production parameter block",
           manifest->mParameterBlock->mSet == 0 && manifest->mParameterBlock->mBinding == 0 &&
               manifest->mParameterBlock->mVisibility == ShaderStageVisibility{ true, true } && parameter_binding.binding == 0 &&
               parameter_binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER && parameter_binding.descriptorCount == 1 &&
               parameter_binding.stageFlags == (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) &&
               parameter_binding.stageFlags == vulkanStages(manifest->mParameterBlock->mVisibility) &&
               !parameters.mHasImmutableSamplers[0]);

    const DescriptorCreateObservation& images = state.mDescriptorCreates[1];
    ensure("sampled-image set create info is canonical",
           images.mDevice == device.mDevice && images.mType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO &&
               images.mNext == nullptr && images.mFlags == 0 && images.mBindingCount == manifest->mSampledImages.size() &&
               images.mAllocator == nullptr);
    for (std::size_t index = 0; index < manifest->mSampledImages.size(); ++index)
    {
        const ShaderSampledImage&           expected = manifest->mSampledImages[index];
        const VkDescriptorSetLayoutBinding& observed = images.mBindings[index];
        ensure("sampled images are the three fragment-visible bindings in set one",
               expected.mSet == 1 && expected.mBinding == index && expected.mVisibility == ShaderStageVisibility{ false, true });
        ensure("each sampled binding follows the production manifest",
               observed.binding == expected.mBinding && observed.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER &&
                   observed.descriptorCount == 1 && observed.stageFlags == VK_SHADER_STAGE_FRAGMENT_BIT &&
                   observed.stageFlags == vulkanStages(expected.mVisibility) && !images.mHasImmutableSamplers[index]);
    }

    const PipelineCreateObservation& pipeline = state.mPipelineCreates[0];
    const auto                       exposed  = (*owner)->descriptorSetLayouts();
    ensure("pipeline layout create info is canonical",
           pipeline.mDevice == device.mDevice && pipeline.mType == VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO &&
               pipeline.mNext == nullptr && pipeline.mFlags == 0 && pipeline.mSetLayoutCount == 2 &&
               pipeline.mPushConstantRangeCount == 0 && pipeline.mPushConstantRanges == nullptr && pipeline.mAllocator == nullptr);
    ensure("descriptor set order is parameter then sampled image",
           pipeline.mSetLayouts[0] == state.mDescriptorOutputs[0] && pipeline.mSetLayouts[1] == state.mDescriptorOutputs[1] &&
               exposed[0] == state.mDescriptorOutputs[0] && exposed[1] == state.mDescriptorOutputs[1]);
    ensure("the owner exposes exact native handles",
           (*owner)->parameterSetLayout() == state.mDescriptorOutputs[0] &&
               (*owner)->sampledImageSetLayout() == state.mDescriptorOutputs[1] && (*owner)->pipelineLayout() == state.mPipelineOutputs[0]);
    ensure("the owner reports only its creating device",
           (*owner)->createdOn(device.mDevice) && !(*owner)->createdOn(fakeHandle<VkDevice>(0x5eedU)));
    ensure("creation order is parameter set, sampled-image set, then pipeline",
           state.mEvents[0].mKind == EventKind::CreateDescriptorSetLayout &&
               state.mEvents[0].mDescriptorLayout == state.mDescriptorOutputs[0] &&
               state.mEvents[1].mKind == EventKind::CreateDescriptorSetLayout &&
               state.mEvents[1].mDescriptorLayout == state.mDescriptorOutputs[1] &&
               state.mEvents[2].mKind == EventKind::CreatePipelineLayout && state.mEvents[2].mPipelineLayout == state.mPipelineOutputs[0]);
}

template<>
template<>
void render_vulkan_material_layout_test_object::test<4>()
{
    FakeState state;
    state.mDescriptorResults[0] = VK_ERROR_OUT_OF_HOST_MEMORY;
    state.mDescriptorOutputs[0] = fakeHandle<VkDescriptorSetLayout>(0x3001U);
    auto result                 = createLegacyNormSpecPipelineLayout(fakeDevice(state));
    ensureCreationError("parameter layout failure preserves its object and result", result, MaterialLayoutCreationCode::CreateFailure,
                        MaterialLayoutObject::ParameterSetLayout, VK_ERROR_OUT_OF_HOST_MEMORY);
    ensure_equals("parameter failure stops after one call", state.mDescriptorCreateCount, std::size_t{ 1 });
    ensure_equals("a poisoned failure output is not owned", state.mDescriptorDestroyCount, std::size_t{ 0 });

    FakeState null_state;
    null_state.mDescriptorOutputs[0] = VK_NULL_HANDLE;
    result                           = createLegacyNormSpecPipelineLayout(fakeDevice(null_state));
    ensureCreationError("success with a null parameter layout fails closed", result, MaterialLayoutCreationCode::NullHandle,
                        MaterialLayoutObject::ParameterSetLayout, VK_SUCCESS);
    ensure_equals("a null parameter layout has nothing to roll back", null_state.mDescriptorDestroyCount, std::size_t{ 0 });
}

template<>
template<>
void render_vulkan_material_layout_test_object::test<5>()
{
    FakeState state;
    state.mDescriptorResults[1] = VK_ERROR_OUT_OF_DEVICE_MEMORY;
    state.mDescriptorOutputs[1] = state.mDescriptorOutputs[0];
    auto result                 = createLegacyNormSpecPipelineLayout(fakeDevice(state));
    ensureCreationError("sampled layout failure preserves its object and result", result, MaterialLayoutCreationCode::CreateFailure,
                        MaterialLayoutObject::SampledImageSetLayout, VK_ERROR_OUT_OF_DEVICE_MEMORY);
    ensure_equals("sampled failure attempts both descriptor layouts", state.mDescriptorCreateCount, std::size_t{ 2 });
    ensure_equals("sampled failure rolls back only the parameter layout", state.mDescriptorDestroyCount, std::size_t{ 1 });
    ensure("a poisoned sampled output alias does not duplicate the real rollback obligation",
           state.mEvents[2].mKind == EventKind::DestroyDescriptorSetLayout &&
               state.mEvents[2].mDescriptorLayout == state.mDescriptorOutputs[0] && state.mDescriptorDestroyCount == 1);

    FakeState null_state;
    null_state.mDescriptorOutputs[1] = VK_NULL_HANDLE;
    result                           = createLegacyNormSpecPipelineLayout(fakeDevice(null_state));
    ensureCreationError("success with a null sampled layout fails closed", result, MaterialLayoutCreationCode::NullHandle,
                        MaterialLayoutObject::SampledImageSetLayout, VK_SUCCESS);
    ensure_equals("a null sampled layout rolls back the parameter layout", null_state.mDescriptorDestroyCount, std::size_t{ 1 });
    ensure("null sampled rollback destroys the exact parameter layout",
           null_state.mEvents[2].mDescriptorLayout == null_state.mDescriptorOutputs[0]);
}

template<>
template<>
void render_vulkan_material_layout_test_object::test<6>()
{
    FakeState state;
    state.mPipelineResults[0] = VK_ERROR_OUT_OF_HOST_MEMORY;
    state.mPipelineOutputs[0] = fakeHandle<VkPipelineLayout>(0x4001U);
    auto result               = createLegacyNormSpecPipelineLayout(fakeDevice(state));
    ensureCreationError("pipeline layout failure preserves its object and result", result, MaterialLayoutCreationCode::CreateFailure,
                        MaterialLayoutObject::PipelineLayout, VK_ERROR_OUT_OF_HOST_MEMORY);
    ensure_equals("pipeline failure is attempted once", state.mPipelineCreateCount, std::size_t{ 1 });
    ensure_equals("a poisoned pipeline output is not owned", state.mPipelineDestroyCount, std::size_t{ 0 });
    ensure_equals("pipeline failure rolls back both descriptor layouts", state.mDescriptorDestroyCount, std::size_t{ 2 });
    ensure("pipeline failure rollback is sampled then parameter",
           state.mEvents[3].mKind == EventKind::DestroyDescriptorSetLayout &&
               state.mEvents[3].mDescriptorLayout == state.mDescriptorOutputs[1] &&
               state.mEvents[4].mKind == EventKind::DestroyDescriptorSetLayout &&
               state.mEvents[4].mDescriptorLayout == state.mDescriptorOutputs[0]);

    FakeState null_state;
    null_state.mPipelineOutputs[0] = VK_NULL_HANDLE;
    result                         = createLegacyNormSpecPipelineLayout(fakeDevice(null_state));
    ensureCreationError("success with a null pipeline layout fails closed", result, MaterialLayoutCreationCode::NullHandle,
                        MaterialLayoutObject::PipelineLayout, VK_SUCCESS);
    ensure_equals("a null pipeline layout is not destroyed", null_state.mPipelineDestroyCount, std::size_t{ 0 });
    ensure_equals("a null pipeline layout rolls back both descriptor layouts", null_state.mDescriptorDestroyCount, std::size_t{ 2 });
}

template<>
template<>
void render_vulkan_material_layout_test_object::test<7>()
{
    FakeState state;
    state.mDescriptorOutputs[1] = state.mDescriptorOutputs[0];

    auto  result = createLegacyNormSpecPipelineLayout(fakeDevice(state));
    auto* owner  = createdLayout(result);
    ensure("equal non-dispatchable descriptor values still represent two successful creates", owner && *owner);
    ensure("the owner retains both equal descriptor values",
           (*owner)->parameterSetLayout() == state.mDescriptorOutputs[0] &&
               (*owner)->sampledImageSetLayout() == state.mDescriptorOutputs[0]);
    owner->reset();
    ensure_equals("each successful descriptor create has one destruction obligation", state.mDescriptorDestroyCount, std::size_t{ 2 });
    ensure("both equal handles are destroyed in reverse creation order",
           state.mEvents[4].mDescriptorLayout == state.mDescriptorOutputs[0] &&
               state.mEvents[5].mDescriptorLayout == state.mDescriptorOutputs[0]);
}

template<>
template<>
void render_vulkan_material_layout_test_object::test<8>()
{
    FakeState                  state;
    const MaterialLayoutDevice device = fakeDevice(state);
    auto                       result = createLegacyNormSpecPipelineLayout(device);
    auto*                      owner  = createdLayout(result);
    ensure("the ownership fixture succeeds", owner && *owner);

    std::unique_ptr<LegacyNormSpecPipelineLayout> first       = std::move(*owner);
    std::unique_ptr<LegacyNormSpecPipelineLayout> transferred = std::move(first);
    ensure("unique_ptr transfer changes the holder without moving the native owner", !first && transferred);
    transferred.reset();
    ensure_equals("final release destroys the pipeline layout once", state.mPipelineDestroyCount, std::size_t{ 1 });
    ensure_equals("final release destroys both descriptor layouts", state.mDescriptorDestroyCount, std::size_t{ 2 });
    ensure("final destruction is pipeline, sampled set, parameter set",
           state.mEvents[3].mKind == EventKind::DestroyPipelineLayout && state.mEvents[3].mPipelineLayout == state.mPipelineOutputs[0] &&
               state.mEvents[4].mKind == EventKind::DestroyDescriptorSetLayout &&
               state.mEvents[4].mDescriptorLayout == state.mDescriptorOutputs[1] &&
               state.mEvents[5].mKind == EventKind::DestroyDescriptorSetLayout &&
               state.mEvents[5].mDescriptorLayout == state.mDescriptorOutputs[0]);
    ensure("destruction uses the creating device and null allocation callbacks",
           state.mEvents[3].mDevice == device.mDevice && state.mEvents[4].mDevice == device.mDevice &&
               state.mEvents[5].mDevice == device.mDevice && state.mEvents[3].mAllocator == nullptr &&
               state.mEvents[4].mAllocator == nullptr && state.mEvents[5].mAllocator == nullptr);
}

template<>
template<>
void render_vulkan_material_layout_test_object::test<9>()
{
    FakeState state;
    state.mDescriptorOutputs[2] = state.mDescriptorOutputs[0];
    state.mDescriptorOutputs[3] = state.mDescriptorOutputs[1];
    state.mPipelineOutputs[1]   = state.mPipelineOutputs[0];

    const MaterialLayoutDevice device        = fakeDevice(state);
    auto                       first_result  = createLegacyNormSpecPipelineLayout(device);
    auto                       second_result = createLegacyNormSpecPipelineLayout(device);
    auto*                      first_ptr     = createdLayout(first_result);
    auto*                      second_ptr    = createdLayout(second_result);
    ensure("both independent layout attempts succeed", first_ptr && *first_ptr && second_ptr && *second_ptr);

    auto first  = std::move(*first_ptr);
    auto second = std::move(*second_ptr);
    ensure("independent attempts produce independent owners even when opaque values repeat", first.get() != second.get());
    ensure_equals("the factory has no hidden descriptor cache", state.mDescriptorCreateCount, std::size_t{ 4 });
    ensure_equals("the factory has no hidden pipeline cache", state.mPipelineCreateCount, std::size_t{ 2 });

    second.reset();
    first.reset();
    ensure_equals("independent owners release both pipelines", state.mPipelineDestroyCount, std::size_t{ 2 });
    ensure_equals("independent owners release all descriptor layouts", state.mDescriptorDestroyCount, std::size_t{ 4 });
}

} // namespace tut
