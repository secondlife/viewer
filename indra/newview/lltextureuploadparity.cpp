/**
 * @file lltextureuploadparity.cpp
 * @brief Account-free parity harness for one streaming texture upload.
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

#include "lltextureuploadparity.h"

#include "llgl.h"
#include "llglslshader.h"
#include "llglstates.h"
#include "llimage.h"
#include "llimagegl.h"
#include "llrendergltextureupload.h"
#include "llrendertarget.h"
#include "llshadermgr.h"
#include "lltextureuploaddiagnostic.h"
#include "llvertexbuffer.h"
#include "llviewercontrol.h"
#include "llviewershadermgr.h"
#include "pipeline.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace LLTextureUploadParity
{
namespace
{

using namespace LLRenderContract;

constexpr const char* TEXTURE_UPLOAD_PROGRAM = "contract.sample-texture";

struct TextureResources
{
    LLPointer<LLVertexBuffer> mScreenTriangle;
    LLPointer<LLImageGL>      mOldImage;
    LLPointer<LLImageGL>      mReplacementImage;
    LLRenderTarget            mOutput;
    GLuint                    mSampler = 0;
    GLuint                    mSavedReplacementName = 0;
    LLRenderGLTextureUpload::LifecycleLedger mLifecycle;

    TextureResources()                                = default;
    TextureResources(const TextureResources&)         = delete;
    TextureResources& operator=(const TextureResources&) = delete;

    ~TextureResources()
    {
        if (mSampler != 0)
        {
            glDeleteSamplers(1, &mSampler);
        }
    }
};

struct TextureSnapshot
{
    std::array<std::vector<std::uint8_t>, TEXTURE_UPLOAD_MIP_LEVELS> mOldMips;
    std::array<std::vector<std::uint8_t>, TEXTURE_UPLOAD_MIP_LEVELS> mReplacementMips;
    std::vector<std::uint8_t>                                        mOutput;
    LLRenderGLTextureUpload::LifecycleLedger                         mLifecycle;

    friend bool operator==(const TextureSnapshot&, const TextureSnapshot&) = default;
};

struct AmbientVertexAttribute
{
    GLint mEnabled = 0;
    GLint mSize = 0;
    GLint mStride = 0;
    GLint mType = 0;
    GLint mNormalized = 0;
    GLint mInteger = 0;
    GLint mDivisor = 0;
    GLint mBuffer = 0;
    void* mPointer = nullptr;
#ifdef GL_VERTEX_ATTRIB_ARRAY_LONG
    GLint mLong = 0;
#endif

    friend bool operator==(const AmbientVertexAttribute&, const AmbientVertexAttribute&) = default;
};

struct AmbientGLState
{
    GLint mActiveTexture = 0;
    U32 mCachedActiveTexture = 0;
    GLint mTexture2D = 0;
    GLint mSampler = 0;
    U32 mCachedTexture = 0;
    LLTexUnit::eTextureType mCachedTextureType = LLTexUnit::TT_NONE;
    bool mCachedTextureHasMips = false;
    GLint mRawCachedTargetTexture = 0;

    GLint mPackBuffer = 0;
    GLint mUnpackBuffer = 0;
    std::array<GLint, 16> mPixelStore{};
#ifdef GL_PACK_INVERT_MESA
    GLint mPackInvert = GL_FALSE;
#endif
#ifdef GL_UNPACK_CLIENT_STORAGE_APPLE
    GLint mClientStorage = GL_FALSE;
#endif

    GLint mArrayBuffer = 0;
    GLint mElementBuffer = 0;
    GLint mVertexArray = 0;
    U32 mCachedArrayBuffer = 0;
    U32 mCachedElementBuffer = 0;
    U32 mCachedAttributeMask = 0;
    std::vector<AmbientVertexAttribute> mAttributes;

    GLint mProgram = 0;
    GLuint mCachedProgram = 0;
    LLGLSLShader* mCachedShader = nullptr;
    GLint mDiffuseUniform = 0;
    bool mCopyUniformsDirty = false;

    GLint mDrawFramebuffer = 0;
    GLint mReadFramebuffer = 0;
    GLint mReadBuffer = 0;
    std::vector<GLint> mDrawBuffers;
    LLRenderTarget* mBoundTarget = nullptr;
    U32 mCachedFramebuffer = 0;
    U32 mCachedWidth = 0;
    U32 mCachedHeight = 0;

    std::array<GLint, 4> mViewport{};
    std::array<GLint, 4> mScissor{};
    std::array<GLboolean, 4> mColorMask{};
    bool mHasClipControl = false;
    GLint mClipOrigin = GL_LOWER_LEFT;
    GLint mClipDepthMode = GL_NEGATIVE_ONE_TO_ONE;
    std::array<GLint, 2> mPolygonMode{};
    GLint mProfileMask = 0;
    std::vector<std::pair<GLenum, bool>> mCapabilities;

    friend bool operator==(const AmbientGLState&, const AmbientGLState&) = default;
};

GLenum textureBindingQuery(LLTexUnit::eTextureType type)
{
    switch (type)
    {
        case LLTexUnit::TT_TEXTURE: return GL_TEXTURE_BINDING_2D;
        case LLTexUnit::TT_RECT_TEXTURE: return GL_TEXTURE_BINDING_RECTANGLE;
        case LLTexUnit::TT_CUBE_MAP: return GL_TEXTURE_BINDING_CUBE_MAP;
        case LLTexUnit::TT_CUBE_MAP_ARRAY: return GL_TEXTURE_BINDING_CUBE_MAP_ARRAY;
        case LLTexUnit::TT_MULTISAMPLE_TEXTURE: return GL_TEXTURE_BINDING_2D_MULTISAMPLE;
        case LLTexUnit::TT_TEXTURE_3D: return GL_TEXTURE_BINDING_3D;
        case LLTexUnit::TT_NONE: return GL_NONE;
    }
    return GL_NONE;
}

bool captureAmbientGLState(AmbientGLState& state)
{
    glGetIntegerv(GL_ACTIVE_TEXTURE, &state.mActiveTexture);
    state.mCachedActiveTexture = gGL.getCurrentTexUnitIndex();
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &state.mTexture2D);
    glGetIntegerv(GL_SAMPLER_BINDING, &state.mSampler);
    LLTexUnit* unit = gGL.getTexUnit(0);
    state.mCachedTexture = unit->getCurrTexture();
    state.mCachedTextureType = unit->getCurrType();
    state.mCachedTextureHasMips = unit->getHasMipMaps();
    const GLenum cached_binding_query = textureBindingQuery(state.mCachedTextureType);
    if (cached_binding_query != GL_NONE)
    {
        glGetIntegerv(cached_binding_query, &state.mRawCachedTargetTexture);
    }
    glActiveTexture(static_cast<GLenum>(state.mActiveTexture));

    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &state.mPackBuffer);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &state.mUnpackBuffer);
    constexpr std::array<GLenum, 16> pixel_store_names{
        GL_PACK_ALIGNMENT,       GL_PACK_ROW_LENGTH,    GL_PACK_SKIP_ROWS,    GL_PACK_SKIP_PIXELS,
        GL_PACK_IMAGE_HEIGHT,    GL_PACK_SKIP_IMAGES,   GL_PACK_SWAP_BYTES,   GL_PACK_LSB_FIRST,
        GL_UNPACK_ALIGNMENT,     GL_UNPACK_ROW_LENGTH,  GL_UNPACK_SKIP_ROWS,  GL_UNPACK_SKIP_PIXELS,
        GL_UNPACK_IMAGE_HEIGHT,  GL_UNPACK_SKIP_IMAGES, GL_UNPACK_SWAP_BYTES, GL_UNPACK_LSB_FIRST
    };
    for (std::size_t index = 0; index < pixel_store_names.size(); ++index)
    {
        glGetIntegerv(pixel_store_names[index], &state.mPixelStore[index]);
    }
#ifdef GL_PACK_INVERT_MESA
    if (gGLManager.mGLExtensions.contains("GL_MESA_pack_invert"))
    {
        glGetIntegerv(GL_PACK_INVERT_MESA, &state.mPackInvert);
    }
#endif
#ifdef GL_UNPACK_CLIENT_STORAGE_APPLE
    if (gGLManager.mGLExtensions.contains("GL_APPLE_client_storage"))
    {
        glGetIntegerv(GL_UNPACK_CLIENT_STORAGE_APPLE, &state.mClientStorage);
    }
#endif

    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &state.mArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &state.mElementBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &state.mVertexArray);
    state.mCachedArrayBuffer = LLVertexBuffer::sGLRenderBuffer;
    state.mCachedElementBuffer = LLVertexBuffer::sGLRenderIndices;
    state.mCachedAttributeMask = LLVertexBuffer::sLastMask;
    GLint attribute_count = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &attribute_count);
    state.mAttributes.resize(static_cast<std::size_t>(std::max(0, attribute_count)));
    for (GLuint index = 0; index < state.mAttributes.size(); ++index)
    {
        AmbientVertexAttribute& attribute = state.mAttributes[index];
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attribute.mEnabled);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &attribute.mSize);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &attribute.mStride);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &attribute.mType);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &attribute.mNormalized);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &attribute.mInteger);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &attribute.mDivisor);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &attribute.mBuffer);
        glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &attribute.mPointer);
#ifdef GL_VERTEX_ATTRIB_ARRAY_LONG
        if (gGLManager.mGLVersion >= 4.29f)
        {
            glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_LONG, &attribute.mLong);
        }
#endif
    }

    glGetIntegerv(GL_CURRENT_PROGRAM, &state.mProgram);
    state.mCachedProgram = LLGLSLShader::sCurBoundShader;
    state.mCachedShader = LLGLSLShader::sCurBoundShaderPtr;
    state.mCopyUniformsDirty = gCopyProgram.mUniformsDirty;
    const GLint diffuse_location = glGetUniformLocation(gCopyProgram.mProgramObject, "diffuseMap");
    if (diffuse_location >= 0)
    {
        glGetUniformiv(gCopyProgram.mProgramObject, diffuse_location, &state.mDiffuseUniform);
    }

    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &state.mDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &state.mReadFramebuffer);
    glGetIntegerv(GL_READ_BUFFER, &state.mReadBuffer);
    GLint draw_buffer_count = 0;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &draw_buffer_count);
    state.mDrawBuffers.resize(static_cast<std::size_t>(std::max(1, draw_buffer_count)));
    for (GLint index = 0; index < draw_buffer_count; ++index)
    {
        glGetIntegerv(static_cast<GLenum>(GL_DRAW_BUFFER0 + index), &state.mDrawBuffers[static_cast<std::size_t>(index)]);
    }
    state.mBoundTarget = LLRenderTarget::getCurrentBoundTarget();
    state.mCachedFramebuffer = LLRenderTarget::sCurFBO;
    state.mCachedWidth = LLRenderTarget::sCurResX;
    state.mCachedHeight = LLRenderTarget::sCurResY;

    glGetIntegerv(GL_VIEWPORT, state.mViewport.data());
    glGetIntegerv(GL_SCISSOR_BOX, state.mScissor.data());
    glGetBooleanv(GL_COLOR_WRITEMASK, state.mColorMask.data());
#if !LL_DARWIN
    state.mHasClipControl = gGLManager.mGLVersion >= 4.49f && glClipControl;
    if (state.mHasClipControl)
    {
        glGetIntegerv(GL_CLIP_ORIGIN, &state.mClipOrigin);
        glGetIntegerv(GL_CLIP_DEPTH_MODE, &state.mClipDepthMode);
    }
#endif
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &state.mProfileMask);
    glGetIntegerv(GL_POLYGON_MODE, state.mPolygonMode.data());
    if ((state.mProfileMask & GL_CONTEXT_CORE_PROFILE_BIT) != 0)
    {
        state.mPolygonMode[1] = state.mPolygonMode[0];
    }
    constexpr std::array<GLenum, 15> capabilities{ GL_BLEND, GL_CULL_FACE, GL_DEPTH_TEST, GL_STENCIL_TEST,
                                                   GL_SCISSOR_TEST, GL_DITHER, GL_FRAMEBUFFER_SRGB,
                                                   GL_RASTERIZER_DISCARD, GL_MULTISAMPLE,
                                                   GL_SAMPLE_ALPHA_TO_COVERAGE, GL_SAMPLE_ALPHA_TO_ONE,
                                                   GL_SAMPLE_COVERAGE, GL_SAMPLE_MASK, GL_SAMPLE_SHADING,
                                                   GL_COLOR_LOGIC_OP };
    for (GLenum capability : capabilities)
    {
        state.mCapabilities.emplace_back(capability, glIsEnabled(capability) == GL_TRUE);
    }
    GLint clip_distance_count = 0;
    glGetIntegerv(GL_MAX_CLIP_DISTANCES, &clip_distance_count);
    for (GLint index = 0; index < clip_distance_count; ++index)
    {
        const GLenum capability = static_cast<GLenum>(GL_CLIP_DISTANCE0 + index);
        state.mCapabilities.emplace_back(capability, glIsEnabled(capability) == GL_TRUE);
    }
    return glGetError() == GL_NO_ERROR;
}

struct AmbientPoisonBuffers
{
    std::array<GLuint, 2> mNames{};
    std::array<GLuint, 2> mCubeNames{};
    GLuint mVertexArray = 0;
    GLint mCopyDiffuseLocation = -1;
    GLint mCopyDiffuseValue = 0;
    bool mCopyUniformsDirty = false;

    bool initialize()
    {
        glGenBuffers(static_cast<GLsizei>(mNames.size()), mNames.data());
        glGenTextures(static_cast<GLsizei>(mCubeNames.size()), mCubeNames.data());
        glGenVertexArrays(1, &mVertexArray);
        if (mNames[0] == 0 || mNames[1] == 0 || mCubeNames[0] == 0 || mCubeNames[1] == 0 || mVertexArray == 0)
        {
            return false;
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, mNames[0]);
        glBufferData(GL_PIXEL_PACK_BUFFER, 4096, nullptr, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mNames[1]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, 4096, nullptr, GL_STREAM_DRAW);
        return glGetError() == GL_NO_ERROR;
    }

    void cleanup()
    {
        glBindVertexArray(0);
        if (mVertexArray != 0)
        {
            glDeleteVertexArrays(1, &mVertexArray);
            mVertexArray = 0;
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        if (mNames[0] != 0 || mNames[1] != 0)
        {
            glDeleteBuffers(static_cast<GLsizei>(mNames.size()), mNames.data());
            mNames = {};
        }
        if (mCubeNames[0] != 0 || mCubeNames[1] != 0)
        {
            glDeleteTextures(static_cast<GLsizei>(mCubeNames.size()), mCubeNames.data());
            mCubeNames = {};
        }
        LLVertexBuffer::sGLRenderBuffer = 0;
        LLVertexBuffer::sGLRenderIndices = 0;
        LLVertexBuffer::sLastMask = 0;
    }

    ~AmbientPoisonBuffers() { cleanup(); }
};

bool poisonAmbientGLState(TextureResources& ambient_resources, AmbientPoisonBuffers& buffers)
{
    clear_glerror();
    ambient_resources.mOutput.bindTarget();
    buffers.mCopyUniformsDirty = gCopyProgram.mUniformsDirty;
    gCopyProgram.mUniformsDirty = false;
    gCopyProgram.bind();
    buffers.mCopyDiffuseLocation = glGetUniformLocation(gCopyProgram.mProgramObject, "diffuseMap");
    if (buffers.mCopyDiffuseLocation < 0)
    {
        return false;
    }
    glGetUniformiv(gCopyProgram.mProgramObject, buffers.mCopyDiffuseLocation, &buffers.mCopyDiffuseValue);
    glUniform1i(buffers.mCopyDiffuseLocation, 3);
    ambient_resources.mScreenTriangle->setBuffer();
    if (!gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, ambient_resources.mOldImage->getTexName(), true))
    {
        return false;
    }
    gGL.getTexUnit(0)->setHasMipMaps(false);
    glBindSampler(0, ambient_resources.mSampler);
    if (!buffers.initialize())
    {
        return false;
    }
    if (!gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_CUBE_MAP, buffers.mCubeNames[0], false))
    {
        return false;
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, buffers.mCubeNames[1]);
    glBindTexture(GL_TEXTURE_2D, ambient_resources.mOldImage->getTexName());
    if (!gDebugProgram.isComplete() || gDebugProgram.mProgramObject == gCopyProgram.mProgramObject)
    {
        return false;
    }
    gDebugProgram.bind();
    gCopyProgram.mUniformsDirty = true;

    glPixelStorei(GL_PACK_ALIGNMENT, 8);
    glPixelStorei(GL_PACK_ROW_LENGTH, 13);
    glPixelStorei(GL_PACK_SKIP_ROWS, 1);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 2);
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, 7);
    glPixelStorei(GL_PACK_SKIP_IMAGES, 1);
    glPixelStorei(GL_PACK_SWAP_BYTES, GL_TRUE);
    glPixelStorei(GL_PACK_LSB_FIRST, GL_TRUE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 13);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 1);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 2);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 7);
    glPixelStorei(GL_UNPACK_SKIP_IMAGES, 1);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE);
#ifdef GL_PACK_INVERT_MESA
    if (gGLManager.mGLExtensions.contains("GL_MESA_pack_invert"))
    {
        glPixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);
    }
#endif
#ifdef GL_UNPACK_CLIENT_STORAGE_APPLE
    if (gGLManager.mGLExtensions.contains("GL_APPLE_client_storage"))
    {
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
    }
#endif

    glViewport(1, 0, 3, 2);
    glScissor(1, 0, 2, 1);
    gGL.setColorMask(false, true, false, true);
#if !LL_DARWIN
    if (gGLManager.mGLVersion >= 4.49f && glClipControl)
    {
        glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
    }
#endif
    GLint profile_mask = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_mask);
    if ((profile_mask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) == 0)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT, GL_LINE);
        glPolygonMode(GL_BACK, GL_POINT);
    }
    constexpr std::array<GLenum, 15> enabled{ GL_BLEND,
                                              GL_CULL_FACE,
                                              GL_DEPTH_TEST,
                                              GL_STENCIL_TEST,
                                              GL_SCISSOR_TEST,
                                              GL_DITHER,
                                              GL_FRAMEBUFFER_SRGB,
                                              GL_RASTERIZER_DISCARD,
                                              GL_MULTISAMPLE,
                                              GL_SAMPLE_ALPHA_TO_COVERAGE,
                                              GL_SAMPLE_ALPHA_TO_ONE,
                                              GL_SAMPLE_COVERAGE,
                                              GL_SAMPLE_MASK,
                                              GL_SAMPLE_SHADING,
                                              GL_COLOR_LOGIC_OP };
    for (GLenum capability : enabled)
    {
        glEnable(capability);
    }
    GLint clip_distance_count = 0;
    glGetIntegerv(GL_MAX_CLIP_DISTANCES, &clip_distance_count);
    for (GLint index = 0; index < clip_distance_count; ++index)
    {
        glEnable(static_cast<GLenum>(GL_CLIP_DISTANCE0 + index));
    }
    glBindVertexArray(buffers.mVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, buffers.mNames[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.mNames[1]);
    LLVertexBuffer::sGLRenderBuffer = buffers.mNames[0];
    LLVertexBuffer::sGLRenderIndices = buffers.mNames[1];
    LLVertexBuffer::sLastMask = 0;
    gGL.getTexUnit(1)->activate();
    return glGetError() == GL_NO_ERROR;
}

void cleanupAmbientGLState(TextureResources& ambient_resources, AmbientPoisonBuffers& buffers)
{
    gGL.getTexUnit(0)->activate();
    if (ambient_resources.mOutput.isBoundInStack())
    {
        ambient_resources.mOutput.flush();
    }
    gCopyProgram.mUniformsDirty = false;
    gCopyProgram.bind();
    if (buffers.mCopyDiffuseLocation >= 0)
    {
        glUniform1i(buffers.mCopyDiffuseLocation, buffers.mCopyDiffuseValue);
    }
    gCopyProgram.mUniformsDirty = buffers.mCopyUniformsDirty;
    gCopyProgram.unbind();
    glBindSampler(0, 0);
    buffers.cleanup();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, 0);
    glPixelStorei(GL_PACK_SKIP_IMAGES, 0);
    glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
    glPixelStorei(GL_PACK_LSB_FIRST, GL_FALSE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
#ifdef GL_PACK_INVERT_MESA
    if (gGLManager.mGLExtensions.contains("GL_MESA_pack_invert")) glPixelStorei(GL_PACK_INVERT_MESA, GL_FALSE);
#endif
#ifdef GL_UNPACK_CLIENT_STORAGE_APPLE
    if (gGLManager.mGLExtensions.contains("GL_APPLE_client_storage"))
    {
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
    }
#endif
    constexpr std::array<GLenum, 15> enabled{ GL_BLEND,
                                              GL_CULL_FACE,
                                              GL_DEPTH_TEST,
                                              GL_STENCIL_TEST,
                                              GL_SCISSOR_TEST,
                                              GL_DITHER,
                                              GL_FRAMEBUFFER_SRGB,
                                              GL_RASTERIZER_DISCARD,
                                              GL_MULTISAMPLE,
                                              GL_SAMPLE_ALPHA_TO_COVERAGE,
                                              GL_SAMPLE_ALPHA_TO_ONE,
                                              GL_SAMPLE_COVERAGE,
                                              GL_SAMPLE_MASK,
                                              GL_SAMPLE_SHADING,
                                              GL_COLOR_LOGIC_OP };
    for (GLenum capability : enabled)
    {
        glDisable(capability);
    }
    glEnable(GL_DITHER);
    GLint clip_distance_count = 0;
    glGetIntegerv(GL_MAX_CLIP_DISTANCES, &clip_distance_count);
    for (GLint index = 0; index < clip_distance_count; ++index)
    {
        glDisable(static_cast<GLenum>(GL_CLIP_DISTANCE0 + index));
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#if !LL_DARWIN
    if (gGLManager.mGLVersion >= 4.49f && glClipControl)
    {
        glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
    }
#endif
    gGL.setColorMask(true, true, true, true);
    gGL.getTexUnit(0)->disable();
    clear_glerror();
}

bool emitFailure(const std::string& reason)
{
    const std::string result = "TEXTURE_UPLOAD_CONTRACT_PARITY result=fail reason=" + reason;
    std::cout << result << std::endl;
    LL_INFOS("RenderContractParity") << result << LL_ENDL;
    return false;
}

std::vector<std::uint8_t> tightBottomLeftSource(const StreamingUploadInputs& inputs)
{
    constexpr std::size_t row_bytes = TEXTURE_UPLOAD_RESIDENT_WIDTH * TEXTURE_UPLOAD_CHANNELS;
    std::vector<std::uint8_t> result(row_bytes * TEXTURE_UPLOAD_RESIDENT_HEIGHT);
    for (std::uint32_t destination_row = 0; destination_row < TEXTURE_UPLOAD_RESIDENT_HEIGHT; ++destination_row)
    {
        const std::uint32_t source_row = TEXTURE_UPLOAD_RESIDENT_HEIGHT - 1 - destination_row;
        std::copy_n(inputs.mPixels.data() + source_row * inputs.mRowPitch, row_bytes,
                    result.data() + destination_row * row_bytes);
    }
    return result;
}

LLPointer<LLImageRaw> rawImage(const std::uint8_t* bytes, std::uint32_t width, std::uint32_t height)
{
    return new LLImageRaw(bytes, static_cast<U16>(width), static_cast<U16>(height), static_cast<S8>(TEXTURE_UPLOAD_CHANNELS));
}

bool setCanonicalTextureParameters(GLuint name)
{
    if (!gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, name, true))
    {
        return false;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, TEXTURE_UPLOAD_MIP_LEVELS - 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000.f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1000.f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
    if (gGLManager.mHasAnisotropic)
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 1.f);
    }
    return glGetError() == GL_NO_ERROR;
}

bool seedImage(LLPointer<LLImageGL>& image, S32 discard,
               const std::array<std::uint8_t, TEXTURE_UPLOAD_MIP_BYTE_COUNT>& mip_bytes)
{
    image = new LLImageGL(true, false);
    image->setNeedsAlphaAndPickMask(false);
    image->setExplicitFormat(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);

    const LLPointer<LLImageRaw> base = rawImage(mip_bytes.data(), TEXTURE_UPLOAD_RESIDENT_WIDTH,
                                                TEXTURE_UPLOAD_RESIDENT_HEIGHT);
    clear_glerror();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    if (!image->createGLTexture(discard, base) || image->getTexName() == 0)
    {
        return false;
    }

    if (!gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, image->getTexName(), true))
    {
        return false;
    }
    for (std::uint32_t mip = 0; mip < TEXTURE_UPLOAD_MIP_LEVELS; ++mip)
    {
        const GLsizei width  = static_cast<GLsizei>(TEXTURE_UPLOAD_RESIDENT_WIDTH >> mip);
        const GLsizei height = static_cast<GLsizei>(TEXTURE_UPLOAD_RESIDENT_HEIGHT >> mip);
        LLImageGL::setManualImage(GL_TEXTURE_2D, static_cast<S32>(mip), GL_RGBA8, width, height, GL_RGBA,
                                  GL_UNSIGNED_BYTE, mip_bytes.data() + TEXTURE_UPLOAD_MIP_BYTE_OFFSETS[mip], false);
    }
    return setCanonicalTextureParameters(image->getTexName());
}

bool initializeScreenTriangle(TextureResources& resources, const TextureUploadFixture& fixture)
{
    resources.mScreenTriangle = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX);
    if (!resources.mScreenTriangle->allocateBuffer(3, 0) || resources.mScreenTriangle->getSize() != 48 ||
        resources.mScreenTriangle->getOffset(LLVertexBuffer::TYPE_VERTEX) != 0)
    {
        return false;
    }
    U8* destination = resources.mScreenTriangle->mapVertexBuffer(LLVertexBuffer::TYPE_VERTEX, 0, 3);
    if (!destination)
    {
        return false;
    }
    std::memcpy(destination, fixture.mScreenTriangle.data(), sizeof(fixture.mScreenTriangle));
    resources.mScreenTriangle->unmapBuffer();
    return true;
}

bool initializeSampler(TextureResources& resources)
{
    glGenSamplers(1, &resources.mSampler);
    if (resources.mSampler == 0)
    {
        return false;
    }
    glSamplerParameteri(resources.mSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(resources.mSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(resources.mSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(resources.mSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(resources.mSampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glSamplerParameterf(resources.mSampler, GL_TEXTURE_MIN_LOD, -1000.f);
    glSamplerParameterf(resources.mSampler, GL_TEXTURE_MAX_LOD, 1000.f);
    glSamplerParameterf(resources.mSampler, GL_TEXTURE_LOD_BIAS, 0.f);
    if (gGLManager.mHasAnisotropic)
    {
        glSamplerParameterf(resources.mSampler, GL_TEXTURE_MAX_ANISOTROPY, 1.f);
    }
    return glGetError() == GL_NO_ERROR;
}

bool seedOutput(TextureResources& resources, const TextureUploadFixture& fixture)
{
    if (!resources.mOutput.allocate(TEXTURE_UPLOAD_OUTPUT_WIDTH, TEXTURE_UPLOAD_OUTPUT_HEIGHT, GL_RGBA, false) ||
        !resources.mOutput.isComplete() || resources.mOutput.getNumTextures() != 1)
    {
        return false;
    }
    if (!gGL.getTexUnit(0)->bindManual(resources.mOutput.getUsage(), resources.mOutput.getTexture(0), true))
    {
        return false;
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEXTURE_UPLOAD_OUTPUT_WIDTH, TEXTURE_UPLOAD_OUTPUT_HEIGHT,
                    GL_RGBA, GL_UNSIGNED_BYTE, fixture.mOutputSentinelRGBA8.data());
    return glGetError() == GL_NO_ERROR;
}

bool initializeResources(TextureResources& resources, const TextureUploadFixture& fixture)
{
    clear_glerror();
    if (!initializeScreenTriangle(resources, fixture) ||
        !seedImage(resources.mOldImage, TEXTURE_UPLOAD_RESIDENT_DISCARD, fixture.mOldMipRGBA8) ||
        !seedImage(resources.mReplacementImage, TEXTURE_UPLOAD_RESIDENT_DISCARD,
                   fixture.mReplacementSentinelMipRGBA8) ||
        !seedOutput(resources, fixture) || !initializeSampler(resources))
    {
        return false;
    }
    resources.mLifecycle.mCurrentImage     = StreamingUploadHandles{}.mOldImage;
    resources.mLifecycle.mLastRevision    = fixture.mPriorRevision;
    resources.mLifecycle.mCompletionPending = false;
    return glGetError() == GL_NO_ERROR;
}

bool readTextureMip(GLuint texture, std::uint32_t mip, std::vector<std::uint8_t>& bytes)
{
    const std::uint32_t width  = TEXTURE_UPLOAD_RESIDENT_WIDTH >> mip;
    const std::uint32_t height = TEXTURE_UPLOAD_RESIDENT_HEIGHT >> mip;
    bytes.assign(static_cast<std::size_t>(width) * height * TEXTURE_UPLOAD_CHANNELS, 0);

    GLint active_texture = GL_TEXTURE0;
    GLint binding        = 0;
    GLint pack_buffer    = 0;
    GLint alignment      = 4;
    GLint row_length     = 0;
    GLint skip_rows      = 0;
    GLint skip_pixels    = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &pack_buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &row_length);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &skip_rows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &skip_pixels);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, static_cast<GLint>(mip), GL_RGBA, GL_UNSIGNED_BYTE, bytes.data());
    const bool success = glGetError() == GL_NO_ERROR;

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(binding));
    glPixelStorei(GL_PACK_ALIGNMENT, alignment);
    glPixelStorei(GL_PACK_ROW_LENGTH, row_length);
    glPixelStorei(GL_PACK_SKIP_ROWS, skip_rows);
    glPixelStorei(GL_PACK_SKIP_PIXELS, skip_pixels);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(pack_buffer));
    glActiveTexture(static_cast<GLenum>(active_texture));
    return success && glGetError() == GL_NO_ERROR;
}

bool readImage(const LLImageGL& image,
               std::array<std::vector<std::uint8_t>, TEXTURE_UPLOAD_MIP_LEVELS>& mips)
{
    if (image.getTexName() == 0)
    {
        return false;
    }
    for (std::uint32_t mip = 0; mip < TEXTURE_UPLOAD_MIP_LEVELS; ++mip)
    {
        if (!readTextureMip(image.getTexName(), mip, mips[mip]))
        {
            return false;
        }
    }
    return true;
}

bool readOutput(const LLRenderTarget& output, std::vector<std::uint8_t>& bytes)
{
    if (!readTextureMip(output.getTexture(0), 0, bytes))
    {
        return false;
    }
    bytes.resize(TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT);
    return true;
}

bool snapshot(const TextureResources& resources, TextureSnapshot& result)
{
    return readImage(*resources.mOldImage, result.mOldMips) &&
           readImage(*resources.mReplacementImage, result.mReplacementMips) &&
           readOutput(resources.mOutput, result.mOutput) &&
           ((result.mLifecycle = resources.mLifecycle), true);
}

bool sampleReplacement(TextureResources& resources)
{
    clear_glerror();
    resources.mOutput.bindTarget();
    {
        LLGLDepthTest depth(GL_FALSE, GL_FALSE);
        LLGLDisable   blend(GL_BLEND);
        LLGLDisable   scissor(GL_SCISSOR_TEST);
        gGL.setColorMask(true, true);
        gCopyProgram.bind();
        const S32 channel = gCopyProgram.enableTexture(LLShaderMgr::DIFFUSE_MAP);
        if (channel < 0 || !gGL.getTexUnit(channel)->bind(resources.mReplacementImage, true, true))
        {
            gCopyProgram.unbind();
            resources.mOutput.flush();
            return false;
        }
        glBindSampler(static_cast<GLuint>(channel), resources.mSampler);
        resources.mScreenTriangle->setBuffer();
        resources.mScreenTriangle->drawArrays(LLRender::TRIANGLES, 0, 3);
        glBindSampler(static_cast<GLuint>(channel), 0);
        gCopyProgram.disableTexture(LLShaderMgr::DIFFUSE_MAP);
        gCopyProgram.unbind();
    }
    resources.mOutput.flush();
    return glGetError() == GL_NO_ERROR;
}

bool captureArtifactPixels(TextureResources& resources, TextureUploadArtifact& result)
{
    result = makeTextureUploadArtifact();
    return readImage(*resources.mReplacementImage, result.mMipRGBA8) &&
           readOutput(resources.mOutput, result.mSampledRGBA8);
}

bool hasMultipleChangedTexels(const std::vector<std::uint8_t>& before, const std::vector<std::uint8_t>& after)
{
    if (before.size() != after.size() || after.size() < TEXTURE_UPLOAD_CHANNELS * 2 ||
        after.size() % TEXTURE_UPLOAD_CHANNELS != 0)
    {
        return false;
    }
    std::optional<std::size_t> first_changed;
    for (std::size_t offset = 0; offset < after.size(); offset += TEXTURE_UPLOAD_CHANNELS)
    {
        const auto before_texel = std::next(before.begin(), static_cast<std::ptrdiff_t>(offset));
        const auto after_texel = std::next(after.begin(), static_cast<std::ptrdiff_t>(offset));
        if (!std::equal(after_texel, std::next(after_texel, TEXTURE_UPLOAD_CHANNELS), before_texel))
        {
            if (!first_changed)
            {
                first_changed = offset;
            }
            else
            {
                const auto first_texel = std::next(after.begin(), static_cast<std::ptrdiff_t>(*first_changed));
                if (!std::equal(after_texel, std::next(after_texel, TEXTURE_UPLOAD_CHANNELS), first_texel))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void populateLifecycleEvidence(TextureUploadArtifact& artifact, const StreamingUploadInputs& inputs,
                               const LLRenderGLTextureUpload::LifecycleLedger& ledger)
{
    artifact.mPriorRevision               = ledger.mLastRevision;
    artifact.mRevision                    = inputs.mRevision;
    artifact.mCompletionCount             = ledger.mCompletionCount + 1;
    artifact.mCompletedDestination        = inputs.mHandles.mReplacementImage;
    artifact.mCompletedRevision           = inputs.mRevision;
    artifact.mCompletedFrame              = inputs.mFrame;
    artifact.mRetirementCount             = ledger.mRetirementCount + 1;
    artifact.mRetiredResource             = inputs.mHandles.mOldImage;
    artifact.mRetirementFrame             = inputs.mFrame;
    artifact.mOldResolvableBefore         = ledger.mCurrentImage == inputs.mHandles.mOldImage;
    artifact.mOldResolvableAfter          = false;
    artifact.mReplacementResolvableAfter = true;
}

bool nontrivialArtifact(const TextureUploadArtifact& before, const TextureUploadArtifact& artifact,
                        const StreamingUploadInputs& inputs)
{
    if (!validateTextureUploadArtifact(artifact) || artifact.mMipRGBA8[0] != tightBottomLeftSource(inputs) ||
        artifact.mSampledRGBA8 != artifact.mMipRGBA8[1] ||
        !hasMultipleChangedTexels(before.mSampledRGBA8, artifact.mSampledRGBA8))
    {
        return false;
    }
    for (std::size_t mip = 0; mip < TEXTURE_UPLOAD_MIP_LEVELS; ++mip)
    {
        if (!hasMultipleChangedTexels(before.mMipRGBA8[mip], artifact.mMipRGBA8[mip]))
        {
            return false;
        }
    }
    return true;
}

bool publishedLifecycle(const LLRenderGLTextureUpload::LifecycleLedger& ledger, const StreamingUploadInputs& inputs,
                        const TextureUploadArtifact& artifact)
{
    return ledger.mCurrentImage == inputs.mHandles.mReplacementImage && ledger.mLastRevision == inputs.mRevision &&
           !ledger.mCompletionPending && ledger.mCompletionCount == 1 &&
           ledger.mCompletedDestination == inputs.mHandles.mReplacementImage &&
           ledger.mCompletedRevision == inputs.mRevision && ledger.mCompletedFrame == inputs.mFrame &&
           ledger.mRetirementCount == 1 && ledger.mRetiredResource == inputs.mHandles.mOldImage &&
           ledger.mRetirementFrame == inputs.mFrame && artifact.mPriorRevision == TEXTURE_UPLOAD_PRIOR_REVISION &&
           artifact.mRevision == inputs.mRevision && artifact.mCompletionCount == ledger.mCompletionCount &&
           artifact.mCompletedDestination == ledger.mCompletedDestination &&
           artifact.mCompletedRevision == ledger.mCompletedRevision && artifact.mCompletedFrame == ledger.mCompletedFrame &&
           artifact.mRetirementCount == ledger.mRetirementCount &&
           artifact.mRetiredResource == ledger.mRetiredResource && artifact.mRetirementFrame == ledger.mRetirementFrame &&
           artifact.mOldResolvableBefore && !artifact.mOldResolvableAfter && artifact.mReplacementResolvableAfter;
}

bool executeDirect(TextureResources& resources, const StreamingUploadInputs& inputs, TextureUploadArtifact& artifact)
{
    TextureUploadArtifact before;
    if (!captureArtifactPixels(resources, before))
    {
        return false;
    }

    const std::vector<std::uint8_t> tight = tightBottomLeftSource(inputs);
    const LLPointer<LLImageRaw> raw = rawImage(tight.data(), TEXTURE_UPLOAD_RESIDENT_WIDTH,
                                               TEXTURE_UPLOAD_RESIDENT_HEIGHT);
    clear_glerror();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    if (!resources.mReplacementImage->createGLTexture(TEXTURE_UPLOAD_RESIDENT_DISCARD, raw) ||
        !setCanonicalTextureParameters(resources.mReplacementImage->getTexName()) || !sampleReplacement(resources))
    {
        return false;
    }

    TextureUploadArtifact completed;
    if (!captureArtifactPixels(resources, completed))
    {
        return false;
    }
    populateLifecycleEvidence(completed, inputs, resources.mLifecycle);
    if (!nontrivialArtifact(before, completed, inputs) || glGetError() != GL_NO_ERROR)
    {
        return false;
    }

    resources.mLifecycle.mCurrentImage          = inputs.mHandles.mReplacementImage;
    resources.mLifecycle.mLastRevision         = inputs.mRevision;
    resources.mLifecycle.mCompletionPending    = false;
    resources.mLifecycle.mCompletionCount      = completed.mCompletionCount;
    resources.mLifecycle.mCompletedDestination = completed.mCompletedDestination;
    resources.mLifecycle.mCompletedRevision    = completed.mCompletedRevision;
    resources.mLifecycle.mCompletedFrame       = completed.mCompletedFrame;
    resources.mLifecycle.mRetirementCount      = completed.mRetirementCount;
    resources.mLifecycle.mRetiredResource      = completed.mRetiredResource;
    resources.mLifecycle.mRetirementFrame      = completed.mRetirementFrame;
    artifact = std::move(completed);
    return true;
}

bool registerResources(LLRenderGLTextureUpload::Registry& registry, TextureResources& resources,
                       const StreamingUploadHandles& handles,
                       LLRenderGLTextureUpload::Sampler sampler = {})
{
    sampler.mName = resources.mSampler;
    return registry.addScreenTriangle(handles.mScreenTriangle, resources.mScreenTriangle) &&
           registry.addImageGenerations(handles.mOldImage, resources.mOldImage, handles.mReplacementImage,
                                        resources.mReplacementImage) &&
           registry.addOutput(handles.mOutput, &resources.mOutput) && registry.addSampler(handles.mSampler, sampler) &&
           registry.addPipeline(handles.mPipeline, { TEXTURE_UPLOAD_PROGRAM, 0 }, &gCopyProgram) &&
           registry.addLifecycle(&resources.mLifecycle);
}

using FrameMutation = std::function<void(FrameSnapshot&)>;

std::vector<std::pair<const char*, FrameMutation>> packetRejections()
{
    return {
        { "frame", [](FrameSnapshot& frame) { frame.mFrame = 0; } },
        { "destination", [](FrameSnapshot& frame) { ++frame.mUploads[0].mDestination.mGeneration; } },
        { "old_generation", [](FrameSnapshot& frame) { ++frame.mImages[0].mHandle.mGeneration; } },
        { "output_generation", [](FrameSnapshot& frame) { ++frame.mImages[2].mHandle.mGeneration; } },
        { "image_alias", [](FrameSnapshot& frame) { frame.mImages[2].mHandle = frame.mImages[0].mHandle; } },
        { "old_mip_count", [](FrameSnapshot& frame) { --frame.mImages[0].mMipLevels; } },
        { "replacement_mip_count", [](FrameSnapshot& frame) { --frame.mImages[1].mMipLevels; } },
        { "output_mip_count", [](FrameSnapshot& frame) { ++frame.mImages[2].mMipLevels; } },
        { "revision", [](FrameSnapshot& frame) { frame.mUploads[0].mRevision = TEXTURE_UPLOAD_PRIOR_REVISION; } },
        { "subresource", [](FrameSnapshot& frame) { frame.mUploads[0].mSubresource.mMipLevel = 1; } },
        { "offset", [](FrameSnapshot& frame) { frame.mUploads[0].mOffset.mX = 1; } },
        { "extent", [](FrameSnapshot& frame) { --frame.mUploads[0].mExtent.mWidth; } },
        { "logical_extent", [](FrameSnapshot& frame) { --frame.mUploads[0].mLogicalExtent.mWidth; } },
        { "discard", [](FrameSnapshot& frame) { --frame.mUploads[0].mResidentDiscard; } },
        { "format", [](FrameSnapshot& frame) { frame.mUploads[0].mSourceFormat = PixelFormat::RGB8Unorm; } },
        { "row_pitch", [](FrameSnapshot& frame) { --frame.mUploads[0].mRowPitch; } },
        { "row_origin", [](FrameSnapshot& frame) { frame.mUploads[0].mRowOrigin = RowOrigin::BottomLeft; } },
        { "mip_policy", [](FrameSnapshot& frame) { frame.mUploads[0].mMipGeneration = MipGeneration::Disabled; } },
        { "pixel_offset", [](FrameSnapshot& frame) { frame.mUploads[0].mPixels.mOffset = 1; } },
        { "pixel_size", [](FrameSnapshot& frame) { --frame.mUploads[0].mPixels.mSize; } },
        { "before_state", [](FrameSnapshot& frame) { frame.mUploads[0].mBefore = ImageState::ShaderRead; } },
        { "during_state", [](FrameSnapshot& frame) { frame.mUploads[0].mDuring = ImageState::ShaderRead; } },
        { "after_state", [](FrameSnapshot& frame) { frame.mUploads[0].mAfter = ImageState::ColorAttachment; } },
        { "sampler", [](FrameSnapshot& frame) { frame.mSamplers[0].mAddressU = AddressMode::Repeat; } },
        { "missing_sampler", [](FrameSnapshot& frame) { frame.mSamplers.clear(); } },
        { "pipeline", [](FrameSnapshot& frame) { frame.mPipelines[0].mProgram.mVariant = 1; } },
        { "missing_pipeline", [](FrameSnapshot& frame) { frame.mPipelines.clear(); } },
        { "output", [](FrameSnapshot& frame) { --frame.mImages[2].mExtent.mWidth; } },
        { "release", [](FrameSnapshot& frame) { frame.mReleases[0].mFrame++; } },
        { "released_resource",
          [](FrameSnapshot& frame) { frame.mReleases[0].mResource = ResourceHandle{ frame.mImages[1].mHandle }; } },
        { "missing_release", [](FrameSnapshot& frame) { frame.mReleases.clear(); } },
        { "extra_pass", [](FrameSnapshot& frame) { frame.mPasses.push_back(frame.mPasses[0]); } }
    };
}

bool setTextureParameter(LLImageGL& image, GLenum parameter, GLint value)
{
    if (!gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, image.getTexName(), true))
    {
        return false;
    }
    glTexParameteri(GL_TEXTURE_2D, parameter, value);
    return glGetError() == GL_NO_ERROR;
}

bool textureParameterEquals(const LLImageGL& image, GLenum parameter, GLint expected)
{
    GLint active = 0;
    GLint binding = 0;
    GLint actual = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    glBindTexture(GL_TEXTURE_2D, image.getTexName());
    glGetTexParameteriv(GL_TEXTURE_2D, parameter, &actual);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(binding));
    glActiveTexture(static_cast<GLenum>(active));
    return actual == expected && glGetError() == GL_NO_ERROR;
}

bool redefineTextureLevel(GLuint name, GLint level, GLint internal_format, GLsizei width, GLsizei height,
                          GLenum format)
{
    if (!gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, name, true))
    {
        return false;
    }
    const std::size_t components = format == GL_RGB ? 3 : 4;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * components);
    for (std::size_t index = 0; index < pixels.size(); ++index)
    {
        pixels[index] = static_cast<std::uint8_t>(0x31u + ((index * 37u) % 0xbdu));
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, level, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels.data());
    return glGetError() == GL_NO_ERROR;
}

bool textureLevelEquals(GLuint name, GLint level, GLenum parameter, GLint expected)
{
    if (!gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, name, true))
    {
        return false;
    }
    GLint actual = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, level, parameter, &actual);
    return actual == expected && glGetError() == GL_NO_ERROR;
}

bool samplerParameterEquals(GLuint sampler, GLenum parameter, GLint expected)
{
    GLint actual = 0;
    glGetSamplerParameteriv(sampler, parameter, &actual);
    return actual == expected && glGetError() == GL_NO_ERROR;
}

struct LiveRejection
{
    const char* mName;
    std::function<bool(TextureResources&)> mPoison;
    std::function<bool(TextureResources&)> mStillPoisoned;
    std::function<void(TextureResources&)> mCleanup;
};

std::vector<LiveRejection> liveRejections()
{
    const auto no_cleanup = [](TextureResources&) {};
    return {
        { "published_generation",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mCurrentImage = StreamingUploadHandles{}.mReplacementImage;
              return true;
          },
          [](TextureResources& resources)
          { return resources.mLifecycle.mCurrentImage == StreamingUploadHandles{}.mReplacementImage; }, no_cleanup },
        { "duplicate_revision",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mLastRevision = TEXTURE_UPLOAD_REVISION;
              return true;
          },
          [](TextureResources& resources) { return resources.mLifecycle.mLastRevision == TEXTURE_UPLOAD_REVISION; },
          no_cleanup },
        { "regressing_revision",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mLastRevision = TEXTURE_UPLOAD_REVISION + 1;
              return true;
          },
          [](TextureResources& resources) { return resources.mLifecycle.mLastRevision == TEXTURE_UPLOAD_REVISION + 1; },
          no_cleanup },
        { "completion_pending",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mCompletionPending = true;
              return true;
          },
          [](TextureResources& resources) { return resources.mLifecycle.mCompletionPending; }, no_cleanup },
        { "existing_completion",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mCompletionCount = 1;
              return true;
          },
          [](TextureResources& resources) { return resources.mLifecycle.mCompletionCount == 1; }, no_cleanup },
        { "active_texture_cache_mismatch",
          [](TextureResources&)
          {
              gGL.getTexUnit(1)->activate();
              glActiveTexture(GL_TEXTURE0);
              return glGetError() == GL_NO_ERROR;
          },
          [](TextureResources&)
          {
              GLint active = 0;
              glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
              return active == GL_TEXTURE0 && gGL.getCurrentTexUnitIndex() == 1 && glGetError() == GL_NO_ERROR;
          },
          [](TextureResources&) { gGL.getTexUnit(0)->activate(); } },
        { "shader_profiling",
          [](TextureResources&)
          {
              LLGLSLShader::sProfileEnabled = true;
              return true;
          },
          [](TextureResources&) { return LLGLSLShader::sProfileEnabled; },
          [](TextureResources&) { LLGLSLShader::sProfileEnabled = false; } },
        { "completion_destination",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mCompletedDestination = StreamingUploadHandles{}.mReplacementImage;
              return true;
          },
          [](TextureResources& resources)
          { return resources.mLifecycle.mCompletedDestination == StreamingUploadHandles{}.mReplacementImage; },
          no_cleanup },
        { "completion_revision",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mCompletedRevision = TEXTURE_UPLOAD_REVISION;
              return true;
          },
          [](TextureResources& resources)
          { return resources.mLifecycle.mCompletedRevision == TEXTURE_UPLOAD_REVISION; }, no_cleanup },
        { "completion_frame",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mCompletedFrame = TEXTURE_UPLOAD_DIAGNOSTIC_FRAME;
              return true;
          },
          [](TextureResources& resources)
          { return resources.mLifecycle.mCompletedFrame == TEXTURE_UPLOAD_DIAGNOSTIC_FRAME; }, no_cleanup },
        { "existing_retirement",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mRetirementCount = 1;
              return true;
          },
          [](TextureResources& resources) { return resources.mLifecycle.mRetirementCount == 1; }, no_cleanup },
        { "retired_resource",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mRetiredResource = StreamingUploadHandles{}.mOldImage;
              return true;
          },
          [](TextureResources& resources)
          { return resources.mLifecycle.mRetiredResource == StreamingUploadHandles{}.mOldImage; }, no_cleanup },
        { "retirement_frame",
          [](TextureResources& resources)
          {
              resources.mLifecycle.mRetirementFrame = TEXTURE_UPLOAD_DIAGNOSTIC_FRAME;
              return true;
          },
          [](TextureResources& resources)
          { return resources.mLifecycle.mRetirementFrame == TEXTURE_UPLOAD_DIAGNOSTIC_FRAME; }, no_cleanup },
        { "replacement_format",
          [](TextureResources& resources)
          {
              resources.mReplacementImage->setExplicitFormat(GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, false);
              return true;
          },
          [](TextureResources& resources) { return resources.mReplacementImage->getPrimaryFormat() == GL_RGB; },
          no_cleanup },
        { "live_name_alias",
          [](TextureResources& resources)
          {
              resources.mSavedReplacementName = resources.mReplacementImage->getTexName();
              resources.mReplacementImage->setTexName(resources.mOldImage->getTexName());
              return resources.mSavedReplacementName != 0;
          },
          [](TextureResources& resources)
          { return resources.mReplacementImage->getTexName() == resources.mOldImage->getTexName(); },
          [](TextureResources& resources)
          {
              resources.mReplacementImage->setTexName(resources.mSavedReplacementName);
              resources.mSavedReplacementName = 0;
          } },
        { "old_level_format",
          [](TextureResources& resources)
          {
              const GLuint name = resources.mOldImage->getTexName();
              return redefineTextureLevel(name, 0, GL_RGB8, 8, 4, GL_RGB) &&
                     redefineTextureLevel(name, 1, GL_RGB8, 4, 2, GL_RGB) &&
                     redefineTextureLevel(name, 2, GL_RGB8, 2, 1, GL_RGB);
          },
          [](TextureResources& resources)
          {
              const GLuint name = resources.mOldImage->getTexName();
              return textureLevelEquals(name, 0, GL_TEXTURE_INTERNAL_FORMAT, GL_RGB8) &&
                     textureLevelEquals(name, 1, GL_TEXTURE_INTERNAL_FORMAT, GL_RGB8) &&
                     textureLevelEquals(name, 2, GL_TEXTURE_INTERNAL_FORMAT, GL_RGB8);
          }, no_cleanup },
        { "replacement_level_extent",
          [](TextureResources& resources)
          {
              return redefineTextureLevel(resources.mReplacementImage->getTexName(), 2, GL_RGBA8, 1, 1, GL_RGBA);
          },
          [](TextureResources& resources)
          { return textureLevelEquals(resources.mReplacementImage->getTexName(), 2, GL_TEXTURE_WIDTH, 1); },
          no_cleanup },
        { "output_level_format",
          [](TextureResources& resources)
          {
              return redefineTextureLevel(resources.mOutput.getTexture(0), 0, GL_RGB8,
                                          TEXTURE_UPLOAD_OUTPUT_WIDTH, TEXTURE_UPLOAD_OUTPUT_HEIGHT, GL_RGB);
          },
          [](TextureResources& resources)
          {
              return textureLevelEquals(resources.mOutput.getTexture(0), 0, GL_TEXTURE_INTERNAL_FORMAT, GL_RGB8);
          }, no_cleanup },
        { "replacement_mips",
          [](TextureResources& resources)
          { return setTextureParameter(*resources.mReplacementImage, GL_TEXTURE_MAX_LEVEL, 1); },
          [](TextureResources& resources)
          { return textureParameterEquals(*resources.mReplacementImage, GL_TEXTURE_MAX_LEVEL, 1); }, no_cleanup },
        { "old_mips", [](TextureResources& resources)
          { return setTextureParameter(*resources.mOldImage, GL_TEXTURE_MAX_LEVEL, 1); },
          [](TextureResources& resources) { return textureParameterEquals(*resources.mOldImage, GL_TEXTURE_MAX_LEVEL, 1); },
          no_cleanup },
        { "output_base_level",
          [](TextureResources& resources)
          {
              if (!gGL.getTexUnit(0)->bindManual(resources.mOutput.getUsage(), resources.mOutput.getTexture(0))) return false;
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 1);
              return glGetError() == GL_NO_ERROR;
          },
          [](TextureResources& resources)
          {
              GLint value = 0;
              if (!gGL.getTexUnit(0)->bindManual(resources.mOutput.getUsage(), resources.mOutput.getTexture(0))) return false;
              glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &value);
              return value == 1 && glGetError() == GL_NO_ERROR;
          }, no_cleanup },
        { "sampler_wrap",
          [](TextureResources& resources)
          {
              glSamplerParameteri(resources.mSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
              return glGetError() == GL_NO_ERROR;
          },
          [](TextureResources& resources)
          { return samplerParameterEquals(resources.mSampler, GL_TEXTURE_WRAP_S, GL_REPEAT); }, no_cleanup },
        { "sampler_filter",
          [](TextureResources& resources)
          {
              glSamplerParameteri(resources.mSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
              return glGetError() == GL_NO_ERROR;
          },
          [](TextureResources& resources)
          { return samplerParameterEquals(resources.mSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST); }, no_cleanup },
        { "program_identity", [](TextureResources&) { gCopyProgram.mName = "Rejected Copy Shader"; return true; },
          [](TextureResources&) { return gCopyProgram.mName == "Rejected Copy Shader"; },
          [](TextureResources&) { gCopyProgram.mName = "Copy Shader"; } },
        { "screen_buffer_allocation",
          [](TextureResources& resources)
          {
              gCopyProgram.bind();
              resources.mScreenTriangle->setBuffer();
              glBufferData(GL_ARRAY_BUFFER, 32, nullptr, GL_STATIC_DRAW);
              const bool success = glGetError() == GL_NO_ERROR;
              gCopyProgram.unbind();
              return success && glGetError() == GL_NO_ERROR;
          },
          [](TextureResources& resources)
          {
              gCopyProgram.bind();
              resources.mScreenTriangle->setBuffer();
              GLint size = 0;
              glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
              const bool poisoned = size == 32 && glGetError() == GL_NO_ERROR;
              gCopyProgram.unbind();
              return poisoned && glGetError() == GL_NO_ERROR;
          },
          [](TextureResources& resources)
          {
              gCopyProgram.bind();
              resources.mScreenTriangle->setBuffer();
              glBufferData(GL_ARRAY_BUFFER, 48, nullptr, GL_STATIC_DRAW);
              gCopyProgram.unbind();
              clear_glerror();
          } },
        { "output_bound", [](TextureResources& resources) { resources.mOutput.bindTarget(); return true; },
          [](TextureResources& resources) { return resources.mOutput.isBoundInStack(); },
          [](TextureResources& resources) { if (resources.mOutput.isBoundInStack()) resources.mOutput.flush(); } }
    };
}

} // namespace

bool run()
{
    if (!gGLManager.mInited || gGLManager.mGLVersion < 4.09f)
    {
        return emitFailure("unsupported_gl_version");
    }
    const TextureUploadFixture fixture     = makeTextureUploadFixture();
    const TextureUploadCase    upload_case = makeTextureUploadCase();
    const auto                 decoded     = decodeStreamingUploadFrame(upload_case.mFrame);
    if (!decoded)
    {
        return emitFailure("fixture_decode");
    }

    TextureResources direct_resources;
    if (!initializeResources(direct_resources, fixture))
    {
        return emitFailure("direct_resources");
    }
    TextureUploadArtifact direct_artifact;
    if (!executeDirect(direct_resources, *decoded, direct_artifact))
    {
        return emitFailure("direct_execution");
    }
    if (!publishedLifecycle(direct_resources.mLifecycle, *decoded, direct_artifact))
    {
        return emitFailure("direct_lifecycle");
    }

    TextureResources contract_resources;
    if (!initializeResources(contract_resources, fixture))
    {
        return emitFailure("contract_resources");
    }
    LLRenderGLTextureUpload::Registry registry;
    if (!registerResources(registry, contract_resources, upload_case.mInputs.mHandles))
    {
        return emitFailure("contract_registry");
    }

    std::size_t rejection_failures = 0;
    std::string first_failure;

    TextureSnapshot off_main_before;
    TextureSnapshot off_main_after;
    TextureUploadArtifact off_main_result;
    off_main_result.mSampledRGBA8 = { 0x5a };
    const TextureUploadArtifact off_main_result_before = off_main_result;
    const bool off_main_snapped_before = snapshot(contract_resources, off_main_before);
    bool off_main_accepted = true;
    std::thread off_main_worker(
        [&]
        {
            off_main_accepted = LLRenderGLTextureUpload::execute(upload_case.mFrame, registry, off_main_result);
        });
    off_main_worker.join();
    const bool off_main_snapped_after = snapshot(contract_resources, off_main_after);
    if (!off_main_snapped_before || !off_main_snapped_after || off_main_accepted ||
        off_main_result != off_main_result_before || off_main_before != off_main_after || glGetError() != GL_NO_ERROR)
    {
        ++rejection_failures;
        first_failure = "off_main_thread_rejection";
    }

    for (const auto& [name, mutate] : packetRejections())
    {
        TextureSnapshot before;
        TextureSnapshot after;
        if (!snapshot(contract_resources, before))
        {
            ++rejection_failures;
            if (first_failure.empty()) first_failure = std::string("snapshot_before ") + name;
            continue;
        }
        FrameSnapshot rejected = upload_case.mFrame;
        mutate(rejected);
        TextureUploadArtifact rejected_result = makeTextureUploadArtifact();
        const TextureUploadArtifact result_before = rejected_result;
        const bool accepted = LLRenderGLTextureUpload::execute(rejected, registry, rejected_result);
        if (!snapshot(contract_resources, after) || accepted || rejected_result != result_before || before != after ||
            glGetError() != GL_NO_ERROR)
        {
            ++rejection_failures;
            if (first_failure.empty()) first_failure = std::string("rejection ") + name;
        }
    }

    for (const LiveRejection& rejection : liveRejections())
    {
        TextureResources rejected_resources;
        LLRenderGLTextureUpload::Registry rejected_registry;
        TextureSnapshot before;
        TextureSnapshot after;
        TextureUploadArtifact rejected_result;
        rejected_result.mSampledRGBA8 = { 0x5a };
        const TextureUploadArtifact result_before = rejected_result;

        clear_glerror();
        const bool initialized = initializeResources(rejected_resources, fixture) &&
                                 registerResources(rejected_registry, rejected_resources, decoded->mHandles);
        const bool poisoned = initialized && rejection.mPoison(rejected_resources);
        const bool snapped_before = poisoned && snapshot(rejected_resources, before);
        const bool accepted = snapped_before &&
                              LLRenderGLTextureUpload::execute(upload_case.mFrame, rejected_registry, rejected_result);
        const bool snapped_after = initialized && snapshot(rejected_resources, after);
        const bool poison_unchanged = poisoned && rejection.mStillPoisoned(rejected_resources);
        const bool gl_clean = glGetError() == GL_NO_ERROR;
        if (poisoned) rejection.mCleanup(rejected_resources);
        gGL.getTexUnit(0)->disable();

        if (!initialized || !poisoned || !snapped_before || !snapped_after || accepted ||
            rejected_result != result_before || before != after || !poison_unchanged || !gl_clean)
        {
            ++rejection_failures;
            if (first_failure.empty()) first_failure = std::string("live_rejection ") + rejection.mName;
        }
    }

    std::array<std::vector<std::uint8_t>, TEXTURE_UPLOAD_MIP_LEVELS> old_before;
    std::array<std::vector<std::uint8_t>, TEXTURE_UPLOAD_MIP_LEVELS> old_after;
    TextureUploadArtifact contract_before;
    TextureUploadArtifact contract_artifact;
    std::string contract_error;
    const bool old_read_before = readImage(*contract_resources.mOldImage, old_before) &&
                                 captureArtifactPixels(contract_resources, contract_before);
    AmbientPoisonBuffers ambient_buffers;
    AmbientGLState ambient_before;
    AmbientGLState ambient_after;
    const bool ambient_poisoned = poisonAmbientGLState(direct_resources, ambient_buffers) &&
                                  captureAmbientGLState(ambient_before);
    const bool executed = rejection_failures == 0 && old_read_before && ambient_poisoned &&
                          LLRenderGLTextureUpload::execute(upload_case.mFrame, registry, contract_artifact,
                                                           &contract_error);
    const bool ambient_captured_after = ambient_poisoned && captureAmbientGLState(ambient_after);
    const bool ambient_state_restored = ambient_poisoned && ambient_captured_after && ambient_before == ambient_after;
    cleanupAmbientGLState(direct_resources, ambient_buffers);
    const bool old_read_after = readImage(*contract_resources.mOldImage, old_after);
    const bool contract_success = old_read_before && executed && ambient_state_restored && old_read_after &&
                                  old_before == old_after &&
                                  nontrivialArtifact(contract_before, contract_artifact, *decoded) &&
                                  publishedLifecycle(contract_resources.mLifecycle, *decoded, contract_artifact) &&
                                  !registry.isResolvable(decoded->mHandles.mOldImage) &&
                                  registry.isResolvable(decoded->mHandles.mReplacementImage);
    const TextureUploadComparisonStats comparison =
        compareTextureUploadArtifacts(direct_artifact, contract_artifact);
    bool success = contract_success && comparison.mComparable && comparison.mMatch;

    if (first_failure.empty() && !old_read_before) first_failure = "old_read_before";
    if (first_failure.empty() && !ambient_poisoned) first_failure = "ambient_state_poison";
    if (first_failure.empty() && !executed)
    {
        first_failure = contract_error.empty() ? "contract_execution" : "contract_execution " + contract_error;
    }
    if (first_failure.empty() && !ambient_state_restored) first_failure = "ambient_state_restore";
    if (first_failure.empty() && !old_read_after) first_failure = "old_read_after";
    if (first_failure.empty() && old_read_before && old_read_after && old_before != old_after)
    {
        first_failure = "old_generation_mutated";
    }
    if (first_failure.empty() && executed && !nontrivialArtifact(contract_before, contract_artifact, *decoded))
    {
        first_failure = "contract_artifact_trivial";
    }
    if (first_failure.empty() && executed &&
        (!publishedLifecycle(contract_resources.mLifecycle, *decoded, contract_artifact) ||
         registry.isResolvable(decoded->mHandles.mOldImage) ||
         !registry.isResolvable(decoded->mHandles.mReplacementImage)))
    {
        first_failure = "contract_lifecycle";
    }
    if (first_failure.empty() && !comparison.mComparable)
    {
        first_failure = "artifact_compare " + comparison.mError;
    }
    if (first_failure.empty() && !comparison.mMatch)
    {
        first_failure = "artifact_mismatch";
    }

    const std::string artifact_path = gSavedSettings.getString("RenderTextureUploadArtifactPath");
    bool artifact_written = false;
    if (success && !artifact_path.empty())
    {
        std::string artifact_error;
        artifact_written = writeTextureUploadArtifact(artifact_path, direct_artifact, &artifact_error);
        if (!artifact_written)
        {
            success = false;
            if (first_failure.empty()) first_failure = "artifact_write";
        }
        else if (!artifact_error.empty())
        {
            LL_WARNS("RenderContractParity") << artifact_error << LL_ENDL;
        }
    }

    std::ostringstream result;
    result << "TEXTURE_UPLOAD_CONTRACT_PARITY result=" << (success ? "pass" : "fail")
           << " resident=8x4 logical=32x16 discard=2 mips=3 sampled=4x2"
           << " mip_bytes=" << comparison.mComparedMipBytes
           << " sample_bytes=" << comparison.mComparedSampleBytes
           << " mismatches=" << comparison.mMismatchCount
           << " rejection_cases=" << 1 + packetRejections().size() + liveRejections().size()
           << " rejection_failures=" << rejection_failures
           << " completions=" << contract_artifact.mCompletionCount
           << " retirements=" << contract_artifact.mRetirementCount
           << " artifact=" << (artifact_path.empty() ? "disabled" : artifact_written ? "written" : "failed");
    if (!first_failure.empty())
    {
        result << " first_failure={" << first_failure << '}';
    }
    std::cout << result.str() << std::endl;
    LL_INFOS("RenderContractParity") << result.str() << LL_ENDL;
    return success;
}

} // namespace LLTextureUploadParity
