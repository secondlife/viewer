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

#include "llmaterialcontract.h"
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
constexpr ImageHandle OLD_STREAMED_IMAGE{ 11, 1 };
constexpr ImageHandle STREAMED_IMAGE{ 11, 2 };

constexpr BufferHandle SCREEN_TRIANGLE_BUFFER{ 3, 1 };

constexpr SamplerHandle POINT_SAMPLER{ 1, 1 };
constexpr SamplerHandle LINEAR_SAMPLER{ 2, 1 };

constexpr PipelineHandle TONEMAP_PIPELINE{ 1, 1 };
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

SamplerResource sampler(SamplerHandle handle, Filter filter, MipFilter mip_filter = MipFilter::Disabled, float max_anisotropy = 1.f,
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
    inputs.mFrame             = 17;
    inputs.mHandles           = { SCREEN_TRIANGLE_BUFFER, SCENE_IMAGE,    EXPOSURE_IMAGE,   OUTPUT_IMAGE,
                                  POINT_SAMPLER,          LINEAR_SAMPLER, TONEMAP_PIPELINE, { 1 } };
    inputs.mSourceExtent      = { 1280, 720 };
    inputs.mDestinationExtent = { 1280, 720 };
    inputs.mParameters        = { 1.25f, 0.7f, 1, 1.8f };
    return *buildTonemapFrame(inputs);
}

FrameSnapshot materialFrame()
{
    MaterialInputs inputs;
    inputs.mFrame                            = 23;
    inputs.mParameters.mEnvironmentIntensity = 0.625f;
    return *buildMaterialFrame(inputs);
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
    constexpr std::array variants{ TonemapVariant::Deferred,           TonemapVariant::NoPost,
                                   TonemapVariant::GammaCorrect,       TonemapVariant::NoPostGammaCorrect,
                                   TonemapVariant::LegacyGammaCorrect, TonemapVariant::NoPostLegacyGammaCorrect };
    constexpr std::array formats{ PixelFormat::RGBA8Unorm, PixelFormat::RGBA16Float };

    for (TonemapVariant variant : variants)
    {
        for (PixelFormat format : formats)
        {
            TonemapInputs inputs;
            inputs.mFrame             = 91;
            inputs.mSourceExtent      = { 17, 9 };
            inputs.mDestinationExtent = { 13, 7 };
            inputs.mDestinationFormat = format;
            inputs.mVariant           = variant;
            inputs.mParameters        = { 1.25f, 0.65f, 1, 1.8f };

            auto frame = buildTonemapFrame(inputs);
            ensure("every compiled tonemap variant and output format builds", frame.has_value());
            inputs.mParameters.mExposure = 3.f;
            auto decoded                 = decodeTonemapFrame(*frame);
            ensure("canonical tonemap packet decodes", decoded.has_value());
            ensure("builder owns parameter bytes", decoded->mParameters.mExposure == 1.25f);
            ensure("source extent survives", decoded->mSourceExtent.mWidth == 17 && decoded->mSourceExtent.mHeight == 9);
            ensure("destination extent survives", decoded->mDestinationExtent.mWidth == 13 && decoded->mDestinationExtent.mHeight == 7);
            ensure("variant survives", decoded->mVariant == variant);
            ensure("output format survives", decoded->mDestinationFormat == format);
            ensure("gamma survives", decoded->mParameters.mGamma == 1.8f);
            ensure("trace uses mirrored addressing",
                   frame->mSamplers[0].mAddressU == AddressMode::Mirror && frame->mSamplers[1].mAddressV == AddressMode::Mirror);
            ensure("trace keeps disabled depth compare", frame->mPipelines[0].mDepthCompare == CompareOp::LessOrEqual);
        }
    }
}

template<>
template<>
void render_contract_test_object::test<17>()
{
    TonemapInputs inputs;
    inputs.mFrame             = 1;
    inputs.mSourceExtent      = { 4, 4 };
    inputs.mDestinationExtent = { 4, 4 };

    inputs.mVariant = static_cast<TonemapVariant>(4);
    ensure("legacy gamma without gamma correction is rejected", !buildTonemapFrame(inputs));

    inputs.mVariant           = TonemapVariant::Deferred;
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
    FrameSnapshot frame                = fullScreenFrame();
    frame.mPipelines[0].mProgram.mName = "unknown.tonemap";
    ensure("decoder rejects an unknown program", !decodeTonemapFrame(frame));

    frame = fullScreenFrame();
    frame.mPasses[0].mScissor.mWidth--;
    ensure("decoder rejects a partial scissor", !decodeTonemapFrame(frame));

    frame                        = fullScreenFrame();
    frame.mSamplers[0].mAddressU = AddressMode::Clamp;
    ensure("decoder rejects state that diverges from the trace", !decodeTonemapFrame(frame));
}

template<>
template<>
void render_contract_test_object::test<19>()
{
    MaterialInputs inputs;
    inputs.mFrame                     = 41;
    inputs.mParameters.mSpecularColor = { 0.2f, 0.4f, 0.6f, 0.8f };
    inputs.mParameters.mMirror        = 1.f;

    auto frame = buildMaterialFrame(inputs);
    ensure("canonical material packet builds", frame.has_value());
    inputs.mParameters.mMirror = 0.f;
    auto decoded               = decodeMaterialFrame(*frame);
    ensure("canonical material packet decodes", decoded.has_value());
    ensure("material builder owns the complete parameter block",
           decoded->mParameters.mMirror == 1.f && decoded->mParameters.mSpecularColor == std::array<float, 4>{ 0.2f, 0.4f, 0.6f, 0.8f });
    ensure("material packet uses the complete 272-byte parameter layout",
           frame->mPipelines[0].mParameterBindings[0].mSize == sizeof(MaterialParameters) &&
               std::get<DrawIndexed>(frame->mPasses[0].mDraws[0]).mResources.mParameters[0].mBytes.mSize == sizeof(MaterialParameters));
    ensure("material packet uses packed viewer buffer sizes",
           frame->mBuffers[0].mSize == MATERIAL_VERTEX_BUFFER_SIZE && frame->mBuffers[1].mSize == MATERIAL_INDEX_BUFFER_SIZE);
    const auto& bindings = std::get<DrawIndexed>(frame->mPasses[0].mDraws[0]).mResources.mVertexBuffers;
    ensure("material packet uses packed color and tangent offsets",
           bindings[3].mOffset == MATERIAL_COLOR_OFFSET && bindings[4].mOffset == MATERIAL_TANGENT_OFFSET);
    ensure("material packet fixes an 8 by 8 target and three input mips",
           frame->mPasses[0].mExtent.mWidth == 8 && frame->mPasses[0].mExtent.mHeight == 8 && frame->mImages[0].mExtent.mWidth == 4 &&
               frame->mImages[0].mMipLevels == 3);
}

template<>
template<>
void render_contract_test_object::test<20>()
{
    FrameSnapshot frame                   = materialFrame();
    frame.mPipelines[0].mProgram.mVariant = 1;
    ensure("material decoder rejects another shader variant", !decodeMaterialFrame(frame));

    frame = materialFrame();
    std::swap(frame.mImages[0], frame.mImages[1]);
    ensure("material decoder rejects reordered image declarations", !decodeMaterialFrame(frame));

    frame             = materialFrame();
    DrawIndexed& draw = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    std::swap(draw.mResources.mVertexBuffers[0], draw.mResources.mVertexBuffers[1]);
    ensure("material decoder rejects reordered vertex bindings", !decodeMaterialFrame(frame));

    frame                                    = materialFrame();
    frame.mPasses[0].mDepthAttachment->mLoad = LoadOp::Clear;
    ensure("material decoder rejects another depth load operation", !decodeMaterialFrame(frame));

    frame = materialFrame();
    frame.mPasses.push_back(frame.mPasses.front());
    frame.mPasses.back().mId = { 2 };
    ensure("material decoder rejects an extra pass", !decodeMaterialFrame(frame));
}

template<>
template<>
void render_contract_test_object::test<21>()
{
    MaterialInputs inputs;
    inputs.mFrame                    = 1;
    inputs.mParameters.mClipPlane[2] = std::numeric_limits<float>::quiet_NaN();
    ensure("material builder rejects non-finite parameters", !buildMaterialFrame(inputs));

    inputs.mParameters                   = {};
    inputs.mParameters.mSpecularColor[1] = 1.01f;
    ensure("material builder rejects a specular component outside [0, 1]", !buildMaterialFrame(inputs));

    inputs.mParameters                   = {};
    inputs.mParameters.mSpecularColor[3] = -0.01f;
    ensure("material builder rejects a negative specular component", !buildMaterialFrame(inputs));

    inputs.mParameters                       = {};
    inputs.mParameters.mEnvironmentIntensity = -0.01f;
    ensure("material builder rejects environment intensity outside [0, 1]", !buildMaterialFrame(inputs));

    inputs.mParameters                       = {};
    inputs.mParameters.mEnvironmentIntensity = 1.01f;
    ensure("material builder rejects environment intensity above one", !buildMaterialFrame(inputs));

    inputs.mParameters                     = {};
    inputs.mParameters.mEmissiveBrightness = 0.5f;
    ensure("material builder requires binary emissive state", !buildMaterialFrame(inputs));

    inputs.mParameters         = {};
    inputs.mParameters.mMirror = 0.5f;
    ensure("material builder requires binary mirror state", !buildMaterialFrame(inputs));

    MaterialInputs aliased_buffers;
    aliased_buffers.mFrame                = 1;
    aliased_buffers.mHandles.mIndexBuffer = { aliased_buffers.mHandles.mVertexBuffer.mIndex,
                                              aliased_buffers.mHandles.mVertexBuffer.mGeneration + 1 };
    ensure("material builder rejects buffer indices aliased across generations", !buildMaterialFrame(aliased_buffers));

    constexpr std::array image_handles{ &MaterialHandles::mDiffuse,  &MaterialHandles::mNormal,   &MaterialHandles::mSpecular,
                                        &MaterialHandles::mGBuffer0, &MaterialHandles::mGBuffer1, &MaterialHandles::mGBuffer2,
                                        &MaterialHandles::mDepth };
    for (std::size_t image = 1; image < image_handles.size(); ++image)
    {
        MaterialInputs aliased_images;
        aliased_images.mFrame = 1;
        aliased_images.mHandles.*
            image_handles[image] = { aliased_images.mHandles.mDiffuse.mIndex,
                                     aliased_images.mHandles.mDiffuse.mGeneration + static_cast<std::uint32_t>(image) };
        ensure("material builder rejects image indices aliased across generations", !buildMaterialFrame(aliased_images));
    }

    FrameSnapshot aliased_buffer_frame = materialFrame();
    BufferHandle aliased_index{ aliased_buffer_frame.mBuffers[0].mHandle.mIndex, aliased_buffer_frame.mBuffers[0].mHandle.mGeneration + 1 };
    aliased_buffer_frame.mBuffers[1].mHandle                                              = aliased_index;
    aliased_buffer_frame.mPasses[0].mBufferAccesses[1].mBuffer                            = aliased_index;
    std::get<DrawIndexed>(aliased_buffer_frame.mPasses[0].mDraws[0]).mIndexBuffer.mBuffer = aliased_index;
    ensure("material decoder rejects buffer indices aliased across generations", !decodeMaterialFrame(aliased_buffer_frame));

    FrameSnapshot aliased_image_frame = materialFrame();
    ImageHandle   aliased_normal{ aliased_image_frame.mImages[0].mHandle.mIndex, aliased_image_frame.mImages[0].mHandle.mGeneration + 1 };
    aliased_image_frame.mImages[1].mHandle                                                              = aliased_normal;
    aliased_image_frame.mPasses[0].mImageAccesses[1].mImage                                             = aliased_normal;
    std::get<DrawIndexed>(aliased_image_frame.mPasses[0].mDraws[0]).mResources.mSampledImages[1].mImage = aliased_normal;
    ensure("material decoder rejects image indices aliased across generations", !decodeMaterialFrame(aliased_image_frame));

    FrameSnapshot frame                             = materialFrame();
    frame.mPipelines[0].mParameterBindings[0].mSize = 160;
    DrawIndexed& draw                               = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    draw.mResources.mParameters[0].mBytes           = bytes(160);
    ensure("material decoder rejects the old 160-byte placeholder", !decodeMaterialFrame(frame));
}

template<>
template<>
void render_contract_test_object::test<22>()
{
    for (std::size_t binding = 0; binding < 7; ++binding)
    {
        FrameSnapshot frame = materialFrame();
        DrawIndexed&  draw  = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
        ++draw.mResources.mVertexBuffers[binding].mOffset;
        ensure("each packed vertex offset is canonical", !decodeMaterialFrame(frame));

        frame = materialFrame();
        ++frame.mPipelines[0].mVertexBindings[binding].mStride;
        ensure("each planar vertex stride is canonical", !decodeMaterialFrame(frame));

        frame                                                   = materialFrame();
        frame.mPipelines[0].mVertexAttributes[binding].mBinding = static_cast<std::uint32_t>((binding + 1) % 7);
        ensure("each material vertex attribute binding is canonical", !decodeMaterialFrame(frame));
    }
}

template<>
template<>
void render_contract_test_object::test<23>()
{
    FrameSnapshot frame = materialFrame();
    frame.mBuffers.pop_back();
    ensure("material decoder rejects a missing resource declaration", !decodeMaterialFrame(frame));

    frame = materialFrame();
    std::swap(frame.mBuffers[0], frame.mBuffers[1]);
    ensure("material decoder rejects reordered buffer declarations", !decodeMaterialFrame(frame));

    frame                       = materialFrame();
    frame.mImages[0].mMipLevels = 2;
    ensure("material decoder rejects a shorter texture mip chain", !decodeMaterialFrame(frame));

    frame                           = materialFrame();
    frame.mImages[1].mExtent.mWidth = 8;
    ensure("material decoder rejects another sampled texture extent", !decodeMaterialFrame(frame));

    frame                             = materialFrame();
    frame.mSamplers[0].mMaxAnisotropy = 4.f;
    ensure("material decoder rejects another anisotropy", !decodeMaterialFrame(frame));

    frame                        = materialFrame();
    frame.mSamplers[0].mAddressV = AddressMode::Clamp;
    ensure("material decoder rejects another texture address mode", !decodeMaterialFrame(frame));

    frame             = materialFrame();
    DrawIndexed& draw = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    std::swap(draw.mResources.mSampledImages[0], draw.mResources.mSampledImages[1]);
    ensure("material decoder rejects reordered sampled bindings", !decodeMaterialFrame(frame));
}

template<>
template<>
void render_contract_test_object::test<24>()
{
    constexpr std::array<std::size_t, 3> COLOR_IMAGES{ 3, 4, 5 };
    for (std::size_t image_index : COLOR_IMAGES)
    {
        FrameSnapshot frame                = materialFrame();
        frame.mImages[image_index].mFormat = PixelFormat::RGB8Unorm;
        ensure("every G-buffer storage format is canonical", !decodeMaterialFrame(frame));
    }

    for (std::size_t attachment = 0; attachment < 3; ++attachment)
    {
        FrameSnapshot frame                                  = materialFrame();
        frame.mPasses[0].mColorAttachments[attachment].mLoad = LoadOp::DontCare;
        ensure("every G-buffer load operation is canonical", !decodeMaterialFrame(frame));

        frame                                                 = materialFrame();
        frame.mPasses[0].mColorAttachments[attachment].mStore = StoreOp::DontCare;
        ensure("every G-buffer store operation is canonical", !decodeMaterialFrame(frame));
    }

    FrameSnapshot frame              = materialFrame();
    frame.mImages[6].mFormat         = PixelFormat::Depth32Float;
    frame.mPipelines[0].mDepthFormat = PixelFormat::Depth32Float;
    ensure("material decoder rejects another depth format", !decodeMaterialFrame(frame));

    frame                                     = materialFrame();
    frame.mPasses[0].mDepthAttachment->mStore = StoreOp::DontCare;
    ensure("material decoder rejects another depth store operation", !decodeMaterialFrame(frame));
}

template<>
template<>
void render_contract_test_object::test<25>()
{
    FrameSnapshot frame      = materialFrame();
    DrawIndexed&  first_draw = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    first_draw.mIndexCount   = 5;
    ensure("material decoder rejects another index count", !decodeMaterialFrame(frame));

    frame                    = materialFrame();
    DrawIndexed& second_draw = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    second_draw.mFirstIndex  = 1;
    second_draw.mIndexCount  = 5;
    ensure("material decoder rejects another first index", !decodeMaterialFrame(frame));

    frame                   = materialFrame();
    DrawIndexed& third_draw = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    third_draw.mMaxVertex   = 2;
    ensure("material decoder rejects another declared vertex range", !decodeMaterialFrame(frame));

    frame                      = materialFrame();
    DrawIndexed& fourth_draw   = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    fourth_draw.mFirstInstance = 1;
    ensure("material decoder rejects another instance range", !decodeMaterialFrame(frame));

    frame                                       = materialFrame();
    DrawIndexed& fifth_draw                     = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    auto         larger_storage                 = std::make_shared<std::vector<std::uint8_t>>(sizeof(MaterialParameters) + 1, 0);
    fifth_draw.mResources.mParameters[0].mBytes = { larger_storage, 1, sizeof(MaterialParameters) };
    ensure("material decoder rejects a non-canonical parameter byte range", !decodeMaterialFrame(frame));

    frame                                              = materialFrame();
    DrawIndexed& sixth_draw                            = std::get<DrawIndexed>(frame.mPasses[0].mDraws[0]);
    sixth_draw.mResources.mParameters[0].mBinding      = 1;
    frame.mPipelines[0].mParameterBindings[0].mBinding = 1;
    ensure("material decoder rejects another parameter binding", !decodeMaterialFrame(frame));

    frame = materialFrame();
    frame.mPasses[0].mDraws.push_back(frame.mPasses[0].mDraws.front());
    ensure("material decoder rejects an extra indexed draw", !decodeMaterialFrame(frame));
}

template<>
template<>
void render_contract_test_object::test<26>()
{
    FrameSnapshot frame           = materialFrame();
    frame.mPasses[0].mViewport.mX = 1.f;
    ensure("material decoder rejects another viewport", !decodeMaterialFrame(frame));

    frame = materialFrame();
    --frame.mPasses[0].mScissor.mWidth;
    ensure("material decoder rejects another scissor", !decodeMaterialFrame(frame));

    frame                         = materialFrame();
    frame.mPipelines[0].mCullMode = CullMode::Disabled;
    ensure("material decoder rejects disabled culling", !decodeMaterialFrame(frame));

    frame                          = materialFrame();
    frame.mPipelines[0].mFrontFace = FrontFace::Clockwise;
    ensure("material decoder rejects another front face", !decodeMaterialFrame(frame));

    frame                             = materialFrame();
    frame.mPipelines[0].mDepthCompare = CompareOp::AlwaysPass;
    ensure("material decoder rejects another depth comparison", !decodeMaterialFrame(frame));

    frame                                           = materialFrame();
    frame.mPipelines[0].mColorTargets[1].mWriteMask = 0x7;
    ensure("material decoder rejects a partial G-buffer write mask", !decodeMaterialFrame(frame));

    frame                   = materialFrame();
    frame.mPasses[0].mLabel = "other material";
    ensure("material decoder rejects another pass identity", !decodeMaterialFrame(frame));
}

template<>
template<>
void render_contract_test_object::test<27>()
{
    FrameSnapshot increasing = streamingUploadFrame();
    TextureUpload next       = increasing.mUploads.front();
    next.mRevision           = 8;
    next.mMipGeneration      = MipGeneration::Disabled;
    next.mBefore             = ImageState::ShaderRead;
    increasing.mUploads.push_back(next);
    ensure("upload revisions may increase for one exact destination", static_cast<bool>(validate(increasing)));

    FrameSnapshot equal = streamingUploadFrame();
    next                = equal.mUploads.front();
    next.mMipGeneration = MipGeneration::Disabled;
    next.mBefore        = ImageState::ShaderRead;
    equal.mUploads.push_back(next);
    ensure("equal upload revisions are rejected for one exact destination", hasError(validate(equal), ValidationCode::InvalidUpload));

    FrameSnapshot decreasing = streamingUploadFrame();
    next                     = decreasing.mUploads.front();
    next.mRevision           = 6;
    next.mMipGeneration      = MipGeneration::Disabled;
    next.mBefore             = ImageState::ShaderRead;
    decreasing.mUploads.push_back(next);
    ensure("decreasing upload revisions are rejected for one exact destination",
           hasError(validate(decreasing), ValidationCode::InvalidUpload));

    FrameSnapshot another_generation = streamingUploadFrame();
    next                             = another_generation.mUploads.front();
    next.mDestination                = OLD_STREAMED_IMAGE;
    another_generation.mUploads.push_back(next);
    ensure("upload revisions are independent across image generations", static_cast<bool>(validate(another_generation)));
}

template<>
template<>
void render_contract_test_object::test<28>()
{
    FrameSnapshot frame = streamingUploadFrame();
    for (ImageResource& image_resource : frame.mImages)
    {
        image_resource.mFormat = PixelFormat::RGB16Float;
    }
    frame.mPipelines[0].mColorTargets[0].mFormat = PixelFormat::RGB16Float;
    frame.mUploads[0].mSourceFormat              = PixelFormat::RGB16Float;
    frame.mUploads[0].mRowPitch                  = 24;
    frame.mUploads[0].mPixels                    = bytes(96);
    ensure("RGB16Float is a valid color target and six-byte upload format", static_cast<bool>(validate(frame)));

    frame.mUploads[0].mRowPitch = 23;
    ensure("RGB16Float upload rows require six bytes per pixel", hasError(validate(frame), ValidationCode::InvalidUpload));
}

} // namespace tut
