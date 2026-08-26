/**
 * @file llrendergltonemap.cpp
 * @brief OpenGL replay of the canonical tonemap packet.
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

#include "llrendergltonemap.h"

#include "llgl.h"
#include "llglslshader.h"
#include "llrendertarget.h"
#include "llshadermgr.h"
#include "llvertexbuffer.h"

#include <cstdint>
#include <optional>

namespace LLRenderGLTonemap
{
namespace
{

struct Prepared
{
    LLRenderContract::TonemapInputs mInputs;
    LLVertexBuffer* mScreenTriangle = nullptr;
    LLRenderTarget* mScene = nullptr;
    LLRenderTarget* mExposure = nullptr;
    LLRenderTarget* mDestination = nullptr;
    LLGLSLShader* mShader = nullptr;
};

std::optional<LLRenderContract::PixelFormat> pixelFormat(const LLRenderTarget& target)
{
    switch (target.getColorFormat())
    {
        case GL_RGBA:
        case GL_RGBA8:
            return LLRenderContract::PixelFormat::RGBA8Unorm;
        case GL_RGBA16F:
            return LLRenderContract::PixelFormat::RGBA16Float;
        case GL_R16F:
            return LLRenderContract::PixelFormat::R16Float;
        default:
            return std::nullopt;
    }
}

bool hasPermutation(const LLGLSLShader& shader, const char* name)
{
    const auto found = shader.mDefines.find(name);
    return found != shader.mDefines.end() && found->second == "1";
}

bool matchesVariant(LLGLSLShader& shader, LLRenderContract::TonemapVariant variant)
{
    const auto bits = static_cast<std::uint64_t>(variant);
    const bool no_post = (bits & 1U) != 0;
    const bool gamma_correct = (bits & 2U) != 0;
    const bool legacy_gamma = (bits & 4U) != 0;

    if (!shader.mFeatures.hasSrgb || !shader.mFeatures.hasTonemap ||
        hasPermutation(shader, "NO_POST") != no_post ||
        hasPermutation(shader, "GAMMA_CORRECT") != gamma_correct ||
        hasPermutation(shader, "LEGACY_GAMMA") != legacy_gamma ||
        shader.getTextureChannel(LLShaderMgr::DEFERRED_DIFFUSE) < 0)
    {
        return false;
    }

    static LLStaticHashedString exposure("exposure");
    static LLStaticHashedString tonemap_mix("tonemap_mix");
    static LLStaticHashedString tonemap_type("tonemap_type");
    if (!no_post && (shader.getTextureChannel(LLShaderMgr::EXPOSURE_MAP) < 0 ||
                     shader.getUniformLocation(exposure) < 0 || shader.getUniformLocation(tonemap_mix) < 0 ||
                     shader.getUniformLocation(tonemap_type) < 0))
    {
        return false;
    }
    return !legacy_gamma || shader.getUniformLocation(LLShaderMgr::GAMMA) >= 0;
}

std::optional<Prepared> prepare(const LLRenderContract::FrameSnapshot& frame, const Registry& registry)
{
    auto inputs = LLRenderContract::decodeTonemapFrame(frame);
    if (!inputs)
    {
        return std::nullopt;
    }

    Prepared result;
    result.mInputs = *inputs;
    result.mScreenTriangle = registry.resolve(inputs->mHandles.mScreenTriangle);
    result.mScene = registry.resolve(inputs->mHandles.mScene);
    result.mExposure = registry.resolve(inputs->mHandles.mExposure);
    result.mDestination = registry.resolve(inputs->mHandles.mDestination);

    const LLRenderContract::PipelineResource& pipeline = frame.mPipelines.front();
    result.mShader = registry.resolve(inputs->mHandles.mPipeline, pipeline.mProgram);
    const Sampler* point_sampler = registry.resolve(inputs->mHandles.mPointSampler);
    const Sampler* linear_sampler = registry.resolve(inputs->mHandles.mLinearSampler);

    if (!result.mScreenTriangle || !result.mScene || !result.mExposure || !result.mDestination || !result.mShader ||
        !point_sampler || *point_sampler != Sampler::Point || !linear_sampler || *linear_sampler != Sampler::Linear)
    {
        return std::nullopt;
    }

    const auto scene_format = pixelFormat(*result.mScene);
    const auto exposure_format = pixelFormat(*result.mExposure);
    const auto destination_format = pixelFormat(*result.mDestination);
    if (!result.mScreenTriangle->hasDataType(LLVertexBuffer::TYPE_VERTEX) || result.mScreenTriangle->getNumVerts() < 3 ||
        result.mScreenTriangle->getSize() < 48 || !result.mShader->isComplete() || !matchesVariant(*result.mShader, inputs->mVariant) ||
        !result.mScene->isComplete() || !result.mExposure->isComplete() || !result.mDestination->isComplete() ||
        result.mDestination->isBoundInStack() || result.mScene == result.mDestination || result.mExposure == result.mDestination ||
        result.mScene->getWidth() != inputs->mSourceExtent.mWidth || result.mScene->getHeight() != inputs->mSourceExtent.mHeight ||
        result.mExposure->getWidth() != 1 || result.mExposure->getHeight() != 1 ||
        result.mDestination->getWidth() != inputs->mDestinationExtent.mWidth ||
        result.mDestination->getHeight() != inputs->mDestinationExtent.mHeight ||
        scene_format != LLRenderContract::PixelFormat::RGBA16Float || exposure_format != LLRenderContract::PixelFormat::R16Float ||
        destination_format != inputs->mDestinationFormat)
    {
        return std::nullopt;
    }

    return result;
}

class ColorMaskRestore
{
public:
    ColorMaskRestore()
    {
        glGetBooleanv(GL_COLOR_WRITEMASK, mMask);
    }

    ~ColorMaskRestore()
    {
        gGL.setColorMask(mMask[0] == GL_TRUE, mMask[1] == GL_TRUE, mMask[2] == GL_TRUE, mMask[3] == GL_TRUE);
    }

private:
    GLboolean mMask[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
};

class ScissorBoxRestore
{
public:
    ScissorBoxRestore()
    {
        glGetIntegerv(GL_SCISSOR_BOX, mBox);
    }

    ~ScissorBoxRestore()
    {
        glScissor(mBox[0], mBox[1], mBox[2], mBox[3]);
    }

private:
    GLint mBox[4] = { 0, 0, 0, 0 };
};

}

bool execute(const LLRenderContract::FrameSnapshot& frame, const Registry& registry)
{
    const auto prepared = prepare(frame, registry);
    if (!prepared)
    {
        return false;
    }

    const auto& inputs = prepared->mInputs;
    prepared->mDestination->bindTarget();
    {
        LLGLDisable blend(GL_BLEND);
        LLGLDisable cull(GL_CULL_FACE);
        LLGLDepthTest depth(GL_FALSE, GL_FALSE, GL_LEQUAL);
        LLGLEnable scissor(GL_SCISSOR_TEST);
        ColorMaskRestore color_mask;
        ScissorBoxRestore scissor_box;

        gGL.setColorMask(true, true);
        glViewport(0, 0, static_cast<GLsizei>(inputs.mDestinationExtent.mWidth),
                   static_cast<GLsizei>(inputs.mDestinationExtent.mHeight));
        glScissor(0, 0, static_cast<GLsizei>(inputs.mDestinationExtent.mWidth),
                  static_cast<GLsizei>(inputs.mDestinationExtent.mHeight));

        prepared->mShader->bind();
        const S32 scene_channel = prepared->mShader->bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, prepared->mScene, false,
                                                                 LLTexUnit::TFO_POINT);
        if (scene_channel >= 0)
        {
            gGL.getTexUnit(scene_channel)->setTextureAddressMode(LLTexUnit::TAM_MIRROR);
        }
        const S32 exposure_channel = prepared->mShader->bindTexture(LLShaderMgr::EXPOSURE_MAP, prepared->mExposure, false,
                                                                    LLTexUnit::TFO_BILINEAR);
        if (exposure_channel >= 0)
        {
            gGL.getTexUnit(exposure_channel)->setTextureAddressMode(LLTexUnit::TAM_MIRROR);
        }

        prepared->mShader->uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES, static_cast<GLfloat>(inputs.mSourceExtent.mWidth),
                                     static_cast<GLfloat>(inputs.mSourceExtent.mHeight));
        prepared->mShader->uniform1f(LLShaderMgr::GAMMA, inputs.mParameters.mGamma);

        static LLStaticHashedString exposure("exposure");
        static LLStaticHashedString tonemap_mix("tonemap_mix");
        static LLStaticHashedString tonemap_type("tonemap_type");
        prepared->mShader->uniform1f(exposure, inputs.mParameters.mExposure);
        prepared->mShader->uniform1f(tonemap_mix, inputs.mParameters.mTonemapMix);
        prepared->mShader->uniform1i(tonemap_type, static_cast<GLint>(inputs.mParameters.mTonemapType));

        prepared->mScreenTriangle->setBuffer();
        prepared->mScreenTriangle->drawArrays(LLRender::TRIANGLES, 0, 3);

        gGL.getTexUnit(0)->unbind(prepared->mScene->getUsage());
        prepared->mShader->unbind();
    }
    prepared->mDestination->flush();
    return true;
}

}
