/**
 * @file llrendervulkanmaterialdescriptor_test.cpp
 * @brief Tests for immutable populated Vulkan material descriptor generations.
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

#include "llmaterialcontract.h"
#include "llrendervulkanmaterialdescriptor.h"
#include "lltut.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace
{
using namespace LLRenderVulkanMaterial;

inline constexpr VkDeviceSize MATERIAL_PARAMETER_SIZE = static_cast<VkDeviceSize>(sizeof(LLRenderContract::MaterialParameters));

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

struct PoolCreateObservation
{
    VkDevice                            mDevice    = VK_NULL_HANDLE;
    VkStructureType                     mType      = VK_STRUCTURE_TYPE_MAX_ENUM;
    const void*                         mNext      = nullptr;
    VkDescriptorPoolCreateFlags         mFlags     = 0;
    std::uint32_t                       mMaxSets   = 0;
    std::uint32_t                       mSizeCount = 0;
    std::array<VkDescriptorPoolSize, 2> mSizes{};
    const VkAllocationCallbacks*        mAllocator = nullptr;
};

struct SetAllocateObservation
{
    VkDevice                              mDevice   = VK_NULL_HANDLE;
    VkStructureType                       mType     = VK_STRUCTURE_TYPE_MAX_ENUM;
    const void*                           mNext     = nullptr;
    VkDescriptorPool                      mPool     = VK_NULL_HANDLE;
    std::uint32_t                         mSetCount = 0;
    std::array<VkDescriptorSetLayout, 16> mLayouts{};
};

struct WriteObservation
{
    VkStructureType        mType            = VK_STRUCTURE_TYPE_MAX_ENUM;
    const void*            mNext            = nullptr;
    VkDescriptorSet        mSet             = VK_NULL_HANDLE;
    std::uint32_t          mBinding         = 0;
    std::uint32_t          mArrayElement    = 0;
    std::uint32_t          mDescriptorCount = 0;
    VkDescriptorType       mDescriptorType  = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    bool                   mHasImageInfo    = false;
    bool                   mHasBufferInfo   = false;
    bool                   mHasTexelView    = false;
    VkDescriptorImageInfo  mImage{};
    VkDescriptorBufferInfo mBuffer{};
};

struct UpdateObservation
{
    VkDevice                         mDevice     = VK_NULL_HANDLE;
    std::uint32_t                    mWriteCount = 0;
    std::uint32_t                    mCopyCount  = 0;
    const VkCopyDescriptorSet*       mCopies     = nullptr;
    std::array<WriteObservation, 32> mWrites{};
};

struct PoolDestroyObservation
{
    VkDevice                     mDevice    = VK_NULL_HANDLE;
    VkDescriptorPool             mPool      = VK_NULL_HANDLE;
    const VkAllocationCallbacks* mAllocator = nullptr;
};

struct FakeState
{
    static constexpr std::size_t MAX_CALLS = 8;
    static constexpr std::size_t MAX_SETS  = 16;

    FakeState()
    {
        for (std::size_t call = 0; call < MAX_CALLS; ++call)
        {
            mPoolOutputs[call] = fakeHandle<VkDescriptorPool>(0x3001U + call);
            for (std::size_t set = 0; set < MAX_SETS; ++set)
            {
                mSetOutputs[call][set] = fakeHandle<VkDescriptorSet>(0x4001U + call * MAX_SETS + set);
            }
        }
    }

    std::array<VkResult, MAX_CALLS>                              mPoolResults{};
    std::array<VkDescriptorPool, MAX_CALLS>                      mPoolOutputs{};
    std::array<PoolCreateObservation, MAX_CALLS>                 mPoolCreates{};
    std::array<VkResult, MAX_CALLS>                              mAllocateResults{};
    std::array<std::array<VkDescriptorSet, MAX_SETS>, MAX_CALLS> mSetOutputs{};
    std::array<SetAllocateObservation, MAX_CALLS>                mAllocates{};
    std::array<UpdateObservation, MAX_CALLS>                     mUpdates{};
    std::array<PoolDestroyObservation, MAX_CALLS>                mPoolDestroys{};
    std::size_t                                                  mPoolCreateCount      = 0;
    std::size_t                                                  mAllocateCount        = 0;
    std::size_t                                                  mUpdateCount          = 0;
    std::size_t                                                  mPoolDestroyCount     = 0;
    std::size_t                                                  mLayoutCreateCount    = 0;
    std::size_t                                                  mLayoutDestroyCount   = 0;
    std::size_t                                                  mPipelineCreateCount  = 0;
    std::size_t                                                  mPipelineDestroyCount = 0;
    bool                                                         mObservationOverflow  = false;
};

VKAPI_ATTR VkResult VKAPI_CALL fakeCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo*,
                                                             const VkAllocationCallbacks*, VkDescriptorSetLayout* output) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || !output)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *output = fakeHandle<VkDescriptorSetLayout>(0x1001U + state->mLayoutCreateCount++);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL fakeDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout, const VkAllocationCallbacks*) noexcept
{
    if (auto* state = reinterpret_cast<FakeState*>(device))
    {
        ++state->mLayoutDestroyCount;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL fakeCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo*, const VkAllocationCallbacks*,
                                                        VkPipelineLayout* output) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || !output)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *output = fakeHandle<VkPipelineLayout>(0x2001U + state->mPipelineCreateCount++);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL fakeDestroyPipelineLayout(VkDevice device, VkPipelineLayout, const VkAllocationCallbacks*) noexcept
{
    if (auto* state = reinterpret_cast<FakeState*>(device))
    {
        ++state->mPipelineDestroyCount;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL fakeCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* info,
                                                        const VkAllocationCallbacks* allocator, VkDescriptorPool* output) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || !info || !output || state->mPoolCreateCount >= FakeState::MAX_CALLS || info->poolSizeCount > 2 ||
        (info->poolSizeCount != 0 && !info->pPoolSizes))
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    const std::size_t index       = state->mPoolCreateCount++;
    auto&             observation = state->mPoolCreates[index];
    observation.mDevice           = device;
    observation.mType             = info->sType;
    observation.mNext             = info->pNext;
    observation.mFlags            = info->flags;
    observation.mMaxSets          = info->maxSets;
    observation.mSizeCount        = info->poolSizeCount;
    observation.mAllocator        = allocator;
    for (std::uint32_t pool_size = 0; pool_size < info->poolSizeCount; ++pool_size)
    {
        observation.mSizes[pool_size] = info->pPoolSizes[pool_size];
    }

    *output = state->mPoolOutputs[index];
    return state->mPoolResults[index];
}

VKAPI_ATTR void VKAPI_CALL fakeDestroyDescriptorPool(VkDevice device, VkDescriptorPool pool,
                                                     const VkAllocationCallbacks* allocator) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || state->mPoolDestroyCount >= FakeState::MAX_CALLS)
    {
        return;
    }
    state->mPoolDestroys[state->mPoolDestroyCount++] = { device, pool, allocator };
}

VKAPI_ATTR VkResult VKAPI_CALL fakeAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* info,
                                                          VkDescriptorSet* output) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || !info || !output || state->mAllocateCount >= FakeState::MAX_CALLS || info->descriptorSetCount > FakeState::MAX_SETS ||
        (info->descriptorSetCount != 0 && !info->pSetLayouts))
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    const std::size_t index       = state->mAllocateCount++;
    auto&             observation = state->mAllocates[index];
    observation.mDevice           = device;
    observation.mType             = info->sType;
    observation.mNext             = info->pNext;
    observation.mPool             = info->descriptorPool;
    observation.mSetCount         = info->descriptorSetCount;
    for (std::uint32_t set = 0; set < info->descriptorSetCount; ++set)
    {
        observation.mLayouts[set] = info->pSetLayouts[set];
        output[set]               = state->mSetOutputs[index][set];
    }
    return state->mAllocateResults[index];
}

VKAPI_ATTR void VKAPI_CALL fakeUpdateDescriptorSets(VkDevice device, std::uint32_t write_count, const VkWriteDescriptorSet* writes,
                                                    std::uint32_t copy_count, const VkCopyDescriptorSet* copies) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || state->mUpdateCount >= FakeState::MAX_CALLS || write_count > 32 || (write_count != 0 && !writes))
    {
        if (state)
        {
            state->mObservationOverflow = true;
        }
        return;
    }

    auto& observation       = state->mUpdates[state->mUpdateCount++];
    observation.mDevice     = device;
    observation.mWriteCount = write_count;
    observation.mCopyCount  = copy_count;
    observation.mCopies     = copies;
    for (std::uint32_t index = 0; index < write_count; ++index)
    {
        const VkWriteDescriptorSet& write    = writes[index];
        WriteObservation&           captured = observation.mWrites[index];
        captured.mType                       = write.sType;
        captured.mNext                       = write.pNext;
        captured.mSet                        = write.dstSet;
        captured.mBinding                    = write.dstBinding;
        captured.mArrayElement               = write.dstArrayElement;
        captured.mDescriptorCount            = write.descriptorCount;
        captured.mDescriptorType             = write.descriptorType;
        captured.mHasImageInfo               = write.pImageInfo != nullptr;
        captured.mHasBufferInfo              = write.pBufferInfo != nullptr;
        captured.mHasTexelView               = write.pTexelBufferView != nullptr;
        if (write.pImageInfo)
        {
            captured.mImage = *write.pImageInfo;
        }
        if (write.pBufferInfo)
        {
            captured.mBuffer = *write.pBufferInfo;
        }
    }
}

MaterialLayoutDevice fakeLayoutDevice(FakeState& state) noexcept
{
    return { reinterpret_cast<VkDevice>(&state),
             { fakeCreateDescriptorSetLayout, fakeDestroyDescriptorSetLayout, fakeCreatePipelineLayout, fakeDestroyPipelineLayout } };
}

MaterialDescriptorDevice fakeDescriptorDevice(FakeState& state) noexcept
{
    return { reinterpret_cast<VkDevice>(&state),
             { fakeCreateDescriptorPool, fakeDestroyDescriptorPool, fakeAllocateDescriptorSets, fakeUpdateDescriptorSets } };
}

std::unique_ptr<LegacyNormSpecPipelineLayout> makeLayout(FakeState& state)
{
    auto  result = createLegacyNormSpecPipelineLayout(fakeLayoutDevice(state));
    auto* owner  = std::get_if<std::unique_ptr<LegacyNormSpecPipelineLayout>>(&result);
    return owner ? std::move(*owner) : nullptr;
}

MaterialDescriptorResources makeResources(std::uintptr_t base) noexcept
{
    MaterialDescriptorResources resources;
    resources.mParameters = { fakeHandle<VkBuffer>(base), MATERIAL_PARAMETER_SIZE, 0 };
    for (std::size_t index = 0; index < resources.mSampledImages.size(); ++index)
    {
        resources.mSampledImages[index] = { fakeHandle<VkSampler>(base + 0x10U + index * 2U),
                                            fakeHandle<VkImageView>(base + 0x11U + index * 2U) };
    }
    return resources;
}

const MaterialDescriptorCreationError* creationError(const MaterialDescriptorCreationResult& result) noexcept
{
    return std::get_if<MaterialDescriptorCreationError>(&result);
}

std::unique_ptr<LegacyNormSpecDescriptorGeneration>* createdGeneration(MaterialDescriptorCreationResult& result) noexcept
{
    return std::get_if<std::unique_ptr<LegacyNormSpecDescriptorGeneration>>(&result);
}

void ensureCreationError(const char* message, const MaterialDescriptorCreationResult& result, MaterialDescriptorCreationCode code,
                         std::optional<std::size_t> tuple_index = std::nullopt, std::optional<std::uint32_t> sampled_index = std::nullopt,
                         VkResult native_result = VK_SUCCESS)
{
    const auto* error = creationError(result);
    tut::ensure(message, error && error->mCode == code && error->mTupleIndex == tuple_index && error->mSampledImageIndex == sampled_index &&
                             error->mResult == native_result);
}

} // namespace

namespace tut
{

struct render_vulkan_material_descriptor_test
{
};

using render_vulkan_material_descriptor_group  = test_group<render_vulkan_material_descriptor_test>;
using render_vulkan_material_descriptor_object = render_vulkan_material_descriptor_group::object;
render_vulkan_material_descriptor_group render_vulkan_material_descriptor_tests("render Vulkan material descriptors");

template<>
template<>
void render_vulkan_material_descriptor_object::test<1>()
{
    static_assert(!std::is_copy_constructible_v<LegacyNormSpecDescriptorGeneration>);
    static_assert(!std::is_copy_assignable_v<LegacyNormSpecDescriptorGeneration>);
    static_assert(!std::is_move_constructible_v<LegacyNormSpecDescriptorGeneration>);
    static_assert(!std::is_move_assignable_v<LegacyNormSpecDescriptorGeneration>);
    static_assert(std::is_nothrow_destructible_v<LegacyNormSpecDescriptorGeneration>);
    static_assert(noexcept(validMaterialDescriptorGenerationCount(std::size_t{})));
    static_assert(noexcept(createLegacyNormSpecDescriptorGeneration(std::declval<const MaterialDescriptorDevice&>(),
                                                                    std::declval<const LegacyNormSpecPipelineLayout&>(),
                                                                    std::declval<const std::vector<MaterialDescriptorResources>&>())));
    static_assert(std::variant_size_v<MaterialDescriptorCreationResult> == 2);
    static_assert(std::is_same_v<std::variant_alternative_t<0, MaterialDescriptorCreationResult>, MaterialDescriptorCreationError>);
    static_assert(std::is_same_v<std::variant_alternative_t<1, MaterialDescriptorCreationResult>,
                                 std::unique_ptr<LegacyNormSpecDescriptorGeneration>>);

    constexpr std::size_t max_count = std::numeric_limits<std::uint32_t>::max() / 4U;
    ensure("one tuple is a valid generation", validMaterialDescriptorGenerationCount(1));
    ensure("the largest exact write count is valid", validMaterialDescriptorGenerationCount(max_count));
    ensure("an empty generation is invalid", !validMaterialDescriptorGenerationCount(0));
    ensure("a write-count overflow is invalid", !validMaterialDescriptorGenerationCount(max_count + 1U));
}

template<>
template<>
void render_vulkan_material_descriptor_object::test<2>()
{
    FakeState state;
    auto      layout = makeLayout(state);
    ensure("the preflight fixture has canonical layouts", layout != nullptr);
    const std::vector<MaterialDescriptorResources> resources{ makeResources(0x5000U) };

    auto result = createLegacyNormSpecDescriptorGeneration({}, *layout, resources);
    ensureCreationError("a null device is rejected", result, MaterialDescriptorCreationCode::InvalidDevice);

    MaterialDescriptorDevice device        = fakeDescriptorDevice(state);
    device.mDispatch.mCreateDescriptorPool = nullptr;
    result                                 = createLegacyNormSpecDescriptorGeneration(device, *layout, resources);
    ensureCreationError("missing pool creation is rejected", result, MaterialDescriptorCreationCode::InvalidDispatch);

    device                                  = fakeDescriptorDevice(state);
    device.mDispatch.mDestroyDescriptorPool = nullptr;
    result                                  = createLegacyNormSpecDescriptorGeneration(device, *layout, resources);
    ensureCreationError("missing pool destruction is rejected", result, MaterialDescriptorCreationCode::InvalidDispatch);

    device                                   = fakeDescriptorDevice(state);
    device.mDispatch.mAllocateDescriptorSets = nullptr;
    result                                   = createLegacyNormSpecDescriptorGeneration(device, *layout, resources);
    ensureCreationError("missing set allocation is rejected", result, MaterialDescriptorCreationCode::InvalidDispatch);

    device                                 = fakeDescriptorDevice(state);
    device.mDispatch.mUpdateDescriptorSets = nullptr;
    result                                 = createLegacyNormSpecDescriptorGeneration(device, *layout, resources);
    ensureCreationError("missing descriptor update is rejected", result, MaterialDescriptorCreationCode::InvalidDispatch);

    FakeState other_state;
    result = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(other_state), *layout, resources);
    ensureCreationError("layouts from another device are rejected", result, MaterialDescriptorCreationCode::LayoutDeviceMismatch);

    result = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, {});
    ensureCreationError("an empty batch is rejected", result, MaterialDescriptorCreationCode::EmptyBatch);

    std::vector<MaterialDescriptorResources> invalid = resources;
    invalid[0].mParameters.mBuffer                   = VK_NULL_HANDLE;
    result = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, invalid);
    ensureCreationError("a null uniform buffer is rejected", result, MaterialDescriptorCreationCode::InvalidUniformBuffer, 0);

    invalid                        = resources;
    invalid[0].mParameters.mOffset = 1;
    result                         = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, invalid);
    ensureCreationError("a uniform block extending past its declared buffer is rejected", result,
                        MaterialDescriptorCreationCode::InvalidUniformRange, 0);

    invalid                      = resources;
    invalid[0].mParameters.mSize = MATERIAL_PARAMETER_SIZE - 1U;
    result                       = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, invalid);
    ensureCreationError("a buffer smaller than the canonical uniform block is rejected", result,
                        MaterialDescriptorCreationCode::InvalidUniformRange, 0);

    invalid                        = resources;
    invalid[0].mParameters.mSize   = std::numeric_limits<VkDeviceSize>::max();
    invalid[0].mParameters.mOffset = std::numeric_limits<VkDeviceSize>::max();
    result                         = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, invalid);
    ensureCreationError("uniform range arithmetic cannot wrap around", result, MaterialDescriptorCreationCode::InvalidUniformRange, 0);

    for (std::uint32_t index = 0; index < 3; ++index)
    {
        invalid                                = resources;
        invalid[0].mSampledImages[index].mView = VK_NULL_HANDLE;
        result                                 = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, invalid);
        ensureCreationError("each null sampled image is rejected", result, MaterialDescriptorCreationCode::InvalidSampledImage, 0, index);

        invalid                                   = resources;
        invalid[0].mSampledImages[index].mSampler = VK_NULL_HANDLE;
        result                                    = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, invalid);
        ensureCreationError("each null sampler is rejected", result, MaterialDescriptorCreationCode::InvalidSampler, 0, index);
    }

    invalid                               = { resources[0], resources[0] };
    invalid[1].mSampledImages[2].mSampler = VK_NULL_HANDLE;
    result                                = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, invalid);
    ensureCreationError("resource errors retain later tuple and sampled-image context", result,
                        MaterialDescriptorCreationCode::InvalidSampler, 1, 2);

    ensure_equals("preflight rejection creates no descriptor pool", state.mPoolCreateCount, std::size_t{ 0 });
    ensure_equals("preflight rejection allocates no descriptor sets", state.mAllocateCount, std::size_t{ 0 });
    ensure_equals("preflight rejection writes no descriptors", state.mUpdateCount, std::size_t{ 0 });
    ensure_equals("device mismatch creates no descriptor pool", other_state.mPoolCreateCount, std::size_t{ 0 });
}

template<>
template<>
void render_vulkan_material_descriptor_object::test<3>()
{
    FakeState state;
    auto      layout = makeLayout(state);
    ensure("the population fixture has canonical layouts", layout != nullptr);
    std::vector<MaterialDescriptorResources> resources{ makeResources(0x5000U), makeResources(0x6000U) };
    resources[1].mParameters.mSize += 64U;
    resources[1].mParameters.mOffset = 64U;

    auto  result = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, resources);
    auto* owner  = createdGeneration(result);
    ensure("two descriptor tuples are populated", owner && *owner);
    ensure_equals("one pool is created", state.mPoolCreateCount, std::size_t{ 1 });
    ensure_equals("all sets are allocated in one call", state.mAllocateCount, std::size_t{ 1 });
    ensure_equals("all descriptors are written in one call", state.mUpdateCount, std::size_t{ 1 });
    ensure("the observation buffers did not overflow", !state.mObservationOverflow);

    const PoolCreateObservation& pool = state.mPoolCreates[0];
    ensure("the immutable arena pool uses exact capacity and no individual-free flag",
           pool.mDevice == fakeDescriptorDevice(state).mDevice && pool.mType == VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO &&
               pool.mNext == nullptr && pool.mFlags == 0 && pool.mMaxSets == 4 && pool.mSizeCount == 2 && pool.mAllocator == nullptr);
    ensure("the pool has two uniform descriptors",
           pool.mSizes[0].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER && pool.mSizes[0].descriptorCount == 2);
    ensure("the pool has six combined image samplers",
           pool.mSizes[1].type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && pool.mSizes[1].descriptorCount == 6);

    const SetAllocateObservation& allocation = state.mAllocates[0];
    const auto                    layouts    = layout->descriptorSetLayouts();
    ensure("the allocation uses the new pool and four alternating layouts",
           allocation.mDevice == fakeDescriptorDevice(state).mDevice &&
               allocation.mType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO && allocation.mNext == nullptr &&
               allocation.mPool == state.mPoolOutputs[0] && allocation.mSetCount == 4 && allocation.mLayouts[0] == layouts[0] &&
               allocation.mLayouts[1] == layouts[1] && allocation.mLayouts[2] == layouts[0] && allocation.mLayouts[3] == layouts[1]);

    const UpdateObservation& update = state.mUpdates[0];
    ensure("the population call writes four descriptors per tuple and no copies",
           update.mDevice == fakeDescriptorDevice(state).mDevice && update.mWriteCount == 8 && update.mCopyCount == 0 &&
               update.mCopies == nullptr);
    for (std::size_t tuple = 0; tuple < resources.size(); ++tuple)
    {
        const WriteObservation& uniform = update.mWrites[tuple * 4];
        ensure("the uniform write targets set zero binding zero",
               uniform.mType == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET && uniform.mNext == nullptr &&
                   uniform.mSet == state.mSetOutputs[0][tuple * 2] && uniform.mBinding == 0 && uniform.mArrayElement == 0 &&
                   uniform.mDescriptorCount == 1 && uniform.mDescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
                   !uniform.mHasImageInfo && uniform.mHasBufferInfo && !uniform.mHasTexelView &&
                   uniform.mBuffer.buffer == resources[tuple].mParameters.mBuffer &&
                   uniform.mBuffer.offset == resources[tuple].mParameters.mOffset && uniform.mBuffer.range == MATERIAL_PARAMETER_SIZE);

        for (std::size_t sampled = 0; sampled < 3; ++sampled)
        {
            const WriteObservation& image = update.mWrites[tuple * 4 + sampled + 1];
            ensure("each sampled write targets its set-one binding",
                   image.mType == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET && image.mNext == nullptr &&
                       image.mSet == state.mSetOutputs[0][tuple * 2 + 1] && image.mBinding == sampled && image.mArrayElement == 0 &&
                       image.mDescriptorCount == 1 && image.mDescriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER &&
                       image.mHasImageInfo && !image.mHasBufferInfo && !image.mHasTexelView &&
                       image.mImage.sampler == resources[tuple].mSampledImages[sampled].mSampler &&
                       image.mImage.imageView == resources[tuple].mSampledImages[sampled].mView &&
                       image.mImage.imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        const auto binding = (*owner)->binding(tuple);
        ensure("the owner returns each ordered set pair and copied resource tuple",
               binding && binding->mSets.mParameters == state.mSetOutputs[0][tuple * 2] &&
                   binding->mSets.mSampledImages == state.mSetOutputs[0][tuple * 2 + 1] && binding->mResources == resources[tuple]);
    }
    ensure("an out-of-range binding is absent", !(*owner)->binding(resources.size()));
    ensure_equals("live generation retains its pool", state.mPoolDestroyCount, std::size_t{ 0 });

    std::unique_ptr<LegacyNormSpecDescriptorGeneration> first = std::move(*owner);
    std::unique_ptr<LegacyNormSpecDescriptorGeneration> final = std::move(first);
    ensure("unique ownership transfers without moving the native owner", !first && final);
    final.reset();
    ensure_equals("final release destroys the pool once", state.mPoolDestroyCount, std::size_t{ 1 });
    ensure("pool destruction uses its creating device, exact pool, and null allocator",
           state.mPoolDestroys[0].mDevice == fakeDescriptorDevice(state).mDevice && state.mPoolDestroys[0].mPool == state.mPoolOutputs[0] &&
               state.mPoolDestroys[0].mAllocator == nullptr);
}

template<>
template<>
void render_vulkan_material_descriptor_object::test<4>()
{
    FakeState state;
    auto      layout = makeLayout(state);
    ensure("the pool-failure fixture has canonical layouts", layout != nullptr);
    const std::vector<MaterialDescriptorResources> resources{ makeResources(0x5000U) };

    state.mPoolResults[0] = VK_ERROR_OUT_OF_HOST_MEMORY;
    state.mPoolOutputs[0] = fakeHandle<VkDescriptorPool>(0xdeadU);
    auto result           = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, resources);
    ensureCreationError("pool failure preserves its native result", result, MaterialDescriptorCreationCode::PoolCreateFailure, std::nullopt,
                        std::nullopt, VK_ERROR_OUT_OF_HOST_MEMORY);
    ensure_equals("a poisoned failure output is not owned", state.mPoolDestroyCount, std::size_t{ 0 });
    ensure_equals("pool failure allocates no sets", state.mAllocateCount, std::size_t{ 0 });
    ensure_equals("pool failure writes no descriptors", state.mUpdateCount, std::size_t{ 0 });

    FakeState null_state;
    auto      null_layout = makeLayout(null_state);
    ensure("the null-pool fixture has canonical layouts", null_layout != nullptr);
    null_state.mPoolOutputs[0] = VK_NULL_HANDLE;
    result                     = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(null_state), *null_layout, resources);
    ensureCreationError("success with a null pool fails closed", result, MaterialDescriptorCreationCode::NullPool);
    ensure_equals("a null pool has no destruction obligation", null_state.mPoolDestroyCount, std::size_t{ 0 });
    ensure_equals("a null pool allocates no sets", null_state.mAllocateCount, std::size_t{ 0 });
}

template<>
template<>
void render_vulkan_material_descriptor_object::test<5>()
{
    FakeState state;
    auto      layout = makeLayout(state);
    ensure("the allocation-failure fixture has canonical layouts", layout != nullptr);
    const std::vector<MaterialDescriptorResources> resources{ makeResources(0x5000U), makeResources(0x6000U) };
    state.mAllocateResults[0] = VK_ERROR_OUT_OF_DEVICE_MEMORY;
    state.mSetOutputs[0][0]   = fakeHandle<VkDescriptorSet>(0xdeadU);

    auto result = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, resources);
    ensureCreationError("set allocation failure preserves its native result", result, MaterialDescriptorCreationCode::SetAllocationFailure,
                        std::nullopt, std::nullopt, VK_ERROR_OUT_OF_DEVICE_MEMORY);
    ensure_equals("set allocation is attempted once", state.mAllocateCount, std::size_t{ 1 });
    ensure_equals("allocation failure writes no descriptors", state.mUpdateCount, std::size_t{ 0 });
    ensure_equals("allocation failure rolls back the pool once", state.mPoolDestroyCount, std::size_t{ 1 });
    ensure("rollback destroys the successful pool, not poisoned set outputs", state.mPoolDestroys[0].mPool == state.mPoolOutputs[0]);
}

template<>
template<>
void render_vulkan_material_descriptor_object::test<6>()
{
    const std::array<std::size_t, 3> null_positions{ 0, 2, 3 };
    for (std::size_t null_position : null_positions)
    {
        FakeState state;
        auto      layout = makeLayout(state);
        ensure("the null-set fixture has canonical layouts", layout != nullptr);
        const std::vector<MaterialDescriptorResources> resources{ makeResources(0x5000U), makeResources(0x6000U) };
        state.mSetOutputs[0][null_position] = VK_NULL_HANDLE;

        auto       result        = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, resources);
        const bool parameter_set = (null_position % 2U) == 0;
        ensureCreationError("a null allocated set fails closed", result,
                            parameter_set ? MaterialDescriptorCreationCode::NullParameterSet
                                          : MaterialDescriptorCreationCode::NullSampledImageSet,
                            null_position / 2U);
        ensure_equals("a null allocated set prevents all descriptor writes", state.mUpdateCount, std::size_t{ 0 });
        ensure_equals("a null allocated set rolls back the whole pool", state.mPoolDestroyCount, std::size_t{ 1 });
    }
}

template<>
template<>
void render_vulkan_material_descriptor_object::test<7>()
{
    FakeState state;
    auto      layout = makeLayout(state);
    ensure("the opaque-alias fixture has canonical layouts", layout != nullptr);
    const MaterialDescriptorResources              shared = makeResources(0x5000U);
    const std::vector<MaterialDescriptorResources> resources{ shared, shared };
    const VkDescriptorSet                          repeated_set = fakeHandle<VkDescriptorSet>(0x7777U);
    state.mSetOutputs[0].fill(repeated_set);

    auto  result = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, resources);
    auto* owner  = createdGeneration(result);
    ensure("equal non-dispatchable set and resource values are accepted", owner && *owner);
    ensure_equals("equal values still populate every descriptor", state.mUpdates[0].mWriteCount, std::uint32_t{ 8 });
    for (std::size_t tuple = 0; tuple < resources.size(); ++tuple)
    {
        const auto binding = (*owner)->binding(tuple);
        ensure("each logical binding retains the repeated opaque values",
               binding && binding->mSets.mParameters == repeated_set && binding->mSets.mSampledImages == repeated_set &&
                   binding->mResources == shared);
    }
    owner->reset();
    ensure_equals("equal set values do not alter pool ownership", state.mPoolDestroyCount, std::size_t{ 1 });
}

template<>
template<>
void render_vulkan_material_descriptor_object::test<8>()
{
    FakeState state;
    auto      layout = makeLayout(state);
    ensure("the copied-metadata fixture has canonical layouts", layout != nullptr);
    std::vector<MaterialDescriptorResources> resources{ makeResources(0x5000U) };
    const MaterialDescriptorResources        expected = resources[0];

    auto  result = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, resources);
    auto* owner  = createdGeneration(result);
    ensure("the descriptor generation is created", owner && *owner);
    resources[0] = {};

    const auto binding = (*owner)->binding(0);
    ensure("borrowed native handles are copied as stable metadata",
           binding && binding->mResources == expected && binding->mSets.mParameters == state.mSetOutputs[0][0] &&
               binding->mSets.mSampledImages == state.mSetOutputs[0][1]);
}

template<>
template<>
void render_vulkan_material_descriptor_object::test<9>()
{
    FakeState state;
    auto      layout = makeLayout(state);
    ensure("the independent-generation fixture has canonical layouts", layout != nullptr);
    const std::vector<MaterialDescriptorResources> resources{ makeResources(0x5000U) };
    state.mPoolOutputs[1] = state.mPoolOutputs[0];
    state.mSetOutputs[1]  = state.mSetOutputs[0];

    auto  first_result  = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, resources);
    auto  second_result = createLegacyNormSpecDescriptorGeneration(fakeDescriptorDevice(state), *layout, resources);
    auto* first_ptr     = createdGeneration(first_result);
    auto* second_ptr    = createdGeneration(second_result);
    ensure("repeated opaque values still produce independent owners",
           first_ptr && *first_ptr && second_ptr && *second_ptr && first_ptr->get() != second_ptr->get());
    ensure_equals("the factory has no hidden pool cache", state.mPoolCreateCount, std::size_t{ 2 });
    ensure_equals("each generation allocates its own logical set batch", state.mAllocateCount, std::size_t{ 2 });
    ensure_equals("each generation populates its descriptors", state.mUpdateCount, std::size_t{ 2 });

    second_ptr->reset();
    first_ptr->reset();
    ensure_equals("both successful pool creates have a destruction obligation", state.mPoolDestroyCount, std::size_t{ 2 });
    ensure("the repeated pool value is destroyed once for each successful create",
           state.mPoolDestroys[0].mPool == state.mPoolOutputs[0] && state.mPoolDestroys[1].mPool == state.mPoolOutputs[0]);
}

} // namespace tut
