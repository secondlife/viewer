/**
 * @file lldrawinfotranslator_test.cpp
 * @brief Tests for the immutable LLDrawInfo translation boundary.
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

#include "lldrawinfotranslator.h"

#include "lldrawpool.h"
#include "llmaterial.h"
#include "llspatialpartition.h"
#include "lltut.h"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>

namespace
{
using namespace LLDrawInfoAdapter;
using namespace LLRenderContract;

constexpr U32 REQUIRED_VERTEX_MASK = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0 |
                                     LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2 | LLVertexBuffer::MAP_COLOR |
                                     LLVertexBuffer::MAP_TANGENT;

constexpr std::size_t roleIndex(TextureRole role)
{
    return static_cast<std::size_t>(role);
}

std::uint64_t vertexBufferSize(U32 mask, U32 count)
{
    std::array<U32, LLVertexBuffer::TYPE_MAX> offsets{};
    return LLVertexBuffer::calcOffsets(mask, offsets.data(), count);
}

class TestResolver final : public Resolver
{
public:
    std::optional<ResolvedGeometry> resolveGeometry(const LLVertexBuffer& buffer) const override
    {
        if (!mResolveGeometry || !mGeometryLive || mGeometrySource != &buffer || mGeometry.mVertexBuffer != mCurrentVertexBuffer ||
            mGeometry.mIndexBuffer != mCurrentIndexBuffer)
        {
            return std::nullopt;
        }
        return mGeometry;
    }

    std::optional<DrawTextureInput> resolveImage(const LLViewerTexture& texture, TextureRole role) const override
    {
        const std::size_t index = roleIndex(role);
        if (!mResolveImages[index] || !mImagesLive[index] || mImageSources[index] != &texture ||
            mImages[index].mImage != mCurrentImages[index] || mImages[index].mSampler != mCurrentSamplers[index])
        {
            return std::nullopt;
        }
        return mImages[index];
    }

    std::optional<PipelineHandle> resolvePipeline(const LegacyNormSpecPipelineKey& key) const override
    {
        if (!mResolvePipeline || !mPipelineLive || key != mExpectedPipelineKey || mPipeline != mCurrentPipeline)
        {
            return std::nullopt;
        }
        return mPipeline;
    }

    const LLVertexBuffer*                 mGeometrySource = nullptr;
    std::array<const LLViewerTexture*, 3> mImageSources{};
    ResolvedGeometry                      mGeometry;
    std::array<DrawTextureInput, 3>       mImages{};
    BufferHandle                          mCurrentVertexBuffer{ 1, 7 };
    BufferHandle                          mCurrentIndexBuffer{ 2, 9 };
    std::array<ImageHandle, 3>            mCurrentImages{ ImageHandle{ 3, 4 }, ImageHandle{ 4, 5 }, ImageHandle{ 8, 6 } };
    std::array<SamplerHandle, 3>          mCurrentSamplers{ SamplerHandle{ 5, 2 }, SamplerHandle{ 7, 3 }, SamplerHandle{ 9, 4 } };
    LegacyNormSpecPipelineKey             mExpectedPipelineKey;
    PipelineHandle                        mPipeline{ 6, 3 };
    PipelineHandle                        mCurrentPipeline{ 6, 3 };
    bool                                  mResolveGeometry = true;
    bool                                  mGeometryLive    = true;
    std::array<bool, 3>                   mResolveImages{ true, true, true };
    std::array<bool, 3>                   mImagesLive{ true, true, true };
    bool                                  mResolvePipeline = true;
    bool                                  mPipelineLive    = true;
};

template<typename T>
class ScopedNonOwningPointer final : private LLPointer<T>
{
public:
    ScopedNonOwningPointer(LLPointer<T>& target, std::uintptr_t address) : mTarget(target)
    {
        llassert(mTarget.isNull());
        this->mPointer = reinterpret_cast<T*>(address);
        LLPointer<T>::swap(mTarget, *this);
    }

    ~ScopedNonOwningPointer()
    {
        LLPointer<T>::swap(mTarget, *this);
        this->mPointer = nullptr;
    }

    ScopedNonOwningPointer(const ScopedNonOwningPointer&)            = delete;
    ScopedNonOwningPointer& operator=(const ScopedNonOwningPointer&) = delete;

private:
    LLPointer<T>& mTarget;
};

struct DrawInfoFixture
{
    DrawInfoFixture()
    {
        gDebugGL = false;

        mBuffer   = new LLVertexBuffer(REQUIRED_VERTEX_MASK);
        mDiffuse  = new LLViewerTexture(false);
        mNormal   = new LLViewerTexture(false);
        mSpecular = new LLViewerTexture(false);
        mMaterial = new LLMaterial();
        mDraw     = new LLDrawInfo(11, 14, 6, 18, mDiffuse, mBuffer, true);

        mDraw->mNormalMap       = mNormal;
        mDraw->mSpecularMap     = mSpecular;
        mDraw->mMaterial        = mMaterial;
        mDraw->mModelMatrix     = &mModelMatrix;
        mDraw->mTextureMatrix   = &mTextureMatrix;
        mDraw->mSpecColor       = LLVector4(0.2f, 0.4f, 0.8f, 0.6f);
        mDraw->mEnvIntensity    = 0.625f;
        mDraw->mAlphaMaskCutoff = 0.375f;

        mModelMatrix.mMatrix[3][0]   = 4.25f;
        mTextureMatrix.mMatrix[0][0] = 0.75f;
        mTextureMatrix.mMatrix[3][1] = 0.125f;

        mContext.mFrame        = 91;
        mContext.mPass         = { 73 };
        mContext.mPipelineKey  = legacyNormSpecModernHDRPipelineKey();
        mContext.mRenderDomain = RenderDomain::World;
        mContext.mSubmission   = SubmissionKind::DeferredMaterial;

        mResolver.mGeometrySource = mBuffer;
        mResolver.mGeometry       = {
            BufferHandle{ 1, 7 }, BufferHandle{ 2, 9 }, vertexBufferSize(REQUIRED_VERTEX_MASK, 65), sizeof(std::uint16_t) * 65, 65, 65,
            IndexType::UInt16
        };
        mResolver.mImageSources        = { mDiffuse, mNormal, mSpecular };
        mResolver.mImages              = { DrawTextureInput{ ImageHandle{ 3, 4 }, SamplerHandle{ 5, 2 }, { 0, 3, 0, 1 } },
                                           DrawTextureInput{ ImageHandle{ 4, 5 }, SamplerHandle{ 7, 3 }, { 0, 2, 0, 1 } },
                                           DrawTextureInput{ ImageHandle{ 8, 6 }, SamplerHandle{ 9, 4 }, { 1, 2, 0, 1 } } };
        mResolver.mExpectedPipelineKey = mContext.mPipelineKey;
    }

    std::optional<LegacyNormSpecDrawPacket> translate() const
    {
        return translateNonRiggedNormSpecDraw(*mDraw, LLRenderPass::PASS_NORMSPEC, mContext, mResolver);
    }

    LLPointer<LLVertexBuffer>  mBuffer;
    LLPointer<LLViewerTexture> mDiffuse;
    LLPointer<LLViewerTexture> mNormal;
    LLPointer<LLViewerTexture> mSpecular;
    LLPointer<LLMaterial>      mMaterial;
    LLPointer<LLDrawInfo>      mDraw;
    LLMatrix4                  mModelMatrix;
    LLMatrix4                  mTextureMatrix;
    Context                    mContext;
    TestResolver               mResolver;
};

} // namespace

namespace tut
{

struct draw_info_translator_test
{
};

using draw_info_translator_test_group  = test_group<draw_info_translator_test>;
using draw_info_translator_test_object = draw_info_translator_test_group::object;
draw_info_translator_test_group draw_info_translator_tests("draw info translator");

template<>
template<>
void draw_info_translator_test_object::test<1>()
{
    DrawInfoFixture fixture;

    const auto packet = fixture.translate();
    ensure("canonical legacy draw translates", packet.has_value());
    ensure("translated packet validates", validLegacyNormSpecDrawPacket(*packet));
    ensure("frame and pass map exactly", packet->mFrame == 91 && packet->mPass == PassId{ 73 });
    ensure("indexed ranges retain element units",
           packet->mFirstIndex == 18 && packet->mIndexCount == 6 && packet->mMinVertex == 11 && packet->mMaxVertex == 14);
    ensure("geometry handles map exactly",
           packet->mHandles.mVertexBuffer == BufferHandle{ 1, 7 } && packet->mHandles.mIndexBuffer == BufferHandle{ 2, 9 });
    ensure("texture records map by role",
           packet->mDescriptors.mDiffuse == fixture.mResolver.mImages[roleIndex(TextureRole::Diffuse)] &&
               packet->mDescriptors.mNormal == fixture.mResolver.mImages[roleIndex(TextureRole::Normal)] &&
               packet->mDescriptors.mSpecular == fixture.mResolver.mImages[roleIndex(TextureRole::Specular)]);
    ensure("model and texture matrices map in storage order",
           packet->mModelMatrix[12] == 4.25f && packet->mDiffuseTextureMatrix[0] == 0.75f && packet->mDiffuseTextureMatrix[13] == 0.125f);
    ensure("shader constants map exactly",
           packet->mSpecularRGBA == std::array<float, 4>{ 0.2f, 0.4f, 0.8f, 0.6f } && packet->mEnvironmentIntensity == 0.625f &&
               packet->mAlphaCutoff == 0.375f && packet->mEmissiveBrightness == 1.f);
    ensure("the complete logical pipeline key is the modern production profile",
           packet->mPipelineKey == legacyNormSpecModernHDRPipelineKey());

    const auto repeated = fixture.translate();
    ensure("translation is deterministic", repeated.has_value() && *repeated == *packet);

    fixture.mResolver.mGeometry.mIndexType       = IndexType::UInt32;
    fixture.mResolver.mGeometry.mIndexBufferSize = sizeof(std::uint32_t) * fixture.mResolver.mGeometry.mIndexCount;
    const auto uint32_packet                     = fixture.translate();
    ensure("UInt32 index geometry translates with four-byte storage",
           uint32_packet.has_value() && uint32_packet->mIndexType == IndexType::UInt32);
}

template<>
template<>
void draw_info_translator_test_object::test<2>()
{
    DrawInfoFixture fixture;
    fixture.mDraw->mModelMatrix   = nullptr;
    fixture.mDraw->mTextureMatrix = nullptr;

    const auto packet = fixture.translate();
    ensure("draw without optional matrices translates", packet.has_value());
    ensure("null model matrix becomes identity", packet->mModelMatrix == DRAW_IDENTITY_MATRIX4);
    ensure("null texture matrix becomes identity", packet->mDiffuseTextureMatrix == DRAW_IDENTITY_MATRIX4);
}

template<>
template<>
void draw_info_translator_test_object::test<3>()
{
    DrawInfoFixture fixture;
    const auto      packet = fixture.translate();
    ensure("baseline packet translates", packet.has_value());
    const LegacyNormSpecDrawPacket owned = *packet;

    fixture.mResolver.mGeometry.mVertexBuffer.mGeneration = 17;
    fixture.mResolver.mGeometry.mIndexBuffer.mGeneration  = 19;
    for (DrawTextureInput& image : fixture.mResolver.mImages)
    {
        image.mImage.mGeneration += 10;
        image.mSampler.mGeneration += 10;
    }
    fixture.mResolver.mPipeline.mGeneration = 23;
    ensure("resolver generation changes cannot change the captured packet", *packet == owned);
    ensure("packet remains valid after resolver generation changes", validLegacyNormSpecDrawPacket(*packet));

    fixture.mDraw->mStart                = 1;
    fixture.mDraw->mEnd                  = 2;
    fixture.mDraw->mCount                = 3;
    fixture.mDraw->mOffset               = 4;
    fixture.mDraw->mSpecColor            = LLVector4(0.f, 0.f, 0.f, 0.f);
    fixture.mDraw->mEnvIntensity         = 0.f;
    fixture.mDraw->mAlphaMaskCutoff      = 1.f;
    fixture.mDraw->mFullbright           = false;
    fixture.mModelMatrix.mMatrix[3][0]   = -19.f;
    fixture.mTextureMatrix.mMatrix[3][1] = -23.f;

    fixture.mDraw     = nullptr;
    fixture.mMaterial = nullptr;
    fixture.mDiffuse  = nullptr;
    fixture.mNormal   = nullptr;
    fixture.mSpecular = nullptr;
    fixture.mBuffer   = nullptr;

    ensure("source mutation and release cannot change the packet", *packet == owned);
    ensure("released-source packet remains valid", validLegacyNormSpecDrawPacket(*packet));
}

template<>
template<>
void draw_info_translator_test_object::test<4>()
{
    DrawInfoFixture fixture;

    ensure("only PASS_NORMSPEC is accepted",
           !translateNonRiggedNormSpecDraw(*fixture.mDraw, LLRenderPass::PASS_NORMSPEC_RIGGED, fixture.mContext, fixture.mResolver));

    fixture.mContext.mSubmission = SubmissionKind::Invalid;
    ensure("the deferred-material submission is required", !fixture.translate());
    fixture.mContext.mSubmission = SubmissionKind::DeferredMaterial;

    for (RenderDomain domain :
         { RenderDomain::Invalid, RenderDomain::HUD, RenderDomain::Impostor, RenderDomain::Reflection, RenderDomain::Cube })
    {
        fixture.mContext.mRenderDomain = domain;
        ensure("only ordinary world rendering is accepted", !fixture.translate());
    }
    fixture.mContext.mRenderDomain = RenderDomain::World;

    fixture.mContext.mFrame = 0;
    ensure("a zero frame identity is rejected", !fixture.translate());
    fixture.mContext.mFrame = 91;

    fixture.mContext.mPass = {};
    ensure("a zero pass identity is rejected", !fixture.translate());
    fixture.mContext.mPass = { 73 };

    fixture.mContext.mPipelineKey.mCullMode = CullMode::Disabled;
    ensure("a non-canonical logical pipeline key is rejected", !fixture.translate());
    fixture.mContext.mPipelineKey = legacyNormSpecModernHDRPipelineKey();

    fixture.mContext.mPipelineKey          = legacyNormSpecCompatibilityPipelineKey();
    fixture.mResolver.mExpectedPipelineKey = fixture.mContext.mPipelineKey;
    ensure("the compatibility production target profile is accepted", fixture.translate().has_value());

    fixture.mContext.mPipelineKey          = legacyNormSpecDiagnosticPipelineKey();
    fixture.mResolver.mExpectedPipelineKey = fixture.mContext.mPipelineKey;
    ensure("the three-target diagnostic profile cannot masquerade as production", !fixture.translate());

    fixture.mContext.mPipelineKey          = legacyNormSpecModernHDRPipelineKey();
    fixture.mResolver.mExpectedPipelineKey = fixture.mContext.mPipelineKey;

    LLPointer<LLMaterial> material = fixture.mDraw->mMaterial;
    fixture.mDraw->mMaterial       = nullptr;
    ensure("the legacy material marker is required", !fixture.translate());
    fixture.mDraw->mMaterial = material;

    LLPointer<LLViewerTexture> diffuse = fixture.mDraw->mTexture;
    fixture.mDraw->mTexture            = nullptr;
    ensure("the diffuse texture is required", !fixture.translate());
    fixture.mDraw->mTexture = diffuse;

    LLPointer<LLViewerTexture> normal = fixture.mDraw->mNormalMap;
    fixture.mDraw->mNormalMap         = nullptr;
    ensure("the normal texture is required", !fixture.translate());
    fixture.mDraw->mNormalMap = normal;

    LLPointer<LLViewerTexture> specular = fixture.mDraw->mSpecularMap;
    fixture.mDraw->mSpecularMap         = nullptr;
    ensure("the specular texture is required", !fixture.translate());
    fixture.mDraw->mSpecularMap = specular;

    LLPointer<LLVertexBuffer> buffer = fixture.mDraw->mVertexBuffer;
    fixture.mDraw->mVertexBuffer     = nullptr;
    ensure("the source vertex buffer is required", !fixture.translate());
    fixture.mDraw->mVertexBuffer = buffer;
}

template<>
template<>
void draw_info_translator_test_object::test<5>()
{
    DrawInfoFixture fixture;

    {
        ScopedNonOwningPointer<LLFetchedGLTFMaterial> gltf(fixture.mDraw->mGLTFMaterial, 0x1000);
        ensure("GLTF material draws are outside this translation shape", !fixture.translate());
    }
    {
        ScopedNonOwningPointer<LLVOAvatar> avatar(fixture.mDraw->mAvatar, 0x2000);
        ensure("avatar draws are outside this translation shape", !fixture.translate());
    }

    fixture.mDraw->mSkinInfo = reinterpret_cast<LLMeshSkinInfo*>(std::uintptr_t{ 0x3000 });
    ensure("skinned draws are outside this translation shape", !fixture.translate());
    fixture.mDraw->mSkinInfo = nullptr;

    fixture.mDraw->mHasGlow = true;
    ensure("glow draws are outside this translation shape", !fixture.translate());
    fixture.mDraw->mHasGlow = false;

    LLMatrix4 auxiliary_matrix;
    fixture.mDraw->mNormalMapMatrix = &auxiliary_matrix;
    ensure("normal-map matrices are outside this translation shape", !fixture.translate());
    fixture.mDraw->mNormalMapMatrix = nullptr;

    fixture.mDraw->mSpecularMapMatrix = &auxiliary_matrix;
    ensure("specular-map matrices are outside this translation shape", !fixture.translate());
    fixture.mDraw->mSpecularMapMatrix = nullptr;

    fixture.mDraw->mTextureList.push_back(fixture.mDiffuse);
    ensure("a repeated diffuse texture-list entry remains supported", fixture.translate().has_value());
    fixture.mDraw->mTextureList.clear();

    LLPointer<LLViewerTexture> distinct_texture = new LLViewerTexture(false);
    fixture.mDraw->mTextureList.push_back(distinct_texture);
    ensure("a distinct batched texture-list entry is rejected", !fixture.translate());
    fixture.mDraw->mTextureList.clear();

    LLPointer<LLVertexBuffer> incomplete = new LLVertexBuffer(REQUIRED_VERTEX_MASK & ~LLVertexBuffer::MAP_TANGENT);
    LLPointer<LLVertexBuffer> complete   = fixture.mDraw->mVertexBuffer;
    fixture.mDraw->mVertexBuffer         = incomplete;
    ensure("missing required vertex attributes are rejected", !fixture.translate());
    fixture.mDraw->mVertexBuffer = complete;
}

template<>
template<>
void draw_info_translator_test_object::test<6>()
{
    DrawInfoFixture fixture;

    fixture.mDraw->mCount = 0;
    ensure("empty index ranges are rejected", !fixture.translate());
    fixture.mDraw->mCount = 6;

    fixture.mDraw->mStart = 15;
    fixture.mDraw->mEnd   = 14;
    ensure("inverted vertex ranges are rejected", !fixture.translate());
    fixture.mDraw->mStart = 11;
    fixture.mDraw->mEnd   = 14;

    fixture.mDraw->mEnd = 65;
    ensure("vertex ranges outside the resolved record are rejected", !fixture.translate());
    fixture.mDraw->mEnd = 14;

    fixture.mDraw->mOffset = 64;
    fixture.mDraw->mCount  = 2;
    ensure("index ranges outside the resolved record are rejected", !fixture.translate());
    fixture.mDraw->mOffset = 18;
    fixture.mDraw->mCount  = 6;

    const BufferHandle index_buffer          = fixture.mResolver.mGeometry.mIndexBuffer;
    fixture.mResolver.mGeometry.mIndexBuffer = fixture.mResolver.mGeometry.mVertexBuffer;
    fixture.mResolver.mCurrentIndexBuffer    = fixture.mResolver.mCurrentVertexBuffer;
    ensure("one handle cannot alias the vertex and index streams", !fixture.translate());
    fixture.mResolver.mGeometry.mIndexBuffer = index_buffer;
    fixture.mResolver.mCurrentIndexBuffer    = index_buffer;

    const std::uint64_t vertex_bytes              = fixture.mResolver.mGeometry.mVertexBufferSize;
    fixture.mResolver.mGeometry.mVertexBufferSize = vertex_bytes - 1;
    ensure("undersized resolved vertex storage is rejected", !fixture.translate());
    fixture.mResolver.mGeometry.mVertexBufferSize = vertex_bytes;

    const std::uint64_t index_bytes              = fixture.mResolver.mGeometry.mIndexBufferSize;
    fixture.mResolver.mGeometry.mIndexBufferSize = index_bytes - 1;
    ensure("undersized resolved index storage is rejected", !fixture.translate());
    fixture.mResolver.mGeometry.mIndexBufferSize = index_bytes;

    fixture.mModelMatrix.mMatrix[0][0] = std::numeric_limits<float>::infinity();
    ensure("non-finite model constants are rejected", !fixture.translate());
    fixture.mModelMatrix.mMatrix[0][0] = 1.f;

    fixture.mTextureMatrix.mMatrix[0][1] = std::numeric_limits<float>::infinity();
    ensure("non-finite texture constants are rejected", !fixture.translate());
    fixture.mTextureMatrix.mMatrix[0][1] = 0.f;

    fixture.mDraw->mSpecColor.mV[2] = 1.01f;
    ensure("out-of-range specular constants are rejected", !fixture.translate());
    fixture.mDraw->mSpecColor.mV[2] = 0.8f;

    fixture.mDraw->mEnvIntensity = -0.01f;
    ensure("out-of-range environment constants are rejected", !fixture.translate());
    fixture.mDraw->mEnvIntensity = 0.625f;

    fixture.mDraw->mAlphaMaskCutoff = 1.01f;
    ensure("out-of-range alpha constants are rejected", !fixture.translate());
    fixture.mDraw->mAlphaMaskCutoff = 0.375f;

    fixture.mResolver.mResolveGeometry = false;
    ensure("a missing geometry resolution is rejected", !fixture.translate());
    fixture.mResolver.mResolveGeometry = true;

    fixture.mResolver.mResolveImages[roleIndex(TextureRole::Normal)] = false;
    ensure("a missing image resolution is rejected", !fixture.translate());
    fixture.mResolver.mResolveImages[roleIndex(TextureRole::Normal)] = true;

    fixture.mResolver.mResolvePipeline = false;
    ensure("a missing pipeline resolution is rejected", !fixture.translate());
    fixture.mResolver.mResolvePipeline = true;

    fixture.mResolver.mGeometryLive = false;
    ensure("retired geometry returns no resolution", !fixture.translate());
    fixture.mResolver.mGeometryLive = true;

    fixture.mResolver.mImagesLive[roleIndex(TextureRole::Normal)] = false;
    ensure("a retired image returns no resolution", !fixture.translate());
    fixture.mResolver.mImagesLive[roleIndex(TextureRole::Normal)] = true;

    fixture.mResolver.mPipelineLive = false;
    ensure("a retired pipeline returns no resolution", !fixture.translate());
    fixture.mResolver.mPipelineLive = true;

    fixture.mResolver.mCurrentVertexBuffer.mGeneration = 8;
    ensure("a nonzero stale buffer generation returns no resolution", !fixture.translate());
    fixture.mResolver.mCurrentVertexBuffer.mGeneration = 7;

    fixture.mResolver.mCurrentImages[roleIndex(TextureRole::Diffuse)].mGeneration = 5;
    ensure("a nonzero stale image generation returns no resolution", !fixture.translate());
    fixture.mResolver.mCurrentImages[roleIndex(TextureRole::Diffuse)].mGeneration = 4;

    fixture.mResolver.mCurrentSamplers[roleIndex(TextureRole::Normal)].mGeneration = 4;
    ensure("a nonzero stale sampler generation returns no resolution", !fixture.translate());
    fixture.mResolver.mCurrentSamplers[roleIndex(TextureRole::Normal)].mGeneration = 3;

    fixture.mResolver.mCurrentPipeline.mGeneration = 4;
    ensure("a nonzero stale pipeline generation returns no resolution", !fixture.translate());
    fixture.mResolver.mCurrentPipeline.mGeneration = 3;

    fixture.mResolver.mGeometry.mVertexBuffer.mGeneration = 0;
    ensure("a zero-generation geometry record is rejected", !fixture.translate());
    fixture.mResolver.mGeometry.mVertexBuffer.mGeneration = 7;

    DrawTextureInput& diffuse  = fixture.mResolver.mImages[roleIndex(TextureRole::Diffuse)];
    diffuse.mImage.mGeneration = 0;
    ensure("a zero-generation image record is rejected", !fixture.translate());
    diffuse.mImage.mGeneration = 4;

    fixture.mResolver.mPipeline.mGeneration = 0;
    ensure("a zero-generation pipeline record is rejected", !fixture.translate());
    fixture.mResolver.mPipeline.mGeneration = 3;

    DrawTextureInput& specular     = fixture.mResolver.mImages[roleIndex(TextureRole::Specular)];
    specular.mRange.mMipLevelCount = 0;
    ensure("an invalid resolved image range is rejected", !fixture.translate());
    specular.mRange.mMipLevelCount = 2;
}

template<>
template<>
void draw_info_translator_test_object::test<7>()
{
    DrawInfoFixture fixture;
    fixture.mDraw->mNormalMap   = fixture.mDiffuse;
    fixture.mDraw->mSpecularMap = fixture.mDiffuse;
    fixture.mResolver.mImageSources.fill(fixture.mDiffuse);

    const ImageHandle shared_image{ 3, 4 };
    fixture.mResolver.mImages[roleIndex(TextureRole::Normal)].mImage   = shared_image;
    fixture.mResolver.mImages[roleIndex(TextureRole::Specular)].mImage = shared_image;
    fixture.mResolver.mCurrentImages[roleIndex(TextureRole::Normal)]   = shared_image;
    fixture.mResolver.mCurrentImages[roleIndex(TextureRole::Specular)] = shared_image;

    const auto packet = fixture.translate();
    ensure("one source texture may fill all three roles", packet.has_value());
    ensure("reused source keeps role-specific samplers",
           packet->mDescriptors.mDiffuse.mSampler != packet->mDescriptors.mNormal.mSampler &&
               packet->mDescriptors.mNormal.mSampler != packet->mDescriptors.mSpecular.mSampler);
    ensure("reused source resolves to one stable image identity",
           packet->mDescriptors.mDiffuse.mImage == packet->mDescriptors.mNormal.mImage &&
               packet->mDescriptors.mNormal.mImage == packet->mDescriptors.mSpecular.mImage);

    const ImageHandle inconsistent_image{ 99, 1 };
    fixture.mResolver.mImages[roleIndex(TextureRole::Specular)].mImage = inconsistent_image;
    fixture.mResolver.mCurrentImages[roleIndex(TextureRole::Specular)] = inconsistent_image;
    ensure("one source texture cannot resolve to inconsistent image identities", !fixture.translate());
}

} // namespace tut
