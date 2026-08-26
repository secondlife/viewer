/**
 * @file llvulkantonemap_main.cpp
 * @brief Offscreen Vulkan replay for the fixed-input tonemap diagnostic.
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

#include "llrendervulkantonemap.h"
#include "lltonemapdiagnostic.h"

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
#include <limits>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{

using LLRenderContract::AddressMode;
using LLRenderContract::Extent2D;
using LLRenderContract::Filter;
using LLRenderContract::PixelFormat;
using LLRenderContract::TonemapArtifact;
using LLRenderContract::TonemapCase;
using LLRenderContract::TonemapCases;
using LLRenderContract::TonemapFixture;
using LLRenderContract::TonemapVariant;

constexpr char PORTABILITY_ENUMERATION_EXTENSION[] = "VK_KHR_portability_enumeration";
constexpr char PORTABILITY_SUBSET_EXTENSION[] = "VK_KHR_portability_subset";
constexpr VkDeviceSize SCENE_BYTES =
    LLRenderContract::TONEMAP_DIAGNOSTIC_COMPONENT_COUNT * sizeof(std::uint16_t);
constexpr VkDeviceSize EXPOSURE_OFFSET = SCENE_BYTES;
constexpr VkDeviceSize FIXTURE_STAGING_BYTES = EXPOSURE_OFFSET + 4;
constexpr VkDeviceSize RGBA8_BYTES = LLRenderContract::TONEMAP_DIAGNOSTIC_COMPONENT_COUNT;
constexpr VkDeviceSize RGBA16F_BYTES =
    LLRenderContract::TONEMAP_DIAGNOSTIC_COMPONENT_COUNT * sizeof(std::uint16_t);

constexpr std::array<TonemapVariant, 6> TONEMAP_VARIANTS{
    TonemapVariant::Deferred,
    TonemapVariant::NoPost,
    TonemapVariant::GammaCorrect,
    TonemapVariant::NoPostGammaCorrect,
    TonemapVariant::LegacyGammaCorrect,
    TonemapVariant::NoPostLegacyGammaCorrect
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
        VkResult result = function(&count, values.data());
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

struct ValidationState
{
    std::atomic<std::uint32_t> mMessages{ 0 };
    std::mutex mMutex;
    std::string mFirstMessage;
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

struct Buffer
{
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mMemory = VK_NULL_HANDLE;
    VkDeviceSize mSize = 0;
    VkDeviceSize mAllocationSize = 0;
    bool mCoherent = false;
};

struct Image
{
    VkImage mImage = VK_NULL_HANDLE;
    VkDeviceMemory mMemory = VK_NULL_HANDLE;
    VkImageView mView = VK_NULL_HANDLE;
    VkFormat mFormat = VK_FORMAT_UNDEFINED;
    Extent2D mExtent;
    VkImageUsageFlags mUsage = 0;
};

struct Destination
{
    PixelFormat mContractFormat = PixelFormat::RGBA8Unorm;
    Image mImage;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkFramebuffer mFramebuffer = VK_NULL_HANDLE;
    std::array<VkPipeline, TONEMAP_VARIANTS.size()> mPipelines{};
};

enum class RegistryMutation
{
    None,
    StaleGeneration,
    WrongProgram,
    WrongVariant,
    WrongExtent,
    WrongFormat,
    WrongSampler,
    WrongDescriptors,
    WrongParameterSize
};

const char* mutationName(RegistryMutation mutation)
{
    switch (mutation)
    {
        case RegistryMutation::None: return "none";
        case RegistryMutation::StaleGeneration: return "stale_generation";
        case RegistryMutation::WrongProgram: return "wrong_program";
        case RegistryMutation::WrongVariant: return "wrong_variant";
        case RegistryMutation::WrongExtent: return "wrong_extent";
        case RegistryMutation::WrongFormat: return "wrong_format";
        case RegistryMutation::WrongSampler: return "wrong_sampler";
        case RegistryMutation::WrongDescriptors: return "wrong_descriptors";
        case RegistryMutation::WrongParameterSize: return "wrong_parameter_size";
    }
    return "unknown";
}

std::size_t variantOffset(TonemapVariant variant)
{
    const auto found = std::find(TONEMAP_VARIANTS.begin(), TONEMAP_VARIANTS.end(), variant);
    if (found == TONEMAP_VARIANTS.end())
    {
        throw Failure("tonemap case contains an unsupported variant");
    }
    return static_cast<std::size_t>(found - TONEMAP_VARIANTS.begin());
}

VkFormat vkFormat(PixelFormat format)
{
    switch (format)
    {
        case PixelFormat::RGBA8Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::RGBA16Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
        default: throw Failure("tonemap case contains an unsupported destination format");
    }
}

VkDeviceSize outputByteCount(PixelFormat format)
{
    return format == PixelFormat::RGBA8Unorm ? RGBA8_BYTES : RGBA16F_BYTES;
}

class VulkanTonemapRun
{
public:
    explicit VulkanTonemapRun(std::filesystem::path shader_directory)
        : mShaderDirectory(std::move(shader_directory))
    {
    }

    ~VulkanTonemapRun()
    {
        shutdown();
    }

    TonemapArtifact run()
    {
        verifyShaderFiles();
        createInstance();
        selectPhysicalDevice();
        createDevice();
        createCommandResources();

        const TonemapFixture fixture = LLRenderContract::makeTonemapFixture();
        createFixtureResources(fixture);
        createDescriptorResources();
        createDestinations();
        createPipelines();

        const TonemapCases cases = LLRenderContract::makeTonemapCases();
        runPreflight(cases.front());

        TonemapArtifact artifact = LLRenderContract::makeTonemapArtifact();
        for (std::size_t offset = 0; offset < cases.size(); ++offset)
        {
            const TonemapCase& diagnostic_case = cases[offset];
            Destination& output = destination(diagnostic_case.mKey.mDestinationFormat);
            LLRenderVulkanTonemap::Registry registry = makeRegistry(diagnostic_case, output, RegistryMutation::None);
            std::string execution_error;
            if (!LLRenderVulkanTonemap::execute(diagnostic_case.mFrame, registry, executionContext(), execution_error))
            {
                std::ostringstream message;
                message << "case " << diagnostic_case.mKey.mIndex << " execution failed: " << execution_error;
                throw Failure(message.str());
            }
            artifact.mCases[offset].mPixels = readPixels(output, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        if (mExecutorSubmissions != cases.size())
        {
            throw Failure("executor submission count does not equal the canonical case count");
        }
        std::string artifact_error;
        if (!LLRenderContract::validateTonemapArtifact(artifact, &artifact_error))
        {
            throw Failure("Vulkan readback is not a canonical artifact: " + artifact_error);
        }
        return artifact;
    }

    void shutdown() noexcept
    {
        if (mDevice != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(mDevice);
            for (Destination& output : mDestinations)
            {
                for (VkPipeline pipeline : output.mPipelines)
                {
                    if (pipeline != VK_NULL_HANDLE)
                    {
                        vkDestroyPipeline(mDevice, pipeline, nullptr);
                    }
                }
                if (output.mFramebuffer != VK_NULL_HANDLE)
                {
                    vkDestroyFramebuffer(mDevice, output.mFramebuffer, nullptr);
                }
                if (output.mRenderPass != VK_NULL_HANDLE)
                {
                    vkDestroyRenderPass(mDevice, output.mRenderPass, nullptr);
                }
                destroyImage(output.mImage);
            }
            mDestinations.clear();

            if (mPipelineLayout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
                mPipelineLayout = VK_NULL_HANDLE;
            }
            if (mDescriptorPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
                mDescriptorPool = VK_NULL_HANDLE;
                mDescriptorSet = VK_NULL_HANDLE;
            }
            if (mDescriptorSetLayout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
                mDescriptorSetLayout = VK_NULL_HANDLE;
            }
            if (mPointSampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(mDevice, mPointSampler, nullptr);
                mPointSampler = VK_NULL_HANDLE;
            }
            if (mLinearSampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(mDevice, mLinearSampler, nullptr);
                mLinearSampler = VK_NULL_HANDLE;
            }
            destroyImage(mScene);
            destroyImage(mExposure);
            destroyBuffer(mScreenTriangle);
            destroyBuffer(mTransferBuffer);
            if (mCommandPool != VK_NULL_HANDLE)
            {
                vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
                mCommandPool = VK_NULL_HANDLE;
                mCommandBuffer = VK_NULL_HANDLE;
            }
            vkDestroyDevice(mDevice, nullptr);
            mDevice = VK_NULL_HANDLE;
            mQueue = VK_NULL_HANDLE;
        }

        if (mDebugMessenger != VK_NULL_HANDLE && mInstance != VK_NULL_HANDLE)
        {
            const auto destroy_debug = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT"));
            if (destroy_debug)
            {
                destroy_debug(mInstance, mDebugMessenger, nullptr);
            }
            mDebugMessenger = VK_NULL_HANDLE;
        }
        if (mInstance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(mInstance, nullptr);
            mInstance = VK_NULL_HANDLE;
        }
    }

    std::uint32_t validationMessageCount() const noexcept
    {
        return mValidation.mMessages.load();
    }

    std::string firstValidationMessage()
    {
        std::lock_guard<std::mutex> lock(mValidation.mMutex);
        return mValidation.mFirstMessage;
    }

    std::uint64_t executorSubmissionCount() const noexcept { return mExecutorSubmissions; }
    std::size_t preflightCount() const noexcept { return mPreflightCount; }
    bool usedPortabilityEnumeration() const noexcept { return mPortabilityEnumeration; }
    bool usedPortabilitySubset() const noexcept { return mPortabilitySubset; }

private:
    void verifyShaderFiles() const
    {
        std::error_code error;
        if (!std::filesystem::is_directory(mShaderDirectory, error) || error)
        {
            throw Failure("shader directory is not readable: " + mShaderDirectory.string());
        }
        std::vector<std::filesystem::path> paths{ mShaderDirectory / "tonemap.vert.spv" };
        for (TonemapVariant variant : TONEMAP_VARIANTS)
        {
            paths.push_back(mShaderDirectory /
                            ("tonemap.frag." + std::to_string(static_cast<std::uint64_t>(variant)) + ".spv"));
        }
        for (const std::filesystem::path& path : paths)
        {
            const std::uintmax_t size = std::filesystem::file_size(path, error);
            if (error || size == 0 || size % sizeof(std::uint32_t) != 0 || size > 16U * 1024U * 1024U)
            {
                throw Failure("SPIR-V file is missing or has an invalid size: " + path.string());
            }
        }
    }

    void createInstance()
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
        application.pApplicationName = "llvulkantonemap";
        application.applicationVersion = 1;
        application.pEngineName = "Second Life tonemap diagnostic";
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

    bool hasRequiredFormats(VkPhysicalDevice physical_device) const
    {
        struct FormatRequirement
        {
            VkFormat mFormat;
            VkFormatFeatureFlags mFeatures;
            bool mBufferFeatures = false;
        };
        constexpr std::array requirements{
            FormatRequirement{ VK_FORMAT_R16G16B16A16_SFLOAT,
                               VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                               VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
                               VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
                               VK_FORMAT_FEATURE_TRANSFER_DST_BIT },
            FormatRequirement{ VK_FORMAT_R16_SFLOAT,
                               VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                               VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
                               VK_FORMAT_FEATURE_TRANSFER_DST_BIT },
            FormatRequirement{ VK_FORMAT_R8G8B8A8_UNORM,
                               VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                               VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
                               VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
                               VK_FORMAT_FEATURE_TRANSFER_DST_BIT },
            FormatRequirement{ VK_FORMAT_R32G32B32_SFLOAT,
                               VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT,
                               true }
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
            VkFormat mFormat;
            VkImageUsageFlags mUsage;
            Extent2D mExtent;
        };
        constexpr VkImageUsageFlags destination_usage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        constexpr std::array image_requirements{
            ImageRequirement{ VK_FORMAT_R16G16B16A16_SFLOAT,
                              VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                              { LLRenderContract::TONEMAP_DIAGNOSTIC_WIDTH,
                                LLRenderContract::TONEMAP_DIAGNOSTIC_HEIGHT } },
            ImageRequirement{ VK_FORMAT_R16_SFLOAT,
                              VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                              { 1, 1 } },
            ImageRequirement{ VK_FORMAT_R8G8B8A8_UNORM, destination_usage,
                              { LLRenderContract::TONEMAP_DIAGNOSTIC_WIDTH,
                                LLRenderContract::TONEMAP_DIAGNOSTIC_HEIGHT } },
            ImageRequirement{ VK_FORMAT_R16G16B16A16_SFLOAT, destination_usage,
                              { LLRenderContract::TONEMAP_DIAGNOSTIC_WIDTH,
                                LLRenderContract::TONEMAP_DIAGNOSTIC_HEIGHT } }
        };
        for (const ImageRequirement& requirement : image_requirements)
        {
            VkImageFormatProperties properties{};
            if (vkGetPhysicalDeviceImageFormatProperties(
                    physical_device, requirement.mFormat, VK_IMAGE_TYPE_2D,
                    VK_IMAGE_TILING_OPTIMAL, requirement.mUsage, 0, &properties) != VK_SUCCESS ||
                properties.maxExtent.width < requirement.mExtent.mWidth ||
                properties.maxExtent.height < requirement.mExtent.mHeight ||
                properties.maxMipLevels < 1 || properties.maxArrayLayers < 1 ||
                (properties.sampleCounts & VK_SAMPLE_COUNT_1_BIT) == 0)
            {
                return false;
            }
        }
        return true;
    }

    std::optional<std::uint32_t> graphicsQueueFamily(VkPhysicalDevice physical_device) const
    {
        std::uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties.data());
        for (std::uint32_t index = 0; index < count; ++index)
        {
            if (properties[index].queueCount != 0 &&
                (properties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
            {
                return index;
            }
        }
        return std::nullopt;
    }

    std::vector<VkExtensionProperties> deviceExtensions(VkPhysicalDevice physical_device) const
    {
        return enumerate<VkExtensionProperties>(
            [physical_device](std::uint32_t* count, VkExtensionProperties* values)
            {
                return vkEnumerateDeviceExtensionProperties(physical_device, nullptr, count, values);
            },
            "vkEnumerateDeviceExtensionProperties");
    }

    void selectPhysicalDevice()
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
            if (properties.apiVersion < VK_API_VERSION_1_1 ||
                properties.limits.maxPushConstantsSize < sizeof(LLRenderContract::TonemapParameters) ||
                properties.limits.maxFramebufferWidth < LLRenderContract::TONEMAP_DIAGNOSTIC_WIDTH ||
                properties.limits.maxFramebufferHeight < LLRenderContract::TONEMAP_DIAGNOSTIC_HEIGHT ||
                !queue_family || !hasRequiredFormats(device))
            {
                continue;
            }
            mPhysicalDevice = device;
            mQueueFamily = *queue_family;
            const auto extensions = deviceExtensions(device);
            mPortabilitySubset = hasName(extensions, PORTABILITY_SUBSET_EXTENSION);
            return;
        }
        throw CapabilityFailure("no Vulkan 1.1 graphics device supports the exact tonemap formats and features");
    }

    void createDevice()
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

    std::uint32_t memoryType(std::uint32_t type_bits,
                             VkMemoryPropertyFlags required,
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

    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred)
    {
        Buffer result;
        result.mSize = size;
        VkBufferCreateInfo create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        create_info.size = size;
        create_info.usage = usage;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(mDevice, &create_info, nullptr, &result.mBuffer), "vkCreateBuffer");
        try
        {
            VkMemoryRequirements requirements{};
            vkGetBufferMemoryRequirements(mDevice, result.mBuffer, &requirements);
            VkMemoryPropertyFlags selected = 0;
            const std::uint32_t memory_type =
                memoryType(requirements.memoryTypeBits, required, preferred, selected);
            result.mAllocationSize = requirements.size;
            result.mCoherent = (selected & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
            VkMemoryAllocateInfo allocation{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            allocation.allocationSize = requirements.size;
            allocation.memoryTypeIndex = memory_type;
            check(vkAllocateMemory(mDevice, &allocation, nullptr, &result.mMemory), "vkAllocateMemory(buffer)");
            check(vkBindBufferMemory(mDevice, result.mBuffer, result.mMemory, 0), "vkBindBufferMemory");
            return result;
        }
        catch (...)
        {
            vkDestroyBuffer(mDevice, result.mBuffer, nullptr);
            if (result.mMemory != VK_NULL_HANDLE)
            {
                vkFreeMemory(mDevice, result.mMemory, nullptr);
            }
            throw;
        }
    }

    Image createImage(VkFormat format, Extent2D extent, VkImageUsageFlags usage)
    {
        Image result;
        result.mFormat = format;
        result.mExtent = extent;
        result.mUsage = usage;

        VkImageCreateInfo create_info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        create_info.imageType = VK_IMAGE_TYPE_2D;
        create_info.format = format;
        create_info.extent = { extent.mWidth, extent.mHeight, 1 };
        create_info.mipLevels = 1;
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
            const std::uint32_t memory_type =
                memoryType(requirements.memoryTypeBits, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, selected);
            VkMemoryAllocateInfo allocation{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            allocation.allocationSize = requirements.size;
            allocation.memoryTypeIndex = memory_type;
            check(vkAllocateMemory(mDevice, &allocation, nullptr, &result.mMemory), "vkAllocateMemory(image)");
            check(vkBindImageMemory(mDevice, result.mImage, result.mMemory, 0), "vkBindImageMemory");

            VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            view_info.image = result.mImage;
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = format;
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.layerCount = 1;
            check(vkCreateImageView(mDevice, &view_info, nullptr, &result.mView), "vkCreateImageView");
            return result;
        }
        catch (...)
        {
            if (result.mView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(mDevice, result.mView, nullptr);
            }
            vkDestroyImage(mDevice, result.mImage, nullptr);
            if (result.mMemory != VK_NULL_HANDLE)
            {
                vkFreeMemory(mDevice, result.mMemory, nullptr);
            }
            throw;
        }
    }

    void destroyBuffer(Buffer& buffer) noexcept
    {
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

    void destroyImage(Image& image) noexcept
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

    void writeBuffer(const Buffer& buffer, const void* source, std::size_t size, VkDeviceSize offset = 0)
    {
        if (offset > buffer.mSize || size > buffer.mSize - offset)
        {
            throw Failure("host write exceeds a Vulkan buffer");
        }
        void* mapped = nullptr;
        check(vkMapMemory(mDevice, buffer.mMemory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory(write)");
        std::memcpy(static_cast<std::uint8_t*>(mapped) + offset, source, size);
        if (!buffer.mCoherent)
        {
            VkMappedMemoryRange range{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
            range.memory = buffer.mMemory;
            range.offset = 0;
            range.size = VK_WHOLE_SIZE;
            const VkResult flush_result = vkFlushMappedMemoryRanges(mDevice, 1, &range);
            vkUnmapMemory(mDevice, buffer.mMemory);
            check(flush_result, "vkFlushMappedMemoryRanges");
            return;
        }
        vkUnmapMemory(mDevice, buffer.mMemory);
    }

    std::vector<std::uint8_t> readBuffer(const Buffer& buffer, std::size_t size)
    {
        if (size > buffer.mSize)
        {
            throw Failure("host read exceeds a Vulkan buffer");
        }
        void* mapped = nullptr;
        check(vkMapMemory(mDevice, buffer.mMemory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory(read)");
        if (!buffer.mCoherent)
        {
            VkMappedMemoryRange range{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
            range.memory = buffer.mMemory;
            range.offset = 0;
            range.size = VK_WHOLE_SIZE;
            const VkResult invalidate_result = vkInvalidateMappedMemoryRanges(mDevice, 1, &range);
            if (invalidate_result != VK_SUCCESS)
            {
                vkUnmapMemory(mDevice, buffer.mMemory);
                check(invalidate_result, "vkInvalidateMappedMemoryRanges");
            }
        }
        std::vector<std::uint8_t> bytes(size);
        std::memcpy(bytes.data(), mapped, size);
        vkUnmapMemory(mDevice, buffer.mMemory);
        return bytes;
    }

    void submitImmediate(const std::function<void(VkCommandBuffer)>& commands)
    {
        check(vkResetCommandBuffer(mCommandBuffer, 0), "vkResetCommandBuffer(immediate)");
        VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(mCommandBuffer, &begin), "vkBeginCommandBuffer(immediate)");
        commands(mCommandBuffer);
        check(vkEndCommandBuffer(mCommandBuffer), "vkEndCommandBuffer(immediate)");
        VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &mCommandBuffer;
        check(vkQueueSubmit(mQueue, 1, &submit, VK_NULL_HANDLE), "vkQueueSubmit(immediate)");
        check(vkQueueWaitIdle(mQueue), "vkQueueWaitIdle(immediate)");
    }

    void createCommandResources()
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

        mTransferBuffer = createBuffer(
            std::max(FIXTURE_STAGING_BYTES, RGBA16F_BYTES),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void createFixtureResources(const TonemapFixture& fixture)
    {
        if (fixture.mExtent.mWidth != LLRenderContract::TONEMAP_DIAGNOSTIC_WIDTH ||
            fixture.mExtent.mHeight != LLRenderContract::TONEMAP_DIAGNOSTIC_HEIGHT ||
            fixture.mRowOrigin != LLRenderContract::RowOrigin::BottomLeft)
        {
            throw Failure("shared tonemap fixture metadata is not canonical");
        }

        mScreenTriangle = createBuffer(
            sizeof(fixture.mScreenTriangle), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        writeBuffer(mScreenTriangle, fixture.mScreenTriangle.data(), sizeof(fixture.mScreenTriangle));

        mScene = createImage(VK_FORMAT_R16G16B16A16_SFLOAT, fixture.mExtent,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        mExposure = createImage(VK_FORMAT_R16_SFLOAT, { 1, 1 },
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        std::array<std::uint8_t, FIXTURE_STAGING_BYTES> upload{};
        std::memcpy(upload.data(), fixture.mSceneRGBA16F.data(), SCENE_BYTES);
        std::memcpy(upload.data() + EXPOSURE_OFFSET, &fixture.mExposureR16F,
                    sizeof(fixture.mExposureR16F));
        writeBuffer(mTransferBuffer, upload.data(), upload.size());

        submitImmediate([this](VkCommandBuffer command_buffer)
        {
            VkBufferMemoryBarrier host_barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
            host_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            host_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            host_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            host_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            host_barrier.buffer = mTransferBuffer.mBuffer;
            host_barrier.size = FIXTURE_STAGING_BYTES;
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr, 1, &host_barrier, 0, nullptr);

            std::array<VkImageMemoryBarrier, 2> to_transfer{};
            for (VkImageMemoryBarrier& barrier : to_transfer)
            {
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.layerCount = 1;
            }
            to_transfer[0].image = mScene.mImage;
            to_transfer[1].image = mExposure.mImage;
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr, 0, nullptr,
                                 static_cast<std::uint32_t>(to_transfer.size()), to_transfer.data());

            VkBufferImageCopy scene_copy{};
            scene_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            scene_copy.imageSubresource.layerCount = 1;
            scene_copy.imageExtent = { mScene.mExtent.mWidth, mScene.mExtent.mHeight, 1 };
            vkCmdCopyBufferToImage(command_buffer, mTransferBuffer.mBuffer, mScene.mImage,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &scene_copy);

            VkBufferImageCopy exposure_copy{};
            exposure_copy.bufferOffset = EXPOSURE_OFFSET;
            exposure_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            exposure_copy.imageSubresource.layerCount = 1;
            exposure_copy.imageExtent = { 1, 1, 1 };
            vkCmdCopyBufferToImage(command_buffer, mTransferBuffer.mBuffer, mExposure.mImage,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &exposure_copy);

            std::array<VkImageMemoryBarrier, 2> to_shader = to_transfer;
            for (VkImageMemoryBarrier& barrier : to_shader)
            {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr, 0, nullptr,
                                 static_cast<std::uint32_t>(to_shader.size()), to_shader.data());
        });
    }

    VkSampler createSampler(VkFilter filter)
    {
        VkSamplerCreateInfo create_info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        create_info.magFilter = filter;
        create_info.minFilter = filter;
        create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        create_info.mipLodBias = 0.f;
        create_info.anisotropyEnable = VK_FALSE;
        create_info.maxAnisotropy = 1.f;
        create_info.compareEnable = VK_FALSE;
        create_info.compareOp = VK_COMPARE_OP_ALWAYS;
        create_info.minLod = 0.f;
        create_info.maxLod = 0.f;
        create_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        create_info.unnormalizedCoordinates = VK_FALSE;
        VkSampler sampler = VK_NULL_HANDLE;
        check(vkCreateSampler(mDevice, &create_info, nullptr, &sampler), "vkCreateSampler");
        return sampler;
    }

    void createDescriptorResources()
    {
        mPointSampler = createSampler(VK_FILTER_NEAREST);
        mLinearSampler = createSampler(VK_FILTER_LINEAR);

        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
        for (std::uint32_t binding = 0; binding < bindings.size(); ++binding)
        {
            bindings[binding].binding = binding;
            bindings[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[binding].descriptorCount = 1;
            bindings[binding].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        VkDescriptorSetLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();
        check(vkCreateDescriptorSetLayout(mDevice, &layout_info, nullptr, &mDescriptorSetLayout),
              "vkCreateDescriptorSetLayout");

        VkPushConstantRange push_constant{};
        push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        push_constant.offset = 0;
        push_constant.size = sizeof(LLRenderContract::TonemapParameters);
        static_assert(sizeof(LLRenderContract::TonemapParameters) == 16);
        VkPipelineLayoutCreateInfo pipeline_layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &mDescriptorSetLayout;
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &push_constant;
        check(vkCreatePipelineLayout(mDevice, &pipeline_layout_info, nullptr, &mPipelineLayout),
              "vkCreatePipelineLayout");

        VkDescriptorPoolSize pool_size{};
        pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_size.descriptorCount = 2;
        VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = &pool_size;
        check(vkCreateDescriptorPool(mDevice, &pool_info, nullptr, &mDescriptorPool),
              "vkCreateDescriptorPool");

        VkDescriptorSetAllocateInfo allocation{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocation.descriptorPool = mDescriptorPool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &mDescriptorSetLayout;
        check(vkAllocateDescriptorSets(mDevice, &allocation, &mDescriptorSet),
              "vkAllocateDescriptorSets");

        std::array<VkDescriptorImageInfo, 2> image_info{};
        image_info[0] = { mPointSampler, mScene.mView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        image_info[1] = { mLinearSampler, mExposure.mView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        std::array<VkWriteDescriptorSet, 2> writes{};
        for (std::uint32_t binding = 0; binding < writes.size(); ++binding)
        {
            writes[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[binding].dstSet = mDescriptorSet;
            writes[binding].dstBinding = binding;
            writes[binding].descriptorCount = 1;
            writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[binding].pImageInfo = &image_info[binding];
        }
        vkUpdateDescriptorSets(mDevice, static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    void createDestinations()
    {
        for (PixelFormat format : { PixelFormat::RGBA8Unorm, PixelFormat::RGBA16Float })
        {
            mDestinations.emplace_back();
            Destination& output = mDestinations.back();
            output.mContractFormat = format;
            output.mImage = createImage(vkFormat(format),
                                        { LLRenderContract::TONEMAP_DIAGNOSTIC_WIDTH,
                                          LLRenderContract::TONEMAP_DIAGNOSTIC_HEIGHT },
                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                        VK_IMAGE_USAGE_SAMPLED_BIT |
                                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                        VK_IMAGE_USAGE_TRANSFER_DST_BIT);

            VkAttachmentDescription attachment{};
            attachment.format = output.mImage.mFormat;
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkAttachmentReference color_reference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color_reference;
            std::array<VkSubpassDependency, 2> dependencies{};
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
            render_pass_info.attachmentCount = 1;
            render_pass_info.pAttachments = &attachment;
            render_pass_info.subpassCount = 1;
            render_pass_info.pSubpasses = &subpass;
            render_pass_info.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
            render_pass_info.pDependencies = dependencies.data();
            check(vkCreateRenderPass(mDevice, &render_pass_info, nullptr, &output.mRenderPass),
                  "vkCreateRenderPass");

            VkFramebufferCreateInfo framebuffer_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            framebuffer_info.renderPass = output.mRenderPass;
            framebuffer_info.attachmentCount = 1;
            framebuffer_info.pAttachments = &output.mImage.mView;
            framebuffer_info.width = output.mImage.mExtent.mWidth;
            framebuffer_info.height = output.mImage.mExtent.mHeight;
            framebuffer_info.layers = 1;
            check(vkCreateFramebuffer(mDevice, &framebuffer_info, nullptr, &output.mFramebuffer),
                  "vkCreateFramebuffer");
        }
    }

    std::vector<std::uint32_t> readSpirv(const std::filesystem::path& path) const
    {
        std::error_code file_error;
        const std::uintmax_t file_size = std::filesystem::file_size(path, file_error);
        if (file_error || file_size == 0 || file_size % sizeof(std::uint32_t) != 0 ||
            file_size > 16U * 1024U * 1024U)
        {
            throw Failure("SPIR-V file has an invalid size: " + path.string());
        }
        std::ifstream input(path, std::ios::binary | std::ios::in);
        if (!input)
        {
            throw Failure("cannot open SPIR-V file: " + path.string());
        }
        std::vector<std::uint32_t> words(static_cast<std::size_t>(file_size) / sizeof(std::uint32_t));
        input.read(reinterpret_cast<char*>(words.data()), static_cast<std::streamsize>(file_size));
        if (!input || input.peek() != std::ifstream::traits_type::eof())
        {
            throw Failure("cannot read complete SPIR-V file: " + path.string());
        }
        return words;
    }

    VkShaderModule createShaderModule(const std::filesystem::path& path)
    {
        const std::vector<std::uint32_t> words = readSpirv(path);
        VkShaderModuleCreateInfo create_info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        create_info.codeSize = words.size() * sizeof(std::uint32_t);
        create_info.pCode = words.data();
        VkShaderModule module = VK_NULL_HANDLE;
        check(vkCreateShaderModule(mDevice, &create_info, nullptr, &module), "vkCreateShaderModule");
        return module;
    }

    VkPipeline createPipeline(VkShaderModule vertex_module, VkShaderModule fragment_module,
                              VkRenderPass render_pass)
    {
        std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertex_module;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragment_module;
        stages[1].pName = "main";

        VkVertexInputBindingDescription vertex_binding{};
        vertex_binding.binding = 0;
        vertex_binding.stride = 16;
        vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        VkVertexInputAttributeDescription vertex_attribute{};
        vertex_attribute.location = 0;
        vertex_attribute.binding = 0;
        vertex_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_attribute.offset = 0;
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
        VkPipelineRasterizationStateCreateInfo rasterization{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterization.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization.cullMode = VK_CULL_MODE_NONE;
        rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization.lineWidth = 1.f;
        VkPipelineMultisampleStateCreateInfo multisample{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depth.depthTestEnable = VK_FALSE;
        depth.depthWriteEnable = VK_FALSE;
        depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        VkPipelineColorBlendAttachmentState color_attachment{};
        color_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        blend.attachmentCount = 1;
        blend.pAttachments = &color_attachment;
        constexpr std::array<VkDynamicState, 2> dynamic_states{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamic{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic.pDynamicStates = dynamic_states.data();

        VkGraphicsPipelineCreateInfo create_info{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        create_info.stageCount = static_cast<std::uint32_t>(stages.size());
        create_info.pStages = stages.data();
        create_info.pVertexInputState = &vertex_input;
        create_info.pInputAssemblyState = &assembly;
        create_info.pViewportState = &viewport;
        create_info.pRasterizationState = &rasterization;
        create_info.pMultisampleState = &multisample;
        create_info.pDepthStencilState = &depth;
        create_info.pColorBlendState = &blend;
        create_info.pDynamicState = &dynamic;
        create_info.layout = mPipelineLayout;
        create_info.renderPass = render_pass;
        create_info.subpass = 0;
        create_info.basePipelineIndex = -1;
        VkPipeline pipeline = VK_NULL_HANDLE;
        check(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline),
              "vkCreateGraphicsPipelines");
        return pipeline;
    }

    void createPipelines()
    {
        VkShaderModule vertex_module = VK_NULL_HANDLE;
        std::array<VkShaderModule, TONEMAP_VARIANTS.size()> fragment_modules{};
        try
        {
            vertex_module = createShaderModule(mShaderDirectory / "tonemap.vert.spv");
            for (std::size_t offset = 0; offset < TONEMAP_VARIANTS.size(); ++offset)
            {
                const auto variant = static_cast<std::uint64_t>(TONEMAP_VARIANTS[offset]);
                fragment_modules[offset] = createShaderModule(
                    mShaderDirectory / ("tonemap.frag." + std::to_string(variant) + ".spv"));
            }
            for (Destination& output : mDestinations)
            {
                for (std::size_t offset = 0; offset < fragment_modules.size(); ++offset)
                {
                    output.mPipelines[offset] =
                        createPipeline(vertex_module, fragment_modules[offset], output.mRenderPass);
                }
            }
        }
        catch (...)
        {
            for (VkShaderModule module : fragment_modules)
            {
                if (module != VK_NULL_HANDLE)
                {
                    vkDestroyShaderModule(mDevice, module, nullptr);
                }
            }
            if (vertex_module != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(mDevice, vertex_module, nullptr);
            }
            throw;
        }
        for (VkShaderModule module : fragment_modules)
        {
            vkDestroyShaderModule(mDevice, module, nullptr);
        }
        vkDestroyShaderModule(mDevice, vertex_module, nullptr);
    }

    Destination& destination(PixelFormat format)
    {
        const auto found = std::find_if(mDestinations.begin(), mDestinations.end(), [format](const Destination& output)
        {
            return output.mContractFormat == format;
        });
        if (found == mDestinations.end())
        {
            throw Failure("destination format was not created");
        }
        return *found;
    }

    LLRenderVulkanTonemap::ExecutionContext executionContext()
    {
        return { mDevice, mCommandBuffer, mQueue, &mExecutorSubmissions };
    }

    LLRenderVulkanTonemap::Registry makeRegistry(const TonemapCase& diagnostic_case,
                                                 Destination& output,
                                                 RegistryMutation mutation)
    {
        const auto& handles = diagnostic_case.mInputs.mHandles;
        LLRenderVulkanTonemap::Registry registry;

        auto scene_handle = handles.mScene;
        if (mutation == RegistryMutation::StaleGeneration)
        {
            ++scene_handle.mGeneration;
        }
        LLRenderVulkanTonemap::ImageBinding destination_binding{
            output.mImage.mImage,
            output.mImage.mView,
            output.mImage.mFormat,
            output.mImage.mExtent,
            output.mImage.mUsage
        };
        if (mutation == RegistryMutation::WrongExtent)
        {
            --destination_binding.mExtent.mWidth;
        }
        if (mutation == RegistryMutation::WrongFormat)
        {
            destination_binding.mFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        }

        LLRenderVulkanTonemap::SamplerBinding point_sampler{
            mPointSampler, Filter::Nearest, Filter::Nearest, AddressMode::Mirror, AddressMode::Mirror
        };
        if (mutation == RegistryMutation::WrongSampler)
        {
            point_sampler.mMinFilter = Filter::Linear;
        }
        const LLRenderVulkanTonemap::SamplerBinding linear_sampler{
            mLinearSampler, Filter::Linear, Filter::Linear, AddressMode::Mirror, AddressMode::Mirror
        };

        LLRenderVulkanTonemap::PipelineBinding pipeline;
        pipeline.mProgram = { "deferred.tonemap", static_cast<std::uint64_t>(diagnostic_case.mInputs.mVariant) };
        if (mutation == RegistryMutation::WrongProgram)
        {
            pipeline.mProgram.mName = "wrong.tonemap";
        }
        if (mutation == RegistryMutation::WrongVariant)
        {
            pipeline.mProgram.mVariant = pipeline.mProgram.mVariant == 0 ? 1 : 0;
        }
        pipeline.mDestinationFormat = diagnostic_case.mInputs.mDestinationFormat;
        pipeline.mExtent = diagnostic_case.mInputs.mDestinationExtent;
        pipeline.mPipeline = output.mPipelines[variantOffset(diagnostic_case.mInputs.mVariant)];
        pipeline.mLayout = mPipelineLayout;
        pipeline.mRenderPass = output.mRenderPass;
        pipeline.mFramebuffer = output.mFramebuffer;
        pipeline.mDescriptorSet = mDescriptorSet;
        pipeline.mSceneView = mScene.mView;
        pipeline.mExposureView = mExposure.mView;
        pipeline.mDestinationView = output.mImage.mView;
        pipeline.mPointSampler = mPointSampler;
        pipeline.mLinearSampler = mLinearSampler;
        pipeline.mDestinationFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        pipeline.mDescriptorBindings = mutation == RegistryMutation::WrongDescriptors
                                           ? std::vector<std::uint32_t>{ 0 }
                                           : std::vector<std::uint32_t>{ 0, 1 };
        pipeline.mVertexStride = 16;
        pipeline.mPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
        pipeline.mPositionOffset = 0;
        pipeline.mPushConstantSize = mutation == RegistryMutation::WrongParameterSize
                                         ? 12
                                         : sizeof(LLRenderContract::TonemapParameters);

        const bool registered =
            registry.addBuffer(handles.mScreenTriangle,
                               { mScreenTriangle.mBuffer, mScreenTriangle.mSize,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT }) &&
            registry.addImage(scene_handle,
                              { mScene.mImage, mScene.mView, mScene.mFormat, mScene.mExtent, mScene.mUsage }) &&
            registry.addImage(handles.mExposure,
                              { mExposure.mImage, mExposure.mView, mExposure.mFormat,
                                mExposure.mExtent, mExposure.mUsage }) &&
            registry.addImage(handles.mDestination, destination_binding) &&
            registry.addSampler(handles.mPointSampler, point_sampler) &&
            registry.addSampler(handles.mLinearSampler, linear_sampler) &&
            registry.addPipeline(handles.mPipeline, pipeline);
        if (!registered)
        {
            throw Failure("could not register complete Vulkan tonemap resources");
        }
        return registry;
    }

    void seedDestination(Destination& output, const std::vector<std::uint8_t>& bytes)
    {
        if (bytes.size() != outputByteCount(output.mContractFormat))
        {
            throw Failure("destination seed has the wrong byte count");
        }
        writeBuffer(mTransferBuffer, bytes.data(), bytes.size());
        submitImmediate([this, &output, size = bytes.size()](VkCommandBuffer command_buffer)
        {
            VkBufferMemoryBarrier host_barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
            host_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            host_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            host_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            host_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            host_barrier.buffer = mTransferBuffer.mBuffer;
            host_barrier.size = size;
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr, 1, &host_barrier, 0, nullptr);

            VkImageMemoryBarrier image_barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.image = output.mImage.mImage;
            image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_barrier.subresourceRange.levelCount = 1;
            image_barrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &image_barrier);

            VkBufferImageCopy copy{};
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.layerCount = 1;
            copy.imageExtent = { output.mImage.mExtent.mWidth, output.mImage.mExtent.mHeight, 1 };
            vkCmdCopyBufferToImage(command_buffer, mTransferBuffer.mBuffer, output.mImage.mImage,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        });
    }

    std::vector<std::uint8_t> readRaw(Destination& output, VkImageLayout old_layout)
    {
        const VkDeviceSize size = outputByteCount(output.mContractFormat);
        submitImmediate([this, &output, old_layout, size](VkCommandBuffer command_buffer)
        {
            VkImageMemoryBarrier image_barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            const bool transfer_destination = old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            if (!transfer_destination && old_layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                throw Failure("readback image is not transfer-destination or shader-readable");
            }
            image_barrier.srcAccessMask = transfer_destination
                                               ? VK_ACCESS_TRANSFER_WRITE_BIT
                                               : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            image_barrier.oldLayout = old_layout;
            image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.image = output.mImage.mImage;
            image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_barrier.subresourceRange.levelCount = 1;
            image_barrier.subresourceRange.layerCount = 1;
            const VkPipelineStageFlags source_stage =
                transfer_destination
                    ? VK_PIPELINE_STAGE_TRANSFER_BIT
                    : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            vkCmdPipelineBarrier(command_buffer, source_stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &image_barrier);

            VkBufferImageCopy copy{};
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.layerCount = 1;
            copy.imageExtent = { output.mImage.mExtent.mWidth, output.mImage.mExtent.mHeight, 1 };
            vkCmdCopyImageToBuffer(command_buffer, output.mImage.mImage,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   mTransferBuffer.mBuffer, 1, &copy);

            VkImageMemoryBarrier restore_barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            restore_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            restore_barrier.dstAccessMask = transfer_destination
                                                 ? VK_ACCESS_TRANSFER_WRITE_BIT
                                                 : VK_ACCESS_SHADER_READ_BIT;
            restore_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            restore_barrier.newLayout = old_layout;
            restore_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            restore_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            restore_barrier.image = output.mImage.mImage;
            restore_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            restore_barrier.subresourceRange.levelCount = 1;
            restore_barrier.subresourceRange.layerCount = 1;
            const VkPipelineStageFlags restore_stage = transfer_destination
                                                           ? VK_PIPELINE_STAGE_TRANSFER_BIT
                                                           : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 restore_stage, 0,
                                 0, nullptr, 0, nullptr, 1, &restore_barrier);

            VkBufferMemoryBarrier host_barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
            host_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            host_barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
            host_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            host_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            host_barrier.buffer = mTransferBuffer.mBuffer;
            host_barrier.size = size;
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_HOST_BIT, 0,
                                 0, nullptr, 1, &host_barrier, 0, nullptr);
        });
        return readBuffer(mTransferBuffer, static_cast<std::size_t>(size));
    }

    std::vector<float> readPixels(Destination& output, VkImageLayout old_layout)
    {
        const std::vector<std::uint8_t> raw = readRaw(output, old_layout);
        std::vector<float> pixels;
        pixels.reserve(LLRenderContract::TONEMAP_DIAGNOSTIC_COMPONENT_COUNT);
        constexpr std::size_t width = LLRenderContract::TONEMAP_DIAGNOSTIC_WIDTH;
        constexpr std::size_t height = LLRenderContract::TONEMAP_DIAGNOSTIC_HEIGHT;
        constexpr std::size_t channels = LLRenderContract::TONEMAP_DIAGNOSTIC_CHANNELS;
        if (output.mContractFormat == PixelFormat::RGBA8Unorm)
        {
            const std::size_t row_bytes = width * channels;
            for (std::size_t bottom_row = 0; bottom_row < height; ++bottom_row)
            {
                const std::size_t source_row = height - 1 - bottom_row;
                for (std::size_t component = 0; component < row_bytes; ++component)
                {
                    pixels.push_back(static_cast<float>(raw[source_row * row_bytes + component]) / 255.f);
                }
            }
            return pixels;
        }
        const std::size_t row_bytes = width * channels * sizeof(std::uint16_t);
        for (std::size_t bottom_row = 0; bottom_row < height; ++bottom_row)
        {
            const std::size_t source_row = height - 1 - bottom_row;
            for (std::size_t offset = 0; offset < row_bytes; offset += sizeof(std::uint16_t))
            {
                std::uint16_t bits = 0;
                std::memcpy(&bits, raw.data() + source_row * row_bytes + offset, sizeof(bits));
                pixels.push_back(LLRenderContract::halfBitsToFloat(bits));
            }
        }
        return pixels;
    }

    void runPreflight(const TonemapCase& diagnostic_case)
    {
        constexpr std::array mutations{
            RegistryMutation::StaleGeneration,
            RegistryMutation::WrongProgram,
            RegistryMutation::WrongVariant,
            RegistryMutation::WrongExtent,
            RegistryMutation::WrongFormat,
            RegistryMutation::WrongSampler,
            RegistryMutation::WrongDescriptors,
            RegistryMutation::WrongParameterSize
        };
        Destination& output = destination(PixelFormat::RGBA8Unorm);
        std::vector<std::uint8_t> sentinel(static_cast<std::size_t>(RGBA8_BYTES));
        for (std::size_t offset = 0; offset < sentinel.size(); ++offset)
        {
            sentinel[offset] = static_cast<std::uint8_t>((offset * 37U + 11U) & 0xffU);
        }

        for (RegistryMutation mutation : mutations)
        {
            seedDestination(output, sentinel);
            LLRenderVulkanTonemap::Registry registry = makeRegistry(diagnostic_case, output, mutation);
            const std::uint64_t submissions_before = mExecutorSubmissions;
            std::string execution_error;
            const bool executed = LLRenderVulkanTonemap::execute(
                diagnostic_case.mFrame, registry, executionContext(), execution_error);
            const VkImageLayout read_layout = executed
                                                   ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                   : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            const std::vector<std::uint8_t> after = readRaw(output, read_layout);
            if (executed || execution_error.empty() || mExecutorSubmissions != submissions_before ||
                after != sentinel)
            {
                std::ostringstream message;
                message << "preflight " << mutationName(mutation)
                        << " did not reject without submission and destination mutation";
                throw Failure(message.str());
            }
            ++mPreflightCount;
        }
    }

    std::filesystem::path mShaderDirectory;
    ValidationState mValidation;
    VkInstance mInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    std::uint32_t mQueueFamily = 0;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkQueue mQueue = VK_NULL_HANDLE;
    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    Buffer mTransferBuffer;
    Buffer mScreenTriangle;
    Image mScene;
    Image mExposure;
    VkSampler mPointSampler = VK_NULL_HANDLE;
    VkSampler mLinearSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
    std::vector<Destination> mDestinations;
    std::uint64_t mExecutorSubmissions = 0;
    std::size_t mPreflightCount = 0;
    bool mPortabilityEnumeration = false;
    bool mPortabilitySubset = false;
};

int fail(const std::string& reason, const std::string& detail)
{
    std::cerr << "VULKAN_TONEMAP result=fail reason=" << reason;
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
        std::cerr << "usage: llvulkantonemap --shader-dir <directory> --output <artifact>\n";
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
        VulkanTonemapRun runner(options.mShaderDirectory);
        TonemapArtifact artifact = runner.run();
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
        if (!LLRenderContract::writeTonemapArtifact(options.mOutput, artifact, &artifact_error))
        {
            throw Failure(artifact_error);
        }

        std::cout << "VULKAN_TONEMAP result=pass"
                  << " cases=" << artifact.mCases.size()
                  << " preflight=" << runner.preflightCount()
                  << " submissions=" << runner.executorSubmissionCount()
                  << " validation_messages=" << runner.validationMessageCount()
                  << " portability_enumeration="
                  << (runner.usedPortabilityEnumeration() ? "enabled" : "not_advertised")
                  << " portability_subset="
                  << (runner.usedPortabilitySubset() ? "enabled" : "not_advertised")
                  << '\n';
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
