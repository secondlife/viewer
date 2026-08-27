/**
 * @file llrendervulkanmateriallayout.cpp
 * @brief Transactional Vulkan layouts for the canonical material interface.
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

#include "llrendervulkanmateriallayout.h"

#include <new>

namespace LLRenderVulkanMaterial
{
namespace
{

    MaterialLayoutCreationError failure(MaterialLayoutCreationCode          code,
                                        std::optional<MaterialLayoutObject> object = std::nullopt,
                                        VkResult                            result = VK_SUCCESS) noexcept
    {
        return { code, object, result };
    }

    VkDescriptorSetLayoutCreateInfo parameterSetCreateInfo(VkDescriptorSetLayoutBinding& binding) noexcept
    {
        binding.binding            = 0;
        binding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount    = 1;
        binding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        binding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings    = &binding;
        return info;
    }

    VkDescriptorSetLayoutCreateInfo sampledImageSetCreateInfo(std::array<VkDescriptorSetLayoutBinding, 3>& bindings) noexcept
    {
        for (std::uint32_t binding_index = 0; binding_index < bindings.size(); ++binding_index)
        {
            VkDescriptorSetLayoutBinding& binding = bindings[binding_index];
            binding.binding                       = binding_index;
            binding.descriptorType                = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount               = 1;
            binding.stageFlags                    = VK_SHADER_STAGE_FRAGMENT_BIT;
            binding.pImmutableSamplers            = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = static_cast<std::uint32_t>(bindings.size());
        info.pBindings    = bindings.data();
        return info;
    }

    VkPipelineLayoutCreateInfo pipelineCreateInfo(const std::array<VkDescriptorSetLayout, 2>& layouts) noexcept
    {
        VkPipelineLayoutCreateInfo info{};
        info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount = static_cast<std::uint32_t>(layouts.size());
        info.pSetLayouts    = layouts.data();
        return info;
    }

} // namespace

struct MaterialLayoutFactory
{
    static std::unique_ptr<LegacyNormSpecPipelineLayout> allocate(const MaterialLayoutDevice& device) noexcept
    {
        return std::unique_ptr<LegacyNormSpecPipelineLayout>(new (std::nothrow) LegacyNormSpecPipelineLayout(device));
    }

    static VkDescriptorSetLayout& parameterSet(LegacyNormSpecPipelineLayout& layout) noexcept { return layout.mDescriptorSetLayouts[0]; }

    static VkDescriptorSetLayout& sampledImageSet(LegacyNormSpecPipelineLayout& layout) noexcept { return layout.mDescriptorSetLayouts[1]; }

    static VkPipelineLayout& pipeline(LegacyNormSpecPipelineLayout& layout) noexcept { return layout.mPipelineLayout; }
};

LegacyNormSpecPipelineLayout::LegacyNormSpecPipelineLayout(const MaterialLayoutDevice& device) noexcept :
    mDevice(device.mDevice),
    mDestroyDescriptorSetLayout(device.mDispatch.mDestroyDescriptorSetLayout),
    mDestroyPipelineLayout(device.mDispatch.mDestroyPipelineLayout)
{
}

LegacyNormSpecPipelineLayout::~LegacyNormSpecPipelineLayout() noexcept
{
    if (mPipelineLayout != VK_NULL_HANDLE)
    {
        mDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
    }
    if (mDescriptorSetLayouts[1] != VK_NULL_HANDLE)
    {
        mDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayouts[1], nullptr);
    }
    if (mDescriptorSetLayouts[0] != VK_NULL_HANDLE)
    {
        mDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayouts[0], nullptr);
    }
}

MaterialLayoutCreationResult createLegacyNormSpecPipelineLayout(const MaterialLayoutDevice& device) noexcept
{
    if (device.mDevice == VK_NULL_HANDLE)
    {
        return failure(MaterialLayoutCreationCode::InvalidDevice);
    }
    if (!device.mDispatch.mCreateDescriptorSetLayout || !device.mDispatch.mDestroyDescriptorSetLayout ||
        !device.mDispatch.mCreatePipelineLayout || !device.mDispatch.mDestroyPipelineLayout)
    {
        return failure(MaterialLayoutCreationCode::InvalidDispatch);
    }

    VkDescriptorSetLayoutBinding                parameter_binding{};
    std::array<VkDescriptorSetLayoutBinding, 3> sampled_bindings{};
    const VkDescriptorSetLayoutCreateInfo       parameter_info = parameterSetCreateInfo(parameter_binding);
    const VkDescriptorSetLayoutCreateInfo       sampled_info   = sampledImageSetCreateInfo(sampled_bindings);

    std::unique_ptr<LegacyNormSpecPipelineLayout> layout = MaterialLayoutFactory::allocate(device);
    if (!layout)
    {
        return failure(MaterialLayoutCreationCode::OwnerAllocationFailure);
    }

    VkDescriptorSetLayout parameter_layout = VK_NULL_HANDLE;
    const VkResult        parameter_result =
        device.mDispatch.mCreateDescriptorSetLayout(device.mDevice, &parameter_info, nullptr, &parameter_layout);
    if (parameter_result != VK_SUCCESS)
    {
        return failure(MaterialLayoutCreationCode::CreateFailure, MaterialLayoutObject::ParameterSetLayout, parameter_result);
    }
    if (parameter_layout == VK_NULL_HANDLE)
    {
        return failure(MaterialLayoutCreationCode::NullHandle, MaterialLayoutObject::ParameterSetLayout);
    }
    MaterialLayoutFactory::parameterSet(*layout) = parameter_layout;

    VkDescriptorSetLayout sampled_layout = VK_NULL_HANDLE;
    const VkResult sampled_result = device.mDispatch.mCreateDescriptorSetLayout(device.mDevice, &sampled_info, nullptr, &sampled_layout);
    if (sampled_result != VK_SUCCESS)
    {
        return failure(MaterialLayoutCreationCode::CreateFailure, MaterialLayoutObject::SampledImageSetLayout, sampled_result);
    }
    if (sampled_layout == VK_NULL_HANDLE)
    {
        return failure(MaterialLayoutCreationCode::NullHandle, MaterialLayoutObject::SampledImageSetLayout);
    }
    MaterialLayoutFactory::sampledImageSet(*layout) = sampled_layout;

    const std::array<VkDescriptorSetLayout, 2> set_layouts     = layout->descriptorSetLayouts();
    const VkPipelineLayoutCreateInfo           pipeline_info   = pipelineCreateInfo(set_layouts);
    VkPipelineLayout                           pipeline_layout = VK_NULL_HANDLE;
    const VkResult pipeline_result = device.mDispatch.mCreatePipelineLayout(device.mDevice, &pipeline_info, nullptr, &pipeline_layout);
    if (pipeline_result != VK_SUCCESS)
    {
        return failure(MaterialLayoutCreationCode::CreateFailure, MaterialLayoutObject::PipelineLayout, pipeline_result);
    }
    if (pipeline_layout == VK_NULL_HANDLE)
    {
        return failure(MaterialLayoutCreationCode::NullHandle, MaterialLayoutObject::PipelineLayout);
    }
    MaterialLayoutFactory::pipeline(*layout) = pipeline_layout;

    return layout;
}

} // namespace LLRenderVulkanMaterial
