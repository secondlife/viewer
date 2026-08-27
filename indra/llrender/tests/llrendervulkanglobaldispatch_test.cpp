/**
 * @file llrendervulkanglobaldispatch_test.cpp
 * @brief Tests for loader-independent Vulkan global command dispatch.
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

#include "llrendervulkanglobaldispatch.h"
#include "lltut.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace
{
using namespace LLRenderVulkan;

enum class MissingCommand : std::uint8_t
{
    None,
    CreateInstance,
    EnumerateInstanceExtensionProperties,
    EnumerateInstanceLayerProperties,
    EnumerateInstanceVersion
};

struct GlobalDispatchState
{
    static constexpr std::size_t MAX_LOOKUPS = 8;

    MissingCommand                       mMissing       = MissingCommand::None;
    VkResult                             mVersionResult = VK_SUCCESS;
    std::uint32_t                        mVersionOutput = VK_API_VERSION_1_2;
    std::array<VkInstance, MAX_LOOKUPS>  mLookupInstances{};
    std::array<const char*, MAX_LOOKUPS> mLookupNames{};
    std::size_t                          mLookupCount  = 0;
    std::size_t                          mVersionCalls = 0;
};

GlobalDispatchState* gGlobalDispatchState = nullptr;

struct ScopedGlobalDispatchState
{
    explicit ScopedGlobalDispatchState(GlobalDispatchState& state) noexcept { gGlobalDispatchState = &state; }
    ~ScopedGlobalDispatchState() noexcept { gGlobalDispatchState = nullptr; }

    ScopedGlobalDispatchState(const ScopedGlobalDispatchState&)            = delete;
    ScopedGlobalDispatchState& operator=(const ScopedGlobalDispatchState&) = delete;
};

VKAPI_ATTR VkResult VKAPI_CALL fakeCreateInstance(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*) noexcept
{
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL fakeEnumerateInstanceExtensionProperties(const char*, std::uint32_t*, VkExtensionProperties*) noexcept
{
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL fakeEnumerateInstanceLayerProperties(std::uint32_t*, VkLayerProperties*) noexcept
{
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL fakeEnumerateInstanceVersion(std::uint32_t* api_version) noexcept
{
    if (!gGlobalDispatchState || !api_version)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    ++gGlobalDispatchState->mVersionCalls;
    *api_version = gGlobalDispatchState->mVersionOutput;
    return gGlobalDispatchState->mVersionResult;
}

template<typename Function>
PFN_vkVoidFunction eraseFunctionType(Function function) noexcept
{
    return reinterpret_cast<PFN_vkVoidFunction>(function);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL fakeGetInstanceProcAddr(VkInstance instance, const char* name) noexcept
{
    if (!gGlobalDispatchState || !name || gGlobalDispatchState->mLookupCount >= GlobalDispatchState::MAX_LOOKUPS)
    {
        return nullptr;
    }

    const std::size_t index                       = gGlobalDispatchState->mLookupCount++;
    gGlobalDispatchState->mLookupInstances[index] = instance;
    gGlobalDispatchState->mLookupNames[index]     = name;

    if (std::strcmp(name, "vkCreateInstance") == 0)
    {
        return gGlobalDispatchState->mMissing == MissingCommand::CreateInstance ? nullptr : eraseFunctionType(fakeCreateInstance);
    }
    if (std::strcmp(name, "vkEnumerateInstanceExtensionProperties") == 0)
    {
        return gGlobalDispatchState->mMissing == MissingCommand::EnumerateInstanceExtensionProperties
                   ? nullptr
                   : eraseFunctionType(fakeEnumerateInstanceExtensionProperties);
    }
    if (std::strcmp(name, "vkEnumerateInstanceLayerProperties") == 0)
    {
        return gGlobalDispatchState->mMissing == MissingCommand::EnumerateInstanceLayerProperties
                   ? nullptr
                   : eraseFunctionType(fakeEnumerateInstanceLayerProperties);
    }
    if (std::strcmp(name, "vkEnumerateInstanceVersion") == 0)
    {
        return gGlobalDispatchState->mMissing == MissingCommand::EnumerateInstanceVersion ? nullptr
                                                                                          : eraseFunctionType(fakeEnumerateInstanceVersion);
    }
    return nullptr;
}

const VulkanGlobalDispatchResolutionError& requireError(const VulkanGlobalDispatchResolutionResult& result)
{
    const auto* error = std::get_if<VulkanGlobalDispatchResolutionError>(&result);
    tut::ensure("resolution returns an error", error != nullptr);
    return *error;
}

void ensureLookup(const GlobalDispatchState& state, std::size_t index, std::string_view expected_name)
{
    tut::ensure("the expected lookup was made", index < state.mLookupCount);
    tut::ensure("the lookup used a null instance", state.mLookupInstances[index] == VK_NULL_HANDLE);
    tut::ensure("the lookup used the exact command name", std::string_view(state.mLookupNames[index]) == expected_name);
}

} // namespace

namespace tut
{

struct render_vulkan_global_dispatch_test
{
};

using render_vulkan_global_dispatch_test_group  = test_group<render_vulkan_global_dispatch_test>;
using render_vulkan_global_dispatch_test_object = render_vulkan_global_dispatch_test_group::object;
render_vulkan_global_dispatch_test_group render_vulkan_global_dispatch_tests("render Vulkan global dispatch");

template<>
template<>
void render_vulkan_global_dispatch_test_object::test<1>()
{
    static_assert(RENDERER_VULKAN_API_VERSION == VK_API_VERSION_1_1);
    static_assert(!std::is_default_constructible_v<VulkanGlobalDispatchGeneration>);
    static_assert(std::is_copy_constructible_v<VulkanGlobalDispatchGeneration>);
    static_assert(std::is_nothrow_copy_constructible_v<VulkanGlobalDispatchGeneration>);
    static_assert(std::is_move_constructible_v<VulkanGlobalDispatchGeneration>);
    static_assert(std::is_nothrow_move_constructible_v<VulkanGlobalDispatchGeneration>);
    static_assert(!std::is_copy_assignable_v<VulkanGlobalDispatchGeneration>);
    static_assert(!std::is_move_assignable_v<VulkanGlobalDispatchGeneration>);
    static_assert(std::is_nothrow_destructible_v<VulkanGlobalDispatchGeneration>);
    static_assert(noexcept(resolveVulkanGlobalDispatchGeneration(std::declval<PFN_vkGetInstanceProcAddr>())));
    static_assert(std::variant_size_v<VulkanGlobalDispatchResolutionResult> == 2);
    static_assert(std::is_same_v<std::variant_alternative_t<0, VulkanGlobalDispatchResolutionResult>, VulkanGlobalDispatchResolutionError>);
    static_assert(std::is_same_v<std::variant_alternative_t<1, VulkanGlobalDispatchResolutionResult>, VulkanGlobalDispatchGeneration>);
    static_assert(std::is_copy_constructible_v<VulkanGlobalDispatchResolutionResult>);
    static_assert(std::is_nothrow_copy_constructible_v<VulkanGlobalDispatchResolutionResult>);
    static_assert(std::is_move_constructible_v<VulkanGlobalDispatchResolutionResult>);
    static_assert(std::is_nothrow_move_constructible_v<VulkanGlobalDispatchResolutionResult>);
    static_assert(!std::is_copy_assignable_v<VulkanGlobalDispatchResolutionResult>);
    static_assert(!std::is_move_assignable_v<VulkanGlobalDispatchResolutionResult>);
    static_assert(std::is_nothrow_destructible_v<VulkanGlobalDispatchResolutionResult>);

    const VulkanGlobalDispatchResolutionError left{ VulkanGlobalDispatchResolutionCode::VersionQueryFailure,
                                                    VulkanGlobalCommand::EnumerateInstanceVersion, VK_ERROR_INITIALIZATION_FAILED, 0 };
    const VulkanGlobalDispatchResolutionError same = left;
    const VulkanGlobalDispatchResolutionError different{ VulkanGlobalDispatchResolutionCode::InsufficientApiVersion,
                                                         VulkanGlobalCommand::EnumerateInstanceVersion, VK_SUCCESS, VK_API_VERSION_1_0 };
    ensure("equal errors compare equal", left == same);
    ensure("different errors compare unequal", !(left == different));
}

template<>
template<>
void render_vulkan_global_dispatch_test_object::test<2>()
{
    const auto                                 result     = resolveVulkanGlobalDispatchGeneration(nullptr);
    const VulkanGlobalDispatchResolutionError& null_error = requireError(result);
    ensure("a null resolver has a distinct error",
           null_error.mCode == VulkanGlobalDispatchResolutionCode::InvalidGetInstanceProcAddr && !null_error.mCommand &&
               null_error.mResult == VK_SUCCESS && null_error.mAvailableApiVersion == 0);

    constexpr std::array<MissingCommand, 3>      missing_commands{ MissingCommand::CreateInstance,
                                                              MissingCommand::EnumerateInstanceExtensionProperties,
                                                              MissingCommand::EnumerateInstanceLayerProperties };
    constexpr std::array<VulkanGlobalCommand, 3> reported_commands{ VulkanGlobalCommand::CreateInstance,
                                                                    VulkanGlobalCommand::EnumerateInstanceExtensionProperties,
                                                                    VulkanGlobalCommand::EnumerateInstanceLayerProperties };
    constexpr std::array<std::string_view, 3>    lookup_names{ "vkCreateInstance", "vkEnumerateInstanceExtensionProperties",
                                                            "vkEnumerateInstanceLayerProperties" };

    for (std::size_t missing_index = 0; missing_index < missing_commands.size(); ++missing_index)
    {
        GlobalDispatchState state;
        state.mMissing = missing_commands[missing_index];
        ScopedGlobalDispatchState scope(state);

        const auto                                 result_for_missing = resolveVulkanGlobalDispatchGeneration(fakeGetInstanceProcAddr);
        const VulkanGlobalDispatchResolutionError& error              = requireError(result_for_missing);
        ensure("a required command absence is reported exactly",
               error.mCode == VulkanGlobalDispatchResolutionCode::MissingRequiredCommand &&
                   error.mCommand == reported_commands[missing_index] && error.mResult == VK_SUCCESS && error.mAvailableApiVersion == 0);
        ensure_equals("resolution stops at the first missing required command", state.mLookupCount, missing_index + 1);
        ensure_equals("version is not queried after a required-command failure", state.mVersionCalls, std::size_t{ 0 });
        for (std::size_t lookup_index = 0; lookup_index <= missing_index; ++lookup_index)
        {
            ensureLookup(state, lookup_index, lookup_names[lookup_index]);
        }
    }
}

template<>
template<>
void render_vulkan_global_dispatch_test_object::test<3>()
{
    GlobalDispatchState state;
    state.mMissing = MissingCommand::EnumerateInstanceVersion;
    ScopedGlobalDispatchState scope(state);

    const auto                                 result = resolveVulkanGlobalDispatchGeneration(fakeGetInstanceProcAddr);
    const VulkanGlobalDispatchResolutionError& error  = requireError(result);
    ensure("an absent optional query uses the mandated Vulkan 1.0 fallback",
           error.mCode == VulkanGlobalDispatchResolutionCode::InsufficientApiVersion &&
               error.mCommand == VulkanGlobalCommand::EnumerateInstanceVersion && error.mResult == VK_SUCCESS &&
               error.mAvailableApiVersion == VK_API_VERSION_1_0);
    ensure_equals("the absent command is not called", state.mVersionCalls, std::size_t{ 0 });
    ensure_equals("all four names are resolved once", state.mLookupCount, std::size_t{ 4 });
    ensureLookup(state, 0, "vkCreateInstance");
    ensureLookup(state, 1, "vkEnumerateInstanceExtensionProperties");
    ensureLookup(state, 2, "vkEnumerateInstanceLayerProperties");
    ensureLookup(state, 3, "vkEnumerateInstanceVersion");
}

template<>
template<>
void render_vulkan_global_dispatch_test_object::test<4>()
{
    GlobalDispatchState state;
    state.mVersionResult = VK_ERROR_OUT_OF_HOST_MEMORY;
    state.mVersionOutput = VK_MAKE_API_VERSION(7, 99, 88, 77);
    ScopedGlobalDispatchState scope(state);

    const auto                                 result = resolveVulkanGlobalDispatchGeneration(fakeGetInstanceProcAddr);
    const VulkanGlobalDispatchResolutionError& error  = requireError(result);
    ensure("a failed version query preserves its result and ignores poisoned output",
           error.mCode == VulkanGlobalDispatchResolutionCode::VersionQueryFailure &&
               error.mCommand == VulkanGlobalCommand::EnumerateInstanceVersion && error.mResult == VK_ERROR_OUT_OF_HOST_MEMORY &&
               error.mAvailableApiVersion == 0);
    ensure_equals("the failed query is called exactly once", state.mVersionCalls, std::size_t{ 1 });
    ensure_equals("resolution makes no lookup after the version query", state.mLookupCount, std::size_t{ 4 });
}

template<>
template<>
void render_vulkan_global_dispatch_test_object::test<5>()
{
    {
        GlobalDispatchState state;
        state.mVersionOutput = VK_MAKE_API_VERSION(1, 1, 3, 0);
        ScopedGlobalDispatchState scope(state);

        const auto                                 result = resolveVulkanGlobalDispatchGeneration(fakeGetInstanceProcAddr);
        const VulkanGlobalDispatchResolutionError& error  = requireError(result);
        ensure("a nonstandard API variant is rejected before its numeric version",
               error.mCode == VulkanGlobalDispatchResolutionCode::UnsupportedApiVariant &&
                   error.mCommand == VulkanGlobalCommand::EnumerateInstanceVersion && error.mResult == VK_SUCCESS &&
                   error.mAvailableApiVersion == state.mVersionOutput);
        ensure_equals("the variant query is called once", state.mVersionCalls, std::size_t{ 1 });
        ensure_equals("the variant rejection performs exactly four lookups", state.mLookupCount, std::size_t{ 4 });
    }

    {
        GlobalDispatchState state;
        state.mVersionOutput = VK_MAKE_API_VERSION(0, 1, 0, 999);
        ScopedGlobalDispatchState scope(state);

        const auto                                 result = resolveVulkanGlobalDispatchGeneration(fakeGetInstanceProcAddr);
        const VulkanGlobalDispatchResolutionError& error  = requireError(result);
        ensure("a standard version below Vulkan 1.1 is rejected exactly",
               error.mCode == VulkanGlobalDispatchResolutionCode::InsufficientApiVersion &&
                   error.mCommand == VulkanGlobalCommand::EnumerateInstanceVersion && error.mResult == VK_SUCCESS &&
                   error.mAvailableApiVersion == state.mVersionOutput);
        ensure_equals("the insufficient-version query is called once", state.mVersionCalls, std::size_t{ 1 });
        ensure_equals("the insufficient-version rejection performs exactly four lookups", state.mLookupCount, std::size_t{ 4 });
    }
}

template<>
template<>
void render_vulkan_global_dispatch_test_object::test<6>()
{
    constexpr std::array<std::uint32_t, 2> accepted_versions{ RENDERER_VULKAN_API_VERSION, VK_MAKE_API_VERSION(0, 1, 4, 37) };

    for (const std::uint32_t accepted_version : accepted_versions)
    {
        GlobalDispatchState state;
        state.mVersionOutput = accepted_version;
        ScopedGlobalDispatchState scope(state);

        const auto  result     = resolveVulkanGlobalDispatchGeneration(fakeGetInstanceProcAddr);
        const auto* generation = std::get_if<VulkanGlobalDispatchGeneration>(&result);
        ensure("the exact floor and a higher standard version return a generation", generation != nullptr);
        ensure("the exact resolver and required functions are retained",
               generation->getInstanceProcAddr() == fakeGetInstanceProcAddr && generation->createInstance() == fakeCreateInstance &&
                   generation->enumerateInstanceExtensionProperties() == fakeEnumerateInstanceExtensionProperties &&
                   generation->enumerateInstanceLayerProperties() == fakeEnumerateInstanceLayerProperties &&
                   generation->enumerateInstanceVersion() == fakeEnumerateInstanceVersion);
        ensure_equals("the exact packed loader API version is retained", generation->loaderApiVersion(), accepted_version);
        ensure_equals("the accepted version query is called exactly once", state.mVersionCalls, std::size_t{ 1 });
        ensure_equals("successful resolution performs exactly four lookups", state.mLookupCount, std::size_t{ 4 });
        ensureLookup(state, 0, "vkCreateInstance");
        ensureLookup(state, 1, "vkEnumerateInstanceExtensionProperties");
        ensureLookup(state, 2, "vkEnumerateInstanceLayerProperties");
        ensureLookup(state, 3, "vkEnumerateInstanceVersion");
    }
}

} // namespace tut
