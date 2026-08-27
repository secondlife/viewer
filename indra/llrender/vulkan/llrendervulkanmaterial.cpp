/**
 * @file llrendervulkanmaterial.cpp
 * @brief Vulkan replay of the canonical Stage 12 material packet.
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

#include "llrendervulkanmaterial.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <type_traits>
#include <utility>

namespace LLRenderVulkanMaterial
{

bool validMaterialDescriptorSetPair(const std::array<VkDescriptorSet, 2>& descriptor_sets) noexcept
{
    return descriptor_sets[0] != VK_NULL_HANDLE && descriptor_sets[1] != VK_NULL_HANDLE;
}

namespace
{

constexpr VkImageUsageFlags MATERIAL_SOURCE_USAGE =
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
constexpr VkImageUsageFlags MATERIAL_COLOR_USAGE = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                          VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                          VK_IMAGE_USAGE_SAMPLED_BIT |
                                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
constexpr VkImageUsageFlags MATERIAL_DEPTH_USAGE = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                          VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
constexpr VkColorComponentFlags MATERIAL_COLOR_WRITE_MASK = VK_COLOR_COMPONENT_R_BIT |
                                                                    VK_COLOR_COMPONENT_G_BIT |
                                                                    VK_COLOR_COMPONENT_B_BIT |
                                                                    VK_COLOR_COMPONENT_A_BIT;

template<typename Entry, typename Handle>
bool hasIndex(const std::vector<Entry>& entries, Handle handle)
{
    return std::any_of(entries.begin(), entries.end(),
                       [handle](const Entry& entry) { return entry.mHandle.mIndex == handle.mIndex; });
}

template<typename Entry, typename Handle, typename Binding>
bool addEntry(std::vector<Entry>& entries, Handle handle, Binding binding, bool complete)
{
    if (!handle || !complete || hasIndex(entries, handle))
    {
        return false;
    }
    entries.push_back({ handle, std::move(binding) });
    return true;
}

template<typename Entry, typename Handle>
const auto* resolveEntry(const std::vector<Entry>& entries, Handle handle)
{
    const auto found = std::find_if(entries.begin(), entries.end(),
                                    [handle](const Entry& entry) { return entry.mHandle == handle; });
    return found == entries.end() ? nullptr : &found->mBinding;
}

bool nonzeroIdentity(const ShaderIdentityToken& identity)
{
    return std::any_of(identity.begin(), identity.end(), [](std::uint8_t byte) { return byte != 0; });
}

bool sameExtent(LLRenderContract::Extent2D left, LLRenderContract::Extent2D right)
{
    return left.mWidth == right.mWidth && left.mHeight == right.mHeight;
}

bool sameViewRange(const VkImageSubresourceRange& range, VkImageAspectFlags aspect,
                   std::uint32_t mip_levels)
{
    return range.aspectMask == aspect && range.baseMipLevel == 0 && range.levelCount == mip_levels &&
           range.baseArrayLayer == 0 && range.layerCount == 1;
}

bool completeImage(const ImageBinding& binding)
{
    return binding.mImage != VK_NULL_HANDLE && binding.mView != VK_NULL_HANDLE &&
           binding.mFormat != VK_FORMAT_UNDEFINED && binding.mExtent.mWidth != 0 &&
           binding.mExtent.mHeight != 0 && binding.mMipLevels != 0 && binding.mUsage != 0 &&
           binding.mAspect != 0 && binding.mViewRange.levelCount != 0 &&
           binding.mViewRange.layerCount != 0;
}

bool completePipeline(const PipelineBinding& binding)
{
    return !binding.mProgram.mName.empty() && binding.mPipeline != VK_NULL_HANDLE &&
           binding.mLayout != VK_NULL_HANDLE && binding.mRenderPass != VK_NULL_HANDLE &&
           binding.mFramebuffer != VK_NULL_HANDLE && validMaterialDescriptorSetPair(binding.mDescriptorSets) &&
           binding.mParameters.mBuffer != VK_NULL_HANDLE &&
           binding.mParameters.mSize != 0 &&
           binding.mParameters.mMemory != VK_NULL_HANDLE && binding.mParameters.mMapped != nullptr &&
           binding.mParameters.mAllocationSize != 0 && nonzeroIdentity(binding.mVertexShaderIdentity) &&
           nonzeroIdentity(binding.mFragmentShaderIdentity) && !binding.mVertexEntryPoint.empty() &&
           !binding.mFragmentEntryPoint.empty();
}

bool canonicalImage(const ImageBinding& image, VkFormat format,
                    LLRenderContract::Extent2D extent, std::uint32_t mip_levels,
                    VkImageUsageFlags required_usage, VkImageAspectFlags aspect,
                    VkImageLayout layout)
{
    return image.mFormat == format && sameExtent(image.mExtent, extent) &&
           image.mMipLevels == mip_levels && image.mUsage == required_usage &&
           image.mAspect == aspect && image.mLayout == layout &&
           sameViewRange(image.mViewRange, aspect, mip_levels);
}

bool canonicalSampler(const SamplerBinding& sampler)
{
    return sampler.mMinFilter == VK_FILTER_LINEAR && sampler.mMagFilter == VK_FILTER_LINEAR &&
           sampler.mMipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR &&
           sampler.mAddressU == VK_SAMPLER_ADDRESS_MODE_REPEAT &&
           sampler.mAddressV == VK_SAMPLER_ADDRESS_MODE_REPEAT &&
           sampler.mAddressW == VK_SAMPLER_ADDRESS_MODE_REPEAT && sampler.mMipLodBias == 0.f &&
           sampler.mAnisotropyEnable == VK_TRUE && sampler.mMaxAnisotropy == 8.f &&
           sampler.mCompareEnable == VK_FALSE && sampler.mCompareOp == VK_COMPARE_OP_ALWAYS &&
           sampler.mMinLod == 0.f && sampler.mMaxLod == 2.f &&
           sampler.mBorderColor == VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK &&
           sampler.mUnnormalizedCoordinates == VK_FALSE;
}

bool canonicalSubpassDependencies(const PipelineBinding& pipeline)
{
    constexpr VkPipelineStageFlags ATTACHMENT_STAGES =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    constexpr VkAccessFlags ATTACHMENT_ACCESS =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    constexpr VkPipelineStageFlags FINAL_STAGES =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    constexpr VkAccessFlags FINAL_ACCESS =
        VK_ACCESS_SHADER_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    const SubpassDependencyState& incoming = pipeline.mSubpassDependencies[0];
    const SubpassDependencyState& outgoing = pipeline.mSubpassDependencies[1];
    return incoming.mSourceSubpass == VK_SUBPASS_EXTERNAL && incoming.mDestinationSubpass == 0 &&
           incoming.mSourceStages == ATTACHMENT_STAGES &&
           incoming.mDestinationStages == ATTACHMENT_STAGES && incoming.mSourceAccess == 0 &&
           incoming.mDestinationAccess == ATTACHMENT_ACCESS && incoming.mFlags == 0 &&
           outgoing.mSourceSubpass == 0 && outgoing.mDestinationSubpass == VK_SUBPASS_EXTERNAL &&
           outgoing.mSourceStages == ATTACHMENT_STAGES && outgoing.mDestinationStages == FINAL_STAGES &&
           outgoing.mSourceAccess == ATTACHMENT_ACCESS && outgoing.mDestinationAccess == FINAL_ACCESS &&
           outgoing.mFlags == 0;
}

bool canonicalVertexState(const PipelineBinding& pipeline)
{
    constexpr std::array<std::uint32_t, 7> STRIDES{ 16, 16, 8, 4, 16, 8, 8 };
    constexpr std::array<VkFormat, 7> FORMATS{
        VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT
    };

    for (std::size_t index = 0; index < STRIDES.size(); ++index)
    {
        const VertexBindingState& binding = pipeline.mVertexBindings[index];
        const VertexAttributeState& attribute = pipeline.mVertexAttributes[index];
        if (binding.mBinding != index || binding.mStride != STRIDES[index] ||
            binding.mInputRate != VK_VERTEX_INPUT_RATE_VERTEX || attribute.mLocation != index ||
            attribute.mBinding != index || attribute.mFormat != FORMATS[index] ||
            attribute.mOffset != 0)
        {
            return false;
        }
    }
    return true;
}

bool canonicalRasterState(const PipelineBinding& pipeline)
{
    const RasterState& raster = pipeline.mRaster;
    const MultisampleState& multisample = pipeline.mMultisample;
    const DepthStencilState& depth = pipeline.mDepthStencil;

    // Vulkan's framebuffer-area equation accounts for the downward native Y
    // axis. The negative viewport restores GL's orientation, so the contract's
    // counter-clockwise front face remains counter-clockwise here.
    if (raster.mTopology != VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ||
        raster.mPrimitiveRestartEnable != VK_FALSE || raster.mDepthClampEnable != VK_FALSE ||
        raster.mRasterizerDiscardEnable != VK_FALSE || raster.mPolygonMode != VK_POLYGON_MODE_FILL ||
        raster.mCullMode != VK_CULL_MODE_BACK_BIT || raster.mFrontFace != VK_FRONT_FACE_COUNTER_CLOCKWISE ||
        raster.mDepthBiasEnable != VK_FALSE || raster.mDepthBiasConstantFactor != 0.f ||
        raster.mDepthBiasClamp != 0.f || raster.mDepthBiasSlopeFactor != 0.f ||
        raster.mLineWidth != 1.f ||
        multisample.mRasterizationSamples != VK_SAMPLE_COUNT_1_BIT ||
        multisample.mSampleShadingEnable != VK_FALSE ||
        multisample.mMinSampleShading != 0.f || multisample.mSampleMask != 0xffffffffu ||
        multisample.mAlphaToCoverageEnable != VK_FALSE || multisample.mAlphaToOneEnable != VK_FALSE ||
        depth.mDepthTestEnable != VK_TRUE || depth.mDepthWriteEnable != VK_TRUE ||
        depth.mDepthCompareOp != VK_COMPARE_OP_LESS_OR_EQUAL ||
        depth.mDepthBoundsTestEnable != VK_FALSE || depth.mStencilTestEnable != VK_FALSE ||
        depth.mFront.failOp != VK_STENCIL_OP_KEEP || depth.mFront.passOp != VK_STENCIL_OP_KEEP ||
        depth.mFront.depthFailOp != VK_STENCIL_OP_KEEP ||
        depth.mFront.compareOp != VK_COMPARE_OP_NEVER || depth.mFront.compareMask != 0 ||
        depth.mFront.writeMask != 0 || depth.mFront.reference != 0 ||
        depth.mBack.failOp != VK_STENCIL_OP_KEEP || depth.mBack.passOp != VK_STENCIL_OP_KEEP ||
        depth.mBack.depthFailOp != VK_STENCIL_OP_KEEP ||
        depth.mBack.compareOp != VK_COMPARE_OP_NEVER || depth.mBack.compareMask != 0 ||
        depth.mBack.writeMask != 0 || depth.mBack.reference != 0 ||
        depth.mMinDepthBounds != 0.f || depth.mMaxDepthBounds != 1.f ||
        pipeline.mLogicOpEnable != VK_FALSE || pipeline.mLogicOp != VK_LOGIC_OP_COPY ||
        pipeline.mBlendConstants != std::array<float, 4>{} ||
        pipeline.mDynamicViewport != VK_TRUE ||
        pipeline.mDynamicScissor != VK_TRUE)
    {
        return false;
    }

    constexpr std::array<VkFormat, 3> FORMATS{ VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_FORMAT_R16G16B16A16_UNORM };
    for (std::size_t index = 0; index < FORMATS.size(); ++index)
    {
        const ColorTargetState& target = pipeline.mColorTargets[index];
        if (target.mFormat != FORMATS[index] || target.mBlendEnable != VK_FALSE ||
            target.mSrcColorBlendFactor != VK_BLEND_FACTOR_ONE ||
            target.mDstColorBlendFactor != VK_BLEND_FACTOR_ZERO ||
            target.mColorBlendOp != VK_BLEND_OP_ADD ||
            target.mSrcAlphaBlendFactor != VK_BLEND_FACTOR_ONE ||
            target.mDstAlphaBlendFactor != VK_BLEND_FACTOR_ZERO ||
            target.mAlphaBlendOp != VK_BLEND_OP_ADD ||
            target.mWriteMask != MATERIAL_COLOR_WRITE_MASK)
        {
            return false;
        }
    }
    return true;
}

bool canonicalPipelineResources(const PipelineBinding& pipeline,
                                const std::array<const ImageBinding*, 3>& sources,
                                const SamplerBinding& sampler,
                                const std::array<const ImageBinding*, 3>& colors,
                                const ImageBinding& depth,
                                const ExecutionContext& context)
{
    if (!sameExtent(pipeline.mExtent,
                    { LLRenderContract::MATERIAL_FRAME_WIDTH, LLRenderContract::MATERIAL_FRAME_HEIGHT }) ||
        !validMaterialDescriptorSetPair(pipeline.mDescriptorSets) ||
        pipeline.mVertexShaderIdentity != context.mRequiredVertexShaderIdentity ||
        pipeline.mFragmentShaderIdentity != context.mRequiredFragmentShaderIdentity ||
        pipeline.mVertexEntryPoint != "main" || pipeline.mFragmentEntryPoint != "main" ||
        pipeline.mParameterDescriptor.mSet != 0 || pipeline.mParameterDescriptor.mBinding != 0 ||
        pipeline.mParameterDescriptor.mType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
        pipeline.mParameterDescriptor.mStages !=
            (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) ||
        pipeline.mParameterDescriptor.mBuffer != pipeline.mParameters.mBuffer ||
        pipeline.mParameterDescriptor.mOffset != 0 ||
        pipeline.mParameterDescriptor.mRange != MATERIAL_PARAMETER_SIZE ||
        pipeline.mParameters.mSize != MATERIAL_PARAMETER_SIZE ||
        pipeline.mParameters.mUsage != VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ||
        pipeline.mParameters.mDescriptorOffset != 0 ||
        pipeline.mParameters.mDescriptorRange != MATERIAL_PARAMETER_SIZE ||
        pipeline.mParameters.mAllocationSize < MATERIAL_PARAMETER_SIZE ||
        (pipeline.mParameters.mMemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
    {
        return false;
    }

    for (std::size_t index = 0; index < sources.size(); ++index)
    {
        const SampledDescriptorBinding& descriptor = pipeline.mSampledDescriptors[index];
        if (descriptor.mSet != 1 || descriptor.mBinding != index ||
            descriptor.mType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
            descriptor.mStages != VK_SHADER_STAGE_FRAGMENT_BIT ||
            descriptor.mView != sources[index]->mView || descriptor.mSampler != sampler.mSampler ||
            descriptor.mLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ||
            pipeline.mColorViews[index] != colors[index]->mView ||
            pipeline.mColorLoadOps[index] != VK_ATTACHMENT_LOAD_OP_CLEAR ||
            pipeline.mColorStoreOps[index] != VK_ATTACHMENT_STORE_OP_STORE)
        {
            return false;
        }
    }

    return pipeline.mDepthView == depth.mView && pipeline.mDepthFormat == VK_FORMAT_D32_SFLOAT &&
           pipeline.mDepthLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD &&
           pipeline.mDepthStoreOp == VK_ATTACHMENT_STORE_OP_STORE &&
           pipeline.mColorInitialLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
           pipeline.mColorFinalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
           pipeline.mDepthInitialLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
           pipeline.mDepthFinalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
           canonicalSubpassDependencies(pipeline);
}

template<std::size_t Size>
bool distinctImages(const std::array<const ImageBinding*, Size>& images)
{
    for (std::size_t left = 0; left < images.size(); ++left)
    {
        for (std::size_t right = left + 1; right < images.size(); ++right)
        {
            if (images[left]->mImage == images[right]->mImage ||
                images[left]->mView == images[right]->mView)
            {
                return false;
            }
        }
    }
    return true;
}

struct Prepared
{
    LLRenderContract::MaterialInputs mInputs;
    const BufferBinding* mVertex = nullptr;
    const BufferBinding* mIndex = nullptr;
    std::array<const ImageBinding*, 3> mSources{};
    std::array<const ImageBinding*, 3> mColors{};
    const ImageBinding* mDepth = nullptr;
    const SamplerBinding* mSampler = nullptr;
    const PipelineBinding* mPipeline = nullptr;
    const LLRenderContract::DrawIndexed* mDraw = nullptr;
};

std::optional<Prepared> prepare(const LLRenderContract::FrameSnapshot& frame,
                                const Registry& registry, const ExecutionContext& context,
                                std::string& error)
{
    const auto inputs = LLRenderContract::decodeMaterialFrame(frame);
    if (!inputs)
    {
        error = "packet is not the canonical material frame";
        return std::nullopt;
    }
    if (context.mDevice == VK_NULL_HANDLE || context.mCommandBuffer == VK_NULL_HANDLE ||
        context.mQueue == VK_NULL_HANDLE || !context.mRecordingAttemptCount ||
        !context.mSubmissionCount || context.mRecordingAttemptCount == context.mSubmissionCount ||
        !nonzeroIdentity(context.mRequiredVertexShaderIdentity) ||
        !nonzeroIdentity(context.mRequiredFragmentShaderIdentity))
    {
        error = "execution context is incomplete";
        return std::nullopt;
    }

    Prepared result;
    result.mInputs = *inputs;
    const LLRenderContract::MaterialHandles& handles = inputs->mHandles;
    result.mVertex = registry.resolve(handles.mVertexBuffer);
    result.mIndex = registry.resolve(handles.mIndexBuffer);
    result.mSources = { registry.resolve(handles.mDiffuse), registry.resolve(handles.mNormal),
                        registry.resolve(handles.mSpecular) };
    result.mColors = { registry.resolve(handles.mGBuffer0), registry.resolve(handles.mGBuffer1),
                       registry.resolve(handles.mGBuffer2) };
    result.mDepth = registry.resolve(handles.mDepth);
    result.mSampler = registry.resolve(handles.mSampler);
    result.mPipeline = registry.resolve(handles.mPipeline, frame.mPipelines.front().mProgram);
    result.mDraw = &std::get<LLRenderContract::DrawIndexed>(frame.mPasses.front().mDraws.front());

    if (!result.mVertex || !result.mIndex ||
        std::any_of(result.mSources.begin(), result.mSources.end(), [](const auto* value) { return !value; }) ||
        std::any_of(result.mColors.begin(), result.mColors.end(), [](const auto* value) { return !value; }) ||
        !result.mDepth || !result.mSampler || !result.mPipeline)
    {
        error = "registry cannot resolve an exact live resource generation";
        return std::nullopt;
    }

    if (result.mVertex->mSize != LLRenderContract::MATERIAL_VERTEX_BUFFER_SIZE ||
        result.mVertex->mUsage != VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ||
        result.mIndex->mSize != LLRenderContract::MATERIAL_INDEX_BUFFER_SIZE ||
        result.mIndex->mUsage != VK_BUFFER_USAGE_INDEX_BUFFER_BIT ||
        result.mVertex->mBuffer == result.mIndex->mBuffer ||
        !result.mIndex->mHasTranslatedIndices ||
        result.mIndex->mTranslatedIndices != MATERIAL_VULKAN_INDICES)
    {
        error = "registry buffer metadata does not match the material packet";
        return std::nullopt;
    }

    constexpr LLRenderContract::Extent2D TEXTURE_EXTENT{
        LLRenderContract::MATERIAL_TEXTURE_WIDTH, LLRenderContract::MATERIAL_TEXTURE_HEIGHT
    };
    constexpr LLRenderContract::Extent2D FRAME_EXTENT{
        LLRenderContract::MATERIAL_FRAME_WIDTH, LLRenderContract::MATERIAL_FRAME_HEIGHT
    };
    for (const ImageBinding* source : result.mSources)
    {
        if (!canonicalImage(*source, VK_FORMAT_R8G8B8A8_UNORM, TEXTURE_EXTENT,
                            LLRenderContract::MATERIAL_TEXTURE_MIP_LEVELS, MATERIAL_SOURCE_USAGE,
                            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
        {
            error = "source image metadata does not match the material packet";
            return std::nullopt;
        }
    }
    constexpr std::array<VkFormat, 3> COLOR_FORMATS{ VK_FORMAT_R8G8B8A8_UNORM,
                                                     VK_FORMAT_R8G8B8A8_UNORM,
                                                     VK_FORMAT_R16G16B16A16_UNORM };
    for (std::size_t index = 0; index < result.mColors.size(); ++index)
    {
        if (!canonicalImage(*result.mColors[index], COLOR_FORMATS[index], FRAME_EXTENT, 1,
                            MATERIAL_COLOR_USAGE, VK_IMAGE_ASPECT_COLOR_BIT,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL))
        {
            error = "color attachment metadata does not match the material packet";
            return std::nullopt;
        }
    }
    if (!canonicalImage(*result.mDepth, VK_FORMAT_D32_SFLOAT, FRAME_EXTENT, 1,
                        MATERIAL_DEPTH_USAGE, VK_IMAGE_ASPECT_DEPTH_BIT,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL))
    {
        error = "depth attachment is not the fixed D32 material substitution";
        return std::nullopt;
    }

    std::array<const ImageBinding*, 7> all_images{ result.mSources[0], result.mSources[1],
                                                   result.mSources[2], result.mColors[0],
                                                   result.mColors[1], result.mColors[2],
                                                   result.mDepth };
    if (!distinctImages(all_images))
    {
        error = "material images or views alias";
        return std::nullopt;
    }
    if (!canonicalSampler(*result.mSampler))
    {
        error = "sampler metadata does not match the material packet";
        return std::nullopt;
    }

    const PipelineBinding& pipeline = *result.mPipeline;
    if (!canonicalPipelineResources(pipeline, result.mSources, *result.mSampler,
                                    result.mColors, *result.mDepth, context))
    {
        error = "pipeline resources or shader identity do not match the material packet";
        return std::nullopt;
    }
    if (!canonicalVertexState(pipeline) || pipeline.mIndexType != VK_INDEX_TYPE_UINT16)
    {
        error = "pipeline vertex or index state does not match the material packet";
        return std::nullopt;
    }
    if (!canonicalRasterState(pipeline))
    {
        error = "pipeline raster, depth, multisample, or color state does not match the material packet";
        return std::nullopt;
    }

    return result;
}

} // namespace

bool Registry::addBuffer(LLRenderContract::BufferHandle handle, BufferBinding binding)
{
    const bool complete = binding.mBuffer != VK_NULL_HANDLE && binding.mSize != 0 && binding.mUsage != 0;
    return addEntry(mBuffers, handle, std::move(binding), complete);
}

bool Registry::addImage(LLRenderContract::ImageHandle handle, ImageBinding binding)
{
    const bool complete = completeImage(binding);
    return addEntry(mImages, handle, std::move(binding), complete);
}

bool Registry::addSampler(LLRenderContract::SamplerHandle handle, SamplerBinding binding)
{
    const bool complete = binding.mSampler != VK_NULL_HANDLE;
    return addEntry(mSamplers, handle, std::move(binding), complete);
}

bool Registry::addPipeline(LLRenderContract::PipelineHandle handle, PipelineBinding binding)
{
    const bool complete = completePipeline(binding);
    return addEntry(mPipelines, handle, std::move(binding), complete);
}

const BufferBinding* Registry::resolve(LLRenderContract::BufferHandle handle) const
{
    return resolveEntry(mBuffers, handle);
}

const ImageBinding* Registry::resolve(LLRenderContract::ImageHandle handle) const
{
    return resolveEntry(mImages, handle);
}

const SamplerBinding* Registry::resolve(LLRenderContract::SamplerHandle handle) const
{
    return resolveEntry(mSamplers, handle);
}

const PipelineBinding* Registry::resolve(LLRenderContract::PipelineHandle handle,
                                         const LLRenderContract::ShaderProgramKey& program) const
{
    const PipelineBinding* binding = resolveEntry(mPipelines, handle);
    if (!binding || binding->mProgram.mName != program.mName ||
        binding->mProgram.mVariant != program.mVariant)
    {
        return nullptr;
    }
    return binding;
}

bool execute(const LLRenderContract::FrameSnapshot& frame, const Registry& registry,
             const ExecutionContext& context, std::string& error)
{
    const auto prepared = prepare(frame, registry, context, error);
    if (!prepared)
    {
        return false;
    }

    static_assert(std::is_trivially_copyable_v<LLRenderContract::MaterialParameters>);
    std::memcpy(prepared->mPipeline->mParameters.mMapped, &prepared->mInputs.mParameters,
                MATERIAL_PARAMETER_SIZE);

    if ((prepared->mPipeline->mParameters.mMemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
        VkMappedMemoryRange mapped_range{};
        mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mapped_range.memory = prepared->mPipeline->mParameters.mMemory;
        mapped_range.offset = 0;
        mapped_range.size = VK_WHOLE_SIZE;
        const VkResult flush_result = vkFlushMappedMemoryRanges(context.mDevice, 1, &mapped_range);
        if (flush_result != VK_SUCCESS)
        {
            error = "vkFlushMappedMemoryRanges failed";
            return false;
        }
    }

    ++*context.mRecordingAttemptCount;
    VkResult result = vkResetCommandBuffer(context.mCommandBuffer, 0);
    if (result != VK_SUCCESS)
    {
        error = "vkResetCommandBuffer failed";
        return false;
    }

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(context.mCommandBuffer, &begin);
    if (result != VK_SUCCESS)
    {
        error = "vkBeginCommandBuffer failed";
        return false;
    }

    VkBufferMemoryBarrier parameter_barrier{};
    parameter_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    parameter_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    parameter_barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
    parameter_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    parameter_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    parameter_barrier.buffer = prepared->mPipeline->mParameters.mBuffer;
    parameter_barrier.offset = 0;
    parameter_barrier.size = MATERIAL_PARAMETER_SIZE;
    vkCmdPipelineBarrier(context.mCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                         VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 1, &parameter_barrier, 0, nullptr);

    std::array<VkImageMemoryBarrier, 3> color_barriers{};
    for (std::size_t index = 0; index < color_barriers.size(); ++index)
    {
        VkImageMemoryBarrier& barrier = color_barriers[index];
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        // The diagnostic's sentinel snapshot leaves each target in transfer-src.
        // The contract's Undefined state means discard, so no source access is
        // carried into the attachment transition even though the live layout is
        // recorded truthfully.
        barrier.oldLayout = prepared->mColors[index]->mLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = prepared->mColors[index]->mImage;
        barrier.subresourceRange = prepared->mColors[index]->mViewRange;
    }
    vkCmdPipelineBarrier(context.mCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr,
                         0, nullptr, static_cast<std::uint32_t>(color_barriers.size()),
                         color_barriers.data());

    std::array<VkClearValue, 3> clear_values{};
    VkRenderPassBeginInfo render_pass{};
    render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass.renderPass = prepared->mPipeline->mRenderPass;
    render_pass.framebuffer = prepared->mPipeline->mFramebuffer;
    render_pass.renderArea.extent = { LLRenderContract::MATERIAL_FRAME_WIDTH,
                                     LLRenderContract::MATERIAL_FRAME_HEIGHT };
    render_pass.clearValueCount = static_cast<std::uint32_t>(clear_values.size());
    render_pass.pClearValues = clear_values.data();
    vkCmdBeginRenderPass(context.mCommandBuffer, &render_pass, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{ 0.f, static_cast<float>(LLRenderContract::MATERIAL_FRAME_HEIGHT),
                         static_cast<float>(LLRenderContract::MATERIAL_FRAME_WIDTH),
                         -static_cast<float>(LLRenderContract::MATERIAL_FRAME_HEIGHT), 0.f, 1.f };
    VkRect2D scissor{ { 0, 0 },
                      { LLRenderContract::MATERIAL_FRAME_WIDTH,
                        LLRenderContract::MATERIAL_FRAME_HEIGHT } };
    vkCmdSetViewport(context.mCommandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(context.mCommandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(context.mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      prepared->mPipeline->mPipeline);
    vkCmdBindDescriptorSets(context.mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            prepared->mPipeline->mLayout, 0,
                            static_cast<std::uint32_t>(prepared->mPipeline->mDescriptorSets.size()),
                            prepared->mPipeline->mDescriptorSets.data(), 0, nullptr);

    std::array<VkBuffer, 7> vertex_buffers{};
    std::array<VkDeviceSize, 7> vertex_offsets{};
    for (std::size_t index = 0; index < vertex_buffers.size(); ++index)
    {
        vertex_buffers[index] = prepared->mVertex->mBuffer;
        vertex_offsets[index] = prepared->mDraw->mResources.mVertexBuffers[index].mOffset;
    }
    vkCmdBindVertexBuffers(context.mCommandBuffer, 0,
                           static_cast<std::uint32_t>(vertex_buffers.size()),
                           vertex_buffers.data(), vertex_offsets.data());
    vkCmdBindIndexBuffer(context.mCommandBuffer, prepared->mIndex->mBuffer, 0,
                         VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(context.mCommandBuffer, prepared->mDraw->mIndexCount,
                     prepared->mDraw->mInstanceCount, prepared->mDraw->mFirstIndex,
                     prepared->mDraw->mBaseVertex, prepared->mDraw->mFirstInstance);
    vkCmdEndRenderPass(context.mCommandBuffer);

    result = vkEndCommandBuffer(context.mCommandBuffer);
    if (result != VK_SUCCESS)
    {
        error = "vkEndCommandBuffer failed";
        return false;
    }

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &context.mCommandBuffer;
    result = vkQueueSubmit(context.mQueue, 1, &submit, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
    {
        error = "vkQueueSubmit failed";
        return false;
    }
    ++*context.mSubmissionCount;
    result = vkQueueWaitIdle(context.mQueue);
    if (result != VK_SUCCESS)
    {
        error = "vkQueueWaitIdle failed";
        return false;
    }

    error.clear();
    return true;
}

} // namespace LLRenderVulkanMaterial
