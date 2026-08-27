/**
 * @file llrendervulkanmaterial.h
 * @brief Narrow Vulkan registry and executor for the Stage 12 material packet.
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

#ifndef LL_LLRENDERVULKANMATERIAL_H
#define LL_LLRENDERVULKANMATERIAL_H

#include "llmaterialcontract.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace LLRenderVulkanMaterial
{

inline constexpr std::array<std::uint16_t, 6> MATERIAL_VULKAN_INDICES{ 2, 0, 1, 3, 0, 2 };
inline constexpr std::size_t                  MATERIAL_PARAMETER_SIZE = sizeof(LLRenderContract::MaterialParameters);

struct BufferBinding
{
    VkBuffer           mBuffer = VK_NULL_HANDLE;
    VkDeviceSize       mSize   = 0;
    VkBufferUsageFlags mUsage  = 0;

    // Set only for the immutable diagnostic index allocation. The cyclic
    // rotation makes Vulkan's first provoking vertex equal GL's last vertex.
    bool                              mHasTranslatedIndices = false;
    std::array<std::uint16_t, 6>      mTranslatedIndices{};
};

struct ImageBinding
{
    VkImage                       mImage = VK_NULL_HANDLE;
    VkImageView                   mView  = VK_NULL_HANDLE;
    VkFormat                      mFormat = VK_FORMAT_UNDEFINED;
    LLRenderContract::Extent2D    mExtent;
    std::uint32_t                 mMipLevels = 0;
    VkImageUsageFlags             mUsage = 0;
    VkImageAspectFlags            mAspect = 0;
    VkImageLayout                 mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageSubresourceRange       mViewRange{};
};

struct SamplerBinding
{
    VkSampler             mSampler = VK_NULL_HANDLE;
    VkFilter              mMinFilter = VK_FILTER_NEAREST;
    VkFilter              mMagFilter = VK_FILTER_NEAREST;
    VkSamplerMipmapMode   mMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode  mAddressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSamplerAddressMode  mAddressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSamplerAddressMode  mAddressW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    float                 mMipLodBias = 0.f;
    VkBool32              mAnisotropyEnable = VK_FALSE;
    float                 mMaxAnisotropy = 1.f;
    VkBool32              mCompareEnable = VK_FALSE;
    VkCompareOp           mCompareOp = VK_COMPARE_OP_ALWAYS;
    float                 mMinLod = 0.f;
    float                 mMaxLod = 0.f;
    VkBorderColor         mBorderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    VkBool32              mUnnormalizedCoordinates = VK_FALSE;
};

// mMapped points at byte zero of mMemory. The diagnostic binds mBuffer at
// memory offset zero and keeps the whole allocation mapped for the run.
struct ParameterBinding
{
    VkBuffer              mBuffer = VK_NULL_HANDLE;
    VkDeviceSize          mSize = 0;
    VkBufferUsageFlags    mUsage = 0;
    VkDeviceMemory        mMemory = VK_NULL_HANDLE;
    void*                 mMapped = nullptr;
    VkDeviceSize          mAllocationSize = 0;
    VkDeviceSize          mDescriptorOffset = 0;
    VkDeviceSize          mDescriptorRange = 0;
    VkMemoryPropertyFlags mMemoryProperties = 0;
};

struct UniformDescriptorBinding
{
    std::uint32_t mSet = 0;
    std::uint32_t mBinding = 0;
    VkDescriptorType mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkShaderStageFlags mStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkBuffer      mBuffer = VK_NULL_HANDLE;
    VkDeviceSize  mOffset = 0;
    VkDeviceSize  mRange = 0;
};

struct SampledDescriptorBinding
{
    std::uint32_t mSet = 0;
    std::uint32_t mBinding = 0;
    VkDescriptorType mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkShaderStageFlags mStages = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkImageView   mView = VK_NULL_HANDLE;
    VkSampler     mSampler = VK_NULL_HANDLE;
    VkImageLayout mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

struct VertexBindingState
{
    std::uint32_t     mBinding = 0;
    std::uint32_t     mStride = 0;
    VkVertexInputRate mInputRate = VK_VERTEX_INPUT_RATE_VERTEX;
};

struct VertexAttributeState
{
    std::uint32_t mLocation = 0;
    std::uint32_t mBinding = 0;
    VkFormat      mFormat = VK_FORMAT_UNDEFINED;
    std::uint32_t mOffset = 0;
};

struct RasterState
{
    VkPrimitiveTopology mTopology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    VkBool32             mPrimitiveRestartEnable = VK_FALSE;
    VkBool32             mDepthClampEnable = VK_FALSE;
    VkBool32             mRasterizerDiscardEnable = VK_FALSE;
    VkPolygonMode        mPolygonMode = VK_POLYGON_MODE_MAX_ENUM;
    VkCullModeFlags      mCullMode = VK_CULL_MODE_NONE;
    VkFrontFace          mFrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkBool32             mDepthBiasEnable = VK_FALSE;
    float                mDepthBiasConstantFactor = 0.f;
    float                mDepthBiasClamp = 0.f;
    float                mDepthBiasSlopeFactor = 0.f;
    float                mLineWidth = 0.f;
};

struct MultisampleState
{
    VkSampleCountFlagBits mRasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkBool32              mSampleShadingEnable = VK_FALSE;
    float                 mMinSampleShading = 0.f;
    VkSampleMask          mSampleMask = 0xffffffffu;
    VkBool32              mAlphaToCoverageEnable = VK_FALSE;
    VkBool32              mAlphaToOneEnable = VK_FALSE;
};

struct DepthStencilState
{
    VkBool32    mDepthTestEnable = VK_FALSE;
    VkBool32    mDepthWriteEnable = VK_FALSE;
    VkCompareOp mDepthCompareOp = VK_COMPARE_OP_ALWAYS;
    VkBool32    mDepthBoundsTestEnable = VK_FALSE;
    VkBool32    mStencilTestEnable = VK_FALSE;
    VkStencilOpState mFront{};
    VkStencilOpState mBack{};
    float       mMinDepthBounds = 0.f;
    float       mMaxDepthBounds = 1.f;
};

struct ColorTargetState
{
    VkFormat              mFormat = VK_FORMAT_UNDEFINED;
    VkBool32              mBlendEnable = VK_FALSE;
    VkBlendFactor         mSrcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor         mDstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp             mColorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor         mSrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor         mDstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp             mAlphaBlendOp = VK_BLEND_OP_ADD;
    VkColorComponentFlags mWriteMask = 0;
};

struct SubpassDependencyState
{
    std::uint32_t        mSourceSubpass = VK_SUBPASS_EXTERNAL;
    std::uint32_t        mDestinationSubpass = VK_SUBPASS_EXTERNAL;
    VkPipelineStageFlags mSourceStages = 0;
    VkPipelineStageFlags mDestinationStages = 0;
    VkAccessFlags        mSourceAccess = 0;
    VkAccessFlags        mDestinationAccess = 0;
    VkDependencyFlags    mFlags = 0;
};

// SHA-256 of the validated SPIR-V module bytes. The runner records the token
// when it creates the immutable pipeline and supplies the required token again
// through ExecutionContext.
using ShaderIdentityToken = std::array<std::uint8_t, 32>;

struct PipelineBinding
{
    LLRenderContract::ShaderProgramKey mProgram;
    VkPipeline                         mPipeline = VK_NULL_HANDLE;
    VkPipelineLayout                   mLayout = VK_NULL_HANDLE;
    VkRenderPass                       mRenderPass = VK_NULL_HANDLE;
    VkFramebuffer                      mFramebuffer = VK_NULL_HANDLE;
    LLRenderContract::Extent2D         mExtent;

    std::array<VkDescriptorSet, 2>             mDescriptorSets{};
    UniformDescriptorBinding                   mParameterDescriptor;
    std::array<SampledDescriptorBinding, 3>    mSampledDescriptors{};
    ParameterBinding                           mParameters;

    std::array<VkImageView, 3>                 mColorViews{};
    VkImageView                                mDepthView = VK_NULL_HANDLE;
    VkFormat                                   mDepthFormat = VK_FORMAT_UNDEFINED;
    std::array<VkAttachmentLoadOp, 3>          mColorLoadOps{};
    std::array<VkAttachmentStoreOp, 3>         mColorStoreOps{};
    VkAttachmentLoadOp                         mDepthLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp                        mDepthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    std::array<SubpassDependencyState, 2>       mSubpassDependencies{};
    VkImageLayout                              mColorInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout                              mColorFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout                              mDepthInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout                              mDepthFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    std::array<VertexBindingState, 7>          mVertexBindings{};
    std::array<VertexAttributeState, 7>        mVertexAttributes{};
    VkIndexType                                mIndexType = VK_INDEX_TYPE_MAX_ENUM;
    RasterState                                mRaster;
    MultisampleState                           mMultisample;
    DepthStencilState                          mDepthStencil;
    std::array<ColorTargetState, 3>            mColorTargets{};
    VkBool32                                   mLogicOpEnable = VK_FALSE;
    VkLogicOp                                  mLogicOp = VK_LOGIC_OP_COPY;
    std::array<float, 4>                       mBlendConstants{};
    VkBool32                                   mDynamicViewport = VK_FALSE;
    VkBool32                                   mDynamicScissor = VK_FALSE;

    ShaderIdentityToken                       mVertexShaderIdentity{};
    ShaderIdentityToken                       mFragmentShaderIdentity{};
    std::string                               mVertexEntryPoint = "main";
    std::string                               mFragmentEntryPoint = "main";
};

// Vulkan non-dispatchable handle values are opaque and may compare equal.
// A canonical pair requires two non-null values, not two distinct values.
bool validMaterialDescriptorSetPair(const std::array<VkDescriptorSet, 2>& descriptor_sets) noexcept;

// The registry borrows immutable objects owned by one synchronous offscreen run.
class Registry
{
public:
    bool addBuffer(LLRenderContract::BufferHandle handle, BufferBinding binding);
    bool addImage(LLRenderContract::ImageHandle handle, ImageBinding binding);
    bool addSampler(LLRenderContract::SamplerHandle handle, SamplerBinding binding);
    bool addPipeline(LLRenderContract::PipelineHandle handle, PipelineBinding binding);

    const BufferBinding* resolve(LLRenderContract::BufferHandle handle) const;
    const ImageBinding* resolve(LLRenderContract::ImageHandle handle) const;
    const SamplerBinding* resolve(LLRenderContract::SamplerHandle handle) const;
    const PipelineBinding* resolve(LLRenderContract::PipelineHandle handle,
                                   const LLRenderContract::ShaderProgramKey& program) const;

private:
    template<typename HandleType, typename BindingType>
    struct Entry
    {
        HandleType  mHandle;
        BindingType mBinding;
    };

    std::vector<Entry<LLRenderContract::BufferHandle, BufferBinding>> mBuffers;
    std::vector<Entry<LLRenderContract::ImageHandle, ImageBinding>> mImages;
    std::vector<Entry<LLRenderContract::SamplerHandle, SamplerBinding>> mSamplers;
    std::vector<Entry<LLRenderContract::PipelineHandle, PipelineBinding>> mPipelines;
};

struct ExecutionContext
{
    VkDevice                mDevice = VK_NULL_HANDLE;
    VkCommandBuffer         mCommandBuffer = VK_NULL_HANDLE;
    VkQueue                 mQueue = VK_NULL_HANDLE;
    std::uint64_t*          mRecordingAttemptCount = nullptr;
    std::uint64_t*          mSubmissionCount = nullptr;
    ShaderIdentityToken     mRequiredVertexShaderIdentity{};
    ShaderIdentityToken     mRequiredFragmentShaderIdentity{};
};

// Performs all packet, handle, immutable metadata, and physical-resource
// checks before copying parameter bytes or attempting command recording.
bool execute(const LLRenderContract::FrameSnapshot& frame, const Registry& registry,
             const ExecutionContext& context, std::string& error);

} // namespace LLRenderVulkanMaterial

#endif // LL_LLRENDERVULKANMATERIAL_H
