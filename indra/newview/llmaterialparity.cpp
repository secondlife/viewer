/**
 * @file llmaterialparity.cpp
 * @brief Account-free parity harness for the indexed deferred material draw.
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

#include "llviewerprecompiledheaders.h"

#include "llmaterialparity.h"

#include "llgl.h"
#include "llglslshader.h"
#include "llglstates.h"
#include "llimagegl.h"
#include "llmaterialdiagnostic.h"
#include "llrenderglmaterial.h"
#include "llrendertarget.h"
#include "llshadermgr.h"
#include "llvertexbuffer.h"
#include "llviewercontrol.h"
#include "llviewershadermgr.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace LLMaterialParity
{
namespace
{

    using namespace LLRenderContract;

    constexpr U32 MATERIAL_VERTEX_MASK = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0 |
                                         LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2 | LLVertexBuffer::MAP_COLOR |
                                         LLVertexBuffer::MAP_TANGENT;
    constexpr std::uint32_t DEPTH24_MAX      = 0xffffffU;
    constexpr const char*   MATERIAL_PROGRAM = "deferred.material.normspec";

    struct MaterialResources
    {
        LLPointer<LLVertexBuffer>                  mGeometry;
        LLPointer<LLVertexBuffer>                  mIncompatibleGeometry;
        LLPointer<LLVertexBuffer>                  mIncompatibleIndexGeometry;
        std::array<GLuint, MATERIAL_TEXTURE_COUNT> mTextures{};
        std::array<GLuint, MATERIAL_TEXTURE_COUNT> mIncompatibleTextures{};
        GLuint                                     mPoisonSampler = 0;
        LLRenderTarget                             mTarget;
        LLRenderTarget                             mIncompatibleColorTarget;
        LLRenderTarget                             mIncompatibleDepthTarget;

        MaterialResources()                                    = default;
        MaterialResources(const MaterialResources&)            = delete;
        MaterialResources& operator=(const MaterialResources&) = delete;

        ~MaterialResources()
        {
            if (mPoisonSampler != 0)
            {
                glDeleteSamplers(1, &mPoisonSampler);
            }
            for (const auto* textures : { &mTextures, &mIncompatibleTextures })
            {
                for (GLuint texture : *textures)
                {
                    if (texture != 0)
                    {
                        LLImageGL::deleteTextures(1, &texture);
                    }
                }
            }
        }
    };

    struct MaterialReadback
    {
        std::array<std::uint8_t, MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT>  mGBuffer0{};
        std::array<std::uint8_t, MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT>  mGBuffer1{};
        std::array<std::uint16_t, MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT> mGBuffer2{};
        std::array<std::uint32_t, MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT> mDepth{};

        friend bool operator==(const MaterialReadback&, const MaterialReadback&) = default;
    };

    struct SubmissionResult
    {
        bool   mPoisoned = false;
        bool   mAccepted = false;
        GLenum mError    = GL_NO_ERROR;
    };

    bool emitFailure(const std::string& reason)
    {
        const std::string result = "MATERIAL_CONTRACT_PARITY result=fail reason=" + reason;
        std::cout << result << std::endl;
        LL_INFOS("RenderContractParity") << result << LL_ENDL;
        return false;
    }

    bool initializeGeometry(MaterialResources& resources, const MaterialFixture& fixture)
    {
        resources.mGeometry = new LLVertexBuffer(MATERIAL_VERTEX_MASK);
        if (!resources.mGeometry->allocateBuffer(4, 6) || resources.mGeometry->getSize() != MATERIAL_VERTEX_BUFFER_SIZE ||
            resources.mGeometry->getIndicesSize() != MATERIAL_INDEX_BUFFER_SIZE ||
            resources.mGeometry->getOffset(LLVertexBuffer::TYPE_VERTEX) != MATERIAL_POSITION_OFFSET ||
            resources.mGeometry->getOffset(LLVertexBuffer::TYPE_NORMAL) != MATERIAL_NORMAL_OFFSET ||
            resources.mGeometry->getOffset(LLVertexBuffer::TYPE_TEXCOORD0) != MATERIAL_TEXCOORD0_OFFSET ||
            resources.mGeometry->getOffset(LLVertexBuffer::TYPE_TEXCOORD1) != MATERIAL_TEXCOORD1_OFFSET ||
            resources.mGeometry->getOffset(LLVertexBuffer::TYPE_TEXCOORD2) != MATERIAL_TEXCOORD2_OFFSET ||
            resources.mGeometry->getOffset(LLVertexBuffer::TYPE_COLOR) != MATERIAL_COLOR_OFFSET ||
            resources.mGeometry->getOffset(LLVertexBuffer::TYPE_TANGENT) != MATERIAL_TANGENT_OFFSET)
        {
            return false;
        }

        auto copy_attribute = [&](LLVertexBuffer::AttributeType type, std::size_t offset, std::size_t size)
        {
            U8* destination = resources.mGeometry->mapVertexBuffer(type, 0, 4);
            if (!destination)
            {
                return false;
            }
            std::memcpy(destination, fixture.mVertexBytes.data() + offset, size);
            return true;
        };

        const bool copied = copy_attribute(LLVertexBuffer::TYPE_VERTEX, MATERIAL_POSITION_OFFSET, 4 * sizeof(LLVector4)) &&
                            copy_attribute(LLVertexBuffer::TYPE_NORMAL, MATERIAL_NORMAL_OFFSET, 4 * sizeof(LLVector4)) &&
                            copy_attribute(LLVertexBuffer::TYPE_TEXCOORD0, MATERIAL_TEXCOORD0_OFFSET, 4 * sizeof(LLVector2)) &&
                            copy_attribute(LLVertexBuffer::TYPE_TEXCOORD1, MATERIAL_TEXCOORD1_OFFSET, 4 * sizeof(LLVector2)) &&
                            copy_attribute(LLVertexBuffer::TYPE_TEXCOORD2, MATERIAL_TEXCOORD2_OFFSET, 4 * sizeof(LLVector2)) &&
                            copy_attribute(LLVertexBuffer::TYPE_COLOR, MATERIAL_COLOR_OFFSET, 4 * sizeof(LLColor4U)) &&
                            copy_attribute(LLVertexBuffer::TYPE_TANGENT, MATERIAL_TANGENT_OFFSET, 4 * sizeof(LLVector4));
        U8* indices = resources.mGeometry->mapIndexBuffer(0, 6);
        if (indices)
        {
            std::memcpy(indices, fixture.mIndices.data(), sizeof(fixture.mIndices));
        }
        resources.mGeometry->unmapBuffer();
        return copied && indices != nullptr;
    }

    bool initializeIncompatibleGeometry(MaterialResources& resources)
    {
        resources.mIncompatibleGeometry = new LLVertexBuffer(MATERIAL_VERTEX_MASK & ~LLVertexBuffer::MAP_TANGENT);
        if (!resources.mIncompatibleGeometry->allocateBuffer(4, 6))
        {
            return false;
        }

        resources.mIncompatibleIndexGeometry = new LLVertexBuffer(MATERIAL_VERTEX_MASK);
        if (!resources.mIncompatibleIndexGeometry->allocateBuffer(4, 6))
        {
            return false;
        }
        constexpr std::array<U16, 6> incompatible_indices{ 0, 2, 1, 0, 3, 2 };
        U8*                          indices = resources.mIncompatibleIndexGeometry->mapIndexBuffer(0, 6);
        if (!indices)
        {
            return false;
        }
        std::memcpy(indices, incompatible_indices.data(), sizeof(incompatible_indices));
        resources.mIncompatibleIndexGeometry->unmapBuffer();
        return true;
    }

    bool initializeTextures(MaterialResources& resources, const MaterialFixture& fixture)
    {
        if (!gGLManager.mHasAnisotropic || gGLManager.mMaxAnisotropy < 8.f)
        {
            return false;
        }

        clear_glerror();
        glGenSamplers(1, &resources.mPoisonSampler);
        if (resources.mPoisonSampler == 0)
        {
            return false;
        }
        glSamplerParameteri(resources.mPoisonSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glSamplerParameteri(resources.mPoisonSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(resources.mPoisonSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(resources.mPoisonSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(resources.mPoisonSampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glSamplerParameterf(resources.mPoisonSampler, GL_TEXTURE_MIN_LOD, 1.f);
        glSamplerParameterf(resources.mPoisonSampler, GL_TEXTURE_MAX_LOD, 1.f);
        glSamplerParameterf(resources.mPoisonSampler, GL_TEXTURE_LOD_BIAS, 2.f);
        glSamplerParameterf(resources.mPoisonSampler, GL_TEXTURE_MAX_ANISOTROPY, 1.f);

        for (std::size_t texture_index = 0; texture_index < resources.mTextures.size(); ++texture_index)
        {
            GLuint& texture = resources.mTextures[texture_index];
            LLImageGL::generateTextures(1, &texture);
            if (texture == 0 || !gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, texture, true))
            {
                return false;
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MATERIAL_TEXTURE_MIP_LEVELS - 1);
            for (std::size_t mip = 0; mip < MATERIAL_TEXTURE_MIP_LEVELS; ++mip)
            {
                const GLsizei extent = static_cast<GLsizei>(MATERIAL_TEXTURE_WIDTH >> mip);
                LLImageGL::setManualImage(GL_TEXTURE_2D, static_cast<S32>(mip), GL_RGBA8, extent, extent, GL_RGBA, GL_UNSIGNED_BYTE,
                                          fixture.mTextureRGBA8[texture_index].data() + MATERIAL_TEXTURE_MIP_BYTE_OFFSETS[mip], false);
            }
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000.f);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1000.f);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.f);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 8.f);
        }
        return glGetError() == GL_NO_ERROR;
    }

    bool initializeIncompatibleTextures(MaterialResources& resources)
    {
        clear_glerror();
        LLImageGL::generateTextures(static_cast<S32>(resources.mIncompatibleTextures.size()), resources.mIncompatibleTextures.data());
        for (std::size_t texture_index = 0; texture_index < resources.mIncompatibleTextures.size(); ++texture_index)
        {
            const GLuint texture = resources.mIncompatibleTextures[texture_index];
            if (texture == 0 || !gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, texture, true))
            {
                return false;
            }

            const GLint   internal_format = texture_index == 0 ? GL_RGBA16 : GL_RGBA8;
            const GLsizei base_extent     = texture_index == 1 ? 8 : MATERIAL_TEXTURE_WIDTH;
            const GLsizei mip_levels      = texture_index == 2 ? MATERIAL_TEXTURE_MIP_LEVELS - 1 : MATERIAL_TEXTURE_MIP_LEVELS;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mip_levels - 1);
            for (GLint mip = 0; mip < mip_levels; ++mip)
            {
                const GLsizei extent = std::max<GLsizei>(1, base_extent >> mip);
                LLImageGL::setManualImage(GL_TEXTURE_2D, mip, internal_format, extent, extent, GL_RGBA, GL_UNSIGNED_BYTE, nullptr, false);
            }
        }
        return glGetError() == GL_NO_ERROR;
    }

    bool initializeTarget(MaterialResources& resources)
    {
        return resources.mTarget.allocate(MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_RGBA, true) &&
               resources.mTarget.addColorAttachment(GL_RGBA) && resources.mTarget.addColorAttachment(GL_RGBA16) &&
               resources.mTarget.isComplete() && resources.mTarget.getNumTextures() == 3 &&
               resources.mTarget.getColorFormat(0) == GL_RGBA && resources.mTarget.getColorFormat(1) == GL_RGBA &&
               resources.mTarget.getColorFormat(2) == GL_RGBA16 && resources.mTarget.getDepth() != 0;
    }

    bool initializeIncompatibleTargets(MaterialResources& resources)
    {
        const bool color_target =
            resources.mIncompatibleColorTarget.allocate(MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_RGBA16, true) &&
            resources.mIncompatibleColorTarget.addColorAttachment(GL_RGBA) &&
            resources.mIncompatibleColorTarget.addColorAttachment(GL_RGBA16);
        const bool depth_target = resources.mIncompatibleDepthTarget.allocate(MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_RGBA, true) &&
                                  resources.mIncompatibleDepthTarget.addColorAttachment(GL_RGBA) &&
                                  resources.mIncompatibleDepthTarget.addColorAttachment(GL_RGBA16);
        if (!color_target || !depth_target ||
            !gGL.getTexUnit(0)->bindManual(resources.mIncompatibleDepthTarget.getUsage(), resources.mIncompatibleDepthTarget.getDepth()))
        {
            return false;
        }
        LLImageGL::setManualImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_DEPTH_COMPONENT,
                                  GL_UNSIGNED_SHORT, nullptr, false);
        return glGetError() == GL_NO_ERROR;
    }

    bool initializeResources(MaterialResources& resources, const MaterialFixture& fixture)
    {
        return initializeGeometry(resources, fixture) && initializeIncompatibleGeometry(resources) &&
               initializeTextures(resources, fixture) && initializeIncompatibleTextures(resources) && initializeTarget(resources) &&
               initializeIncompatibleTargets(resources);
    }

    bool seedTarget(LLRenderTarget& target, const MaterialFixture& fixture)
    {
        std::array<GLfloat, MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT> depth{};
        std::transform(fixture.mDepth24.begin(), fixture.mDepth24.end(), depth.begin(), materialDepth24);

        GLint unpack_alignment = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_alignment);
        clear_glerror();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        gGL.getTexUnit(0)->bindManual(target.getUsage(), target.getTexture(0));
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE,
                        fixture.mGBuffer0SentinelRGBA8.data());
        gGL.getTexUnit(0)->bindManual(target.getUsage(), target.getTexture(1));
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE,
                        fixture.mGBuffer1SentinelRGBA8.data());
        gGL.getTexUnit(0)->bindManual(target.getUsage(), target.getTexture(2));
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_RGBA, GL_UNSIGNED_SHORT,
                        fixture.mGBuffer2SentinelRGBA16.data());
        gGL.getTexUnit(0)->bindManual(target.getUsage(), target.getDepth());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());

        const bool success = glGetError() == GL_NO_ERROR;
        glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
        return success;
    }

    bool readTarget(LLRenderTarget& target, MaterialReadback& readback)
    {
        std::array<GLfloat, MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT> depth{};
        GLint                                                          pack_alignment = 4;
        glGetIntegerv(GL_PACK_ALIGNMENT, &pack_alignment);
        clear_glerror();
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

        target.bindTarget();
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, readback.mGBuffer0.data());
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glReadPixels(0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, readback.mGBuffer1.data());
        glReadBuffer(GL_COLOR_ATTACHMENT2);
        glReadPixels(0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_RGBA, GL_UNSIGNED_SHORT, readback.mGBuffer2.data());
        glReadPixels(0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());
        const bool gl_success = glGetError() == GL_NO_ERROR;
        target.flush();
        glPixelStorei(GL_PACK_ALIGNMENT, pack_alignment);
        if (!gl_success)
        {
            return false;
        }

        for (std::size_t pixel = 0; pixel < depth.size(); ++pixel)
        {
            if (!std::isfinite(depth[pixel]) || depth[pixel] < 0.f || depth[pixel] > 1.f)
            {
                return false;
            }
            readback.mDepth[pixel] = static_cast<std::uint32_t>(std::llround(static_cast<double>(depth[pixel]) * DEPTH24_MAX));
        }
        return true;
    }

    MaterialArtifact artifactFrom(const MaterialReadback& readback)
    {
        MaterialArtifact artifact = makeMaterialArtifact();
        artifact.mGBuffer0RGBA8.reserve(readback.mGBuffer0.size());
        artifact.mGBuffer1RGBA8.reserve(readback.mGBuffer1.size());
        artifact.mGBuffer2RGBA16.reserve(readback.mGBuffer2.size());
        artifact.mDepth24.reserve(readback.mDepth.size());
        std::transform(readback.mGBuffer0.begin(), readback.mGBuffer0.end(), std::back_inserter(artifact.mGBuffer0RGBA8), materialUnorm8);
        std::transform(readback.mGBuffer1.begin(), readback.mGBuffer1.end(), std::back_inserter(artifact.mGBuffer1RGBA8), materialUnorm8);
        std::transform(readback.mGBuffer2.begin(), readback.mGBuffer2.end(), std::back_inserter(artifact.mGBuffer2RGBA16), materialUnorm16);
        std::transform(readback.mDepth.begin(), readback.mDepth.end(), std::back_inserter(artifact.mDepth24), materialDepth24);
        return artifact;
    }

    void eraseCachedUniform(LLGLSLShader& shader, U32 uniform)
    {
        const GLint location = shader.getUniformLocation(uniform);
        if (location >= 0)
        {
            shader.mValue.erase(location);
        }
    }

    void applyParameters(LLGLSLShader& shader, const MaterialParameters& parameters)
    {
        // drawRange() synchronizes viewer matrices. Synchronize first, then upload
        // the canonical matrices so that synchronization cannot overwrite them.
        gGL.syncMatrices();
        shader.uniformMatrix4fv(LLShaderMgr::MODELVIEW_MATRIX, 1, GL_FALSE, parameters.mModelviewMatrix.data());
        shader.uniformMatrix4fv(LLShaderMgr::MODELVIEW_PROJECTION_MATRIX, 1, GL_FALSE, parameters.mModelviewProjectionMatrix.data());
        shader.uniformMatrix3fv(LLShaderMgr::NORMAL_MATRIX, 1, GL_FALSE, parameters.mNormalMatrix.data());
        shader.uniformMatrix4fv(LLShaderMgr::TEXTURE_MATRIX0, 1, GL_FALSE, parameters.mTextureMatrix0.data());

        constexpr std::array<U32, 8> cached_uniforms{ LLShaderMgr::DIFFUSE_MAP,           LLShaderMgr::BUMP_MAP,
                                                      LLShaderMgr::SPECULAR_MAP,          LLShaderMgr::EMISSIVE_BRIGHTNESS,
                                                      LLShaderMgr::ENVIRONMENT_INTENSITY, LLShaderMgr::SPECULAR_COLOR,
                                                      LLShaderMgr::MIRROR_FLAG,           LLShaderMgr::CLIP_PLANE };
        for (U32 uniform : cached_uniforms)
        {
            eraseCachedUniform(shader, uniform);
        }

        shader.uniform1i(LLShaderMgr::DIFFUSE_MAP, LLRender::DIFFUSE_MAP);
        shader.uniform1i(LLShaderMgr::BUMP_MAP, LLRender::NORMAL_MAP);
        shader.uniform1i(LLShaderMgr::SPECULAR_MAP, LLRender::SPECULAR_MAP);
        shader.uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, parameters.mEmissiveBrightness);
        shader.uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, parameters.mEnvironmentIntensity);
        shader.uniform4fv(LLShaderMgr::SPECULAR_COLOR, 1, parameters.mSpecularColor.data());
        shader.uniform1f(LLShaderMgr::MIRROR_FLAG, parameters.mMirror);
        shader.uniform4fv(LLShaderMgr::CLIP_PLANE, 1, parameters.mClipPlane.data());
    }

    bool bindMaterialTexture(U32 unit_index, GLuint texture)
    {
        glBindSampler(unit_index, 0);
        LLTexUnit* unit = gGL.getTexUnit(unit_index);
        if (!unit->bindManual(LLTexUnit::TT_TEXTURE, texture, true))
        {
            return false;
        }
        unit->activate();
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MATERIAL_TEXTURE_MIP_LEVELS - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000.f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1000.f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.f);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 8.f);
        return true;
    }

    class DrawStateRestore
    {
    public:
        DrawStateRestore()
        {
            for (std::size_t i = 0; i < mCapabilities.size(); ++i)
            {
                mEnabled[i] = glIsEnabled(mCapabilities[i]);
            }
            glGetIntegerv(GL_VIEWPORT, mViewport.data());
            glGetIntegerv(GL_SCISSOR_BOX, mScissor.data());
            glGetBooleanv(GL_COLOR_WRITEMASK, mColorMask.data());
            glGetBooleanv(GL_DEPTH_WRITEMASK, &mDepthMask);
            glGetIntegerv(GL_DEPTH_FUNC, &mDepthFunction);
            glGetDoublev(GL_DEPTH_RANGE, mDepthRange.data());
            glGetIntegerv(GL_CULL_FACE_MODE, &mCullFace);
            glGetIntegerv(GL_FRONT_FACE, &mFrontFace);
            glGetIntegerv(GL_POLYGON_MODE, mPolygonMode.data());
            glGetIntegerv(GL_PROVOKING_VERTEX, &mProvokingVertex);
            glGetIntegerv(GL_BLEND_SRC_RGB, &mBlendSourceRgb);
            glGetIntegerv(GL_BLEND_DST_RGB, &mBlendDestinationRgb);
            glGetIntegerv(GL_BLEND_SRC_ALPHA, &mBlendSourceAlpha);
            glGetIntegerv(GL_BLEND_DST_ALPHA, &mBlendDestinationAlpha);
            glGetIntegerv(GL_BLEND_EQUATION_RGB, &mBlendEquationRgb);
            glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &mBlendEquationAlpha);
        }

        ~DrawStateRestore()
        {
            for (std::size_t i = 0; i < mCapabilities.size(); ++i)
            {
                mEnabled[i] ? glEnable(mCapabilities[i]) : glDisable(mCapabilities[i]);
            }
            glViewport(mViewport[0], mViewport[1], mViewport[2], mViewport[3]);
            glScissor(mScissor[0], mScissor[1], mScissor[2], mScissor[3]);
            glColorMask(mColorMask[0], mColorMask[1], mColorMask[2], mColorMask[3]);
            glDepthMask(mDepthMask);
            glDepthFunc(static_cast<GLenum>(mDepthFunction));
            glDepthRange(mDepthRange[0], mDepthRange[1]);
            glCullFace(static_cast<GLenum>(mCullFace));
            glFrontFace(static_cast<GLenum>(mFrontFace));
            glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(mPolygonMode[0]));
            glProvokingVertex(static_cast<GLenum>(mProvokingVertex));
            glBlendFuncSeparate(static_cast<GLenum>(mBlendSourceRgb), static_cast<GLenum>(mBlendDestinationRgb),
                                static_cast<GLenum>(mBlendSourceAlpha), static_cast<GLenum>(mBlendDestinationAlpha));
            glBlendEquationSeparate(static_cast<GLenum>(mBlendEquationRgb), static_cast<GLenum>(mBlendEquationAlpha));
        }

        void applyMaterialState()
        {
            constexpr std::array<GLboolean, 15> enabled{ GL_FALSE, GL_TRUE,  GL_TRUE,  GL_TRUE,  GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE,
                                                         GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE };
            applyCapabilities(enabled);
            glViewport(0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT);
            glScissor(0, 0, MATERIAL_FRAME_WIDTH, MATERIAL_FRAME_HEIGHT);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
            glDepthRange(0.0, 1.0);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glProvokingVertex(GL_LAST_VERTEX_CONVENTION);
            glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
            glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        }

        void applyPoisonState()
        {
            constexpr std::array<GLboolean, 15> enabled{ GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE,
                                                         GL_TRUE, GL_TRUE,  GL_TRUE,  GL_TRUE,  GL_TRUE, GL_TRUE, GL_TRUE };
            applyCapabilities(enabled);
            glViewport(1, 2, 3, 4);
            glScissor(2, 1, 1, 2);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_ALWAYS);
            glDepthRange(0.75, 0.25);
            glCullFace(GL_FRONT);
            glFrontFace(GL_CW);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
            glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_SUBTRACT);
        }

    private:
        void applyCapabilities(const std::array<GLboolean, 15>& enabled)
        {
            for (std::size_t i = 0; i < mCapabilities.size(); ++i)
            {
                enabled[i] ? glEnable(mCapabilities[i]) : glDisable(mCapabilities[i]);
            }
        }

        const std::array<GLenum, 15> mCapabilities{ GL_BLEND,
                                                    GL_CULL_FACE,
                                                    GL_DEPTH_TEST,
                                                    GL_SCISSOR_TEST,
                                                    GL_STENCIL_TEST,
                                                    GL_POLYGON_OFFSET_FILL,
                                                    GL_SAMPLE_ALPHA_TO_COVERAGE,
                                                    GL_SAMPLE_COVERAGE,
                                                    GL_FRAMEBUFFER_SRGB,
                                                    GL_COLOR_LOGIC_OP,
                                                    GL_RASTERIZER_DISCARD,
                                                    GL_PRIMITIVE_RESTART,
                                                    GL_DEPTH_CLAMP,
                                                    GL_DITHER,
                                                    GL_MULTISAMPLE };
        std::array<GLboolean, 15>    mEnabled{};
        std::array<GLint, 4>         mViewport{};
        std::array<GLint, 4>         mScissor{};
        std::array<GLboolean, 4>     mColorMask{};
        GLboolean                    mDepthMask     = GL_TRUE;
        GLint                        mDepthFunction = GL_LESS;
        std::array<GLdouble, 2>      mDepthRange{};
        GLint                        mCullFace  = GL_BACK;
        GLint                        mFrontFace = GL_CCW;
        std::array<GLint, 2>         mPolygonMode{};
        GLint                        mProvokingVertex       = GL_LAST_VERTEX_CONVENTION;
        GLint                        mBlendSourceRgb        = GL_ONE;
        GLint                        mBlendDestinationRgb   = GL_ZERO;
        GLint                        mBlendSourceAlpha      = GL_ONE;
        GLint                        mBlendDestinationAlpha = GL_ZERO;
        GLint                        mBlendEquationRgb      = GL_FUNC_ADD;
        GLint                        mBlendEquationAlpha    = GL_FUNC_ADD;
    };

    bool submitLegacy(MaterialResources& resources, LLGLSLShader& shader, const FrameSnapshot& frame)
    {
        const std::optional<MaterialInputs> inputs = decodeMaterialFrame(frame);
        if (!inputs)
        {
            return false;
        }

        shader.bind();
        resources.mTarget.bindTarget();
        bool bound = false;
        {
            DrawStateRestore state;
            state.applyMaterialState();

            constexpr GLfloat clear[4] = { 0.f, 0.f, 0.f, 0.f };
            glClearBufferfv(GL_COLOR, 0, clear);
            glClearBufferfv(GL_COLOR, 1, clear);
            glClearBufferfv(GL_COLOR, 2, clear);

            applyParameters(shader, inputs->mParameters);
            bound = bindMaterialTexture(LLRender::DIFFUSE_MAP, resources.mTextures[0]) &&
                    bindMaterialTexture(LLRender::NORMAL_MAP, resources.mTextures[1]) &&
                    bindMaterialTexture(LLRender::SPECULAR_MAP, resources.mTextures[2]);
            if (bound)
            {
                resources.mGeometry->setBuffer();
                for (GLuint location : { GLuint{ LLVertexBuffer::TYPE_VERTEX }, GLuint{ LLVertexBuffer::TYPE_NORMAL },
                                         GLuint{ LLVertexBuffer::TYPE_TEXCOORD0 }, GLuint{ LLVertexBuffer::TYPE_TEXCOORD1 },
                                         GLuint{ LLVertexBuffer::TYPE_TEXCOORD2 }, GLuint{ LLVertexBuffer::TYPE_COLOR },
                                         GLuint{ LLVertexBuffer::TYPE_TANGENT } })
                {
                    glVertexAttribDivisor(location, 0);
                }
                resources.mGeometry->drawRange(LLRender::TRIANGLES, 0, 3, 6, 0);
            }

            for (U32 unit : { U32{ LLRender::DIFFUSE_MAP }, U32{ LLRender::NORMAL_MAP }, U32{ LLRender::SPECULAR_MAP } })
            {
                glBindSampler(unit, 0);
                gGL.getTexUnit(unit)->unbind(LLTexUnit::TT_TEXTURE);
            }
            LLGLSLShader::unbind();
        }
        resources.mTarget.flush();
        return bound;
    }

    class TextureBindingRestore
    {
    public:
        TextureBindingRestore() : mActiveUnit(gGL.getCurrentTexUnitIndex())
        {
            for (U32 unit = 0; unit < mTextureBindings.size(); ++unit)
            {
                gGL.getTexUnit(unit)->activate();
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &mTextureBindings[unit]);
                glGetIntegeri_v(GL_SAMPLER_BINDING, unit, &mSamplerBindings[unit]);
            }
            gGL.getTexUnit(mActiveUnit)->activate();
        }

        ~TextureBindingRestore()
        {
            for (U32 unit = 0; unit < mTextureBindings.size(); ++unit)
            {
                LLTexUnit* texture_unit = gGL.getTexUnit(unit);
                texture_unit->bindManual(LLTexUnit::TT_TEXTURE, static_cast<GLuint>(mTextureBindings[unit]), true);
                texture_unit->activate();
                // unbind(TT_TEXTURE) substitutes the viewer white texture. Restore
                // the exact raw binding too, including an original zero binding.
                glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(mTextureBindings[unit]));
                glBindSampler(unit, static_cast<GLuint>(mSamplerBindings[unit]));
            }
            gGL.getTexUnit(mActiveUnit)->activate();
        }

    private:
        U32                                       mActiveUnit = 0;
        std::array<GLint, MATERIAL_TEXTURE_COUNT> mTextureBindings{};
        std::array<GLint, MATERIAL_TEXTURE_COUNT> mSamplerBindings{};
    };

    bool poisonTexture(U32 unit_index, GLuint texture, GLuint sampler)
    {
        LLTexUnit* unit = gGL.getTexUnit(unit_index);
        if (!unit->bindManual(LLTexUnit::TT_TEXTURE, texture, true))
        {
            return false;
        }
        unit->activate();
        glBindSampler(unit_index, sampler);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 1.f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1.f);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 2.f);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ALPHA);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 1.f);
        return true;
    }

    bool poisonBindings(MaterialResources& resources, LLGLSLShader& shader)
    {
        MaterialParameters poison;
        poison.mModelviewMatrix.fill(0.25f);
        poison.mModelviewProjectionMatrix.fill(-0.5f);
        poison.mNormalMatrix.fill(0.75f);
        poison.mTextureMatrix0.fill(0.125f);
        poison.mSpecularColor        = { 0.91f, 0.07f, 0.43f, 0.19f };
        poison.mClipPlane            = { -0.4f, 0.3f, -0.2f, -8.f };
        poison.mEnvironmentIntensity = 0.03f;
        poison.mEmissiveBrightness   = 0.97f;
        poison.mMirror               = 1.f;

        clear_glerror();
        shader.bind();
        applyParameters(shader, poison);
        const bool bound = poisonTexture(LLRender::DIFFUSE_MAP, resources.mTextures[2], resources.mPoisonSampler) &&
                           poisonTexture(LLRender::NORMAL_MAP, resources.mTextures[0], resources.mPoisonSampler) &&
                           poisonTexture(LLRender::SPECULAR_MAP, resources.mTextures[1], resources.mPoisonSampler);
        resources.mGeometry->setBuffer();
        glVertexAttribDivisor(LLVertexBuffer::TYPE_VERTEX, 2);
        glVertexAttribDivisor(LLVertexBuffer::TYPE_TANGENT, 1);
        LLGLSLShader::unbind();
        return bound && glGetError() == GL_NO_ERROR;
    }

    template<typename Submit>
    SubmissionResult submitPoisoned(MaterialResources& poison_resources, LLGLSLShader& shader, Submit&& submit)
    {
        SubmissionResult      result;
        TextureBindingRestore bindings;
        result.mPoisoned = poisonBindings(poison_resources, shader);
        if (!result.mPoisoned)
        {
            return result;
        }

        {
            DrawStateRestore poison;
            clear_glerror();
            poison_resources.mTarget.bindTarget();
            if (glGetError() == GL_NO_ERROR)
            {
                poison.applyPoisonState();
                result.mAccepted = submit();
                result.mError    = glGetError();
            }
            else
            {
                result.mError = GL_INVALID_FRAMEBUFFER_OPERATION;
            }
            poison_resources.mTarget.flush();
            if (const GLenum restore_error = glGetError(); result.mError == GL_NO_ERROR)
            {
                result.mError = restore_error;
            }
        }
        return result;
    }

    template<typename Handle>
    Handle stale(Handle handle)
    {
        ++handle.mGeneration;
        return handle;
    }

    enum class RegistryMutation
    {
        Unchanged,
        StaleVertex,
        StaleIndex,
        StaleDiffuse,
        StaleNormal,
        StaleSpecular,
        StaleSampler,
        StalePipeline,
        StaleGBuffer0,
        StaleGBuffer1,
        StaleGBuffer2,
        StaleDepth,
        WrongProgram,
        WrongVariant,
        WrongFormat,
        WrongExtent,
        WrongMips,
        WrongSampler,
        LiveWrongLayout,
        LiveWrongIndices,
        LiveWrongTextureFormat,
        LiveWrongTextureExtent,
        LiveWrongTextureMips,
        LiveWrongColorTarget,
        LiveWrongDepthTarget
    };

    bool registerResources(LLRenderGLMaterial::Registry& registry,
                           const MaterialInputs&         inputs,
                           MaterialResources&            resources,
                           LLGLSLShader&                 shader,
                           RegistryMutation              mutation)
    {
        MaterialHandles handles = inputs.mHandles;
        if (mutation == RegistryMutation::StaleVertex)
            handles.mVertexBuffer = stale(handles.mVertexBuffer);
        if (mutation == RegistryMutation::StaleIndex)
            handles.mIndexBuffer = stale(handles.mIndexBuffer);
        if (mutation == RegistryMutation::StaleDiffuse)
            handles.mDiffuse = stale(handles.mDiffuse);
        if (mutation == RegistryMutation::StaleNormal)
            handles.mNormal = stale(handles.mNormal);
        if (mutation == RegistryMutation::StaleSpecular)
            handles.mSpecular = stale(handles.mSpecular);
        if (mutation == RegistryMutation::StaleSampler)
            handles.mSampler = stale(handles.mSampler);
        if (mutation == RegistryMutation::StalePipeline)
            handles.mPipeline = stale(handles.mPipeline);
        if (mutation == RegistryMutation::StaleGBuffer0)
            handles.mGBuffer0 = stale(handles.mGBuffer0);
        if (mutation == RegistryMutation::StaleGBuffer1)
            handles.mGBuffer1 = stale(handles.mGBuffer1);
        if (mutation == RegistryMutation::StaleGBuffer2)
            handles.mGBuffer2 = stale(handles.mGBuffer2);
        if (mutation == RegistryMutation::StaleDepth)
            handles.mDepth = stale(handles.mDepth);

        std::array<LLRenderGLMaterial::SampledImage, MATERIAL_TEXTURE_COUNT> images{};
        for (std::size_t image = 0; image < images.size(); ++image)
        {
            images[image] = { resources.mTextures[image],
                              PixelFormat::RGBA8Unorm,
                              { MATERIAL_TEXTURE_WIDTH, MATERIAL_TEXTURE_HEIGHT },
                              MATERIAL_TEXTURE_MIP_LEVELS };
        }
        if (mutation == RegistryMutation::WrongFormat)
            images[0].mFormat = PixelFormat::RGBA16Unorm;
        if (mutation == RegistryMutation::WrongExtent)
            images[0].mExtent.mWidth = MATERIAL_TEXTURE_WIDTH / 2;
        if (mutation == RegistryMutation::WrongMips)
            --images[0].mMipLevels;
        if (mutation == RegistryMutation::LiveWrongTextureFormat)
            images[0].mTexture = resources.mIncompatibleTextures[0];
        if (mutation == RegistryMutation::LiveWrongTextureExtent)
            images[0].mTexture = resources.mIncompatibleTextures[1];
        if (mutation == RegistryMutation::LiveWrongTextureMips)
            images[0].mTexture = resources.mIncompatibleTextures[2];

        LLRenderGLMaterial::Sampler sampler;
        if (mutation == RegistryMutation::WrongSampler)
        {
            sampler.mMinFilter     = Filter::Nearest;
            sampler.mMaxAnisotropy = 1.f;
        }

        ShaderProgramKey program{ MATERIAL_PROGRAM, 0 };
        if (mutation == RegistryMutation::WrongProgram)
            program.mName = "deferred.material.debug";
        if (mutation == RegistryMutation::WrongVariant)
            program.mVariant = 1;

        LLVertexBuffer* geometry = resources.mGeometry.get();
        if (mutation == RegistryMutation::LiveWrongLayout)
            geometry = resources.mIncompatibleGeometry.get();
        if (mutation == RegistryMutation::LiveWrongIndices)
            geometry = resources.mIncompatibleIndexGeometry.get();

        LLRenderTarget* target = &resources.mTarget;
        if (mutation == RegistryMutation::LiveWrongColorTarget)
            target = &resources.mIncompatibleColorTarget;
        if (mutation == RegistryMutation::LiveWrongDepthTarget)
            target = &resources.mIncompatibleDepthTarget;

        return registry.addVertexBuffer(handles.mVertexBuffer, handles.mIndexBuffer, geometry) &&
               registry.addSampledImage(handles.mDiffuse, images[0]) && registry.addSampledImage(handles.mNormal, images[1]) &&
               registry.addSampledImage(handles.mSpecular, images[2]) &&
               registry.addRenderTarget({ handles.mGBuffer0, handles.mGBuffer1, handles.mGBuffer2 }, handles.mDepth, target) &&
               registry.addSampler(handles.mSampler, sampler) && registry.addPipeline(handles.mPipeline, std::move(program), &shader);
    }

    enum class FrameMutation
    {
        Unchanged,
        WrongLayout,
        WrongImageRange,
        WrongIndexRange,
        WrongParameterSize
    };

    void mutateFrame(FrameSnapshot& frame, FrameMutation mutation)
    {
        if (mutation == FrameMutation::WrongLayout)
        {
            frame.mPipelines.front().mVertexBindings.front().mStride = 12;
        }
        else if (mutation == FrameMutation::WrongImageRange)
        {
            DrawIndexed& draw                                            = std::get<DrawIndexed>(frame.mPasses.front().mDraws.front());
            draw.mResources.mSampledImages.front().mRange.mMipLevelCount = MATERIAL_TEXTURE_MIP_LEVELS - 1;
        }
        else if (mutation == FrameMutation::WrongIndexRange)
        {
            DrawIndexed& draw = std::get<DrawIndexed>(frame.mPasses.front().mDraws.front());
            draw.mFirstIndex  = 1;
        }
        else if (mutation == FrameMutation::WrongParameterSize)
        {
            DrawIndexed& draw = std::get<DrawIndexed>(frame.mPasses.front().mDraws.front());
            --draw.mResources.mParameters.front().mBytes.mSize;
        }
    }

    struct RejectionSpec
    {
        const char*      mName;
        RegistryMutation mRegistryMutation;
        FrameMutation    mFrameMutation;
    };

    constexpr std::array REJECTIONS{
        RejectionSpec{ "stale_vertex", RegistryMutation::StaleVertex, FrameMutation::Unchanged },
        RejectionSpec{ "stale_index", RegistryMutation::StaleIndex, FrameMutation::Unchanged },
        RejectionSpec{ "stale_diffuse", RegistryMutation::StaleDiffuse, FrameMutation::Unchanged },
        RejectionSpec{ "stale_normal", RegistryMutation::StaleNormal, FrameMutation::Unchanged },
        RejectionSpec{ "stale_specular", RegistryMutation::StaleSpecular, FrameMutation::Unchanged },
        RejectionSpec{ "stale_sampler", RegistryMutation::StaleSampler, FrameMutation::Unchanged },
        RejectionSpec{ "stale_pipeline", RegistryMutation::StalePipeline, FrameMutation::Unchanged },
        RejectionSpec{ "stale_gbuffer0", RegistryMutation::StaleGBuffer0, FrameMutation::Unchanged },
        RejectionSpec{ "stale_gbuffer1", RegistryMutation::StaleGBuffer1, FrameMutation::Unchanged },
        RejectionSpec{ "stale_gbuffer2", RegistryMutation::StaleGBuffer2, FrameMutation::Unchanged },
        RejectionSpec{ "stale_depth", RegistryMutation::StaleDepth, FrameMutation::Unchanged },
        RejectionSpec{ "wrong_program", RegistryMutation::WrongProgram, FrameMutation::Unchanged },
        RejectionSpec{ "wrong_variant", RegistryMutation::WrongVariant, FrameMutation::Unchanged },
        RejectionSpec{ "wrong_format", RegistryMutation::WrongFormat, FrameMutation::Unchanged },
        RejectionSpec{ "wrong_extent", RegistryMutation::WrongExtent, FrameMutation::Unchanged },
        RejectionSpec{ "wrong_layout", RegistryMutation::Unchanged, FrameMutation::WrongLayout },
        RejectionSpec{ "wrong_image_range", RegistryMutation::Unchanged, FrameMutation::WrongImageRange },
        RejectionSpec{ "wrong_index_range", RegistryMutation::Unchanged, FrameMutation::WrongIndexRange },
        RejectionSpec{ "wrong_mips", RegistryMutation::WrongMips, FrameMutation::Unchanged },
        RejectionSpec{ "wrong_sampler", RegistryMutation::WrongSampler, FrameMutation::Unchanged },
        RejectionSpec{ "live_wrong_layout", RegistryMutation::LiveWrongLayout, FrameMutation::Unchanged },
        RejectionSpec{ "live_wrong_indices", RegistryMutation::LiveWrongIndices, FrameMutation::Unchanged },
        RejectionSpec{ "live_wrong_texture_format", RegistryMutation::LiveWrongTextureFormat, FrameMutation::Unchanged },
        RejectionSpec{ "live_wrong_texture_extent", RegistryMutation::LiveWrongTextureExtent, FrameMutation::Unchanged },
        RejectionSpec{ "live_wrong_texture_mips", RegistryMutation::LiveWrongTextureMips, FrameMutation::Unchanged },
        RejectionSpec{ "live_wrong_color_target", RegistryMutation::LiveWrongColorTarget, FrameMutation::Unchanged },
        RejectionSpec{ "live_wrong_depth_target", RegistryMutation::LiveWrongDepthTarget, FrameMutation::Unchanged },
        RejectionSpec{ "wrong_parameter_size", RegistryMutation::Unchanged, FrameMutation::WrongParameterSize }
    };

    template<typename Code, std::size_t Size>
    void comparePlane(const std::array<Code, Size>& reference,
                      const std::array<Code, Size>& candidate,
                      const char*                   plane,
                      U64&                          mismatches,
                      double&                       max_abs_delta,
                      std::string&                  first_failure)
    {
        constexpr double denominator = static_cast<double>(std::numeric_limits<Code>::max());
        for (std::size_t component = 0; component < Size; ++component)
        {
            if (reference[component] == candidate[component])
            {
                continue;
            }
            ++mismatches;
            max_abs_delta =
                std::max(max_abs_delta,
                         std::abs(static_cast<double>(reference[component]) - static_cast<double>(candidate[component])) / denominator);
            if (first_failure.empty())
            {
                std::ostringstream failure;
                failure << "parity plane=" << plane << " pixel=" << component / MATERIAL_DIAGNOSTIC_CHANNELS
                        << " channel=" << component % MATERIAL_DIAGNOSTIC_CHANNELS << " legacy=" << static_cast<U64>(reference[component])
                        << " contract=" << static_cast<U64>(candidate[component]);
                first_failure = failure.str();
            }
        }
    }

    void compareDepth(const MaterialReadback& reference,
                      const MaterialReadback& candidate,
                      U64&                    mismatches,
                      double&                 max_abs_delta,
                      std::string&            first_failure)
    {
        for (std::size_t pixel = 0; pixel < reference.mDepth.size(); ++pixel)
        {
            if (reference.mDepth[pixel] == candidate.mDepth[pixel])
            {
                continue;
            }
            ++mismatches;
            max_abs_delta = std::max(max_abs_delta,
                                     std::abs(static_cast<double>(reference.mDepth[pixel]) - static_cast<double>(candidate.mDepth[pixel])) /
                                         DEPTH24_MAX);
            if (first_failure.empty())
            {
                std::ostringstream failure;
                failure << "parity plane=depth pixel=" << pixel << " legacy=" << reference.mDepth[pixel]
                        << " contract=" << candidate.mDepth[pixel];
                first_failure = failure.str();
            }
        }
    }

    template<typename Code, std::size_t Size, std::size_t DepthSize>
    bool hasDistinctWrittenPixels(const std::array<Code, Size>&               values,
                                  const std::array<Code, Size>&               sentinel,
                                  const std::array<std::uint32_t, DepthSize>& depth,
                                  const std::array<std::uint32_t, DepthSize>& depth_sentinel)
    {
        static_assert(Size % MATERIAL_DIAGNOSTIC_CHANNELS == 0);
        static_assert(Size / MATERIAL_DIAGNOSTIC_CHANNELS == DepthSize);
        std::size_t first_pixel           = DepthSize;
        bool        changed_from_sentinel = false;
        bool        distinct              = false;
        for (std::size_t pixel = 0; pixel < DepthSize; ++pixel)
        {
            if (depth[pixel] == depth_sentinel[pixel])
            {
                continue;
            }
            if (first_pixel == DepthSize)
            {
                first_pixel = pixel;
            }
            for (std::size_t channel = 0; channel < MATERIAL_DIAGNOSTIC_CHANNELS; ++channel)
            {
                const std::size_t component       = pixel * MATERIAL_DIAGNOSTIC_CHANNELS + channel;
                const std::size_t first_component = first_pixel * MATERIAL_DIAGNOSTIC_CHANNELS + channel;
                changed_from_sentinel             = changed_from_sentinel || values[component] != sentinel[component];
                distinct                          = distinct || values[component] != values[first_component];
            }
        }
        return changed_from_sentinel && distinct;
    }

    struct DepthGate
    {
        U32  mPasses              = 0;
        U32  mFailures            = 0;
        U32  mMirrorClippedPasses = 0;
        bool mValid               = false;
    };

    DepthGate verifyDepthGate(const MaterialFixture& fixture, const MaterialReadback& readback)
    {
        struct Vertex
        {
            double                mX;
            double                mY;
            double                mZ;
            double                mReciprocalW;
            std::array<double, 3> mViewOverW;
        };
        std::array<float, 16> positions{};
        std::memcpy(positions.data(), fixture.mVertexBytes.data() + MATERIAL_POSITION_OFFSET, sizeof(positions));
        std::array<Vertex, 4> vertices{};
        auto                  transform = [](const std::array<float, 16>& matrix, const float* value)
        {
            std::array<double, 4> result{};
            for (std::size_t row = 0; row < result.size(); ++row)
            {
                for (std::size_t column = 0; column < result.size(); ++column)
                {
                    result[row] += static_cast<double>(matrix[column * 4 + row]) * value[column];
                }
            }
            return result;
        };
        for (std::size_t vertex = 0; vertex < vertices.size(); ++vertex)
        {
            const float*                position     = positions.data() + vertex * 4;
            const std::array<double, 4> clip         = transform(fixture.mParameters.mModelviewProjectionMatrix, position);
            const std::array<double, 4> view         = transform(fixture.mParameters.mModelviewMatrix, position);
            const double                reciprocal_w = 1.0 / clip[3];
            vertices[vertex]                         = { (clip[0] * reciprocal_w * 0.5 + 0.5) * MATERIAL_FRAME_WIDTH,
                                                         (clip[1] * reciprocal_w * 0.5 + 0.5) * MATERIAL_FRAME_HEIGHT,
                                                         clip[2] * reciprocal_w * 0.5 + 0.5,
                                                         reciprocal_w,
                                                         { view[0] * reciprocal_w, view[1] * reciprocal_w, view[2] * reciprocal_w } };
        }

        auto cross = [](const Vertex& first, const Vertex& second, double x, double y)
        {
            return (second.mX - first.mX) * (y - first.mY) - (second.mY - first.mY) * (x - first.mX);
        };

        DepthGate gate;
        for (U32 y = 0; y < MATERIAL_FRAME_HEIGHT; ++y)
        {
            for (U32 x = 0; x < MATERIAL_FRAME_WIDTH; ++x)
            {
                const double sample_x       = x + 0.5;
                const double sample_y       = y + 0.5;
                bool         covered        = false;
                double       fragment_depth = 0.0;
                for (std::size_t triangle = 0; triangle < fixture.mIndices.size(); triangle += 3)
                {
                    const Vertex& a    = vertices[fixture.mIndices[triangle]];
                    const Vertex& b    = vertices[fixture.mIndices[triangle + 1]];
                    const Vertex& c    = vertices[fixture.mIndices[triangle + 2]];
                    const double  area = cross(a, b, c.mX, c.mY);
                    const double  wa   = cross(b, c, sample_x, sample_y) / area;
                    const double  wb   = cross(c, a, sample_x, sample_y) / area;
                    const double  wc   = 1.0 - wa - wb;
                    if (std::min({ wa, wb, wc }) <= 0.08)
                    {
                        continue;
                    }
                    fragment_depth                     = wa * a.mZ + wb * b.mZ + wc * c.mZ;
                    const double          reciprocal_w = wa * a.mReciprocalW + wb * b.mReciprocalW + wc * c.mReciprocalW;
                    std::array<double, 3> view_position{};
                    for (std::size_t component = 0; component < view_position.size(); ++component)
                    {
                        view_position[component] =
                            (wa * a.mViewOverW[component] + wb * b.mViewOverW[component] + wc * c.mViewOverW[component]) / reciprocal_w;
                    }
                    const auto&  clip_plane    = fixture.mParameters.mClipPlane;
                    const double clip_distance = view_position[0] * clip_plane[0] + view_position[1] * clip_plane[1] +
                                                 view_position[2] * clip_plane[2] + clip_plane[3];
                    if (fixture.mParameters.mMirror > 0.f && clip_distance < 0.0)
                    {
                        const std::size_t pixel        = y * MATERIAL_FRAME_WIDTH + x;
                        const double      loaded_depth = materialDepth24(fixture.mDepth24[pixel]);
                        if (std::abs(fragment_depth - loaded_depth) >= 0.02 && fragment_depth <= loaded_depth)
                        {
                            if (readback.mDepth[pixel] != fixture.mDepth24[pixel])
                            {
                                return gate;
                            }
                            ++gate.mMirrorClippedPasses;
                        }
                        covered = false;
                        break;
                    }
                    covered = true;
                    break;
                }
                if (!covered)
                {
                    continue;
                }

                const std::size_t pixel        = y * MATERIAL_FRAME_WIDTH + x;
                const double      loaded_depth = materialDepth24(fixture.mDepth24[pixel]);
                if (std::abs(fragment_depth - loaded_depth) < 0.02)
                {
                    continue;
                }
                const bool expected_pass = fragment_depth <= loaded_depth;
                const bool depth_changed = readback.mDepth[pixel] != fixture.mDepth24[pixel];
                if (expected_pass != depth_changed)
                {
                    return gate;
                }
                if (expected_pass)
                {
                    ++gate.mPasses;
                }
                else
                {
                    ++gate.mFailures;
                }
            }
        }
        gate.mValid = gate.mPasses > 0 && gate.mFailures > 0 && gate.mMirrorClippedPasses > 0;
        return gate;
    }

    bool nontrivialOutput(const MaterialFixture& fixture, const MaterialReadback& readback, DepthGate& depth_gate)
    {
        depth_gate = verifyDepthGate(fixture, readback);
        return readback.mGBuffer0 != fixture.mGBuffer0SentinelRGBA8 && readback.mGBuffer1 != fixture.mGBuffer1SentinelRGBA8 &&
               readback.mGBuffer2 != fixture.mGBuffer2SentinelRGBA16 &&
               hasDistinctWrittenPixels(readback.mGBuffer0, fixture.mGBuffer0SentinelRGBA8, readback.mDepth, fixture.mDepth24) &&
               hasDistinctWrittenPixels(readback.mGBuffer1, fixture.mGBuffer1SentinelRGBA8, readback.mDepth, fixture.mDepth24) &&
               hasDistinctWrittenPixels(readback.mGBuffer2, fixture.mGBuffer2SentinelRGBA16, readback.mDepth, fixture.mDepth24) &&
               depth_gate.mValid;
    }

} // namespace

bool run()
{
    const MaterialFixture fixture       = makeMaterialFixture();
    const MaterialCase    material_case = makeMaterialCase();
    LLGLSLShader&         shader        = gDeferredMaterialProgram[12];

    MaterialResources legacy;
    MaterialResources contract;
    if (!initializeResources(legacy, fixture) || !initializeResources(contract, fixture) || !seedTarget(legacy.mTarget, fixture) ||
        !seedTarget(contract.mTarget, fixture))
    {
        return emitFailure("fixture_setup");
    }

    LLRenderGLMaterial::Registry registry;
    if (!registerResources(registry, material_case.mInputs, contract, shader, RegistryMutation::Unchanged))
    {
        return emitFailure("registry_setup");
    }

    const SubmissionResult legacy_submission =
        submitPoisoned(contract, shader, [&]() { return submitLegacy(legacy, shader, material_case.mFrame); });
    const SubmissionResult contract_submission =
        submitPoisoned(legacy, shader, [&]() { return LLRenderGLMaterial::execute(material_case.mFrame, registry); });
    if (!legacy_submission.mPoisoned || !legacy_submission.mAccepted || legacy_submission.mError != GL_NO_ERROR ||
        !contract_submission.mPoisoned || !contract_submission.mAccepted || contract_submission.mError != GL_NO_ERROR)
    {
        std::ostringstream failure;
        failure << "valid_submission"
                << " legacy_poisoned=" << legacy_submission.mPoisoned << " legacy_accepted=" << legacy_submission.mAccepted
                << " legacy_gl_error=0x" << std::hex << legacy_submission.mError << " contract_poisoned=" << std::dec
                << contract_submission.mPoisoned << " contract_accepted=" << contract_submission.mAccepted << " contract_gl_error=0x"
                << std::hex << contract_submission.mError;
        return emitFailure(failure.str());
    }

    MaterialReadback legacy_readback;
    MaterialReadback contract_readback;
    if (!readTarget(legacy.mTarget, legacy_readback) || !readTarget(contract.mTarget, contract_readback))
    {
        return emitFailure("valid_readback");
    }

    U64         mismatches    = 0;
    double      max_abs_delta = 0.0;
    std::string first_failure;
    comparePlane(legacy_readback.mGBuffer0, contract_readback.mGBuffer0, "gbuffer0", mismatches, max_abs_delta, first_failure);
    comparePlane(legacy_readback.mGBuffer1, contract_readback.mGBuffer1, "gbuffer1", mismatches, max_abs_delta, first_failure);
    comparePlane(legacy_readback.mGBuffer2, contract_readback.mGBuffer2, "gbuffer2", mismatches, max_abs_delta, first_failure);
    compareDepth(legacy_readback, contract_readback, mismatches, max_abs_delta, first_failure);

    DepthGate  depth_gate;
    const bool nontrivial = nontrivialOutput(fixture, legacy_readback, depth_gate);
    if (!nontrivial && first_failure.empty())
    {
        first_failure = "nontrivial_output_gate";
    }

    U32 rejection_failures = 0;
    for (const RejectionSpec& rejection : REJECTIONS)
    {
        LLRenderTarget* rejection_target = &contract.mTarget;
        if (rejection.mRegistryMutation == RegistryMutation::LiveWrongColorTarget)
        {
            rejection_target = &contract.mIncompatibleColorTarget;
        }
        else if (rejection.mRegistryMutation == RegistryMutation::LiveWrongDepthTarget)
        {
            rejection_target = &contract.mIncompatibleDepthTarget;
        }

        if (!seedTarget(*rejection_target, fixture))
        {
            ++rejection_failures;
            if (first_failure.empty())
                first_failure = std::string("rejection_seed ") + rejection.mName;
            continue;
        }

        MaterialReadback before;
        MaterialReadback after;
        FrameSnapshot    rejected_frame = material_case.mFrame;
        mutateFrame(rejected_frame, rejection.mFrameMutation);
        LLRenderGLMaterial::Registry rejected_registry;
        const bool registered = registerResources(rejected_registry, material_case.mInputs, contract, shader, rejection.mRegistryMutation);
        const bool baseline_read = readTarget(*rejection_target, before);
        SubmissionResult submission;
        if (registered && baseline_read)
        {
            submission = submitPoisoned(legacy, shader, [&]() { return LLRenderGLMaterial::execute(rejected_frame, rejected_registry); });
        }
        const bool after_read = readTarget(*rejection_target, after);
        if (!registered || !baseline_read || !submission.mPoisoned || submission.mAccepted || submission.mError != GL_NO_ERROR ||
            !after_read || before != after)
        {
            ++rejection_failures;
            if (first_failure.empty())
                first_failure = std::string("rejection ") + rejection.mName;
        }
    }

    bool              success          = mismatches == 0 && nontrivial && rejection_failures == 0;
    const std::string artifact_path    = gSavedSettings.getString("RenderMaterialArtifactPath");
    bool              artifact_written = false;
    if (success && !artifact_path.empty())
    {
        MaterialArtifact artifact = artifactFrom(legacy_readback);
        std::string      artifact_error;
        artifact_written =
            validateMaterialArtifact(artifact, &artifact_error) && writeMaterialArtifact(artifact_path, artifact, &artifact_error);
        if (!artifact_written)
        {
            success       = false;
            first_failure = "artifact_write";
        }
    }

    std::ostringstream result;
    result << "MATERIAL_CONTRACT_PARITY result=" << (success ? "pass" : "fail") << " case=nonrigged_normspec_indexed"
           << " shader_index=12 shader_class=3"
           << " components=" << (MATERIAL_DIAGNOSTIC_COLOR_COMPONENT_COUNT * 3 + MATERIAL_DIAGNOSTIC_DEPTH_COMPONENT_COUNT)
           << " mismatches=" << mismatches << " max_abs_delta=" << max_abs_delta << " depth_passes=" << depth_gate.mPasses
           << " depth_failures=" << depth_gate.mFailures << " mirror_clipped_passes=" << depth_gate.mMirrorClippedPasses
           << " rejection_cases=" << REJECTIONS.size() << " rejection_failures=" << rejection_failures << " artifact="
           << (artifact_path.empty() ? "disabled"
               : artifact_written    ? "written"
                                     : "failed");
    if (!first_failure.empty())
    {
        result << " first_failure={" << first_failure << '}';
    }
    std::cout << result.str() << std::endl;
    LL_INFOS("RenderContractParity") << result.str() << LL_ENDL;
    return success;
}

} // namespace LLMaterialParity
