/**
 * @file llvulkantextureupload_main.cpp
 * @brief Offscreen Vulkan replay for the fixed streamed-texture upload.
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

#include "llrendervulkantextureupload.h"
#include "lltextureuploaddiagnostic.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
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
using LLRenderContract::StreamingUploadLifecycle;
using LLRenderContract::TextureUploadArtifact;
using LLRenderContract::TextureUploadCase;
using LLRenderContract::TextureUploadFixture;
using LLRenderVulkanTextureUpload::ShaderIdentityToken;

constexpr char PORTABILITY_ENUMERATION_EXTENSION[] = "VK_KHR_portability_enumeration";
constexpr char PORTABILITY_SUBSET_EXTENSION[]      = "VK_KHR_portability_subset";
constexpr VkFormat COLOR_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
constexpr Extent2D RESIDENT_EXTENT{ LLRenderContract::TEXTURE_UPLOAD_RESIDENT_WIDTH,
                                    LLRenderContract::TEXTURE_UPLOAD_RESIDENT_HEIGHT };
constexpr Extent2D OUTPUT_EXTENT{ LLRenderContract::TEXTURE_UPLOAD_OUTPUT_WIDTH,
                                  LLRenderContract::TEXTURE_UPLOAD_OUTPUT_HEIGHT };
constexpr VkDeviceSize SCREEN_BYTES = 48;
constexpr VkDeviceSize STAGING_BYTES = LLRenderContract::TEXTURE_UPLOAD_SOURCE_BYTE_COUNT;
constexpr VkDeviceSize READBACK_BYTES = LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_COUNT +
                                        LLRenderContract::TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT;
constexpr VkImageUsageFlags REPLACEMENT_USAGE = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                 VK_IMAGE_USAGE_SAMPLED_BIT;
constexpr VkImageUsageFlags OUTPUT_USAGE = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                            VK_IMAGE_USAGE_SAMPLED_BIT;

static_assert(SCREEN_BYTES == 48);
static_assert(STAGING_BYTES == 144);
static_assert(READBACK_BYTES == 200);

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

LLRenderVulkanTextureUpload::NativeOwnershipToken nextOwnershipToken()
{
    static std::atomic<LLRenderVulkanTextureUpload::NativeOwnershipToken> next{ 1 };
    auto candidate = next.load(std::memory_order_relaxed);
    for (;;)
    {
        if (candidate == 0 || candidate == std::numeric_limits<decltype(candidate)>::max())
        {
            throw Failure("Vulkan diagnostic ownership tokens are exhausted");
        }
        if (next.compare_exchange_weak(candidate, candidate + 1,
                                       std::memory_order_relaxed,
                                       std::memory_order_relaxed))
        {
            return candidate;
        }
    }
}

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
    bool have_output = false;
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
            have_shader_directory = true;
        }
        else
        {
            if (have_output)
            {
                error = "--output was specified more than once";
                return false;
            }
            options.mOutput = value;
            have_output = true;
        }
    }
    if (!have_shader_directory || !have_output)
    {
        error = "both --shader-dir and --output are required";
        return false;
    }
    return true;
}

bool occupiedPath(const std::filesystem::path& path, std::string& error)
{
    std::error_code status_error;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, status_error);
    if (!status_error)
    {
        return status.type() != std::filesystem::file_type::not_found;
    }
    if (status_error == std::errc::no_such_file_or_directory)
    {
        return false;
    }
    error = "cannot inspect output path: " + status_error.message();
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
        0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6U, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
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
    for (std::size_t offset = 0; offset < padded.size(); offset += 64)
    {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t index = 0; index < 16; ++index)
        {
            const std::size_t byte = offset + index * 4;
            words[index] = static_cast<std::uint32_t>(padded[byte]) << 24 |
                           static_cast<std::uint32_t>(padded[byte + 1]) << 16 |
                           static_cast<std::uint32_t>(padded[byte + 2]) << 8 |
                           static_cast<std::uint32_t>(padded[byte + 3]);
        }
        for (std::size_t index = 16; index < words.size(); ++index)
        {
            const std::uint32_t s0 = rotateRight(words[index - 15], 7) ^ rotateRight(words[index - 15], 18) ^
                                     (words[index - 15] >> 3);
            const std::uint32_t s1 = rotateRight(words[index - 2], 17) ^ rotateRight(words[index - 2], 19) ^
                                     (words[index - 2] >> 10);
            words[index] = words[index - 16] + s0 + words[index - 7] + s1;
        }
        std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
        for (std::size_t index = 0; index < words.size(); ++index)
        {
            const std::uint32_t s1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            const std::uint32_t choose = (e & f) ^ (~e & g);
            const std::uint32_t temporary1 = h + s1 + choose + K[index] + words[index];
            const std::uint32_t s0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temporary2 = s0 + majority;
            h = g; g = f; f = e; e = d + temporary1;
            d = c; c = b; b = a; a = temporary1 + temporary2;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }
    ShaderIdentityToken result{};
    for (std::size_t index = 0; index < state.size(); ++index)
    {
        result[index * 4]     = static_cast<std::uint8_t>(state[index] >> 24);
        result[index * 4 + 1] = static_cast<std::uint8_t>(state[index] >> 16);
        result[index * 4 + 2] = static_cast<std::uint8_t>(state[index] >> 8);
        result[index * 4 + 3] = static_cast<std::uint8_t>(state[index]);
    }
    return result;
}

struct Buffer
{
    VkBuffer              mBuffer = VK_NULL_HANDLE;
    VkDeviceMemory        mMemory = VK_NULL_HANDLE;
    void*                 mMapped = nullptr;
    VkDeviceSize          mSize = 0;
    VkDeviceSize          mAllocationSize = 0;
    VkBufferUsageFlags    mUsage = 0;
    VkMemoryPropertyFlags mMemoryProperties = 0;
};

struct Image
{
    VkImage               mImage = VK_NULL_HANDLE;
    VkDeviceMemory        mMemory = VK_NULL_HANDLE;
    VkImageView           mView = VK_NULL_HANDLE;
    VkFormat              mFormat = VK_FORMAT_UNDEFINED;
    Extent2D              mExtent;
    std::uint32_t         mMipLevels = 0;
    VkImageUsageFlags     mUsage = 0;
    VkDeviceSize          mAllocationSize = 0;
    VkImageLayout         mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageSubresourceRange mViewRange{};
};

enum class RegistryMutation
{
    None,
    StaleScreen,
    StaleOld,
    StaleReplacement,
    StaleOutput,
    StaleSampler,
    StalePipeline,
    WrongScreenSize,
    WrongScreenUsage,
    WrongStagingSize,
    WrongStagingUsage,
    WrongStagingMemory,
    WrongReadbackSize,
    WrongReadbackUsage,
    WrongReadbackMemory,
    AliasBuffers,
    AliasBufferMemory,
    WrongOldFormat,
    WrongOldExtent,
    WrongOldMips,
    WrongOldUsage,
    WrongOldLayout,
    WrongOldViewRange,
    WrongOldSnapshot,
    WrongReplacementFormat,
    WrongReplacementExtent,
    WrongReplacementMips,
    WrongReplacementUsage,
    WrongReplacementLayout,
    WrongReplacementViewRange,
    WrongOutputFormat,
    WrongOutputExtent,
    WrongOutputMips,
    WrongOutputUsage,
    WrongOutputLayout,
    WrongOutputViewRange,
    AliasImages,
    AliasImageMemory,
    AliasViews,
    WrongSampler,
    WrongProgram,
    WrongVariant,
    WrongPipelineExtent,
    WrongDescriptor,
    WrongRenderPass,
    WrongFramebuffer,
    WrongVertexState,
    WrongViewportPolicy,
    WrongRasterState,
    WrongShaderIdentity,
    MissingLifecycle,
    WrongLifecycleCurrent,
    WrongLifecycleRevision,
    PendingLifecycle,
    PriorCompletion,
    PriorRetirement,
    WrongOwnershipToken
};

enum class PacketMutation
{
    None,
    WrongFrame,
    WrongUploadDestinationGeneration,
    WrongOldPacketGeneration,
    WrongOutputPacketGeneration,
    PacketImageAlias,
    WrongOldPacketMipCount,
    WrongReplacementPacketMipCount,
    WrongOutputPacketMipCount,
    DuplicateRevision,
    WrongSubresource,
    WrongOffset,
    WrongResidentExtent,
    WrongLogicalExtent,
    WrongResidentDiscard,
    WrongRowPitch,
    WrongRowOrigin,
    WrongSourceFormat,
    WrongMipGeneration,
    WrongBefore,
    WrongDuring,
    WrongAfter,
    WrongPixelOffset,
    WrongPixelSize,
    WrongPixelStorageSize,
    WrongSamplerPacket,
    MissingSamplerPacket,
    WrongPipelinePacket,
    MissingPipelinePacket,
    WrongOutputPacketExtent,
    WrongReleaseFrame,
    WrongRelease,
    MissingRelease,
    ExtraPass,
    MissingPass,
    WrongViewport,
    MissingDraw,
    ExtraDraw,
    WrongSampleRange
};

enum class ContextMutation
{
    None,
    NullDevice,
    NullCommandBuffer,
    NullQueue,
    MissingRecordingCounter,
    MissingSubmissionCounter,
    WrongVertexIdentity,
    WrongFragmentIdentity,
    NullCommandPool,
    WrongOwnershipToken,
    IgnoredQueueFamily,
    MissingGraphicsQueueFlag,
    MissingTransferQueueFlag,
    WrongQueueCount,
    WrongQueueIndex,
    WrongCommandPoolFamily,
    MissingCommandPoolReset,
    WrongCommandBufferLevel
};

struct RejectionSpec
{
    const char*      mName;
    RegistryMutation mRegistry = RegistryMutation::None;
    PacketMutation   mPacket = PacketMutation::None;
    ContextMutation  mContext = ContextMutation::None;
};

class VulkanTextureUploadRun
{
public:
    explicit VulkanTextureUploadRun(std::filesystem::path shader_directory)
        : mShaderDirectory(std::move(shader_directory)),
          mOwnershipToken(nextOwnershipToken())
    {
    }

    ~VulkanTextureUploadRun() { shutdown(); }

    TextureUploadArtifact run();
    void shutdown() noexcept;

    std::uint32_t validationMessageCount() const noexcept { return mValidation.mMessages.load(); }
    std::uint64_t recordingAttemptCount() const noexcept { return mExecutorRecordings; }
    std::uint64_t submissionCount() const noexcept { return mExecutorSubmissions; }
    std::size_t rejectionCount() const noexcept { return mRejectionCount; }
    bool usedPortabilityEnumeration() const noexcept { return mPortabilityEnumeration; }
    bool usedPortabilitySubset() const noexcept { return mPortabilitySubset; }

private:
    void readShaders();
    void createInstance();
    bool hasRequiredFormats(VkPhysicalDevice physical_device) const;
    bool hasRequiredMemory(VkPhysicalDevice physical_device) const;
    std::optional<std::uint32_t> graphicsQueueFamily(VkPhysicalDevice physical_device) const;
    std::vector<VkExtensionProperties> deviceExtensions(VkPhysicalDevice physical_device) const;
    void selectPhysicalDevice();
    void createDevice();
    void createCommandResources();
    std::uint32_t memoryType(std::uint32_t bits, VkMemoryPropertyFlags required,
                             VkMemoryPropertyFlags preferred, VkMemoryPropertyFlags& selected) const;
    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool mapped);
    Image createImage(Extent2D extent, std::uint32_t mip_levels, VkImageUsageFlags usage);
    void destroyBuffer(Buffer& buffer) noexcept;
    void destroyImage(Image& image) noexcept;
    void createResources(const TextureUploadFixture& fixture);
    void createDescriptors();
    void createRenderPass();
    void createPipeline();
    void seedImages(const TextureUploadFixture& fixture);
    void submitDiagnostic(const std::function<void(VkCommandBuffer)>& recorder);
    void writeBuffer(Buffer& buffer, const void* source, std::size_t size);
    std::vector<std::uint8_t> readBuffer(const Buffer& buffer, std::size_t size);
    std::vector<std::uint8_t> snapshotImage(Image& image);
    void transition(Image& image, VkImageLayout old_layout, VkImageLayout new_layout,
                    VkAccessFlags source_access, VkAccessFlags destination_access,
                    VkPipelineStageFlags source_stage, VkPipelineStageFlags destination_stage);
    FrameSnapshot mutateFrame(const TextureUploadCase& diagnostic_case, PacketMutation mutation) const;
    LLRenderVulkanTextureUpload::Registry makeRegistry(const TextureUploadCase& diagnostic_case,
                                                       RegistryMutation mutation,
                                                       StreamingUploadLifecycle* lifecycle);
    LLRenderVulkanTextureUpload::ExecutionContext executionContext(ContextMutation mutation = ContextMutation::None);
    void runRejections(const TextureUploadCase& diagnostic_case, const TextureUploadFixture& fixture);

    std::filesystem::path mShaderDirectory;
    LLRenderVulkanTextureUpload::NativeOwnershipToken mOwnershipToken = 0;
    ValidationState       mValidation;
    VkInstance            mInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice      mPhysicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties mDeviceProperties{};
    std::uint32_t         mQueueFamily = 0;
    VkQueueFlags          mQueueFamilyFlags = 0;
    VkDevice              mDevice = VK_NULL_HANDLE;
    VkQueue               mQueue = VK_NULL_HANDLE;
    VkCommandPool         mCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer       mCommandBuffer = VK_NULL_HANDLE;
    Buffer                mScreen;
    Buffer                mStaging;
    Buffer                mReadback;
    Image                 mOld;
    Image                 mReplacement;
    Image                 mOutputImage;
    VkSampler             mSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool      mDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet       mDescriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout      mPipelineLayout = VK_NULL_HANDLE;
    VkRenderPass          mRenderPass = VK_NULL_HANDLE;
    VkFramebuffer         mFramebuffer = VK_NULL_HANDLE;
    VkPipeline            mPipeline = VK_NULL_HANDLE;
    std::vector<std::uint32_t> mVertexSpirv;
    std::vector<std::uint32_t> mFragmentSpirv;
    ShaderIdentityToken   mVertexIdentity{};
    ShaderIdentityToken   mFragmentIdentity{};
    std::array<std::uint8_t, LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_COUNT> mOldPreExecutionSnapshot{};
    std::uint64_t         mExecutorRecordings = 0;
    std::uint64_t         mExecutorSubmissions = 0;
    std::size_t           mRejectionCount = 0;
    bool                  mPortabilityEnumeration = false;
    bool                  mPortabilitySubset = false;
};

void VulkanTextureUploadRun::readShaders()
{
    std::error_code directory_error;
    if (!std::filesystem::is_directory(mShaderDirectory, directory_error) || directory_error)
    {
        throw Failure("shader directory is not readable: " + mShaderDirectory.string());
    }
    const auto read = [](const std::filesystem::path& path, std::vector<std::uint32_t>& words,
                         ShaderIdentityToken& identity)
    {
        std::error_code size_error;
        const std::uintmax_t size = std::filesystem::file_size(path, size_error);
        if (size_error || size == 0 || size % sizeof(std::uint32_t) != 0 || size > 16U * 1024U * 1024U)
        {
            throw Failure("SPIR-V file is missing or has an invalid size: " + path.string());
        }
        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            throw Failure("cannot open SPIR-V file: " + path.string());
        }
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
    read(mShaderDirectory / "textureupload.vert.spv", mVertexSpirv, mVertexIdentity);
    read(mShaderDirectory / "textureupload.frag.spv", mFragmentSpirv, mFragmentIdentity);
}

void VulkanTextureUploadRun::createInstance()
{
    const auto layers = enumerate<VkLayerProperties>(
        [](std::uint32_t* count, VkLayerProperties* values)
        {
            return vkEnumerateInstanceLayerProperties(count, values);
        },
        "vkEnumerateInstanceLayerProperties");
    if (!hasName(layers, "VK_LAYER_KHRONOS_validation"))
    {
        throw CapabilityFailure("VK_LAYER_KHRONOS_validation is required but unavailable");
    }
    const auto extensions = enumerate<VkExtensionProperties>(
        [](std::uint32_t* count, VkExtensionProperties* values)
        {
            return vkEnumerateInstanceExtensionProperties(nullptr, count, values);
        },
        "vkEnumerateInstanceExtensionProperties");
    if (!hasName(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
    {
        throw CapabilityFailure("VK_EXT_debug_utils is required but unavailable");
    }
    std::uint32_t loader_version = VK_API_VERSION_1_0;
    const auto enumerate_version = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
        vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
    if (enumerate_version)
    {
        check(enumerate_version(&loader_version), "vkEnumerateInstanceVersion");
    }
    if (loader_version < VK_API_VERSION_1_1)
    {
        throw CapabilityFailure("the Vulkan 1.1 loader required by the shader target is unavailable");
    }

    std::vector<const char*> enabled_extensions{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    VkInstanceCreateFlags flags = 0;
    if (hasName(extensions, PORTABILITY_ENUMERATION_EXTENSION))
    {
        enabled_extensions.push_back(PORTABILITY_ENUMERATION_EXTENSION);
        flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        mPortabilityEnumeration = true;
    }

    VkDebugUtilsMessengerCreateInfoEXT debug_info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = validationCallback;
    debug_info.pUserData = &mValidation;

    VkApplicationInfo application{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    application.pApplicationName = "llvulkantextureupload";
    application.applicationVersion = 1;
    application.pEngineName = "Second Life texture upload diagnostic";
    application.engineVersion = 1;
    application.apiVersion = VK_API_VERSION_1_1;

    const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    VkInstanceCreateInfo create_info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    create_info.pNext = &debug_info;
    create_info.flags = flags;
    create_info.pApplicationInfo = &application;
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = &validation_layer;
    create_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_extensions.size());
    create_info.ppEnabledExtensionNames = enabled_extensions.data();
    check(vkCreateInstance(&create_info, nullptr, &mInstance), "vkCreateInstance");

    const auto create_debug = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT"));
    if (!create_debug)
    {
        throw Failure("vkCreateDebugUtilsMessengerEXT is unavailable after enabling VK_EXT_debug_utils");
    }
    check(create_debug(mInstance, &debug_info, nullptr, &mDebugMessenger),
          "vkCreateDebugUtilsMessengerEXT");
}

bool VulkanTextureUploadRun::hasRequiredFormats(VkPhysicalDevice physical_device) const
{
    VkFormatProperties properties{};
    vkGetPhysicalDeviceFormatProperties(physical_device, COLOR_FORMAT, &properties);
    constexpr VkFormatFeatureFlags REQUIRED = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                                               VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
                                               VK_FORMAT_FEATURE_BLIT_SRC_BIT |
                                               VK_FORMAT_FEATURE_BLIT_DST_BIT |
                                               VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
                                               VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
                                               VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if ((properties.optimalTilingFeatures & REQUIRED) != REQUIRED)
    {
        return false;
    }
    VkFormatProperties vertex_properties{};
    vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_R32G32B32_SFLOAT, &vertex_properties);
    if ((vertex_properties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) == 0)
    {
        return false;
    }

    struct Requirement
    {
        Extent2D mExtent;
        std::uint32_t mMips;
        VkImageUsageFlags mUsage;
    };
    constexpr std::array requirements{
        Requirement{ RESIDENT_EXTENT, LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS,
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                         VK_IMAGE_USAGE_SAMPLED_BIT },
        Requirement{ OUTPUT_EXTENT, 1, OUTPUT_USAGE }
    };
    for (const Requirement& requirement : requirements)
    {
        VkImageFormatProperties image_properties{};
        if (vkGetPhysicalDeviceImageFormatProperties(physical_device, COLOR_FORMAT, VK_IMAGE_TYPE_2D,
                                                     VK_IMAGE_TILING_OPTIMAL, requirement.mUsage, 0,
                                                     &image_properties) != VK_SUCCESS ||
            image_properties.maxExtent.width < requirement.mExtent.mWidth ||
            image_properties.maxExtent.height < requirement.mExtent.mHeight ||
            image_properties.maxMipLevels < requirement.mMips || image_properties.maxArrayLayers < 1 ||
            (image_properties.sampleCounts & VK_SAMPLE_COUNT_1_BIT) == 0)
        {
            return false;
        }
    }
    return true;
}

bool VulkanTextureUploadRun::hasRequiredMemory(VkPhysicalDevice physical_device) const
{
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);
    for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index)
    {
        if ((properties.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
        {
            return true;
        }
    }
    return false;
}

std::optional<std::uint32_t> VulkanTextureUploadRun::graphicsQueueFamily(VkPhysicalDevice physical_device) const
{
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties.data());
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const VkQueueFlags required = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
        if (properties[index].queueCount != 0 && (properties[index].queueFlags & required) == required)
        {
            return index;
        }
    }
    return std::nullopt;
}

std::vector<VkExtensionProperties> VulkanTextureUploadRun::deviceExtensions(
    VkPhysicalDevice physical_device) const
{
    return enumerate<VkExtensionProperties>(
        [physical_device](std::uint32_t* count, VkExtensionProperties* values)
        {
            return vkEnumerateDeviceExtensionProperties(physical_device, nullptr, count, values);
        },
        "vkEnumerateDeviceExtensionProperties");
}

void VulkanTextureUploadRun::selectPhysicalDevice()
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
        vkGetPhysicalDeviceProperties(device, &properties);
        const auto queue_family = graphicsQueueFamily(device);
        const auto& limits = properties.limits;
        if (properties.apiVersion < VK_API_VERSION_1_1 || !queue_family || !hasRequiredFormats(device) ||
            !hasRequiredMemory(device) || limits.maxFramebufferWidth < OUTPUT_EXTENT.mWidth ||
            limits.maxFramebufferHeight < OUTPUT_EXTENT.mHeight || limits.maxImageDimension2D < RESIDENT_EXTENT.mWidth ||
            limits.maxVertexInputBindings < 1 || limits.maxVertexInputAttributes < 1 ||
            limits.maxPerStageDescriptorSamplers < 1 || limits.maxDescriptorSetSamplers < 1 ||
            limits.maxPerStageDescriptorSampledImages < 1 || limits.maxDescriptorSetSampledImages < 1 ||
            limits.maxBoundDescriptorSets < 1)
        {
            continue;
        }
        mPhysicalDevice = device;
        mDeviceProperties = properties;
        mQueueFamily = *queue_family;
        std::uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_properties(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_properties.data());
        mQueueFamilyFlags = queue_properties[mQueueFamily].queueFlags;
        mPortabilitySubset = hasName(deviceExtensions(device), PORTABILITY_SUBSET_EXTENSION);
        return;
    }
    throw CapabilityFailure("no Vulkan 1.1 graphics device supports the exact RGBA8 upload, linear blit, sample, and readback requirements");
}

void VulkanTextureUploadRun::createDevice()
{
    const float priority = 1.f;
    VkDeviceQueueCreateInfo queue_info{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queue_info.queueFamilyIndex = mQueueFamily;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    std::vector<const char*> extensions;
    if (mPortabilitySubset)
    {
        extensions.push_back(PORTABILITY_SUBSET_EXTENSION);
    }
    VkDeviceCreateInfo create_info{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_info;
    create_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    check(vkCreateDevice(mPhysicalDevice, &create_info, nullptr, &mDevice), "vkCreateDevice");
    vkGetDeviceQueue(mDevice, mQueueFamily, 0, &mQueue);
    if (mQueue == VK_NULL_HANDLE)
    {
        throw Failure("vkGetDeviceQueue returned a null graphics queue");
    }
}

void VulkanTextureUploadRun::createCommandResources()
{
    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = mQueueFamily;
    check(vkCreateCommandPool(mDevice, &pool_info, nullptr, &mCommandPool), "vkCreateCommandPool");
    VkCommandBufferAllocateInfo allocation{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocation.commandPool = mCommandPool;
    allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocation.commandBufferCount = 1;
    check(vkAllocateCommandBuffers(mDevice, &allocation, &mCommandBuffer), "vkAllocateCommandBuffers");
}

std::uint32_t VulkanTextureUploadRun::memoryType(std::uint32_t bits, VkMemoryPropertyFlags required,
                                                 VkMemoryPropertyFlags preferred,
                                                 VkMemoryPropertyFlags& selected) const
{
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &properties);
    for (int pass = 0; pass < 2; ++pass)
    {
        for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index)
        {
            const VkMemoryPropertyFlags flags = properties.memoryTypes[index].propertyFlags;
            if ((bits & (1U << index)) != 0 && (flags & required) == required &&
                (pass != 0 || (flags & preferred) == preferred))
            {
                selected = flags;
                return index;
            }
        }
    }
    throw Failure("no Vulkan memory type satisfies the required properties");
}

Buffer VulkanTextureUploadRun::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool mapped)
{
    Buffer result;
    result.mSize = size;
    result.mUsage = usage;
    VkBufferCreateInfo create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    check(vkCreateBuffer(mDevice, &create_info, nullptr, &result.mBuffer), "vkCreateBuffer");
    try
    {
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(mDevice, result.mBuffer, &requirements);
        const VkMemoryPropertyFlags required = mapped ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0;
        const VkMemoryPropertyFlags preferred = mapped ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkMemoryPropertyFlags selected = 0;
        const std::uint32_t memory_type = memoryType(requirements.memoryTypeBits, required, preferred, selected);
        result.mAllocationSize = requirements.size;
        result.mMemoryProperties = selected;
        VkMemoryAllocateInfo allocation{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memory_type;
        check(vkAllocateMemory(mDevice, &allocation, nullptr, &result.mMemory), "vkAllocateMemory(buffer)");
        check(vkBindBufferMemory(mDevice, result.mBuffer, result.mMemory, 0), "vkBindBufferMemory");
        if (mapped)
        {
            check(vkMapMemory(mDevice, result.mMemory, 0, VK_WHOLE_SIZE, 0, &result.mMapped), "vkMapMemory(buffer)");
        }
        return result;
    }
    catch (...)
    {
        destroyBuffer(result);
        throw;
    }
}

Image VulkanTextureUploadRun::createImage(Extent2D extent, std::uint32_t mip_levels,
                                           VkImageUsageFlags usage)
{
    Image result;
    result.mFormat = COLOR_FORMAT;
    result.mExtent = extent;
    result.mMipLevels = mip_levels;
    result.mUsage = usage;
    result.mViewRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, 1 };
    VkImageCreateInfo create_info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = COLOR_FORMAT;
    create_info.extent = { extent.mWidth, extent.mHeight, 1 };
    create_info.mipLevels = mip_levels;
    create_info.arrayLayers = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    check(vkCreateImage(mDevice, &create_info, nullptr, &result.mImage), "vkCreateImage");
    try
    {
        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(mDevice, result.mImage, &requirements);
        VkMemoryPropertyFlags selected = 0;
        const std::uint32_t memory_type = memoryType(requirements.memoryTypeBits, 0,
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, selected);
        result.mAllocationSize = requirements.size;
        VkMemoryAllocateInfo allocation{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memory_type;
        check(vkAllocateMemory(mDevice, &allocation, nullptr, &result.mMemory), "vkAllocateMemory(image)");
        check(vkBindImageMemory(mDevice, result.mImage, result.mMemory, 0), "vkBindImageMemory");
        VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = result.mImage;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = COLOR_FORMAT;
        view_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                 VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        view_info.subresourceRange = result.mViewRange;
        check(vkCreateImageView(mDevice, &view_info, nullptr, &result.mView), "vkCreateImageView");
        return result;
    }
    catch (...)
    {
        destroyImage(result);
        throw;
    }
}

void VulkanTextureUploadRun::destroyBuffer(Buffer& buffer) noexcept
{
    if (buffer.mMapped && buffer.mMemory != VK_NULL_HANDLE)
    {
        vkUnmapMemory(mDevice, buffer.mMemory);
    }
    if (buffer.mBuffer != VK_NULL_HANDLE) vkDestroyBuffer(mDevice, buffer.mBuffer, nullptr);
    if (buffer.mMemory != VK_NULL_HANDLE) vkFreeMemory(mDevice, buffer.mMemory, nullptr);
    buffer = {};
}

void VulkanTextureUploadRun::destroyImage(Image& image) noexcept
{
    if (image.mView != VK_NULL_HANDLE) vkDestroyImageView(mDevice, image.mView, nullptr);
    if (image.mImage != VK_NULL_HANDLE) vkDestroyImage(mDevice, image.mImage, nullptr);
    if (image.mMemory != VK_NULL_HANDLE) vkFreeMemory(mDevice, image.mMemory, nullptr);
    image = {};
}

void VulkanTextureUploadRun::writeBuffer(Buffer& buffer, const void* source, std::size_t size)
{
    if (!buffer.mMapped || size > buffer.mSize)
    {
        throw Failure("mapped buffer write exceeds the fixed allocation");
    }
    std::memcpy(buffer.mMapped, source, size);
    if ((buffer.mMemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
        VkMappedMemoryRange range{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        range.memory = buffer.mMemory;
        range.offset = 0;
        range.size = VK_WHOLE_SIZE;
        check(vkFlushMappedMemoryRanges(mDevice, 1, &range), "vkFlushMappedMemoryRanges");
    }
}

std::vector<std::uint8_t> VulkanTextureUploadRun::readBuffer(const Buffer& buffer, std::size_t size)
{
    if (!buffer.mMapped || size > buffer.mSize)
    {
        throw Failure("mapped buffer read exceeds the fixed allocation");
    }
    if ((buffer.mMemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
        VkMappedMemoryRange range{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        range.memory = buffer.mMemory;
        range.offset = 0;
        range.size = VK_WHOLE_SIZE;
        check(vkInvalidateMappedMemoryRanges(mDevice, 1, &range), "vkInvalidateMappedMemoryRanges");
    }
    const auto* begin = static_cast<const std::uint8_t*>(buffer.mMapped);
    return { begin, begin + size };
}

void VulkanTextureUploadRun::submitDiagnostic(const std::function<void(VkCommandBuffer)>& recorder)
{
    check(vkResetCommandBuffer(mCommandBuffer, 0), "vkResetCommandBuffer(diagnostic)");
    VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(mCommandBuffer, &begin), "vkBeginCommandBuffer(diagnostic)");
    recorder(mCommandBuffer);
    check(vkEndCommandBuffer(mCommandBuffer), "vkEndCommandBuffer(diagnostic)");
    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &mCommandBuffer;
    check(vkQueueSubmit(mQueue, 1, &submit, VK_NULL_HANDLE), "vkQueueSubmit(diagnostic)");
    check(vkQueueWaitIdle(mQueue), "vkQueueWaitIdle(diagnostic)");
}

void VulkanTextureUploadRun::transition(Image& image, VkImageLayout old_layout, VkImageLayout new_layout,
                                         VkAccessFlags source_access, VkAccessFlags destination_access,
                                         VkPipelineStageFlags source_stage,
                                         VkPipelineStageFlags destination_stage)
{
    submitDiagnostic([&](VkCommandBuffer command_buffer)
    {
        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.srcAccessMask = source_access;
        barrier.dstAccessMask = destination_access;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.mImage;
        barrier.subresourceRange = image.mViewRange;
        vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);
    });
    image.mLayout = new_layout;
}

void VulkanTextureUploadRun::createResources(const TextureUploadFixture& fixture)
{
    mScreen = createBuffer(SCREEN_BYTES, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
    mStaging = createBuffer(STAGING_BYTES, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    mReadback = createBuffer(READBACK_BYTES, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
    writeBuffer(mScreen, fixture.mScreenTriangle.data(), static_cast<std::size_t>(SCREEN_BYTES));

    mOld = createImage(RESIDENT_EXTENT, LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS, REPLACEMENT_USAGE);
    mReplacement = createImage(RESIDENT_EXTENT, LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS, REPLACEMENT_USAGE);
    mOutputImage = createImage(OUTPUT_EXTENT, 1, OUTPUT_USAGE);

    VkSamplerCreateInfo sampler_info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 0.f;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = 0.f;
    sampler_info.maxLod = 2.f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    check(vkCreateSampler(mDevice, &sampler_info, nullptr, &mSampler), "vkCreateSampler");
}

void VulkanTextureUploadRun::createDescriptors()
{
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layout_info.bindingCount = 1;
    layout_info.pBindings = &binding;
    check(vkCreateDescriptorSetLayout(mDevice, &layout_info, nullptr, &mDescriptorSetLayout),
          "vkCreateDescriptorSetLayout");

    VkPipelineLayoutCreateInfo pipeline_layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &mDescriptorSetLayout;
    check(vkCreatePipelineLayout(mDevice, &pipeline_layout_info, nullptr, &mPipelineLayout),
          "vkCreatePipelineLayout");

    VkDescriptorPoolSize pool_size{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
    VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    check(vkCreateDescriptorPool(mDevice, &pool_info, nullptr, &mDescriptorPool), "vkCreateDescriptorPool");
    VkDescriptorSetAllocateInfo allocation{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocation.descriptorPool = mDescriptorPool;
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &mDescriptorSetLayout;
    check(vkAllocateDescriptorSets(mDevice, &allocation, &mDescriptorSet), "vkAllocateDescriptorSets");

    VkDescriptorImageInfo image_info{};
    image_info.sampler = mSampler;
    image_info.imageView = mReplacement.mView;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = mDescriptorSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image_info;
    vkUpdateDescriptorSets(mDevice, 1, &write, 0, nullptr);
}

void VulkanTextureUploadRun::createRenderPass()
{
    VkAttachmentDescription attachment{};
    attachment.format = COLOR_FORMAT;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference reference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &reference;
    VkRenderPassCreateInfo create_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    create_info.attachmentCount = 1;
    create_info.pAttachments = &attachment;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;
    check(vkCreateRenderPass(mDevice, &create_info, nullptr, &mRenderPass), "vkCreateRenderPass");

    VkFramebufferCreateInfo framebuffer_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    framebuffer_info.renderPass = mRenderPass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = &mOutputImage.mView;
    framebuffer_info.width = OUTPUT_EXTENT.mWidth;
    framebuffer_info.height = OUTPUT_EXTENT.mHeight;
    framebuffer_info.layers = 1;
    check(vkCreateFramebuffer(mDevice, &framebuffer_info, nullptr, &mFramebuffer), "vkCreateFramebuffer");
}

void VulkanTextureUploadRun::createPipeline()
{
    const auto create_module = [this](const std::vector<std::uint32_t>& words)
    {
        VkShaderModule module = VK_NULL_HANDLE;
        VkShaderModuleCreateInfo create_info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        create_info.codeSize = words.size() * sizeof(std::uint32_t);
        create_info.pCode = words.data();
        check(vkCreateShaderModule(mDevice, &create_info, nullptr, &module), "vkCreateShaderModule");
        return module;
    };
    VkShaderModule vertex = create_module(mVertexSpirv);
    VkShaderModule fragment = VK_NULL_HANDLE;
    try
    {
        fragment = create_module(mFragmentSpirv);
        std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertex;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragment;
        stages[1].pName = "main";

        VkVertexInputBindingDescription vertex_binding{ 0, 16, VK_VERTEX_INPUT_RATE_VERTEX };
        VkVertexInputAttributeDescription vertex_attribute{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };
        VkPipelineVertexInputStateCreateInfo vertex_input{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertex_input.vertexBindingDescriptionCount = 1;
        vertex_input.pVertexBindingDescriptions = &vertex_binding;
        vertex_input.vertexAttributeDescriptionCount = 1;
        vertex_input.pVertexAttributeDescriptions = &vertex_attribute;
        VkPipelineInputAssemblyStateCreateInfo assembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster.lineWidth = 1.f;
        VkPipelineMultisampleStateCreateInfo multisample{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth.minDepthBounds = 0.f;
        depth.maxDepthBounds = 1.f;
        VkPipelineColorBlendAttachmentState blend{};
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend.colorBlendOp = VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend.alphaBlendOp = VK_BLEND_OP_ADD;
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo color{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        color.logicOp = VK_LOGIC_OP_COPY;
        color.attachmentCount = 1;
        color.pAttachments = &blend;
        constexpr std::array dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamic{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic.pDynamicStates = dynamic_states.data();

        VkGraphicsPipelineCreateInfo pipeline_info{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipeline_info.stageCount = static_cast<std::uint32_t>(stages.size());
        pipeline_info.pStages = stages.data();
        pipeline_info.pVertexInputState = &vertex_input;
        pipeline_info.pInputAssemblyState = &assembly;
        pipeline_info.pViewportState = &viewport;
        pipeline_info.pRasterizationState = &raster;
        pipeline_info.pMultisampleState = &multisample;
        pipeline_info.pDepthStencilState = &depth;
        pipeline_info.pColorBlendState = &color;
        pipeline_info.pDynamicState = &dynamic;
        pipeline_info.layout = mPipelineLayout;
        pipeline_info.renderPass = mRenderPass;
        pipeline_info.subpass = 0;
        check(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &mPipeline),
              "vkCreateGraphicsPipelines");
    }
    catch (...)
    {
        if (fragment != VK_NULL_HANDLE) vkDestroyShaderModule(mDevice, fragment, nullptr);
        vkDestroyShaderModule(mDevice, vertex, nullptr);
        throw;
    }
    vkDestroyShaderModule(mDevice, fragment, nullptr);
    vkDestroyShaderModule(mDevice, vertex, nullptr);
}

void VulkanTextureUploadRun::seedImages(const TextureUploadFixture& fixture)
{
    const auto seed = [this](Image& image, const std::uint8_t* bytes)
    {
        for (std::uint32_t mip = 0; mip < image.mMipLevels; ++mip)
        {
            const std::uint32_t width = std::max(1U, image.mExtent.mWidth >> mip);
            const std::uint32_t height = std::max(1U, image.mExtent.mHeight >> mip);
            const std::size_t size = static_cast<std::size_t>(width) * height * 4;
            const std::size_t offset = image.mMipLevels == 1 ? 0 :
                LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_OFFSETS[mip];
            writeBuffer(mStaging, bytes + offset, size);
            submitDiagnostic([this, &image, mip, width, height, size](VkCommandBuffer command_buffer)
            {
                VkBufferMemoryBarrier buffer_barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
                buffer_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                buffer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                buffer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                buffer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                buffer_barrier.buffer = mStaging.mBuffer;
                buffer_barrier.size = size;
                VkImageMemoryBarrier image_barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
                image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                image_barrier.image = image.mImage;
                image_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, mip, 1, 0, 1 };
                vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                     0, nullptr, 1, &buffer_barrier, 1, &image_barrier);
                VkBufferImageCopy copy{};
                copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mip, 0, 1 };
                copy.imageExtent = { width, height, 1 };
                vkCmdCopyBufferToImage(command_buffer, mStaging.mBuffer, image.mImage,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
            });
        }
        transition(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                   VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    };
    seed(mOld, fixture.mOldMipRGBA8.data());
    seed(mReplacement, fixture.mReplacementSentinelMipRGBA8.data());
    seed(mOutputImage, fixture.mOutputSentinelRGBA8.data());
}

std::vector<std::uint8_t> VulkanTextureUploadRun::snapshotImage(Image& image)
{
    std::size_t total = 0;
    for (std::uint32_t mip = 0; mip < image.mMipLevels; ++mip)
    {
        total += static_cast<std::size_t>(std::max(1U, image.mExtent.mWidth >> mip)) *
                 std::max(1U, image.mExtent.mHeight >> mip) * 4;
    }
    if (total > READBACK_BYTES)
    {
        throw Failure("image snapshot exceeds the fixed readback buffer");
    }
    submitDiagnostic([this, &image](VkCommandBuffer command_buffer)
    {
        VkImageMemoryBarrier to_transfer{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        to_transfer.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        to_transfer.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.image = image.mImage;
        to_transfer.subresourceRange = image.mViewRange;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &to_transfer);
        std::array<VkBufferImageCopy, LLRenderContract::TEXTURE_UPLOAD_MIP_LEVELS> copies{};
        VkDeviceSize offset = 0;
        for (std::uint32_t mip = 0; mip < image.mMipLevels; ++mip)
        {
            const std::uint32_t width = std::max(1U, image.mExtent.mWidth >> mip);
            const std::uint32_t height = std::max(1U, image.mExtent.mHeight >> mip);
            copies[mip].bufferOffset = offset;
            copies[mip].imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mip, 0, 1 };
            copies[mip].imageExtent = { width, height, 1 };
            offset += static_cast<VkDeviceSize>(width) * height * 4;
        }
        vkCmdCopyImageToBuffer(command_buffer, image.mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               mReadback.mBuffer, image.mMipLevels, copies.data());
        VkImageMemoryBarrier restore = to_transfer;
        restore.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        restore.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        restore.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        restore.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkBufferMemoryBarrier host{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        host.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        host.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        host.buffer = mReadback.mBuffer;
        host.size = VK_WHOLE_SIZE;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT, 0,
                             0, nullptr, 1, &host, 1, &restore);
    });
    return readBuffer(mReadback, total);
}

FrameSnapshot VulkanTextureUploadRun::mutateFrame(const TextureUploadCase& diagnostic_case,
                                                   PacketMutation mutation) const
{
    FrameSnapshot frame = diagnostic_case.mFrame;
    if (mutation == PacketMutation::None)
    {
        return frame;
    }
    auto& upload = frame.mUploads.front();
    auto& pass = frame.mPasses.front();
    switch (mutation)
    {
        case PacketMutation::None: break;
        case PacketMutation::WrongFrame:
            frame.mFrame = 0;
            break;
        case PacketMutation::WrongUploadDestinationGeneration:
            ++upload.mDestination.mGeneration;
            break;
        case PacketMutation::WrongOldPacketGeneration:
            ++frame.mImages[0].mHandle.mGeneration;
            break;
        case PacketMutation::WrongOutputPacketGeneration:
            ++frame.mImages[2].mHandle.mGeneration;
            break;
        case PacketMutation::PacketImageAlias:
            frame.mImages[2].mHandle = frame.mImages[0].mHandle;
            break;
        case PacketMutation::WrongOldPacketMipCount:
            --frame.mImages[0].mMipLevels;
            break;
        case PacketMutation::WrongReplacementPacketMipCount:
            --frame.mImages[1].mMipLevels;
            break;
        case PacketMutation::WrongOutputPacketMipCount:
            ++frame.mImages[2].mMipLevels;
            break;
        case PacketMutation::DuplicateRevision:
            upload.mRevision = LLRenderContract::TEXTURE_UPLOAD_PRIOR_REVISION;
            break;
        case PacketMutation::WrongSubresource:
            upload.mSubresource.mMipLevel = 1;
            break;
        case PacketMutation::WrongOffset:
            upload.mOffset.mX = 1;
            break;
        case PacketMutation::WrongResidentExtent:
            --upload.mExtent.mWidth;
            break;
        case PacketMutation::WrongLogicalExtent:
            --upload.mLogicalExtent.mWidth;
            break;
        case PacketMutation::WrongResidentDiscard:
            --upload.mResidentDiscard;
            break;
        case PacketMutation::WrongRowPitch:
            --upload.mRowPitch;
            break;
        case PacketMutation::WrongRowOrigin:
            upload.mRowOrigin = LLRenderContract::RowOrigin::BottomLeft;
            break;
        case PacketMutation::WrongSourceFormat:
            upload.mSourceFormat = LLRenderContract::PixelFormat::RGB8Unorm;
            break;
        case PacketMutation::WrongMipGeneration:
            upload.mMipGeneration = LLRenderContract::MipGeneration::Disabled;
            break;
        case PacketMutation::WrongBefore:
            upload.mBefore = LLRenderContract::ImageState::ShaderRead;
            break;
        case PacketMutation::WrongDuring:
            upload.mDuring = LLRenderContract::ImageState::ShaderRead;
            break;
        case PacketMutation::WrongAfter:
            upload.mAfter = LLRenderContract::ImageState::ColorAttachment;
            break;
        case PacketMutation::WrongPixelOffset:
            upload.mPixels.mOffset = 1;
            break;
        case PacketMutation::WrongPixelSize:
            --upload.mPixels.mSize;
            break;
        case PacketMutation::WrongPixelStorageSize:
        {
            auto storage = std::make_shared<std::vector<std::uint8_t>>(*upload.mPixels.mStorage);
            storage->push_back(0xa5U);
            upload.mPixels.mStorage = std::move(storage);
            break;
        }
        case PacketMutation::WrongSamplerPacket:
            frame.mSamplers[0].mAddressU = LLRenderContract::AddressMode::Repeat;
            break;
        case PacketMutation::MissingSamplerPacket:
            frame.mSamplers.clear();
            break;
        case PacketMutation::WrongPipelinePacket:
            ++frame.mPipelines[0].mProgram.mVariant;
            break;
        case PacketMutation::MissingPipelinePacket:
            frame.mPipelines.clear();
            break;
        case PacketMutation::WrongOutputPacketExtent:
            --frame.mImages[2].mExtent.mWidth;
            break;
        case PacketMutation::WrongReleaseFrame:
            ++frame.mReleases[0].mFrame;
            break;
        case PacketMutation::WrongRelease:
            frame.mReleases.front().mResource = LLRenderContract::ResourceHandle{
                diagnostic_case.mInputs.mHandles.mReplacementImage };
            break;
        case PacketMutation::MissingRelease:
            frame.mReleases.clear();
            break;
        case PacketMutation::ExtraPass:
            frame.mPasses.push_back(frame.mPasses[0]);
            break;
        case PacketMutation::MissingPass:
            frame.mPasses.clear();
            break;
        case PacketMutation::WrongViewport:
            --pass.mViewport.mWidth;
            break;
        case PacketMutation::MissingDraw:
            pass.mDraws.clear();
            break;
        case PacketMutation::ExtraDraw:
            pass.mDraws.push_back(pass.mDraws[0]);
            break;
        case PacketMutation::WrongSampleRange:
        {
            auto& draw = std::get<LLRenderContract::Draw>(pass.mDraws.front());
            --draw.mResources.mSampledImages.front().mRange.mMipLevelCount;
            break;
        }
    }
    return frame;
}

LLRenderVulkanTextureUpload::ExecutionContext VulkanTextureUploadRun::executionContext(
    ContextMutation mutation)
{
    LLRenderVulkanTextureUpload::ExecutionContext context;
    context.mDevice = mDevice;
    context.mCommandPool = mCommandPool;
    context.mCommandBuffer = mCommandBuffer;
    context.mQueue = mQueue;
    context.mOwnershipToken = mOwnershipToken;
    context.mQueueFamilyIndex = mQueueFamily;
    context.mQueueFamilyFlags = mQueueFamilyFlags;
    context.mQueueCount = 1;
    context.mQueueIndex = 0;
    context.mCommandPoolQueueFamilyIndex = mQueueFamily;
    context.mCommandPoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    context.mCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    context.mRecordingAttemptCount = &mExecutorRecordings;
    context.mSubmissionCount = &mExecutorSubmissions;
    context.mRequiredVertexShaderIdentity = mVertexIdentity;
    context.mRequiredFragmentShaderIdentity = mFragmentIdentity;
    switch (mutation)
    {
        case ContextMutation::None: break;
        case ContextMutation::NullDevice: context.mDevice = VK_NULL_HANDLE; break;
        case ContextMutation::NullCommandBuffer: context.mCommandBuffer = VK_NULL_HANDLE; break;
        case ContextMutation::NullQueue: context.mQueue = VK_NULL_HANDLE; break;
        case ContextMutation::MissingRecordingCounter: context.mRecordingAttemptCount = nullptr; break;
        case ContextMutation::MissingSubmissionCounter: context.mSubmissionCount = nullptr; break;
        case ContextMutation::WrongVertexIdentity: ++context.mRequiredVertexShaderIdentity.front(); break;
        case ContextMutation::WrongFragmentIdentity: ++context.mRequiredFragmentShaderIdentity.front(); break;
        case ContextMutation::NullCommandPool: context.mCommandPool = VK_NULL_HANDLE; break;
        case ContextMutation::WrongOwnershipToken: ++context.mOwnershipToken; break;
        case ContextMutation::IgnoredQueueFamily: context.mQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; break;
        case ContextMutation::MissingGraphicsQueueFlag: context.mQueueFamilyFlags &= ~VK_QUEUE_GRAPHICS_BIT; break;
        case ContextMutation::MissingTransferQueueFlag: context.mQueueFamilyFlags &= ~VK_QUEUE_TRANSFER_BIT; break;
        case ContextMutation::WrongQueueCount: ++context.mQueueCount; break;
        case ContextMutation::WrongQueueIndex: ++context.mQueueIndex; break;
        case ContextMutation::WrongCommandPoolFamily: ++context.mCommandPoolQueueFamilyIndex; break;
        case ContextMutation::MissingCommandPoolReset:
            context.mCommandPoolFlags &= ~VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            break;
        case ContextMutation::WrongCommandBufferLevel:
            context.mCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
            break;
    }
    return context;
}

LLRenderVulkanTextureUpload::Registry VulkanTextureUploadRun::makeRegistry(
    const TextureUploadCase& diagnostic_case, RegistryMutation mutation,
    StreamingUploadLifecycle* lifecycle)
{
    const TextureUploadFixture fixture = LLRenderContract::makeTextureUploadFixture();
    const auto& handles = diagnostic_case.mInputs.mHandles;
    auto screen_handle = handles.mScreenTriangle;
    auto old_handle = handles.mOldImage;
    auto replacement_handle = handles.mReplacementImage;
    auto output_handle = handles.mOutput;
    auto sampler_handle = handles.mSampler;
    auto pipeline_handle = handles.mPipeline;
    if (mutation == RegistryMutation::StaleScreen) ++screen_handle.mGeneration;
    if (mutation == RegistryMutation::StaleOld) ++old_handle.mGeneration;
    if (mutation == RegistryMutation::StaleReplacement) ++replacement_handle.mGeneration;
    if (mutation == RegistryMutation::StaleOutput) ++output_handle.mGeneration;
    if (mutation == RegistryMutation::StaleSampler) ++sampler_handle.mGeneration;
    if (mutation == RegistryMutation::StalePipeline) ++pipeline_handle.mGeneration;

    LLRenderVulkanTextureUpload::BufferBinding screen;
    screen.mBuffer = mScreen.mBuffer;
    screen.mMemory = mScreen.mMemory;
    screen.mOwnershipToken = mOwnershipToken;
    screen.mMapped = mScreen.mMapped;
    screen.mSize = mScreen.mSize;
    screen.mAllocationSize = mScreen.mAllocationSize;
    screen.mMemoryOffset = 0;
    screen.mCreateFlags = 0;
    screen.mUsage = mScreen.mUsage;
    screen.mSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    screen.mMemoryProperties = mScreen.mMemoryProperties;
    screen.mHasFixtureBytes = true;
    std::memcpy(screen.mFixtureBytes.data(), fixture.mScreenTriangle.data(), screen.mFixtureBytes.size());
    if (mutation == RegistryMutation::WrongScreenSize) --screen.mSize;
    if (mutation == RegistryMutation::WrongScreenUsage) screen.mUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (mutation == RegistryMutation::WrongOwnershipToken) ++screen.mOwnershipToken;

    const auto image_binding = [this](const Image& image, bool output)
    {
        LLRenderVulkanTextureUpload::ImageBinding binding;
        binding.mImage = image.mImage;
        binding.mView = image.mView;
        binding.mMemory = image.mMemory;
        binding.mOwnershipToken = mOwnershipToken;
        binding.mAllocationSize = image.mAllocationSize;
        binding.mMemoryOffset = 0;
        binding.mCreateFlags = 0;
        binding.mImageType = VK_IMAGE_TYPE_2D;
        binding.mFormat = image.mFormat;
        binding.mResidentExtent = image.mExtent;
        binding.mLogicalExtent = output ? image.mExtent :
            Extent2D{ LLRenderContract::TEXTURE_UPLOAD_LOGICAL_WIDTH,
                      LLRenderContract::TEXTURE_UPLOAD_LOGICAL_HEIGHT };
        binding.mResidentDiscard = output ? 0 : LLRenderContract::TEXTURE_UPLOAD_RESIDENT_DISCARD;
        binding.mMipLevels = image.mMipLevels;
        binding.mArrayLayers = 1;
        binding.mSamples = VK_SAMPLE_COUNT_1_BIT;
        binding.mTiling = VK_IMAGE_TILING_OPTIMAL;
        binding.mUsage = image.mUsage;
        binding.mSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        binding.mAspect = VK_IMAGE_ASPECT_COLOR_BIT;
        binding.mLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        binding.mViewType = VK_IMAGE_VIEW_TYPE_2D;
        binding.mViewFormat = image.mFormat;
        binding.mViewComponents = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                    VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        binding.mViewRange = image.mViewRange;
        return binding;
    };
    auto old_image = image_binding(mOld, false);
    auto replacement_image = image_binding(mReplacement, false);
    auto output_image = image_binding(mOutputImage, true);
    old_image.mHasPreExecutionMipSnapshot = true;
    old_image.mPreExecutionMipRGBA8 = mOldPreExecutionSnapshot;
    if (mutation == RegistryMutation::WrongOldFormat) old_image.mFormat = VK_FORMAT_B8G8R8A8_UNORM;
    if (mutation == RegistryMutation::WrongOldExtent) --old_image.mResidentExtent.mWidth;
    if (mutation == RegistryMutation::WrongOldMips) --old_image.mMipLevels;
    if (mutation == RegistryMutation::WrongOldUsage) old_image.mUsage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
    if (mutation == RegistryMutation::WrongOldLayout) old_image.mLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    if (mutation == RegistryMutation::WrongOldViewRange) --old_image.mViewRange.levelCount;
    if (mutation == RegistryMutation::WrongOldSnapshot) ++old_image.mPreExecutionMipRGBA8.front();
    if (mutation == RegistryMutation::WrongReplacementFormat) replacement_image.mFormat = VK_FORMAT_B8G8R8A8_UNORM;
    if (mutation == RegistryMutation::WrongReplacementExtent) --replacement_image.mResidentExtent.mWidth;
    if (mutation == RegistryMutation::WrongReplacementMips) --replacement_image.mMipLevels;
    if (mutation == RegistryMutation::WrongReplacementUsage) replacement_image.mUsage &= ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (mutation == RegistryMutation::WrongReplacementLayout) replacement_image.mLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    if (mutation == RegistryMutation::WrongReplacementViewRange) --replacement_image.mViewRange.levelCount;
    if (mutation == RegistryMutation::WrongOutputFormat) output_image.mFormat = VK_FORMAT_B8G8R8A8_UNORM;
    if (mutation == RegistryMutation::WrongOutputExtent) --output_image.mResidentExtent.mWidth;
    if (mutation == RegistryMutation::WrongOutputMips) ++output_image.mMipLevels;
    if (mutation == RegistryMutation::WrongOutputUsage) output_image.mUsage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
    if (mutation == RegistryMutation::WrongOutputLayout) output_image.mLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    if (mutation == RegistryMutation::WrongOutputViewRange) ++output_image.mViewRange.levelCount;
    if (mutation == RegistryMutation::AliasImages) replacement_image.mImage = old_image.mImage;
    if (mutation == RegistryMutation::AliasImageMemory) replacement_image.mMemory = old_image.mMemory;
    if (mutation == RegistryMutation::AliasViews) replacement_image.mView = old_image.mView;

    LLRenderVulkanTextureUpload::SamplerBinding sampler;
    sampler.mSampler = mSampler;
    sampler.mOwnershipToken = mOwnershipToken;
    sampler.mCreateFlags = 0;
    sampler.mMinFilter = VK_FILTER_LINEAR;
    sampler.mMagFilter = VK_FILTER_LINEAR;
    sampler.mMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.mAddressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.mAddressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.mAddressW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.mMipLodBias = 0.f;
    sampler.mAnisotropyEnable = VK_FALSE;
    sampler.mMaxAnisotropy = 1.f;
    sampler.mCompareEnable = VK_FALSE;
    sampler.mCompareOp = VK_COMPARE_OP_ALWAYS;
    sampler.mMinLod = 0.f;
    sampler.mMaxLod = 2.f;
    sampler.mBorderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler.mUnnormalizedCoordinates = VK_FALSE;
    if (mutation == RegistryMutation::WrongSampler) sampler.mMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    LLRenderVulkanTextureUpload::PipelineBinding pipeline;
    pipeline.mProgram = { "contract.sample-texture", 0 };
    pipeline.mPipeline = mPipeline;
    pipeline.mLayout = mPipelineLayout;
    pipeline.mRenderPass = mRenderPass;
    pipeline.mFramebuffer = mFramebuffer;
    pipeline.mDescriptorSet = mDescriptorSet;
    pipeline.mOwnershipToken = mOwnershipToken;
    pipeline.mExtent = OUTPUT_EXTENT;
    pipeline.mSampledDescriptor = { 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                    VK_SHADER_STAGE_FRAGMENT_BIT, mReplacement.mView, mSampler,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    pipeline.mColorView = mOutputImage.mView;
    pipeline.mColorFormat = COLOR_FORMAT;
    pipeline.mColorSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline.mColorLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    pipeline.mColorStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline.mStencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    pipeline.mStencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    pipeline.mColorInitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    pipeline.mColorFinalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    pipeline.mSubpassDependencyCount = 0;
    pipeline.mVertexBinding = { 0, 16, VK_VERTEX_INPUT_RATE_VERTEX };
    pipeline.mVertexAttribute = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };
    pipeline.mRaster.mTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline.mRaster.mPrimitiveRestartEnable = VK_FALSE;
    pipeline.mRaster.mDepthClampEnable = VK_FALSE;
    pipeline.mRaster.mRasterizerDiscardEnable = VK_FALSE;
    pipeline.mRaster.mPolygonMode = VK_POLYGON_MODE_FILL;
    pipeline.mRaster.mCullMode = VK_CULL_MODE_NONE;
    pipeline.mRaster.mFrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline.mRaster.mDepthBiasEnable = VK_FALSE;
    pipeline.mRaster.mLineWidth = 1.f;
    pipeline.mMultisample.mRasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline.mMultisample.mSampleShadingEnable = VK_FALSE;
    pipeline.mMultisample.mMinSampleShading = 0.f;
    pipeline.mMultisample.mSampleMask = 0xffffffffU;
    pipeline.mMultisample.mAlphaToCoverageEnable = VK_FALSE;
    pipeline.mMultisample.mAlphaToOneEnable = VK_FALSE;
    pipeline.mDepthStencil.mDepthTestEnable = VK_FALSE;
    pipeline.mDepthStencil.mDepthWriteEnable = VK_FALSE;
    pipeline.mDepthStencil.mDepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipeline.mDepthStencil.mDepthBoundsTestEnable = VK_FALSE;
    pipeline.mDepthStencil.mStencilTestEnable = VK_FALSE;
    pipeline.mDepthStencil.mMinDepthBounds = 0.f;
    pipeline.mDepthStencil.mMaxDepthBounds = 1.f;
    pipeline.mColorTarget.mFormat = COLOR_FORMAT;
    pipeline.mColorTarget.mBlendEnable = VK_FALSE;
    pipeline.mColorTarget.mSrcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pipeline.mColorTarget.mDstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipeline.mColorTarget.mColorBlendOp = VK_BLEND_OP_ADD;
    pipeline.mColorTarget.mSrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipeline.mColorTarget.mDstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipeline.mColorTarget.mAlphaBlendOp = VK_BLEND_OP_ADD;
    pipeline.mColorTarget.mWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pipeline.mLogicOpEnable = VK_FALSE;
    pipeline.mLogicOp = VK_LOGIC_OP_COPY;
    pipeline.mDynamicViewport = VK_TRUE;
    pipeline.mDynamicScissor = VK_TRUE;
    pipeline.mVertexShaderIdentity = mVertexIdentity;
    pipeline.mFragmentShaderIdentity = mFragmentIdentity;
    if (mutation == RegistryMutation::WrongProgram) pipeline.mProgram.mName = "wrong.sample-texture";
    if (mutation == RegistryMutation::WrongVariant) ++pipeline.mProgram.mVariant;
    if (mutation == RegistryMutation::WrongPipelineExtent) --pipeline.mExtent.mWidth;
    if (mutation == RegistryMutation::WrongDescriptor) pipeline.mSampledDescriptor.mView = mOld.mView;
    if (mutation == RegistryMutation::WrongRenderPass) pipeline.mRenderPass = VK_NULL_HANDLE;
    if (mutation == RegistryMutation::WrongFramebuffer) pipeline.mFramebuffer = VK_NULL_HANDLE;
    if (mutation == RegistryMutation::WrongVertexState) pipeline.mVertexBinding.mStride = 12;
    if (mutation == RegistryMutation::WrongViewportPolicy) pipeline.mDynamicViewport = VK_FALSE;
    if (mutation == RegistryMutation::WrongRasterState) pipeline.mRaster.mCullMode = VK_CULL_MODE_BACK_BIT;
    if (mutation == RegistryMutation::WrongShaderIdentity) ++pipeline.mFragmentShaderIdentity.front();

    const auto buffer_binding = [this](const Buffer& buffer)
    {
        LLRenderVulkanTextureUpload::BufferBinding binding;
        binding.mBuffer = buffer.mBuffer;
        binding.mMemory = buffer.mMemory;
        binding.mOwnershipToken = mOwnershipToken;
        binding.mMapped = buffer.mMapped;
        binding.mSize = buffer.mSize;
        binding.mAllocationSize = buffer.mAllocationSize;
        binding.mMemoryOffset = 0;
        binding.mCreateFlags = 0;
        binding.mUsage = buffer.mUsage;
        binding.mSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        binding.mMemoryProperties = buffer.mMemoryProperties;
        return binding;
    };
    LLRenderVulkanTextureUpload::TransferResources transfer{ buffer_binding(mStaging), buffer_binding(mReadback) };
    if (mutation == RegistryMutation::WrongStagingSize) --transfer.mStaging.mSize;
    if (mutation == RegistryMutation::WrongStagingUsage) transfer.mStaging.mUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (mutation == RegistryMutation::WrongStagingMemory) transfer.mStaging.mMemoryProperties &= ~VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    if (mutation == RegistryMutation::WrongReadbackSize) --transfer.mReadback.mSize;
    if (mutation == RegistryMutation::WrongReadbackUsage) transfer.mReadback.mUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (mutation == RegistryMutation::WrongReadbackMemory) transfer.mReadback.mMemoryProperties &= ~VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    if (mutation == RegistryMutation::AliasBuffers) transfer.mReadback.mBuffer = transfer.mStaging.mBuffer;
    if (mutation == RegistryMutation::AliasBufferMemory) transfer.mReadback.mMemory = transfer.mStaging.mMemory;

    if (lifecycle)
    {
        if (mutation == RegistryMutation::WrongLifecycleCurrent) lifecycle->mCurrentImage = handles.mReplacementImage;
        if (mutation == RegistryMutation::WrongLifecycleRevision) lifecycle->mLastRevision = LLRenderContract::TEXTURE_UPLOAD_REVISION;
        if (mutation == RegistryMutation::PendingLifecycle) lifecycle->mCompletionPending = true;
        if (mutation == RegistryMutation::PriorCompletion)
        {
            lifecycle->mCompletionCount = 1;
            lifecycle->mCompletedDestination = handles.mOldImage;
            lifecycle->mCompletedRevision = LLRenderContract::TEXTURE_UPLOAD_PRIOR_REVISION;
            lifecycle->mCompletedFrame = diagnostic_case.mInputs.mFrame;
        }
        if (mutation == RegistryMutation::PriorRetirement)
        {
            lifecycle->mRetirementCount = 1;
            lifecycle->mRetiredResource = handles.mOldImage;
            lifecycle->mRetirementFrame = diagnostic_case.mInputs.mFrame;
        }
    }

    LLRenderVulkanTextureUpload::Registry registry;
    const bool registered = registry.addScreenTriangle(screen_handle, screen) &&
        registry.addImageGenerations(old_handle, old_image, replacement_handle, replacement_image) &&
        registry.addOutput(output_handle, output_image) &&
        registry.addSampler(sampler_handle, sampler) &&
        registry.addPipeline(pipeline_handle, pipeline) &&
        registry.addTransferResources(transfer);
    if (!registered)
    {
        if (mutation == RegistryMutation::None)
        {
            throw Failure("could not register complete Vulkan texture-upload resources");
        }
        return registry;
    }
    if (mutation != RegistryMutation::MissingLifecycle && !registry.addLifecycle(lifecycle))
    {
        if (mutation == RegistryMutation::None)
        {
            throw Failure("could not register Vulkan texture-upload lifecycle");
        }
    }
    return registry;
}

void VulkanTextureUploadRun::runRejections(const TextureUploadCase& diagnostic_case,
                                            const TextureUploadFixture& fixture)
{
    constexpr std::array REJECTIONS{
        RejectionSpec{ "stale_screen", RegistryMutation::StaleScreen },
        RejectionSpec{ "stale_old", RegistryMutation::StaleOld },
        RejectionSpec{ "stale_replacement", RegistryMutation::StaleReplacement },
        RejectionSpec{ "stale_output", RegistryMutation::StaleOutput },
        RejectionSpec{ "stale_sampler", RegistryMutation::StaleSampler },
        RejectionSpec{ "stale_pipeline", RegistryMutation::StalePipeline },
        RejectionSpec{ "wrong_screen_size", RegistryMutation::WrongScreenSize },
        RejectionSpec{ "wrong_screen_usage", RegistryMutation::WrongScreenUsage },
        RejectionSpec{ "wrong_staging_size", RegistryMutation::WrongStagingSize },
        RejectionSpec{ "wrong_staging_usage", RegistryMutation::WrongStagingUsage },
        RejectionSpec{ "wrong_staging_memory", RegistryMutation::WrongStagingMemory },
        RejectionSpec{ "wrong_readback_size", RegistryMutation::WrongReadbackSize },
        RejectionSpec{ "wrong_readback_usage", RegistryMutation::WrongReadbackUsage },
        RejectionSpec{ "wrong_readback_memory", RegistryMutation::WrongReadbackMemory },
        RejectionSpec{ "alias_buffers", RegistryMutation::AliasBuffers },
        RejectionSpec{ "alias_buffer_memory", RegistryMutation::AliasBufferMemory },
        RejectionSpec{ "wrong_old_format", RegistryMutation::WrongOldFormat },
        RejectionSpec{ "wrong_old_extent", RegistryMutation::WrongOldExtent },
        RejectionSpec{ "wrong_old_mips", RegistryMutation::WrongOldMips },
        RejectionSpec{ "wrong_old_usage", RegistryMutation::WrongOldUsage },
        RejectionSpec{ "wrong_old_layout", RegistryMutation::WrongOldLayout },
        RejectionSpec{ "wrong_old_view_range", RegistryMutation::WrongOldViewRange },
        RejectionSpec{ "wrong_old_snapshot", RegistryMutation::WrongOldSnapshot },
        RejectionSpec{ "wrong_replacement_format", RegistryMutation::WrongReplacementFormat },
        RejectionSpec{ "wrong_replacement_extent", RegistryMutation::WrongReplacementExtent },
        RejectionSpec{ "wrong_replacement_mips", RegistryMutation::WrongReplacementMips },
        RejectionSpec{ "wrong_replacement_usage", RegistryMutation::WrongReplacementUsage },
        RejectionSpec{ "wrong_replacement_layout", RegistryMutation::WrongReplacementLayout },
        RejectionSpec{ "wrong_replacement_view_range", RegistryMutation::WrongReplacementViewRange },
        RejectionSpec{ "wrong_output_format", RegistryMutation::WrongOutputFormat },
        RejectionSpec{ "wrong_output_extent", RegistryMutation::WrongOutputExtent },
        RejectionSpec{ "wrong_output_mips", RegistryMutation::WrongOutputMips },
        RejectionSpec{ "wrong_output_usage", RegistryMutation::WrongOutputUsage },
        RejectionSpec{ "wrong_output_layout", RegistryMutation::WrongOutputLayout },
        RejectionSpec{ "wrong_output_view_range", RegistryMutation::WrongOutputViewRange },
        RejectionSpec{ "alias_images", RegistryMutation::AliasImages },
        RejectionSpec{ "alias_image_memory", RegistryMutation::AliasImageMemory },
        RejectionSpec{ "alias_views", RegistryMutation::AliasViews },
        RejectionSpec{ "wrong_sampler", RegistryMutation::WrongSampler },
        RejectionSpec{ "wrong_program", RegistryMutation::WrongProgram },
        RejectionSpec{ "wrong_variant", RegistryMutation::WrongVariant },
        RejectionSpec{ "wrong_pipeline_extent", RegistryMutation::WrongPipelineExtent },
        RejectionSpec{ "wrong_descriptor", RegistryMutation::WrongDescriptor },
        RejectionSpec{ "wrong_render_pass", RegistryMutation::WrongRenderPass },
        RejectionSpec{ "wrong_framebuffer", RegistryMutation::WrongFramebuffer },
        RejectionSpec{ "wrong_vertex_state", RegistryMutation::WrongVertexState },
        RejectionSpec{ "wrong_viewport_policy", RegistryMutation::WrongViewportPolicy },
        RejectionSpec{ "wrong_raster_state", RegistryMutation::WrongRasterState },
        RejectionSpec{ "wrong_shader_identity", RegistryMutation::WrongShaderIdentity },
        RejectionSpec{ "missing_lifecycle", RegistryMutation::MissingLifecycle },
        RejectionSpec{ "wrong_lifecycle_current", RegistryMutation::WrongLifecycleCurrent },
        RejectionSpec{ "wrong_lifecycle_revision", RegistryMutation::WrongLifecycleRevision },
        RejectionSpec{ "pending_lifecycle", RegistryMutation::PendingLifecycle },
        RejectionSpec{ "prior_completion", RegistryMutation::PriorCompletion },
        RejectionSpec{ "prior_retirement", RegistryMutation::PriorRetirement },
        RejectionSpec{ "mismatched_resource_ownership", RegistryMutation::WrongOwnershipToken },
        // The complete frozen Stage 14 neutral packet matrix, retaining its
        // original case names so cross-backend coverage remains auditable.
        RejectionSpec{ "frame", RegistryMutation::None, PacketMutation::WrongFrame },
        RejectionSpec{ "destination", RegistryMutation::None, PacketMutation::WrongUploadDestinationGeneration },
        RejectionSpec{ "old_generation", RegistryMutation::None, PacketMutation::WrongOldPacketGeneration },
        RejectionSpec{ "output_generation", RegistryMutation::None, PacketMutation::WrongOutputPacketGeneration },
        RejectionSpec{ "image_alias", RegistryMutation::None, PacketMutation::PacketImageAlias },
        RejectionSpec{ "old_mip_count", RegistryMutation::None, PacketMutation::WrongOldPacketMipCount },
        RejectionSpec{ "replacement_mip_count", RegistryMutation::None,
                       PacketMutation::WrongReplacementPacketMipCount },
        RejectionSpec{ "output_mip_count", RegistryMutation::None, PacketMutation::WrongOutputPacketMipCount },
        RejectionSpec{ "revision", RegistryMutation::None, PacketMutation::DuplicateRevision },
        RejectionSpec{ "subresource", RegistryMutation::None, PacketMutation::WrongSubresource },
        RejectionSpec{ "offset", RegistryMutation::None, PacketMutation::WrongOffset },
        RejectionSpec{ "extent", RegistryMutation::None, PacketMutation::WrongResidentExtent },
        RejectionSpec{ "logical_extent", RegistryMutation::None, PacketMutation::WrongLogicalExtent },
        RejectionSpec{ "discard", RegistryMutation::None, PacketMutation::WrongResidentDiscard },
        RejectionSpec{ "format", RegistryMutation::None, PacketMutation::WrongSourceFormat },
        RejectionSpec{ "row_pitch", RegistryMutation::None, PacketMutation::WrongRowPitch },
        RejectionSpec{ "row_origin", RegistryMutation::None, PacketMutation::WrongRowOrigin },
        RejectionSpec{ "mip_policy", RegistryMutation::None, PacketMutation::WrongMipGeneration },
        RejectionSpec{ "pixel_offset", RegistryMutation::None, PacketMutation::WrongPixelOffset },
        RejectionSpec{ "pixel_size", RegistryMutation::None, PacketMutation::WrongPixelSize },
        RejectionSpec{ "before_state", RegistryMutation::None, PacketMutation::WrongBefore },
        RejectionSpec{ "during_state", RegistryMutation::None, PacketMutation::WrongDuring },
        RejectionSpec{ "after_state", RegistryMutation::None, PacketMutation::WrongAfter },
        RejectionSpec{ "sampler", RegistryMutation::None, PacketMutation::WrongSamplerPacket },
        RejectionSpec{ "missing_sampler", RegistryMutation::None, PacketMutation::MissingSamplerPacket },
        RejectionSpec{ "pipeline", RegistryMutation::None, PacketMutation::WrongPipelinePacket },
        RejectionSpec{ "missing_pipeline", RegistryMutation::None, PacketMutation::MissingPipelinePacket },
        RejectionSpec{ "output", RegistryMutation::None, PacketMutation::WrongOutputPacketExtent },
        RejectionSpec{ "release", RegistryMutation::None, PacketMutation::WrongReleaseFrame },
        RejectionSpec{ "released_resource", RegistryMutation::None, PacketMutation::WrongRelease },
        RejectionSpec{ "missing_release", RegistryMutation::None, PacketMutation::MissingRelease },
        RejectionSpec{ "extra_pass", RegistryMutation::None, PacketMutation::ExtraPass },

        // A small decoder-shape supplement covers storage ownership and both
        // missing/duplicate command containers without duplicating every field
        // already frozen by the neutral decoder tests.
        RejectionSpec{ "pixel_storage_size", RegistryMutation::None, PacketMutation::WrongPixelStorageSize },
        RejectionSpec{ "missing_pass", RegistryMutation::None, PacketMutation::MissingPass },
        RejectionSpec{ "viewport", RegistryMutation::None, PacketMutation::WrongViewport },
        RejectionSpec{ "missing_draw", RegistryMutation::None, PacketMutation::MissingDraw },
        RejectionSpec{ "extra_draw", RegistryMutation::None, PacketMutation::ExtraDraw },
        RejectionSpec{ "sampled_range", RegistryMutation::None, PacketMutation::WrongSampleRange },
        RejectionSpec{ "null_device", RegistryMutation::None, PacketMutation::None, ContextMutation::NullDevice },
        RejectionSpec{ "null_command_buffer", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::NullCommandBuffer },
        RejectionSpec{ "null_queue", RegistryMutation::None, PacketMutation::None, ContextMutation::NullQueue },
        RejectionSpec{ "missing_recording_counter", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::MissingRecordingCounter },
        RejectionSpec{ "missing_submission_counter", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::MissingSubmissionCounter },
        RejectionSpec{ "wrong_required_vertex_identity", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::WrongVertexIdentity },
        RejectionSpec{ "wrong_required_fragment_identity", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::WrongFragmentIdentity },
        RejectionSpec{ "null_command_pool", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::NullCommandPool },
        RejectionSpec{ "wrong_context_ownership", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::WrongOwnershipToken },
        RejectionSpec{ "ignored_queue_family", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::IgnoredQueueFamily },
        RejectionSpec{ "missing_graphics_queue_flag", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::MissingGraphicsQueueFlag },
        RejectionSpec{ "missing_transfer_queue_flag", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::MissingTransferQueueFlag },
        RejectionSpec{ "wrong_queue_count", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::WrongQueueCount },
        RejectionSpec{ "wrong_queue_index", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::WrongQueueIndex },
        RejectionSpec{ "wrong_command_pool_family", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::WrongCommandPoolFamily },
        RejectionSpec{ "missing_command_pool_reset", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::MissingCommandPoolReset },
        RejectionSpec{ "wrong_command_buffer_level", RegistryMutation::None, PacketMutation::None,
                       ContextMutation::WrongCommandBufferLevel }
    };

    const auto resolution = [&diagnostic_case](const LLRenderVulkanTextureUpload::Registry& registry)
    {
        const auto& handles = diagnostic_case.mInputs.mHandles;
        return std::array<bool, 10>{
            registry.resolve(handles.mScreenTriangle) != nullptr,
            registry.resolveRegisteredImage(handles.mOldImage) != nullptr,
            registry.resolveRegisteredImage(handles.mReplacementImage) != nullptr,
            registry.resolveOutput(handles.mOutput) != nullptr,
            registry.resolve(handles.mSampler) != nullptr,
            registry.resolve(handles.mPipeline, diagnostic_case.mFrame.mPipelines.front().mProgram) != nullptr,
            registry.transferResources() != nullptr,
            registry.lifecycle() != nullptr,
            registry.isResolvable(handles.mOldImage),
            registry.isResolvable(handles.mReplacementImage)
        };
    };

    std::array<std::uint8_t, STAGING_BYTES> staging_poison{};
    std::array<std::uint8_t, READBACK_BYTES> readback_poison{};
    for (std::size_t index = 0; index < staging_poison.size(); ++index)
    {
        staging_poison[index] = static_cast<std::uint8_t>((index * 43U + 17U) & 0xffU);
    }
    for (std::size_t index = 0; index < readback_poison.size(); ++index)
    {
        readback_poison[index] = static_cast<std::uint8_t>((index * 29U + 91U) & 0xffU);
    }

    for (const RejectionSpec& rejection : REJECTIONS)
    {
        const std::vector<std::uint8_t> old_before = snapshotImage(mOld);
        const std::vector<std::uint8_t> replacement_before = snapshotImage(mReplacement);
        const std::vector<std::uint8_t> output_before = snapshotImage(mOutputImage);
        if (!std::equal(old_before.begin(), old_before.end(), fixture.mOldMipRGBA8.begin()) ||
            !std::equal(replacement_before.begin(), replacement_before.end(),
                        fixture.mReplacementSentinelMipRGBA8.begin()) ||
            !std::equal(output_before.begin(), output_before.end(), fixture.mOutputSentinelRGBA8.begin()))
        {
            throw Failure("a rejection sentinel was not canonical before executor preflight");
        }
        writeBuffer(mStaging, staging_poison.data(), staging_poison.size());
        writeBuffer(mReadback, readback_poison.data(), readback_poison.size());

        StreamingUploadLifecycle lifecycle;
        lifecycle.mCurrentImage = diagnostic_case.mInputs.mHandles.mOldImage;
        lifecycle.mLastRevision = LLRenderContract::TEXTURE_UPLOAD_PRIOR_REVISION;
        LLRenderVulkanTextureUpload::Registry registry = makeRegistry(
            diagnostic_case, rejection.mRegistry, &lifecycle);
        const auto resolution_before = resolution(registry);
        const StreamingUploadLifecycle lifecycle_before = lifecycle;
        const TextureUploadArtifact result_before = LLRenderContract::makeTextureUploadArtifact();
        TextureUploadArtifact result = result_before;
        const FrameSnapshot frame = mutateFrame(diagnostic_case, rejection.mPacket);
        const std::uint64_t recordings_before = mExecutorRecordings;
        const std::uint64_t submissions_before = mExecutorSubmissions;
        const std::uint32_t validation_before = validationMessageCount();
        std::string execution_error;
        const bool executed = LLRenderVulkanTextureUpload::execute(
            frame, registry, executionContext(rejection.mContext), result, &execution_error);

        const std::vector<std::uint8_t> staging_after = readBuffer(mStaging, staging_poison.size());
        const std::vector<std::uint8_t> readback_after = readBuffer(mReadback, readback_poison.size());
        const auto resolution_after = resolution(registry);
        if (executed || execution_error.empty() || mExecutorRecordings != recordings_before ||
            mExecutorSubmissions != submissions_before || validationMessageCount() != validation_before ||
            result != result_before || lifecycle != lifecycle_before || resolution_after != resolution_before ||
            !std::equal(staging_after.begin(), staging_after.end(), staging_poison.begin()) ||
            !std::equal(readback_after.begin(), readback_after.end(), readback_poison.begin()))
        {
            throw Failure(std::string("fail-closed rejection failed before image snapshot: ") + rejection.mName);
        }
        const std::vector<std::uint8_t> old_after = snapshotImage(mOld);
        const std::vector<std::uint8_t> replacement_after = snapshotImage(mReplacement);
        const std::vector<std::uint8_t> output_after = snapshotImage(mOutputImage);
        if (old_after != old_before || replacement_after != replacement_before || output_after != output_before)
        {
            throw Failure(std::string("fail-closed rejection mutated an image: ") + rejection.mName);
        }
        if (validationMessageCount() != validation_before)
        {
            throw Failure(std::string("fail-closed rejection produced a validation message: ") + rejection.mName);
        }
        ++mRejectionCount;
    }
}

TextureUploadArtifact VulkanTextureUploadRun::run()
{
    readShaders();
    createInstance();
    selectPhysicalDevice();
    createDevice();
    createCommandResources();
    const TextureUploadFixture fixture = LLRenderContract::makeTextureUploadFixture();
    const TextureUploadCase diagnostic_case = LLRenderContract::makeTextureUploadCase();
    createResources(fixture);
    createDescriptors();
    createRenderPass();
    createPipeline();
    seedImages(fixture);
    const std::vector<std::uint8_t> old_snapshot = snapshotImage(mOld);
    if (old_snapshot.size() != mOldPreExecutionSnapshot.size() ||
        !std::equal(old_snapshot.begin(), old_snapshot.end(), fixture.mOldMipRGBA8.begin()))
    {
        throw Failure("old-image pre-execution snapshot does not match the fixed sentinel");
    }
    std::copy(old_snapshot.begin(), old_snapshot.end(), mOldPreExecutionSnapshot.begin());
    runRejections(diagnostic_case, fixture);

    if (mExecutorRecordings != 0 || mExecutorSubmissions != 0)
    {
        throw Failure("rejection matrix did not remain before executor recording");
    }
    const std::vector<std::uint8_t> old_before = snapshotImage(mOld);
    StreamingUploadLifecycle lifecycle;
    lifecycle.mCurrentImage = diagnostic_case.mInputs.mHandles.mOldImage;
    lifecycle.mLastRevision = LLRenderContract::TEXTURE_UPLOAD_PRIOR_REVISION;
    LLRenderVulkanTextureUpload::Registry registry = makeRegistry(
        diagnostic_case, RegistryMutation::None, &lifecycle);
    TextureUploadArtifact result = LLRenderContract::makeTextureUploadArtifact();
    std::string execution_error;
    if (!LLRenderVulkanTextureUpload::execute(diagnostic_case.mFrame, registry,
                                               executionContext(), result, &execution_error))
    {
        throw Failure("valid Vulkan texture upload failed: " + execution_error);
    }
    if (mExecutorRecordings != 1 || mExecutorSubmissions != 1)
    {
        throw Failure("valid Vulkan texture upload did not record and submit exactly once");
    }
    const std::vector<std::uint8_t> old_after = snapshotImage(mOld);
    if (old_after != old_before ||
        !std::equal(old_after.begin(), old_after.end(), fixture.mOldMipRGBA8.begin()))
    {
        throw Failure("valid Vulkan upload mutated the retired old-image sentinel");
    }
    if (lifecycle.mCurrentImage != diagnostic_case.mInputs.mHandles.mReplacementImage ||
        lifecycle.mLastRevision != LLRenderContract::TEXTURE_UPLOAD_REVISION ||
        lifecycle.mCompletionPending || lifecycle.mCompletionCount != 1 ||
        lifecycle.mCompletedDestination != diagnostic_case.mInputs.mHandles.mReplacementImage ||
        lifecycle.mCompletedRevision != LLRenderContract::TEXTURE_UPLOAD_REVISION ||
        lifecycle.mCompletedFrame != LLRenderContract::TEXTURE_UPLOAD_DIAGNOSTIC_FRAME ||
        lifecycle.mRetirementCount != 1 ||
        lifecycle.mRetiredResource != diagnostic_case.mInputs.mHandles.mOldImage ||
        lifecycle.mRetirementFrame != LLRenderContract::TEXTURE_UPLOAD_DIAGNOSTIC_FRAME ||
        registry.isResolvable(diagnostic_case.mInputs.mHandles.mOldImage) ||
        !registry.isResolvable(diagnostic_case.mInputs.mHandles.mReplacementImage))
    {
        throw Failure("valid Vulkan upload did not publish the exact lifecycle transition");
    }
    std::string artifact_error;
    if (!LLRenderContract::validateTextureUploadArtifact(result, &artifact_error))
    {
        throw Failure("Vulkan texture-upload artifact is invalid: " + artifact_error);
    }
    return result;
}

void VulkanTextureUploadRun::shutdown() noexcept
{
    if (mDevice != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(mDevice);
        if (mPipeline != VK_NULL_HANDLE) vkDestroyPipeline(mDevice, mPipeline, nullptr);
        if (mFramebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(mDevice, mFramebuffer, nullptr);
        if (mRenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
        if (mPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
        if (mDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
        if (mDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
        if (mSampler != VK_NULL_HANDLE) vkDestroySampler(mDevice, mSampler, nullptr);
        destroyImage(mOutputImage);
        destroyImage(mReplacement);
        destroyImage(mOld);
        destroyBuffer(mReadback);
        destroyBuffer(mStaging);
        destroyBuffer(mScreen);
        if (mCommandPool != VK_NULL_HANDLE) vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        vkDestroyDevice(mDevice, nullptr);
        mDevice = VK_NULL_HANDLE;
        mQueue = VK_NULL_HANDLE;
        mCommandPool = VK_NULL_HANDLE;
        mCommandBuffer = VK_NULL_HANDLE;
    }
    if (mDebugMessenger != VK_NULL_HANDLE && mInstance != VK_NULL_HANDLE)
    {
        const auto destroy_debug = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroy_debug) destroy_debug(mInstance, mDebugMessenger, nullptr);
        mDebugMessenger = VK_NULL_HANDLE;
    }
    if (mInstance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(mInstance, nullptr);
        mInstance = VK_NULL_HANDLE;
    }
}

std::string boundedDetail(std::string detail)
{
    for (char& character : detail)
    {
        if (character == '\n' || character == '\r' || character == '{' || character == '}')
        {
            character = ' ';
        }
    }
    constexpr std::size_t LIMIT = 240;
    if (detail.size() > LIMIT)
    {
        detail.resize(LIMIT);
    }
    return detail;
}

int fail(const std::string& reason, const std::string& detail)
{
    std::cerr << "VULKAN_TEXTURE_UPLOAD result=fail reason=" << reason;
    if (!detail.empty())
    {
        std::cerr << " detail={" << boundedDetail(detail) << '}';
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
        std::cerr << "usage: llvulkantextureupload --shader-dir <directory> --output <artifact>\n";
        return 2;
    }
    try
    {
        std::string path_error;
        if (occupiedPath(options.mOutput, path_error))
        {
            throw Failure(path_error.empty() ? "output artifact already exists: " + options.mOutput.string()
                                            : path_error);
        }
        VulkanTextureUploadRun runner(options.mShaderDirectory);
        TextureUploadArtifact artifact = runner.run();
        runner.shutdown();
        if (runner.validationMessageCount() != 0)
        {
            throw Failure(std::to_string(runner.validationMessageCount()) + " validation messages");
        }
        std::string artifact_error;
        if (!LLRenderContract::writeTextureUploadArtifact(options.mOutput, artifact, &artifact_error))
        {
            throw Failure(artifact_error);
        }
        if (!artifact_error.empty())
        {
            std::cerr << "VULKAN_TEXTURE_UPLOAD publication_warning=cleanup\n";
        }
        std::cout << "VULKAN_TEXTURE_UPLOAD result=pass"
                  << " rejection_cases=" << runner.rejectionCount()
                  << " recording_attempts=" << runner.recordingAttemptCount()
                  << " submissions=" << runner.submissionCount()
                  << " mip_bytes=" << LLRenderContract::TEXTURE_UPLOAD_MIP_BYTE_COUNT
                  << " sample_bytes=" << LLRenderContract::TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT
                  << " mismatches=0"
                  << " completions=" << artifact.mCompletionCount
                  << " retirements=" << artifact.mRetirementCount
                  << " validation_messages=" << runner.validationMessageCount()
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
