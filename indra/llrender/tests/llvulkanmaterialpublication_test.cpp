/**
 * @file llvulkanmaterialpublication_test.cpp
 * @brief Tests for neutral material shader publication and retirement.
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

#include "llvulkanmaterialpublication.h"
#include "lltut.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
using namespace LLRenderContract;

constexpr std::uint32_t SPIRV_MAGIC              = 0x07230203U;
constexpr std::uint32_t SPIRV_VERSION_1_0        = 0x00010000U;
constexpr std::uint32_t OP_ENTRY_POINT           = 15U;
constexpr std::uint32_t EXECUTION_MODEL_VERTEX   = 0U;
constexpr std::uint32_t EXECUTION_MODEL_FRAGMENT = 4U;
constexpr std::size_t   MAX_MODULE_WORDS         = (16U * 1024U * 1024U) / sizeof(std::uint32_t);

ShaderProgramKey canonicalKey()
{
    return legacyNormSpecModernHDRPipelineKey().mProgram;
}

std::vector<std::uint32_t> module(ShaderStage stage, std::uint32_t generator)
{
    const std::uint32_t execution_model = stage == ShaderStage::Vertex ? EXECUTION_MODEL_VERTEX : EXECUTION_MODEL_FRAGMENT;
    return { SPIRV_MAGIC, SPIRV_VERSION_1_0, generator, 2U, 0U, (5U << 16U) | OP_ENTRY_POINT, execution_model, 1U, 0x6e69616dU, 0U };
}

// This fixture passes the Stage 20 bounded runtime checks. It is deliberately
// not evidence of spirv-val, reflection, or packaged-byte provenance.
LoadedShaderProgram structurallyAcceptedProgram(std::uint32_t generator)
{
    return { canonicalKey(),
             { ShaderStage::Vertex, "main", module(ShaderStage::Vertex, generator) },
             { ShaderStage::Fragment, "main", module(ShaderStage::Fragment, generator) } };
}

} // namespace

namespace tut
{

struct vulkan_material_publication_test
{
};

using vulkan_material_publication_test_group  = test_group<vulkan_material_publication_test>;
using vulkan_material_publication_test_object = vulkan_material_publication_test_group::object;
vulkan_material_publication_test_group vulkan_material_publication_tests("vulkan material publication");

template<>
template<>
void vulkan_material_publication_test_object::test<1>()
{
    static_assert(!std::is_same_v<ShaderHandle, BufferHandle>);
    static_assert(!std::is_same_v<ShaderHandle, PipelineHandle>);
    static_assert(!std::is_copy_constructible_v<LegacyNormSpecShaderPublication>);
    static_assert(!std::is_copy_assignable_v<LegacyNormSpecShaderPublication>);
    static_assert(!std::is_move_constructible_v<LegacyNormSpecShaderPublication>);
    static_assert(!std::is_move_assignable_v<LegacyNormSpecShaderPublication>);

    const auto next = nextHandleGeneration(ShaderHandle{ 7, 9 });
    ensure("the next generation preserves its typed index", next && *next == ShaderHandle{ 7, 10 });
    const auto last = nextHandleGeneration(ShaderHandle{ 7, std::numeric_limits<std::uint32_t>::max() - 1 });
    ensure("the maximum generation itself remains reachable",
           last && *last == ShaderHandle{ 7, std::numeric_limits<std::uint32_t>::max() });
    ensure("a zero handle has no next generation", !nextHandleGeneration(ShaderHandle{}));
    ensure("a zero index has no next generation", !nextHandleGeneration(ShaderHandle{ 0, 9 }));
    ensure("a zero generation has no next generation", !nextHandleGeneration(ShaderHandle{ 7, 0 }));
    ensure("generation wrap fails closed", !nextHandleGeneration(ShaderHandle{ 7, std::numeric_limits<std::uint32_t>::max() }));
}

template<>
template<>
void vulkan_material_publication_test_object::test<2>()
{
    LegacyNormSpecShaderPublication publication;
    ensure("an empty owner has no canonical current handle", !publication.current(canonicalKey()));

    LoadedShaderProgram original     = structurallyAcceptedProgram(17);
    std::uint32_t*      aliased_word = &original.mVertex.mWords[2];
    const auto          handle       = publication.publish(std::move(original));
    ensure("the first canonical publication uses the fixed initial handle", handle && *handle == ShaderHandle{ 1, 1 });
    ensure("the canonical program resolves to the current handle", publication.current(canonicalKey()) == handle);

    ShaderProgramKey wrong_key = canonicalKey();
    ++wrong_key.mVariant;
    ensure("a wrong program key has no current handle", !publication.current(wrong_key));

    const auto lease = publication.resolveForFrame(*handle, 3);
    ensure("the exact current handle acquires an immutable frame lease",
           lease && lease->mHandle == *handle && lease->mFrame == 3 && lease->mProgram);
    *aliased_word = 99;
    ensure("published words do not retain aliases from an rvalue caller", lease->mProgram->mVertex.mWords[2] == 17);
}

template<>
template<>
void vulkan_material_publication_test_object::test<3>()
{
    LoadedShaderProgram value = structurallyAcceptedProgram(1);
    ensure("the canonical in-memory program passes bounded runtime checks", validLegacyNormSpecProductionShaderProgram(value));

    value.mProgram.mName = "other.material";
    ensure("a wrong program identity is invalid", !validLegacyNormSpecProductionShaderProgram(value));
    value = structurallyAcceptedProgram(1);
    ++value.mProgram.mVariant;
    ensure("a wrong program variant is invalid", !validLegacyNormSpecProductionShaderProgram(value));
    value                = structurallyAcceptedProgram(1);
    value.mVertex.mStage = ShaderStage::Fragment;
    ensure("a mislabeled vertex stage is invalid", !validLegacyNormSpecProductionShaderProgram(value));
    value                     = structurallyAcceptedProgram(1);
    value.mVertex.mEntryPoint = "other";
    ensure("a wrong vertex entry-point label is invalid", !validLegacyNormSpecProductionShaderProgram(value));
    value                  = structurallyAcceptedProgram(1);
    value.mFragment.mStage = ShaderStage::Vertex;
    ensure("a mislabeled fragment stage is invalid", !validLegacyNormSpecProductionShaderProgram(value));
    value                       = structurallyAcceptedProgram(1);
    value.mFragment.mEntryPoint = "other";
    ensure("a wrong fragment entry-point label is invalid", !validLegacyNormSpecProductionShaderProgram(value));
    value                   = structurallyAcceptedProgram(1);
    value.mVertex.mWords[0] = 0;
    ensure("invalid vertex words are rejected", !validLegacyNormSpecProductionShaderProgram(value));
    value                     = structurallyAcceptedProgram(1);
    value.mFragment.mWords[6] = EXECUTION_MODEL_VERTEX;
    ensure("invalid fragment execution-model words are rejected", !validLegacyNormSpecProductionShaderProgram(value));
    value = structurallyAcceptedProgram(1);
    value.mVertex.mWords.resize(MAX_MODULE_WORDS + 1, 0U);
    ensure("an in-memory module cannot bypass the file-size ceiling", !validLegacyNormSpecProductionShaderProgram(value));

    LegacyNormSpecShaderPublication publication;
    ensure("an invalid first publication fails atomically",
           !publication.publish(value) && !publication.current(canonicalKey()) && publication.completedThrough() == 0);
}

template<>
template<>
void vulkan_material_publication_test_object::test<4>()
{
    LegacyNormSpecShaderPublication publication;
    const auto                      published = publication.publish(structurallyAcceptedProgram(1));
    ensure("the rejection fixture publishes", published.has_value());
    const ShaderHandle handle = *published;
    ensure("the setup frame resolves", publication.resolveForFrame(handle, 10).has_value());

    ensure("a zero handle is rejected", !publication.resolveForFrame({}, 10));
    ensure("an unknown index is rejected", !publication.resolveForFrame({ 9, 1 }, 100));
    ensure("an old generation is rejected", !publication.resolveForFrame({ 1, 0 }, 100));
    ensure("a future generation is rejected", !publication.resolveForFrame({ 1, 2 }, 100));
    ensure("a zero frame is rejected", !publication.resolveForFrame(handle, 0));
    ensure("rejections do not advance record order", publication.resolveForFrame(handle, 10).has_value());
    ensure("recorded frame regression is rejected", !publication.resolveForFrame(handle, 9));

    const auto completed = publication.completeThrough(10);
    ensure("completion does not retire the current generation", completed && completed->empty());
    ensure("a completed frame cannot acquire a lease", !publication.resolveForFrame(handle, 10));
    ensure("the next frame can still acquire the current generation", publication.resolveForFrame(handle, 11).has_value());
}

template<>
template<>
void vulkan_material_publication_test_object::test<5>()
{
    LegacyNormSpecShaderPublication publication;
    LoadedShaderProgram             first         = structurallyAcceptedProgram(1);
    const auto                      old_published = publication.publish(first);
    ensure("the replacement fixture first generation publishes", old_published.has_value());
    const ShaderHandle old_handle = *old_published;
    auto               old_lease  = publication.resolveForFrame(old_handle, 7);
    ensure("the old generation lease is acquired", old_lease.has_value());
    std::weak_ptr<const LoadedShaderProgram> old_storage = old_lease->mProgram;

    LoadedShaderProgram replacement = structurallyAcceptedProgram(2);
    const auto          new_handle  = publication.publish(replacement);
    ensure("replacement preserves the index and advances exactly one generation", new_handle && *new_handle == ShaderHandle{ 1, 2 });
    ensure("record order cannot regress across a replacement", !publication.resolveForFrame(*new_handle, 6));
    ensure("old and replacement leases may coexist in the same recorded frame", publication.resolveForFrame(*new_handle, 7).has_value());
    ensure("the replaced handle is stale immediately", !publication.resolveForFrame(old_handle, 8));
    ensure("the replacement is the only canonical current generation", publication.current(canonicalKey()) == new_handle);

    auto new_lease                  = publication.resolveForFrame(*new_handle, 8);
    replacement.mFragment.mWords[2] = 99;
    ensure("replacement bytes are immutable after caller mutation", new_lease && new_lease->mProgram->mFragment.mWords[2] == 2);
    old_lease.reset();
    ensure("the owner retains superseded bytes before completion", !old_storage.expired());
}

template<>
template<>
void vulkan_material_publication_test_object::test<6>()
{
    LegacyNormSpecShaderPublication publication;
    const auto                      old_published = publication.publish(structurallyAcceptedProgram(1));
    ensure("the completion fixture first generation publishes", old_published.has_value());
    const ShaderHandle old_handle = *old_published;
    auto               old_lease  = publication.resolveForFrame(old_handle, 10);
    ensure("the last-use lease is acquired", old_lease.has_value());
    std::weak_ptr<const LoadedShaderProgram> old_storage   = old_lease->mProgram;
    const auto                               new_published = publication.publish(structurallyAcceptedProgram(2));
    ensure("the completion fixture replacement publishes", new_published.has_value());
    const ShaderHandle new_handle = *new_published;
    old_lease.reset();

    const auto early = publication.completeThrough(9);
    ensure("completion before last use retires nothing", early && early->empty() && !old_storage.expired());
    const auto exact = publication.completeThrough(10);
    ensure("completion at last use retires the exact superseded generation",
           exact && *exact == std::vector<ShaderGenerationRetirement>{ { old_handle, 10 } } && old_storage.expired());
    ensure("the current generation survives completion",
           publication.current(canonicalKey()) == new_handle && publication.resolveForFrame(new_handle, 11).has_value());
}

template<>
template<>
void vulkan_material_publication_test_object::test<7>()
{
    LegacyNormSpecShaderPublication publication;
    const auto                      first_published = publication.publish(structurallyAcceptedProgram(1));
    ensure("the ordered-retirement fixture first generation publishes", first_published.has_value());
    const ShaderHandle first            = *first_published;
    auto               first_lease      = publication.resolveForFrame(first, 5);
    const auto         second_published = publication.publish(structurallyAcceptedProgram(2));
    ensure("the ordered-retirement fixture second generation publishes", second_published.has_value());
    const ShaderHandle second          = *second_published;
    auto               second_lease    = publication.resolveForFrame(second, 8);
    const auto         third_published = publication.publish(structurallyAcceptedProgram(3));
    ensure("the ordered-retirement fixture third generation publishes", third_published.has_value());
    const ShaderHandle third = *third_published;
    first_lease.reset();
    second_lease.reset();

    const auto retired = publication.completeThrough(8);
    ensure("multiple generations retire in publication order",
           retired && *retired == std::vector<ShaderGenerationRetirement>{ { first, 5 }, { second, 8 } });
    ensure("the newest generation is never retired by completion alone",
           publication.current(canonicalKey()) == third && publication.resolveForFrame(third, 9).has_value());
}

template<>
template<>
void vulkan_material_publication_test_object::test<8>()
{
    LegacyNormSpecShaderPublication publication;
    ensure("zero completion fails without advancing the watermark", !publication.completeThrough(0) && publication.completedThrough() == 0);
    const auto initial_completion = publication.completeThrough(5);
    ensure("completion may precede first publication", initial_completion && initial_completion->empty());
    const auto first_published = publication.publish(structurallyAcceptedProgram(1));
    ensure("the atomicity fixture first generation publishes", first_published.has_value());
    const ShaderHandle first = *first_published;

    LoadedShaderProgram invalid   = structurallyAcceptedProgram(2);
    invalid.mFragment.mEntryPoint = "wrong";
    ensure("an invalid replacement leaves the current generation unchanged",
           !publication.publish(invalid) && publication.current(canonicalKey()) == first);

    const auto second_published = publication.publish(structurallyAcceptedProgram(2));
    ensure("the publication after an invalid replacement is exactly the next generation",
           second_published && *second_published == ShaderHandle{ 1, 2 });
    const ShaderHandle second           = *second_published;
    const auto         equal_completion = publication.completeThrough(5);
    ensure("an equal watermark retires a newly pending generation with no recorded use",
           equal_completion && *equal_completion == std::vector<ShaderGenerationRetirement>{ { first, 0 } });
    ensure("completion regression fails without changing the watermark or current generation",
           !publication.completeThrough(4) && publication.completedThrough() == 5 && publication.current(canonicalKey()) == second);
    const auto repeated = publication.completeThrough(5);
    ensure("equal completion is otherwise idempotent", repeated && repeated->empty());
}

template<>
template<>
void vulkan_material_publication_test_object::test<9>()
{
    LegacyNormSpecShaderPublication publication;
    const auto                      first_published = publication.publish(structurallyAcceptedProgram(1));
    ensure("the external-lease fixture first generation publishes", first_published.has_value());
    const ShaderHandle first = *first_published;
    auto               lease = publication.resolveForFrame(first, 3);
    ensure("the external ownership lease is acquired", lease.has_value());
    std::weak_ptr<const LoadedShaderProgram> storage = lease->mProgram;
    ensure("the external-lease fixture replacement publishes", publication.publish(structurallyAcceptedProgram(2)).has_value());

    const auto retired = publication.completeThrough(3);
    ensure("logical retirement is reported while an outside lease remains valid",
           retired && *retired == std::vector<ShaderGenerationRetirement>{ { first, 3 } } && !storage.expired() &&
               lease->mProgram->mVertex.mWords[2] == 1);
    ensure("the retired handle cannot be reacquired", !publication.resolveForFrame(first, 4));
    lease.reset();
    ensure("physical bytes release after the final outside owner", storage.expired());
}

} // namespace tut
