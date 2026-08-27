/**
 * @file llshadermanifest.h
 * @brief API-neutral shader assembly and interface manifest.
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

#ifndef LL_LLSHADERMANIFEST_H
#define LL_LLSHADERMANIFEST_H

#include "lldrawpacketcontract.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace LLRenderContract
{

enum class ShaderBackend : std::uint8_t
{
    OpenGL,
    Vulkan
};

enum class ShaderStage : std::uint8_t
{
    Vertex,
    Fragment
};

struct ShaderStageVisibility
{
    bool mVertex   = false;
    bool mFragment = false;

    friend constexpr bool operator==(const ShaderStageVisibility&, const ShaderStageVisibility&) = default;
};

enum class ShaderSourceRole : std::uint8_t
{
    Primary,
    Feature,
    Wrapper,
    SharedInclude
};

struct ShaderSourceUnit
{
    ShaderStage                  mStage = ShaderStage::Vertex;
    std::string                  mPath;
    ShaderSourceRole             mRole = ShaderSourceRole::Primary;
    std::optional<std::uint32_t> mShaderClass;

    friend bool operator==(const ShaderSourceUnit&, const ShaderSourceUnit&) = default;
};

struct ShaderDefine
{
    std::string           mName;
    std::string           mValue;
    ShaderStageVisibility mVisibility;

    friend bool operator==(const ShaderDefine&, const ShaderDefine&) = default;
};

struct ShaderEntryPoint
{
    ShaderStage mStage = ShaderStage::Vertex;
    std::string mName;

    friend bool operator==(const ShaderEntryPoint&, const ShaderEntryPoint&) = default;
};

enum class ShaderValueType : std::uint8_t
{
    Float,
    Float2,
    Float3,
    Float4
};

struct ShaderVertexInput
{
    std::string    mName;
    VertexSemantic mSemantic = VertexSemantic::Position;
    VertexFormat   mFormat   = VertexFormat::Float3;
    std::uint32_t  mLocation = 0;
    std::uint32_t  mBinding  = 0;
    std::uint32_t  mStride   = 0;

    friend bool operator==(const ShaderVertexInput&, const ShaderVertexInput&) = default;
};

enum class ShaderInterpolation : std::uint8_t
{
    Smooth,
    Flat
};

struct ShaderInterstageVariable
{
    std::string         mName;
    std::uint32_t       mLocation      = 0;
    ShaderValueType     mType          = ShaderValueType::Float;
    ShaderInterpolation mInterpolation = ShaderInterpolation::Smooth;

    friend bool operator==(const ShaderInterstageVariable&, const ShaderInterstageVariable&) = default;
};

enum class SampledImageRole : std::uint8_t
{
    Diffuse,
    Normal,
    Specular
};

enum class ShaderImageDimension : std::uint8_t
{
    TwoD
};

struct ShaderSampledImage
{
    std::string           mName;
    SampledImageRole      mRole      = SampledImageRole::Diffuse;
    ShaderImageDimension  mDimension = ShaderImageDimension::TwoD;
    std::uint32_t         mSet       = 0;
    std::uint32_t         mBinding   = 0;
    ShaderStageVisibility mVisibility;

    friend bool operator==(const ShaderSampledImage&, const ShaderSampledImage&) = default;
};

enum class ShaderParameterOrigin : std::uint8_t
{
    CopiedDraw,
    DrawAndFrameDerived,
    FrameOrPass,
    FixedDefault
};

struct ShaderLogicalParameter
{
    std::string           mName;
    std::string           mShaderName;
    std::uint32_t         mWordOffset = 0;
    std::uint32_t         mWordCount  = 0;
    ShaderParameterOrigin mOrigin     = ShaderParameterOrigin::CopiedDraw;
    ShaderStageVisibility mVisibility;
    std::optional<float>  mFixedValue;

    friend bool operator==(const ShaderLogicalParameter&, const ShaderLogicalParameter&) = default;
};

struct ShaderParameterBlock
{
    std::string           mName;
    std::uint32_t         mSet      = 0;
    std::uint32_t         mBinding  = 0;
    std::uint32_t         mByteSize = 0;
    ShaderStageVisibility mVisibility;

    friend bool operator==(const ShaderParameterBlock&, const ShaderParameterBlock&) = default;
};

enum class ShaderFragmentOutputRole : std::uint8_t
{
    DiffuseEmissive,
    SpecularGloss,
    NormalEnvironment,
    EmissiveBuffer
};

struct ShaderLogicalFragmentOutput
{
    ShaderFragmentOutputRole mRole     = ShaderFragmentOutputRole::DiffuseEmissive;
    std::uint32_t            mLocation = 0;
    ShaderValueType          mType     = ShaderValueType::Float4;

    friend bool operator==(const ShaderLogicalFragmentOutput&, const ShaderLogicalFragmentOutput&) = default;
};

struct ShaderFragmentOutputDeclaration
{
    std::string     mName;
    std::uint32_t   mFirstLocation       = 0;
    std::uint32_t   mElementCount        = 0;
    ShaderValueType mElementType         = ShaderValueType::Float4;
    std::uint32_t   mLogicalElementCount = 0;
    bool            mExtraElementsInert  = false;

    friend bool operator==(const ShaderFragmentOutputDeclaration&, const ShaderFragmentOutputDeclaration&) = default;
};

struct ShaderLinkedBlockBaggage
{
    std::string              mName;
    std::uint32_t            mBinding  = 0;
    std::uint32_t            mByteSize = 0;
    ShaderStageVisibility    mVisibility;
    std::vector<std::string> mActiveMembers;

    friend bool operator==(const ShaderLinkedBlockBaggage&, const ShaderLinkedBlockBaggage&) = default;
};

struct ShaderPushConstantRange
{
    std::uint32_t         mByteOffset = 0;
    std::uint32_t         mByteSize   = 0;
    ShaderStageVisibility mVisibility;

    friend bool operator==(const ShaderPushConstantRange&, const ShaderPushConstantRange&) = default;
};

struct ShaderManifest
{
    ShaderProgramKey                             mProgram;
    ShaderBackend                                mBackend = ShaderBackend::OpenGL;
    std::vector<ShaderEntryPoint>                mEntryPoints;
    std::vector<ShaderSourceUnit>                mSourceUnits;
    std::vector<ShaderDefine>                    mDefines;
    std::vector<ShaderVertexInput>               mVertexInputs;
    std::vector<ShaderInterstageVariable>        mInterstageVariables;
    std::vector<ShaderSampledImage>              mSampledImages;
    std::vector<ShaderLogicalParameter>          mLogicalParameters;
    std::optional<ShaderParameterBlock>          mParameterBlock;
    std::vector<ShaderLogicalFragmentOutput>     mLogicalFragmentOutputs;
    std::vector<ShaderFragmentOutputDeclaration> mFragmentOutputDeclarations;
    std::vector<ShaderLinkedBlockBaggage>        mLinkedBlockBaggage;
    std::vector<ShaderPushConstantRange>         mPushConstantRanges;

    friend bool operator==(const ShaderManifest& left, const ShaderManifest& right);
};

bool validShaderManifest(const ShaderManifest& manifest) noexcept;
bool validLegacyNormSpecDiagnosticShaderManifest(const ShaderManifest& manifest) noexcept;
bool validLegacyNormSpecProductionShaderManifest(const ShaderManifest& manifest) noexcept;

ShaderManifest legacyNormSpecDiagnosticShaderManifest(ShaderBackend backend);

std::optional<ShaderManifest> legacyNormSpecShaderManifest(const LegacyNormSpecPipelineKey& pipeline_key, ShaderBackend backend);

} // namespace LLRenderContract

#endif // LL_LLSHADERMANIFEST_H
