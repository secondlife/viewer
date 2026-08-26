/**
 * @file llrendervulkantonemapregistry_test.cpp
 * @brief Focused tests for the Vulkan tonemap registry.
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

#include "llrendervulkantonemap.h"
#include "lltut.h"

#include <cstdint>
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

} // namespace

namespace tut
{

struct render_vulkan_tonemap_registry_test
{
};

using render_vulkan_tonemap_registry_group = test_group<render_vulkan_tonemap_registry_test>;
using render_vulkan_tonemap_registry_object = render_vulkan_tonemap_registry_group::object;
render_vulkan_tonemap_registry_group render_vulkan_tonemap_registry_tests("render Vulkan tonemap registry");

template<>
template<>
void render_vulkan_tonemap_registry_object::test<1>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanTonemap;

    Registry registry;
    const BufferHandle buffer_handle{ 1, 4 };
    const ImageHandle image_handle{ 1, 5 };
    const SamplerHandle sampler_handle{ 1, 6 };
    const PipelineHandle pipeline_handle{ 1, 7 };

    BufferBinding buffer{ fakeHandle<VkBuffer>(0x10), 48, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT };
    ImageBinding image{ fakeHandle<VkImage>(0x20), fakeHandle<VkImageView>(0x21),
                        VK_FORMAT_R8G8B8A8_UNORM, { 8, 8 }, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT };
    SamplerBinding sampler{ fakeHandle<VkSampler>(0x30), Filter::Nearest, Filter::Nearest,
                            AddressMode::Mirror, AddressMode::Mirror };
    PipelineBinding pipeline;
    pipeline.mProgram = { "deferred.tonemap", 3 };
    pipeline.mDestinationFormat = PixelFormat::RGBA8Unorm;
    pipeline.mExtent = { 8, 8 };
    pipeline.mPipeline = fakeHandle<VkPipeline>(0x40);
    pipeline.mLayout = fakeHandle<VkPipelineLayout>(0x41);
    pipeline.mRenderPass = fakeHandle<VkRenderPass>(0x42);
    pipeline.mFramebuffer = fakeHandle<VkFramebuffer>(0x43);
    pipeline.mDescriptorSet = fakeHandle<VkDescriptorSet>(0x44);
    pipeline.mSceneView = fakeHandle<VkImageView>(0x50);
    pipeline.mExposureView = fakeHandle<VkImageView>(0x51);
    pipeline.mDestinationView = image.mView;
    pipeline.mPointSampler = sampler.mSampler;
    pipeline.mLinearSampler = fakeHandle<VkSampler>(0x31);
    pipeline.mDestinationFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    pipeline.mDescriptorBindings = { 0, 1 };
    pipeline.mVertexStride = 16;
    pipeline.mPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
    pipeline.mPushConstantSize = 16;

    ensure("buffer registers", registry.addBuffer(buffer_handle, buffer));
    ensure("image registers", registry.addImage(image_handle, image));
    ensure("sampler registers", registry.addSampler(sampler_handle, sampler));
    ensure("pipeline registers", registry.addPipeline(pipeline_handle, pipeline));
    ensure("exact buffer resolves", registry.resolve(buffer_handle) != nullptr);
    ensure("exact image resolves", registry.resolve(image_handle) != nullptr);
    ensure("exact sampler resolves", registry.resolve(sampler_handle) != nullptr);
    ensure("exact pipeline resolves", registry.resolve(pipeline_handle, { "deferred.tonemap", 3 }) != nullptr);
}

template<>
template<>
void render_vulkan_tonemap_registry_object::test<2>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanTonemap;

    Registry registry;
    const ImageBinding image{ fakeHandle<VkImage>(0x20), fakeHandle<VkImageView>(0x21),
                              VK_FORMAT_R8G8B8A8_UNORM, { 8, 8 }, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT };
    PipelineBinding pipeline;
    pipeline.mProgram = { "deferred.tonemap", 3 };
    pipeline.mPipeline = fakeHandle<VkPipeline>(0x40);
    pipeline.mLayout = fakeHandle<VkPipelineLayout>(0x41);
    pipeline.mRenderPass = fakeHandle<VkRenderPass>(0x42);
    pipeline.mFramebuffer = fakeHandle<VkFramebuffer>(0x43);
    pipeline.mDescriptorSet = fakeHandle<VkDescriptorSet>(0x44);

    ensure("image registers", registry.addImage({ 2, 7 }, image));
    ensure("pipeline registers", registry.addPipeline({ 4, 9 }, pipeline));
    ensure("stale image generation is rejected", registry.resolve(ImageHandle{ 2, 6 }) == nullptr);
    ensure("unknown image index is rejected", registry.resolve(ImageHandle{ 8, 7 }) == nullptr);
    ensure("program variant mismatch is rejected",
           registry.resolve(PipelineHandle{ 4, 9 }, { "deferred.tonemap", 2 }) == nullptr);
    ensure("program name mismatch is rejected",
           registry.resolve(PipelineHandle{ 4, 9 }, { "other.tonemap", 3 }) == nullptr);
    ensure("stale pipeline generation is rejected",
           registry.resolve(PipelineHandle{ 4, 8 }, { "deferred.tonemap", 3 }) == nullptr);
    ensure("duplicate live index is rejected", !registry.addImage({ 2, 8 }, image));
    ImageBinding incomplete = image;
    incomplete.mView = VK_NULL_HANDLE;
    ensure("incomplete image is rejected", !registry.addImage({ 9, 1 }, incomplete));
}

} // namespace tut
