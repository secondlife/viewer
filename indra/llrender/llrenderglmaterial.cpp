/**
 * @file llrenderglmaterial.cpp
 * @brief OpenGL replay of the canonical Stage 12 material packet.
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

#include "llrenderglmaterial.h"

#include "llgl.h"
#include "llglslshader.h"
#include "llrender.h"
#include "llrendertarget.h"
#include "llshadermgr.h"
#include "llvertexbuffer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace LLRenderGLMaterial
{
namespace
{

    constexpr U32 MATERIAL_VERTEX_MASK = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0 |
                                         LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2 | LLVertexBuffer::MAP_COLOR |
                                         LLVertexBuffer::MAP_TANGENT;

    struct Prepared
    {
        LLRenderContract::MaterialInputs   mInputs;
        LLVertexBuffer*                    mVertexBuffer = nullptr;
        std::array<const SampledImage*, 3> mSampledImages{};
        LLRenderTarget*                    mTarget  = nullptr;
        const Sampler*                     mSampler = nullptr;
        LLGLSLShader*                      mShader  = nullptr;
    };

    bool sameExtent(LLRenderContract::Extent2D left, LLRenderContract::Extent2D right)
    {
        return left.mWidth == right.mWidth && left.mHeight == right.mHeight;
    }

    bool noGlError()
    {
        return glGetError() == GL_NO_ERROR;
    }

    bool targetColorFormat(const LLRenderTarget& target, U32 attachment, LLRenderContract::PixelFormat format)
    {
        const U32 gl_format = target.getColorFormat(attachment);
        if (format == LLRenderContract::PixelFormat::RGBA8Unorm)
        {
            return gl_format == GL_RGBA || gl_format == GL_RGBA8;
        }
        return format == LLRenderContract::PixelFormat::RGBA16Unorm && gl_format == GL_RGBA16;
    }

    GLint internalFormat(LLRenderContract::PixelFormat format)
    {
        switch (format)
        {
            case LLRenderContract::PixelFormat::RGBA8Unorm:
                return GL_RGBA8;
            case LLRenderContract::PixelFormat::RGBA16Unorm:
                return GL_RGBA16;
            case LLRenderContract::PixelFormat::Depth24Unorm:
                return GL_DEPTH_COMPONENT24;
            default:
                return 0;
        }
    }

    bool compatibleInternalFormat(LLRenderContract::PixelFormat format, GLint actual)
    {
        if (format == LLRenderContract::PixelFormat::Depth24Unorm)
        {
            // Apple's OpenGL implementation stores an explicit DEPTH_COMPONENT24
            // request as normalized DEPTH_COMPONENT32. Readback is still
            // canonicalized to 24 bits by the shared diagnostic.
            return actual == GL_DEPTH_COMPONENT24 || actual == GL_DEPTH_COMPONENT32;
        }
        return actual == internalFormat(format);
    }

    // Texture inspection uses one temporary binding and restores the raw binding and
    // active unit. It never changes texture parameters or framebuffer attachments.
    bool liveTexture(GLuint texture, LLRenderContract::PixelFormat format, LLRenderContract::Extent2D extent, std::uint32_t mip_levels)
    {
        if (texture == 0 || mip_levels == 0 || !glIsTexture(texture))
        {
            return false;
        }

        GLint previous_active = GL_TEXTURE0;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active);
        glActiveTexture(GL_TEXTURE0);

        GLint previous_binding = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_binding);
        glBindTexture(GL_TEXTURE_2D, texture);

        bool valid = true;
        for (std::uint32_t level = 0; level < mip_levels; ++level)
        {
            GLint width        = 0;
            GLint height       = 0;
            GLint level_format = 0;
            glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_HEIGHT, &height);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_INTERNAL_FORMAT, &level_format);

            const GLint expected_width  = static_cast<GLint>(std::max<std::uint32_t>(1, extent.mWidth >> level));
            const GLint expected_height = static_cast<GLint>(std::max<std::uint32_t>(1, extent.mHeight >> level));
            valid = valid && width == expected_width && height == expected_height && compatibleInternalFormat(format, level_format);
        }

        GLint extra_width = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(mip_levels), GL_TEXTURE_WIDTH, &extra_width);
        valid = valid && extra_width == 0;

        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous_binding));
        glActiveTexture(static_cast<GLenum>(previous_active));
        return valid && noGlError();
    }

    struct ActiveVariable
    {
        GLint  mLocation = -1;
        GLint  mSize     = 0;
        GLenum mType     = 0;
    };

    using ActiveVariables = std::map<std::string, ActiveVariable>;

    std::optional<ActiveVariables> activeVariables(GLuint program, GLenum count_name, GLenum max_length_name, bool attributes)
    {
        GLint count      = 0;
        GLint max_length = 0;
        glGetProgramiv(program, count_name, &count);
        glGetProgramiv(program, max_length_name, &max_length);
        if (count < 0 || max_length <= 0)
        {
            return std::nullopt;
        }

        std::vector<GLchar> name(static_cast<std::size_t>(max_length));
        ActiveVariables     result;
        for (GLint index = 0; index < count; ++index)
        {
            GLsizei length = 0;
            GLint   size   = 0;
            GLenum  type   = 0;
            if (attributes)
            {
                glGetActiveAttrib(program, static_cast<GLuint>(index), max_length, &length, &size, &type, name.data());
            }
            else
            {
                glGetActiveUniform(program, static_cast<GLuint>(index), max_length, &length, &size, &type, name.data());
            }
            if (length <= 0)
            {
                return std::nullopt;
            }

            std::string variable(name.data(), static_cast<std::size_t>(length));
            if (!attributes && variable.ends_with("[0]"))
            {
                variable.resize(variable.size() - 3);
            }
            const GLint location =
                attributes ? glGetAttribLocation(program, variable.c_str()) : glGetUniformLocation(program, variable.c_str());
            if ((attributes && location < 0) || !result.emplace(std::move(variable), ActiveVariable{ location, size, type }).second)
            {
                return std::nullopt;
            }
        }
        if (!noGlError())
        {
            return std::nullopt;
        }
        return result;
    }

    bool exactVariables(const ActiveVariables& actual, const ActiveVariables& expected)
    {
        if (actual.size() != expected.size())
        {
            return false;
        }
        for (const auto& [name, expected_variable] : expected)
        {
            const auto found = actual.find(name);
            if (found == actual.end() || found->second.mLocation != expected_variable.mLocation ||
                found->second.mSize != expected_variable.mSize || found->second.mType != expected_variable.mType)
            {
                return false;
            }
        }
        return true;
    }

    bool matchesShader(LLGLSLShader& shader)
    {
        if (!shader.isComplete() || shader.mName != "Material Shader 12" || shader.mShaderLevel != 3 || shader.mUsingBinaryProgram ||
            shader.mAttributeMask != MATERIAL_VERTEX_MASK || shader.mFeatures.hasObjectSkinning || !shader.mFeatures.hasReflectionProbes ||
            shader.mProgramObject == 0 || !glIsProgram(shader.mProgramObject))
        {
            return false;
        }

        const LLGLSLShader::defines_map_t                 expected_defines{ { "DIFFUSE_ALPHA_MODE", "0" },
                                                                            { "HAS_NORMAL_MAP", "1" },
                                                                            { "HAS_SPECULAR_MAP", "1" } };
        const std::vector<std::pair<std::string, GLenum>> expected_files{ { "deferred/materialV.glsl", GL_VERTEX_SHADER },
                                                                          { "deferred/materialF.glsl", GL_FRAGMENT_SHADER } };
        if (shader.mDefines != expected_defines || shader.mShaderFiles != expected_files)
        {
            return false;
        }

        GLint linked = GL_FALSE;
        glGetProgramiv(shader.mProgramObject, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE)
        {
            return false;
        }

        const ActiveVariables expected_attributes{ { "position", { LLVertexBuffer::TYPE_VERTEX, 1, GL_FLOAT_VEC3 } },
                                                   { "normal", { LLVertexBuffer::TYPE_NORMAL, 1, GL_FLOAT_VEC3 } },
                                                   { "texcoord0", { LLVertexBuffer::TYPE_TEXCOORD0, 1, GL_FLOAT_VEC2 } },
                                                   { "texcoord1", { LLVertexBuffer::TYPE_TEXCOORD1, 1, GL_FLOAT_VEC2 } },
                                                   { "texcoord2", { LLVertexBuffer::TYPE_TEXCOORD2, 1, GL_FLOAT_VEC2 } },
                                                   { "diffuse_color", { LLVertexBuffer::TYPE_COLOR, 1, GL_FLOAT_VEC4 } },
                                                   { "tangent", { LLVertexBuffer::TYPE_TANGENT, 1, GL_FLOAT_VEC4 } } };
        const auto attributes = activeVariables(shader.mProgramObject, GL_ACTIVE_ATTRIBUTES, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, true);
        if (!attributes || !exactVariables(*attributes, expected_attributes))
        {
            return false;
        }

        const ActiveVariables expected_uniform_types{ { "modelview_matrix", { 0, 1, GL_FLOAT_MAT4 } },
                                                      { "modelview_projection_matrix", { 0, 1, GL_FLOAT_MAT4 } },
                                                      { "normal_matrix", { 0, 1, GL_FLOAT_MAT3 } },
                                                      { "texture_matrix0", { 0, 1, GL_FLOAT_MAT4 } },
                                                      { "diffuseMap", { 0, 1, GL_SAMPLER_2D } },
                                                      { "bumpMap", { 0, 1, GL_SAMPLER_2D } },
                                                      { "specularMap", { 0, 1, GL_SAMPLER_2D } },
                                                      { "emissive_brightness", { 0, 1, GL_FLOAT } },
                                                      { "env_intensity", { 0, 1, GL_FLOAT } },
                                                      { "specular_color", { 0, 1, GL_FLOAT_VEC4 } },
                                                      { "mirror_flag", { 0, 1, GL_FLOAT } },
                                                      { "clipPlane", { 0, 1, GL_FLOAT_VEC4 } },
                                                      { "refBox", { -1, 256, GL_FLOAT_MAT4 } },
                                                      { "heroBox", { -1, 1, GL_FLOAT_MAT4 } },
                                                      { "refSphere", { -1, 256, GL_FLOAT_VEC4 } },
                                                      { "refParams", { -1, 256, GL_FLOAT_VEC4 } },
                                                      { "heroSphere", { -1, 1, GL_FLOAT_VEC4 } },
                                                      { "refIndex", { -1, 256, GL_INT_VEC4 } },
                                                      { "refNeighbor", { -1, 1024, GL_INT_VEC4 } },
                                                      { "refBucket", { -1, 256, GL_INT_VEC4 } },
                                                      { "refmapCount", { -1, 1, GL_INT } },
                                                      { "heroShape", { -1, 1, GL_INT } },
                                                      { "heroMipCount", { -1, 1, GL_INT } },
                                                      { "heroProbeCount", { -1, 1, GL_INT } } };
        const auto            uniforms = activeVariables(shader.mProgramObject, GL_ACTIVE_UNIFORMS, GL_ACTIVE_UNIFORM_MAX_LENGTH, false);
        if (!uniforms || uniforms->size() != expected_uniform_types.size())
        {
            return false;
        }
        for (const auto& [name, expected] : expected_uniform_types)
        {
            const auto found = uniforms->find(name);
            const bool valid_location =
                found != uniforms->end() && (expected.mLocation < 0 ? found->second.mLocation == -1 : found->second.mLocation >= 0);
            if (!valid_location || found->second.mSize != expected.mSize || found->second.mType != expected.mType)
            {
                return false;
            }
        }

        GLint active_blocks = 0;
        glGetProgramiv(shader.mProgramObject, GL_ACTIVE_UNIFORM_BLOCKS, &active_blocks);
        const GLuint reflection_block = glGetUniformBlockIndex(shader.mProgramObject, "ReflectionProbes");
        GLint        block_binding    = -1;
        GLint        block_size       = 0;
        GLint        block_uniforms   = 0;
        if (reflection_block != GL_INVALID_INDEX)
        {
            glGetActiveUniformBlockiv(shader.mProgramObject, reflection_block, GL_UNIFORM_BLOCK_BINDING, &block_binding);
            glGetActiveUniformBlockiv(shader.mProgramObject, reflection_block, GL_UNIFORM_BLOCK_DATA_SIZE, &block_size);
            glGetActiveUniformBlockiv(shader.mProgramObject, reflection_block, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &block_uniforms);
        }
        if (active_blocks != 1 || reflection_block == GL_INVALID_INDEX || block_binding != LLGLSLShader::UB_REFLECTION_PROBES ||
            block_size != 49248 || block_uniforms != 12)
        {
            return false;
        }

        const GLint output0 = glGetFragDataLocation(shader.mProgramObject, "frag_data[0]");
        const GLint output1 = glGetFragDataLocation(shader.mProgramObject, "frag_data[1]");
        const GLint output2 = glGetFragDataLocation(shader.mProgramObject, "frag_data[2]");
        const GLint output3 = glGetFragDataLocation(shader.mProgramObject, "frag_data[3]");
        if (output0 != 0 || output1 != 1 || output2 != 2 || output3 != 3)
        {
            return false;
        }

        const S32 diffuse_channel  = shader.getTextureChannel(LLShaderMgr::DIFFUSE_MAP);
        const S32 bump_channel     = shader.getTextureChannel(LLShaderMgr::BUMP_MAP);
        const S32 specular_channel = shader.getTextureChannel(LLShaderMgr::SPECULAR_MAP);
        if (shader.mActiveTextureChannels != 3 || diffuse_channel != LLRender::DIFFUSE_MAP || bump_channel != LLRender::NORMAL_MAP ||
            specular_channel != LLRender::SPECULAR_MAP)
        {
            return false;
        }

        constexpr std::array<std::pair<U32, const char*>, 12> uniform_mappings{
            std::pair<U32, const char*>{ LLShaderMgr::MODELVIEW_MATRIX, "modelview_matrix" },
            { LLShaderMgr::MODELVIEW_PROJECTION_MATRIX, "modelview_projection_matrix" },
            { LLShaderMgr::NORMAL_MATRIX, "normal_matrix" },
            { LLShaderMgr::TEXTURE_MATRIX0, "texture_matrix0" },
            { LLShaderMgr::DIFFUSE_MAP, "diffuseMap" },
            { LLShaderMgr::BUMP_MAP, "bumpMap" },
            { LLShaderMgr::SPECULAR_MAP, "specularMap" },
            { LLShaderMgr::EMISSIVE_BRIGHTNESS, "emissive_brightness" },
            { LLShaderMgr::ENVIRONMENT_INTENSITY, "env_intensity" },
            { LLShaderMgr::SPECULAR_COLOR, "specular_color" },
            { LLShaderMgr::MIRROR_FLAG, "mirror_flag" },
            { LLShaderMgr::CLIP_PLANE, "clipPlane" }
        };
        for (const auto& [uniform, name] : uniform_mappings)
        {
            if (shader.getUniformLocation(uniform) != glGetUniformLocation(shader.mProgramObject, name))
            {
                return false;
            }
        }
        return noGlError();
    }

    bool matchesVertexMetadata(const LLVertexBuffer& buffer)
    {
        return buffer.getNumVerts() == 4 && buffer.getNumIndices() == 6 && buffer.getTypeMask() == MATERIAL_VERTEX_MASK &&
               buffer.getSize() == LLRenderContract::MATERIAL_VERTEX_BUFFER_SIZE &&
               buffer.getIndicesSize() == LLRenderContract::MATERIAL_INDEX_BUFFER_SIZE &&
               buffer.getOffset(LLVertexBuffer::TYPE_VERTEX) == LLRenderContract::MATERIAL_POSITION_OFFSET &&
               buffer.getOffset(LLVertexBuffer::TYPE_NORMAL) == LLRenderContract::MATERIAL_NORMAL_OFFSET &&
               buffer.getOffset(LLVertexBuffer::TYPE_TEXCOORD0) == LLRenderContract::MATERIAL_TEXCOORD0_OFFSET &&
               buffer.getOffset(LLVertexBuffer::TYPE_TEXCOORD1) == LLRenderContract::MATERIAL_TEXCOORD1_OFFSET &&
               buffer.getOffset(LLVertexBuffer::TYPE_TEXCOORD2) == LLRenderContract::MATERIAL_TEXCOORD2_OFFSET &&
               buffer.getOffset(LLVertexBuffer::TYPE_COLOR) == LLRenderContract::MATERIAL_COLOR_OFFSET &&
               buffer.getOffset(LLVertexBuffer::TYPE_TANGENT) == LLRenderContract::MATERIAL_TANGENT_OFFSET;
    }

    struct VertexArrayExpectation
    {
        GLuint         mLocation;
        GLint          mComponents;
        GLenum         mType;
        GLboolean      mNormalized;
        GLsizei        mStride;
        std::uintptr_t mOffset;
    };

    bool liveVertexBuffers(LLVertexBuffer& buffer)
    {
        buffer.setBuffer();

        GLint vertex_name = 0;
        GLint index_name  = 0;
        GLint vertex_size = 0;
        GLint index_size  = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vertex_name);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &index_name);
        if (vertex_name == 0 || index_name == 0 || vertex_name == index_name ||
            vertex_name != static_cast<GLint>(LLVertexBuffer::sGLRenderBuffer) ||
            index_name != static_cast<GLint>(LLVertexBuffer::sGLRenderIndices) || !glIsBuffer(static_cast<GLuint>(vertex_name)) ||
            !glIsBuffer(static_cast<GLuint>(index_name)))
        {
            return false;
        }
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vertex_size);
        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &index_size);
        if (vertex_size < static_cast<GLint>(LLRenderContract::MATERIAL_VERTEX_BUFFER_SIZE) ||
            index_size < static_cast<GLint>(LLRenderContract::MATERIAL_INDEX_BUFFER_SIZE))
        {
            return false;
        }

        constexpr std::array<VertexArrayExpectation, 7> expectations{
            VertexArrayExpectation{ LLVertexBuffer::TYPE_VERTEX, 3, GL_FLOAT, GL_FALSE, 16, LLRenderContract::MATERIAL_POSITION_OFFSET },
            { LLVertexBuffer::TYPE_NORMAL, 3, GL_FLOAT, GL_FALSE, 16, LLRenderContract::MATERIAL_NORMAL_OFFSET },
            { LLVertexBuffer::TYPE_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 8, LLRenderContract::MATERIAL_TEXCOORD0_OFFSET },
            { LLVertexBuffer::TYPE_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, 8, LLRenderContract::MATERIAL_TEXCOORD1_OFFSET },
            { LLVertexBuffer::TYPE_TEXCOORD2, 2, GL_FLOAT, GL_FALSE, 8, LLRenderContract::MATERIAL_TEXCOORD2_OFFSET },
            { LLVertexBuffer::TYPE_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4, LLRenderContract::MATERIAL_COLOR_OFFSET },
            { LLVertexBuffer::TYPE_TANGENT, 4, GL_FLOAT, GL_FALSE, 16, LLRenderContract::MATERIAL_TANGENT_OFFSET }
        };
        for (const VertexArrayExpectation& expected : expectations)
        {
            glVertexAttribDivisor(expected.mLocation, 0);
            GLint enabled      = GL_FALSE;
            GLint components   = 0;
            GLint type         = 0;
            GLint normalized   = GL_FALSE;
            GLint integer      = GL_FALSE;
            GLint stride       = 0;
            GLint divisor      = 0;
            GLint bound_buffer = 0;
            void* pointer      = nullptr;
            glGetVertexAttribiv(expected.mLocation, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            glGetVertexAttribiv(expected.mLocation, GL_VERTEX_ATTRIB_ARRAY_SIZE, &components);
            glGetVertexAttribiv(expected.mLocation, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
            glGetVertexAttribiv(expected.mLocation, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
            glGetVertexAttribiv(expected.mLocation, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &integer);
            glGetVertexAttribiv(expected.mLocation, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
            glGetVertexAttribiv(expected.mLocation, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &divisor);
            glGetVertexAttribiv(expected.mLocation, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &bound_buffer);
            glGetVertexAttribPointerv(expected.mLocation, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointer);
            if (enabled != GL_TRUE || components != expected.mComponents || type != static_cast<GLint>(expected.mType) ||
                normalized != expected.mNormalized || integer != GL_FALSE || stride != expected.mStride || bound_buffer != vertex_name ||
                divisor != 0 || reinterpret_cast<std::uintptr_t>(pointer) != expected.mOffset)
            {
                return false;
            }
        }

        std::array<std::uint16_t, 6> indices{};
        glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices.data());
        return noGlError() && indices == LLRenderContract::MATERIAL_INDICES;
    }

    std::optional<Prepared> prepare(const LLRenderContract::FrameSnapshot& frame, const Registry& registry)
    {
        auto inputs = LLRenderContract::decodeMaterialFrame(frame);
        if (!inputs || !noGlError())
        {
            return std::nullopt;
        }

        Prepared result;
        result.mInputs        = *inputs;
        result.mVertexBuffer  = registry.resolveVertexBuffer(inputs->mHandles.mVertexBuffer, inputs->mHandles.mIndexBuffer);
        result.mSampledImages = { registry.resolveSampledImage(inputs->mHandles.mDiffuse),
                                  registry.resolveSampledImage(inputs->mHandles.mNormal),
                                  registry.resolveSampledImage(inputs->mHandles.mSpecular) };
        result.mSampler       = registry.resolve(inputs->mHandles.mSampler);
        result.mShader        = registry.resolve(inputs->mHandles.mPipeline, frame.mPipelines.front().mProgram);

        const TargetImage gbuffer0 = registry.resolveTargetImage(inputs->mHandles.mGBuffer0);
        const TargetImage gbuffer1 = registry.resolveTargetImage(inputs->mHandles.mGBuffer1);
        const TargetImage gbuffer2 = registry.resolveTargetImage(inputs->mHandles.mGBuffer2);
        const TargetImage depth    = registry.resolveTargetImage(inputs->mHandles.mDepth);
        result.mTarget             = gbuffer0.mTarget;

        constexpr Sampler                expected_sampler;
        const LLRenderContract::Extent2D texture_extent{ LLRenderContract::MATERIAL_TEXTURE_WIDTH,
                                                         LLRenderContract::MATERIAL_TEXTURE_HEIGHT };
        if (!result.mVertexBuffer || !result.mSampledImages[0] || !result.mSampledImages[1] || !result.mSampledImages[2] ||
            !result.mSampler || *result.mSampler != expected_sampler || !result.mShader || !result.mTarget ||
            gbuffer1.mTarget != result.mTarget || gbuffer2.mTarget != result.mTarget || depth.mTarget != result.mTarget ||
            gbuffer0.mAspect != TargetAspect::Color || gbuffer0.mAttachment != 0 || gbuffer1.mAspect != TargetAspect::Color ||
            gbuffer1.mAttachment != 1 || gbuffer2.mAspect != TargetAspect::Color || gbuffer2.mAttachment != 2 ||
            depth.mAspect != TargetAspect::Depth || !matchesVertexMetadata(*result.mVertexBuffer) || !gGLManager.mHasAnisotropic ||
            gGLManager.mMaxAnisotropy < 8.f)
        {
            return std::nullopt;
        }

        if (!result.mTarget->isComplete() || result.mTarget->isBoundInStack() ||
            LLRenderTarget::getCurrentBoundTarget() == result.mTarget || result.mTarget->getUsage() != LLTexUnit::TT_TEXTURE ||
            result.mTarget->getWidth() != LLRenderContract::MATERIAL_FRAME_WIDTH ||
            result.mTarget->getHeight() != LLRenderContract::MATERIAL_FRAME_HEIGHT || result.mTarget->getNumTextures() != 3 ||
            !targetColorFormat(*result.mTarget, 0, LLRenderContract::PixelFormat::RGBA8Unorm) ||
            !targetColorFormat(*result.mTarget, 1, LLRenderContract::PixelFormat::RGBA8Unorm) ||
            !targetColorFormat(*result.mTarget, 2, LLRenderContract::PixelFormat::RGBA16Unorm))
        {
            return std::nullopt;
        }

        std::array<GLuint, 7> texture_names{ result.mSampledImages[0]->mTexture, result.mSampledImages[1]->mTexture,
                                             result.mSampledImages[2]->mTexture, result.mTarget->getTexture(0),
                                             result.mTarget->getTexture(1),      result.mTarget->getTexture(2),
                                             result.mTarget->getDepth() };
        std::array<GLuint, 7> sorted_names = texture_names;
        std::sort(sorted_names.begin(), sorted_names.end());
        if (sorted_names.front() == 0 || std::adjacent_find(sorted_names.begin(), sorted_names.end()) != sorted_names.end())
        {
            return std::nullopt;
        }

        for (const SampledImage* image : result.mSampledImages)
        {
            if (image->mFormat != LLRenderContract::PixelFormat::RGBA8Unorm || !sameExtent(image->mExtent, texture_extent) ||
                image->mMipLevels != LLRenderContract::MATERIAL_TEXTURE_MIP_LEVELS ||
                !liveTexture(image->mTexture, image->mFormat, image->mExtent, image->mMipLevels))
            {
                return std::nullopt;
            }
        }

        const LLRenderContract::Extent2D frame_extent{ LLRenderContract::MATERIAL_FRAME_WIDTH, LLRenderContract::MATERIAL_FRAME_HEIGHT };
        if (!liveTexture(texture_names[3], LLRenderContract::PixelFormat::RGBA8Unorm, frame_extent, 1))
        {
            return std::nullopt;
        }
        if (!liveTexture(texture_names[4], LLRenderContract::PixelFormat::RGBA8Unorm, frame_extent, 1))
        {
            return std::nullopt;
        }
        if (!liveTexture(texture_names[5], LLRenderContract::PixelFormat::RGBA16Unorm, frame_extent, 1))
        {
            return std::nullopt;
        }
        if (!liveTexture(texture_names[6], LLRenderContract::PixelFormat::Depth24Unorm, frame_extent, 1))
        {
            return std::nullopt;
        }
        if (!matchesShader(*result.mShader))
        {
            return std::nullopt;
        }
        return result;
    }

    bool bindAndValidateVertexInput(Prepared& prepared)
    {
        const bool uniforms_dirty        = prepared.mShader->mUniformsDirty;
        prepared.mShader->mUniformsDirty = false;
        prepared.mShader->bind();
        prepared.mShader->mUniformsDirty = uniforms_dirty;

        GLint current_program = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
        if (current_program != static_cast<GLint>(prepared.mShader->mProgramObject) ||
            LLGLSLShader::sCurBoundShader != prepared.mShader->mProgramObject || LLGLSLShader::sCurBoundShaderPtr != prepared.mShader ||
            !noGlError() || !liveVertexBuffers(*prepared.mVertexBuffer))
        {
            LLGLSLShader::unbind();
            return false;
        }
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
            // The single-sample fixture disables dithering and multisampling so the
            // stored attachment bytes do not depend on ambient driver state.
            constexpr std::array<GLboolean, 15> enabled{ GL_FALSE, GL_TRUE,  GL_TRUE,  GL_TRUE,  GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE,
                                                         GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE };
            for (std::size_t i = 0; i < mCapabilities.size(); ++i)
            {
                enabled[i] ? glEnable(mCapabilities[i]) : glDisable(mCapabilities[i]);
            }
            glViewport(0, 0, LLRenderContract::MATERIAL_FRAME_WIDTH, LLRenderContract::MATERIAL_FRAME_HEIGHT);
            glScissor(0, 0, LLRenderContract::MATERIAL_FRAME_WIDTH, LLRenderContract::MATERIAL_FRAME_HEIGHT);
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

    private:
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

    void bindSampledImage(U32 unit, const SampledImage& image, const Sampler& sampler)
    {
        glBindSampler(unit, 0);
        LLTexUnit* texture_unit = gGL.getTexUnit(unit);
        texture_unit->bindManual(LLTexUnit::TT_TEXTURE, image.mTexture, true);
        texture_unit->activate();
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, image.mTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(image.mMipLevels - 1));
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
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, sampler.mMaxAnisotropy);
    }

    void eraseCachedUniform(LLGLSLShader& shader, U32 uniform)
    {
        const GLint location = shader.getUniformLocation(uniform);
        if (location >= 0)
        {
            shader.mValue.erase(location);
        }
    }

    void applyUniforms(LLGLSLShader& shader, const LLRenderContract::MaterialParameters& parameters)
    {
        // Align the viewer matrix hashes first; the explicit packet values below are
        // then left intact by LLVertexBuffer::drawRange's mandatory matrix sync.
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

} // namespace

bool execute(const LLRenderContract::FrameSnapshot& frame, const Registry& registry)
{
    auto prepared = prepare(frame, registry);
    if (!prepared || !bindAndValidateVertexInput(*prepared))
    {
        return false;
    }

    bool success = true;
    prepared->mTarget->bindTarget();
    {
        DrawStateRestore state;
        state.applyMaterialState();

        constexpr GLfloat clear[4] = { 0.f, 0.f, 0.f, 0.f };
        glClearBufferfv(GL_COLOR, 0, clear);
        glClearBufferfv(GL_COLOR, 1, clear);
        glClearBufferfv(GL_COLOR, 2, clear);

        applyUniforms(*prepared->mShader, prepared->mInputs.mParameters);
        bindSampledImage(LLRender::DIFFUSE_MAP, *prepared->mSampledImages[0], *prepared->mSampler);
        bindSampledImage(LLRender::NORMAL_MAP, *prepared->mSampledImages[1], *prepared->mSampler);
        bindSampledImage(LLRender::SPECULAR_MAP, *prepared->mSampledImages[2], *prepared->mSampler);

        prepared->mVertexBuffer->setBuffer();
        prepared->mVertexBuffer->drawRange(LLRender::TRIANGLES, 0, 3, 6, 0);
        success = noGlError();

        for (U32 unit : { U32{ LLRender::DIFFUSE_MAP }, U32{ LLRender::NORMAL_MAP }, U32{ LLRender::SPECULAR_MAP } })
        {
            glBindSampler(unit, 0);
            gGL.getTexUnit(unit)->unbind(LLTexUnit::TT_TEXTURE);
        }
        LLGLSLShader::unbind();
    }
    prepared->mTarget->flush();
    return success;
}

} // namespace LLRenderGLMaterial
