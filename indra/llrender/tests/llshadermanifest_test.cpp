/**
 * @file llshadermanifest_test.cpp
 * @brief Tests for the canonical legacy material shader manifests.
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

#include "linden_common.h"

#include "llshadermanifest.h"
#include "lltut.h"

#include <algorithm>
#include <limits>

namespace
{
using namespace LLRenderContract;

template<typename Range, typename Predicate>
const typename Range::value_type* findValue(const Range& values, Predicate predicate)
{
    const auto found = std::find_if(values.begin(), values.end(), predicate);
    return found == values.end() ? nullptr : &*found;
}

} // namespace

namespace tut
{

struct shader_manifest_test
{
};

using shader_manifest_test_group  = test_group<shader_manifest_test>;
using shader_manifest_test_object = shader_manifest_test_group::object;
shader_manifest_test_group shader_manifest_tests("shader manifest");

template<>
template<>
void shader_manifest_test_object::test<1>()
{
    const ShaderManifest manifest = legacyNormSpecDiagnosticShaderManifest(ShaderBackend::OpenGL);
    ensure("OpenGL manifest is structurally valid", validShaderManifest(manifest));
    ensure("OpenGL manifest is the exact diagnostic recipe", validLegacyNormSpecDiagnosticShaderManifest(manifest));
    ensure("diagnostic semantic program is explicit",
           manifest.mProgram.mName == LEGACY_NORMSPEC_PIPELINE_NAME && manifest.mProgram.mVariant == LEGACY_NORMSPEC_DIAGNOSTIC_VARIANT);
    ensure("OpenGL primary and feature assembly order is complete",
           manifest.mSourceUnits.size() == 20 && manifest.mSourceUnits[0].mPath == "deferred/materialV.glsl" &&
               manifest.mSourceUnits[0].mShaderClass == 1 && manifest.mSourceUnits[1].mPath == "deferred/materialF.glsl" &&
               manifest.mSourceUnits[1].mShaderClass == 3 && manifest.mSourceUnits[12].mPath == "deferred/deferredUtil.glsl" &&
               manifest.mSourceUnits[19].mPath == "objects/nonindexedTextureV.glsl");
    const auto* screen_space = findValue(manifest.mSourceUnits, [](const ShaderSourceUnit& source)
                                         { return source.mPath == "deferred/screenSpaceReflUtil.glsl"; });
    const auto* reflection_probe =
        findValue(manifest.mSourceUnits, [](const ShaderSourceUnit& source) { return source.mPath == "deferred/reflectionProbeF.glsl"; });
    ensure("OpenGL resolved feature classes are exact",
           std::all_of(manifest.mSourceUnits.begin(), manifest.mSourceUnits.end(),
                       [](const ShaderSourceUnit& source) { return source.mShaderClass.has_value(); }) &&
               screen_space && screen_space->mShaderClass == 1 && reflection_probe && reflection_probe->mShaderClass == 3);
    ensure(
        "diagnostic assembly omits emissive and shadow permutations",
        manifest.mDefines.size() == 3 &&
            !findValue(manifest.mDefines, [](const ShaderDefine& define) { return define.mName == "HAS_EMISSIVE"; }) &&
            !findValue(manifest.mDefines, [](const ShaderDefine& define) { return define.mName == "HAS_SUN_SHADOW"; }) &&
            !findValue(manifest.mSourceUnits, [](const ShaderSourceUnit& source) { return source.mPath == "deferred/shadowUtil.glsl"; }));

    const auto* color =
        findValue(manifest.mVertexInputs, [](const ShaderVertexInput& input) { return input.mSemantic == VertexSemantic::Color; });
    const auto* tangent =
        findValue(manifest.mVertexInputs, [](const ShaderVertexInput& input) { return input.mSemantic == VertexSemantic::Tangent; });
    ensure("OpenGL sparse attribute mapping is recorded",
           manifest.mVertexInputs.size() == 7 && color && color->mLocation == 6 && color->mStride == 4 && tangent &&
               tangent->mLocation == 8 && tangent->mStride == 16);
    ensure("OpenGL sampler channels are exact", manifest.mSampledImages.size() == 3 && manifest.mSampledImages[0].mSet == 0 &&
                                                    manifest.mSampledImages[0].mBinding == 0 && manifest.mSampledImages[2].mBinding == 2);

    const auto* mirror =
        findValue(manifest.mLogicalParameters, [](const ShaderLogicalParameter& parameter) { return parameter.mName == "mirror"; });
    ensure("all 68 words have named origins",
           manifest.mLogicalParameters.size() == 9 && manifest.mLogicalParameters.front().mWordOffset == 0 && mirror &&
               mirror->mWordOffset == 67 && mirror->mWordCount == 1 && mirror->mOrigin == ShaderParameterOrigin::FixedDefault &&
               mirror->mFixedValue == 0.f);
    ensure("logical writes are separate from the inert linked declaration",
           manifest.mLogicalFragmentOutputs.size() == 3 && manifest.mFragmentOutputDeclarations.size() == 1 &&
               manifest.mFragmentOutputDeclarations[0].mElementCount == 4 &&
               manifest.mFragmentOutputDeclarations[0].mLogicalElementCount == 3 &&
               manifest.mFragmentOutputDeclarations[0].mExtraElementsInert);
    ensure("OpenGL linked reflection baggage is explicit",
           manifest.mLinkedBlockBaggage.size() == 1 && manifest.mLinkedBlockBaggage[0].mName == "ReflectionProbes" &&
               manifest.mLinkedBlockBaggage[0].mBinding == 0 && manifest.mLinkedBlockBaggage[0].mByteSize == 49248 &&
               manifest.mLinkedBlockBaggage[0].mActiveMembers.size() == 12);
}

template<>
template<>
void shader_manifest_test_object::test<2>()
{
    const ShaderManifest manifest = legacyNormSpecDiagnosticShaderManifest(ShaderBackend::Vulkan);
    ensure("Vulkan manifest is structurally valid", validShaderManifest(manifest));
    ensure("Vulkan manifest is the exact diagnostic recipe", validLegacyNormSpecDiagnosticShaderManifest(manifest));
    ensure("Vulkan wrapper and shared source order is exact",
           manifest.mSourceUnits.size() == 5 && manifest.mSourceUnits[0].mPath == "indra/llrender/vulkan/shaders/material.vert.glsl" &&
               manifest.mSourceUnits[1].mPath == "indra/newview/app_settings/shaders/class1/deferred/materialV.glsl" &&
               manifest.mSourceUnits[2].mPath == "indra/llrender/vulkan/shaders/material.frag.glsl" &&
               manifest.mSourceUnits[4].mShaderClass == 3);
    ensure("Vulkan declaration defines remain diagnostic",
           manifest.mDefines.size() == 9 && manifest.mDefines.front().mName == "LL_VULKAN_SHADER" &&
               manifest.mDefines.back().mName == "GBUFFER_FLAG_HAS_ATMOS" && manifest.mDefines.back().mValue == "0.34");
    ensure("Vulkan vertex locations and bindings are dense",
           manifest.mVertexInputs.size() == 7 && manifest.mVertexInputs[0].mLocation == 0 && manifest.mVertexInputs[0].mBinding == 0 &&
               manifest.mVertexInputs[3].mLocation == 3 && manifest.mVertexInputs[3].mBinding == 3 &&
               manifest.mVertexInputs[6].mLocation == 6 && manifest.mVertexInputs[6].mBinding == 6);

    const auto* flat_sign =
        findValue(manifest.mInterstageVariables, [](const ShaderInterstageVariable& variable) { return variable.mName == "vary_sign"; });
    ensure("the complete Vulkan stage interface preserves flat tangent sign",
           manifest.mInterstageVariables.size() == 8 && flat_sign && flat_sign->mLocation == 2 &&
               flat_sign->mType == ShaderValueType::Float && flat_sign->mInterpolation == ShaderInterpolation::Flat);
    ensure("Vulkan sampled images occupy set one",
           std::all_of(manifest.mSampledImages.begin(), manifest.mSampledImages.end(),
                       [](const ShaderSampledImage& image) { return image.mSet == 1 && image.mVisibility.mFragment; }));
    ensure("the 272-byte packet is visible to both stages at set zero",
           manifest.mParameterBlock && manifest.mParameterBlock->mName == "MaterialParameterPacket" &&
               manifest.mParameterBlock->mSet == 0 && manifest.mParameterBlock->mBinding == 0 &&
               manifest.mParameterBlock->mByteSize == 272 && manifest.mParameterBlock->mVisibility.mVertex &&
               manifest.mParameterBlock->mVisibility.mFragment);
    ensure("Vulkan declares only the three proven outputs and no hidden resources",
           manifest.mFragmentOutputDeclarations.size() == 1 && manifest.mFragmentOutputDeclarations[0].mElementCount == 3 &&
               manifest.mFragmentOutputDeclarations[0].mLogicalElementCount == 3 &&
               !manifest.mFragmentOutputDeclarations[0].mExtraElementsInert && manifest.mLinkedBlockBaggage.empty() &&
               manifest.mPushConstantRanges.empty());
}

template<>
template<>
void shader_manifest_test_object::test<3>()
{
    LegacyNormSpecPipelineKey diagnostic_key = legacyNormSpecDiagnosticPipelineKey();
    const auto                resolved       = legacyNormSpecShaderManifest(diagnostic_key, ShaderBackend::OpenGL);
    ensure("the diagnostic key resolves", resolved.has_value());
    const ShaderManifest owned = *resolved;

    diagnostic_key.mProgram.mName.assign("mutated.after.lookup");
    diagnostic_key.mColorTargets.clear();
    ensure("lookup returns value-owned strings and arrays",
           *resolved == owned && resolved->mProgram.mName == LEGACY_NORMSPEC_PIPELINE_NAME);

    ensure("both production profiles are valid descriptions", validLegacyNormSpecPipelineKey(legacyNormSpecModernHDRPipelineKey()) &&
                                                                  validLegacyNormSpecPipelineKey(legacyNormSpecCompatibilityPipelineKey()));
    ensure("production compatibility has no OpenGL artifact",
           !legacyNormSpecShaderManifest(legacyNormSpecCompatibilityPipelineKey(), ShaderBackend::OpenGL));

    LegacyNormSpecPipelineKey changed = legacyNormSpecDiagnosticPipelineKey();
    changed.mShaderVariant.mEmissive  = LegacyNormSpecEmissive::Enabled;
    ensure("emissive dimension mutation cannot alias the diagnostic", !legacyNormSpecShaderManifest(changed, ShaderBackend::OpenGL));
    changed                                = legacyNormSpecDiagnosticPipelineKey();
    changed.mShaderVariant.mShadowAssembly = ShadowAssembly::Sun;
    ensure("shadow assembly mutation cannot alias the diagnostic", !legacyNormSpecShaderManifest(changed, ShaderBackend::Vulkan));
    changed                = legacyNormSpecDiagnosticPipelineKey();
    changed.mTargetProfile = LegacyNormSpecTargetProfile::Compatibility;
    ensure("target-profile mutation cannot alias the diagnostic", !legacyNormSpecShaderManifest(changed, ShaderBackend::OpenGL));
    changed                   = legacyNormSpecDiagnosticPipelineKey();
    changed.mProgram.mVariant = LEGACY_NORMSPEC_PRODUCTION_VARIANT;
    ensure("encoded program mutation cannot alias the diagnostic", !legacyNormSpecShaderManifest(changed, ShaderBackend::Vulkan));
    ensure("unknown backends fail closed",
           !legacyNormSpecShaderManifest(legacyNormSpecDiagnosticPipelineKey(), static_cast<ShaderBackend>(255)));
    const ShaderManifest unknown_backend = legacyNormSpecDiagnosticShaderManifest(static_cast<ShaderBackend>(255));
    ensure("direct construction cannot turn an unknown backend into Vulkan",
           unknown_backend.mBackend == static_cast<ShaderBackend>(255) && !validShaderManifest(unknown_backend));
}

template<>
template<>
void shader_manifest_test_object::test<4>()
{
    const ShaderManifest canonical = legacyNormSpecDiagnosticShaderManifest(ShaderBackend::OpenGL);
    ShaderManifest       changed   = canonical;
    changed.mSourceUnits.push_back(changed.mSourceUnits.front());
    ensure("duplicate stage source units are structurally invalid", !validShaderManifest(changed));

    changed = canonical;
    std::swap(changed.mSourceUnits[2], changed.mSourceUnits[3]);
    ensure("source link order is part of the exact recipe",
           validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed                               = canonical;
    changed.mSourceUnits[14].mShaderClass = 2;
    ensure("resolved feature class is part of the exact recipe",
           validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed = canonical;
    changed.mDefines.push_back(changed.mDefines.front());
    ensure("duplicate visible defines are rejected", !validShaderManifest(changed));

    changed                            = canonical;
    changed.mVertexInputs[1].mLocation = changed.mVertexInputs[0].mLocation;
    ensure("duplicate vertex locations are rejected", !validShaderManifest(changed));

    changed                            = canonical;
    changed.mSampledImages[1].mBinding = changed.mSampledImages[0].mBinding;
    ensure("duplicate descriptor coordinates are rejected", !validShaderManifest(changed));

    changed                                   = canonical;
    changed.mLogicalParameters[5].mVisibility = {};
    ensure("parameters require stage visibility", !validShaderManifest(changed));

    changed = canonical;
    changed.mLogicalParameters[5].mWordOffset++;
    ensure("missing and overlapping parameter words are rejected", !validShaderManifest(changed));

    changed                                                    = canonical;
    changed.mFragmentOutputDeclarations[0].mExtraElementsInert = false;
    ensure("extra backend declarations must be marked inert", !validShaderManifest(changed));

    changed = canonical;
    changed.mLinkedBlockBaggage[0].mActiveMembers.push_back("refBox");
    ensure("duplicate linked-block members are rejected", !validShaderManifest(changed));
}

template<>
template<>
void shader_manifest_test_object::test<5>()
{
    const ShaderManifest canonical = legacyNormSpecDiagnosticShaderManifest(ShaderBackend::Vulkan);
    ShaderManifest       changed   = canonical;
    changed.mSourceUnits.pop_back();
    ensure("missing included material math fails exact validation",
           validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed = canonical;
    changed.mInterstageVariables.erase(changed.mInterstageVariables.begin() + 2);
    ensure("missing flat interstage data fails exact validation",
           validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed                                        = canonical;
    changed.mParameterBlock->mVisibility.mFragment = false;
    ensure("incorrect reflected block visibility fails exact validation",
           validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed = canonical;
    std::swap(changed.mDefines[0], changed.mDefines[1]);
    ensure("define order is exact", validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed          = canonical;
    changed.mBackend = ShaderBackend::OpenGL;
    ensure("backend identity is exact", validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed                       = canonical;
    changed.mEntryPoints[0].mName = "vertexMain";
    ensure("entry-point spelling is exact", validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed = canonical;
    std::swap(changed.mVertexInputs[0].mSemantic, changed.mVertexInputs[1].mSemantic);
    changed.mVertexInputs[0].mFormat = VertexFormat::Float4;
    ensure("vertex semantic and format mapping is exact",
           validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed                               = canonical;
    changed.mLogicalParameters[5].mOrigin = ShaderParameterOrigin::CopiedDraw;
    ensure("parameter provenance is exact", validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed                            = canonical;
    changed.mSampledImages[2].mBinding = 9;
    ensure("descriptor coordinates are exact", validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed = canonical;
    for (ShaderLogicalFragmentOutput& output : changed.mLogicalFragmentOutputs)
    {
        ++output.mLocation;
    }
    ++changed.mFragmentOutputDeclarations[0].mFirstLocation;
    ensure("fragment output locations are exact", validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed                                                    = canonical;
    changed.mFragmentOutputDeclarations[0].mElementCount       = 4;
    changed.mFragmentOutputDeclarations[0].mExtraElementsInert = true;
    ensure("fragment declaration counts are exact", validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed                           = canonical;
    changed.mParameterBlock->mSet     = 1;
    changed.mParameterBlock->mBinding = 0;
    ensure("parameter blocks cannot collide with sampled images", !validShaderManifest(changed));

    changed = canonical;
    changed.mPushConstantRanges.push_back({ 0, 4, { true, false } });
    ensure("an extra push-constant range is not the proven recipe",
           validShaderManifest(changed) && !validLegacyNormSpecDiagnosticShaderManifest(changed));
    changed.mPushConstantRanges[0].mByteSize = 0;
    ensure("zero-sized push constants are structurally invalid", !validShaderManifest(changed));

    changed                                               = canonical;
    changed.mFragmentOutputDeclarations[0].mElementCount  = std::numeric_limits<std::uint32_t>::max();
    changed.mFragmentOutputDeclarations[0].mFirstLocation = 1;
    ensure("overflowing output declarations are rejected", !validShaderManifest(changed));
}

template<>
template<>
void shader_manifest_test_object::test<6>()
{
    const auto modern        = legacyNormSpecShaderManifest(legacyNormSpecModernHDRPipelineKey(), ShaderBackend::Vulkan);
    const auto compatibility = legacyNormSpecShaderManifest(legacyNormSpecCompatibilityPipelineKey(), ShaderBackend::Vulkan);
    ensure("both production profiles resolve a Vulkan shader contract", modern.has_value() && compatibility.has_value());
    ensure("target formats do not fork the production shader contract", *modern == *compatibility);

    const ShaderManifest& manifest   = *modern;
    const ShaderManifest  diagnostic = legacyNormSpecDiagnosticShaderManifest(ShaderBackend::Vulkan);
    ensure("the production shader contract is structurally and exactly valid",
           validShaderManifest(manifest) && validLegacyNormSpecProductionShaderManifest(manifest) &&
               !validLegacyNormSpecDiagnosticShaderManifest(manifest));
    ensure("the production semantic program variant is explicit",
           manifest.mProgram.mName == LEGACY_NORMSPEC_PIPELINE_NAME && manifest.mProgram.mVariant == LEGACY_NORMSPEC_PRODUCTION_VARIANT);
    ensure("production retains the proven source and vertex-stage contracts",
           manifest.mSourceUnits == diagnostic.mSourceUnits && manifest.mVertexInputs == diagnostic.mVertexInputs &&
               manifest.mInterstageVariables == diagnostic.mInterstageVariables);
    ensure("production retains the proven material resources",
           manifest.mSampledImages == diagnostic.mSampledImages && manifest.mLogicalParameters == diagnostic.mLogicalParameters &&
               manifest.mParameterBlock == diagnostic.mParameterBlock && manifest.mLinkedBlockBaggage.empty() &&
               manifest.mPushConstantRanges.empty());

    ensure("the ordered production compile and effective macro recipe is complete",
           manifest.mDefines.size() == 19 && manifest.mDefines[0].mName == "LL_VULKAN_MATERIAL_PRODUCTION" &&
               manifest.mDefines[0].mVisibility.mVertex && !manifest.mDefines[0].mVisibility.mFragment &&
               manifest.mDefines[5].mName == "HAS_EMISSIVE" && manifest.mDefines[8].mName == "SPOT_SHADOW" &&
               manifest.mDefines[9].mName == "LL_VULKAN_MATERIAL_PRODUCTION" && !manifest.mDefines[9].mVisibility.mVertex &&
               manifest.mDefines[9].mVisibility.mFragment && manifest.mDefines[14].mName == "GBUFFER_FLAG_HAS_ATMOS" &&
               manifest.mDefines[15].mName == "HAS_EMISSIVE" && manifest.mDefines[16].mName == "HAS_SUN_SHADOW" &&
               manifest.mDefines[17].mName == "SUN_SHADOW" && manifest.mDefines[18].mName == "SPOT_SHADOW");
    ensure("production exposes a fourth logical emissive output",
           manifest.mLogicalFragmentOutputs.size() == 4 &&
               manifest.mLogicalFragmentOutputs[3].mRole == ShaderFragmentOutputRole::EmissiveBuffer &&
               manifest.mLogicalFragmentOutputs[3].mLocation == 3 && manifest.mLogicalFragmentOutputs[3].mType == ShaderValueType::Float4);
    ensure("production declares exactly four backend outputs",
           manifest.mFragmentOutputDeclarations.size() == 1 && manifest.mFragmentOutputDeclarations[0].mFirstLocation == 0 &&
               manifest.mFragmentOutputDeclarations[0].mElementCount == 4 &&
               manifest.mFragmentOutputDeclarations[0].mLogicalElementCount == 4 &&
               !manifest.mFragmentOutputDeclarations[0].mExtraElementsInert);

    ensure("production profiles have no OpenGL shader contract",
           !legacyNormSpecShaderManifest(legacyNormSpecModernHDRPipelineKey(), ShaderBackend::OpenGL) &&
               !legacyNormSpecShaderManifest(legacyNormSpecCompatibilityPipelineKey(), ShaderBackend::OpenGL));
    ensure("production lookup leaves the diagnostic factory exact",
           diagnostic == legacyNormSpecDiagnosticShaderManifest(ShaderBackend::Vulkan) &&
               validLegacyNormSpecDiagnosticShaderManifest(diagnostic));
}

template<>
template<>
void shader_manifest_test_object::test<7>()
{
    const auto resolved = legacyNormSpecShaderManifest(legacyNormSpecModernHDRPipelineKey(), ShaderBackend::Vulkan);
    ensure("the production mutation fixture resolves", resolved.has_value());
    const ShaderManifest canonical = *resolved;
    ShaderManifest       changed   = canonical;

    std::swap(changed.mDefines[0], changed.mDefines[1]);
    ensure("production compile-selector order is exact",
           validShaderManifest(changed) && !validLegacyNormSpecProductionShaderManifest(changed));

    changed = canonical;
    changed.mDefines.erase(changed.mDefines.begin());
    ensure("the production compile selector is required",
           validShaderManifest(changed) && !validLegacyNormSpecProductionShaderManifest(changed));

    changed                   = canonical;
    changed.mProgram.mVariant = LEGACY_NORMSPEC_DIAGNOSTIC_VARIANT;
    ensure("production program identity is exact",
           validShaderManifest(changed) && !validLegacyNormSpecProductionShaderManifest(changed) &&
               !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed = canonical;
    std::swap(changed.mSourceUnits[0], changed.mSourceUnits[1]);
    ensure("production source order is exact", validShaderManifest(changed) && !validLegacyNormSpecProductionShaderManifest(changed));

    changed          = canonical;
    changed.mBackend = ShaderBackend::OpenGL;
    ensure("production backend identity is exact",
           validShaderManifest(changed) && !validLegacyNormSpecProductionShaderManifest(changed) &&
               !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed                            = canonical;
    changed.mSampledImages[0].mBinding = 3;
    ensure("production descriptor coordinates are exact",
           validShaderManifest(changed) && !validLegacyNormSpecProductionShaderManifest(changed));

    changed                     = canonical;
    changed.mDefines[15].mValue = "0";
    changed.mProgram.mVariant   = LEGACY_NORMSPEC_DIAGNOSTIC_VARIANT;
    ensure("mixed production macros and diagnostic program identity match no exact recipe",
           validShaderManifest(changed) && !validLegacyNormSpecProductionShaderManifest(changed) &&
               !validLegacyNormSpecDiagnosticShaderManifest(changed));

    changed = canonical;
    changed.mLogicalFragmentOutputs.pop_back();
    changed.mFragmentOutputDeclarations[0].mElementCount        = 3;
    changed.mFragmentOutputDeclarations[0].mLogicalElementCount = 3;
    ensure("a three-output production variant is not canonical",
           validShaderManifest(changed) && !validLegacyNormSpecProductionShaderManifest(changed));

    changed                                  = canonical;
    changed.mLogicalFragmentOutputs[3].mRole = ShaderFragmentOutputRole::NormalEnvironment;
    ensure("duplicate logical output roles are structurally invalid", !validShaderManifest(changed));

    changed                                  = canonical;
    changed.mLogicalFragmentOutputs[3].mRole = static_cast<ShaderFragmentOutputRole>(255);
    ensure("unknown logical output roles are structurally invalid", !validShaderManifest(changed));

    changed                                      = canonical;
    changed.mLogicalFragmentOutputs[3].mLocation = 4;
    changed.mFragmentOutputDeclarations          = { { "frag_data", 0, 3, ShaderValueType::Float4, 3, false },
                                                     { "emissive_output", 4, 1, ShaderValueType::Float4, 1, false } };
    ensure("the production output declaration shape is exact",
           validShaderManifest(changed) && !validLegacyNormSpecProductionShaderManifest(changed));

    LegacyNormSpecPipelineKey mixed_key = legacyNormSpecModernHDRPipelineKey();
    mixed_key.mTargetProfile            = LegacyNormSpecTargetProfile::Compatibility;
    ensure("mixed production profile dimensions do not resolve", !legacyNormSpecShaderManifest(mixed_key, ShaderBackend::Vulkan));

    mixed_key                          = legacyNormSpecCompatibilityPipelineKey();
    mixed_key.mShaderVariant.mEmissive = LegacyNormSpecEmissive::Disabled;
    ensure("mixed production shader dimensions do not resolve", !legacyNormSpecShaderManifest(mixed_key, ShaderBackend::Vulkan));
}

} // namespace tut
