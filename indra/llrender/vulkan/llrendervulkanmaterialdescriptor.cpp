/**
 * @file llrendervulkanmaterialdescriptor.cpp
 * @brief Immutable populated Vulkan descriptors for the canonical material interface.
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

#include "llrendervulkanmaterialdescriptor.h"

#include "llmaterialcontract.h"

#include <limits>
#include <new>
#include <utility>

namespace LLRenderVulkanMaterial
{
namespace
{

    MaterialDescriptorCreationError failure(MaterialDescriptorCreationCode code,
                                            std::optional<std::size_t>     tuple_index         = std::nullopt,
                                            std::optional<std::uint32_t>   sampled_image_index = std::nullopt,
                                            VkResult                       result              = VK_SUCCESS) noexcept
    {
        return { code, tuple_index, sampled_image_index, result };
    }

    bool validUniformRange(const MaterialUniformDescriptorResource& uniform) noexcept
    {
        constexpr VkDeviceSize PARAMETER_SIZE = static_cast<VkDeviceSize>(sizeof(LLRenderContract::MaterialParameters));
        return uniform.mOffset <= uniform.mSize && uniform.mSize - uniform.mOffset >= PARAMETER_SIZE;
    }

} // namespace

struct MaterialDescriptorFactory
{
    static std::unique_ptr<LegacyNormSpecDescriptorGeneration> allocate(const MaterialDescriptorDevice&          device,
                                                                        std::vector<MaterialDescriptorBinding>&& bindings) noexcept
    {
        return std::unique_ptr<LegacyNormSpecDescriptorGeneration>(new (std::nothrow)
                                                                       LegacyNormSpecDescriptorGeneration(device, std::move(bindings)));
    }

    static VkDescriptorPool& pool(LegacyNormSpecDescriptorGeneration& generation) noexcept { return generation.mPool; }

    static MaterialDescriptorBinding& binding(LegacyNormSpecDescriptorGeneration& generation, std::size_t index) noexcept
    {
        return generation.mBindings[index];
    }
};

LegacyNormSpecDescriptorGeneration::LegacyNormSpecDescriptorGeneration(const MaterialDescriptorDevice&          device,
                                                                       std::vector<MaterialDescriptorBinding>&& bindings) noexcept :
    mDevice(device.mDevice),
    mDestroyDescriptorPool(device.mDispatch.mDestroyDescriptorPool),
    mBindings(std::move(bindings))
{
}

LegacyNormSpecDescriptorGeneration::~LegacyNormSpecDescriptorGeneration() noexcept
{
    if (mPool != VK_NULL_HANDLE)
    {
        mDestroyDescriptorPool(mDevice, mPool, nullptr);
    }
}

std::optional<MaterialDescriptorBinding> LegacyNormSpecDescriptorGeneration::binding(std::size_t index) const noexcept
{
    if (index >= mBindings.size())
    {
        return std::nullopt;
    }
    return mBindings[index];
}

bool validMaterialDescriptorGenerationCount(std::size_t count) noexcept
{
    constexpr std::size_t MAX_COUNT = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) / 4U;
    return count != 0 && count <= MAX_COUNT;
}

MaterialDescriptorCreationResult createLegacyNormSpecDescriptorGeneration(
    const MaterialDescriptorDevice&                 device,
    const LegacyNormSpecPipelineLayout&             layout,
    const std::vector<MaterialDescriptorResources>& resources) noexcept
{
    if (device.mDevice == VK_NULL_HANDLE)
    {
        return failure(MaterialDescriptorCreationCode::InvalidDevice);
    }
    if (!device.mDispatch.mCreateDescriptorPool || !device.mDispatch.mDestroyDescriptorPool || !device.mDispatch.mAllocateDescriptorSets ||
        !device.mDispatch.mUpdateDescriptorSets)
    {
        return failure(MaterialDescriptorCreationCode::InvalidDispatch);
    }
    if (!layout.createdOn(device.mDevice))
    {
        return failure(MaterialDescriptorCreationCode::LayoutDeviceMismatch);
    }
    if (resources.empty())
    {
        return failure(MaterialDescriptorCreationCode::EmptyBatch);
    }
    if (!validMaterialDescriptorGenerationCount(resources.size()))
    {
        return failure(MaterialDescriptorCreationCode::BatchTooLarge);
    }

    for (std::size_t tuple_index = 0; tuple_index < resources.size(); ++tuple_index)
    {
        const MaterialDescriptorResources& tuple = resources[tuple_index];
        if (tuple.mParameters.mBuffer == VK_NULL_HANDLE)
        {
            return failure(MaterialDescriptorCreationCode::InvalidUniformBuffer, tuple_index);
        }
        if (!validUniformRange(tuple.mParameters))
        {
            return failure(MaterialDescriptorCreationCode::InvalidUniformRange, tuple_index);
        }
        for (std::uint32_t image_index = 0; image_index < tuple.mSampledImages.size(); ++image_index)
        {
            const MaterialSampledDescriptorResource& sampled = tuple.mSampledImages[image_index];
            if (sampled.mView == VK_NULL_HANDLE)
            {
                return failure(MaterialDescriptorCreationCode::InvalidSampledImage, tuple_index, image_index);
            }
            if (sampled.mSampler == VK_NULL_HANDLE)
            {
                return failure(MaterialDescriptorCreationCode::InvalidSampler, tuple_index, image_index);
            }
        }
    }

    const std::uint32_t tuple_count = static_cast<std::uint32_t>(resources.size());
    const std::uint32_t set_count   = tuple_count * 2U;
    const std::uint32_t image_count = tuple_count * 3U;
    const std::uint32_t write_count = tuple_count * 4U;

    std::vector<MaterialDescriptorBinding> bindings;
    std::vector<VkDescriptorSetLayout>     set_layouts;
    std::vector<VkDescriptorSet>           descriptor_sets;
    std::vector<VkDescriptorBufferInfo>    buffer_infos;
    std::vector<VkDescriptorImageInfo>     image_infos;
    std::vector<VkWriteDescriptorSet>      writes;
    try
    {
        bindings.resize(resources.size());
        set_layouts.resize(set_count);
        descriptor_sets.resize(set_count, VK_NULL_HANDLE);
        buffer_infos.resize(tuple_count);
        image_infos.resize(image_count);
        writes.resize(write_count);
    }
    catch (...)
    {
        return failure(MaterialDescriptorCreationCode::OwnerAllocationFailure);
    }

    const std::array<VkDescriptorSetLayout, 2> canonical_layouts = layout.descriptorSetLayouts();
    for (std::size_t tuple_index = 0; tuple_index < resources.size(); ++tuple_index)
    {
        bindings[tuple_index].mResources   = resources[tuple_index];
        set_layouts[tuple_index * 2U]      = canonical_layouts[0];
        set_layouts[tuple_index * 2U + 1U] = canonical_layouts[1];

        const MaterialUniformDescriptorResource& uniform     = resources[tuple_index].mParameters;
        VkDescriptorBufferInfo&                  buffer_info = buffer_infos[tuple_index];
        buffer_info.buffer                                   = uniform.mBuffer;
        buffer_info.offset                                   = uniform.mOffset;
        buffer_info.range                                    = static_cast<VkDeviceSize>(sizeof(LLRenderContract::MaterialParameters));

        VkWriteDescriptorSet& uniform_write = writes[tuple_index * 4U];
        uniform_write.sType                 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniform_write.dstBinding            = 0;
        uniform_write.descriptorCount       = 1;
        uniform_write.descriptorType        = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_write.pBufferInfo           = &buffer_info;

        for (std::uint32_t image_index = 0; image_index < 3U; ++image_index)
        {
            const std::size_t                        image_offset = tuple_index * 3U + image_index;
            const MaterialSampledDescriptorResource& sampled      = resources[tuple_index].mSampledImages[image_index];
            VkDescriptorImageInfo&                   image_info   = image_infos[image_offset];
            image_info.sampler                                    = sampled.mSampler;
            image_info.imageView                                  = sampled.mView;
            image_info.imageLayout                                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet& image_write = writes[tuple_index * 4U + image_index + 1U];
            image_write.sType                 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            image_write.dstBinding            = image_index;
            image_write.descriptorCount       = 1;
            image_write.descriptorType        = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            image_write.pImageInfo            = &image_info;
        }
    }

    std::unique_ptr<LegacyNormSpecDescriptorGeneration> generation = MaterialDescriptorFactory::allocate(device, std::move(bindings));
    if (!generation)
    {
        return failure(MaterialDescriptorCreationCode::OwnerAllocationFailure);
    }

    const std::array<VkDescriptorPoolSize, 2> pool_sizes{ VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, tuple_count },
                                                          VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, image_count } };
    VkDescriptorPoolCreateInfo                pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets       = set_count;
    pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes    = pool_sizes.data();

    VkDescriptorPool pool        = VK_NULL_HANDLE;
    const VkResult   pool_result = device.mDispatch.mCreateDescriptorPool(device.mDevice, &pool_info, nullptr, &pool);
    if (pool_result != VK_SUCCESS)
    {
        return failure(MaterialDescriptorCreationCode::PoolCreateFailure, std::nullopt, std::nullopt, pool_result);
    }
    if (pool == VK_NULL_HANDLE)
    {
        return failure(MaterialDescriptorCreationCode::NullPool);
    }
    MaterialDescriptorFactory::pool(*generation) = pool;

    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool     = pool;
    allocate_info.descriptorSetCount = set_count;
    allocate_info.pSetLayouts        = set_layouts.data();
    const VkResult allocation_result = device.mDispatch.mAllocateDescriptorSets(device.mDevice, &allocate_info, descriptor_sets.data());
    if (allocation_result != VK_SUCCESS)
    {
        return failure(MaterialDescriptorCreationCode::SetAllocationFailure, std::nullopt, std::nullopt, allocation_result);
    }

    for (std::size_t set_index = 0; set_index < descriptor_sets.size(); ++set_index)
    {
        if (descriptor_sets[set_index] != VK_NULL_HANDLE)
        {
            continue;
        }
        const std::size_t tuple_index = set_index / 2U;
        return failure(set_index % 2U == 0U ? MaterialDescriptorCreationCode::NullParameterSet
                                            : MaterialDescriptorCreationCode::NullSampledImageSet,
                       tuple_index);
    }

    for (std::size_t tuple_index = 0; tuple_index < resources.size(); ++tuple_index)
    {
        MaterialDescriptorBinding& binding = MaterialDescriptorFactory::binding(*generation, tuple_index);
        binding.mSets.mParameters          = descriptor_sets[tuple_index * 2U];
        binding.mSets.mSampledImages       = descriptor_sets[tuple_index * 2U + 1U];

        VkWriteDescriptorSet& uniform_write = writes[tuple_index * 4U];
        uniform_write.dstSet                = binding.mSets.mParameters;
        for (std::uint32_t image_index = 0; image_index < 3U; ++image_index)
        {
            writes[tuple_index * 4U + image_index + 1U].dstSet = binding.mSets.mSampledImages;
        }
    }

    device.mDispatch.mUpdateDescriptorSets(device.mDevice, write_count, writes.data(), 0, nullptr);
    return generation;
}

} // namespace LLRenderVulkanMaterial
