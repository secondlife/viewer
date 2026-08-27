/**
 * @file llshadermanifest_dump.cpp
 * @brief Emit the manifest-derived Vulkan material reflection expectation.
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

#include <cstdio>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace
{
using namespace LLRenderContract;

enum class MaterialProfile
{
    Diagnostic,
    Production
};

std::optional<MaterialProfile> materialProfile(std::string_view name)
{
    if (name == "diagnostic")
    {
        return MaterialProfile::Diagnostic;
    }
    if (name == "production")
    {
        return MaterialProfile::Production;
    }
    return std::nullopt;
}

LegacyNormSpecPipelineKey pipelineKey(MaterialProfile profile)
{
    switch (profile)
    {
        case MaterialProfile::Diagnostic:
            return legacyNormSpecDiagnosticPipelineKey();
        case MaterialProfile::Production:
            return legacyNormSpecModernHDRPipelineKey();
    }
    throw std::invalid_argument("unknown material profile");
}

bool validProfileManifest(MaterialProfile profile, const ShaderManifest& manifest) noexcept
{
    switch (profile)
    {
        case MaterialProfile::Diagnostic:
            return validLegacyNormSpecDiagnosticShaderManifest(manifest);
        case MaterialProfile::Production:
            return validLegacyNormSpecProductionShaderManifest(manifest);
    }
    return false;
}

void writeJsonString(std::string_view value)
{
    constexpr char HEX[] = "0123456789abcdef";

    std::cout.put('"');
    for (const unsigned char character : value)
    {
        switch (character)
        {
            case '"':
                std::cout << "\\\"";
                break;
            case '\\':
                std::cout << "\\\\";
                break;
            case '\b':
                std::cout << "\\b";
                break;
            case '\f':
                std::cout << "\\f";
                break;
            case '\n':
                std::cout << "\\n";
                break;
            case '\r':
                std::cout << "\\r";
                break;
            case '\t':
                std::cout << "\\t";
                break;
            default:
                if (character < 0x20)
                {
                    std::cout << "\\u00" << HEX[character >> 4] << HEX[character & 0x0f];
                }
                else
                {
                    std::cout.put(static_cast<char>(character));
                }
        }
    }
    std::cout.put('"');
}

std::string_view moduleName(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::Vertex:
            return "vertex";
        case ShaderStage::Fragment:
            return "fragment";
    }
    throw std::invalid_argument("unknown shader stage");
}

std::string_view reflectionStage(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::Vertex:
            return "vert";
        case ShaderStage::Fragment:
            return "frag";
    }
    throw std::invalid_argument("unknown shader stage");
}

std::string_view valueType(ShaderValueType type)
{
    switch (type)
    {
        case ShaderValueType::Float:
            return "float";
        case ShaderValueType::Float2:
            return "vec2";
        case ShaderValueType::Float3:
            return "vec3";
        case ShaderValueType::Float4:
            return "vec4";
    }
    throw std::invalid_argument("unknown shader value type");
}

std::string_view vertexValueType(VertexFormat format)
{
    switch (format)
    {
        case VertexFormat::Float2:
            return "vec2";
        case VertexFormat::Float3:
            return "vec3";
        case VertexFormat::Float4:
        case VertexFormat::UNorm8x4:
            return "vec4";
    }
    throw std::invalid_argument("unknown vertex format");
}

std::string_view sampledImageType(ShaderImageDimension dimension)
{
    switch (dimension)
    {
        case ShaderImageDimension::TwoD:
            return "sampler2D";
    }
    throw std::invalid_argument("unknown sampled-image dimension");
}

void writeStages(ShaderStageVisibility visibility)
{
    std::cout.put('[');
    bool needs_comma = false;
    if (visibility.mVertex)
    {
        writeJsonString("vertex");
        needs_comma = true;
    }
    if (visibility.mFragment)
    {
        if (needs_comma)
        {
            std::cout.put(',');
        }
        writeJsonString("fragment");
    }
    std::cout.put(']');
}

template<typename Range, typename Writer>
void writeArray(const Range& range, Writer writer)
{
    std::cout.put('[');
    bool needs_comma = false;
    for (const auto& item : range)
    {
        if (needs_comma)
        {
            std::cout.put(',');
        }
        writer(item);
        needs_comma = true;
    }
    std::cout.put(']');
}

void writeEntryPoints(const ShaderManifest& manifest)
{
    writeArray(manifest.mEntryPoints,
               [](const ShaderEntryPoint& entry)
               {
                   std::cout << "{\"module\":";
                   writeJsonString(moduleName(entry.mStage));
                   std::cout << ",\"name\":";
                   writeJsonString(entry.mName);
                   std::cout << ",\"stage\":";
                   writeJsonString(reflectionStage(entry.mStage));
                   std::cout.put('}');
               });
}

void writeVertexInputs(const ShaderManifest& manifest)
{
    writeArray(manifest.mVertexInputs,
               [](const ShaderVertexInput& input)
               {
                   std::cout << "{\"name\":";
                   writeJsonString(input.mName);
                   std::cout << ",\"location\":" << input.mLocation << ",\"type\":";
                   writeJsonString(vertexValueType(input.mFormat));
                   std::cout.put('}');
               });
}

void writeInterstageVariables(const ShaderManifest& manifest)
{
    writeArray(manifest.mInterstageVariables,
               [](const ShaderInterstageVariable& variable)
               {
                   std::cout << "{\"name\":";
                   writeJsonString(variable.mName);
                   std::cout << ",\"location\":" << variable.mLocation << ",\"type\":";
                   writeJsonString(valueType(variable.mType));
                   std::cout.put('}');
               });
}

void writeUniformBlocks(const ShaderManifest& manifest)
{
    std::cout.put('[');
    if (manifest.mParameterBlock)
    {
        const ShaderParameterBlock& block = *manifest.mParameterBlock;
        std::cout << "{\"name\":";
        writeJsonString(block.mName);
        std::cout << ",\"set\":" << block.mSet << ",\"binding\":" << block.mBinding << ",\"size\":" << block.mByteSize << ",\"stages\":";
        writeStages(block.mVisibility);
        std::cout.put('}');
    }
    std::cout.put(']');
}

void writeSampledImages(const ShaderManifest& manifest)
{
    writeArray(manifest.mSampledImages,
               [](const ShaderSampledImage& image)
               {
                   std::cout << "{\"name\":";
                   writeJsonString(image.mName);
                   std::cout << ",\"set\":" << image.mSet << ",\"binding\":" << image.mBinding << ",\"type\":";
                   writeJsonString(sampledImageType(image.mDimension));
                   std::cout << ",\"stages\":";
                   writeStages(image.mVisibility);
                   std::cout.put('}');
               });
}

void writeFragmentOutputs(const ShaderManifest& manifest)
{
    std::cout.put('[');
    bool needs_comma = false;
    for (const ShaderFragmentOutputDeclaration& declaration : manifest.mFragmentOutputDeclarations)
    {
        for (std::uint32_t offset = 0; offset < declaration.mElementCount; ++offset)
        {
            if (needs_comma)
            {
                std::cout.put(',');
            }
            std::cout << "{\"name\":";
            writeJsonString(declaration.mName);
            std::cout << ",\"location\":" << declaration.mFirstLocation + offset << ",\"type\":";
            writeJsonString(valueType(declaration.mElementType));
            std::cout.put('}');
            needs_comma = true;
        }
    }
    std::cout.put(']');
}

void writePushConstantRanges(const ShaderManifest& manifest)
{
    writeArray(manifest.mPushConstantRanges,
               [](const ShaderPushConstantRange& range)
               {
                   std::cout << "{\"offset\":" << range.mByteOffset << ",\"size\":" << range.mByteSize << ",\"stages\":";
                   writeStages(range.mVisibility);
                   std::cout.put('}');
               });
}

void writeFlatInterfaces(const ShaderManifest& manifest)
{
    std::cout.put('[');
    bool needs_comma = false;
    for (const ShaderInterstageVariable& variable : manifest.mInterstageVariables)
    {
        if (variable.mInterpolation != ShaderInterpolation::Flat)
        {
            continue;
        }
        if (needs_comma)
        {
            std::cout.put(',');
        }
        std::cout << "{\"name\":";
        writeJsonString(variable.mName);
        std::cout << ",\"location\":" << variable.mLocation << ",\"modules\":[\"vertex\",\"fragment\"]}";
        needs_comma = true;
    }
    std::cout.put(']');
}

void writeExpectation(const ShaderManifest& manifest)
{
    std::cout << "{\"schema\":1,\"entry_points\":";
    writeEntryPoints(manifest);
    std::cout << ",\"vertex_inputs\":";
    writeVertexInputs(manifest);
    std::cout << ",\"interstage_variables\":";
    writeInterstageVariables(manifest);
    std::cout << ",\"uniform_blocks\":";
    writeUniformBlocks(manifest);
    std::cout << ",\"combined_image_samplers\":";
    writeSampledImages(manifest);
    std::cout << ",\"fragment_outputs\":";
    writeFragmentOutputs(manifest);
    std::cout << ",\"push_constant_ranges\":";
    writePushConstantRanges(manifest);
    std::cout << ",\"flat_interfaces\":";
    writeFlatInterfaces(manifest);
    std::cout << "}\n";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 5 || std::string_view(argv[1]) != "--profile" || std::string_view(argv[3]) != "--output")
    {
        std::cerr << "usage: " << argv[0] << " --profile diagnostic|production --output PATH\n";
        return 2;
    }

    const std::optional<MaterialProfile> profile = materialProfile(argv[2]);
    if (!profile)
    {
        std::cerr << "usage: " << argv[0] << " --profile diagnostic|production --output PATH\n";
        return 2;
    }

    try
    {
        const std::optional<LLRenderContract::ShaderManifest> manifest =
            LLRenderContract::legacyNormSpecShaderManifest(pipelineKey(*profile), LLRenderContract::ShaderBackend::Vulkan);
        if (!manifest || !validProfileManifest(*profile, *manifest) || manifest->mBackend != LLRenderContract::ShaderBackend::Vulkan)
        {
            std::cerr << "cannot dump an invalid Vulkan material profile manifest\n";
            return 1;
        }
        if (std::freopen(argv[4], "wb", stdout) == nullptr)
        {
            std::cerr << "cannot open shader-manifest output " << argv[4] << '\n';
            return 1;
        }
        writeExpectation(*manifest);
        std::cout.flush();
        if (!std::cout)
        {
            std::cerr << "cannot write Vulkan material profile manifest\n";
            return 1;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "cannot dump Vulkan material profile manifest: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
