/**
 * @file llvulkanmaterial_main.cpp
 * @brief Offscreen Vulkan replay for the fixed indexed-material diagnostic.
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

#include "llmaterialdiagnostic.h"
#include "llrendervulkanglobaldispatch.h"
#include "llrendervulkanmaterial.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace
{

using LLRenderContract::Extent2D;
using LLRenderContract::FrameSnapshot;
using LLRenderContract::MaterialArtifact;
using LLRenderContract::MaterialCase;
using LLRenderContract::MaterialFixture;
using LLRenderContract::MaterialParameters;
using LLRenderVulkanMaterial::ShaderIdentityToken;

constexpr char PORTABILITY_ENUMERATION_EXTENSION[] = "VK_KHR_portability_enumeration";
constexpr char PORTABILITY_SUBSET_EXTENSION[]      = "VK_KHR_portability_subset";

constexpr Extent2D FRAME_EXTENT{ LLRenderContract::MATERIAL_FRAME_WIDTH,
                                 LLRenderContract::MATERIAL_FRAME_HEIGHT };
constexpr Extent2D TEXTURE_EXTENT{ LLRenderContract::MATERIAL_TEXTURE_WIDTH,
                                   LLRenderContract::MATERIAL_TEXTURE_HEIGHT };
constexpr VkDeviceSize RGBA8_BYTES =
    LLRenderContract::MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT * sizeof(std::uint8_t);
constexpr VkDeviceSize RGBA16_BYTES =
    LLRenderContract::MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT * sizeof(std::uint16_t);
constexpr VkDeviceSize DEPTH32_BYTES =
    LLRenderContract::MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT * sizeof(float);
constexpr VkDeviceSize TEXTURE_BYTES =
    LLRenderContract::MATERIAL_TEXTURE_COMPONENT_COUNT * sizeof(std::uint8_t);
constexpr VkDeviceSize TRANSFER_BYTES = std::max({ RGBA8_BYTES, RGBA16_BYTES, DEPTH32_BYTES, TEXTURE_BYTES });

constexpr std::array<std::uint32_t, 7> VERTEX_STRIDES{ 16, 16, 8, 4, 16, 8, 8 };
constexpr std::array<VkFormat, 7>       VERTEX_FORMATS{
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32_SFLOAT
};
constexpr std::array<VkDeviceSize, 7> VERTEX_OFFSETS{
    LLRenderContract::MATERIAL_POSITION_OFFSET,
    LLRenderContract::MATERIAL_NORMAL_OFFSET,
    LLRenderContract::MATERIAL_TEXCOORD0_OFFSET,
    LLRenderContract::MATERIAL_COLOR_OFFSET,
    LLRenderContract::MATERIAL_TANGENT_OFFSET,
    LLRenderContract::MATERIAL_TEXCOORD1_OFFSET,
    LLRenderContract::MATERIAL_TEXCOORD2_OFFSET
};

class Failure : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class CapabilityFailure : public Failure
{
public:
    using Failure::Failure;
};

void check(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS)
    {
        std::ostringstream message;
        message << operation << " failed with VkResult " << result;
        throw Failure(message.str());
    }
}

template<typename Value, typename Function>
std::vector<Value> enumerate(Function&& function, const char* operation)
{
    for (;;)
    {
        std::uint32_t count = 0;
        check(function(&count, nullptr), operation);
        std::vector<Value> values(count);
        const VkResult result = function(&count, values.data());
        if (result == VK_INCOMPLETE)
        {
            continue;
        }
        check(result, operation);
        values.resize(count);
        return values;
    }
}

template<typename Property>
bool hasName(const std::vector<Property>& properties, const char* expected)
{
    return std::any_of(properties.begin(), properties.end(), [expected](const Property& property)
    {
        if constexpr (std::is_same_v<Property, VkLayerProperties>)
        {
            return std::strcmp(property.layerName, expected) == 0;
        }
        else
        {
            return std::strcmp(property.extensionName, expected) == 0;
        }
    });
}

struct Options
{
    std::filesystem::path mShaderDirectory;
    std::filesystem::path mOutput;
};

bool parseOptions(int argc, char** argv, Options& options, std::string& error)
{
    bool have_shader_directory = false;
    bool have_output           = false;
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument != "--shader-dir" && argument != "--output")
        {
            error = "unknown argument: " + argument;
            return false;
        }
        if (index + 1 >= argc)
        {
            error = argument + " requires a path";
            return false;
        }

        const std::filesystem::path value = argv[++index];
        if (value.empty())
        {
            error = argument + " requires a non-empty path";
            return false;
        }
        if (argument == "--shader-dir")
        {
            if (have_shader_directory)
            {
                error = "--shader-dir was specified more than once";
                return false;
            }
            options.mShaderDirectory = value;
            have_shader_directory    = true;
        }
        else
        {
            if (have_output)
            {
                error = "--output was specified more than once";
                return false;
            }
            options.mOutput = value;
            have_output     = true;
        }
    }

    if (!have_shader_directory || !have_output)
    {
        error = "both --shader-dir and --output are required";
        return false;
    }
    return true;
}

struct ValidationState
{
    std::atomic<std::uint32_t> mMessages{ 0 };
    std::mutex                 mMutex;
    std::string                mFirstMessage;
};

VKAPI_ATTR VkBool32 VKAPI_CALL validationCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    auto& state = *static_cast<ValidationState*>(user_data);
    ++state.mMessages;
    std::lock_guard<std::mutex> lock(state.mMutex);
    if (state.mFirstMessage.empty() && callback_data && callback_data->pMessage)
    {
        state.mFirstMessage = callback_data->pMessage;
    }
    return VK_FALSE;
}

std::uint32_t rotateRight(std::uint32_t value, std::uint32_t shift)
{
    return (value >> shift) | (value << (32U - shift));
}

ShaderIdentityToken sha256(const std::vector<std::uint8_t>& input)
{
    constexpr std::array<std::uint32_t, 64> K{
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
        0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
        0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
        0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
        0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
        0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
        0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
        0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
    };
    std::array<std::uint32_t, 8> state{ 0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
                                        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U };
    std::vector<std::uint8_t> padded = input;
    const std::uint64_t bit_size = static_cast<std::uint64_t>(input.size()) * 8U;
    padded.push_back(0x80U);
    while (padded.size() % 64 != 56)
    {
        padded.push_back(0);
    }
    for (int shift = 56; shift >= 0; shift -= 8)
    {
        padded.push_back(static_cast<std::uint8_t>(bit_size >> shift));
    }

    for (std::size_t block = 0; block < padded.size(); block += 64)
    {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t word = 0; word < 16; ++word)
        {
            const std::size_t byte = block + word * 4;
            words[word] = (static_cast<std::uint32_t>(padded[byte]) << 24) |
                          (static_cast<std::uint32_t>(padded[byte + 1]) << 16) |
                          (static_cast<std::uint32_t>(padded[byte + 2]) << 8) |
                          static_cast<std::uint32_t>(padded[byte + 3]);
        }
        for (std::size_t word = 16; word < words.size(); ++word)
        {
            const std::uint32_t s0 = rotateRight(words[word - 15], 7) ^ rotateRight(words[word - 15], 18) ^
                                     (words[word - 15] >> 3);
            const std::uint32_t s1 = rotateRight(words[word - 2], 17) ^ rotateRight(words[word - 2], 19) ^
                                     (words[word - 2] >> 10);
            words[word] = words[word - 16] + s0 + words[word - 7] + s1;
        }

        std::uint32_t a = state[0];
        std::uint32_t b = state[1];
        std::uint32_t c = state[2];
        std::uint32_t d = state[3];
        std::uint32_t e = state[4];
        std::uint32_t f = state[5];
        std::uint32_t g = state[6];
        std::uint32_t h = state[7];
        for (std::size_t round = 0; round < words.size(); ++round)
        {
            const std::uint32_t sum1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            const std::uint32_t choose = (e & f) ^ (~e & g);
            const std::uint32_t temporary1 = h + sum1 + choose + K[round] + words[round];
            const std::uint32_t sum0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temporary2 = sum0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + temporary1;
            d = c;
            c = b;
            b = a;
            a = temporary1 + temporary2;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    ShaderIdentityToken result{};
    for (std::size_t word = 0; word < state.size(); ++word)
    {
        result[word * 4]     = static_cast<std::uint8_t>(state[word] >> 24);
        result[word * 4 + 1] = static_cast<std::uint8_t>(state[word] >> 16);
        result[word * 4 + 2] = static_cast<std::uint8_t>(state[word] >> 8);
        result[word * 4 + 3] = static_cast<std::uint8_t>(state[word]);
    }
    return result;
}

struct Buffer
{
    VkBuffer              mBuffer = VK_NULL_HANDLE;
    VkDeviceMemory        mMemory = VK_NULL_HANDLE;
    VkDeviceSize          mSize = 0;
    VkDeviceSize          mAllocationSize = 0;
    VkMemoryPropertyFlags mMemoryProperties = 0;
    void*                 mMapped = nullptr;
};

struct Image
{
    VkImage            mImage = VK_NULL_HANDLE;
    VkDeviceMemory     mMemory = VK_NULL_HANDLE;
    VkImageView        mView = VK_NULL_HANDLE;
    VkFormat           mFormat = VK_FORMAT_UNDEFINED;
    Extent2D           mExtent;
    std::uint32_t      mMipLevels = 0;
    VkImageUsageFlags  mUsage = 0;
    VkImageAspectFlags mAspect = 0;
    VkImageLayout      mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

struct MaterialReadback
{
    std::array<std::uint8_t, LLRenderContract::MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT>  mGBuffer0{};
    std::array<std::uint8_t, LLRenderContract::MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT>  mGBuffer1{};
    std::array<std::uint16_t, LLRenderContract::MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT> mGBuffer2{};
    std::array<std::uint32_t, LLRenderContract::MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT> mDepth{};
};

struct DepthGate
{
    std::uint32_t mPasses = 0;
    std::uint32_t mFailures = 0;
    std::uint32_t mMirrorClippedPasses = 0;
    bool          mValid = false;
};

enum class RegistryMutation
{
    None,
    StaleVertex,
    StaleIndex,
    StaleDiffuse,
    StaleNormal,
    StaleSpecular,
    StaleSampler,
    StalePipeline,
    StaleGBuffer0,
    StaleGBuffer1,
    StaleGBuffer2,
    StaleDepth,
    WrongProgram,
    WrongVariant,
    WrongVertexLayout,
    WrongTranslatedIndices,
    WrongTextureFormat,
    WrongTextureExtent,
    WrongTextureMips,
    MissingUsageBit,
    LiveLayoutDrift,
    ViewRangeDrift,
    WrongSampler,
    WrongColorFormat,
    WrongTargetExtent,
    WrongDepthFormat,
    WrongDescriptors,
    WrongParameterRange,
    WrongParameterOffset,
    WrongAttachmentView,
    AttachmentAliasing,
    WrongIndexType,
    WrongTopology,
    WrongRenderPassMetadata,
    WrongPipelineMetadata,
    WrongShaderIdentity
};

enum class FrameMutation
{
    None,
    WrongLayout,
    WrongImageRange,
    WrongIndexRange,
    WrongParameterSize
};

struct RejectionSpec
{
    const char*      mName;
    RegistryMutation mRegistryMutation;
    FrameMutation    mFrameMutation;
};

constexpr std::array REJECTIONS{
    RejectionSpec{ "stale_vertex", RegistryMutation::StaleVertex, FrameMutation::None },
    RejectionSpec{ "stale_index", RegistryMutation::StaleIndex, FrameMutation::None },
    RejectionSpec{ "stale_diffuse", RegistryMutation::StaleDiffuse, FrameMutation::None },
    RejectionSpec{ "stale_normal", RegistryMutation::StaleNormal, FrameMutation::None },
    RejectionSpec{ "stale_specular", RegistryMutation::StaleSpecular, FrameMutation::None },
    RejectionSpec{ "stale_sampler", RegistryMutation::StaleSampler, FrameMutation::None },
    RejectionSpec{ "stale_pipeline", RegistryMutation::StalePipeline, FrameMutation::None },
    RejectionSpec{ "stale_gbuffer0", RegistryMutation::StaleGBuffer0, FrameMutation::None },
    RejectionSpec{ "stale_gbuffer1", RegistryMutation::StaleGBuffer1, FrameMutation::None },
    RejectionSpec{ "stale_gbuffer2", RegistryMutation::StaleGBuffer2, FrameMutation::None },
    RejectionSpec{ "stale_depth", RegistryMutation::StaleDepth, FrameMutation::None },
    RejectionSpec{ "wrong_program", RegistryMutation::WrongProgram, FrameMutation::None },
    RejectionSpec{ "wrong_variant", RegistryMutation::WrongVariant, FrameMutation::None },
    RejectionSpec{ "wrong_vertex_layout", RegistryMutation::WrongVertexLayout, FrameMutation::None },
    RejectionSpec{ "wrong_translated_indices", RegistryMutation::WrongTranslatedIndices, FrameMutation::None },
    RejectionSpec{ "wrong_texture_format", RegistryMutation::WrongTextureFormat, FrameMutation::None },
    RejectionSpec{ "wrong_texture_extent", RegistryMutation::WrongTextureExtent, FrameMutation::None },
    RejectionSpec{ "wrong_texture_mips", RegistryMutation::WrongTextureMips, FrameMutation::None },
    RejectionSpec{ "missing_usage_bit", RegistryMutation::MissingUsageBit, FrameMutation::None },
    RejectionSpec{ "live_layout_drift", RegistryMutation::LiveLayoutDrift, FrameMutation::None },
    RejectionSpec{ "view_range_drift", RegistryMutation::ViewRangeDrift, FrameMutation::None },
    RejectionSpec{ "wrong_sampler", RegistryMutation::WrongSampler, FrameMutation::None },
    RejectionSpec{ "wrong_color_format", RegistryMutation::WrongColorFormat, FrameMutation::None },
    RejectionSpec{ "wrong_target_extent", RegistryMutation::WrongTargetExtent, FrameMutation::None },
    RejectionSpec{ "wrong_depth_format", RegistryMutation::WrongDepthFormat, FrameMutation::None },
    RejectionSpec{ "wrong_descriptors", RegistryMutation::WrongDescriptors, FrameMutation::None },
    RejectionSpec{ "wrong_parameter_range", RegistryMutation::WrongParameterRange, FrameMutation::None },
    RejectionSpec{ "wrong_parameter_offset", RegistryMutation::WrongParameterOffset, FrameMutation::None },
    RejectionSpec{ "wrong_attachment_view", RegistryMutation::WrongAttachmentView, FrameMutation::None },
    RejectionSpec{ "attachment_aliasing", RegistryMutation::AttachmentAliasing, FrameMutation::None },
    RejectionSpec{ "wrong_index_type", RegistryMutation::WrongIndexType, FrameMutation::None },
    RejectionSpec{ "wrong_topology", RegistryMutation::WrongTopology, FrameMutation::None },
    RejectionSpec{ "wrong_render_pass_metadata", RegistryMutation::WrongRenderPassMetadata, FrameMutation::None },
    RejectionSpec{ "wrong_pipeline_metadata", RegistryMutation::WrongPipelineMetadata, FrameMutation::None },
    RejectionSpec{ "wrong_shader_identity", RegistryMutation::WrongShaderIdentity, FrameMutation::None },
    RejectionSpec{ "frame_wrong_layout", RegistryMutation::None, FrameMutation::WrongLayout },
    RejectionSpec{ "frame_wrong_image_range", RegistryMutation::None, FrameMutation::WrongImageRange },
    RejectionSpec{ "frame_wrong_index_range", RegistryMutation::None, FrameMutation::WrongIndexRange },
    RejectionSpec{ "frame_wrong_parameter_size", RegistryMutation::None, FrameMutation::WrongParameterSize }
};

template<typename Handle>
Handle stale(Handle handle)
{
    ++handle.mGeneration;
    return handle;
}

class VulkanMaterialRun
{
public:
    explicit VulkanMaterialRun(std::filesystem::path shader_directory)
        : mShaderDirectory(std::move(shader_directory))
    {
    }

    ~VulkanMaterialRun()
    {
        shutdown();
    }

    MaterialArtifact run();
    void shutdown() noexcept;

    std::uint32_t validationMessageCount() const noexcept { return mValidation.mMessages.load(); }
    std::string firstValidationMessage()
    {
        std::lock_guard<std::mutex> lock(mValidation.mMutex);
        return mValidation.mFirstMessage;
    }
    std::uint64_t executorRecordingCount() const noexcept { return mExecutorRecordings; }
    std::uint64_t executorSubmissionCount() const noexcept { return mExecutorSubmissions; }
    std::size_t rejectionCount() const noexcept { return mRejectionCount; }
    bool usedPortabilityEnumeration() const noexcept { return mPortabilityEnumeration; }
    bool usedPortabilitySubset() const noexcept { return mPortabilitySubset; }
    std::uint32_t vendorId() const noexcept { return mDeviceProperties.vendorID; }
    std::uint32_t deviceId() const noexcept { return mDeviceProperties.deviceID; }
    std::uint32_t apiVersion() const noexcept { return mDeviceProperties.apiVersion; }
    std::uint32_t driverVersion() const noexcept { return mDeviceProperties.driverVersion; }
    const DepthGate& depthGate() const noexcept { return mDepthGate; }

private:
    void loadShaderFiles();
    void createInstance();
    bool hasRequiredFormats(VkPhysicalDevice physical_device) const;
    bool hasHostCoherentMemory(VkPhysicalDevice physical_device) const;
    std::optional<std::uint32_t> graphicsQueueFamily(VkPhysicalDevice physical_device) const;
    std::vector<VkExtensionProperties> deviceExtensions(VkPhysicalDevice physical_device) const;
    void selectPhysicalDevice();
    void createDevice();
    std::uint32_t memoryType(std::uint32_t type_bits, VkMemoryPropertyFlags required,
                             VkMemoryPropertyFlags preferred, VkMemoryPropertyFlags& selected_properties) const;
    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool persistent_map = false);
    Image createImage(VkFormat format, Extent2D extent, std::uint32_t mip_levels,
                      VkImageUsageFlags usage, VkImageAspectFlags aspect);
    void destroyBuffer(Buffer& buffer) noexcept;
    void destroyImage(Image& image) noexcept;
    void writeBuffer(const Buffer& buffer, const void* source, std::size_t size, VkDeviceSize offset = 0);
    std::vector<std::uint8_t> readBuffer(const Buffer& buffer, std::size_t size) const;
    void submitImmediate(const std::function<void(VkCommandBuffer)>& commands);
    void createCommandResources();
    void createFixtureBuffers(const MaterialFixture& fixture);
    void createTextures(const MaterialFixture& fixture);
    void uploadTexture(Image& image, const std::array<std::uint8_t, LLRenderContract::MATERIAL_TEXTURE_COMPONENT_COUNT>& bytes);
    void createSampler();
    void createDescriptorResources();
    void createTargets();
    void createPipeline();
    VkShaderModule createShaderModule(const std::vector<std::uint32_t>& words);
    LLRenderVulkanMaterial::ExecutionContext executionContext();
    LLRenderVulkanMaterial::Registry makeRegistry(const MaterialCase& diagnostic_case, RegistryMutation mutation);
    void mutateFrame(FrameSnapshot& frame, FrameMutation mutation) const;
    void seedTargets(const MaterialFixture& fixture);
    void seedImage(Image& image, const std::vector<std::uint8_t>& bytes, VkImageLayout final_layout,
                   VkAccessFlags final_access, VkPipelineStageFlags final_stage);
    std::vector<std::uint8_t> readImage(Image& image);
    std::array<std::vector<std::uint8_t>, 4> targetSnapshot();
    void runRejections(const MaterialCase& diagnostic_case, const MaterialFixture& fixture);
    MaterialReadback readTargets();
    MaterialArtifact artifactFrom(const MaterialReadback& readback) const;
    DepthGate verifyDepthGate(const MaterialFixture& fixture, const MaterialReadback& readback) const;
    bool nontrivialOutput(const MaterialFixture& fixture, const MaterialReadback& readback, DepthGate& gate) const;

    std::filesystem::path mShaderDirectory;
    std::vector<std::uint32_t> mVertexSpirv;
    std::vector<std::uint32_t> mFragmentSpirv;
    ShaderIdentityToken mVertexIdentity{};
    ShaderIdentityToken mFragmentIdentity{};

    std::optional<LLRenderVulkan::VulkanGlobalDispatchGeneration> mGlobalDispatch;
    VkInstance               mInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice         mPhysicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties mDeviceProperties{};
    std::uint32_t            mQueueFamily = 0;
    VkDevice                 mDevice = VK_NULL_HANDLE;
    VkQueue                  mQueue = VK_NULL_HANDLE;
    VkCommandPool            mCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer          mCommandBuffer = VK_NULL_HANDLE;

    Buffer mTransferBuffer;
    Buffer mVertexBuffer;
    Buffer mIndexBuffer;
    Buffer mParameterBuffer;
    std::array<Image, LLRenderContract::MATERIAL_TEXTURE_COUNT> mTextures;
    VkSampler mSampler = VK_NULL_HANDLE;
    std::array<VkDescriptorSetLayout, 2> mDescriptorSetLayouts{};
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, 2> mDescriptorSets{};
    std::array<Image, 3> mColors;
    Image mDepth;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkFramebuffer mFramebuffer = VK_NULL_HANDLE;
    VkPipeline mPipeline = VK_NULL_HANDLE;

    ValidationState mValidation;
    std::uint64_t mExecutorRecordings = 0;
    std::uint64_t mExecutorSubmissions = 0;
    std::size_t mRejectionCount = 0;
    bool mPortabilityEnumeration = false;
    bool mPortabilitySubset = false;
    DepthGate mDepthGate;
};

void VulkanMaterialRun::loadShaderFiles()
{
    std::error_code error;
    if (!std::filesystem::is_directory(mShaderDirectory, error) || error)
    {
        throw Failure("shader directory is not readable: " + mShaderDirectory.string());
    }

    auto read = [&error](const std::filesystem::path& path,
                         std::vector<std::uint32_t>& words,
                         ShaderIdentityToken& identity)
    {
        error.clear();
        const std::uintmax_t size = std::filesystem::file_size(path, error);
        if (error || size < sizeof(std::uint32_t) || size % sizeof(std::uint32_t) != 0 || size > 16U * 1024U * 1024U)
        {
            throw Failure("SPIR-V file is missing or has an invalid size: " + path.string());
        }
        std::ifstream input(path, std::ios::binary | std::ios::in);
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        if (!input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        {
            throw Failure("cannot read complete SPIR-V file: " + path.string());
        }
        words.resize(bytes.size() / sizeof(std::uint32_t));
        std::memcpy(words.data(), bytes.data(), bytes.size());
        if (words.front() != 0x07230203U)
        {
            throw Failure("SPIR-V file has the wrong magic word: " + path.string());
        }
        identity = sha256(bytes);
    };

    read(mShaderDirectory / "material.vert.spv", mVertexSpirv, mVertexIdentity);
    read(mShaderDirectory / "material.frag.spv", mFragmentSpirv, mFragmentIdentity);
}

void VulkanMaterialRun::createInstance()
{
    LLRenderVulkan::VulkanGlobalDispatchResolutionResult dispatch_result =
        LLRenderVulkan::resolveVulkanGlobalDispatchGeneration(vkGetInstanceProcAddr);
    if (const auto* error = std::get_if<LLRenderVulkan::VulkanGlobalDispatchResolutionError>(&dispatch_result))
    {
        if (error->mCode == LLRenderVulkan::VulkanGlobalDispatchResolutionCode::VersionQueryFailure)
        {
            check(error->mResult, "vkEnumerateInstanceVersion");
        }
        if (error->mCode == LLRenderVulkan::VulkanGlobalDispatchResolutionCode::UnsupportedApiVariant)
        {
            throw CapabilityFailure("the Vulkan loader reported an unsupported API variant");
        }
        if (error->mCode == LLRenderVulkan::VulkanGlobalDispatchResolutionCode::InsufficientApiVersion)
        {
            throw CapabilityFailure("the Vulkan 1.1 loader required by the shader target is unavailable");
        }
        throw CapabilityFailure("the Vulkan global command set required by the shader target is unavailable");
    }
    mGlobalDispatch.emplace(std::get<LLRenderVulkan::VulkanGlobalDispatchGeneration>(std::move(dispatch_result)));
    const LLRenderVulkan::VulkanGlobalDispatchGeneration& global_dispatch = *mGlobalDispatch;

    const auto layers = enumerate<VkLayerProperties>(
        [&global_dispatch](std::uint32_t* count, VkLayerProperties* values)
        {
            return global_dispatch.enumerateInstanceLayerProperties()(count, values);
        },
        "vkEnumerateInstanceLayerProperties");
    if (!hasName(layers, "VK_LAYER_KHRONOS_validation"))
    {
        throw CapabilityFailure("VK_LAYER_KHRONOS_validation is required but unavailable");
    }

    const auto extensions = enumerate<VkExtensionProperties>(
        [&global_dispatch](std::uint32_t* count, VkExtensionProperties* values)
        {
            return global_dispatch.enumerateInstanceExtensionProperties()(nullptr, count, values);
        },
        "vkEnumerateInstanceExtensionProperties");
    if (!hasName(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
    {
        throw CapabilityFailure("VK_EXT_debug_utils is required but unavailable");
    }

    std::vector<const char*> enabled_extensions{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    VkInstanceCreateFlags flags = 0;
    if (hasName(extensions, PORTABILITY_ENUMERATION_EXTENSION))
    {
        enabled_extensions.push_back(PORTABILITY_ENUMERATION_EXTENSION);
        flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        mPortabilityEnumeration = true;
    }

    VkDebugUtilsMessengerCreateInfoEXT debug_info{};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = validationCallback;
    debug_info.pUserData       = &mValidation;

    VkApplicationInfo application{};
    application.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.pApplicationName   = "llvulkanmaterial";
    application.applicationVersion = 1;
    application.pEngineName        = "Second Life material diagnostic";
    application.engineVersion      = 1;
    application.apiVersion         = LLRenderVulkan::RENDERER_VULKAN_API_VERSION;

    const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    VkInstanceCreateInfo create_info{};
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext                   = &debug_info;
    create_info.flags                   = flags;
    create_info.pApplicationInfo        = &application;
    create_info.enabledLayerCount       = 1;
    create_info.ppEnabledLayerNames     = &validation_layer;
    create_info.enabledExtensionCount   = static_cast<std::uint32_t>(enabled_extensions.size());
    create_info.ppEnabledExtensionNames = enabled_extensions.data();
    VkInstance     instance        = VK_NULL_HANDLE;
    const VkResult instance_result = global_dispatch.createInstance()(&create_info, nullptr, &instance);
    check(instance_result, "vkCreateInstance");
    if (instance == VK_NULL_HANDLE)
    {
        throw Failure("vkCreateInstance returned success with a null instance");
    }
    mInstance = instance;

    const auto create_debug = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        global_dispatch.getInstanceProcAddr()(mInstance, "vkCreateDebugUtilsMessengerEXT"));
    if (!create_debug)
    {
        throw Failure("vkCreateDebugUtilsMessengerEXT is unavailable after enabling VK_EXT_debug_utils");
    }
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    const VkResult debug_result = create_debug(mInstance, &debug_info, nullptr, &debug_messenger);
    check(debug_result, "vkCreateDebugUtilsMessengerEXT");
    if (debug_messenger == VK_NULL_HANDLE)
    {
        throw Failure("vkCreateDebugUtilsMessengerEXT returned success with a null messenger");
    }
    mDebugMessenger = debug_messenger;
}

bool VulkanMaterialRun::hasRequiredFormats(VkPhysicalDevice physical_device) const
{
    struct FormatRequirement
    {
        VkFormat             mFormat;
        VkFormatFeatureFlags mFeatures;
        bool                 mBufferFeatures;
    };
    constexpr std::array requirements{
        FormatRequirement{ VK_FORMAT_R8G8B8A8_UNORM,
                           VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
                               VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
                               VK_FORMAT_FEATURE_TRANSFER_DST_BIT,
                           false },
        FormatRequirement{ VK_FORMAT_R16G16B16A16_UNORM,
                           VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
                               VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
                               VK_FORMAT_FEATURE_TRANSFER_DST_BIT,
                           false },
        FormatRequirement{ VK_FORMAT_D32_SFLOAT,
                           VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
                               VK_FORMAT_FEATURE_TRANSFER_DST_BIT,
                           false },
        FormatRequirement{ VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT, true },
        FormatRequirement{ VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT, true },
        FormatRequirement{ VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT, true },
        FormatRequirement{ VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT, true }
    };
    for (const FormatRequirement& requirement : requirements)
    {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(physical_device, requirement.mFormat, &properties);
        const VkFormatFeatureFlags available = requirement.mBufferFeatures
                                                   ? properties.bufferFeatures
                                                   : properties.optimalTilingFeatures;
        if ((available & requirement.mFeatures) != requirement.mFeatures)
        {
            return false;
        }
    }

    struct ImageRequirement
    {
        VkFormat          mFormat;
        VkImageUsageFlags mUsage;
        Extent2D          mExtent;
        std::uint32_t     mMipLevels;
    };
    constexpr std::array image_requirements{
        ImageRequirement{ VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                          TEXTURE_EXTENT,
                          LLRenderContract::MATERIAL_TEXTURE_MIP_LEVELS },
        ImageRequirement{ VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          FRAME_EXTENT,
                          1 },
        ImageRequirement{ VK_FORMAT_R16G16B16A16_UNORM,
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          FRAME_EXTENT,
                          1 },
        ImageRequirement{ VK_FORMAT_D32_SFLOAT,
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                          FRAME_EXTENT,
                          1 }
    };
    for (const ImageRequirement& requirement : image_requirements)
    {
        VkImageFormatProperties properties{};
        if (vkGetPhysicalDeviceImageFormatProperties(physical_device, requirement.mFormat, VK_IMAGE_TYPE_2D,
                                                     VK_IMAGE_TILING_OPTIMAL, requirement.mUsage, 0, &properties) != VK_SUCCESS ||
            properties.maxExtent.width < requirement.mExtent.mWidth ||
            properties.maxExtent.height < requirement.mExtent.mHeight ||
            properties.maxMipLevels < requirement.mMipLevels || properties.maxArrayLayers < 1 ||
            (properties.sampleCounts & VK_SAMPLE_COUNT_1_BIT) == 0)
        {
            return false;
        }
    }
    return true;
}

bool VulkanMaterialRun::hasHostCoherentMemory(VkPhysicalDevice physical_device) const
{
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);
    for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index)
    {
        constexpr VkMemoryPropertyFlags REQUIRED =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if ((properties.memoryTypes[index].propertyFlags & REQUIRED) == REQUIRED)
        {
            return true;
        }
    }
    return false;
}

std::optional<std::uint32_t> VulkanMaterialRun::graphicsQueueFamily(VkPhysicalDevice physical_device) const
{
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties.data());
    for (std::uint32_t index = 0; index < count; ++index)
    {
        if (properties[index].queueCount != 0 && (properties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            return index;
        }
    }
    return std::nullopt;
}

std::vector<VkExtensionProperties> VulkanMaterialRun::deviceExtensions(VkPhysicalDevice physical_device) const
{
    return enumerate<VkExtensionProperties>(
        [physical_device](std::uint32_t* count, VkExtensionProperties* values)
        {
            return vkEnumerateDeviceExtensionProperties(physical_device, nullptr, count, values);
        },
        "vkEnumerateDeviceExtensionProperties");
}

void VulkanMaterialRun::selectPhysicalDevice()
{
    const auto devices = enumerate<VkPhysicalDevice>(
        [this](std::uint32_t* count, VkPhysicalDevice* values)
        {
            return vkEnumeratePhysicalDevices(mInstance, count, values);
        },
        "vkEnumeratePhysicalDevices");
    for (VkPhysicalDevice device : devices)
    {
        VkPhysicalDeviceProperties properties{};
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);
        const auto queue_family = graphicsQueueFamily(device);
        const auto& limits      = properties.limits;
        if (properties.apiVersion < VK_API_VERSION_1_1 || features.samplerAnisotropy != VK_TRUE ||
            limits.maxSamplerAnisotropy < 8.f || limits.maxUniformBufferRange < sizeof(MaterialParameters) ||
            limits.maxVertexInputBindings < VERTEX_STRIDES.size() ||
            limits.maxVertexInputAttributes < VERTEX_FORMATS.size() || limits.maxColorAttachments < 3 ||
            limits.maxBoundDescriptorSets < 2 || limits.maxPerStageDescriptorSamplers < 3 ||
            limits.maxDescriptorSetSamplers < 3 || limits.maxPerStageDescriptorSampledImages < 3 ||
            limits.maxDescriptorSetSampledImages < 3 || limits.maxPerStageResources < 4 ||
            limits.maxPerStageDescriptorUniformBuffers < 1 ||
            limits.maxDescriptorSetUniformBuffers < 1 || limits.maxFramebufferWidth < FRAME_EXTENT.mWidth ||
            limits.maxFramebufferHeight < FRAME_EXTENT.mHeight || !queue_family || !hasRequiredFormats(device) ||
            !hasHostCoherentMemory(device))
        {
            continue;
        }
        mPhysicalDevice = device;
        mDeviceProperties = properties;
        mQueueFamily    = *queue_family;
        const auto extensions = deviceExtensions(device);
        mPortabilitySubset = hasName(extensions, PORTABILITY_SUBSET_EXTENSION);
        return;
    }
    throw CapabilityFailure("no Vulkan 1.1 graphics device supports the exact material formats, limits, and anisotropy");
}

void VulkanMaterialRun::createDevice()
{
    const float priority = 1.f;
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = mQueueFamily;
    queue_info.queueCount       = 1;
    queue_info.pQueuePriorities = &priority;

    std::vector<const char*> extensions;
    if (mPortabilitySubset)
    {
        extensions.push_back(PORTABILITY_SUBSET_EXTENSION);
    }

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;
    VkDeviceCreateInfo create_info{};
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount    = 1;
    create_info.pQueueCreateInfos       = &queue_info;
    create_info.enabledExtensionCount   = static_cast<std::uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.pEnabledFeatures        = &features;
    check(vkCreateDevice(mPhysicalDevice, &create_info, nullptr, &mDevice), "vkCreateDevice");
    vkGetDeviceQueue(mDevice, mQueueFamily, 0, &mQueue);
    if (mQueue == VK_NULL_HANDLE)
    {
        throw Failure("vkGetDeviceQueue returned a null graphics queue");
    }
}

std::uint32_t VulkanMaterialRun::memoryType(std::uint32_t type_bits, VkMemoryPropertyFlags required,
                                            VkMemoryPropertyFlags preferred,
                                            VkMemoryPropertyFlags& selected_properties) const
{
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &properties);
    for (int pass = 0; pass < 2; ++pass)
    {
        for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index)
        {
            const VkMemoryPropertyFlags flags = properties.memoryTypes[index].propertyFlags;
            const bool preferred_match = (flags & preferred) == preferred;
            if ((type_bits & (1U << index)) != 0 && (flags & required) == required &&
                (pass != 0 || preferred_match))
            {
                selected_properties = flags;
                return index;
            }
        }
    }
    throw Failure("no Vulkan memory type satisfies the required properties");
}

Buffer VulkanMaterialRun::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool persistent_map)
{
    Buffer result;
    result.mSize = size;
    VkBufferCreateInfo create_info{};
    create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size        = size;
    create_info.usage       = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    check(vkCreateBuffer(mDevice, &create_info, nullptr, &result.mBuffer), "vkCreateBuffer");
    try
    {
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(mDevice, result.mBuffer, &requirements);
        const VkMemoryPropertyFlags required =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        VkMemoryPropertyFlags selected = 0;
        const std::uint32_t memory_type = memoryType(requirements.memoryTypeBits, required, required, selected);
        result.mAllocationSize   = requirements.size;
        result.mMemoryProperties = selected;
        VkMemoryAllocateInfo allocation{};
        allocation.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation.allocationSize  = requirements.size;
        allocation.memoryTypeIndex = memory_type;
        check(vkAllocateMemory(mDevice, &allocation, nullptr, &result.mMemory), "vkAllocateMemory(buffer)");
        check(vkBindBufferMemory(mDevice, result.mBuffer, result.mMemory, 0), "vkBindBufferMemory");
        if (persistent_map)
        {
            check(vkMapMemory(mDevice, result.mMemory, 0, VK_WHOLE_SIZE, 0, &result.mMapped), "vkMapMemory(parameters)");
        }
        return result;
    }
    catch (...)
    {
        if (result.mMapped)
        {
            vkUnmapMemory(mDevice, result.mMemory);
        }
        if (result.mBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(mDevice, result.mBuffer, nullptr);
        }
        if (result.mMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(mDevice, result.mMemory, nullptr);
        }
        throw;
    }
}

Image VulkanMaterialRun::createImage(VkFormat format, Extent2D extent, std::uint32_t mip_levels,
                                     VkImageUsageFlags usage, VkImageAspectFlags aspect)
{
    Image result;
    result.mFormat    = format;
    result.mExtent    = extent;
    result.mMipLevels = mip_levels;
    result.mUsage     = usage;
    result.mAspect    = aspect;

    VkImageCreateInfo create_info{};
    create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType     = VK_IMAGE_TYPE_2D;
    create_info.format        = format;
    create_info.extent        = { extent.mWidth, extent.mHeight, 1 };
    create_info.mipLevels     = mip_levels;
    create_info.arrayLayers   = 1;
    create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage         = usage;
    create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    check(vkCreateImage(mDevice, &create_info, nullptr, &result.mImage), "vkCreateImage");
    try
    {
        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(mDevice, result.mImage, &requirements);
        VkMemoryPropertyFlags selected = 0;
        const std::uint32_t memory_type =
            memoryType(requirements.memoryTypeBits, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, selected);
        VkMemoryAllocateInfo allocation{};
        allocation.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation.allocationSize  = requirements.size;
        allocation.memoryTypeIndex = memory_type;
        check(vkAllocateMemory(mDevice, &allocation, nullptr, &result.mMemory), "vkAllocateMemory(image)");
        check(vkBindImageMemory(mDevice, result.mImage, result.mMemory, 0), "vkBindImageMemory");

        VkImageViewCreateInfo view_info{};
        view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                           = result.mImage;
        view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                          = format;
        view_info.subresourceRange.aspectMask     = aspect;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = mip_levels;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;
        check(vkCreateImageView(mDevice, &view_info, nullptr, &result.mView), "vkCreateImageView");
        return result;
    }
    catch (...)
    {
        if (result.mView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(mDevice, result.mView, nullptr);
        }
        if (result.mImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(mDevice, result.mImage, nullptr);
        }
        if (result.mMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(mDevice, result.mMemory, nullptr);
        }
        throw;
    }
}

void VulkanMaterialRun::destroyBuffer(Buffer& buffer) noexcept
{
    if (buffer.mMapped && buffer.mMemory != VK_NULL_HANDLE)
    {
        vkUnmapMemory(mDevice, buffer.mMemory);
    }
    if (buffer.mBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(mDevice, buffer.mBuffer, nullptr);
    }
    if (buffer.mMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(mDevice, buffer.mMemory, nullptr);
    }
    buffer = {};
}

void VulkanMaterialRun::destroyImage(Image& image) noexcept
{
    if (image.mView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(mDevice, image.mView, nullptr);
    }
    if (image.mImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(mDevice, image.mImage, nullptr);
    }
    if (image.mMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(mDevice, image.mMemory, nullptr);
    }
    image = {};
}

void VulkanMaterialRun::writeBuffer(const Buffer& buffer, const void* source, std::size_t size, VkDeviceSize offset)
{
    if (!source || offset > buffer.mSize || size > buffer.mSize - offset)
    {
        throw Failure("host write exceeds a Vulkan buffer");
    }
    if ((buffer.mMemoryProperties & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) !=
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        throw Failure("diagnostic buffer is not host-visible and coherent");
    }

    void* mapped = buffer.mMapped;
    bool temporary_mapping = false;
    if (!mapped)
    {
        check(vkMapMemory(mDevice, buffer.mMemory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory(write)");
        temporary_mapping = true;
    }
    std::memcpy(static_cast<std::uint8_t*>(mapped) + offset, source, size);
    if (temporary_mapping)
    {
        vkUnmapMemory(mDevice, buffer.mMemory);
    }
}

std::vector<std::uint8_t> VulkanMaterialRun::readBuffer(const Buffer& buffer, std::size_t size) const
{
    if (size > buffer.mSize || !buffer.mMapped)
    {
        throw Failure("host read exceeds or cannot access a persistently mapped Vulkan buffer");
    }
    std::vector<std::uint8_t> bytes(size);
    std::memcpy(bytes.data(), buffer.mMapped, size);
    return bytes;
}

void VulkanMaterialRun::submitImmediate(const std::function<void(VkCommandBuffer)>& commands)
{
    check(vkResetCommandBuffer(mCommandBuffer, 0), "vkResetCommandBuffer(immediate)");
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(mCommandBuffer, &begin), "vkBeginCommandBuffer(immediate)");
    commands(mCommandBuffer);
    check(vkEndCommandBuffer(mCommandBuffer), "vkEndCommandBuffer(immediate)");
    VkSubmitInfo submit{};
    submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers    = &mCommandBuffer;
    check(vkQueueSubmit(mQueue, 1, &submit, VK_NULL_HANDLE), "vkQueueSubmit(immediate)");
    check(vkQueueWaitIdle(mQueue), "vkQueueWaitIdle(immediate)");
}

void VulkanMaterialRun::createCommandResources()
{
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = mQueueFamily;
    check(vkCreateCommandPool(mDevice, &pool_info, nullptr, &mCommandPool), "vkCreateCommandPool");

    VkCommandBufferAllocateInfo allocation{};
    allocation.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocation.commandPool        = mCommandPool;
    allocation.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocation.commandBufferCount = 1;
    check(vkAllocateCommandBuffers(mDevice, &allocation, &mCommandBuffer), "vkAllocateCommandBuffers");

    mTransferBuffer = createBuffer(TRANSFER_BYTES,
                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                   true);
}

void VulkanMaterialRun::createFixtureBuffers(const MaterialFixture& fixture)
{
    if (fixture.mExtent.mWidth != FRAME_EXTENT.mWidth || fixture.mExtent.mHeight != FRAME_EXTENT.mHeight ||
        fixture.mRowOrigin != LLRenderContract::RowOrigin::BottomLeft ||
        fixture.mIndices != LLRenderContract::MATERIAL_INDICES ||
        LLRenderContract::materialFixtureFingerprint() != 0x4e52ab4e75b6748bULL)
    {
        throw Failure("material fixture identity or canonical indices changed");
    }

    mVertexBuffer = createBuffer(LLRenderContract::MATERIAL_VERTEX_BUFFER_SIZE,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    writeBuffer(mVertexBuffer, fixture.mVertexBytes.data(), fixture.mVertexBytes.size());

    mIndexBuffer = createBuffer(LLRenderContract::MATERIAL_INDEX_BUFFER_SIZE,
                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    writeBuffer(mIndexBuffer, LLRenderVulkanMaterial::MATERIAL_VULKAN_INDICES.data(),
                sizeof(LLRenderVulkanMaterial::MATERIAL_VULKAN_INDICES));

    mParameterBuffer = createBuffer(sizeof(MaterialParameters), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
    std::array<std::uint8_t, sizeof(MaterialParameters)> initial_parameters{};
    initial_parameters.fill(0xa5U);
    writeBuffer(mParameterBuffer, initial_parameters.data(), initial_parameters.size());
}

void VulkanMaterialRun::uploadTexture(
    Image& image,
    const std::array<std::uint8_t, LLRenderContract::MATERIAL_TEXTURE_COMPONENT_COUNT>& bytes)
{
    static_assert(TEXTURE_BYTES == LLRenderContract::MATERIAL_TEXTURE_COMPONENT_COUNT);
    writeBuffer(mTransferBuffer, bytes.data(), bytes.size());
    submitImmediate([this, &image](VkCommandBuffer command_buffer)
    {
        VkBufferMemoryBarrier host_to_transfer{};
        host_to_transfer.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        host_to_transfer.srcAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
        host_to_transfer.dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
        host_to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        host_to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        host_to_transfer.buffer              = mTransferBuffer.mBuffer;
        host_to_transfer.offset              = 0;
        host_to_transfer.size                = TEXTURE_BYTES;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 1, &host_to_transfer, 0, nullptr);

        VkImageMemoryBarrier to_transfer{};
        to_transfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_transfer.srcAccessMask                   = 0;
        to_transfer.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_transfer.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        to_transfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_transfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.image                           = image.mImage;
        to_transfer.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        to_transfer.subresourceRange.baseMipLevel   = 0;
        to_transfer.subresourceRange.levelCount     = image.mMipLevels;
        to_transfer.subresourceRange.baseArrayLayer = 0;
        to_transfer.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_transfer);

        std::array<VkBufferImageCopy, LLRenderContract::MATERIAL_TEXTURE_MIP_LEVELS> copies{};
        for (std::uint32_t mip = 0; mip < copies.size(); ++mip)
        {
            copies[mip].bufferOffset                    = LLRenderContract::MATERIAL_TEXTURE_MIP_BYTE_OFFSETS[mip];
            copies[mip].imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            copies[mip].imageSubresource.mipLevel       = mip;
            copies[mip].imageSubresource.baseArrayLayer = 0;
            copies[mip].imageSubresource.layerCount     = 1;
            copies[mip].imageExtent = { LLRenderContract::MATERIAL_TEXTURE_WIDTH >> mip,
                                        LLRenderContract::MATERIAL_TEXTURE_HEIGHT >> mip,
                                        1 };
        }
        vkCmdCopyBufferToImage(command_buffer, mTransferBuffer.mBuffer, image.mImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               static_cast<std::uint32_t>(copies.size()), copies.data());

        VkImageMemoryBarrier to_shader = to_transfer;
        to_shader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_shader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        to_shader.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_shader.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &to_shader);
    });
    image.mLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void VulkanMaterialRun::createTextures(const MaterialFixture& fixture)
{
    constexpr VkImageUsageFlags USAGE = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    for (std::size_t index = 0; index < mTextures.size(); ++index)
    {
        mTextures[index] = createImage(VK_FORMAT_R8G8B8A8_UNORM, TEXTURE_EXTENT,
                                       LLRenderContract::MATERIAL_TEXTURE_MIP_LEVELS,
                                       USAGE, VK_IMAGE_ASPECT_COLOR_BIT);
        uploadTexture(mTextures[index], fixture.mTextureRGBA8[index]);
    }
}

void VulkanMaterialRun::createSampler()
{
    VkSamplerCreateInfo create_info{};
    create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.magFilter               = VK_FILTER_LINEAR;
    create_info.minFilter               = VK_FILTER_LINEAR;
    create_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.mipLodBias              = 0.f;
    create_info.anisotropyEnable        = VK_TRUE;
    create_info.maxAnisotropy           = 8.f;
    create_info.compareEnable           = VK_FALSE;
    create_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    create_info.minLod                  = 0.f;
    create_info.maxLod                  = 2.f;
    create_info.borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    create_info.unnormalizedCoordinates = VK_FALSE;
    check(vkCreateSampler(mDevice, &create_info, nullptr, &mSampler), "vkCreateSampler(material)");
}

void VulkanMaterialRun::createDescriptorResources()
{
    VkDescriptorSetLayoutBinding uniform_binding{};
    uniform_binding.binding         = 0;
    uniform_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_binding.descriptorCount = 1;
    uniform_binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo uniform_layout{};
    uniform_layout.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uniform_layout.bindingCount = 1;
    uniform_layout.pBindings    = &uniform_binding;
    check(vkCreateDescriptorSetLayout(mDevice, &uniform_layout, nullptr, &mDescriptorSetLayouts[0]),
          "vkCreateDescriptorSetLayout(parameters)");

    std::array<VkDescriptorSetLayoutBinding, 3> sampled_bindings{};
    for (std::uint32_t binding = 0; binding < sampled_bindings.size(); ++binding)
    {
        sampled_bindings[binding].binding         = binding;
        sampled_bindings[binding].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampled_bindings[binding].descriptorCount = 1;
        sampled_bindings[binding].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo sampled_layout{};
    sampled_layout.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sampled_layout.bindingCount = static_cast<std::uint32_t>(sampled_bindings.size());
    sampled_layout.pBindings    = sampled_bindings.data();
    check(vkCreateDescriptorSetLayout(mDevice, &sampled_layout, nullptr, &mDescriptorSetLayouts[1]),
          "vkCreateDescriptorSetLayout(textures)");

    VkPipelineLayoutCreateInfo pipeline_layout{};
    pipeline_layout.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout.setLayoutCount = static_cast<std::uint32_t>(mDescriptorSetLayouts.size());
    pipeline_layout.pSetLayouts    = mDescriptorSetLayouts.data();
    check(vkCreatePipelineLayout(mDevice, &pipeline_layout, nullptr, &mPipelineLayout),
          "vkCreatePipelineLayout(material)");

    std::array<VkDescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
    pool_sizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 };
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets       = 2;
    pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes    = pool_sizes.data();
    check(vkCreateDescriptorPool(mDevice, &pool_info, nullptr, &mDescriptorPool), "vkCreateDescriptorPool(material)");

    VkDescriptorSetAllocateInfo allocation{};
    allocation.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocation.descriptorPool     = mDescriptorPool;
    allocation.descriptorSetCount = static_cast<std::uint32_t>(mDescriptorSetLayouts.size());
    allocation.pSetLayouts        = mDescriptorSetLayouts.data();
    check(vkAllocateDescriptorSets(mDevice, &allocation, mDescriptorSets.data()), "vkAllocateDescriptorSets(material)");

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = mParameterBuffer.mBuffer;
    buffer_info.offset = 0;
    buffer_info.range  = sizeof(MaterialParameters);
    std::array<VkDescriptorImageInfo, 3> image_infos{};
    for (std::size_t index = 0; index < image_infos.size(); ++index)
    {
        image_infos[index].sampler     = mSampler;
        image_infos[index].imageView   = mTextures[index].mView;
        image_infos[index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    std::array<VkWriteDescriptorSet, 4> writes{};
    writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet          = mDescriptorSets[0];
    writes[0].dstBinding      = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo     = &buffer_info;
    for (std::size_t index = 0; index < image_infos.size(); ++index)
    {
        writes[index + 1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[index + 1].dstSet          = mDescriptorSets[1];
        writes[index + 1].dstBinding      = static_cast<std::uint32_t>(index);
        writes[index + 1].descriptorCount = 1;
        writes[index + 1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[index + 1].pImageInfo      = &image_infos[index];
    }
    vkUpdateDescriptorSets(mDevice, static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void VulkanMaterialRun::createTargets()
{
    constexpr VkImageUsageFlags COLOR_USAGE = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                VK_IMAGE_USAGE_SAMPLED_BIT |
                                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    constexpr std::array<VkFormat, 3> COLOR_FORMATS{ VK_FORMAT_R8G8B8A8_UNORM,
                                                     VK_FORMAT_R8G8B8A8_UNORM,
                                                     VK_FORMAT_R16G16B16A16_UNORM };
    for (std::size_t index = 0; index < mColors.size(); ++index)
    {
        mColors[index] = createImage(COLOR_FORMATS[index], FRAME_EXTENT, 1, COLOR_USAGE,
                                     VK_IMAGE_ASPECT_COLOR_BIT);
    }
    constexpr VkImageUsageFlags DEPTH_USAGE = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    mDepth = createImage(VK_FORMAT_D32_SFLOAT, FRAME_EXTENT, 1, DEPTH_USAGE,
                         VK_IMAGE_ASPECT_DEPTH_BIT);

    std::array<VkAttachmentDescription, 4> attachments{};
    for (std::size_t index = 0; index < mColors.size(); ++index)
    {
        attachments[index].format         = mColors[index].mFormat;
        attachments[index].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[index].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[index].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[index].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[index].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[index].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    attachments[3].format         = VK_FORMAT_D32_SFLOAT;
    attachments[3].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[3].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[3].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[3].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 3> color_references{};
    for (std::uint32_t index = 0; index < color_references.size(); ++index)
    {
        color_references[index].attachment = index;
        color_references[index].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference depth_reference{};
    depth_reference.attachment = 3;
    depth_reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = static_cast<std::uint32_t>(color_references.size());
    subpass.pColorAttachments       = color_references.data();
    subpass.pDepthStencilAttachment = &depth_reference;

    constexpr VkPipelineStageFlags ATTACHMENT_STAGES = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                                         VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    constexpr VkAccessFlags ATTACHMENT_ACCESS = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass    = 0;
    dependencies[0].srcStageMask  = ATTACHMENT_STAGES;
    dependencies[0].dstStageMask  = ATTACHMENT_STAGES;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstAccessMask = ATTACHMENT_ACCESS;
    dependencies[1].srcSubpass    = 0;
    dependencies[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask  = ATTACHMENT_STAGES;
    dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask = ATTACHMENT_ACCESS;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
                                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    render_pass_info.pAttachments    = attachments.data();
    render_pass_info.subpassCount    = 1;
    render_pass_info.pSubpasses      = &subpass;
    render_pass_info.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
    render_pass_info.pDependencies   = dependencies.data();
    check(vkCreateRenderPass(mDevice, &render_pass_info, nullptr, &mRenderPass),
          "vkCreateRenderPass(material)");

    std::array<VkImageView, 4> views{ mColors[0].mView, mColors[1].mView, mColors[2].mView,
                                      mDepth.mView };
    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass      = mRenderPass;
    framebuffer_info.attachmentCount = static_cast<std::uint32_t>(views.size());
    framebuffer_info.pAttachments    = views.data();
    framebuffer_info.width           = FRAME_EXTENT.mWidth;
    framebuffer_info.height          = FRAME_EXTENT.mHeight;
    framebuffer_info.layers          = 1;
    check(vkCreateFramebuffer(mDevice, &framebuffer_info, nullptr, &mFramebuffer),
          "vkCreateFramebuffer(material)");
}

VkShaderModule VulkanMaterialRun::createShaderModule(const std::vector<std::uint32_t>& words)
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = words.size() * sizeof(std::uint32_t);
    create_info.pCode    = words.data();
    VkShaderModule module = VK_NULL_HANDLE;
    check(vkCreateShaderModule(mDevice, &create_info, nullptr, &module), "vkCreateShaderModule(material)");
    return module;
}

void VulkanMaterialRun::createPipeline()
{
    VkShaderModule vertex_module   = createShaderModule(mVertexSpirv);
    VkShaderModule fragment_module = VK_NULL_HANDLE;
    try
    {
        fragment_module = createShaderModule(mFragmentSpirv);

        std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
        stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertex_module;
        stages[0].pName  = "main";
        stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragment_module;
        stages[1].pName  = "main";

        std::array<VkVertexInputBindingDescription, 7> bindings{};
        std::array<VkVertexInputAttributeDescription, 7> attributes{};
        for (std::uint32_t index = 0; index < bindings.size(); ++index)
        {
            bindings[index].binding   = index;
            bindings[index].stride    = VERTEX_STRIDES[index];
            bindings[index].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            attributes[index].location = index;
            attributes[index].binding  = index;
            attributes[index].format   = VERTEX_FORMATS[index];
            attributes[index].offset   = 0;
        }
        VkPipelineVertexInputStateCreateInfo vertex_input{};
        vertex_input.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input.vertexBindingDescriptionCount   = static_cast<std::uint32_t>(bindings.size());
        vertex_input.pVertexBindingDescriptions      = bindings.data();
        vertex_input.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
        vertex_input.pVertexAttributeDescriptions    = attributes.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewport{};
        viewport.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport.viewportCount = 1;
        viewport.scissorCount  = 1;

        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.depthClampEnable        = VK_FALSE;
        raster.rasterizerDiscardEnable = VK_FALSE;
        raster.polygonMode             = VK_POLYGON_MODE_FILL;
        raster.cullMode                = VK_CULL_MODE_BACK_BIT;
        raster.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster.depthBiasEnable         = VK_FALSE;
        raster.lineWidth               = 1.f;

        VkPipelineMultisampleStateCreateInfo multisample{};
        multisample.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample.sampleShadingEnable  = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depth{};
        depth.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth.depthTestEnable       = VK_TRUE;
        depth.depthWriteEnable      = VK_TRUE;
        depth.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth.depthBoundsTestEnable = VK_FALSE;
        depth.stencilTestEnable     = VK_FALSE;
        depth.minDepthBounds        = 0.f;
        depth.maxDepthBounds        = 1.f;

        std::array<VkPipelineColorBlendAttachmentState, 3> blend_attachments{};
        for (auto& attachment : blend_attachments)
        {
            attachment.blendEnable         = VK_FALSE;
            attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            attachment.colorBlendOp        = VK_BLEND_OP_ADD;
            attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
            attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        }
        VkPipelineColorBlendStateCreateInfo blend{};
        blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend.logicOpEnable   = VK_FALSE;
        blend.logicOp         = VK_LOGIC_OP_COPY;
        blend.attachmentCount = static_cast<std::uint32_t>(blend_attachments.size());
        blend.pAttachments    = blend_attachments.data();

        constexpr std::array<VkDynamicState, 2> DYNAMIC_STATES{ VK_DYNAMIC_STATE_VIEWPORT,
                                                                 VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamic{};
        dynamic.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic.dynamicStateCount = static_cast<std::uint32_t>(DYNAMIC_STATES.size());
        dynamic.pDynamicStates    = DYNAMIC_STATES.data();

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount          = static_cast<std::uint32_t>(stages.size());
        pipeline_info.pStages             = stages.data();
        pipeline_info.pVertexInputState   = &vertex_input;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState      = &viewport;
        pipeline_info.pRasterizationState = &raster;
        pipeline_info.pMultisampleState   = &multisample;
        pipeline_info.pDepthStencilState  = &depth;
        pipeline_info.pColorBlendState    = &blend;
        pipeline_info.pDynamicState       = &dynamic;
        pipeline_info.layout              = mPipelineLayout;
        pipeline_info.renderPass          = mRenderPass;
        pipeline_info.subpass             = 0;
        check(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &mPipeline),
              "vkCreateGraphicsPipelines(material)");
    }
    catch (...)
    {
        if (fragment_module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(mDevice, fragment_module, nullptr);
        }
        vkDestroyShaderModule(mDevice, vertex_module, nullptr);
        throw;
    }
    vkDestroyShaderModule(mDevice, fragment_module, nullptr);
    vkDestroyShaderModule(mDevice, vertex_module, nullptr);
}

LLRenderVulkanMaterial::ExecutionContext VulkanMaterialRun::executionContext()
{
    LLRenderVulkanMaterial::ExecutionContext context;
    context.mDevice                         = mDevice;
    context.mCommandBuffer                  = mCommandBuffer;
    context.mQueue                          = mQueue;
    context.mRecordingAttemptCount          = &mExecutorRecordings;
    context.mSubmissionCount                = &mExecutorSubmissions;
    context.mRequiredVertexShaderIdentity   = mVertexIdentity;
    context.mRequiredFragmentShaderIdentity = mFragmentIdentity;
    return context;
}

LLRenderVulkanMaterial::Registry VulkanMaterialRun::makeRegistry(
    const MaterialCase& diagnostic_case,
    RegistryMutation mutation)
{
    using namespace LLRenderVulkanMaterial;
    const auto& handles = diagnostic_case.mInputs.mHandles;
    Registry registry;

    auto buffer_binding = [](const Buffer& buffer, VkBufferUsageFlags usage)
    {
        BufferBinding binding;
        binding.mBuffer = buffer.mBuffer;
        binding.mSize   = buffer.mSize;
        binding.mUsage  = usage;
        return binding;
    };
    BufferBinding vertex = buffer_binding(mVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    BufferBinding index  = buffer_binding(mIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    index.mHasTranslatedIndices = true;
    index.mTranslatedIndices    = MATERIAL_VULKAN_INDICES;
    if (mutation == RegistryMutation::WrongTranslatedIndices)
    {
        index.mTranslatedIndices = LLRenderContract::MATERIAL_INDICES;
    }
    if (!registry.addBuffer(mutation == RegistryMutation::StaleVertex ? stale(handles.mVertexBuffer)
                                                                       : handles.mVertexBuffer,
                            vertex) ||
        !registry.addBuffer(mutation == RegistryMutation::StaleIndex ? stale(handles.mIndexBuffer)
                                                                      : handles.mIndexBuffer,
                            index))
    {
        throw Failure("cannot register material buffers");
    }

    auto image_binding = [](const Image& image)
    {
        ImageBinding binding;
        binding.mImage     = image.mImage;
        binding.mView      = image.mView;
        binding.mFormat    = image.mFormat;
        binding.mExtent    = image.mExtent;
        binding.mMipLevels = image.mMipLevels;
        binding.mUsage     = image.mUsage;
        binding.mAspect    = image.mAspect;
        binding.mLayout    = image.mLayout;
        binding.mViewRange.aspectMask     = image.mAspect;
        binding.mViewRange.baseMipLevel   = 0;
        binding.mViewRange.levelCount     = image.mMipLevels;
        binding.mViewRange.baseArrayLayer = 0;
        binding.mViewRange.layerCount     = 1;
        return binding;
    };
    std::array<ImageBinding, 3> sources{};
    std::array<ImageBinding, 3> colors{};
    for (std::size_t index_value = 0; index_value < sources.size(); ++index_value)
    {
        sources[index_value] = image_binding(mTextures[index_value]);
        colors[index_value]  = image_binding(mColors[index_value]);
    }
    ImageBinding depth = image_binding(mDepth);

    if (mutation == RegistryMutation::WrongTextureFormat)
        sources[0].mFormat = VK_FORMAT_R8G8B8A8_SRGB;
    if (mutation == RegistryMutation::WrongTextureExtent)
        --sources[0].mExtent.mWidth;
    if (mutation == RegistryMutation::WrongTextureMips)
        --sources[0].mMipLevels;
    if (mutation == RegistryMutation::MissingUsageBit)
        colors[0].mUsage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
    if (mutation == RegistryMutation::LiveLayoutDrift)
        colors[0].mLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    if (mutation == RegistryMutation::ViewRangeDrift)
        colors[0].mViewRange.baseMipLevel = 1;
    if (mutation == RegistryMutation::WrongColorFormat)
        colors[2].mFormat = VK_FORMAT_R8G8B8A8_UNORM;
    if (mutation == RegistryMutation::WrongTargetExtent)
        --colors[0].mExtent.mWidth;
    if (mutation == RegistryMutation::WrongDepthFormat)
        depth.mFormat = VK_FORMAT_D16_UNORM;
    if (mutation == RegistryMutation::AttachmentAliasing)
    {
        colors[0].mImage = colors[1].mImage;
        colors[0].mView  = colors[1].mView;
    }

    const std::array image_handles{ handles.mDiffuse, handles.mNormal, handles.mSpecular,
                                    handles.mGBuffer0, handles.mGBuffer1, handles.mGBuffer2,
                                    handles.mDepth };
    std::array<ImageBinding, 7> image_bindings{ sources[0], sources[1], sources[2],
                                                colors[0], colors[1], colors[2], depth };
    for (std::size_t index_value = 0; index_value < image_bindings.size(); ++index_value)
    {
        auto handle = image_handles[index_value];
        const bool stale_image =
            (index_value == 0 && mutation == RegistryMutation::StaleDiffuse) ||
            (index_value == 1 && mutation == RegistryMutation::StaleNormal) ||
            (index_value == 2 && mutation == RegistryMutation::StaleSpecular) ||
            (index_value == 3 && mutation == RegistryMutation::StaleGBuffer0) ||
            (index_value == 4 && mutation == RegistryMutation::StaleGBuffer1) ||
            (index_value == 5 && mutation == RegistryMutation::StaleGBuffer2) ||
            (index_value == 6 && mutation == RegistryMutation::StaleDepth);
        if (stale_image)
        {
            handle = stale(handle);
        }
        if (!registry.addImage(handle, image_bindings[index_value]))
        {
            throw Failure("cannot register material images");
        }
    }

    SamplerBinding sampler;
    sampler.mSampler                 = mSampler;
    sampler.mMinFilter               = VK_FILTER_LINEAR;
    sampler.mMagFilter               = VK_FILTER_LINEAR;
    sampler.mMipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.mAddressU                = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.mAddressV                = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.mAddressW                = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.mMipLodBias              = 0.f;
    sampler.mAnisotropyEnable        = VK_TRUE;
    sampler.mMaxAnisotropy           = 8.f;
    sampler.mCompareEnable           = VK_FALSE;
    sampler.mCompareOp               = VK_COMPARE_OP_ALWAYS;
    sampler.mMinLod                  = 0.f;
    sampler.mMaxLod                  = 2.f;
    sampler.mBorderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler.mUnnormalizedCoordinates = VK_FALSE;
    if (mutation == RegistryMutation::WrongSampler)
        sampler.mMaxAnisotropy = 4.f;
    if (!registry.addSampler(mutation == RegistryMutation::StaleSampler ? stale(handles.mSampler)
                                                                         : handles.mSampler,
                             sampler))
    {
        throw Failure("cannot register material sampler");
    }

    PipelineBinding pipeline;
    pipeline.mProgram           = diagnostic_case.mFrame.mPipelines.front().mProgram;
    pipeline.mPipeline          = mPipeline;
    pipeline.mLayout            = mPipelineLayout;
    pipeline.mRenderPass        = mRenderPass;
    pipeline.mFramebuffer       = mFramebuffer;
    pipeline.mExtent            = FRAME_EXTENT;
    pipeline.mDescriptorSets    = mDescriptorSets;
    pipeline.mParameterDescriptor.mSet     = 0;
    pipeline.mParameterDescriptor.mBinding = 0;
    pipeline.mParameterDescriptor.mBuffer  = mParameterBuffer.mBuffer;
    pipeline.mParameterDescriptor.mOffset  = 0;
    pipeline.mParameterDescriptor.mRange   = sizeof(MaterialParameters);
    pipeline.mParameters.mBuffer           = mParameterBuffer.mBuffer;
    pipeline.mParameters.mUsage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    pipeline.mParameters.mSize             = mParameterBuffer.mSize;
    pipeline.mParameters.mMemory           = mParameterBuffer.mMemory;
    pipeline.mParameters.mMapped           = mParameterBuffer.mMapped;
    pipeline.mParameters.mAllocationSize   = mParameterBuffer.mAllocationSize;
    pipeline.mParameters.mDescriptorOffset = 0;
    pipeline.mParameters.mDescriptorRange  = sizeof(MaterialParameters);
    pipeline.mParameters.mMemoryProperties = mParameterBuffer.mMemoryProperties;
    for (std::uint32_t index_value = 0; index_value < mTextures.size(); ++index_value)
    {
        pipeline.mSampledDescriptors[index_value].mSet       = 1;
        pipeline.mSampledDescriptors[index_value].mBinding   = index_value;
        pipeline.mSampledDescriptors[index_value].mView      = mTextures[index_value].mView;
        pipeline.mSampledDescriptors[index_value].mSampler   = mSampler;
        pipeline.mSampledDescriptors[index_value].mLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        pipeline.mColorViews[index_value]                    = mColors[index_value].mView;
        pipeline.mColorLoadOps[index_value]                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        pipeline.mColorStoreOps[index_value]                 = VK_ATTACHMENT_STORE_OP_STORE;
        pipeline.mVertexBindings[index_value].mBinding       = index_value;
    }
    for (std::uint32_t index_value = 0; index_value < pipeline.mVertexBindings.size(); ++index_value)
    {
        pipeline.mVertexBindings[index_value].mBinding   = index_value;
        pipeline.mVertexBindings[index_value].mStride    = VERTEX_STRIDES[index_value];
        pipeline.mVertexBindings[index_value].mInputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        pipeline.mVertexAttributes[index_value].mLocation = index_value;
        pipeline.mVertexAttributes[index_value].mBinding  = index_value;
        pipeline.mVertexAttributes[index_value].mFormat   = VERTEX_FORMATS[index_value];
        pipeline.mVertexAttributes[index_value].mOffset   = 0;
    }
    pipeline.mDepthView          = mDepth.mView;
    pipeline.mDepthFormat        = VK_FORMAT_D32_SFLOAT;
    pipeline.mDepthLoadOp        = VK_ATTACHMENT_LOAD_OP_LOAD;
    pipeline.mDepthStoreOp       = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline.mColorInitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    pipeline.mColorFinalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    pipeline.mDepthInitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    pipeline.mDepthFinalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    pipeline.mSubpassDependencies[0].mSourceSubpass      = VK_SUBPASS_EXTERNAL;
    pipeline.mSubpassDependencies[0].mDestinationSubpass = 0;
    pipeline.mSubpassDependencies[0].mSourceStages       = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    pipeline.mSubpassDependencies[0].mDestinationStages =
        pipeline.mSubpassDependencies[0].mSourceStages;
    pipeline.mSubpassDependencies[0].mSourceAccess      = 0;
    pipeline.mSubpassDependencies[0].mDestinationAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    pipeline.mSubpassDependencies[1].mSourceSubpass      = 0;
    pipeline.mSubpassDependencies[1].mDestinationSubpass = VK_SUBPASS_EXTERNAL;
    pipeline.mSubpassDependencies[1].mSourceStages = pipeline.mSubpassDependencies[0].mSourceStages;
    pipeline.mSubpassDependencies[1].mDestinationStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    pipeline.mSubpassDependencies[1].mSourceAccess =
        pipeline.mSubpassDependencies[0].mDestinationAccess;
    pipeline.mSubpassDependencies[1].mDestinationAccess = VK_ACCESS_SHADER_READ_BIT |
                                                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    pipeline.mIndexType          = VK_INDEX_TYPE_UINT16;
    pipeline.mRaster.mTopology                = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline.mRaster.mPrimitiveRestartEnable  = VK_FALSE;
    pipeline.mRaster.mDepthClampEnable        = VK_FALSE;
    pipeline.mRaster.mRasterizerDiscardEnable = VK_FALSE;
    pipeline.mRaster.mPolygonMode             = VK_POLYGON_MODE_FILL;
    pipeline.mRaster.mCullMode                = VK_CULL_MODE_BACK_BIT;
    pipeline.mRaster.mFrontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline.mRaster.mDepthBiasEnable         = VK_FALSE;
    pipeline.mRaster.mLineWidth               = 1.f;
    pipeline.mMultisample.mRasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline.mDepthStencil.mDepthTestEnable     = VK_TRUE;
    pipeline.mDepthStencil.mDepthWriteEnable    = VK_TRUE;
    pipeline.mDepthStencil.mDepthCompareOp      = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipeline.mDepthStencil.mMinDepthBounds      = 0.f;
    pipeline.mDepthStencil.mMaxDepthBounds      = 1.f;
    constexpr std::array<VkFormat, 3> COLOR_FORMATS{ VK_FORMAT_R8G8B8A8_UNORM,
                                                     VK_FORMAT_R8G8B8A8_UNORM,
                                                     VK_FORMAT_R16G16B16A16_UNORM };
    for (std::size_t index_value = 0; index_value < pipeline.mColorTargets.size(); ++index_value)
    {
        auto& target                 = pipeline.mColorTargets[index_value];
        target.mFormat               = COLOR_FORMATS[index_value];
        target.mBlendEnable          = VK_FALSE;
        target.mSrcColorBlendFactor  = VK_BLEND_FACTOR_ONE;
        target.mDstColorBlendFactor  = VK_BLEND_FACTOR_ZERO;
        target.mColorBlendOp         = VK_BLEND_OP_ADD;
        target.mSrcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
        target.mDstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
        target.mAlphaBlendOp         = VK_BLEND_OP_ADD;
        target.mWriteMask            = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
    pipeline.mLogicOpEnable         = VK_FALSE;
    pipeline.mLogicOp               = VK_LOGIC_OP_COPY;
    pipeline.mDynamicViewport       = VK_TRUE;
    pipeline.mDynamicScissor        = VK_TRUE;
    pipeline.mVertexShaderIdentity   = mVertexIdentity;
    pipeline.mFragmentShaderIdentity = mFragmentIdentity;

    if (mutation == RegistryMutation::WrongProgram)
        pipeline.mProgram.mName = "deferred.material.wrong";
    if (mutation == RegistryMutation::WrongVariant)
        ++pipeline.mProgram.mVariant;
    if (mutation == RegistryMutation::WrongVertexLayout)
        ++pipeline.mVertexBindings[0].mStride;
    if (mutation == RegistryMutation::WrongDescriptors)
        std::swap(pipeline.mSampledDescriptors[0].mView, pipeline.mSampledDescriptors[1].mView);
    if (mutation == RegistryMutation::WrongParameterRange)
        --pipeline.mParameters.mDescriptorRange;
    if (mutation == RegistryMutation::WrongParameterOffset)
        pipeline.mParameters.mDescriptorOffset = 4;
    if (mutation == RegistryMutation::WrongAttachmentView)
        pipeline.mColorViews[0] = mColors[1].mView;
    if (mutation == RegistryMutation::WrongIndexType)
        pipeline.mIndexType = VK_INDEX_TYPE_UINT32;
    if (mutation == RegistryMutation::WrongTopology)
        pipeline.mRaster.mTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    if (mutation == RegistryMutation::WrongRenderPassMetadata)
        pipeline.mColorInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (mutation == RegistryMutation::WrongPipelineMetadata)
        pipeline.mLogicOp = VK_LOGIC_OP_CLEAR;
    if (mutation == RegistryMutation::WrongShaderIdentity)
        pipeline.mVertexShaderIdentity[0] ^= 0xffU;

    if (!registry.addPipeline(mutation == RegistryMutation::StalePipeline ? stale(handles.mPipeline)
                                                                           : handles.mPipeline,
                              std::move(pipeline)))
    {
        throw Failure("cannot register material pipeline");
    }
    return registry;
}

void VulkanMaterialRun::mutateFrame(FrameSnapshot& frame, FrameMutation mutation) const
{
    if (mutation == FrameMutation::None)
    {
        return;
    }
    auto& pass = frame.mPasses.front();
    auto& draw = std::get<LLRenderContract::DrawIndexed>(pass.mDraws.front());
    switch (mutation)
    {
        case FrameMutation::WrongLayout:
            frame.mPipelines.front().mVertexBindings.front().mStride = 12;
            break;
        case FrameMutation::WrongImageRange:
            draw.mResources.mSampledImages.front().mRange.mMipLevelCount =
                LLRenderContract::MATERIAL_TEXTURE_MIP_LEVELS - 1;
            break;
        case FrameMutation::WrongIndexRange:
            draw.mFirstIndex = 1;
            break;
        case FrameMutation::WrongParameterSize:
            --draw.mResources.mParameters.front().mBytes.mSize;
            break;
        case FrameMutation::None:
            break;
    }
}

void VulkanMaterialRun::seedImage(Image& image, const std::vector<std::uint8_t>& bytes,
                                  VkImageLayout final_layout, VkAccessFlags final_access,
                                  VkPipelineStageFlags final_stage)
{
    VkDeviceSize expected_size = 0;
    switch (image.mFormat)
    {
        case VK_FORMAT_R8G8B8A8_UNORM:
            expected_size = RGBA8_BYTES;
            break;
        case VK_FORMAT_R16G16B16A16_UNORM:
            expected_size = RGBA16_BYTES;
            break;
        case VK_FORMAT_D32_SFLOAT:
            expected_size = DEPTH32_BYTES;
            break;
        default:
            throw Failure("cannot seed an unsupported material target format");
    }
    if (bytes.size() != expected_size)
    {
        throw Failure("material target seed has the wrong byte count");
    }

    VkAccessFlags source_access = 0;
    VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    switch (image.mLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            source_access = VK_ACCESS_TRANSFER_READ_BIT;
            source_stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            source_access = VK_ACCESS_SHADER_READ_BIT;
            source_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            source_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            source_stage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            source_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        default:
            throw Failure("material target has an untracked layout before seed");
    }

    writeBuffer(mTransferBuffer, bytes.data(), bytes.size());
    const VkImageLayout old_layout = image.mLayout;
    submitImmediate([this, &image, expected_size, old_layout, source_access, source_stage,
                     final_layout, final_access, final_stage](VkCommandBuffer command_buffer)
    {
        VkBufferMemoryBarrier host_to_transfer{};
        host_to_transfer.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        host_to_transfer.srcAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
        host_to_transfer.dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
        host_to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        host_to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        host_to_transfer.buffer              = mTransferBuffer.mBuffer;
        host_to_transfer.offset              = 0;
        host_to_transfer.size                = expected_size;

        VkImageMemoryBarrier to_transfer{};
        to_transfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_transfer.srcAccessMask                   = source_access;
        to_transfer.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_transfer.oldLayout                       = old_layout;
        to_transfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_transfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.image                           = image.mImage;
        to_transfer.subresourceRange.aspectMask     = image.mAspect;
        to_transfer.subresourceRange.baseMipLevel   = 0;
        to_transfer.subresourceRange.levelCount     = 1;
        to_transfer.subresourceRange.baseArrayLayer = 0;
        to_transfer.subresourceRange.layerCount     = 1;
        vkCmdPipelineBarrier(command_buffer, source_stage | VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                             1, &host_to_transfer, 1, &to_transfer);

        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask     = image.mAspect;
        copy.imageSubresource.mipLevel       = 0;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount     = 1;
        copy.imageExtent                     = { image.mExtent.mWidth, image.mExtent.mHeight, 1 };
        vkCmdCopyBufferToImage(command_buffer, mTransferBuffer.mBuffer, image.mImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        VkImageMemoryBarrier to_final = to_transfer;
        to_final.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_final.dstAccessMask = final_access;
        to_final.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_final.newLayout     = final_layout;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, final_stage,
                             0, 0, nullptr, 0, nullptr, 1, &to_final);
    });
    image.mLayout = final_layout;
}

void VulkanMaterialRun::seedTargets(const MaterialFixture& fixture)
{
    auto bottom_to_top_bytes = [](const auto& source)
    {
        using Value = typename std::decay_t<decltype(source)>::value_type;
        constexpr std::size_t channels = LLRenderContract::MATERIAL_DIAGNOSTIC_CHANNELS;
        constexpr std::size_t width    = LLRenderContract::MATERIAL_FRAME_WIDTH;
        constexpr std::size_t height   = LLRenderContract::MATERIAL_FRAME_HEIGHT;
        constexpr std::size_t size = std::tuple_size_v<std::decay_t<decltype(source)>>;
        std::array<Value, size> result{};
        for (std::size_t top_row = 0; top_row < height; ++top_row)
        {
            const std::size_t bottom_row = height - 1 - top_row;
            std::copy_n(source.begin() + bottom_row * width * channels, width * channels,
                        result.begin() + top_row * width * channels);
        }
        std::vector<std::uint8_t> bytes(sizeof(result));
        std::memcpy(bytes.data(), result.data(), sizeof(result));
        return bytes;
    };

    seedImage(mColors[0], bottom_to_top_bytes(fixture.mGBuffer0SentinelRGBA8),
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT,
              VK_PIPELINE_STAGE_TRANSFER_BIT);
    seedImage(mColors[1], bottom_to_top_bytes(fixture.mGBuffer1SentinelRGBA8),
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT,
              VK_PIPELINE_STAGE_TRANSFER_BIT);
    seedImage(mColors[2], bottom_to_top_bytes(fixture.mGBuffer2SentinelRGBA16),
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT,
              VK_PIPELINE_STAGE_TRANSFER_BIT);

    std::array<float, LLRenderContract::MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT> depth{};
    for (std::size_t top_row = 0; top_row < FRAME_EXTENT.mHeight; ++top_row)
    {
        const std::size_t bottom_row = FRAME_EXTENT.mHeight - 1 - top_row;
        for (std::size_t x = 0; x < FRAME_EXTENT.mWidth; ++x)
        {
            depth[top_row * FRAME_EXTENT.mWidth + x] =
                LLRenderContract::materialDepth24(fixture.mDepth24[bottom_row * FRAME_EXTENT.mWidth + x]);
        }
    }
    std::vector<std::uint8_t> depth_bytes(sizeof(depth));
    std::memcpy(depth_bytes.data(), depth.data(), sizeof(depth));
    seedImage(mDepth, depth_bytes, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
}

std::vector<std::uint8_t> VulkanMaterialRun::readImage(Image& image)
{
    VkDeviceSize byte_count = 0;
    switch (image.mFormat)
    {
        case VK_FORMAT_R8G8B8A8_UNORM:
            byte_count = RGBA8_BYTES;
            break;
        case VK_FORMAT_R16G16B16A16_UNORM:
            byte_count = RGBA16_BYTES;
            break;
        case VK_FORMAT_D32_SFLOAT:
            byte_count = DEPTH32_BYTES;
            break;
        default:
            throw Failure("cannot read an unsupported material target format");
    }

    VkAccessFlags source_access = 0;
    VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    switch (image.mLayout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            source_access = VK_ACCESS_TRANSFER_READ_BIT;
            source_stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            source_access = VK_ACCESS_SHADER_READ_BIT;
            source_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            source_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        default:
            throw Failure("material target has an untracked layout before readback");
    }
    const VkImageLayout old_layout = image.mLayout;
    submitImmediate([this, &image, byte_count, old_layout, source_access, source_stage]
                    (VkCommandBuffer command_buffer)
    {
        VkBufferMemoryBarrier prepare_buffer{};
        prepare_buffer.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        prepare_buffer.srcAccessMask       = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
        prepare_buffer.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        prepare_buffer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        prepare_buffer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        prepare_buffer.buffer              = mTransferBuffer.mBuffer;
        prepare_buffer.offset              = 0;
        prepare_buffer.size                = byte_count;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                             1, &prepare_buffer, 0, nullptr);

        VkImageMemoryBarrier to_transfer{};
        if (old_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            to_transfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            to_transfer.srcAccessMask                   = source_access;
            to_transfer.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
            to_transfer.oldLayout                       = old_layout;
            to_transfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            to_transfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            to_transfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            to_transfer.image                           = image.mImage;
            to_transfer.subresourceRange.aspectMask     = image.mAspect;
            to_transfer.subresourceRange.baseMipLevel   = 0;
            to_transfer.subresourceRange.levelCount     = 1;
            to_transfer.subresourceRange.baseArrayLayer = 0;
            to_transfer.subresourceRange.layerCount     = 1;
            vkCmdPipelineBarrier(command_buffer, source_stage, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &to_transfer);
        }

        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask     = image.mAspect;
        copy.imageSubresource.mipLevel       = 0;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount     = 1;
        copy.imageExtent                     = { image.mExtent.mWidth, image.mExtent.mHeight, 1 };
        vkCmdCopyImageToBuffer(command_buffer, image.mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               mTransferBuffer.mBuffer, 1, &copy);

        VkBufferMemoryBarrier to_host = prepare_buffer;
        to_host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr,
                             1, &to_host, 0, nullptr);

        if (old_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            VkImageMemoryBarrier restore = to_transfer;
            restore.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            restore.dstAccessMask = source_access;
            restore.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            restore.newLayout     = old_layout;
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, source_stage,
                                 0, 0, nullptr, 0, nullptr, 1, &restore);
        }
    });

    std::vector<std::uint8_t> result(static_cast<std::size_t>(byte_count));
    std::memcpy(result.data(), mTransferBuffer.mMapped, result.size());
    return result;
}

std::array<std::vector<std::uint8_t>, 4> VulkanMaterialRun::targetSnapshot()
{
    return { readImage(mColors[0]), readImage(mColors[1]), readImage(mColors[2]), readImage(mDepth) };
}

void VulkanMaterialRun::runRejections(const MaterialCase& diagnostic_case,
                                      const MaterialFixture& fixture)
{
    std::array<std::uint8_t, sizeof(MaterialParameters)> parameter_sentinel{};
    parameter_sentinel.fill(0xa5U);
    for (const RejectionSpec& rejection : REJECTIONS)
    {
        seedTargets(fixture);
        writeBuffer(mParameterBuffer, parameter_sentinel.data(), parameter_sentinel.size());
        const auto targets_before = targetSnapshot();
        const auto parameters_before = readBuffer(mParameterBuffer, parameter_sentinel.size());
        const std::uint64_t recordings_before = mExecutorRecordings;
        const std::uint64_t submissions_before = mExecutorSubmissions;
        const std::uint32_t validation_before = validationMessageCount();

        FrameSnapshot frame = diagnostic_case.mFrame;
        mutateFrame(frame, rejection.mFrameMutation);
        LLRenderVulkanMaterial::Registry registry = makeRegistry(diagnostic_case, rejection.mRegistryMutation);
        LLRenderVulkanMaterial::ExecutionContext context = executionContext();
        std::string error;
        const bool accepted = LLRenderVulkanMaterial::execute(frame, registry, context, error);

        const auto parameters_after = readBuffer(mParameterBuffer, parameter_sentinel.size());
        const auto targets_after = targetSnapshot();
        if (accepted || error.empty() || mExecutorRecordings != recordings_before ||
            mExecutorSubmissions != submissions_before || parameters_after != parameters_before ||
            targets_after != targets_before || validationMessageCount() != validation_before)
        {
            throw Failure(std::string("fail-closed rejection failed: ") + rejection.mName);
        }
        ++mRejectionCount;
    }
}

MaterialReadback VulkanMaterialRun::readTargets()
{
    MaterialReadback result;
    const auto gbuffer0 = readImage(mColors[0]);
    const auto gbuffer1 = readImage(mColors[1]);
    const auto gbuffer2 = readImage(mColors[2]);
    const auto depth    = readImage(mDepth);

    std::array<std::uint8_t, LLRenderContract::MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT> native0{};
    std::array<std::uint8_t, LLRenderContract::MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT> native1{};
    std::array<std::uint16_t, LLRenderContract::MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT> native2{};
    std::array<float, LLRenderContract::MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT> native_depth{};
    std::memcpy(native0.data(), gbuffer0.data(), gbuffer0.size());
    std::memcpy(native1.data(), gbuffer1.data(), gbuffer1.size());
    std::memcpy(native2.data(), gbuffer2.data(), gbuffer2.size());
    std::memcpy(native_depth.data(), depth.data(), depth.size());

    constexpr std::size_t color_row = LLRenderContract::MATERIAL_FRAME_WIDTH *
                                      LLRenderContract::MATERIAL_DIAGNOSTIC_CHANNELS;
    for (std::size_t bottom_row = 0; bottom_row < FRAME_EXTENT.mHeight; ++bottom_row)
    {
        const std::size_t top_row = FRAME_EXTENT.mHeight - 1 - bottom_row;
        std::copy_n(native0.begin() + top_row * color_row, color_row,
                    result.mGBuffer0.begin() + bottom_row * color_row);
        std::copy_n(native1.begin() + top_row * color_row, color_row,
                    result.mGBuffer1.begin() + bottom_row * color_row);
        std::copy_n(native2.begin() + top_row * color_row, color_row,
                    result.mGBuffer2.begin() + bottom_row * color_row);
        for (std::size_t x = 0; x < FRAME_EXTENT.mWidth; ++x)
        {
            const float value = native_depth[top_row * FRAME_EXTENT.mWidth + x];
            if (!std::isfinite(value) || value < 0.f || value > 1.f)
            {
                throw Failure("D32 material readback is outside normalized depth range");
            }
            constexpr double DEPTH24_MAX = 16777215.0;
            const auto code = static_cast<std::uint32_t>(std::llround(static_cast<double>(value) * DEPTH24_MAX));
            result.mDepth[bottom_row * FRAME_EXTENT.mWidth + x] = code;
        }
    }
    return result;
}

MaterialArtifact VulkanMaterialRun::artifactFrom(const MaterialReadback& readback) const
{
    MaterialArtifact artifact = LLRenderContract::makeMaterialArtifact();
    artifact.mGBuffer0RGBA8.reserve(readback.mGBuffer0.size());
    artifact.mGBuffer1RGBA8.reserve(readback.mGBuffer1.size());
    artifact.mGBuffer2RGBA16.reserve(readback.mGBuffer2.size());
    artifact.mDepth24.reserve(readback.mDepth.size());
    std::transform(readback.mGBuffer0.begin(), readback.mGBuffer0.end(),
                   std::back_inserter(artifact.mGBuffer0RGBA8), LLRenderContract::materialUnorm8);
    std::transform(readback.mGBuffer1.begin(), readback.mGBuffer1.end(),
                   std::back_inserter(artifact.mGBuffer1RGBA8), LLRenderContract::materialUnorm8);
    std::transform(readback.mGBuffer2.begin(), readback.mGBuffer2.end(),
                   std::back_inserter(artifact.mGBuffer2RGBA16), LLRenderContract::materialUnorm16);
    std::transform(readback.mDepth.begin(), readback.mDepth.end(),
                   std::back_inserter(artifact.mDepth24), LLRenderContract::materialDepth24);
    return artifact;
}

template<typename Code, std::size_t Size, std::size_t DepthSize>
bool hasDistinctWrittenPixels(const std::array<Code, Size>& values,
                              const std::array<Code, Size>& sentinel,
                              const std::array<std::uint32_t, DepthSize>& depth,
                              const std::array<std::uint32_t, DepthSize>& depth_sentinel)
{
    static_assert(Size % LLRenderContract::MATERIAL_DIAGNOSTIC_CHANNELS == 0);
    static_assert(Size / LLRenderContract::MATERIAL_DIAGNOSTIC_CHANNELS == DepthSize);
    std::size_t first_pixel = DepthSize;
    bool changed_from_sentinel = false;
    bool distinct = false;
    for (std::size_t pixel = 0; pixel < DepthSize; ++pixel)
    {
        if (depth[pixel] == depth_sentinel[pixel])
        {
            continue;
        }
        if (first_pixel == DepthSize)
        {
            first_pixel = pixel;
        }
        for (std::size_t channel = 0; channel < LLRenderContract::MATERIAL_DIAGNOSTIC_CHANNELS; ++channel)
        {
            const std::size_t component =
                pixel * LLRenderContract::MATERIAL_DIAGNOSTIC_CHANNELS + channel;
            const std::size_t first_component =
                first_pixel * LLRenderContract::MATERIAL_DIAGNOSTIC_CHANNELS + channel;
            changed_from_sentinel = changed_from_sentinel || values[component] != sentinel[component];
            distinct = distinct || values[component] != values[first_component];
        }
    }
    return changed_from_sentinel && distinct;
}

DepthGate VulkanMaterialRun::verifyDepthGate(const MaterialFixture& fixture,
                                             const MaterialReadback& readback) const
{
    struct Vertex
    {
        double                mX;
        double                mY;
        double                mZ;
        double                mReciprocalW;
        std::array<double, 3> mViewOverW;
    };
    std::array<float, 16> positions{};
    std::memcpy(positions.data(), fixture.mVertexBytes.data() + LLRenderContract::MATERIAL_POSITION_OFFSET,
                sizeof(positions));
    std::array<Vertex, 4> vertices{};
    auto transform = [](const std::array<float, 16>& matrix, const float* value)
    {
        std::array<double, 4> result{};
        for (std::size_t row = 0; row < result.size(); ++row)
        {
            for (std::size_t column = 0; column < result.size(); ++column)
            {
                result[row] += static_cast<double>(matrix[column * 4 + row]) * value[column];
            }
        }
        return result;
    };
    for (std::size_t vertex = 0; vertex < vertices.size(); ++vertex)
    {
        const float* position = positions.data() + vertex * 4;
        const std::array<double, 4> clip =
            transform(fixture.mParameters.mModelviewProjectionMatrix, position);
        const std::array<double, 4> view = transform(fixture.mParameters.mModelviewMatrix, position);
        const double reciprocal_w = 1.0 / clip[3];
        vertices[vertex] = {
            (clip[0] * reciprocal_w * 0.5 + 0.5) * LLRenderContract::MATERIAL_FRAME_WIDTH,
            (clip[1] * reciprocal_w * 0.5 + 0.5) * LLRenderContract::MATERIAL_FRAME_HEIGHT,
            clip[2] * reciprocal_w * 0.5 + 0.5,
            reciprocal_w,
            { view[0] * reciprocal_w, view[1] * reciprocal_w, view[2] * reciprocal_w }
        };
    }

    auto cross = [](const Vertex& first, const Vertex& second, double x, double y)
    {
        return (second.mX - first.mX) * (y - first.mY) -
               (second.mY - first.mY) * (x - first.mX);
    };

    DepthGate gate;
    for (std::uint32_t y = 0; y < LLRenderContract::MATERIAL_FRAME_HEIGHT; ++y)
    {
        for (std::uint32_t x = 0; x < LLRenderContract::MATERIAL_FRAME_WIDTH; ++x)
        {
            const double sample_x = x + 0.5;
            const double sample_y = y + 0.5;
            bool covered = false;
            double fragment_depth = 0.0;
            for (std::size_t triangle = 0; triangle < fixture.mIndices.size(); triangle += 3)
            {
                const Vertex& a = vertices[fixture.mIndices[triangle]];
                const Vertex& b = vertices[fixture.mIndices[triangle + 1]];
                const Vertex& c = vertices[fixture.mIndices[triangle + 2]];
                const double area = cross(a, b, c.mX, c.mY);
                const double wa = cross(b, c, sample_x, sample_y) / area;
                const double wb = cross(c, a, sample_x, sample_y) / area;
                const double wc = 1.0 - wa - wb;
                if (std::min({ wa, wb, wc }) <= 0.08)
                {
                    continue;
                }
                fragment_depth = wa * a.mZ + wb * b.mZ + wc * c.mZ;
                const double reciprocal_w =
                    wa * a.mReciprocalW + wb * b.mReciprocalW + wc * c.mReciprocalW;
                std::array<double, 3> view_position{};
                for (std::size_t component = 0; component < view_position.size(); ++component)
                {
                    view_position[component] =
                        (wa * a.mViewOverW[component] + wb * b.mViewOverW[component] +
                         wc * c.mViewOverW[component]) /
                        reciprocal_w;
                }
                const auto& clip_plane = fixture.mParameters.mClipPlane;
                const double clip_distance = view_position[0] * clip_plane[0] +
                                             view_position[1] * clip_plane[1] +
                                             view_position[2] * clip_plane[2] + clip_plane[3];
                if (fixture.mParameters.mMirror > 0.f && clip_distance < 0.0)
                {
                    const std::size_t pixel =
                        static_cast<std::size_t>(y) * LLRenderContract::MATERIAL_FRAME_WIDTH + x;
                    const double loaded_depth = LLRenderContract::materialDepth24(fixture.mDepth24[pixel]);
                    if (std::abs(fragment_depth - loaded_depth) >= 0.02 &&
                        fragment_depth <= loaded_depth)
                    {
                        if (readback.mDepth[pixel] != fixture.mDepth24[pixel])
                        {
                            return gate;
                        }
                        ++gate.mMirrorClippedPasses;
                    }
                    covered = false;
                    break;
                }
                covered = true;
                break;
            }
            if (!covered)
            {
                continue;
            }

            const std::size_t pixel =
                static_cast<std::size_t>(y) * LLRenderContract::MATERIAL_FRAME_WIDTH + x;
            const double loaded_depth = LLRenderContract::materialDepth24(fixture.mDepth24[pixel]);
            if (std::abs(fragment_depth - loaded_depth) < 0.02)
            {
                continue;
            }
            const bool expected_pass = fragment_depth <= loaded_depth;
            const bool depth_changed = readback.mDepth[pixel] != fixture.mDepth24[pixel];
            if (expected_pass != depth_changed)
            {
                return gate;
            }
            if (expected_pass)
            {
                ++gate.mPasses;
            }
            else
            {
                ++gate.mFailures;
            }
        }
    }
    gate.mValid = gate.mPasses > 0 && gate.mFailures > 0 && gate.mMirrorClippedPasses > 0;
    return gate;
}

bool VulkanMaterialRun::nontrivialOutput(const MaterialFixture& fixture,
                                         const MaterialReadback& readback,
                                         DepthGate& gate) const
{
    gate = verifyDepthGate(fixture, readback);
    return readback.mGBuffer0 != fixture.mGBuffer0SentinelRGBA8 &&
           readback.mGBuffer1 != fixture.mGBuffer1SentinelRGBA8 &&
           readback.mGBuffer2 != fixture.mGBuffer2SentinelRGBA16 &&
           hasDistinctWrittenPixels(readback.mGBuffer0, fixture.mGBuffer0SentinelRGBA8,
                                    readback.mDepth, fixture.mDepth24) &&
           hasDistinctWrittenPixels(readback.mGBuffer1, fixture.mGBuffer1SentinelRGBA8,
                                    readback.mDepth, fixture.mDepth24) &&
           hasDistinctWrittenPixels(readback.mGBuffer2, fixture.mGBuffer2SentinelRGBA16,
                                    readback.mDepth, fixture.mDepth24) &&
           gate.mValid;
}

MaterialArtifact VulkanMaterialRun::run()
{
    loadShaderFiles();
    createInstance();
    selectPhysicalDevice();
    createDevice();
    createCommandResources();

    const MaterialFixture fixture = LLRenderContract::makeMaterialFixture();
    const MaterialCase diagnostic_case = LLRenderContract::makeMaterialCase();
    if (diagnostic_case.mInputs.mParameters != fixture.mParameters)
    {
        throw Failure("material case and fixture parameters differ");
    }
    createFixtureBuffers(fixture);
    createTextures(fixture);
    createSampler();
    createDescriptorResources();
    createTargets();
    createPipeline();

    runRejections(diagnostic_case, fixture);
    if (mRejectionCount != REJECTIONS.size() || mExecutorRecordings != 0 || mExecutorSubmissions != 0)
    {
        throw Failure("material rejection matrix did not remain pre-recording");
    }

    seedTargets(fixture);
    std::array<std::uint8_t, sizeof(MaterialParameters)> parameter_sentinel{};
    parameter_sentinel.fill(0xa5U);
    writeBuffer(mParameterBuffer, parameter_sentinel.data(), parameter_sentinel.size());

    LLRenderVulkanMaterial::Registry registry = makeRegistry(diagnostic_case, RegistryMutation::None);
    LLRenderVulkanMaterial::ExecutionContext context = executionContext();
    std::string execution_error;
    if (!LLRenderVulkanMaterial::execute(diagnostic_case.mFrame, registry, context, execution_error))
    {
        throw Failure("valid material replay was rejected: " + execution_error);
    }
    for (Image& color : mColors)
    {
        color.mLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    mDepth.mLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    if (!execution_error.empty() || mExecutorRecordings != 1 || mExecutorSubmissions != 1)
    {
        throw Failure("valid material replay did not record and submit exactly once");
    }

    std::array<std::uint8_t, sizeof(MaterialParameters)> expected_parameters{};
    std::memcpy(expected_parameters.data(), &fixture.mParameters, sizeof(fixture.mParameters));
    const auto actual_parameters = readBuffer(mParameterBuffer, expected_parameters.size());
    if (!std::equal(actual_parameters.begin(), actual_parameters.end(), expected_parameters.begin()))
    {
        throw Failure("valid material replay did not upload the exact 272-byte parameter packet");
    }

    const MaterialReadback readback = readTargets();
    if (!nontrivialOutput(fixture, readback, mDepthGate))
    {
        const bool gbuffer0_distinct = hasDistinctWrittenPixels(
            readback.mGBuffer0, fixture.mGBuffer0SentinelRGBA8, readback.mDepth, fixture.mDepth24);
        const bool gbuffer1_distinct = hasDistinctWrittenPixels(
            readback.mGBuffer1, fixture.mGBuffer1SentinelRGBA8, readback.mDepth, fixture.mDepth24);
        const bool gbuffer2_distinct = hasDistinctWrittenPixels(
            readback.mGBuffer2, fixture.mGBuffer2SentinelRGBA16, readback.mDepth, fixture.mDepth24);
        const auto nonzero0 = std::count_if(readback.mGBuffer0.begin(), readback.mGBuffer0.end(),
                                            [](auto value) { return value != 0; });
        const auto nonzero1 = std::count_if(readback.mGBuffer1.begin(), readback.mGBuffer1.end(),
                                            [](auto value) { return value != 0; });
        const auto nonzero2 = std::count_if(readback.mGBuffer2.begin(), readback.mGBuffer2.end(),
                                            [](auto value) { return value != 0; });
        std::size_t depth_changed = 0;
        for (std::size_t pixel = 0; pixel < readback.mDepth.size(); ++pixel)
        {
            depth_changed += readback.mDepth[pixel] != fixture.mDepth24[pixel] ? 1U : 0U;
        }
        std::ostringstream detail;
        detail << "material nontrivial-output gate failed"
               << "; g0_changed=" << (readback.mGBuffer0 != fixture.mGBuffer0SentinelRGBA8)
               << " g0_distinct=" << gbuffer0_distinct << " g0_nonzero=" << nonzero0
               << "; g1_changed=" << (readback.mGBuffer1 != fixture.mGBuffer1SentinelRGBA8)
               << " g1_distinct=" << gbuffer1_distinct << " g1_nonzero=" << nonzero1
               << "; g2_changed=" << (readback.mGBuffer2 != fixture.mGBuffer2SentinelRGBA16)
               << " g2_distinct=" << gbuffer2_distinct << " g2_nonzero=" << nonzero2
               << "; depth_changed=" << depth_changed
               << " depth_passes=" << mDepthGate.mPasses
               << " depth_failures=" << mDepthGate.mFailures
               << " mirror_clipped_passes=" << mDepthGate.mMirrorClippedPasses;
        throw Failure(detail.str());
    }
    MaterialArtifact artifact = artifactFrom(readback);
    std::string artifact_error;
    if (!LLRenderContract::validateMaterialArtifact(artifact, &artifact_error))
    {
        throw Failure("material artifact is invalid: " + artifact_error);
    }
    return artifact;
}

void VulkanMaterialRun::shutdown() noexcept
{
    if (mDevice != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(mDevice);
        if (mPipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(mDevice, mPipeline, nullptr);
        if (mFramebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(mDevice, mFramebuffer, nullptr);
        if (mRenderPass != VK_NULL_HANDLE)
            vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
        for (Image& color : mColors)
            destroyImage(color);
        destroyImage(mDepth);
        if (mDescriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
        if (mPipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
        for (VkDescriptorSetLayout layout : mDescriptorSetLayouts)
        {
            if (layout != VK_NULL_HANDLE)
                vkDestroyDescriptorSetLayout(mDevice, layout, nullptr);
        }
        if (mSampler != VK_NULL_HANDLE)
            vkDestroySampler(mDevice, mSampler, nullptr);
        for (Image& texture : mTextures)
            destroyImage(texture);
        destroyBuffer(mParameterBuffer);
        destroyBuffer(mIndexBuffer);
        destroyBuffer(mVertexBuffer);
        destroyBuffer(mTransferBuffer);
        if (mCommandPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        vkDestroyDevice(mDevice, nullptr);
    }
    mPipeline = VK_NULL_HANDLE;
    mFramebuffer = VK_NULL_HANDLE;
    mRenderPass = VK_NULL_HANDLE;
    mDescriptorPool = VK_NULL_HANDLE;
    mPipelineLayout = VK_NULL_HANDLE;
    mDescriptorSetLayouts = {};
    mDescriptorSets = {};
    mSampler = VK_NULL_HANDLE;
    mCommandPool = VK_NULL_HANDLE;
    mCommandBuffer = VK_NULL_HANDLE;
    mQueue = VK_NULL_HANDLE;
    mDevice = VK_NULL_HANDLE;
    mPhysicalDevice = VK_NULL_HANDLE;

    if (mDebugMessenger != VK_NULL_HANDLE && mInstance != VK_NULL_HANDLE && mGlobalDispatch)
    {
        const auto destroy_debug = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            mGlobalDispatch->getInstanceProcAddr()(mInstance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroy_debug)
            destroy_debug(mInstance, mDebugMessenger, nullptr);
    }
    if (mInstance != VK_NULL_HANDLE)
        vkDestroyInstance(mInstance, nullptr);
    mDebugMessenger = VK_NULL_HANDLE;
    mInstance = VK_NULL_HANDLE;
    mGlobalDispatch.reset();
}

int fail(const std::string& reason, const std::string& detail)
{
    std::cerr << "VULKAN_MATERIAL result=fail reason=" << reason;
    if (!detail.empty())
    {
        std::cerr << " detail={" << detail << '}';
    }
    std::cerr << '\n';
    return 1;
}

} // namespace

int main(int argc, char** argv)
{
    Options options;
    std::string option_error;
    if (!parseOptions(argc, argv, options, option_error))
    {
        fail("usage", option_error);
        std::cerr << "usage: llvulkanmaterial --shader-dir <directory> --output <artifact>\n";
        return 2;
    }

    try
    {
        std::error_code file_error;
        if (std::filesystem::exists(options.mOutput, file_error) || file_error)
        {
            throw Failure(file_error ? "cannot inspect output path: " + file_error.message()
                                    : "output artifact already exists: " + options.mOutput.string());
        }

        VulkanMaterialRun runner(options.mShaderDirectory);
        MaterialArtifact artifact = runner.run();
        runner.shutdown();
        if (runner.validationMessageCount() != 0)
        {
            std::ostringstream message;
            message << runner.validationMessageCount() << " validation messages";
            const std::string first = runner.firstValidationMessage();
            if (!first.empty())
            {
                message << "; first: " << first;
            }
            throw Failure(message.str());
        }

        std::string artifact_error;
        if (!LLRenderContract::writeMaterialArtifact(options.mOutput, artifact, &artifact_error))
        {
            throw Failure(artifact_error);
        }

        const DepthGate& depth = runner.depthGate();
        std::cout << "VULKAN_MATERIAL result=pass"
                  << " case=nonrigged_normspec_indexed"
                  << " components="
                  << (LLRenderContract::MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT * 3 +
                      LLRenderContract::MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT)
                  << " rejection_cases=" << runner.rejectionCount()
                  << " recordings=" << runner.executorRecordingCount()
                  << " submissions=" << runner.executorSubmissionCount()
                  << " depth_passes=" << depth.mPasses
                  << " depth_failures=" << depth.mFailures
                  << " mirror_clipped_passes=" << depth.mMirrorClippedPasses
                  << " validation_messages=" << runner.validationMessageCount()
                  << " vendor_id=" << runner.vendorId()
                  << " device_id=" << runner.deviceId()
                  << " api_version=" << runner.apiVersion()
                  << " driver_version=" << runner.driverVersion()
                  << " portability_enumeration="
                  << (runner.usedPortabilityEnumeration() ? "enabled" : "not_advertised")
                  << " portability_subset="
                  << (runner.usedPortabilitySubset() ? "enabled" : "not_advertised")
                  << " artifact=written\n";
        return 0;
    }
    catch (const CapabilityFailure& exception)
    {
        return fail("capability", exception.what());
    }
    catch (const std::exception& exception)
    {
        return fail("execution", exception.what());
    }
}
