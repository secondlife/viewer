/**
 * @file llrendervulkantextureupload.h
 * @brief Narrow Vulkan registry and executor for one streamed texture upload.
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

#ifndef LL_LLRENDERVULKANTEXTUREUPLOAD_H
#define LL_LLRENDERVULKANTEXTUREUPLOAD_H

#include "lltextureuploaddiagnostic.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace LLRenderVulkanTextureUpload
{

inline constexpr std::size_t SCREEN_TRIANGLE_BYTE_SIZE = 48;
inline constexpr std::size_t STAGING_BYTE_SIZE = LLRenderContract::TEXTURE_UPLOAD_SOURCE_BYTE_COUNT;
inline constexpr std::size_t READBACK_BYTE_SIZE =
    LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_COUNT + LLRenderContract::TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT;
inline constexpr std::size_t OUTPUT_READBACK_BYTE_OFFSET = LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_COUNT;

// A TopLeft, 36-byte-pitch packet is copied one tight row at a time into a
// BottomLeft Vulkan image. The four poison words never enter the image.
inline constexpr std::array<VkDeviceSize, LLRenderContract::TEXTURE_UPLOAD_RESIDENT_HEIGHT>
    UPLOAD_ROW_SOURCE_OFFSETS{ 108, 72, 36, 0 };

// SHA-256 of the validated SPIR-V module bytes.
using ShaderIdentityToken = std::array<std::uint8_t, 32>;

// Opaque, nonzero identity assigned by the standalone owner. Every native
// object participating in one execution must carry the device's same token.
using NativeOwnershipToken = std::uint64_t;

// mMapped addresses byte zero of mMemory and remains valid through execute().
// Each diagnostic allocation is dedicated, bound at offset zero, and mapped
// over the whole allocation when host access is required.
struct BufferBinding
{
    VkBuffer              mBuffer = VK_NULL_HANDLE;
    VkDeviceMemory        mMemory = VK_NULL_HANDLE;
    NativeOwnershipToken  mOwnershipToken = 0;
    void*                 mMapped = nullptr;
    VkDeviceSize          mSize = 0;
    VkDeviceSize          mAllocationSize = 0;
    VkDeviceSize          mMemoryOffset = 0;
    VkBufferCreateFlags   mCreateFlags = 0;
    VkBufferUsageFlags    mUsage = 0;
    VkSharingMode         mSharingMode = VK_SHARING_MODE_MAX_ENUM;
    VkMemoryPropertyFlags mMemoryProperties = 0;

    // Required only for the immutable 48-byte screen-triangle registration.
    bool                                               mHasFixtureBytes = false;
    std::array<std::uint8_t, SCREEN_TRIANGLE_BYTE_SIZE> mFixtureBytes{};
};

struct ImageBinding
{
    VkImage                    mImage = VK_NULL_HANDLE;
    VkImageView                mView = VK_NULL_HANDLE;
    VkDeviceMemory             mMemory = VK_NULL_HANDLE;
    NativeOwnershipToken       mOwnershipToken = 0;
    VkDeviceSize               mAllocationSize = 0;
    VkDeviceSize               mMemoryOffset = 0;
    VkImageCreateFlags         mCreateFlags = 0;
    VkImageType                mImageType = VK_IMAGE_TYPE_MAX_ENUM;
    VkFormat                   mFormat = VK_FORMAT_UNDEFINED;
    LLRenderContract::Extent2D mResidentExtent;
    LLRenderContract::Extent2D mLogicalExtent;
    std::uint32_t              mResidentDiscard = 0;
    std::uint32_t              mMipLevels = 0;
    std::uint32_t              mArrayLayers = 0;
    VkSampleCountFlagBits      mSamples = VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
    VkImageTiling              mTiling = VK_IMAGE_TILING_MAX_ENUM;
    VkImageUsageFlags          mUsage = 0;
    VkSharingMode              mSharingMode = VK_SHARING_MODE_MAX_ENUM;
    VkImageAspectFlags         mAspect = 0;
    VkImageLayout              mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageViewType            mViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    VkFormat                   mViewFormat = VK_FORMAT_UNDEFINED;
    VkComponentMapping         mViewComponents{};
    VkImageSubresourceRange    mViewRange{};

    // Set only on the old generation. These are the bytes from a completed,
    // queue-idle Vulkan readback immediately before executor registration.
    bool mHasPreExecutionMipSnapshot = false;
    std::array<std::uint8_t, LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_COUNT>
        mPreExecutionMipRGBA8{};
};

struct SamplerBinding
{
    VkSampler            mSampler = VK_NULL_HANDLE;
    NativeOwnershipToken mOwnershipToken = 0;
    VkSamplerCreateFlags mCreateFlags = 0;
    VkFilter             mMinFilter = VK_FILTER_NEAREST;
    VkFilter             mMagFilter = VK_FILTER_NEAREST;
    VkSamplerMipmapMode  mMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode mAddressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode mAddressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode mAddressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float                mMipLodBias = 0.f;
    VkBool32             mAnisotropyEnable = VK_FALSE;
    float                mMaxAnisotropy = 1.f;
    VkBool32             mCompareEnable = VK_FALSE;
    VkCompareOp          mCompareOp = VK_COMPARE_OP_ALWAYS;
    float                mMinLod = 0.f;
    float                mMaxLod = 0.f;
    VkBorderColor        mBorderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    VkBool32             mUnnormalizedCoordinates = VK_FALSE;
};

struct SampledDescriptorBinding
{
    std::uint32_t      mSet = 0;
    std::uint32_t      mBinding = 0;
    VkDescriptorType   mType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    VkShaderStageFlags mStages = 0;
    VkImageView        mView = VK_NULL_HANDLE;
    VkSampler          mSampler = VK_NULL_HANDLE;
    VkImageLayout      mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

struct VertexBindingState
{
    std::uint32_t     mBinding = 0;
    std::uint32_t     mStride = 0;
    VkVertexInputRate mInputRate = VK_VERTEX_INPUT_RATE_MAX_ENUM;
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
    VkSampleCountFlagBits mRasterizationSamples = VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
    VkBool32              mSampleShadingEnable = VK_FALSE;
    float                 mMinSampleShading = 0.f;
    VkSampleMask          mSampleMask = 0;
    VkBool32              mAlphaToCoverageEnable = VK_FALSE;
    VkBool32              mAlphaToOneEnable = VK_FALSE;
};

struct DepthStencilState
{
    VkBool32        mDepthTestEnable = VK_FALSE;
    VkBool32        mDepthWriteEnable = VK_FALSE;
    VkCompareOp     mDepthCompareOp = VK_COMPARE_OP_ALWAYS;
    VkBool32        mDepthBoundsTestEnable = VK_FALSE;
    VkBool32        mStencilTestEnable = VK_FALSE;
    VkStencilOpState mFront{};
    VkStencilOpState mBack{};
    float           mMinDepthBounds = 0.f;
    float           mMaxDepthBounds = 1.f;
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

struct PipelineBinding
{
    LLRenderContract::ShaderProgramKey mProgram;
    VkPipeline                         mPipeline = VK_NULL_HANDLE;
    VkPipelineLayout                   mLayout = VK_NULL_HANDLE;
    VkRenderPass                       mRenderPass = VK_NULL_HANDLE;
    VkFramebuffer                      mFramebuffer = VK_NULL_HANDLE;
    VkDescriptorSet                    mDescriptorSet = VK_NULL_HANDLE;
    NativeOwnershipToken               mOwnershipToken = 0;
    LLRenderContract::Extent2D         mExtent;

    SampledDescriptorBinding mSampledDescriptor;

    VkImageView            mColorView = VK_NULL_HANDLE;
    VkFormat               mColorFormat = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits  mColorSamples = VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
    VkAttachmentLoadOp     mColorLoadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
    VkAttachmentStoreOp    mColorStoreOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
    VkAttachmentLoadOp     mStencilLoadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
    VkAttachmentStoreOp    mStencilStoreOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
    VkImageLayout          mColorInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout          mColorFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    std::uint32_t          mSubpassDependencyCount = 0;

    VertexBindingState   mVertexBinding;
    VertexAttributeState mVertexAttribute;
    RasterState          mRaster;
    MultisampleState     mMultisample;
    DepthStencilState    mDepthStencil;
    ColorTargetState     mColorTarget;
    VkBool32             mLogicOpEnable = VK_FALSE;
    VkLogicOp            mLogicOp = VK_LOGIC_OP_COPY;
    std::array<float, 4> mBlendConstants{};
    VkBool32             mDynamicViewport = VK_FALSE;
    VkBool32             mDynamicScissor = VK_FALSE;

    ShaderIdentityToken mVertexShaderIdentity{};
    ShaderIdentityToken mFragmentShaderIdentity{};
    std::string         mVertexEntryPoint = "main";
    std::string         mFragmentEntryPoint = "main";
};

struct TransferResources
{
    BufferBinding mStaging;
    BufferBinding mReadback;
};

using LifecycleLedger = LLRenderContract::StreamingUploadLifecycle;
using ExecutionResult = LLRenderContract::TextureUploadArtifact;

// The registry borrows immutable native bindings and one lifecycle ledger for
// a synchronous diagnostic run. Registration never invokes Vulkan.
class Registry
{
public:
    bool addScreenTriangle(LLRenderContract::BufferHandle handle, BufferBinding binding);
    bool addImageGenerations(LLRenderContract::ImageHandle old_handle, ImageBinding old_image,
                             LLRenderContract::ImageHandle replacement_handle, ImageBinding replacement_image);
    bool addOutput(LLRenderContract::ImageHandle handle, ImageBinding output);
    bool addSampler(LLRenderContract::SamplerHandle handle, SamplerBinding sampler);
    bool addPipeline(LLRenderContract::PipelineHandle handle, PipelineBinding pipeline);
    bool addTransferResources(TransferResources resources);
    bool addLifecycle(LifecycleLedger* ledger);

    const BufferBinding* resolve(LLRenderContract::BufferHandle handle) const;
    const ImageBinding* resolveRegisteredImage(LLRenderContract::ImageHandle handle) const;
    const ImageBinding* resolveOutput(LLRenderContract::ImageHandle handle) const;
    const SamplerBinding* resolve(LLRenderContract::SamplerHandle handle) const;
    const PipelineBinding* resolve(LLRenderContract::PipelineHandle handle,
                                   const LLRenderContract::ShaderProgramKey& program) const;
    const TransferResources* transferResources() const;
    LifecycleLedger* lifecycle() const;
    bool isResolvable(LLRenderContract::ImageHandle handle) const;

private:
    LLRenderContract::BufferHandle mScreenHandle;
    BufferBinding                  mScreenTriangle;
    bool                           mHasScreenTriangle = false;

    LLRenderContract::ImageHandle mOldHandle;
    ImageBinding                  mOldImage;
    LLRenderContract::ImageHandle mReplacementHandle;
    ImageBinding                  mReplacementImage;
    bool                          mHasImageGenerations = false;

    LLRenderContract::ImageHandle mOutputHandle;
    ImageBinding                  mOutput;
    bool                          mHasOutput = false;

    LLRenderContract::SamplerHandle mSamplerHandle;
    SamplerBinding                  mSampler;
    bool                            mHasSampler = false;

    LLRenderContract::PipelineHandle mPipelineHandle;
    PipelineBinding                  mPipeline;
    bool                             mHasPipeline = false;

    TransferResources mTransferResources;
    bool              mHasTransferResources = false;
    LifecycleLedger*  mLifecycle = nullptr;
};

struct ExecutionContext
{
    VkDevice                 mDevice = VK_NULL_HANDLE;
    VkCommandPool            mCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer          mCommandBuffer = VK_NULL_HANDLE;
    VkQueue                  mQueue = VK_NULL_HANDLE;
    NativeOwnershipToken     mOwnershipToken = 0;
    std::uint32_t            mQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    VkQueueFlags             mQueueFamilyFlags = 0;
    std::uint32_t            mQueueCount = 0;
    std::uint32_t            mQueueIndex = 0;
    std::uint32_t            mCommandPoolQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    VkCommandPoolCreateFlags mCommandPoolFlags = 0;
    VkCommandBufferLevel     mCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
    std::uint64_t*           mRecordingAttemptCount = nullptr;
    std::uint64_t*           mSubmissionCount = nullptr;
    ShaderIdentityToken      mRequiredVertexShaderIdentity{};
    ShaderIdentityToken      mRequiredFragmentShaderIdentity{};
};

// All packet, registry, ledger, immutable metadata, and alias checks complete
// before mapped memory, command state, counters, or publication are touched.
bool execute(const LLRenderContract::FrameSnapshot& frame, Registry& registry,
             const ExecutionContext& context, ExecutionResult& result,
             std::string* error = nullptr);

} // namespace LLRenderVulkanTextureUpload

#endif // LL_LLRENDERVULKANTEXTUREUPLOAD_H
