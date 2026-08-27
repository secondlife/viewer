/**
 * @file llrendervulkantextureuploadregistry_test.cpp
 * @brief Context-free tests for the Vulkan streamed-upload registry and mapping.
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

#include "llrendervulkantextureupload.h"
#include "lltut.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>

namespace
{

constexpr LLRenderVulkanTextureUpload::NativeOwnershipToken OWNER_TOKEN =
    0x51a6e15f0c0ffee1ULL;

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

LLRenderVulkanTextureUpload::BufferBinding completeBufferBinding(
    std::uintptr_t base, void* mapped, VkDeviceSize size,
    VkBufferUsageFlags usage)
{
    LLRenderVulkanTextureUpload::BufferBinding binding;
    binding.mBuffer = fakeHandle<VkBuffer>(base);
    binding.mMemory = fakeHandle<VkDeviceMemory>(base + 1);
    binding.mOwnershipToken = OWNER_TOKEN;
    binding.mMapped = mapped;
    binding.mSize = size;
    binding.mAllocationSize = size;
    binding.mMemoryOffset = 0;
    binding.mCreateFlags = 0;
    binding.mUsage = usage;
    binding.mSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    binding.mMemoryProperties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    return binding;
}

LLRenderVulkanTextureUpload::ImageBinding completeImageBinding(
    std::uintptr_t base, LLRenderContract::Extent2D resident,
    LLRenderContract::Extent2D logical, std::uint32_t discard,
    std::uint32_t mip_levels, VkImageUsageFlags usage)
{
    LLRenderVulkanTextureUpload::ImageBinding image;
    image.mImage = fakeHandle<VkImage>(base);
    image.mView = fakeHandle<VkImageView>(base + 1);
    image.mMemory = fakeHandle<VkDeviceMemory>(base + 2);
    image.mOwnershipToken = OWNER_TOKEN;
    image.mAllocationSize = 4096;
    image.mMemoryOffset = 0;
    image.mCreateFlags = 0;
    image.mImageType = VK_IMAGE_TYPE_2D;
    image.mFormat = VK_FORMAT_R8G8B8A8_UNORM;
    image.mResidentExtent = resident;
    image.mLogicalExtent = logical;
    image.mResidentDiscard = discard;
    image.mMipLevels = mip_levels;
    image.mArrayLayers = 1;
    image.mSamples = VK_SAMPLE_COUNT_1_BIT;
    image.mTiling = VK_IMAGE_TILING_OPTIMAL;
    image.mUsage = usage;
    image.mSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image.mAspect = VK_IMAGE_ASPECT_COLOR_BIT;
    image.mLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image.mViewType = VK_IMAGE_VIEW_TYPE_2D;
    image.mViewFormat = VK_FORMAT_R8G8B8A8_UNORM;
    image.mViewComponents = {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY
    };
    image.mViewRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, 1 };
    return image;
}

LLRenderVulkanTextureUpload::PipelineBinding completePipelineBinding(
    VkImageView color_view)
{
    using namespace LLRenderVulkanTextureUpload;

    PipelineBinding pipeline;
    pipeline.mProgram = { "contract.sample-texture", 0 };
    pipeline.mPipeline = fakeHandle<VkPipeline>(0x500);
    pipeline.mLayout = fakeHandle<VkPipelineLayout>(0x501);
    pipeline.mRenderPass = fakeHandle<VkRenderPass>(0x502);
    pipeline.mFramebuffer = fakeHandle<VkFramebuffer>(0x503);
    pipeline.mDescriptorSet = fakeHandle<VkDescriptorSet>(0x504);
    pipeline.mOwnershipToken = OWNER_TOKEN;
    pipeline.mExtent = {
        LLRenderContract::TEXTURE_UPLOAD_OUTPUT_WIDTH,
        LLRenderContract::TEXTURE_UPLOAD_OUTPUT_HEIGHT
    };
    pipeline.mColorView = color_view;
    pipeline.mColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    pipeline.mVertexShaderIdentity[0] = 0x5a;
    pipeline.mFragmentShaderIdentity[0] = 0xa5;
    return pipeline;
}

} // namespace

namespace tut
{

struct render_vulkan_texture_upload_registry_test
{
};

using render_vulkan_texture_upload_registry_group =
    test_group<render_vulkan_texture_upload_registry_test>;
using render_vulkan_texture_upload_registry_object =
    render_vulkan_texture_upload_registry_group::object;
render_vulkan_texture_upload_registry_group
    render_vulkan_texture_upload_registry_tests("render Vulkan texture upload registry");

template<>
template<>
void render_vulkan_texture_upload_registry_object::test<1>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanTextureUpload;

    std::array<std::uint8_t, SCREEN_TRIANGLE_BYTE_SIZE> screen_bytes{};
    std::array<std::uint8_t, STAGING_BYTE_SIZE> staging_bytes{};
    std::array<std::uint8_t, READBACK_BYTE_SIZE> readback_bytes{};
    constexpr VkImageUsageFlags streamed_usage =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT;

    BufferBinding screen = completeBufferBinding(
        0x100, screen_bytes.data(), screen_bytes.size(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    screen.mHasFixtureBytes = true;
    screen.mFixtureBytes = screen_bytes;
    ImageBinding old_image = completeImageBinding(
        0x200, { 8, 4 }, { 32, 16 }, 2, 3, streamed_usage);
    old_image.mHasPreExecutionMipSnapshot = true;
    old_image.mPreExecutionMipRGBA8 = makeTextureUploadFixture().mOldMipRGBA8;
    const ImageBinding replacement = completeImageBinding(
        0x210, { 8, 4 }, { 32, 16 }, 2, 3, streamed_usage);
    const ImageBinding output = completeImageBinding(
        0x220, { 4, 2 }, { 4, 2 }, 0, 1,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    SamplerBinding sampler;
    sampler.mSampler = fakeHandle<VkSampler>(0x300);
    sampler.mOwnershipToken = OWNER_TOKEN;
    const PipelineBinding pipeline = completePipelineBinding(output.mView);
    TransferResources transfer{
        completeBufferBinding(0x400, staging_bytes.data(), staging_bytes.size(),
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
        completeBufferBinding(0x410, readback_bytes.data(), readback_bytes.size(),
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT)
    };
    LifecycleLedger lifecycle{ { 11, 4 }, TEXTURE_UPLOAD_PRIOR_REVISION };

    Registry registry;
    ensure("screen registration succeeds",
           registry.addScreenTriangle({ 1, 7 }, screen));
    ensure("consecutive image generations register",
           registry.addImageGenerations({ 11, 4 }, old_image, { 11, 5 }, replacement));
    ensure("output registration succeeds", registry.addOutput({ 12, 3 }, output));
    ensure("sampler registration succeeds", registry.addSampler({ 2, 8 }, sampler));
    ensure("pipeline registration succeeds", registry.addPipeline({ 3, 9 }, pipeline));
    ensure("transfer resources register", registry.addTransferResources(transfer));
    ensure("lifecycle registration succeeds", registry.addLifecycle(&lifecycle));

    ensure("exact screen generation resolves", registry.resolve(BufferHandle{ 1, 7 }) != nullptr);
    ensure("exact old generation resolves", registry.resolveRegisteredImage({ 11, 4 }) != nullptr);
    ensure("exact replacement generation resolves", registry.resolveRegisteredImage({ 11, 5 }) != nullptr);
    ensure("exact output generation resolves", registry.resolveOutput({ 12, 3 }) != nullptr);
    ensure("exact sampler generation resolves", registry.resolve(SamplerHandle{ 2, 8 }) != nullptr);
    ensure("exact pipeline and program resolve",
           registry.resolve(PipelineHandle{ 3, 9 }, { "contract.sample-texture", 0 }) != nullptr);
    ensure("transfer registration is retained", registry.transferResources() != nullptr);
    ensure("registry borrows lifecycle", registry.lifecycle() == &lifecycle);
    ensure("published old generation resolves", registry.isResolvable({ 11, 4 }));
    ensure("unpublished replacement does not resolve", !registry.isResolvable({ 11, 5 }));
}

template<>
template<>
void render_vulkan_texture_upload_registry_object::test<2>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanTextureUpload;

    std::array<std::uint8_t, SCREEN_TRIANGLE_BYTE_SIZE> bytes{};
    BufferBinding screen = completeBufferBinding(
        0x100, bytes.data(), bytes.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    screen.mHasFixtureBytes = true;
    ImageBinding first = completeImageBinding(
        0x200, { 8, 4 }, { 32, 16 }, 2, 3,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT);
    ImageBinding second = completeImageBinding(
        0x210, { 8, 4 }, { 32, 16 }, 2, 3,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT);
    first.mHasPreExecutionMipSnapshot = true;

    BufferBinding incomplete_screen = screen;
    incomplete_screen.mMemory = VK_NULL_HANDLE;
    ensure("incomplete screen is rejected",
           !Registry{}.addScreenTriangle({ 1, 1 }, incomplete_screen));
    BufferBinding ownerless_screen = screen;
    ownerless_screen.mOwnershipToken = 0;
    ensure("screen without native ownership is rejected",
           !Registry{}.addScreenTriangle({ 1, 1 }, ownerless_screen));
    ensure("different image indices are rejected",
           !Registry{}.addImageGenerations({ 11, 1 }, first, { 12, 2 }, second));
    ensure("skipped image generation is rejected",
           !Registry{}.addImageGenerations({ 11, 1 }, first, { 11, 3 }, second));
    ensure("generation overflow is rejected",
           !Registry{}.addImageGenerations(
               { 11, std::numeric_limits<std::uint32_t>::max() }, first,
               { 11, 1 }, second));
    second.mMemory = first.mMemory;
    ensure("aliased image memory is rejected",
           !Registry{}.addImageGenerations({ 11, 1 }, first, { 11, 2 }, second));

    ImageBinding ownerless_old = first;
    ownerless_old.mOwnershipToken = 0;
    ensure("image without native ownership is rejected",
           !Registry{}.addImageGenerations({ 11, 1 }, ownerless_old,
                                            { 11, 2 }, completeImageBinding(
                                                0x220, { 8, 4 }, { 32, 16 }, 2, 3,
                                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                    VK_IMAGE_USAGE_SAMPLED_BIT)));

    PipelineBinding pipeline = completePipelineBinding(fakeHandle<VkImageView>(0x221));
    pipeline.mVertexShaderIdentity = {};
    ensure("pipeline without immutable shader identity is rejected",
           !Registry{}.addPipeline({ 1, 1 }, pipeline));
    pipeline = completePipelineBinding(fakeHandle<VkImageView>(0x221));
    pipeline.mOwnershipToken = 0;
    ensure("pipeline without native ownership is rejected",
           !Registry{}.addPipeline({ 1, 1 }, pipeline));

    SamplerBinding ownerless_sampler;
    ownerless_sampler.mSampler = fakeHandle<VkSampler>(0x300);
    ensure("sampler without native ownership is rejected",
           !Registry{}.addSampler({ 1, 1 }, ownerless_sampler));

    LifecycleLedger pending{ { 11, 1 }, TEXTURE_UPLOAD_PRIOR_REVISION, true };
    ensure("pending lifecycle is rejected", !Registry{}.addLifecycle(&pending));
    LifecycleLedger completed{ { 11, 1 }, TEXTURE_UPLOAD_PRIOR_REVISION };
    completed.mCompletionCount = 1;
    ensure("prepublished completion is rejected", !Registry{}.addLifecycle(&completed));
}

template<>
template<>
void render_vulkan_texture_upload_registry_object::test<3>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanTextureUpload;

    ensure_equals("screen triangle stays exactly 48 bytes",
                  SCREEN_TRIANGLE_BYTE_SIZE, static_cast<std::size_t>(48));
    ensure_equals("staging stays exactly 144 bytes",
                  STAGING_BYTE_SIZE, static_cast<std::size_t>(144));
    ensure_equals("readback stays exactly 200 bytes",
                  READBACK_BYTE_SIZE, static_cast<std::size_t>(200));
    ensure_equals("output follows the three tight mips",
                  OUTPUT_READBACK_BYTE_OFFSET, static_cast<std::size_t>(168));
    const std::array<VkDeviceSize, 4> expected_rows{ 108, 72, 36, 0 };
    ensure("row regions reverse only source row order",
           UPLOAD_ROW_SOURCE_OFFSETS == expected_rows);
    ensure_equals("mip zero readback begins at zero",
                  TEXTURE_UPLOAD_MIP_BYTE_OFFSETS[0], static_cast<std::size_t>(0));
    ensure_equals("mip one readback begins at 128",
                  TEXTURE_UPLOAD_MIP_BYTE_OFFSETS[1], static_cast<std::size_t>(128));
    ensure_equals("mip two readback begins at 160",
                  TEXTURE_UPLOAD_MIP_BYTE_OFFSETS[2], static_cast<std::size_t>(160));
}

template<>
template<>
void render_vulkan_texture_upload_registry_object::test<4>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanTextureUpload;

    const TextureUploadCase upload_case = makeTextureUploadCase();
    Registry registry;
    LifecycleLedger lifecycle{
        upload_case.mInputs.mHandles.mOldImage, TEXTURE_UPLOAD_PRIOR_REVISION
    };
    ensure("lifecycle setup succeeds", registry.addLifecycle(&lifecycle));
    const LifecycleLedger lifecycle_before = lifecycle;

    ExecutionResult result = makeTextureUploadArtifact();
    result.mSampledRGBA8 = { 0x5a };
    const ExecutionResult result_before = result;
    std::uint64_t recording_attempts = 0;
    std::uint64_t submissions = 0;
    ExecutionContext context;
    context.mDevice = fakeHandle<VkDevice>(0x600);
    context.mCommandPool = fakeHandle<VkCommandPool>(0x603);
    context.mCommandBuffer = fakeHandle<VkCommandBuffer>(0x601);
    context.mQueue = fakeHandle<VkQueue>(0x602);
    context.mOwnershipToken = OWNER_TOKEN;
    context.mQueueFamilyIndex = 2;
    context.mQueueFamilyFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
    context.mQueueCount = 1;
    context.mQueueIndex = 0;
    context.mCommandPoolQueueFamilyIndex = 2;
    context.mCommandPoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    context.mCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    context.mRecordingAttemptCount = &recording_attempts;
    context.mSubmissionCount = &submissions;
    context.mRequiredVertexShaderIdentity[0] = 1;
    context.mRequiredFragmentShaderIdentity[0] = 2;

    std::string error;
    ensure("unresolved registry is rejected before Vulkan",
           !execute(upload_case.mFrame, registry, context, result, &error));
    ensure("preflight rejection explains failure", !error.empty());
    ensure_equals("preflight performs no recording attempt", recording_attempts,
                  std::uint64_t{ 0 });
    ensure_equals("preflight performs no submission", submissions,
                  std::uint64_t{ 0 });
    ensure("preflight leaves lifecycle unchanged", lifecycle == lifecycle_before);
    ensure("preflight leaves caller result unchanged", result == result_before);
}

template<>
template<>
void render_vulkan_texture_upload_registry_object::test<5>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanTextureUpload;

    const TextureUploadCase upload_case = makeTextureUploadCase();
    std::uint64_t recording_attempts = 0;
    std::uint64_t submissions = 0;
    ExecutionContext canonical;
    canonical.mDevice = fakeHandle<VkDevice>(0x700);
    canonical.mCommandPool = fakeHandle<VkCommandPool>(0x701);
    canonical.mCommandBuffer = fakeHandle<VkCommandBuffer>(0x702);
    canonical.mQueue = fakeHandle<VkQueue>(0x703);
    canonical.mOwnershipToken = OWNER_TOKEN;
    canonical.mQueueFamilyIndex = 3;
    canonical.mQueueFamilyFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
    canonical.mQueueCount = 1;
    canonical.mQueueIndex = 0;
    canonical.mCommandPoolQueueFamilyIndex = 3;
    canonical.mCommandPoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    canonical.mCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    canonical.mRecordingAttemptCount = &recording_attempts;
    canonical.mSubmissionCount = &submissions;
    canonical.mRequiredVertexShaderIdentity[0] = 1;
    canonical.mRequiredFragmentShaderIdentity[0] = 2;

    std::array<ExecutionContext, 11> poisoned{};
    poisoned.fill(canonical);
    poisoned[0].mOwnershipToken = 0;
    poisoned[1].mCommandPool = VK_NULL_HANDLE;
    poisoned[2].mQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    poisoned[3].mQueueFamilyFlags = VK_QUEUE_TRANSFER_BIT;
    poisoned[4].mQueueFamilyFlags = VK_QUEUE_GRAPHICS_BIT;
    poisoned[5].mQueueCount = 0;
    poisoned[6].mQueueCount = 2;
    poisoned[7].mQueueIndex = 1;
    poisoned[8].mCommandPoolQueueFamilyIndex = 4;
    poisoned[9].mCommandPoolFlags = 0;
    poisoned[10].mCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

    for (const ExecutionContext& context : poisoned)
    {
        Registry registry;
        ExecutionResult result = makeTextureUploadArtifact();
        result.mSampledRGBA8 = { 0x7b };
        const ExecutionResult result_before = result;
        std::string error;
        ensure("poisoned command context is rejected",
               !execute(upload_case.mFrame, registry, context, result, &error));
        ensure_equals("context poison is identified before registry resolution",
                      error, std::string("execution context is incomplete"));
        ensure("context rejection leaves caller result unchanged",
               result == result_before);
    }
    ensure_equals("context poison performs no recording attempt", recording_attempts,
                  std::uint64_t{ 0 });
    ensure_equals("context poison performs no submission", submissions,
                  std::uint64_t{ 0 });
}

template<>
template<>
void render_vulkan_texture_upload_registry_object::test<6>()
{
    using namespace LLRenderContract;
    using namespace LLRenderVulkanTextureUpload;

    const TextureUploadCase upload_case = makeTextureUploadCase();
    const TextureUploadFixture fixture = makeTextureUploadFixture();
    const StreamingUploadHandles& handles = upload_case.mInputs.mHandles;
    constexpr VkImageUsageFlags streamed_usage =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT;
    std::array<std::uint8_t, SCREEN_TRIANGLE_BYTE_SIZE> screen_bytes{};
    std::memcpy(screen_bytes.data(), fixture.mScreenTriangle.data(),
                screen_bytes.size());
    std::array<std::uint8_t, STAGING_BYTE_SIZE> staging_bytes{};
    staging_bytes.fill(0xa7);
    const auto staging_before = staging_bytes;
    std::array<std::uint8_t, READBACK_BYTE_SIZE> readback_bytes{};

    BufferBinding screen = completeBufferBinding(
        0x800, screen_bytes.data(), screen_bytes.size(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    screen.mHasFixtureBytes = true;
    screen.mFixtureBytes = screen_bytes;
    ImageBinding old_image = completeImageBinding(
        0x810, { 8, 4 }, { 32, 16 }, 2, 3, streamed_usage);
    old_image.mHasPreExecutionMipSnapshot = true;
    old_image.mPreExecutionMipRGBA8 = fixture.mOldMipRGBA8;
    const ImageBinding replacement = completeImageBinding(
        0x820, { 8, 4 }, { 32, 16 }, 2, 3, streamed_usage);
    const ImageBinding output = completeImageBinding(
        0x830, { 4, 2 }, { 4, 2 }, 0, 1,
        streamed_usage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    SamplerBinding sampler;
    sampler.mSampler = fakeHandle<VkSampler>(0x840);
    sampler.mOwnershipToken = OWNER_TOKEN + 1;
    const PipelineBinding pipeline = completePipelineBinding(output.mView);
    TransferResources transfer{
        completeBufferBinding(0x850, staging_bytes.data(), staging_bytes.size(),
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
        completeBufferBinding(0x860, readback_bytes.data(), readback_bytes.size(),
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT)
    };
    LifecycleLedger lifecycle{ handles.mOldImage, TEXTURE_UPLOAD_PRIOR_REVISION };
    const LifecycleLedger lifecycle_before = lifecycle;
    Registry registry;
    ensure("ownership fixture screen registers",
           registry.addScreenTriangle(handles.mScreenTriangle, screen));
    ensure("ownership fixture images register",
           registry.addImageGenerations(handles.mOldImage, old_image,
                                        handles.mReplacementImage, replacement));
    ensure("ownership fixture output registers",
           registry.addOutput(handles.mOutput, output));
    ensure("nonzero mismatched sampler owner registers",
           registry.addSampler(handles.mSampler, sampler));
    ensure("ownership fixture pipeline registers",
           registry.addPipeline(handles.mPipeline, pipeline));
    ensure("ownership fixture transfer registers",
           registry.addTransferResources(transfer));
    ensure("ownership fixture lifecycle registers", registry.addLifecycle(&lifecycle));

    std::uint64_t recording_attempts = 0;
    std::uint64_t submissions = 0;
    ExecutionContext context;
    context.mDevice = fakeHandle<VkDevice>(0x870);
    context.mCommandPool = fakeHandle<VkCommandPool>(0x871);
    context.mCommandBuffer = fakeHandle<VkCommandBuffer>(0x872);
    context.mQueue = fakeHandle<VkQueue>(0x873);
    context.mOwnershipToken = OWNER_TOKEN;
    context.mQueueFamilyIndex = 1;
    context.mQueueFamilyFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
    context.mQueueCount = 1;
    context.mCommandPoolQueueFamilyIndex = 1;
    context.mCommandPoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    context.mCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    context.mRecordingAttemptCount = &recording_attempts;
    context.mSubmissionCount = &submissions;
    context.mRequiredVertexShaderIdentity[0] = 1;
    context.mRequiredFragmentShaderIdentity[0] = 2;
    ExecutionResult result = makeTextureUploadArtifact();
    result.mSampledRGBA8 = { 0x42 };
    const ExecutionResult result_before = result;
    std::string error;
    ensure("mismatched native ownership is rejected",
           !execute(upload_case.mFrame, registry, context, result, &error));
    ensure_equals("native ownership mismatch is identified", error,
                  std::string("native resources do not share the execution context owner"));
    ensure("ownership rejection leaves staging untouched",
           staging_bytes == staging_before);
    ensure("ownership rejection leaves lifecycle untouched",
           lifecycle == lifecycle_before);
    ensure("ownership rejection leaves caller result untouched",
           result == result_before);
    ensure_equals("ownership rejection performs no recording", recording_attempts,
                  std::uint64_t{ 0 });
    ensure_equals("ownership rejection performs no submission", submissions,
                  std::uint64_t{ 0 });
}

} // namespace tut
