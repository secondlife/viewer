/**
 * @file llrendervulkanmaterialpipelinecache_test.cpp
 * @brief Tests for transactional Vulkan material pipeline-cache ownership.
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

#include "llrendervulkanmaterialpipelinecache.h"
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

struct PipelineCacheCreateObservation
{
    VkDevice                     mDevice          = VK_NULL_HANDLE;
    VkStructureType              mType            = VK_STRUCTURE_TYPE_MAX_ENUM;
    const void*                  mNext            = nullptr;
    VkPipelineCacheCreateFlags   mFlags           = 0;
    std::size_t                  mInitialDataSize = 0;
    const void*                  mInitialData     = nullptr;
    const VkAllocationCallbacks* mAllocator       = nullptr;
};

struct PipelineCacheDestroyObservation
{
    VkDevice                     mDevice        = VK_NULL_HANDLE;
    VkPipelineCache              mPipelineCache = VK_NULL_HANDLE;
    const VkAllocationCallbacks* mAllocator     = nullptr;
};

struct PipelineCacheState
{
    static constexpr std::size_t MAX_CALLS = 4;

    PipelineCacheState()
    {
        for (std::size_t index = 0; index < MAX_CALLS; ++index)
        {
            mOutputs[index] = fakeHandle<VkPipelineCache>(0x7001U + index);
        }
    }

    std::array<VkResult, MAX_CALLS>                        mResults{};
    std::array<VkPipelineCache, MAX_CALLS>                 mOutputs{};
    std::array<PipelineCacheCreateObservation, MAX_CALLS>  mCreates{};
    std::array<PipelineCacheDestroyObservation, MAX_CALLS> mDestroys{};
    std::size_t                                            mCreateCount  = 0;
    std::size_t                                            mDestroyCount = 0;
};

VKAPI_ATTR VkResult VKAPI_CALL fakeCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* info,
                                                       const VkAllocationCallbacks* allocator, VkPipelineCache* output) noexcept
{
    auto* state = reinterpret_cast<PipelineCacheState*>(device);
    if (!state || !info || !output || state->mCreateCount >= PipelineCacheState::MAX_CALLS)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    const std::size_t index = state->mCreateCount++;
    state->mCreates[index]  = { device, info->sType, info->pNext, info->flags, info->initialDataSize, info->pInitialData, allocator };
    *output                 = state->mOutputs[index];
    return state->mResults[index];
}

VKAPI_ATTR void VKAPI_CALL fakeDestroyPipelineCache(VkDevice device, VkPipelineCache pipeline_cache,
                                                    const VkAllocationCallbacks* allocator) noexcept
{
    auto* state = reinterpret_cast<PipelineCacheState*>(device);
    if (!state || state->mDestroyCount >= PipelineCacheState::MAX_CALLS)
    {
        return;
    }

    state->mDestroys[state->mDestroyCount++] = { device, pipeline_cache, allocator };
}

MaterialPipelineCacheDevice fakeDevice(PipelineCacheState& state) noexcept
{
    return { reinterpret_cast<VkDevice>(&state), { fakeCreatePipelineCache, fakeDestroyPipelineCache } };
}

} // namespace

namespace tut
{

struct render_vulkan_material_pipeline_cache_test
{
};

using render_vulkan_material_pipeline_cache_test_group  = test_group<render_vulkan_material_pipeline_cache_test>;
using render_vulkan_material_pipeline_cache_test_object = render_vulkan_material_pipeline_cache_test_group::object;
render_vulkan_material_pipeline_cache_test_group render_vulkan_material_pipeline_cache_tests("render Vulkan material pipeline cache");

template<>
template<>
void render_vulkan_material_pipeline_cache_test_object::test<1>()
{
    static_assert(!std::is_copy_constructible_v<MaterialPipelineCache>);
    static_assert(!std::is_copy_assignable_v<MaterialPipelineCache>);
    static_assert(!std::is_move_constructible_v<MaterialPipelineCache>);
    static_assert(!std::is_move_assignable_v<MaterialPipelineCache>);
    static_assert(std::is_nothrow_destructible_v<MaterialPipelineCache>);
    static_assert(noexcept(createMaterialPipelineCache(std::declval<const MaterialPipelineCacheDevice&>())));
    static_assert(std::variant_size_v<MaterialPipelineCacheCreationResult> == 2);
    static_assert(std::is_same_v<std::variant_alternative_t<0, MaterialPipelineCacheCreationResult>, MaterialPipelineCacheCreationError>);
    static_assert(
        std::is_same_v<std::variant_alternative_t<1, MaterialPipelineCacheCreationResult>, std::unique_ptr<MaterialPipelineCache>>);
    static_assert(!std::is_copy_constructible_v<MaterialPipelineCacheCreationResult>);
    static_assert(!std::is_copy_assignable_v<MaterialPipelineCacheCreationResult>);
    static_assert(std::is_nothrow_move_constructible_v<MaterialPipelineCacheCreationResult>);
    static_assert(std::is_nothrow_move_assignable_v<MaterialPipelineCacheCreationResult>);
    static_assert(std::is_nothrow_destructible_v<MaterialPipelineCacheCreationResult>);

    const MaterialPipelineCacheCreationError left{ MaterialPipelineCacheCreationCode::CreateFailure, VK_ERROR_DEVICE_LOST };
    const MaterialPipelineCacheCreationError same{ MaterialPipelineCacheCreationCode::CreateFailure, VK_ERROR_DEVICE_LOST };
    const MaterialPipelineCacheCreationError different{ MaterialPipelineCacheCreationCode::NullPipelineCache, VK_SUCCESS };
    ensure("equal errors compare equal", left == same);
    ensure("different errors compare unequal", !(left == different));
}

template<>
template<>
void render_vulkan_material_pipeline_cache_test_object::test<2>()
{
    PipelineCacheState state;

    auto        result = createMaterialPipelineCache({});
    const auto* error  = std::get_if<MaterialPipelineCacheCreationError>(&result);
    ensure("a null device is rejected",
           error && error->mCode == MaterialPipelineCacheCreationCode::InvalidDevice && error->mResult == VK_SUCCESS);

    MaterialPipelineCacheDevice device    = fakeDevice(state);
    device.mDispatch.mCreatePipelineCache = nullptr;
    result                                = createMaterialPipelineCache(device);
    error                                 = std::get_if<MaterialPipelineCacheCreationError>(&result);
    ensure("a missing create callback is rejected",
           error && error->mCode == MaterialPipelineCacheCreationCode::InvalidDispatch && error->mResult == VK_SUCCESS);

    device                                 = fakeDevice(state);
    device.mDispatch.mDestroyPipelineCache = nullptr;
    result                                 = createMaterialPipelineCache(device);
    error                                  = std::get_if<MaterialPipelineCacheCreationError>(&result);
    ensure("a missing destroy callback is rejected",
           error && error->mCode == MaterialPipelineCacheCreationCode::InvalidDispatch && error->mResult == VK_SUCCESS);
    ensure("preflight failures make no native calls", state.mCreateCount == 0 && state.mDestroyCount == 0);
}

template<>
template<>
void render_vulkan_material_pipeline_cache_test_object::test<3>()
{
    PipelineCacheState          state;
    MaterialPipelineCacheDevice device = fakeDevice(state);
    PipelineCacheState          other_state;

    {
        auto  result = createMaterialPipelineCache(device);
        auto* owner  = std::get_if<std::unique_ptr<MaterialPipelineCache>>(&result);
        ensure("cold creation returns an owner", owner && *owner);
        ensure("the cache handle is a stable borrow",
               (*owner)->pipelineCache() == state.mOutputs[0] && (*owner)->pipelineCache() == (*owner)->pipelineCache());
        ensure("the creating device is retained", (*owner)->createdOn(device.mDevice) &&
                                                      !(*owner)->createdOn(reinterpret_cast<VkDevice>(&other_state)) &&
                                                      !(*owner)->createdOn(VK_NULL_HANDLE));

        const PipelineCacheCreateObservation& create = state.mCreates[0];
        ensure("the cold cache create call is exact",
               state.mCreateCount == 1 && create.mDevice == device.mDevice &&
                   create.mType == VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO && create.mNext == nullptr && create.mFlags == 0 &&
                   create.mInitialDataSize == 0 && create.mInitialData == nullptr && create.mAllocator == nullptr);
        ensure("a live owner has not destroyed its cache", state.mDestroyCount == 0);
    }

    ensure("destruction releases the exact cache once",
           state.mDestroyCount == 1 && state.mDestroys[0].mDevice == device.mDevice &&
               state.mDestroys[0].mPipelineCache == state.mOutputs[0] && state.mDestroys[0].mAllocator == nullptr);
}

template<>
template<>
void render_vulkan_material_pipeline_cache_test_object::test<4>()
{
    PipelineCacheState state;
    state.mResults[0] = VK_ERROR_OUT_OF_HOST_MEMORY;
    ensure("the poisoned failure output is non-null", state.mOutputs[0] != VK_NULL_HANDLE);

    auto        result = createMaterialPipelineCache(fakeDevice(state));
    const auto* error  = std::get_if<MaterialPipelineCacheCreationError>(&result);
    ensure("native failure is preserved",
           error && error->mCode == MaterialPipelineCacheCreationCode::CreateFailure && error->mResult == VK_ERROR_OUT_OF_HOST_MEMORY);
    ensure("an output written on failure is not owned or destroyed", state.mCreateCount == 1 && state.mDestroyCount == 0);
}

template<>
template<>
void render_vulkan_material_pipeline_cache_test_object::test<5>()
{
    PipelineCacheState state;
    state.mOutputs[0] = VK_NULL_HANDLE;

    auto        result = createMaterialPipelineCache(fakeDevice(state));
    const auto* error  = std::get_if<MaterialPipelineCacheCreationError>(&result);
    ensure("success with a null cache is rejected",
           error && error->mCode == MaterialPipelineCacheCreationCode::NullPipelineCache && error->mResult == VK_SUCCESS);
    ensure("a null cache is never destroyed", state.mCreateCount == 1 && state.mDestroyCount == 0);
}

template<>
template<>
void render_vulkan_material_pipeline_cache_test_object::test<6>()
{
    PipelineCacheState    first_state;
    PipelineCacheState    second_state;
    const VkPipelineCache shared_opaque_value = fakeHandle<VkPipelineCache>(0x7fffU);
    first_state.mOutputs[0]                   = shared_opaque_value;
    second_state.mOutputs[0]                  = shared_opaque_value;

    const MaterialPipelineCacheDevice first_device  = fakeDevice(first_state);
    const MaterialPipelineCacheDevice second_device = fakeDevice(second_state);
    auto                              first_result  = createMaterialPipelineCache(first_device);
    auto                              second_result = createMaterialPipelineCache(second_device);
    auto*                             first_owner   = std::get_if<std::unique_ptr<MaterialPipelineCache>>(&first_result);
    auto*                             second_owner  = std::get_if<std::unique_ptr<MaterialPipelineCache>>(&second_result);

    ensure("independent owners may borrow equal opaque values",
           first_owner && *first_owner && second_owner && *second_owner && (*first_owner)->pipelineCache() == shared_opaque_value &&
               (*second_owner)->pipelineCache() == shared_opaque_value && (*first_owner)->createdOn(first_device.mDevice) &&
               !(*first_owner)->createdOn(second_device.mDevice) && (*second_owner)->createdOn(second_device.mDevice) &&
               !(*second_owner)->createdOn(first_device.mDevice));

    first_owner->reset();
    ensure("destroying the first owner does not retire the second",
           first_state.mDestroyCount == 1 && second_state.mDestroyCount == 0 && (*second_owner)->pipelineCache() == shared_opaque_value);
    second_owner->reset();
    ensure("each owner destroys its device-local cache",
           second_state.mDestroyCount == 1 && first_state.mDestroys[0].mDevice == first_device.mDevice &&
               first_state.mDestroys[0].mPipelineCache == shared_opaque_value &&
               second_state.mDestroys[0].mDevice == second_device.mDevice &&
               second_state.mDestroys[0].mPipelineCache == shared_opaque_value && first_state.mDestroys[0].mAllocator == nullptr &&
               second_state.mDestroys[0].mAllocator == nullptr);
}

} // namespace tut
