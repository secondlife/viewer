/**
 * @file llmaterialparametercontract_test.cpp
 * @brief Tests for materializing owned production material parameters.
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

#include "llmaterialparametercontract.h"
#include "lltut.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>

namespace
{
using namespace LLRenderContract;

static_assert(sizeof(MaterialParameters) == 68 * sizeof(float));
static_assert(offsetof(MaterialParameters, mModelviewMatrix) == 0 * sizeof(float));
static_assert(offsetof(MaterialParameters, mModelviewProjectionMatrix) == 16 * sizeof(float));
static_assert(offsetof(MaterialParameters, mNormalMatrix) == 32 * sizeof(float));
static_assert(offsetof(MaterialParameters, mTextureMatrix0) == 41 * sizeof(float));
static_assert(offsetof(MaterialParameters, mSpecularColor) == 57 * sizeof(float));
static_assert(offsetof(MaterialParameters, mClipPlane) == 61 * sizeof(float));
static_assert(offsetof(MaterialParameters, mEnvironmentIntensity) == 65 * sizeof(float));
static_assert(offsetof(MaterialParameters, mEmissiveBrightness) == 66 * sizeof(float));
static_assert(offsetof(MaterialParameters, mMirror) == 67 * sizeof(float));

LegacyNormSpecDrawInputs completeInputs(const LegacyNormSpecPipelineKey& pipeline_key)
{
    LegacyNormSpecDrawInputs inputs;
    inputs.mFrame                    = 91;
    inputs.mPass                     = { 12 };
    inputs.mHandles                  = { BufferHandle{ 1, 7 }, BufferHandle{ 2, 9 }, PipelineHandle{ 6, 3 } };
    inputs.mDescriptors              = { { ImageHandle{ 3, 4 }, SamplerHandle{ 5, 2 }, { 0, 3, 0, 1 } },
                                         { ImageHandle{ 4, 5 }, SamplerHandle{ 7, 3 }, { 0, 3, 0, 1 } },
                                         { ImageHandle{ 8, 6 }, SamplerHandle{ 9, 4 }, { 1, 2, 0, 1 } } };
    inputs.mPipelineKey              = pipeline_key;
    inputs.mFirstIndex               = 18;
    inputs.mIndexCount               = 6;
    inputs.mMinVertex                = 11;
    inputs.mMaxVertex                = 14;
    inputs.mDiffuseTextureMatrix[0]  = 0.75f;
    inputs.mDiffuseTextureMatrix[5]  = 0.5f;
    inputs.mDiffuseTextureMatrix[12] = 0.125f;
    inputs.mDiffuseTextureMatrix[13] = -0.25f;
    inputs.mSpecularRGBA             = { 0.2f, 0.4f, 0.8f, 0.6f };
    inputs.mEnvironmentIntensity     = 0.625f;
    inputs.mAlphaCutoff              = 0.375f;
    inputs.mEmissiveBrightness       = 1.f;
    return inputs;
}

std::optional<LegacyNormSpecDrawPacket> completePacket(const LegacyNormSpecPipelineKey& pipeline_key)
{
    return buildLegacyNormSpecDrawPacket(completeInputs(pipeline_key));
}

LegacyNormSpecWorldParameterContext completeContext()
{
    LegacyNormSpecWorldParameterContext context;
    context.mFrame         = 91;
    context.mPass          = { 12 };
    context.mBaseModelview = DRAW_IDENTITY_MATRIX4;
    context.mProjection    = DRAW_IDENTITY_MATRIX4;
    context.mEyeClipPlane  = { 0.25f, -0.5f, 0.75f, -1.25f };
    return context;
}

DrawMatrix4 affine(float scale_x, float scale_y, float scale_z, float translate_x, float translate_y, float translate_z)
{
    DrawMatrix4 matrix = DRAW_IDENTITY_MATRIX4;
    matrix[0]          = scale_x;
    matrix[5]          = scale_y;
    matrix[10]         = scale_z;
    matrix[12]         = translate_x;
    matrix[13]         = translate_y;
    matrix[14]         = translate_z;
    return matrix;
}

template<std::size_t Size>
bool approximatelyEqual(const std::array<float, Size>& left, const std::array<float, Size>& right, float tolerance = 1.e-6f)
{
    for (std::size_t index = 0; index < Size; ++index)
    {
        if (!std::isfinite(left[index]) || !std::isfinite(right[index]) || std::fabs(left[index] - right[index]) > tolerance)
        {
            return false;
        }
    }
    return true;
}

std::array<float, 68> parameterWords(const MaterialParameters& parameters)
{
    std::array<float, 68> words{};
    std::memcpy(words.data(), &parameters, sizeof(parameters));
    return words;
}

} // namespace

namespace tut
{

struct material_parameter_contract_test
{
};

using material_parameter_contract_test_group  = test_group<material_parameter_contract_test>;
using material_parameter_contract_test_object = material_parameter_contract_test_group::object;
material_parameter_contract_test_group material_parameter_contract_tests("material parameter contract");

template<>
template<>
void material_parameter_contract_test_object::test<1>()
{
    const auto packet = completePacket(legacyNormSpecModernHDRPipelineKey());
    ensure("production packet builds", packet.has_value());
    const LegacyNormSpecWorldParameterContext context    = completeContext();
    const auto                                parameters = materializeLegacyNormSpecWorldParameters(*packet, context);

    ensure("identity transforms materialize", parameters.has_value());
    ensure("materialized parameters validate", validMaterialParameters(*parameters));
    ensure("identity modelview and projection are preserved",
           parameters->mModelviewMatrix == DRAW_IDENTITY_MATRIX4 && parameters->mModelviewProjectionMatrix == DRAW_IDENTITY_MATRIX4);
    ensure("identity normal matrix is preserved",
           parameters->mNormalMatrix == std::array<float, 9>{ 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f });
    ensure("draw-owned parameters are copied",
           parameters->mTextureMatrix0 == packet->mDiffuseTextureMatrix && parameters->mSpecularColor == packet->mSpecularRGBA &&
               parameters->mEnvironmentIntensity == packet->mEnvironmentIntensity &&
               parameters->mEmissiveBrightness == packet->mEmissiveBrightness);
    ensure("World clip state is copied with mirror disabled",
           parameters->mClipPlane == context.mEyeClipPlane && parameters->mMirror == 0.f);

    const std::array<float, 68> words = parameterWords(*parameters);
    ensure("derived matrices begin at their declared word offsets",
           words[0] == 1.f && words[16] == 1.f && words[32] == 1.f && words[36] == 1.f && words[40] == 1.f);
    ensure("copied fields begin at their declared word offsets",
           words[41] == packet->mDiffuseTextureMatrix[0] && words[57] == packet->mSpecularRGBA[0] &&
               words[61] == context.mEyeClipPlane[0] && words[65] == packet->mEnvironmentIntensity &&
               words[66] == packet->mEmissiveBrightness && words[67] == 0.f);
}

template<>
template<>
void material_parameter_contract_test_object::test<2>()
{
    auto packet = completePacket(legacyNormSpecModernHDRPipelineKey());
    ensure("multiplication packet builds", packet.has_value());
    packet->mModelMatrix = affine(2.f, 3.f, 4.f, 1.f, 2.f, 3.f);

    LegacyNormSpecWorldParameterContext context = completeContext();
    context.mBaseModelview                      = affine(1.f, 1.f, 1.f, 10.f, 20.f, 30.f);
    context.mProjection                         = affine(5.f, 6.f, 7.f, 0.f, 0.f, 0.f);

    const auto parameters = materializeLegacyNormSpecWorldParameters(*packet, context);
    ensure("noncommuting transforms materialize", parameters.has_value());
    ensure("base modelview post-multiplies the draw model",
           parameters->mModelviewMatrix[0] == 2.f && parameters->mModelviewMatrix[5] == 3.f && parameters->mModelviewMatrix[10] == 4.f &&
               parameters->mModelviewMatrix[12] == 11.f && parameters->mModelviewMatrix[13] == 22.f &&
               parameters->mModelviewMatrix[14] == 33.f);
    ensure("projection pre-multiplies the composed modelview",
           parameters->mModelviewProjectionMatrix[0] == 10.f && parameters->mModelviewProjectionMatrix[5] == 18.f &&
               parameters->mModelviewProjectionMatrix[10] == 28.f && parameters->mModelviewProjectionMatrix[12] == 55.f &&
               parameters->mModelviewProjectionMatrix[13] == 132.f && parameters->mModelviewProjectionMatrix[14] == 231.f);
    ensure("ordered transform output validates", validMaterialParameters(*parameters));
}

template<>
template<>
void material_parameter_contract_test_object::test<3>()
{
    auto packet = completePacket(legacyNormSpecModernHDRPipelineKey());
    ensure("normal-matrix packet builds", packet.has_value());

    DrawMatrix4 rotated_nonuniform_scale = DRAW_IDENTITY_MATRIX4;
    rotated_nonuniform_scale[0]          = 0.f;
    rotated_nonuniform_scale[1]          = 2.f;
    rotated_nonuniform_scale[4]          = -3.f;
    rotated_nonuniform_scale[5]          = 0.f;
    rotated_nonuniform_scale[10]         = 4.f;
    packet->mModelMatrix                 = rotated_nonuniform_scale;

    const auto parameters = materializeLegacyNormSpecWorldParameters(*packet, completeContext());
    ensure("rotated nonuniform scale materializes", parameters.has_value());
    const std::array<float, 9> expected_normal{ 0.f, 0.5f, 0.f, -1.f / 3.f, 0.f, 0.f, 0.f, 0.f, 0.25f };
    ensure("normal matrix is the inverse transpose", approximatelyEqual(parameters->mNormalMatrix, expected_normal));
    ensure("normal-matrix output validates", validMaterialParameters(*parameters));
}

template<>
template<>
void material_parameter_contract_test_object::test<4>()
{
    auto modern        = completePacket(legacyNormSpecModernHDRPipelineKey());
    auto compatibility = completePacket(legacyNormSpecCompatibilityPipelineKey());
    ensure("both production profile packets build", modern.has_value() && compatibility.has_value());

    modern->mAlphaCutoff                                  = 0.125f;
    LegacyNormSpecDrawPacket same_profile_alpha           = *modern;
    same_profile_alpha.mAlphaCutoff                       = 0.875f;
    compatibility->mAlphaCutoff                           = modern->mAlphaCutoff;
    LegacyNormSpecWorldParameterContext context           = completeContext();
    const auto                          modern_parameters = materializeLegacyNormSpecWorldParameters(*modern, context);
    const auto same_profile_alpha_parameters              = materializeLegacyNormSpecWorldParameters(same_profile_alpha, context);
    const auto compatibility_parameters                   = materializeLegacyNormSpecWorldParameters(*compatibility, context);
    ensure("both production profiles and alpha fixtures materialize",
           modern_parameters.has_value() && same_profile_alpha_parameters.has_value() && compatibility_parameters.has_value());
    ensure("unused alpha cutoff does not change the parameter block", *modern_parameters == *same_profile_alpha_parameters);
    ensure("production target profile does not change the parameter block", *modern_parameters == *compatibility_parameters);

    const MaterialParameters owned = *modern_parameters;
    modern->mModelMatrix.fill(7.f);
    modern->mDiffuseTextureMatrix.fill(-3.f);
    modern->mSpecularRGBA.fill(0.f);
    context.mBaseModelview.fill(9.f);
    context.mProjection.fill(-5.f);
    context.mEyeClipPlane.fill(4.f);
    ensure("source mutation cannot change returned parameters", *modern_parameters == owned);
}

template<>
template<>
void material_parameter_contract_test_object::test<5>()
{
    const auto diagnostic = completePacket(legacyNormSpecDiagnosticPipelineKey());
    const auto production = completePacket(legacyNormSpecModernHDRPipelineKey());
    ensure("profile rejection fixtures build", diagnostic.has_value() && production.has_value());
    ensure("the diagnostic profile is not a production World material packet",
           !materializeLegacyNormSpecWorldParameters(*diagnostic, completeContext()));

    LegacyNormSpecDrawPacket malformed = *production;
    malformed.mIndexCount              = 0;
    ensure("malformed draw ranges are rejected",
           !validLegacyNormSpecDrawPacket(malformed) && !materializeLegacyNormSpecWorldParameters(malformed, completeContext()));

    malformed                                = *production;
    malformed.mHandles.mPipeline.mGeneration = 0;
    ensure("malformed resource generations are rejected",
           !validLegacyNormSpecDrawPacket(malformed) && !materializeLegacyNormSpecWorldParameters(malformed, completeContext()));
}

template<>
template<>
void material_parameter_contract_test_object::test<6>()
{
    const auto packet = completePacket(legacyNormSpecModernHDRPipelineKey());
    ensure("identity rejection packet builds", packet.has_value());

    LegacyNormSpecWorldParameterContext context = completeContext();
    context.mFrame                              = 0;
    ensure("zero context frame is rejected", !materializeLegacyNormSpecWorldParameters(*packet, context));
    context       = completeContext();
    context.mPass = {};
    ensure("zero context pass is rejected", !materializeLegacyNormSpecWorldParameters(*packet, context));
    context = completeContext();
    context.mFrame += 1;
    ensure("mismatched frame is rejected", !materializeLegacyNormSpecWorldParameters(*packet, context));
    context = completeContext();
    context.mPass.mValue += 1;
    ensure("mismatched pass is rejected", !materializeLegacyNormSpecWorldParameters(*packet, context));

    LegacyNormSpecDrawPacket malformed = *packet;
    malformed.mFrame                   = 0;
    ensure("zero packet frame is rejected", !materializeLegacyNormSpecWorldParameters(malformed, completeContext()));
    malformed       = *packet;
    malformed.mPass = {};
    ensure("zero packet pass is rejected", !materializeLegacyNormSpecWorldParameters(malformed, completeContext()));
}

template<>
template<>
void material_parameter_contract_test_object::test<7>()
{
    const auto packet = completePacket(legacyNormSpecModernHDRPipelineKey());
    ensure("numeric rejection packet builds", packet.has_value());

    LegacyNormSpecWorldParameterContext context = completeContext();
    context.mBaseModelview[0]                   = std::numeric_limits<float>::quiet_NaN();
    ensure("non-finite base modelview is rejected", !materializeLegacyNormSpecWorldParameters(*packet, context));
    context                = completeContext();
    context.mProjection[5] = std::numeric_limits<float>::infinity();
    ensure("non-finite projection is rejected", !materializeLegacyNormSpecWorldParameters(*packet, context));
    context                  = completeContext();
    context.mEyeClipPlane[2] = -std::numeric_limits<float>::infinity();
    ensure("non-finite eye-space clip plane is rejected", !materializeLegacyNormSpecWorldParameters(*packet, context));

    LegacyNormSpecDrawPacket singular = *packet;
    singular.mModelMatrix[0]          = 0.f;
    ensure("finite singular modelview is rejected",
           validLegacyNormSpecDrawPacket(singular) && !materializeLegacyNormSpecWorldParameters(singular, completeContext()));

    // Row 2 is exactly row 0 plus four times row 1. Floating-point elimination
    // must not mistake its cancellation residue for an invertible pivot.
    singular.mModelMatrix = { 3.f, 14.f, 59.f, 0.f, -6.f, 12.f, 42.f, 0.f, -8.f, -18.f, -80.f, 0.f, -7.f, -20.f, -87.f, 1.f };
    ensure("dependent nonzero rows are rejected",
           validLegacyNormSpecDrawPacket(singular) && !materializeLegacyNormSpecWorldParameters(singular, completeContext()));

    LegacyNormSpecDrawPacket small_but_well_scaled = *packet;
    small_but_well_scaled.mModelMatrix[0]          = 1.e-20f;
    ensure("a small well-scaled invertible axis is accepted",
           materializeLegacyNormSpecWorldParameters(small_but_well_scaled, completeContext()).has_value());

    LegacyNormSpecDrawPacket large_translation = *packet;
    large_translation.mModelMatrix[0]          = 0.8f;
    large_translation.mModelMatrix[1]          = 0.6f;
    large_translation.mModelMatrix[4]          = -0.6f;
    large_translation.mModelMatrix[5]          = 0.8f;
    large_translation.mModelMatrix[12]         = 1'000'000.f;
    large_translation.mModelMatrix[13]         = -1'000'000.f;
    ensure("a rotated transform with a large finite translation is accepted",
           materializeLegacyNormSpecWorldParameters(large_translation, completeContext()).has_value());

    LegacyNormSpecDrawPacket projective = *packet;
    projective.mModelMatrix[3]          = 0.25f;
    ensure("a non-affine modelview is rejected",
           validLegacyNormSpecDrawPacket(projective) && !materializeLegacyNormSpecWorldParameters(projective, completeContext()));
}

template<>
template<>
void material_parameter_contract_test_object::test<8>()
{
    auto packet = completePacket(legacyNormSpecModernHDRPipelineKey());
    ensure("overflow packet builds", packet.has_value());
    packet->mModelMatrix[0] = 2.f;

    LegacyNormSpecWorldParameterContext context = completeContext();
    context.mBaseModelview[0]                   = std::numeric_limits<float>::max();
    ensure("finite modelview factors that overflow are rejected",
           std::isfinite(context.mBaseModelview[0]) && std::isfinite(packet->mModelMatrix[0]) &&
               !materializeLegacyNormSpecWorldParameters(*packet, context));

    context                = completeContext();
    context.mProjection[0] = std::numeric_limits<float>::max();
    ensure("finite projection factors that overflow are rejected",
           std::isfinite(context.mProjection[0]) && !materializeLegacyNormSpecWorldParameters(*packet, context));
}

} // namespace tut
