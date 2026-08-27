/**
 * @file llrendervulkanmaterialregistry_test.cpp
 * @brief Focused tests for the Vulkan material registry and frozen index mapping.
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

#include "llrendervulkanmaterial.h"
#include "lltut.h"

#include <array>
#include <cstdint>
#include <string>
#include <type_traits>

namespace
{

template<typename Handle>
Handle fakeHandle(std::uintptr_t value)
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

LLRenderVulkanMaterial::ImageBinding completeImageBinding()
{
    LLRenderVulkanMaterial::ImageBinding image;
    image.mImage = fakeHandle<VkImage>(0x20);
    image.mView = fakeHandle<VkImageView>(0x21);
    image.mFormat = VK_FORMAT_R8G8B8A8_UNORM;
    image.mExtent = { 8, 8 };
    image.mMipLevels = 1;
    image.mUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image.mAspect = VK_IMAGE_ASPECT_COLOR_BIT;
    image.mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image.mViewRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    return image;
}

LLRenderVulkanMaterial::PipelineBinding completePipelineBinding()
{
    using namespace LLRenderVulkanMaterial;

    static std::array<std::uint8_t, MATERIAL_PARAMETER_SIZE> mapped_parameters{};

    PipelineBinding pipeline;
    pipeline.mProgram = { "deferred.material.normspec", 0 };
    pipeline.mPipeline = fakeHandle<VkPipeline>(0x40);
    pipeline.mLayout = fakeHandle<VkPipelineLayout>(0x41);
    pipeline.mRenderPass = fakeHandle<VkRenderPass>(0x42);
    pipeline.mFramebuffer = fakeHandle<VkFramebuffer>(0x43);
    pipeline.mDescriptorSets = { fakeHandle<VkDescriptorSet>(0x44),
                                 fakeHandle<VkDescriptorSet>(0x45) };
    pipeline.mParameters.mBuffer = fakeHandle<VkBuffer>(0x46);
    pipeline.mParameters.mSize = mapped_parameters.size();
    pipeline.mParameters.mUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    pipeline.mParameters.mMemory = fakeHandle<VkDeviceMemory>(0x47);
    pipeline.mParameters.mMapped = mapped_parameters.data();
    pipeline.mParameters.mAllocationSize = mapped_parameters.size();
    pipeline.mParameters.mDescriptorRange = mapped_parameters.size();
    pipeline.mVertexShaderIdentity[0] = 0x5a;
    pipeline.mFragmentShaderIdentity[0] = 0xa5;
    return pipeline;
}

} // namespace

namespace tut
{

struct render_vulkan_material_registry_test
{
};

using render_vulkan_material_registry_group = test_group<render_vulkan_material_registry_test>;
using render_vulkan_material_registry_object = render_vulkan_material_registry_group::object;
render_vulkan_material_registry_group render_vulkan_material_registry_tests("render Vulkan material registry");

template<>
template<>
void render_vulkan_material_registry_object::test<1>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanMaterial;

    Registry registry;
    const BufferHandle buffer_handle{ 1, 4 };
    const ImageHandle image_handle{ 1, 5 };
    const SamplerHandle sampler_handle{ 1, 6 };
    const PipelineHandle pipeline_handle{ 1, 7 };

    BufferBinding buffer;
    buffer.mBuffer = fakeHandle<VkBuffer>(0x10);
    buffer.mSize = MATERIAL_INDEX_BUFFER_SIZE;
    buffer.mUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffer.mHasTranslatedIndices = true;
    buffer.mTranslatedIndices = MATERIAL_VULKAN_INDICES;
    const ImageBinding image = completeImageBinding();
    SamplerBinding sampler;
    sampler.mSampler = fakeHandle<VkSampler>(0x30);
    const PipelineBinding pipeline = completePipelineBinding();

    ensure("buffer registers", registry.addBuffer(buffer_handle, buffer));
    ensure("image registers", registry.addImage(image_handle, image));
    ensure("sampler registers", registry.addSampler(sampler_handle, sampler));
    ensure("pipeline registers", registry.addPipeline(pipeline_handle, pipeline));
    ensure("exact buffer resolves", registry.resolve(buffer_handle) != nullptr);
    ensure("exact image resolves", registry.resolve(image_handle) != nullptr);
    ensure("exact sampler resolves", registry.resolve(sampler_handle) != nullptr);
    ensure("exact pipeline and program resolve",
           registry.resolve(pipeline_handle, { "deferred.material.normspec", 0 }) != nullptr);
    ensure("translated indices are retained",
           registry.resolve(buffer_handle)->mTranslatedIndices == MATERIAL_VULKAN_INDICES);
    ensure("vertex shader identity is retained",
           registry.resolve(pipeline_handle, pipeline.mProgram)->mVertexShaderIdentity ==
               pipeline.mVertexShaderIdentity);
}

template<>
template<>
void render_vulkan_material_registry_object::test<2>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanMaterial;

    Registry registry;
    const ImageBinding image = completeImageBinding();
    const PipelineBinding pipeline = completePipelineBinding();

    ensure("image registers", registry.addImage({ 2, 7 }, image));
    ensure("pipeline registers", registry.addPipeline({ 4, 9 }, pipeline));
    ensure("stale image generation is rejected", registry.resolve(ImageHandle{ 2, 6 }) == nullptr);
    ensure("unknown image index is rejected", registry.resolve(ImageHandle{ 8, 7 }) == nullptr);
    ensure("program variant mismatch is rejected",
           registry.resolve(PipelineHandle{ 4, 9 }, { "deferred.material.normspec", 1 }) == nullptr);
    ensure("program name mismatch is rejected",
           registry.resolve(PipelineHandle{ 4, 9 }, { "other.material", 0 }) == nullptr);
    ensure("stale pipeline generation is rejected",
           registry.resolve(PipelineHandle{ 4, 8 }, pipeline.mProgram) == nullptr);
    ensure("duplicate live image index is rejected", !registry.addImage({ 2, 8 }, image));

    ImageBinding incomplete_image = image;
    incomplete_image.mView = VK_NULL_HANDLE;
    ensure("incomplete image is rejected", !registry.addImage({ 9, 1 }, incomplete_image));
    PipelineBinding incomplete_pipeline = pipeline;
    incomplete_pipeline.mVertexShaderIdentity = {};
    ensure("pipeline without immutable shader identity is rejected",
           !registry.addPipeline({ 9, 1 }, incomplete_pipeline));

    std::array<VkDescriptorSet, 2> equal_descriptor_sets{
        pipeline.mDescriptorSets[0], pipeline.mDescriptorSets[0]
    };
    ensure("equal non-null opaque descriptor set values form a valid pair",
           validMaterialDescriptorSetPair(equal_descriptor_sets));
    equal_descriptor_sets[0] = VK_NULL_HANDLE;
    ensure("a null parameter set is rejected",
           !validMaterialDescriptorSetPair(equal_descriptor_sets));
    equal_descriptor_sets = { pipeline.mDescriptorSets[0], VK_NULL_HANDLE };
    ensure("a null sampled-image set is rejected",
           !validMaterialDescriptorSetPair(equal_descriptor_sets));
}

template<>
template<>
void render_vulkan_material_registry_object::test<3>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanMaterial;

    ensure_equals("parameter packet remains exactly 272 bytes",
                  MATERIAL_PARAMETER_SIZE, static_cast<std::size_t>(272));
    for (std::size_t triangle = 0; triangle < MATERIAL_INDICES.size(); triangle += 3)
    {
        ensure_equals("Vulkan first vertex equals GL last vertex",
                      MATERIAL_VULKAN_INDICES[triangle], MATERIAL_INDICES[triangle + 2]);
        ensure_equals("cyclic rotation retains canonical first vertex",
                      MATERIAL_VULKAN_INDICES[triangle + 1], MATERIAL_INDICES[triangle]);
        ensure_equals("cyclic rotation retains canonical second vertex",
                      MATERIAL_VULKAN_INDICES[triangle + 2], MATERIAL_INDICES[triangle + 1]);
    }
}

template<>
template<>
void render_vulkan_material_registry_object::test<4>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanMaterial;

    MaterialInputs inputs;
    inputs.mFrame = 1;
    const auto frame = buildMaterialFrame(inputs);
    ensure("canonical material frame builds", frame.has_value());

    Registry registry;
    std::uint64_t recording_attempts = 0;
    std::uint64_t submissions = 0;
    ExecutionContext context;
    context.mDevice = fakeHandle<VkDevice>(0x100);
    context.mCommandBuffer = fakeHandle<VkCommandBuffer>(0x101);
    context.mQueue = fakeHandle<VkQueue>(0x102);
    context.mRecordingAttemptCount = &recording_attempts;
    context.mSubmissionCount = &submissions;
    context.mRequiredVertexShaderIdentity[0] = 1;
    context.mRequiredFragmentShaderIdentity[0] = 2;

    std::string error;
    ensure("unresolved registry is rejected", !execute(*frame, registry, context, error));
    ensure("preflight rejection explains failure", !error.empty());
    ensure_equals("preflight performs no recording attempt", recording_attempts, std::uint64_t{ 0 });
    ensure_equals("preflight performs no submission", submissions, std::uint64_t{ 0 });
}

} // namespace tut
