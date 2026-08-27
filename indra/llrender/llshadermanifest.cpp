/**
 * @file llshadermanifest.cpp
 * @brief Canonical legacy material shader recipes and validation.
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

#include "llshadermanifest.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <string_view>

namespace LLRenderContract
{
namespace
{

    constexpr ShaderStageVisibility VERTEX_STAGE{ true, false };
    constexpr ShaderStageVisibility FRAGMENT_STAGE{ false, true };
    constexpr ShaderStageVisibility BOTH_STAGES{ true, true };

    bool valid(ShaderBackend value) noexcept
    {
        switch (value)
        {
            case ShaderBackend::OpenGL:
            case ShaderBackend::Vulkan:
                return true;
        }
        return false;
    }

    bool valid(ShaderStage value) noexcept
    {
        switch (value)
        {
            case ShaderStage::Vertex:
            case ShaderStage::Fragment:
                return true;
        }
        return false;
    }

    bool valid(ShaderSourceRole value) noexcept
    {
        switch (value)
        {
            case ShaderSourceRole::Primary:
            case ShaderSourceRole::Feature:
            case ShaderSourceRole::Wrapper:
            case ShaderSourceRole::SharedInclude:
                return true;
        }
        return false;
    }

    bool valid(ShaderValueType value) noexcept
    {
        switch (value)
        {
            case ShaderValueType::Float:
            case ShaderValueType::Float2:
            case ShaderValueType::Float3:
            case ShaderValueType::Float4:
                return true;
        }
        return false;
    }

    bool valid(ShaderInterpolation value) noexcept
    {
        switch (value)
        {
            case ShaderInterpolation::Smooth:
            case ShaderInterpolation::Flat:
                return true;
        }
        return false;
    }

    bool valid(SampledImageRole value) noexcept
    {
        switch (value)
        {
            case SampledImageRole::Diffuse:
            case SampledImageRole::Normal:
            case SampledImageRole::Specular:
                return true;
        }
        return false;
    }

    bool valid(ShaderImageDimension value) noexcept
    {
        return value == ShaderImageDimension::TwoD;
    }

    bool valid(ShaderParameterOrigin value) noexcept
    {
        switch (value)
        {
            case ShaderParameterOrigin::CopiedDraw:
            case ShaderParameterOrigin::DrawAndFrameDerived:
            case ShaderParameterOrigin::FrameOrPass:
            case ShaderParameterOrigin::FixedDefault:
                return true;
        }
        return false;
    }

    bool valid(ShaderFragmentOutputRole value) noexcept
    {
        switch (value)
        {
            case ShaderFragmentOutputRole::DiffuseEmissive:
            case ShaderFragmentOutputRole::SpecularGloss:
            case ShaderFragmentOutputRole::NormalEnvironment:
            case ShaderFragmentOutputRole::EmissiveBuffer:
                return true;
        }
        return false;
    }

    bool valid(VertexSemantic value) noexcept
    {
        switch (value)
        {
            case VertexSemantic::Position:
            case VertexSemantic::Normal:
            case VertexSemantic::TexCoord0:
            case VertexSemantic::Color:
            case VertexSemantic::Tangent:
            case VertexSemantic::TexCoord1:
            case VertexSemantic::TexCoord2:
                return true;
        }
        return false;
    }

    bool valid(VertexFormat value) noexcept
    {
        switch (value)
        {
            case VertexFormat::Float2:
            case VertexFormat::Float3:
            case VertexFormat::Float4:
            case VertexFormat::UNorm8x4:
                return true;
        }
        return false;
    }

    bool active(ShaderStageVisibility visibility) noexcept
    {
        return visibility.mVertex || visibility.mFragment;
    }

    bool overlaps(ShaderStageVisibility left, ShaderStageVisibility right) noexcept
    {
        return (left.mVertex && right.mVertex) || (left.mFragment && right.mFragment);
    }

    std::uint32_t byteSize(VertexFormat format) noexcept
    {
        switch (format)
        {
            case VertexFormat::Float2:
                return 8;
            case VertexFormat::Float3:
                return 12;
            case VertexFormat::Float4:
                return 16;
            case VertexFormat::UNorm8x4:
                return 4;
        }
        return 0;
    }

    bool sameProgram(const ShaderProgramKey& left, const ShaderProgramKey& right) noexcept
    {
        return left.mName == right.mName && left.mVariant == right.mVariant;
    }

    bool validDefines(const std::vector<ShaderDefine>& defines)
    {
        std::set<std::string> vertex_names;
        std::set<std::string> fragment_names;
        for (const ShaderDefine& define : defines)
        {
            if (define.mName.empty() || define.mValue.empty() || !active(define.mVisibility) ||
                (define.mVisibility.mVertex && !vertex_names.insert(define.mName).second) ||
                (define.mVisibility.mFragment && !fragment_names.insert(define.mName).second))
            {
                return false;
            }
        }
        return true;
    }

    bool validVertexInputs(const std::vector<ShaderVertexInput>& inputs)
    {
        std::set<std::string>    names;
        std::set<VertexSemantic> semantics;
        std::set<std::uint32_t>  locations;
        for (const ShaderVertexInput& input : inputs)
        {
            if (input.mName.empty() || !valid(input.mSemantic) || !valid(input.mFormat) || input.mStride < byteSize(input.mFormat) ||
                !names.insert(input.mName).second || !semantics.insert(input.mSemantic).second || !locations.insert(input.mLocation).second)
            {
                return false;
            }
        }
        return !inputs.empty();
    }

    bool validInterstage(const std::vector<ShaderInterstageVariable>& variables)
    {
        std::set<std::string>   names;
        std::set<std::uint32_t> locations;
        for (const ShaderInterstageVariable& variable : variables)
        {
            if (variable.mName.empty() || !valid(variable.mType) || !valid(variable.mInterpolation) ||
                !names.insert(variable.mName).second || !locations.insert(variable.mLocation).second)
            {
                return false;
            }
        }
        return true;
    }

    bool validSampledImages(const std::vector<ShaderSampledImage>& images)
    {
        std::set<std::string>                             names;
        std::set<SampledImageRole>                        roles;
        std::set<std::pair<std::uint32_t, std::uint32_t>> bindings;
        for (const ShaderSampledImage& image : images)
        {
            if (image.mName.empty() || !valid(image.mRole) || !valid(image.mDimension) || !active(image.mVisibility) ||
                !names.insert(image.mName).second || !roles.insert(image.mRole).second ||
                !bindings.emplace(image.mSet, image.mBinding).second)
            {
                return false;
            }
        }
        return !images.empty();
    }

    bool validParameters(const std::vector<ShaderLogicalParameter>& parameters)
    {
        std::set<std::string> names;
        std::set<std::string> shader_names;
        std::uint32_t         next_offset = 0;
        for (const ShaderLogicalParameter& parameter : parameters)
        {
            const bool fixed = parameter.mOrigin == ShaderParameterOrigin::FixedDefault;
            if (parameter.mName.empty() || parameter.mShaderName.empty() || parameter.mWordCount == 0 ||
                parameter.mWordOffset != next_offset || !valid(parameter.mOrigin) || !active(parameter.mVisibility) ||
                !names.insert(parameter.mName).second || !shader_names.insert(parameter.mShaderName).second ||
                fixed != parameter.mFixedValue.has_value() || (parameter.mFixedValue && !std::isfinite(*parameter.mFixedValue)) ||
                parameter.mWordOffset > std::numeric_limits<std::uint32_t>::max() - parameter.mWordCount)
            {
                return false;
            }
            next_offset += parameter.mWordCount;
        }
        return next_offset != 0;
    }

    bool validOutputs(const ShaderManifest& manifest)
    {
        constexpr std::uint32_t            MAX_INTERFACE_LOCATIONS = 32;
        std::set<ShaderFragmentOutputRole> roles;
        std::set<std::uint32_t>            logical_locations;
        for (const ShaderLogicalFragmentOutput& output : manifest.mLogicalFragmentOutputs)
        {
            if (!valid(output.mRole) || !valid(output.mType) || !roles.insert(output.mRole).second ||
                !logical_locations.insert(output.mLocation).second)
            {
                return false;
            }
        }
        if (manifest.mLogicalFragmentOutputs.empty())
        {
            return false;
        }

        std::set<std::string>   names;
        std::set<std::uint32_t> declared_locations;
        std::uint32_t           logical_declarations = 0;
        for (const ShaderFragmentOutputDeclaration& declaration : manifest.mFragmentOutputDeclarations)
        {
            if (declaration.mName.empty() || declaration.mElementCount == 0 || declaration.mElementCount > MAX_INTERFACE_LOCATIONS ||
                !valid(declaration.mElementType) || declaration.mLogicalElementCount > declaration.mElementCount ||
                declaration.mFirstLocation > std::numeric_limits<std::uint32_t>::max() - declaration.mElementCount ||
                !names.insert(declaration.mName).second ||
                (declaration.mElementCount != declaration.mLogicalElementCount) != declaration.mExtraElementsInert)
            {
                return false;
            }
            for (std::uint32_t index = 0; index < declaration.mElementCount; ++index)
            {
                if (!declared_locations.insert(declaration.mFirstLocation + index).second)
                {
                    return false;
                }
            }
            logical_declarations += declaration.mLogicalElementCount;
        }
        if (logical_declarations != manifest.mLogicalFragmentOutputs.size())
        {
            return false;
        }
        return std::all_of(manifest.mLogicalFragmentOutputs.begin(), manifest.mLogicalFragmentOutputs.end(),
                           [&manifest](const ShaderLogicalFragmentOutput& output)
                           {
                               return std::any_of(manifest.mFragmentOutputDeclarations.begin(), manifest.mFragmentOutputDeclarations.end(),
                                                  [&output](const ShaderFragmentOutputDeclaration& declaration)
                                                  {
                                                      return output.mType == declaration.mElementType &&
                                                             output.mLocation >= declaration.mFirstLocation &&
                                                             output.mLocation <
                                                                 declaration.mFirstLocation + declaration.mLogicalElementCount;
                                                  });
                           });
    }

    bool validBaggage(const std::vector<ShaderLinkedBlockBaggage>& blocks)
    {
        std::set<std::string>   block_names;
        std::set<std::uint32_t> bindings;
        for (const ShaderLinkedBlockBaggage& block : blocks)
        {
            std::set<std::string> members;
            if (block.mName.empty() || block.mByteSize == 0 || !active(block.mVisibility) || !block_names.insert(block.mName).second ||
                !bindings.insert(block.mBinding).second || block.mActiveMembers.empty() ||
                std::any_of(block.mActiveMembers.begin(), block.mActiveMembers.end(),
                            [&members](const std::string& member) { return member.empty() || !members.insert(member).second; }))
            {
                return false;
            }
        }
        return true;
    }

    bool validPushConstants(const std::vector<ShaderPushConstantRange>& ranges) noexcept
    {
        std::uint32_t last_end = 0;
        for (const ShaderPushConstantRange& range : ranges)
        {
            if (range.mByteSize == 0 || !active(range.mVisibility) || range.mByteOffset < last_end ||
                range.mByteOffset > std::numeric_limits<std::uint32_t>::max() - range.mByteSize)
            {
                return false;
            }
            last_end = range.mByteOffset + range.mByteSize;
        }
        return true;
    }

    ShaderManifest commonManifest(ShaderBackend backend)
    {
        ShaderManifest manifest;
        manifest.mProgram           = legacyNormSpecDiagnosticProgramKey();
        manifest.mBackend           = backend;
        manifest.mEntryPoints       = { { ShaderStage::Vertex, "main" }, { ShaderStage::Fragment, "main" } };
        manifest.mSampledImages     = { { "diffuseMap", SampledImageRole::Diffuse, ShaderImageDimension::TwoD, 0, 0, FRAGMENT_STAGE },
                                        { "bumpMap", SampledImageRole::Normal, ShaderImageDimension::TwoD, 0, 1, FRAGMENT_STAGE },
                                        { "specularMap", SampledImageRole::Specular, ShaderImageDimension::TwoD, 0, 2, FRAGMENT_STAGE } };
        manifest.mLogicalParameters = {
            { "modelview", "modelview_matrix", 0, 16, ShaderParameterOrigin::DrawAndFrameDerived, VERTEX_STAGE, std::nullopt },
            { "modelview_projection", "modelview_projection_matrix", 16, 16, ShaderParameterOrigin::DrawAndFrameDerived, VERTEX_STAGE,
              std::nullopt },
            { "normal", "normal_matrix", 32, 9, ShaderParameterOrigin::DrawAndFrameDerived, VERTEX_STAGE, std::nullopt },
            { "diffuse_texture", "texture_matrix0", 41, 16, ShaderParameterOrigin::CopiedDraw, VERTEX_STAGE, std::nullopt },
            { "specular", "specular_color", 57, 4, ShaderParameterOrigin::CopiedDraw, FRAGMENT_STAGE, std::nullopt },
            { "clip", "clipPlane", 61, 4, ShaderParameterOrigin::FrameOrPass, FRAGMENT_STAGE, std::nullopt },
            { "environment", "env_intensity", 65, 1, ShaderParameterOrigin::CopiedDraw, FRAGMENT_STAGE, std::nullopt },
            { "emissive", "emissive_brightness", 66, 1, ShaderParameterOrigin::CopiedDraw, FRAGMENT_STAGE, std::nullopt },
            { "mirror", "mirror_flag", 67, 1, ShaderParameterOrigin::FixedDefault, FRAGMENT_STAGE, 0.f }
        };
        manifest.mLogicalFragmentOutputs = { { ShaderFragmentOutputRole::DiffuseEmissive, 0, ShaderValueType::Float4 },
                                             { ShaderFragmentOutputRole::SpecularGloss, 1, ShaderValueType::Float4 },
                                             { ShaderFragmentOutputRole::NormalEnvironment, 2, ShaderValueType::Float4 } };
        return manifest;
    }

    ShaderManifest openGLManifest()
    {
        ShaderManifest manifest              = commonManifest(ShaderBackend::OpenGL);
        manifest.mSourceUnits                = { { ShaderStage::Vertex, "deferred/materialV.glsl", ShaderSourceRole::Primary, 1 },
                                                 { ShaderStage::Fragment, "deferred/materialF.glsl", ShaderSourceRole::Primary, 3 },
                                                 { ShaderStage::Vertex, "windlight/atmosphericsVarsV.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Vertex, "windlight/atmosphericsHelpersV.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Vertex, "environment/srgbF.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Vertex, "windlight/atmosphericsFuncs.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Vertex, "windlight/atmosphericsV.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Vertex, "deferred/textureUtilV.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Fragment, "deferred/globalF.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Fragment, "environment/srgbF.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Fragment, "windlight/atmosphericsVarsF.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Fragment, "windlight/atmosphericsHelpersF.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Fragment, "deferred/deferredUtil.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Fragment, "deferred/screenSpaceReflUtil.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Fragment, "deferred/reflectionProbeF.glsl", ShaderSourceRole::Feature, 3 },
                                                 { ShaderStage::Fragment, "windlight/gammaF.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Fragment, "windlight/atmosphericsFuncs.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Fragment, "windlight/atmosphericsF.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Fragment, "environment/waterFogF.glsl", ShaderSourceRole::Feature, 1 },
                                                 { ShaderStage::Vertex, "objects/nonindexedTextureV.glsl", ShaderSourceRole::Feature, 1 } };
        manifest.mDefines                    = { { "DIFFUSE_ALPHA_MODE", "0", BOTH_STAGES },
                                                 { "HAS_NORMAL_MAP", "1", BOTH_STAGES },
                                                 { "HAS_SPECULAR_MAP", "1", BOTH_STAGES } };
        manifest.mVertexInputs               = { { "position", VertexSemantic::Position, VertexFormat::Float3, 0, 0, 16 },
                                                 { "normal", VertexSemantic::Normal, VertexFormat::Float3, 1, 0, 16 },
                                                 { "texcoord0", VertexSemantic::TexCoord0, VertexFormat::Float2, 2, 0, 8 },
                                                 { "diffuse_color", VertexSemantic::Color, VertexFormat::UNorm8x4, 6, 0, 4 },
                                                 { "tangent", VertexSemantic::Tangent, VertexFormat::Float4, 8, 0, 16 },
                                                 { "texcoord1", VertexSemantic::TexCoord1, VertexFormat::Float2, 3, 0, 8 },
                                                 { "texcoord2", VertexSemantic::TexCoord2, VertexFormat::Float2, 4, 0, 8 } };
        manifest.mFragmentOutputDeclarations = { { "frag_data", 0, 4, ShaderValueType::Float4, 3, true } };
        manifest.mLinkedBlockBaggage         = { { "ReflectionProbes",
                                                   0,
                                                   49248,
                                                   FRAGMENT_STAGE,
                                                   { "refBox", "heroBox", "refSphere", "refParams", "heroSphere", "refIndex", "refNeighbor",
                                                     "refBucket", "refmapCount", "heroShape", "heroMipCount", "heroProbeCount" } } };
        return manifest;
    }

    ShaderManifest vulkanManifest()
    {
        ShaderManifest manifest = commonManifest(ShaderBackend::Vulkan);
        manifest.mSourceUnits   = {
            { ShaderStage::Vertex, "indra/llrender/vulkan/shaders/material.vert.glsl", ShaderSourceRole::Wrapper, std::nullopt },
            { ShaderStage::Vertex, "indra/newview/app_settings/shaders/class1/deferred/materialV.glsl", ShaderSourceRole::SharedInclude,
                1 },
            { ShaderStage::Fragment, "indra/llrender/vulkan/shaders/material.frag.glsl", ShaderSourceRole::Wrapper, std::nullopt },
            { ShaderStage::Fragment, "indra/newview/app_settings/shaders/class1/deferred/globalF.glsl", ShaderSourceRole::SharedInclude,
                1 },
            { ShaderStage::Fragment, "indra/newview/app_settings/shaders/class3/deferred/materialF.glsl", ShaderSourceRole::SharedInclude,
                3 }
        };
        manifest.mDefines             = { { "LL_VULKAN_SHADER", "1", VERTEX_STAGE },
                                          { "DIFFUSE_ALPHA_MODE", "0", VERTEX_STAGE },
                                          { "HAS_NORMAL_MAP", "1", VERTEX_STAGE },
                                          { "HAS_SPECULAR_MAP", "1", VERTEX_STAGE },
                                          { "LL_VULKAN_SHADER", "1", FRAGMENT_STAGE },
                                          { "DIFFUSE_ALPHA_MODE", "0", FRAGMENT_STAGE },
                                          { "HAS_NORMAL_MAP", "1", FRAGMENT_STAGE },
                                          { "HAS_SPECULAR_MAP", "1", FRAGMENT_STAGE },
                                          { "GBUFFER_FLAG_HAS_ATMOS", "0.34", FRAGMENT_STAGE } };
        manifest.mVertexInputs        = { { "position", VertexSemantic::Position, VertexFormat::Float3, 0, 0, 16 },
                                          { "normal", VertexSemantic::Normal, VertexFormat::Float3, 1, 1, 16 },
                                          { "texcoord0", VertexSemantic::TexCoord0, VertexFormat::Float2, 2, 2, 8 },
                                          { "diffuse_color", VertexSemantic::Color, VertexFormat::UNorm8x4, 3, 3, 4 },
                                          { "tangent", VertexSemantic::Tangent, VertexFormat::Float4, 4, 4, 16 },
                                          { "texcoord1", VertexSemantic::TexCoord1, VertexFormat::Float2, 5, 5, 8 },
                                          { "texcoord2", VertexSemantic::TexCoord2, VertexFormat::Float2, 6, 6, 8 } };
        manifest.mInterstageVariables = { { "vary_position", 0, ShaderValueType::Float3, ShaderInterpolation::Smooth },
                                          { "vary_tangent", 1, ShaderValueType::Float3, ShaderInterpolation::Smooth },
                                          { "vary_sign", 2, ShaderValueType::Float, ShaderInterpolation::Flat },
                                          { "vary_normal", 3, ShaderValueType::Float3, ShaderInterpolation::Smooth },
                                          { "vary_texcoord1", 4, ShaderValueType::Float2, ShaderInterpolation::Smooth },
                                          { "vary_texcoord2", 5, ShaderValueType::Float2, ShaderInterpolation::Smooth },
                                          { "vertex_color", 6, ShaderValueType::Float4, ShaderInterpolation::Smooth },
                                          { "vary_texcoord0", 7, ShaderValueType::Float2, ShaderInterpolation::Smooth } };
        for (ShaderSampledImage& image : manifest.mSampledImages)
        {
            image.mSet = 1;
        }
        manifest.mParameterBlock             = ShaderParameterBlock{ "MaterialParameterPacket", 0, 0, 272, BOTH_STAGES };
        manifest.mFragmentOutputDeclarations = { { "frag_data", 0, 3, ShaderValueType::Float4, 3, false } };
        return manifest;
    }

    ShaderManifest productionVulkanManifest()
    {
        ShaderManifest manifest = vulkanManifest();
        manifest.mProgram       = legacyNormSpecProductionProgramKey();
        manifest.mDefines       = { { "LL_VULKAN_MATERIAL_PRODUCTION", "1", VERTEX_STAGE },
                                    { "LL_VULKAN_SHADER", "1", VERTEX_STAGE },
                                    { "DIFFUSE_ALPHA_MODE", "0", VERTEX_STAGE },
                                    { "HAS_NORMAL_MAP", "1", VERTEX_STAGE },
                                    { "HAS_SPECULAR_MAP", "1", VERTEX_STAGE },
                                    { "HAS_EMISSIVE", "1", VERTEX_STAGE },
                                    { "HAS_SUN_SHADOW", "1", VERTEX_STAGE },
                                    { "SUN_SHADOW", "1", VERTEX_STAGE },
                                    { "SPOT_SHADOW", "1", VERTEX_STAGE },
                                    { "LL_VULKAN_MATERIAL_PRODUCTION", "1", FRAGMENT_STAGE },
                                    { "LL_VULKAN_SHADER", "1", FRAGMENT_STAGE },
                                    { "DIFFUSE_ALPHA_MODE", "0", FRAGMENT_STAGE },
                                    { "HAS_NORMAL_MAP", "1", FRAGMENT_STAGE },
                                    { "HAS_SPECULAR_MAP", "1", FRAGMENT_STAGE },
                                    { "GBUFFER_FLAG_HAS_ATMOS", "0.34", FRAGMENT_STAGE },
                                    { "HAS_EMISSIVE", "1", FRAGMENT_STAGE },
                                    { "HAS_SUN_SHADOW", "1", FRAGMENT_STAGE },
                                    { "SUN_SHADOW", "1", FRAGMENT_STAGE },
                                    { "SPOT_SHADOW", "1", FRAGMENT_STAGE } };
        manifest.mLogicalFragmentOutputs.push_back({ ShaderFragmentOutputRole::EmissiveBuffer, 3, ShaderValueType::Float4 });
        manifest.mFragmentOutputDeclarations = { { "frag_data", 0, 4, ShaderValueType::Float4, 4, false } };
        return manifest;
    }

    ShaderManifest canonicalManifest(ShaderBackend backend)
    {
        switch (backend)
        {
            case ShaderBackend::OpenGL:
                return openGLManifest();
            case ShaderBackend::Vulkan:
                return vulkanManifest();
        }
        ShaderManifest manifest;
        manifest.mBackend = backend;
        return manifest;
    }

} // namespace

bool operator==(const ShaderManifest& left, const ShaderManifest& right)
{
    return sameProgram(left.mProgram, right.mProgram) && left.mBackend == right.mBackend && left.mEntryPoints == right.mEntryPoints &&
           left.mSourceUnits == right.mSourceUnits && left.mDefines == right.mDefines && left.mVertexInputs == right.mVertexInputs &&
           left.mInterstageVariables == right.mInterstageVariables && left.mSampledImages == right.mSampledImages &&
           left.mLogicalParameters == right.mLogicalParameters && left.mParameterBlock == right.mParameterBlock &&
           left.mLogicalFragmentOutputs == right.mLogicalFragmentOutputs &&
           left.mFragmentOutputDeclarations == right.mFragmentOutputDeclarations && left.mLinkedBlockBaggage == right.mLinkedBlockBaggage &&
           left.mPushConstantRanges == right.mPushConstantRanges;
}

// Structural validation uses temporary sets. Preserve the public fail-closed
// noexcept contract if those allocations or canonical construction fail.
bool validShaderManifest(const ShaderManifest& manifest) noexcept
try
{
    if (manifest.mProgram.mName.empty() || !valid(manifest.mBackend) || manifest.mEntryPoints.size() != 2 ||
        manifest.mEntryPoints[0].mStage != ShaderStage::Vertex || manifest.mEntryPoints[1].mStage != ShaderStage::Fragment ||
        manifest.mEntryPoints[0].mName.empty() || manifest.mEntryPoints[1].mName.empty() || manifest.mSourceUnits.empty() ||
        !validDefines(manifest.mDefines) || !validVertexInputs(manifest.mVertexInputs) || !validInterstage(manifest.mInterstageVariables) ||
        !validSampledImages(manifest.mSampledImages) || !validParameters(manifest.mLogicalParameters) || !validOutputs(manifest) ||
        !validBaggage(manifest.mLinkedBlockBaggage) || !validPushConstants(manifest.mPushConstantRanges))
    {
        return false;
    }

    std::set<std::pair<ShaderStage, std::string>> sources;
    for (const ShaderSourceUnit& source : manifest.mSourceUnits)
    {
        if (!valid(source.mStage) || !valid(source.mRole) || source.mPath.empty() || (source.mShaderClass && *source.mShaderClass == 0))
        {
            return false;
        }
        if (!sources.emplace(source.mStage, source.mPath).second)
        {
            return false;
        }
    }
    for (std::size_t left = 0; left < manifest.mDefines.size(); ++left)
    {
        for (std::size_t right = left + 1; right < manifest.mDefines.size(); ++right)
        {
            if (manifest.mDefines[left].mName == manifest.mDefines[right].mName &&
                overlaps(manifest.mDefines[left].mVisibility, manifest.mDefines[right].mVisibility))
            {
                return false;
            }
        }
    }
    if (manifest.mParameterBlock && (manifest.mParameterBlock->mName.empty() || manifest.mParameterBlock->mByteSize == 0 ||
                                     !active(manifest.mParameterBlock->mVisibility)))
    {
        return false;
    }
    if (manifest.mParameterBlock &&
        std::any_of(manifest.mSampledImages.begin(), manifest.mSampledImages.end(), [&manifest](const ShaderSampledImage& image)
                    { return image.mSet == manifest.mParameterBlock->mSet && image.mBinding == manifest.mParameterBlock->mBinding; }))
    {
        return false;
    }
    return true;
}
catch (...)
{
    return false;
}

bool validLegacyNormSpecDiagnosticShaderManifest(const ShaderManifest& manifest) noexcept
try
{
    return validShaderManifest(manifest) && manifest == canonicalManifest(manifest.mBackend);
}
catch (...)
{
    return false;
}

bool validLegacyNormSpecProductionShaderManifest(const ShaderManifest& manifest) noexcept
try
{
    return validShaderManifest(manifest) && manifest == productionVulkanManifest();
}
catch (...)
{
    return false;
}

ShaderManifest legacyNormSpecDiagnosticShaderManifest(ShaderBackend backend)
{
    return canonicalManifest(backend);
}

std::optional<ShaderManifest> legacyNormSpecShaderManifest(const LegacyNormSpecPipelineKey& pipeline_key, ShaderBackend backend)
{
    if (!valid(backend) || !validLegacyNormSpecPipelineKey(pipeline_key))
    {
        return std::nullopt;
    }
    if (pipeline_key == legacyNormSpecDiagnosticPipelineKey())
    {
        return canonicalManifest(backend);
    }
    if (backend == ShaderBackend::Vulkan &&
        (pipeline_key == legacyNormSpecModernHDRPipelineKey() || pipeline_key == legacyNormSpecCompatibilityPipelineKey()))
    {
        return productionVulkanManifest();
    }
    return std::nullopt;
}

} // namespace LLRenderContract
