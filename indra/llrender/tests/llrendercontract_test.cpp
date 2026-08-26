/**
 * @file llrendercontract_test.cpp
 * @brief Tests for the API-neutral renderer contract.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llrendercontract.h"
#include "lltonemapcontract.h"
#include "lltut.h"

#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>

namespace
{
using namespace LLRenderContract;

constexpr ImageHandle SCENE_IMAGE{ 1, 1 };
constexpr ImageHandle EXPOSURE_IMAGE{ 2, 1 };
constexpr ImageHandle OUTPUT_IMAGE{ 3, 1 };
constexpr ImageHandle DIFFUSE_IMAGE{ 4, 1 };
constexpr ImageHandle NORMAL_IMAGE{ 5, 1 };
constexpr ImageHandle SPECULAR_IMAGE{ 6, 1 };
constexpr ImageHandle GBUFFER0_IMAGE{ 7, 1 };
constexpr ImageHandle GBUFFER1_IMAGE{ 8, 1 };
constexpr ImageHandle GBUFFER2_IMAGE{ 9, 1 };
constexpr ImageHandle DEPTH_IMAGE{ 10, 1 };
constexpr ImageHandle OLD_STREAMED_IMAGE{ 11, 1 };
constexpr ImageHandle STREAMED_IMAGE{ 11, 2 };

constexpr BufferHandle VERTEX_BUFFER{ 1, 1 };
constexpr BufferHandle INDEX_BUFFER{ 2, 1 };
constexpr BufferHandle SCREEN_TRIANGLE_BUFFER{ 3, 1 };

constexpr SamplerHandle POINT_SAMPLER{ 1, 1 };
constexpr SamplerHandle LINEAR_SAMPLER{ 2, 1 };

constexpr PipelineHandle TONEMAP_PIPELINE{ 1, 1 };
constexpr PipelineHandle MATERIAL_PIPELINE{ 2, 1 };
constexpr PipelineHandle TEXTURE_PIPELINE{ 3, 1 };

ByteRange bytes(std::size_t size)
{
    auto storage = std::make_shared<std::vector<std::uint8_t>>(size, 0);
    return { storage, 0, size };
}

ImageResource image(ImageHandle handle, std::uint32_t width, std::uint32_t height, PixelFormat format,
                    ResourceLifetime lifetime = ResourceLifetime::Persistent, std::uint32_t mip_levels = 1)
{
    ImageResource resource;
    resource.mHandle    = handle;
    resource.mExtent    = { width, height };
    resource.mFormat    = format;
    resource.mLifetime  = lifetime;
    resource.mMipLevels = mip_levels;
    return resource;
}

SamplerResource sampler(SamplerHandle handle, Filter filter, MipFilter mip_filter = MipFilter::None, float max_anisotropy = 1.f,
                        AddressMode address = AddressMode::Clamp)
{
    SamplerResource resource;
    resource.mHandle        = handle;
    resource.mMinFilter     = filter;
    resource.mMagFilter     = filter;
    resource.mMipFilter     = mip_filter;
    resource.mMaxAnisotropy = max_anisotropy;
    resource.mAddressU      = address;
    resource.mAddressV      = address;
    return resource;
}

RenderPass pass(PassId id, std::uint32_t width, std::uint32_t height)
{
    RenderPass result;
    result.mId       = id;
    result.mExtent   = { width, height };
    result.mViewport = { 0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f };
    result.mScissor  = { 0, 0, width, height };
    return result;
}

FrameSnapshot fullScreenFrame()
{
    TonemapInputs inputs;
    inputs.mFrame = 17;
    inputs.mHandles = { SCREEN_TRIANGLE_BUFFER, SCENE_IMAGE, EXPOSURE_IMAGE, OUTPUT_IMAGE,
                        POINT_SAMPLER, LINEAR_SAMPLER, TONEMAP_PIPELINE, { 1 } };
    inputs.mSourceExtent = { 1280, 720 };
    inputs.mDestinationExtent = { 1280, 720 };
    inputs.mParameters = { 1.25f, 0.7f, 1, 1.8f };
    return *buildTonemapFrame(inputs);
}

FrameSnapshot materialFrame()
{
    FrameSnapshot frame;
    frame.mFrame   = 23;
    frame.mBuffers = { { VERTEX_BUFFER, 4096, ResourceLifetime::Persistent }, { INDEX_BUFFER, 128, ResourceLifetime::Persistent } };
    frame.mImages  = { image(DIFFUSE_IMAGE, 64, 64, PixelFormat::RGBA8Unorm, ResourceLifetime::Persistent, 7),
                       image(NORMAL_IMAGE, 64, 64, PixelFormat::RGBA8Unorm, ResourceLifetime::Persistent, 7),
                       image(SPECULAR_IMAGE, 64, 64, PixelFormat::RGBA8Unorm, ResourceLifetime::Persistent, 7),
                       image(GBUFFER0_IMAGE, 64, 64, PixelFormat::RGBA8Unorm),
                       image(GBUFFER1_IMAGE, 64, 64, PixelFormat::RGBA8Unorm),
                       image(GBUFFER2_IMAGE, 64, 64, PixelFormat::RGBA16Unorm),
                       image(DEPTH_IMAGE, 64, 64, PixelFormat::Depth24Unorm) };
    frame.mSamplers.push_back(sampler(LINEAR_SAMPLER, Filter::Linear, MipFilter::Linear, 8.f, AddressMode::Repeat));

    PipelineResource pipeline;
    pipeline.mHandle            = MATERIAL_PIPELINE;
    pipeline.mProgram           = { "deferred.material.normspec", 0 };
    pipeline.mCullMode          = CullMode::Back;
    pipeline.mDepthTestEnabled  = true;
    pipeline.mDepthWriteEnabled = true;
    pipeline.mDepthCompare      = CompareOp::LessOrEqual;
    pipeline.mDepthFormat       = PixelFormat::Depth24Unorm;
    pipeline.mColorTargets      = { { PixelFormat::RGBA8Unorm, false, 0xf },
                                    { PixelFormat::RGBA8Unorm, false, 0xf },
                                    { PixelFormat::RGBA16Unorm, false, 0xf } };
    pipeline.mVertexBindings    = { { 0, 16 }, { 1, 16 }, { 2, 8 }, { 3, 4 }, { 4, 16 }, { 5, 8 }, { 6, 8 } };
    pipeline.mVertexAttributes  = {
        { VertexSemantic::Position, VertexFormat::Float3, 0, 0 },  { VertexSemantic::Normal, VertexFormat::Float3, 1, 0 },
        { VertexSemantic::TexCoord0, VertexFormat::Float2, 2, 0 }, { VertexSemantic::Color, VertexFormat::UNorm8x4, 3, 0 },
        { VertexSemantic::Tangent, VertexFormat::Float4, 4, 0 },   { VertexSemantic::TexCoord1, VertexFormat::Float2, 5, 0 },
        { VertexSemantic::TexCoord2, VertexFormat::Float2, 6, 0 }
    };
    pipeline.mSampledImageBindings = { 0, 1, 2 };
    pipeline.mParameterBindings    = { { 0, 160 } };
    frame.mPipelines.push_back(pipeline);

    RenderPass material        = pass({ 1 }, 64, 64);
    material.mLabel            = "indexed material";
    material.mBufferAccesses   = { { VERTEX_BUFFER, BufferAccessKind::VertexRead }, { INDEX_BUFFER, BufferAccessKind::IndexRead } };
    material.mImageAccesses    = { { DIFFUSE_IMAGE,
                                     { 0, 7, 0, 1 },
                                     ImageAccessKind::SampledRead,
                                     ImageState::ShaderRead,
                                     ImageState::ShaderRead,
                                     ImageState::ShaderRead },
                                   { NORMAL_IMAGE,
                                     { 0, 7, 0, 1 },
                                     ImageAccessKind::SampledRead,
                                     ImageState::ShaderRead,
                                     ImageState::ShaderRead,
                                     ImageState::ShaderRead },
                                   { SPECULAR_IMAGE,
                                     { 0, 7, 0, 1 },
                                     ImageAccessKind::SampledRead,
                                     ImageState::ShaderRead,
                                     ImageState::ShaderRead,
                                     ImageState::ShaderRead },
                                   { GBUFFER0_IMAGE,
                                     {},
                                     ImageAccessKind::ColorAttachmentWrite,
                                     ImageState::Undefined,
                                     ImageState::ColorAttachment,
                                     ImageState::ShaderRead },
                                   { GBUFFER1_IMAGE,
                                     {},
                                     ImageAccessKind::ColorAttachmentWrite,
                                     ImageState::Undefined,
                                     ImageState::ColorAttachment,
                                     ImageState::ShaderRead },
                                   { GBUFFER2_IMAGE,
                                     {},
                                     ImageAccessKind::ColorAttachmentWrite,
                                     ImageState::Undefined,
                                     ImageState::ColorAttachment,
                                     ImageState::ShaderRead },
                                   { DEPTH_IMAGE,
                                     {},
                                     ImageAccessKind::DepthAttachmentReadWrite,
                                     ImageState::DepthAttachment,
                                     ImageState::DepthAttachment,
                                     ImageState::DepthAttachment } };
    material.mColorAttachments = { { GBUFFER0_IMAGE, {}, LoadOp::Clear, StoreOp::Store, {} },
                                   { GBUFFER1_IMAGE, {}, LoadOp::Clear, StoreOp::Store, {} },
                                   { GBUFFER2_IMAGE, {}, LoadOp::Clear, StoreOp::Store, {} } };
    material.mDepthAttachment  = DepthAttachment{ DEPTH_IMAGE, {}, LoadOp::Load, StoreOp::Store, 1.f };

    DrawIndexed draw;
    draw.mResources.mPipeline      = MATERIAL_PIPELINE;
    draw.mResources.mVertexBuffers = { { 0, VERTEX_BUFFER, 0 },   { 1, VERTEX_BUFFER, 256 },  { 2, VERTEX_BUFFER, 512 },
                                       { 3, VERTEX_BUFFER, 768 }, { 4, VERTEX_BUFFER, 1024 }, { 5, VERTEX_BUFFER, 1280 },
                                       { 6, VERTEX_BUFFER, 1536 } };
    draw.mResources.mSampledImages = { { 0, DIFFUSE_IMAGE, { 0, 7, 0, 1 }, LINEAR_SAMPLER },
                                       { 1, NORMAL_IMAGE, { 0, 7, 0, 1 }, LINEAR_SAMPLER },
                                       { 2, SPECULAR_IMAGE, { 0, 7, 0, 1 }, LINEAR_SAMPLER } };
    draw.mResources.mParameters.push_back({ 0, bytes(160) });
    draw.mIndexBuffer = { INDEX_BUFFER, 0, IndexType::UInt16 };
    draw.mIndexCount  = 6;
    draw.mMinVertex   = 0;
    draw.mMaxVertex   = 3;
    material.mDraws.emplace_back(std::move(draw));
    frame.mPasses.push_back(std::move(material));
    return frame;
}

FrameSnapshot streamingUploadFrame()
{
    FrameSnapshot frame;
    frame.mFrame                     = 31;
    ImageResource old_streamed_image = image(OLD_STREAMED_IMAGE, 4, 4, PixelFormat::RGBA8Unorm);
    old_streamed_image.mMipLevels    = 3;
    ImageResource streamed_image     = image(STREAMED_IMAGE, 4, 4, PixelFormat::RGBA8Unorm);
    streamed_image.mMipLevels        = 3;
    frame.mImages = { old_streamed_image, streamed_image, image(OUTPUT_IMAGE, 4, 4, PixelFormat::RGBA8Unorm, ResourceLifetime::External) };
    frame.mSamplers.push_back(sampler(LINEAR_SAMPLER, Filter::Linear, MipFilter::Linear));

    PipelineResource pipeline;
    pipeline.mHandle  = TEXTURE_PIPELINE;
    pipeline.mProgram = { "contract.sample-texture", 0 };
    pipeline.mColorTargets.push_back({ PixelFormat::RGBA8Unorm, false, 0xf });
    pipeline.mSampledImageBindings = { 0 };
    frame.mPipelines.push_back(pipeline);

    TextureUpload upload;
    upload.mDestination     = STREAMED_IMAGE;
    upload.mRevision        = 7;
    upload.mExtent          = { 4, 4 };
    upload.mLogicalExtent   = { 16, 16 };
    upload.mResidentDiscard = 2;
    upload.mSourceFormat    = PixelFormat::RGBA8Unorm;
    upload.mRowPitch        = 16;
    upload.mPixels          = bytes(64);
    upload.mMipGeneration   = MipGeneration::GenerateRemaining;
    frame.mUploads.push_back(upload);

    RenderPass sample     = pass({ 1 }, 4, 4);
    sample.mLabel         = "sample streamed image";
    sample.mImageAccesses = { { STREAMED_IMAGE,
                                { 0, 3, 0, 1 },
                                ImageAccessKind::SampledRead,
                                ImageState::ShaderRead,
                                ImageState::ShaderRead,
                                ImageState::ShaderRead },
                              { OUTPUT_IMAGE,
                                {},
                                ImageAccessKind::ColorAttachmentWrite,
                                ImageState::Undefined,
                                ImageState::ColorAttachment,
                                ImageState::ShaderRead } };
    sample.mColorAttachments.push_back({ OUTPUT_IMAGE, {}, LoadOp::DontCare, StoreOp::Store, {} });

    Draw draw;
    draw.mResources.mPipeline = TEXTURE_PIPELINE;
    draw.mResources.mSampledImages.push_back({ 0, STREAMED_IMAGE, { 0, 3, 0, 1 }, LINEAR_SAMPLER });
    draw.mVertexCount = 3;
    sample.mDraws.emplace_back(std::move(draw));
    frame.mPasses.push_back(std::move(sample));
    frame.mReleases.push_back({ ResourceHandle{ OLD_STREAMED_IMAGE }, frame.mFrame });
    return frame;
}

bool hasError(const ValidationResult& result, ValidationCode code)
{
    return std::find_if(result.mErrors.begin(), result.mErrors.end(),
                        [code](const ValidationError& error) { return error.mCode == code; }) != result.mErrors.end();
}

} // namespace

namespace tut
{
struct render_contract_test
{
};

using render_contract_test_group  = test_group<render_contract_test>;
using render_contract_test_object = render_contract_test_group::object;
render_contract_test_group render_contract_tests("render contract");

template<>
template<>
void render_contract_test_object::test<1>()
{
    static_assert(!std::is_same_v<BufferHandle, ImageHandle>);
    static_assert(!std::is_same_v<ImageHandle, SamplerHandle>);
    static_assert(!std::is_same_v<SamplerHandle, PipelineHandle>);

    ensure("full-screen pass validates", static_cast<bool>(validate(fullScreenFrame())));
    ensure("indexed material draw validates", static_cast<bool>(validate(materialFrame())));
    ensure("streaming texture upload validates", static_cast<bool>(validate(streamingUploadFrame())));
}

template<>
template<>
void render_contract_test_object::test<2>()
{
    FrameSnapshot frame = fullScreenFrame();
    frame.mPasses[0].mImageAccesses.pop_back();
    const ValidationResult result = validate(frame);
    ensure("full-screen attachment without an explicit write is rejected", hasError(result, ValidationCode::MissingAccess));
}

template<>
template<>
void render_contract_test_object::test<3>()
{
    FrameSnapshot frame           = materialFrame();
    DrawIndexed&  draw            = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    draw.mFirstIndex              = 100;
    draw.mIndexCount              = 100;
    const ValidationResult result = validate(frame);
    ensure("indexed material range beyond the index buffer is rejected", hasError(result, ValidationCode::OutOfBounds));
}

template<>
template<>
void render_contract_test_object::test<4>()
{
    FrameSnapshot frame             = streamingUploadFrame();
    frame.mUploads[0].mPixels.mSize = 63;
    const ValidationResult result   = validate(frame);
    ensure("undersized streaming payload is rejected", hasError(result, ValidationCode::OutOfBounds));
}

template<>
template<>
void render_contract_test_object::test<5>()
{
    FrameSnapshot frame = fullScreenFrame();
    Draw&         draw  = std::get<Draw>(frame.mPasses[0].mDraws[0]);
    draw.mResources.mSampledImages[0].mImage.mGeneration++;
    const ValidationResult result = validate(frame);
    ensure("stale sampled-image generation is rejected", hasError(result, ValidationCode::MissingResource));
}

template<>
template<>
void render_contract_test_object::test<6>()
{
    FrameSnapshot frame           = streamingUploadFrame();
    frame.mReleases[0].mFrame     = frame.mFrame - 1;
    const ValidationResult result = validate(frame);
    ensure("release before frame completion is rejected", hasError(result, ValidationCode::InvalidRelease));
}

template<>
template<>
void render_contract_test_object::test<7>()
{
    FrameSnapshot frame                                         = materialFrame();
    frame.mPasses[0].mDepthAttachment->mSubresource.mMipLevel   = std::numeric_limits<std::uint32_t>::max();
    frame.mPasses[0].mImageAccesses.back().mRange.mBaseMipLevel = std::numeric_limits<std::uint32_t>::max();
    const ValidationResult result                               = validate(frame);
    ensure("invalid depth subresource is rejected without traversing it", hasError(result, ValidationCode::OutOfBounds));
}

template<>
template<>
void render_contract_test_object::test<8>()
{
    FrameSnapshot frame                         = materialFrame();
    DrawIndexed&  draw                          = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    draw.mResources.mParameters[0].mBytes.mSize = 1;
    const ValidationResult result               = validate(frame);
    ensure("parameter size must match its pipeline layout", hasError(result, ValidationCode::InvalidBinding));
}

template<>
template<>
void render_contract_test_object::test<9>()
{
    FrameSnapshot frame           = materialFrame();
    DrawIndexed&  draw            = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    draw.mIndexBuffer.mType       = static_cast<IndexType>(99);
    const ValidationResult result = validate(frame);
    ensure("index type must be inside the contract domain", hasError(result, ValidationCode::InvalidBinding));
}

template<>
template<>
void render_contract_test_object::test<10>()
{
    FrameSnapshot frame                      = fullScreenFrame();
    frame.mPasses[0].mImageAccesses[0].mKind = static_cast<ImageAccessKind>(99);
    const ValidationResult result            = validate(frame);
    ensure("image access kind must be inside the contract domain", hasError(result, ValidationCode::InvalidState));
}

template<>
template<>
void render_contract_test_object::test<11>()
{
    FrameSnapshot frame                = streamingUploadFrame();
    frame.mUploads[0].mResidentDiscard = 0;
    const ValidationResult result      = validate(frame);
    ensure("logical extent and resident discard must agree", hasError(result, ValidationCode::InvalidUpload));
}

template<>
template<>
void render_contract_test_object::test<12>()
{
    FrameSnapshot frame                                      = streamingUploadFrame();
    frame.mPasses[0].mImageAccesses[0].mRange.mMipLevelCount = 1;
    const ValidationResult result                            = validate(frame);
    ensure("sampled mip range must be covered by an explicit access", hasError(result, ValidationCode::MissingAccess));
}

template<>
template<>
void render_contract_test_object::test<13>()
{
    FrameSnapshot frame           = materialFrame();
    DrawIndexed&  draw            = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    draw.mIndexBuffer.mOffset     = 1;
    const ValidationResult result = validate(frame);
    ensure("index offset must align to the index type", hasError(result, ValidationCode::InvalidBinding));
}

template<>
template<>
void render_contract_test_object::test<14>()
{
    FrameSnapshot frame                          = fullScreenFrame();
    frame.mPasses[0].mColorAttachments[0].mStore = StoreOp::DontCare;
    const ValidationResult result                = validate(frame);
    ensure("discarded attachment contents cannot be published as readable", hasError(result, ValidationCode::InvalidState));
}

template<>
template<>
void render_contract_test_object::test<15>()
{
    FrameSnapshot frame = fullScreenFrame();
    frame.mPasses[0].mDependencies.push_back({ 2 });
    const ValidationResult result = validate(frame);
    ensure("pass dependencies must name earlier passes", hasError(result, ValidationCode::InvalidDependency));
}

template<>
template<>
void render_contract_test_object::test<16>()
{
    constexpr std::array variants{ TonemapVariant::Deferred, TonemapVariant::NoPost, TonemapVariant::GammaCorrect,
                                   TonemapVariant::NoPostGammaCorrect, TonemapVariant::LegacyGammaCorrect,
                                   TonemapVariant::NoPostLegacyGammaCorrect };
    constexpr std::array formats{ PixelFormat::RGBA8Unorm, PixelFormat::RGBA16Float };

    for (TonemapVariant variant : variants)
    {
        for (PixelFormat format : formats)
        {
            TonemapInputs inputs;
            inputs.mFrame = 91;
            inputs.mSourceExtent = { 17, 9 };
            inputs.mDestinationExtent = { 13, 7 };
            inputs.mDestinationFormat = format;
            inputs.mVariant = variant;
            inputs.mParameters = { 1.25f, 0.65f, 1, 1.8f };

            auto frame = buildTonemapFrame(inputs);
            ensure("every compiled tonemap variant and output format builds", frame.has_value());
            inputs.mParameters.mExposure = 3.f;
            auto decoded = decodeTonemapFrame(*frame);
            ensure("canonical tonemap packet decodes", decoded.has_value());
            ensure("builder owns parameter bytes", decoded->mParameters.mExposure == 1.25f);
            ensure("source extent survives", decoded->mSourceExtent.mWidth == 17 && decoded->mSourceExtent.mHeight == 9);
            ensure("destination extent survives", decoded->mDestinationExtent.mWidth == 13 && decoded->mDestinationExtent.mHeight == 7);
            ensure("variant survives", decoded->mVariant == variant);
            ensure("output format survives", decoded->mDestinationFormat == format);
            ensure("gamma survives", decoded->mParameters.mGamma == 1.8f);
            ensure("trace uses mirrored addressing", frame->mSamplers[0].mAddressU == AddressMode::Mirror &&
                                                     frame->mSamplers[1].mAddressV == AddressMode::Mirror);
            ensure("trace keeps disabled depth compare", frame->mPipelines[0].mDepthCompare == CompareOp::LessOrEqual);
        }
    }
}

template<>
template<>
void render_contract_test_object::test<17>()
{
    TonemapInputs inputs;
    inputs.mFrame = 1;
    inputs.mSourceExtent = { 4, 4 };
    inputs.mDestinationExtent = { 4, 4 };

    inputs.mVariant = static_cast<TonemapVariant>(4);
    ensure("legacy gamma without gamma correction is rejected", !buildTonemapFrame(inputs));

    inputs.mVariant = TonemapVariant::Deferred;
    inputs.mDestinationFormat = PixelFormat::RGB8Unorm;
    ensure("unsupported output format is rejected", !buildTonemapFrame(inputs));

    inputs.mDestinationFormat = PixelFormat::RGBA16Float;
    inputs.mParameters.mGamma = std::numeric_limits<float>::quiet_NaN();
    ensure("non-finite parameters are rejected", !buildTonemapFrame(inputs));
}

template<>
template<>
void render_contract_test_object::test<18>()
{
    FrameSnapshot frame = fullScreenFrame();
    frame.mPipelines[0].mProgram.mName = "unknown.tonemap";
    ensure("decoder rejects an unknown program", !decodeTonemapFrame(frame));

    frame = fullScreenFrame();
    frame.mPasses[0].mScissor.mWidth--;
    ensure("decoder rejects a partial scissor", !decodeTonemapFrame(frame));

    frame = fullScreenFrame();
    frame.mSamplers[0].mAddressU = AddressMode::Clamp;
    ensure("decoder rejects state that diverges from the trace", !decodeTonemapFrame(frame));
}

} // namespace tut
