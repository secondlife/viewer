/**
 * @file lldrawpacketcontract_test.cpp
 * @brief Tests for the owned prepared draw packet.
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

#include "lldrawpacketcontract.h"
#include "lltut.h"

#include <limits>

namespace
{
using namespace LLRenderContract;

LegacyNormSpecDrawInputs completeInputs()
{
    LegacyNormSpecDrawInputs inputs;
    inputs.mFrame                    = 91;
    inputs.mPass                     = { 12 };
    inputs.mHandles                  = { BufferHandle{ 1, 7 }, BufferHandle{ 2, 9 }, PipelineHandle{ 6, 3 } };
    inputs.mDescriptors              = { { ImageHandle{ 3, 4 }, SamplerHandle{ 5, 2 }, { 0, 3, 0, 1 } },
                                         { ImageHandle{ 4, 5 }, SamplerHandle{ 7, 3 }, { 0, 3, 0, 1 } },
                                         { ImageHandle{ 4, 5 }, SamplerHandle{ 8, 6 }, { 1, 2, 0, 1 } } };
    inputs.mPipelineKey              = legacyNormSpecPipelineKey();
    inputs.mFirstIndex               = 18;
    inputs.mIndexCount               = 6;
    inputs.mMinVertex                = 11;
    inputs.mMaxVertex                = 14;
    inputs.mModelMatrix[12]          = 4.25f;
    inputs.mDiffuseTextureMatrix[0]  = 0.75f;
    inputs.mDiffuseTextureMatrix[13] = 0.125f;
    inputs.mSpecularRGBA             = { 0.2f, 0.4f, 0.8f, 0.6f };
    inputs.mEnvironmentIntensity     = 0.625f;
    inputs.mAlphaCutoff              = 0.375f;
    inputs.mEmissiveBrightness       = 1.f;
    return inputs;
}

} // namespace

namespace tut
{

struct draw_packet_contract_test
{
};

using draw_packet_contract_test_group  = test_group<draw_packet_contract_test>;
using draw_packet_contract_test_object = draw_packet_contract_test_group::object;
draw_packet_contract_test_group draw_packet_contract_tests("draw packet contract");

template<>
template<>
void draw_packet_contract_test_object::test<1>()
{
    const auto packet = buildLegacyNormSpecDrawPacket(completeInputs());
    ensure("complete packet builds", packet.has_value());
    ensure("complete packet validates", validLegacyNormSpecDrawPacket(*packet));
    ensure("frame and pass identities survive preparation", packet->mFrame == 91 && packet->mPass == PassId{ 12 });
    ensure("logical pipeline key is named and deterministic",
           packet->mPipelineKey == legacyNormSpecPipelineKey() && packet->mPipelineKey.mProgram.mName == "deferred.material.normspec" &&
               packet->mPipelineKey.mProgram.mVariant == 0);
    ensure("pipeline state is copied from explicit context",
           packet->mPipelineKey.mVertexLayout == DrawVertexLayout::LegacyMaterialNormSpec &&
               packet->mPipelineKey.mTopology == PrimitiveTopology::TriangleList && packet->mPipelineKey.mDepthTestEnabled &&
               packet->mPipelineKey.mDepthWriteEnabled && packet->mPipelineKey.mColorTargets.size() == 3 &&
               packet->mPipelineKey.mColorTargets[2].mFormat == PixelFormat::RGBA16Unorm &&
               packet->mPipelineKey.mColorTargets[2].mWriteMask == 0xf && packet->mPipelineKey.mDepthFormat == PixelFormat::Depth24Unorm);
    ensure("indexed draw range maps without changing units",
           packet->mFirstIndex == 18 && packet->mIndexCount == 6 && packet->mMinVertex == 11 && packet->mMaxVertex == 14);
    ensure("texture roles may resolve to the same image generation",
           packet->mDescriptors.mNormal.mImage == packet->mDescriptors.mSpecular.mImage &&
               packet->mDescriptors.mNormal.mSampler != packet->mDescriptors.mSpecular.mSampler &&
               packet->mDescriptors.mSpecular.mRange.mBaseMipLevel == 1);
    ensure("prepared constants retain their copied values",
           packet->mModelMatrix[12] == 4.25f && packet->mDiffuseTextureMatrix[13] == 0.125f && packet->mSpecularRGBA[2] == 0.8f &&
               packet->mEnvironmentIntensity == 0.625f && packet->mAlphaCutoff == 0.375f && packet->mEmissiveBrightness == 1.f);
}

template<>
template<>
void draw_packet_contract_test_object::test<2>()
{
    LegacyNormSpecDrawInputs inputs = completeInputs();
    const auto               packet = buildLegacyNormSpecDrawPacket(inputs);
    ensure("baseline packet builds", packet.has_value());
    const LegacyNormSpecDrawPacket owned = *packet;

    inputs.mDescriptors.mDiffuse.mImage.mGeneration++;
    inputs.mModelMatrix.fill(-7.f);
    inputs.mDiffuseTextureMatrix.fill(9.f);
    inputs.mSpecularRGBA.fill(0.f);
    inputs.mEnvironmentIntensity = 0.f;
    inputs.mAlphaCutoff          = 1.f;
    inputs.mEmissiveBrightness   = 0.f;

    ensure("source mutation cannot change the prepared packet", *packet == owned);
    LegacyNormSpecDrawPacket changed = owned;
    changed.mMaxVertex++;
    ensure("packet equality observes draw data", changed != owned);
    changed                             = owned;
    changed.mPipelineKey.mProgram.mName = "viewer.deferred.legacy-material.other";
    ensure("validator rejects a different logical program", !validLegacyNormSpecDrawPacket(changed));
}

template<>
template<>
void draw_packet_contract_test_object::test<3>()
{
    LegacyNormSpecDrawInputs invalid = completeInputs();
    invalid.mFrame                   = 0;
    ensure("zero frame identities are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid       = completeInputs();
    invalid.mPass = {};
    ensure("zero pass identities are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                                    = completeInputs();
    invalid.mHandles.mVertexBuffer.mGeneration = 0;
    ensure("zero handle generations are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                       = completeInputs();
    invalid.mHandles.mIndexBuffer = invalid.mHandles.mVertexBuffer;
    ensure("vertex and index streams require distinct handles", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                                           = completeInputs();
    invalid.mDescriptors.mNormal.mSampler.mGeneration = 0;
    ensure("every descriptor owns a valid sampler handle", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                                              = completeInputs();
    invalid.mDescriptors.mSpecular.mRange.mBaseMipLevel  = std::numeric_limits<std::uint32_t>::max();
    invalid.mDescriptors.mSpecular.mRange.mMipLevelCount = 1;
    ensure("overflowing descriptor ranges are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid             = completeInputs();
    invalid.mIndexCount = 0;
    ensure("empty index ranges are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid             = completeInputs();
    invalid.mFirstIndex = std::numeric_limits<std::uint32_t>::max() - 1;
    invalid.mIndexCount = 3;
    ensure("overflowing index ranges are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid            = completeInputs();
    invalid.mMinVertex = 15;
    invalid.mMaxVertex = 14;
    ensure("inverted vertex ranges are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid            = completeInputs();
    invalid.mIndexType = static_cast<IndexType>(255);
    ensure("unknown index types are rejected", !buildLegacyNormSpecDrawPacket(invalid));
}

template<>
template<>
void draw_packet_contract_test_object::test<4>()
{
    LegacyNormSpecDrawInputs invalid = completeInputs();
    invalid.mModelMatrix[0]          = std::numeric_limits<float>::infinity();
    ensure("non-finite matrices are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                  = completeInputs();
    invalid.mSpecularRGBA[2] = 1.01f;
    ensure("specular components outside unit range are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                       = completeInputs();
    invalid.mEnvironmentIntensity = -0.01f;
    ensure("environment intensity outside unit range is rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid              = completeInputs();
    invalid.mAlphaCutoff = 1.01f;
    ensure("alpha cutoff outside unit range is rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                     = completeInputs();
    invalid.mEmissiveBrightness = 0.5f;
    ensure("emissive brightness must encode fullbright as zero or one", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                            = completeInputs();
    invalid.mPipelineKey.mVertexLayout = static_cast<DrawVertexLayout>(255);
    ensure("unknown vertex layouts are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                       = completeInputs();
    invalid.mPipelineKey.mSamples = 3;
    ensure("non-canonical sample counts are rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                        = completeInputs();
    invalid.mPipelineKey.mCullMode = CullMode::Disabled;
    ensure("non-canonical culling is rejected", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                                 = completeInputs();
    invalid.mPipelineKey.mDepthTestEnabled  = false;
    invalid.mPipelineKey.mDepthWriteEnabled = true;
    ensure("depth writes require depth testing", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                                             = completeInputs();
    invalid.mPipelineKey.mColorTargets[0].mBlendEnabled = true;
    ensure("opaque color targets reject blending", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                                          = completeInputs();
    invalid.mPipelineKey.mColorTargets[0].mWriteMask = 0x7;
    ensure("normspec color targets require full writes", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                                       = completeInputs();
    invalid.mPipelineKey.mColorTargets[1].mFormat = PixelFormat::RGBA16Unorm;
    ensure("normspec color target order is canonical", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                                       = completeInputs();
    invalid.mPipelineKey.mColorTargets[0].mFormat = PixelFormat::Depth24Unorm;
    ensure("depth formats cannot be color targets", !buildLegacyNormSpecDrawPacket(invalid));

    invalid                           = completeInputs();
    invalid.mPipelineKey.mDepthFormat = PixelFormat::RGBA8Unorm;
    ensure("color formats cannot be depth targets", !buildLegacyNormSpecDrawPacket(invalid));
}

} // namespace tut
