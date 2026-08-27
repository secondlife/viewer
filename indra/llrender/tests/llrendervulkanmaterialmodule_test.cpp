/**
 * @file llrendervulkanmaterialmodule_test.cpp
 * @brief Tests for transactional Vulkan material shader modules.
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

#include "llrendervulkanmaterialmodule.h"
#include "lltut.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace
{
using namespace LLRenderContract;
using namespace LLRenderVulkanMaterial;

constexpr std::uint32_t SPIRV_MAGIC = 0x07230203U;

std::vector<std::uint32_t> module(ShaderStage stage, std::uint32_t generator)
{
    // These minimal #version 450 modules were compiled for vulkan1.1 and pass
    // spirv-val. They establish a valid fake-dispatch fixture, not packaged
    // production-byte provenance or the material interface.
    std::vector<std::uint32_t> words;
    if (stage == ShaderStage::Vertex)
    {
        words = { 0x07230203U, 0x00010300U, 0x0008000bU, 0x00000006U, 0x00000000U, 0x00020011U, 0x00000001U, 0x0006000bU, 0x00000001U,
                  0x4c534c47U, 0x6474732eU, 0x3035342eU, 0x00000000U, 0x0003000eU, 0x00000000U, 0x00000001U, 0x0005000fU, 0x00000000U,
                  0x00000004U, 0x6e69616dU, 0x00000000U, 0x00030003U, 0x00000002U, 0x000001c2U, 0x00040005U, 0x00000004U, 0x6e69616dU,
                  0x00000000U, 0x00020013U, 0x00000002U, 0x00030021U, 0x00000003U, 0x00000002U, 0x00050036U, 0x00000002U, 0x00000004U,
                  0x00000000U, 0x00000003U, 0x000200f8U, 0x00000005U, 0x000100fdU, 0x00010038U };
    }
    else
    {
        words = { 0x07230203U, 0x00010300U, 0x0008000bU, 0x00000006U, 0x00000000U, 0x00020011U, 0x00000001U, 0x0006000bU, 0x00000001U,
                  0x4c534c47U, 0x6474732eU, 0x3035342eU, 0x00000000U, 0x0003000eU, 0x00000000U, 0x00000001U, 0x0005000fU, 0x00000004U,
                  0x00000004U, 0x6e69616dU, 0x00000000U, 0x00030010U, 0x00000004U, 0x00000007U, 0x00030003U, 0x00000002U, 0x000001c2U,
                  0x00040005U, 0x00000004U, 0x6e69616dU, 0x00000000U, 0x00020013U, 0x00000002U, 0x00030021U, 0x00000003U, 0x00000002U,
                  0x00050036U, 0x00000002U, 0x00000004U, 0x00000000U, 0x00000003U, 0x000200f8U, 0x00000005U, 0x000100fdU, 0x00010038U };
    }
    words[2] = generator;
    return words;
}

LoadedShaderProgram structurallyAcceptedProgram(std::uint32_t generator = 1)
{
    return { legacyNormSpecModernHDRPipelineKey().mProgram,
             { ShaderStage::Vertex, "main", module(ShaderStage::Vertex, generator) },
             { ShaderStage::Fragment, "main", module(ShaderStage::Fragment, generator) } };
}

ShaderGenerationLease acceptedLease(std::uint64_t frame = 1)
{
    return { { 1, 1 }, frame, std::make_shared<const LoadedShaderProgram>(structurallyAcceptedProgram()) };
}

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

enum class EventKind : std::uint8_t
{
    Create,
    Destroy
};

struct Event
{
    EventKind                    mKind      = EventKind::Create;
    VkDevice                     mDevice    = VK_NULL_HANDLE;
    VkShaderModule               mModule    = VK_NULL_HANDLE;
    const VkAllocationCallbacks* mAllocator = nullptr;
};

struct CreateObservation
{
    VkDevice                     mDevice    = VK_NULL_HANDLE;
    VkStructureType              mType      = VK_STRUCTURE_TYPE_MAX_ENUM;
    const void*                  mNext      = nullptr;
    VkShaderModuleCreateFlags    mFlags     = 0;
    std::size_t                  mCodeSize  = 0;
    const std::uint32_t*         mCode      = nullptr;
    std::uint32_t                mFirstWord = 0;
    const VkAllocationCallbacks* mAllocator = nullptr;
};

struct FakeState
{
    static constexpr std::size_t MAX_CALLS = 4;

    FakeState()
    {
        for (std::size_t index = 0; index < MAX_CALLS; ++index)
        {
            mOutputs[index] = fakeHandle<VkShaderModule>(0x1001U + index);
        }
    }

    std::array<VkResult, MAX_CALLS>          mResults{ VK_SUCCESS, VK_SUCCESS, VK_SUCCESS, VK_SUCCESS };
    std::array<VkShaderModule, MAX_CALLS>    mOutputs{};
    std::array<CreateObservation, MAX_CALLS> mCreates{};
    std::array<Event, MAX_CALLS * 2>         mEvents{};
    std::size_t                              mCreateCount  = 0;
    std::size_t                              mDestroyCount = 0;
    std::size_t                              mEventCount   = 0;
};

VKAPI_ATTR VkResult VKAPI_CALL fakeCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* info,
                                                      const VkAllocationCallbacks* allocator, VkShaderModule* output) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || !info || !output || state->mCreateCount >= FakeState::MAX_CALLS)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    const std::size_t index              = state->mCreateCount++;
    state->mCreates[index]               = { device,
                                             info->sType,
                                             info->pNext,
                                             info->flags,
                                             info->codeSize,
                                             info->pCode,
                               info->pCode && info->codeSize >= sizeof(std::uint32_t) ? info->pCode[0] : 0U,
                                             allocator };
    *output                              = state->mOutputs[index];
    state->mEvents[state->mEventCount++] = { EventKind::Create, device, *output, allocator };
    return state->mResults[index];
}

VKAPI_ATTR void VKAPI_CALL fakeDestroyShaderModule(VkDevice device, VkShaderModule module, const VkAllocationCallbacks* allocator) noexcept
{
    auto* state = reinterpret_cast<FakeState*>(device);
    if (!state || state->mEventCount >= state->mEvents.size())
    {
        return;
    }
    ++state->mDestroyCount;
    state->mEvents[state->mEventCount++] = { EventKind::Destroy, device, module, allocator };
}

ShaderModuleDevice fakeDevice(FakeState& state) noexcept
{
    return { reinterpret_cast<VkDevice>(&state), { fakeCreateShaderModule, fakeDestroyShaderModule } };
}

} // namespace

namespace tut
{

struct render_vulkan_material_module_test
{
};

using render_vulkan_material_module_test_group  = test_group<render_vulkan_material_module_test>;
using render_vulkan_material_module_test_object = render_vulkan_material_module_test_group::object;
render_vulkan_material_module_test_group render_vulkan_material_module_tests("render vulkan material module");

template<>
template<>
void render_vulkan_material_module_test_object::test<1>()
{
    static_assert(!std::is_copy_constructible_v<ShaderModuleGeneration>);
    static_assert(!std::is_copy_assignable_v<ShaderModuleGeneration>);
    static_assert(!std::is_move_constructible_v<ShaderModuleGeneration>);
    static_assert(!std::is_move_assignable_v<ShaderModuleGeneration>);
    static_assert(std::is_nothrow_destructible_v<ShaderModuleGeneration>);
    static_assert(noexcept(
        createLegacyNormSpecShaderModules(std::declval<const ShaderModuleDevice&>(), std::declval<const ShaderGenerationLease&>())));
    static_assert(std::variant_size_v<ShaderModuleCreationResult> == 2);
    static_assert(std::is_same_v<std::variant_alternative_t<0, ShaderModuleCreationResult>, ShaderModuleCreationError>);
    static_assert(std::is_same_v<std::variant_alternative_t<1, ShaderModuleCreationResult>, std::unique_ptr<ShaderModuleGeneration>>);
}

template<>
template<>
void render_vulkan_material_module_test_object::test<2>()
{
    FakeState                   state;
    const ShaderGenerationLease lease = acceptedLease();

    auto        result = createLegacyNormSpecShaderModules({}, lease);
    const auto* error  = std::get_if<ShaderModuleCreationError>(&result);
    ensure("a null device is rejected", error && error->mCode == ShaderModuleCreationCode::InvalidDevice);

    ShaderModuleDevice device            = fakeDevice(state);
    device.mDispatch.mCreateShaderModule = nullptr;
    result                               = createLegacyNormSpecShaderModules(device, lease);
    error                                = std::get_if<ShaderModuleCreationError>(&result);
    ensure("a missing create dispatch is rejected", error && error->mCode == ShaderModuleCreationCode::InvalidDispatch);

    device                                = fakeDevice(state);
    device.mDispatch.mDestroyShaderModule = nullptr;
    result                                = createLegacyNormSpecShaderModules(device, lease);
    error                                 = std::get_if<ShaderModuleCreationError>(&result);
    ensure("a missing destroy dispatch is rejected before creation", error && error->mCode == ShaderModuleCreationCode::InvalidDispatch);
    ensure_equals("device and dispatch rejection makes no native call", state.mEventCount, std::size_t{ 0 });
}

template<>
template<>
void render_vulkan_material_module_test_object::test<3>()
{
    FakeState                   state;
    const ShaderModuleDevice    device = fakeDevice(state);
    const ShaderGenerationLease valid  = acceptedLease();

    auto expect_invalid = [&](ShaderGenerationLease lease, const char* message)
    {
        auto        result = createLegacyNormSpecShaderModules(device, lease);
        const auto* error  = std::get_if<ShaderModuleCreationError>(&result);
        ensure(message, error && error->mCode == ShaderModuleCreationCode::InvalidLease && !error->mStage && error->mResult == VK_SUCCESS);
    };

    ShaderGenerationLease lease = valid;
    lease.mProgram.reset();
    expect_invalid(lease, "a null program is rejected");
    lease         = valid;
    lease.mHandle = {};
    expect_invalid(lease, "a zero handle is rejected");
    lease         = valid;
    lease.mHandle = { 2, 1 };
    expect_invalid(lease, "a noncanonical publication index is rejected");
    lease        = valid;
    lease.mFrame = 0;
    expect_invalid(lease, "a zero frame is rejected");

    LoadedShaderProgram malformed = *valid.mProgram;
    malformed.mProgram.mName      = "other.material";
    lease                         = valid;
    lease.mProgram                = std::make_shared<const LoadedShaderProgram>(malformed);
    expect_invalid(lease, "a wrong program is rejected");
    malformed                = *valid.mProgram;
    malformed.mVertex.mStage = ShaderStage::Fragment;
    lease.mProgram           = std::make_shared<const LoadedShaderProgram>(malformed);
    expect_invalid(lease, "a swapped stage is rejected");
    malformed                       = *valid.mProgram;
    malformed.mFragment.mEntryPoint = "other";
    lease.mProgram                  = std::make_shared<const LoadedShaderProgram>(malformed);
    expect_invalid(lease, "a wrong entry point is rejected");
    malformed                   = *valid.mProgram;
    malformed.mVertex.mWords[0] = 0;
    lease.mProgram              = std::make_shared<const LoadedShaderProgram>(malformed);
    expect_invalid(lease, "malformed SPIR-V is rejected");

    ensure_equals("malformed leases make no native call", state.mEventCount, std::size_t{ 0 });
}

template<>
template<>
void render_vulkan_material_module_test_object::test<4>()
{
    FakeState                   state;
    const ShaderModuleDevice    device     = fakeDevice(state);
    const ShaderGenerationLease lease      = acceptedLease(17);
    auto                        result     = createLegacyNormSpecShaderModules(device, lease);
    auto*                       generation = std::get_if<std::unique_ptr<ShaderModuleGeneration>>(&result);
    ensure("a canonical lease creates one owned module pair", generation && *generation);

    ensure_equals("the pair creates exactly two modules", state.mCreateCount, std::size_t{ 2 });
    ensure_equals("creation does not destroy live modules", state.mDestroyCount, std::size_t{ 0 });
    for (std::size_t index = 0; index < 2; ++index)
    {
        const CreateObservation& observed = state.mCreates[index];
        ensure("the creating device is exact", observed.mDevice == device.mDevice);
        ensure("shader create info has the exact type", observed.mType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
        ensure("shader create info has no extension chain", observed.mNext == nullptr);
        ensure_equals("shader create flags are zero", observed.mFlags, VkShaderModuleCreateFlags{ 0 });
        ensure("allocation callbacks are null", observed.mAllocator == nullptr);
        ensure_equals("the SPIR-V magic is forwarded", observed.mFirstWord, SPIRV_MAGIC);
    }
    ensure_equals("vertex byte count is exact", state.mCreates[0].mCodeSize, lease.mProgram->mVertex.mWords.size() * sizeof(std::uint32_t));
    ensure("the retained vertex word pointer is exact", state.mCreates[0].mCode == lease.mProgram->mVertex.mWords.data());
    ensure_equals("fragment byte count is exact", state.mCreates[1].mCodeSize,
                  lease.mProgram->mFragment.mWords.size() * sizeof(std::uint32_t));
    ensure("the retained fragment word pointer is exact", state.mCreates[1].mCode == lease.mProgram->mFragment.mWords.data());
    ensure("the owner preserves the logical handle", (*generation)->handle() == lease.mHandle);
    ensure("the owner retains the exact immutable program", &(*generation)->program() == lease.mProgram.get());
    ensure("the owner exposes the exact vertex module", (*generation)->vertexModule() == state.mOutputs[0]);
    ensure("the owner exposes the exact fragment module", (*generation)->fragmentModule() == state.mOutputs[1]);
    ensure("native creation is vertex then fragment",
           state.mEvents[0].mKind == EventKind::Create && state.mEvents[0].mModule == state.mOutputs[0] &&
               state.mEvents[1].mKind == EventKind::Create && state.mEvents[1].mModule == state.mOutputs[1]);

    std::unique_ptr<ShaderModuleGeneration> first       = std::move(*generation);
    std::unique_ptr<ShaderModuleGeneration> transferred = std::move(first);
    ensure("unique_ptr transfer moves ownership without moving the native owner", !first && transferred);
    transferred.reset();
    ensure_equals("final release destroys exactly two modules", state.mDestroyCount, std::size_t{ 2 });
    ensure("release destroys fragment then vertex",
           state.mEvents[2].mKind == EventKind::Destroy && state.mEvents[2].mModule == state.mOutputs[1] &&
               state.mEvents[3].mKind == EventKind::Destroy && state.mEvents[3].mModule == state.mOutputs[0]);
    ensure("destruction uses the creating device and null allocator",
           state.mEvents[2].mDevice == device.mDevice && state.mEvents[3].mDevice == device.mDevice &&
               state.mEvents[2].mAllocator == nullptr && state.mEvents[3].mAllocator == nullptr);
}

template<>
template<>
void render_vulkan_material_module_test_object::test<5>()
{
    FakeState                                state;
    std::weak_ptr<const LoadedShaderProgram> storage;
    std::unique_ptr<ShaderModuleGeneration>  generation;
    LegacyNormSpecShaderPublication          publication;
    const auto                               published = publication.publish(structurallyAcceptedProgram(71));
    ensure("the retention fixture publishes", published.has_value());
    auto lease = publication.resolveForFrame(*published, 9);
    ensure("the retention fixture resolves", lease.has_value());
    storage = lease->mProgram;

    auto  result     = createLegacyNormSpecShaderModules(fakeDevice(state), *lease);
    auto* result_ptr = std::get_if<std::unique_ptr<ShaderModuleGeneration>>(&result);
    ensure("the retention fixture creates native ownership", result_ptr && *result_ptr);
    generation = std::move(*result_ptr);
    lease.reset();
    ensure("the retention fixture publishes a replacement", publication.publish(structurallyAcceptedProgram(72)).has_value());
    const auto retired = publication.completeThrough(9);
    ensure("the source generation retires from publication ownership",
           retired && retired->size() == 1 && retired->front().mHandle == *published);

    ensure("the native owner retains logically retired publication storage", generation && !storage.expired());
    ensure_equals("retained source remains readable", generation->program().mVertex.mWords[2], std::uint32_t{ 71 });
    generation.reset();
    ensure("source storage releases with the native owner", storage.expired());
}

template<>
template<>
void render_vulkan_material_module_test_object::test<6>()
{
    FakeState state;
    state.mResults[0] = VK_ERROR_OUT_OF_HOST_MEMORY;
    state.mOutputs[0] = fakeHandle<VkShaderModule>(0x2001U);

    auto        result = createLegacyNormSpecShaderModules(fakeDevice(state), acceptedLease());
    const auto* error  = std::get_if<ShaderModuleCreationError>(&result);
    ensure("a vertex failure preserves its stage and result",
           error && error->mCode == ShaderModuleCreationCode::CreateFailure && error->mStage == ShaderStage::Vertex &&
               error->mResult == VK_ERROR_OUT_OF_HOST_MEMORY);
    ensure_equals("a vertex failure stops after one create", state.mCreateCount, std::size_t{ 1 });
    ensure_equals("a handle written with a failed result is not treated as owned", state.mDestroyCount, std::size_t{ 0 });

    FakeState null_state;
    null_state.mOutputs[0] = VK_NULL_HANDLE;
    result                 = createLegacyNormSpecShaderModules(fakeDevice(null_state), acceptedLease());
    error                  = std::get_if<ShaderModuleCreationError>(&result);
    ensure("success with a null vertex fails closed",
           error && error->mCode == ShaderModuleCreationCode::NullModule && error->mStage == ShaderStage::Vertex &&
               error->mResult == VK_SUCCESS);
    ensure_equals("a null vertex has nothing to destroy", null_state.mDestroyCount, std::size_t{ 0 });
}

template<>
template<>
void render_vulkan_material_module_test_object::test<7>()
{
    FakeState state;
    state.mResults[1] = VK_ERROR_OUT_OF_DEVICE_MEMORY;
    state.mOutputs[1] = state.mOutputs[0];

    auto        result = createLegacyNormSpecShaderModules(fakeDevice(state), acceptedLease());
    const auto* error  = std::get_if<ShaderModuleCreationError>(&result);
    ensure("a fragment failure preserves its stage and result",
           error && error->mCode == ShaderModuleCreationCode::CreateFailure && error->mStage == ShaderStage::Fragment &&
               error->mResult == VK_ERROR_OUT_OF_DEVICE_MEMORY);
    ensure_equals("a fragment failure attempts both stages", state.mCreateCount, std::size_t{ 2 });
    ensure_equals("fragment failure rolls back only the owned vertex", state.mDestroyCount, std::size_t{ 1 });
    ensure("fragment failure rollback follows both create attempts",
           state.mEvents[2].mKind == EventKind::Destroy && state.mEvents[2].mModule == state.mOutputs[0]);

    FakeState null_state;
    null_state.mOutputs[1] = VK_NULL_HANDLE;
    result                 = createLegacyNormSpecShaderModules(fakeDevice(null_state), acceptedLease());
    error                  = std::get_if<ShaderModuleCreationError>(&result);
    ensure("success with a null fragment fails closed",
           error && error->mCode == ShaderModuleCreationCode::NullModule && error->mStage == ShaderStage::Fragment);
    ensure_equals("a null fragment rolls back the vertex", null_state.mDestroyCount, std::size_t{ 1 });
    ensure("null fragment rollback destroys the exact vertex", null_state.mEvents[2].mModule == null_state.mOutputs[0]);
}

template<>
template<>
void render_vulkan_material_module_test_object::test<8>()
{
    FakeState state;
    state.mOutputs[1] = state.mOutputs[0];

    auto  result     = createLegacyNormSpecShaderModules(fakeDevice(state), acceptedLease());
    auto* generation = std::get_if<std::unique_ptr<ShaderModuleGeneration>>(&result);
    ensure("equal non-dispatchable handle values still represent two successful creates", generation && *generation);
    ensure("the owner retains both equal handle values",
           (*generation)->vertexModule() == state.mOutputs[0] && (*generation)->fragmentModule() == state.mOutputs[0]);
    generation->reset();
    ensure_equals("each successful create has one destruction obligation", state.mDestroyCount, std::size_t{ 2 });
    ensure("equal handles are destroyed in reverse creation order",
           state.mEvents[2].mModule == state.mOutputs[0] && state.mEvents[3].mModule == state.mOutputs[0]);
}

template<>
template<>
void render_vulkan_material_module_test_object::test<9>()
{
    FakeState                   state;
    const ShaderModuleDevice    device = fakeDevice(state);
    const ShaderGenerationLease lease  = acceptedLease(23);

    auto  first_result  = createLegacyNormSpecShaderModules(device, lease);
    auto  second_result = createLegacyNormSpecShaderModules(device, lease);
    auto* first_ptr     = std::get_if<std::unique_ptr<ShaderModuleGeneration>>(&first_result);
    auto* second_ptr    = std::get_if<std::unique_ptr<ShaderModuleGeneration>>(&second_result);
    ensure("both independent creation attempts succeed", first_ptr && *first_ptr && second_ptr && *second_ptr);
    auto first  = std::move(*first_ptr);
    auto second = std::move(*second_ptr);
    ensure("the same valid lease creates independent owners", first && second && first.get() != second.get());
    ensure_equals("the factory has no hidden generation cache", state.mCreateCount, std::size_t{ 4 });
    ensure("the pairs own four distinct native handles",
           first->vertexModule() != second->vertexModule() && first->fragmentModule() != second->fragmentModule());

    second.reset();
    first.reset();
    ensure_equals("independent owners release all four handles", state.mDestroyCount, std::size_t{ 4 });
}

} // namespace tut
