/**
 * @file llrendergltextureupload.cpp
 * @brief OpenGL replay of the canonical streamed texture upload.
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

#include "llrendergltextureupload.h"

#include "llgl.h"
#include "llglslshader.h"
#include "llimage.h"
#include "llimagegl.h"
#include "llrender.h"
#include "llrendertarget.h"
#include "llshadermgr.h"
#include "llthread.h"
#include "llvertexbuffer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace LLRenderGLTextureUpload
{

bool Registry::addScreenTriangle(LLRenderContract::BufferHandle handle, LLVertexBuffer* buffer)
{
    if (mScreenTriangle || !handle || !buffer)
    {
        return false;
    }
    mScreenHandle   = handle;
    mScreenTriangle = buffer;
    return true;
}

bool Registry::addImageGenerations(LLRenderContract::ImageHandle old_handle, LLImageGL* old_image,
                                   LLRenderContract::ImageHandle replacement_handle, LLImageGL* replacement_image)
{
    if (mOldImage || mReplacementImage || !old_handle || !replacement_handle || !old_image || !replacement_image ||
        old_image == replacement_image || old_handle.mIndex != replacement_handle.mIndex ||
        old_handle.mGeneration == std::numeric_limits<std::uint32_t>::max() ||
        replacement_handle.mGeneration != old_handle.mGeneration + 1 ||
        (mOutput && mOutputHandle.mIndex == old_handle.mIndex) ||
        (mLifecycle && mLifecycle->mCurrentImage != old_handle))
    {
        return false;
    }

    mOldHandle        = old_handle;
    mOldImage         = old_image;
    mReplacementHandle = replacement_handle;
    mReplacementImage = replacement_image;
    return true;
}

bool Registry::addOutput(LLRenderContract::ImageHandle handle, LLRenderTarget* output)
{
    if (mOutput || !handle || !output || (mOldImage && handle.mIndex == mOldHandle.mIndex))
    {
        return false;
    }
    mOutputHandle = handle;
    mOutput       = output;
    return true;
}

bool Registry::addSampler(LLRenderContract::SamplerHandle handle, Sampler sampler)
{
    const bool canonical = sampler.mName != 0 && sampler.mMinFilter == LLRenderContract::Filter::Linear &&
                           sampler.mMagFilter == LLRenderContract::Filter::Linear &&
                           sampler.mMipFilter == LLRenderContract::MipFilter::Linear &&
                           sampler.mAddressU == LLRenderContract::AddressMode::Clamp &&
                           sampler.mAddressV == LLRenderContract::AddressMode::Clamp &&
                           std::isfinite(sampler.mMaxAnisotropy) && sampler.mMaxAnisotropy == 1.f;
    if (mSamplerHandle || !handle || !canonical)
    {
        return false;
    }
    mSamplerHandle = handle;
    mSampler       = sampler;
    return true;
}

bool Registry::addPipeline(LLRenderContract::PipelineHandle handle, LLRenderContract::ShaderProgramKey program,
                           LLGLSLShader* shader)
{
    if (mShader || !handle || !shader || program.mName != "contract.sample-texture" || program.mVariant != 0)
    {
        return false;
    }
    mPipelineHandle = handle;
    mProgram        = std::move(program);
    mShader         = shader;
    return true;
}

bool Registry::addLifecycle(LifecycleLedger* ledger)
{
    if (mLifecycle || !ledger || !ledger->mCurrentImage || ledger->mCompletionPending || ledger->mCompletionCount != 0 ||
        ledger->mRetirementCount != 0 || ledger->mCompletedDestination || ledger->mRetiredResource ||
        ledger->mCompletedRevision != 0 || ledger->mCompletedFrame != 0 || ledger->mRetirementFrame != 0 ||
        (mOldImage && ledger->mCurrentImage != mOldHandle))
    {
        return false;
    }
    mLifecycle = ledger;
    return true;
}

LLVertexBuffer* Registry::resolve(LLRenderContract::BufferHandle handle) const
{
    return mScreenHandle == handle ? mScreenTriangle : nullptr;
}

LLImageGL* Registry::resolveRegisteredImage(LLRenderContract::ImageHandle handle) const
{
    if (mOldHandle == handle)
    {
        return mOldImage;
    }
    return mReplacementHandle == handle ? mReplacementImage : nullptr;
}

LLRenderTarget* Registry::resolveOutput(LLRenderContract::ImageHandle handle) const
{
    return mOutputHandle == handle ? mOutput : nullptr;
}

const Sampler* Registry::resolve(LLRenderContract::SamplerHandle handle) const
{
    return mSamplerHandle == handle ? &mSampler : nullptr;
}

LLGLSLShader* Registry::resolve(LLRenderContract::PipelineHandle handle,
                               const LLRenderContract::ShaderProgramKey& program) const
{
    return mPipelineHandle == handle && mProgram.mName == program.mName && mProgram.mVariant == program.mVariant ? mShader : nullptr;
}

LifecycleLedger* Registry::lifecycle() const
{
    return mLifecycle;
}

bool Registry::isResolvable(LLRenderContract::ImageHandle handle) const
{
    return mLifecycle && mLifecycle->mCurrentImage == handle && resolveRegisteredImage(handle);
}

namespace
{

using namespace LLRenderContract;

constexpr U32 SCREEN_VERTEX_MASK = LLVertexBuffer::MAP_VERTEX;
constexpr std::uint64_t COMPLETION_TIMEOUT_NS = 5'000'000'000ULL;

struct Prepared
{
    StreamingUploadInputs mInputs;
    LLVertexBuffer*        mScreenTriangle = nullptr;
    LLImageGL*             mOldImage = nullptr;
    LLImageGL*             mReplacementImage = nullptr;
    LLRenderTarget*        mOutput = nullptr;
    const Sampler*         mSampler = nullptr;
    LLGLSLShader*          mShader = nullptr;
    LifecycleLedger*       mLifecycle = nullptr;
};

bool noGlError()
{
    bool clean = true;
    while (glGetError() != GL_NO_ERROR)
    {
        clean = false;
    }
    return clean;
}

void setEnabled(GLenum capability, bool enabled)
{
    if (enabled)
    {
        glEnable(capability);
    }
    else
    {
        glDisable(capability);
    }
}

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

// Captures all state touched by live-resource inspection, upload, draw, and
// readback. Resource contents are intentionally outside this state snapshot.
class GLStateRestore
{
public:
    GLStateRestore()
    {
        glGetIntegerv(GL_ACTIVE_TEXTURE, &mActiveTexture);
        mCachedTextureUnitIndex = gGL.getCurrentTexUnitIndex();
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &mTexture2D);
        glGetIntegerv(GL_SAMPLER_BINDING, &mSampler);
        LLTexUnit* unit = gGL.getTexUnit(0);
        mCachedTexture = unit->getCurrTexture();
        mCachedTextureType = unit->getCurrType();
        mCachedTextureHasMips = unit->getHasMipMaps();
        const GLenum cached_binding_query = textureBindingQuery(mCachedTextureType);
        if (cached_binding_query != GL_NONE)
        {
            mCachedTextureTarget = LLTexUnit::getInternalType(mCachedTextureType);
            glGetIntegerv(cached_binding_query, &mRawCachedTargetTexture);
        }

        glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &mPackBuffer);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &mUnpackBuffer);
        capturePixelStore();
#ifdef GL_PACK_INVERT_MESA
        mHasPackInvert = gGLManager.mGLExtensions.contains("GL_MESA_pack_invert");
        if (mHasPackInvert)
        {
            glGetIntegerv(GL_PACK_INVERT_MESA, &mPackInvert);
        }
#endif
#ifdef GL_UNPACK_CLIENT_STORAGE_APPLE
        mHasClientStorage = gGLManager.mGLExtensions.contains("GL_APPLE_client_storage");
        if (mHasClientStorage)
        {
            glGetIntegerv(GL_UNPACK_CLIENT_STORAGE_APPLE, &mClientStorage);
        }
#endif

        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &mArrayBuffer);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &mElementBuffer);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &mVertexArray);
        mCachedArrayBuffer = LLVertexBuffer::sGLRenderBuffer;
        mCachedElementBuffer = LLVertexBuffer::sGLRenderIndices;
        mCachedAttributeMask = LLVertexBuffer::sLastMask;
        glGenVertexArrays(1, &mExecutionVertexArray);

        glGetIntegerv(GL_CURRENT_PROGRAM, &mProgram);
        mCachedProgram = LLGLSLShader::sCurBoundShader;
        mCachedShader = LLGLSLShader::sCurBoundShaderPtr;

        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &mDrawFramebuffer);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &mReadFramebuffer);
        glGetIntegerv(GL_READ_BUFFER, &mReadBuffer);
        GLint draw_buffer_count = 0;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &draw_buffer_count);
        mDrawBuffers.resize(static_cast<std::size_t>(std::max(1, draw_buffer_count)), GL_NONE);
        for (GLint index = 0; index < draw_buffer_count; ++index)
        {
            glGetIntegerv(static_cast<GLenum>(GL_DRAW_BUFFER0 + index), &mDrawBuffers[static_cast<std::size_t>(index)]);
        }
        mBoundTarget = LLRenderTarget::sBoundTarget;
        mCachedFramebuffer = LLRenderTarget::sCurFBO;
        mCachedWidth = LLRenderTarget::sCurResX;
        mCachedHeight = LLRenderTarget::sCurResY;

        glGetIntegerv(GL_VIEWPORT, mViewport.data());
        glGetIntegerv(GL_SCISSOR_BOX, mScissor.data());
        glGetBooleanv(GL_COLOR_WRITEMASK, mColorMask.data());
#if !LL_DARWIN
        mHasClipControl = gGLManager.mGLVersion >= 4.49f && glClipControl;
        if (mHasClipControl)
        {
            glGetIntegerv(GL_CLIP_ORIGIN, &mClipOrigin);
            glGetIntegerv(GL_CLIP_DEPTH_MODE, &mClipDepthMode);
        }
#endif
        GLint profile_mask = 0;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_mask);
        mCoreProfile = (profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) != 0;
        glGetIntegerv(GL_POLYGON_MODE, mPolygonMode.data());
        if (mCoreProfile)
        {
            mPolygonMode[1] = mPolygonMode[0];
        }

        constexpr std::array<GLenum, 15> capabilities{ GL_BLEND,
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
        for (GLenum capability : capabilities)
        {
            mCapabilities.emplace(capability, glIsEnabled(capability) == GL_TRUE);
        }
        GLint clip_distance_count = 0;
        glGetIntegerv(GL_MAX_CLIP_DISTANCES, &clip_distance_count);
        for (GLint index = 0; index < clip_distance_count; ++index)
        {
            const GLenum capability = static_cast<GLenum>(GL_CLIP_DISTANCE0 + index);
            mCapabilities.emplace(capability, glIsEnabled(capability) == GL_TRUE);
        }
        mValid = mExecutionVertexArray != 0 && noGlError() &&
                 mActiveTexture == static_cast<GLint>(GL_TEXTURE0 + mCachedTextureUnitIndex);
    }

    GLStateRestore(const GLStateRestore&) = delete;
    GLStateRestore& operator=(const GLStateRestore&) = delete;

    ~GLStateRestore() { restore(); }

    bool restore()
    {
        if (mRestored)
        {
            return mRestoreSucceeded;
        }
        mRestored = true;
        restoreFramebuffer();

        glUseProgram(static_cast<GLuint>(mProgram));
        LLGLSLShader::sCurBoundShader = mCachedProgram;
        LLGLSLShader::sCurBoundShaderPtr = mCachedShader;

        glBindVertexArray(static_cast<GLuint>(mVertexArray));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(mArrayBuffer));
        LLVertexBuffer::sGLRenderBuffer = mCachedArrayBuffer;
        LLVertexBuffer::sGLRenderIndices = mCachedElementBuffer;
        LLVertexBuffer::sLastMask = mCachedAttributeMask;
        if (mExecutionVertexArray != 0)
        {
            glDeleteVertexArrays(1, &mExecutionVertexArray);
            mExecutionVertexArray = 0;
        }

        restorePixelStore();
#ifdef GL_PACK_INVERT_MESA
        if (mHasPackInvert)
        {
            glPixelStorei(GL_PACK_INVERT_MESA, mPackInvert);
        }
#endif
#ifdef GL_UNPACK_CLIENT_STORAGE_APPLE
        if (mHasClientStorage)
        {
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, mClientStorage);
        }
#endif
        glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(mPackBuffer));
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(mUnpackBuffer));

        glActiveTexture(GL_TEXTURE0);
        if (mCachedTexture != 0)
        {
            gGL.getTexUnit(0)->bindManual(mCachedTextureType, mCachedTexture, mCachedTextureHasMips);
        }
        else
        {
            if (mCachedTextureType == LLTexUnit::TT_NONE)
            {
                gGL.getTexUnit(0)->disable();
            }
            else
            {
                gGL.getTexUnit(0)->enable(mCachedTextureType);
                gGL.getTexUnit(0)->unbind(mCachedTextureType);
            }
        }
        gGL.getTexUnit(0)->setHasMipMaps(mCachedTextureHasMips);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(mTexture2D));
        if (mCachedTextureTarget != GL_NONE && mCachedTextureTarget != GL_TEXTURE_2D)
        {
            glBindTexture(mCachedTextureTarget, static_cast<GLuint>(mRawCachedTargetTexture));
        }
        glBindSampler(0, static_cast<GLuint>(mSampler));
        gGL.getTexUnit(mCachedTextureUnitIndex)->activate();
        glActiveTexture(static_cast<GLenum>(mActiveTexture));

        glViewport(mViewport[0], mViewport[1], mViewport[2], mViewport[3]);
        glScissor(mScissor[0], mScissor[1], mScissor[2], mScissor[3]);
        glColorMask(mColorMask[0], mColorMask[1], mColorMask[2], mColorMask[3]);
#if !LL_DARWIN
        if (mHasClipControl)
        {
            glClipControl(static_cast<GLenum>(mClipOrigin), static_cast<GLenum>(mClipDepthMode));
        }
#endif
        if (mCoreProfile)
        {
            glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(mPolygonMode[0]));
        }
        else
        {
            glPolygonMode(GL_FRONT, static_cast<GLenum>(mPolygonMode[0]));
            glPolygonMode(GL_BACK, static_cast<GLenum>(mPolygonMode[1]));
        }
        for (const auto& [capability, enabled] : mCapabilities)
        {
            setEnabled(capability, enabled);
        }
        mRestoreSucceeded = noGlError();
        return mRestoreSucceeded;
    }

    bool valid() const { return mValid; }

    void isolateViewerCaches()
    {
        glBindVertexArray(mExecutionVertexArray);
        LLTexUnit* unit = gGL.getTexUnit(0);
        unit->enable(LLTexUnit::TT_TEXTURE);
        unit->unbind(LLTexUnit::TT_TEXTURE);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindSampler(0, 0);

        glUseProgram(0);
        LLGLSLShader::sCurBoundShader = 0;
        LLGLSLShader::sCurBoundShaderPtr = nullptr;
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        LLVertexBuffer::sGLRenderBuffer = 0;
        LLVertexBuffer::sGLRenderIndices = 0;
        LLVertexBuffer::sLastMask = 0;
    }

    void setTightPixelTransfer() const
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
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
        if (mHasPackInvert)
        {
            glPixelStorei(GL_PACK_INVERT_MESA, GL_FALSE);
        }
#endif
#ifdef GL_UNPACK_CLIENT_STORAGE_APPLE
        if (mHasClientStorage)
        {
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
        }
#endif
    }

private:
    void capturePixelStore()
    {
        constexpr std::array<GLenum, 16> names{ GL_PACK_ALIGNMENT,      GL_PACK_ROW_LENGTH,    GL_PACK_SKIP_ROWS,
                                                GL_PACK_SKIP_PIXELS,    GL_PACK_IMAGE_HEIGHT,  GL_PACK_SKIP_IMAGES,
                                                GL_PACK_SWAP_BYTES,     GL_PACK_LSB_FIRST,      GL_UNPACK_ALIGNMENT,
                                                GL_UNPACK_ROW_LENGTH,   GL_UNPACK_SKIP_ROWS,   GL_UNPACK_SKIP_PIXELS,
                                                GL_UNPACK_IMAGE_HEIGHT, GL_UNPACK_SKIP_IMAGES, GL_UNPACK_SWAP_BYTES,
                                                GL_UNPACK_LSB_FIRST };
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            glGetIntegerv(names[index], &mPixelStore[index]);
        }
    }

    void restorePixelStore() const
    {
        constexpr std::array<GLenum, 16> names{ GL_PACK_ALIGNMENT,      GL_PACK_ROW_LENGTH,    GL_PACK_SKIP_ROWS,
                                                GL_PACK_SKIP_PIXELS,    GL_PACK_IMAGE_HEIGHT,  GL_PACK_SKIP_IMAGES,
                                                GL_PACK_SWAP_BYTES,     GL_PACK_LSB_FIRST,      GL_UNPACK_ALIGNMENT,
                                                GL_UNPACK_ROW_LENGTH,   GL_UNPACK_SKIP_ROWS,   GL_UNPACK_SKIP_PIXELS,
                                                GL_UNPACK_IMAGE_HEIGHT, GL_UNPACK_SKIP_IMAGES, GL_UNPACK_SWAP_BYTES,
                                                GL_UNPACK_LSB_FIRST };
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            glPixelStorei(names[index], mPixelStore[index]);
        }
    }

    void restoreFramebuffer() const
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(mDrawFramebuffer));
        if (mDrawFramebuffer == 0)
        {
            glDrawBuffer(static_cast<GLenum>(mDrawBuffers.front()));
        }
        else
        {
            std::vector<GLenum> buffers;
            buffers.reserve(mDrawBuffers.size());
            std::transform(mDrawBuffers.begin(), mDrawBuffers.end(), std::back_inserter(buffers),
                           [](GLint value) { return static_cast<GLenum>(value); });
            glDrawBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(mReadFramebuffer));
        glReadBuffer(static_cast<GLenum>(mReadBuffer));
        LLRenderTarget::sBoundTarget = mBoundTarget;
        LLRenderTarget::sCurFBO = static_cast<U32>(mCachedFramebuffer);
        LLRenderTarget::sCurResX = static_cast<U32>(mCachedWidth);
        LLRenderTarget::sCurResY = static_cast<U32>(mCachedHeight);
    }

    bool mValid = false;
    bool mRestored = false;
    bool mRestoreSucceeded = false;
    GLint mActiveTexture = GL_TEXTURE0;
    U32 mCachedTextureUnitIndex = 0;
    GLint mTexture2D = 0;
    GLint mSampler = 0;
    U32 mCachedTexture = 0;
    LLTexUnit::eTextureType mCachedTextureType = LLTexUnit::TT_TEXTURE;
    bool mCachedTextureHasMips = false;
    GLenum mCachedTextureTarget = GL_NONE;
    GLint mRawCachedTargetTexture = 0;

    GLint mPackBuffer = 0;
    GLint mUnpackBuffer = 0;
    std::array<GLint, 16> mPixelStore{};
#ifdef GL_PACK_INVERT_MESA
    bool mHasPackInvert = false;
    GLint mPackInvert = GL_FALSE;
#endif
#ifdef GL_UNPACK_CLIENT_STORAGE_APPLE
    bool mHasClientStorage = false;
    GLint mClientStorage = GL_FALSE;
#endif

    GLint mArrayBuffer = 0;
    GLint mElementBuffer = 0;
    GLint mVertexArray = 0;
    GLuint mExecutionVertexArray = 0;
    U32 mCachedArrayBuffer = 0;
    U32 mCachedElementBuffer = 0;
    U32 mCachedAttributeMask = 0;

    GLint mProgram = 0;
    GLuint mCachedProgram = 0;
    LLGLSLShader* mCachedShader = nullptr;

    GLint mDrawFramebuffer = 0;
    GLint mReadFramebuffer = 0;
    GLint mReadBuffer = GL_BACK;
    std::vector<GLint> mDrawBuffers;
    LLRenderTarget* mBoundTarget = nullptr;
    GLint mCachedFramebuffer = 0;
    GLint mCachedWidth = 0;
    GLint mCachedHeight = 0;

    std::array<GLint, 4> mViewport{};
    std::array<GLint, 4> mScissor{};
    std::array<GLboolean, 4> mColorMask{};
#if !LL_DARWIN
    bool mHasClipControl = false;
    GLint mClipOrigin = GL_LOWER_LEFT;
    GLint mClipDepthMode = GL_NEGATIVE_ONE_TO_ONE;
#endif
    std::array<GLint, 2> mPolygonMode{};
    bool mCoreProfile = false;
    std::map<GLenum, bool> mCapabilities;
};

struct ActiveVariable
{
    GLint  mLocation = -1;
    GLint  mSize = 0;
    GLenum mType = 0;
};

using ActiveVariables = std::map<std::string, ActiveVariable>;

std::optional<ActiveVariables> activeVariables(GLuint program, GLenum count_name, GLenum max_length_name, bool attributes)
{
    GLint count = 0;
    GLint max_length = 0;
    glGetProgramiv(program, count_name, &count);
    glGetProgramiv(program, max_length_name, &max_length);
    if (count < 0 || max_length <= 0)
    {
        return std::nullopt;
    }

    std::vector<GLchar> name(static_cast<std::size_t>(max_length));
    ActiveVariables result;
    for (GLint index = 0; index < count; ++index)
    {
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
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
        const GLint location = attributes ? glGetAttribLocation(program, variable.c_str())
                                          : glGetUniformLocation(program, variable.c_str());
        if (location < 0 || !result.emplace(std::move(variable), ActiveVariable{ location, size, type }).second)
        {
            return std::nullopt;
        }
    }
    return result;
}

bool matchesShader(LLGLSLShader& shader)
{
    const std::vector<std::pair<std::string, GLenum>> expected_files{ { "interface/copyV.glsl", GL_VERTEX_SHADER },
                                                                      { "interface/copyF.glsl", GL_FRAGMENT_SHADER } };
    if (!shader.isComplete() || shader.mName != "Copy Shader" || shader.mShaderFiles != expected_files || !shader.mDefines.empty() ||
        shader.mProgramObject == 0 || !glIsProgram(shader.mProgramObject) ||
        shader.mAttributeMask != SCREEN_VERTEX_MASK || shader.mActiveTextureChannels != 1 ||
        shader.getTextureChannel(LLShaderMgr::DIFFUSE_MAP) != 0)
    {
        return false;
    }

    GLint linked = GL_FALSE;
    glGetProgramiv(shader.mProgramObject, GL_LINK_STATUS, &linked);
    const auto attributes = activeVariables(shader.mProgramObject, GL_ACTIVE_ATTRIBUTES, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, true);
    const auto uniforms = activeVariables(shader.mProgramObject, GL_ACTIVE_UNIFORMS, GL_ACTIVE_UNIFORM_MAX_LENGTH, false);
    if (linked != GL_TRUE || !attributes || !uniforms || attributes->size() != 1 || uniforms->size() != 1)
    {
        return false;
    }

    const auto position = attributes->find("position");
    const auto diffuse = uniforms->find("diffuseMap");
    return position != attributes->end() && position->second.mLocation == LLVertexBuffer::TYPE_VERTEX &&
           position->second.mSize == 1 && position->second.mType == GL_FLOAT_VEC3 && diffuse != uniforms->end() &&
           diffuse->second.mSize == 1 && diffuse->second.mType == GL_SAMPLER_2D &&
           shader.getUniformLocation(LLShaderMgr::DIFFUSE_MAP) == diffuse->second.mLocation &&
           glGetFragDataLocation(shader.mProgramObject, "frag_color") == 0 && noGlError();
}

bool matchesImageMetadata(const LLImageGL& image)
{
    return image.getCurrentWidth() == static_cast<S32>(TEXTURE_UPLOAD_LOGICAL_WIDTH) &&
           image.getCurrentHeight() == static_cast<S32>(TEXTURE_UPLOAD_LOGICAL_HEIGHT) &&
           image.getWidth() == static_cast<S32>(TEXTURE_UPLOAD_RESIDENT_WIDTH) &&
           image.getHeight() == static_cast<S32>(TEXTURE_UPLOAD_RESIDENT_HEIGHT) &&
           image.getDiscardLevel() == static_cast<S32>(TEXTURE_UPLOAD_RESIDENT_DISCARD) && image.getMaxDiscardLevel() == 4 &&
           image.getComponents() == TEXTURE_UPLOAD_CHANNELS && image.getUseMipMaps() && image.getHasExplicitFormat() &&
           image.getPrimaryFormat() == GL_RGBA && image.getFormatType() == GL_UNSIGNED_BYTE && image.getTexTarget() == GL_TEXTURE_2D &&
           image.getTarget() == LLTexUnit::TT_TEXTURE && image.getHasGLTexture() && image.isGLTextureCreated() && image.getTexName() != 0;
}

void bindPreservingUniformDirty(LLGLSLShader& shader)
{
    const bool uniforms_dirty = shader.mUniformsDirty;
    shader.mUniformsDirty = false;
    shader.bind();
    shader.mUniformsDirty = uniforms_dirty;
}

bool matchesTextureLevels(GLuint texture, std::uint32_t width, std::uint32_t height, std::uint32_t levels,
                          bool exact_mip_range, std::string* error = nullptr,
                          GLenum alternate_internal_format = GL_NONE)
{
    const auto fail = [error](std::string reason)
    {
        if (error)
        {
            *error = std::move(reason);
        }
        return false;
    };
    if (texture == 0 || !glIsTexture(texture))
    {
        return fail("name");
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLint base_level = -1;
    GLint max_level = -1;
    GLint swizzle_r = 0;
    GLint swizzle_g = 0;
    GLint swizzle_b = 0;
    GLint swizzle_a = 0;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &base_level);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &max_level);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, &swizzle_r);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, &swizzle_g);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, &swizzle_b);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, &swizzle_a);
    if (base_level != 0 || (exact_mip_range && max_level != static_cast<GLint>(levels - 1)) ||
        swizzle_r != GL_RED || swizzle_g != GL_GREEN || swizzle_b != GL_BLUE || swizzle_a != GL_ALPHA)
    {
        return fail("parameters");
    }

    for (std::uint32_t level = 0; level < levels; ++level)
    {
        GLint level_width = 0;
        GLint level_height = 0;
        GLint internal_format = 0;
        GLint compressed = GL_TRUE;
        GLint red_size = 0;
        GLint green_size = 0;
        GLint blue_size = 0;
        GLint alpha_size = 0;
        GLint red_type = 0;
        GLint green_type = 0;
        GLint blue_type = 0;
        GLint alpha_type = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_WIDTH, &level_width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_HEIGHT, &level_height);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_INTERNAL_FORMAT, &internal_format);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_COMPRESSED, &compressed);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_RED_SIZE, &red_size);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_GREEN_SIZE, &green_size);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_BLUE_SIZE, &blue_size);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_ALPHA_SIZE, &alpha_size);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_RED_TYPE, &red_type);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_GREEN_TYPE, &green_type);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_BLUE_TYPE, &blue_type);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(level), GL_TEXTURE_ALPHA_TYPE, &alpha_type);

        const GLint expected_width = static_cast<GLint>(std::max<std::uint32_t>(1, width >> level));
        const GLint expected_height = static_cast<GLint>(std::max<std::uint32_t>(1, height >> level));
        const std::string prefix = "level_" + std::to_string(level) + '_';
        if (level_width != expected_width || level_height != expected_height) return fail(prefix + "extent");
        if (internal_format != GL_RGBA8 && internal_format != static_cast<GLint>(alternate_internal_format))
        {
            return fail(prefix + "format_" + std::to_string(internal_format));
        }
        if (compressed != GL_FALSE) return fail(prefix + "compressed");
        if (red_size != 8 || green_size != 8 || blue_size != 8 || alpha_size != 8)
        {
            return fail(prefix + "component_size");
        }
        if (red_type != GL_UNSIGNED_NORMALIZED || green_type != GL_UNSIGNED_NORMALIZED ||
            blue_type != GL_UNSIGNED_NORMALIZED || alpha_type != GL_UNSIGNED_NORMALIZED)
        {
            return fail(prefix + "component_type");
        }
    }
    GLint extra_width = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, static_cast<GLint>(levels), GL_TEXTURE_WIDTH, &extra_width);
    if (extra_width != 0) return fail("extra_level");
    if (!noGlError()) return fail("gl_error");
    return true;
}

bool matchesOutput(LLRenderTarget& output, std::string* error = nullptr)
{
    const auto fail = [error](const char* reason)
    {
        if (error)
        {
            *error = reason;
        }
        return false;
    };
    if (!output.isComplete() || output.isBoundInStack() || output.getWidth() != TEXTURE_UPLOAD_OUTPUT_WIDTH ||
        output.getHeight() != TEXTURE_UPLOAD_OUTPUT_HEIGHT || output.getUsage() != LLTexUnit::TT_TEXTURE ||
        output.getNumTextures() != 1 || (output.getColorFormat() != GL_RGBA && output.getColorFormat() != GL_RGBA8) ||
        output.getDepth() != 0 || output.getTexture() == 0)
    {
        return fail("metadata");
    }
    std::string texture_error;
    if (!matchesTextureLevels(output.getTexture(), TEXTURE_UPLOAD_OUTPUT_WIDTH, TEXTURE_UPLOAD_OUTPUT_HEIGHT, 1,
                              false, &texture_error, GL_RGBA))
    {
        if (error)
        {
            *error = "texture_" + texture_error;
        }
        return false;
    }

    output.bindTarget();
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    GLint object_type = 0;
    GLint object_name = 0;
    GLint component_type = 0;
    GLint color_encoding = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &object_type);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &object_name);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &component_type);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &color_encoding);
    const bool valid = status == GL_FRAMEBUFFER_COMPLETE && object_type == GL_TEXTURE &&
                       object_name == static_cast<GLint>(output.getTexture()) && component_type == GL_UNSIGNED_NORMALIZED &&
                       color_encoding == GL_LINEAR && noGlError();
    output.flush();
    if (!valid) return fail("framebuffer");
    if (!noGlError()) return fail("flush");
    return true;
}

bool matchesSampler(const Sampler& sampler)
{
    if (!glIsSampler(sampler.mName))
    {
        return false;
    }
    GLint min_filter = 0;
    GLint mag_filter = 0;
    GLint wrap_s = 0;
    GLint wrap_t = 0;
    GLint compare_mode = 0;
    GLfloat min_lod = 0.f;
    GLfloat max_lod = 0.f;
    GLfloat lod_bias = 0.f;
    glGetSamplerParameteriv(sampler.mName, GL_TEXTURE_MIN_FILTER, &min_filter);
    glGetSamplerParameteriv(sampler.mName, GL_TEXTURE_MAG_FILTER, &mag_filter);
    glGetSamplerParameteriv(sampler.mName, GL_TEXTURE_WRAP_S, &wrap_s);
    glGetSamplerParameteriv(sampler.mName, GL_TEXTURE_WRAP_T, &wrap_t);
    glGetSamplerParameteriv(sampler.mName, GL_TEXTURE_COMPARE_MODE, &compare_mode);
    glGetSamplerParameterfv(sampler.mName, GL_TEXTURE_MIN_LOD, &min_lod);
    glGetSamplerParameterfv(sampler.mName, GL_TEXTURE_MAX_LOD, &max_lod);
    glGetSamplerParameterfv(sampler.mName, GL_TEXTURE_LOD_BIAS, &lod_bias);
    bool valid = min_filter == GL_LINEAR_MIPMAP_LINEAR && mag_filter == GL_LINEAR && wrap_s == GL_CLAMP_TO_EDGE &&
                 wrap_t == GL_CLAMP_TO_EDGE && compare_mode == GL_NONE && min_lod == -1000.f && max_lod == 1000.f && lod_bias == 0.f;
    if (gGLManager.mHasAnisotropic)
    {
        GLfloat anisotropy = 0.f;
        glGetSamplerParameterfv(sampler.mName, GL_TEXTURE_MAX_ANISOTROPY, &anisotropy);
        valid = valid && anisotropy == 1.f;
    }
    return valid && noGlError();
}

bool matchesScreenTriangle(LLVertexBuffer& buffer, LLGLSLShader& shader, std::string* error = nullptr)
{
    const auto fail = [error](std::string reason)
    {
        if (error)
        {
            *error = std::move(reason);
        }
        return false;
    };
    if (buffer.getNumVerts() != 3 || buffer.getNumIndices() != 0 || buffer.getTypeMask() != SCREEN_VERTEX_MASK ||
        buffer.getSize() != 48 || buffer.getIndicesSize() != 0 || buffer.getOffset(LLVertexBuffer::TYPE_VERTEX) != 0)
    {
        return fail("metadata");
    }

    bindPreservingUniformDirty(shader);
    buffer.setBuffer();
    GLint vertex_name = 0;
    GLint vertex_size = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vertex_name);
    if (vertex_name == 0 || vertex_name != static_cast<GLint>(LLVertexBuffer::sGLRenderBuffer) ||
        !glIsBuffer(static_cast<GLuint>(vertex_name)))
    {
        return fail("buffer_identity");
    }
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vertex_size);
    U32 expected_allocation = buffer.getSize();
#if LL_DARWIN || LL_ARM64
    if (!gGLManager.mIsApple)
#endif
    {
        U32 next_power_of_two = 1;
        while (next_power_of_two < expected_allocation)
        {
            next_power_of_two *= 2;
        }
        const U32 block_size = std::max(next_power_of_two / 8, 16U);
        expected_allocation += block_size - (expected_allocation % block_size);
    }
    if (vertex_size != static_cast<GLint>(expected_allocation))
    {
        return fail("allocation_" + std::to_string(vertex_size));
    }
    if (!noGlError())
    {
        return fail("gl_error");
    }
    return true;
}

std::optional<Prepared> prepare(const FrameSnapshot& frame, Registry& registry)
{
    auto inputs = decodeStreamingUploadFrame(frame);
    if (!inputs)
    {
        return std::nullopt;
    }
    if (inputs->mFrame != TEXTURE_UPLOAD_DIAGNOSTIC_FRAME || inputs->mHandles != StreamingUploadHandles{})
    {
        return std::nullopt;
    }
    Prepared result;
    result.mInputs = std::move(*inputs);
    result.mScreenTriangle = registry.resolve(result.mInputs.mHandles.mScreenTriangle);
    result.mOldImage = registry.resolveRegisteredImage(result.mInputs.mHandles.mOldImage);
    result.mReplacementImage = registry.resolveRegisteredImage(result.mInputs.mHandles.mReplacementImage);
    result.mOutput = registry.resolveOutput(result.mInputs.mHandles.mOutput);
    result.mSampler = registry.resolve(result.mInputs.mHandles.mSampler);
    result.mShader = registry.resolve(result.mInputs.mHandles.mPipeline, frame.mPipelines.front().mProgram);
    result.mLifecycle = registry.lifecycle();

    if (!result.mScreenTriangle || !result.mOldImage || !result.mReplacementImage || !result.mOutput || !result.mSampler ||
        !result.mShader || !result.mLifecycle || result.mOldImage == result.mReplacementImage ||
        result.mLifecycle->mCurrentImage != result.mInputs.mHandles.mOldImage ||
        result.mLifecycle->mLastRevision != TEXTURE_UPLOAD_PRIOR_REVISION ||
        result.mInputs.mRevision <= result.mLifecycle->mLastRevision || result.mLifecycle->mCompletionPending ||
        result.mLifecycle->mCompletionCount != 0 || result.mLifecycle->mCompletedDestination ||
        result.mLifecycle->mCompletedRevision != 0 || result.mLifecycle->mCompletedFrame != 0 ||
        result.mLifecycle->mRetirementCount != 0 || result.mLifecycle->mRetiredResource ||
        result.mLifecycle->mRetirementFrame != 0 ||
        !registry.isResolvable(result.mInputs.mHandles.mOldImage) ||
        registry.isResolvable(result.mInputs.mHandles.mReplacementImage) ||
        !matchesImageMetadata(*result.mOldImage) || !matchesImageMetadata(*result.mReplacementImage) ||
        !result.mOutput->isComplete())
    {
        return std::nullopt;
    }
    return result;
}

bool livePreflight(const Prepared& prepared, std::string* error)
{
    const auto fail = [error](const char* reason)
    {
        if (error)
        {
            *error = reason;
        }
        return false;
    };
    const GLuint old_name = prepared.mOldImage->getTexName();
    const GLuint replacement_name = prepared.mReplacementImage->getTexName();
    const GLuint output_name = prepared.mOutput->getTexture();
    if (old_name == replacement_name || old_name == output_name || replacement_name == output_name)
    {
        return fail("resource_alias");
    }
    std::string texture_error;
    if (!matchesTextureLevels(old_name, TEXTURE_UPLOAD_RESIDENT_WIDTH, TEXTURE_UPLOAD_RESIDENT_HEIGHT,
                              TEXTURE_UPLOAD_MIP_LEVELS, true, &texture_error))
    {
        if (error) *error = "old_image_" + texture_error;
        return false;
    }
    if (!matchesTextureLevels(replacement_name, TEXTURE_UPLOAD_RESIDENT_WIDTH, TEXTURE_UPLOAD_RESIDENT_HEIGHT,
                              TEXTURE_UPLOAD_MIP_LEVELS, true, &texture_error))
    {
        if (error) *error = "replacement_image_" + texture_error;
        return false;
    }
    std::string output_error;
    if (!matchesOutput(*prepared.mOutput, &output_error))
    {
        if (error)
        {
            *error = "output_" + output_error;
        }
        return false;
    }
    if (!matchesSampler(*prepared.mSampler)) return fail("sampler");
    if (!matchesShader(*prepared.mShader)) return fail("shader");
    std::string screen_error;
    if (!matchesScreenTriangle(*prepared.mScreenTriangle, *prepared.mShader, &screen_error))
    {
        if (error) *error = "screen_triangle_" + screen_error;
        return false;
    }
    if (!noGlError()) return fail("gl_error");
    return true;
}

LLPointer<LLImageRaw> normalizeSource(const StreamingUploadInputs& inputs)
{
    LLPointer<LLImageRaw> raw = new LLImageRaw(TEXTURE_UPLOAD_RESIDENT_WIDTH, TEXTURE_UPLOAD_RESIDENT_HEIGHT,
                                               TEXTURE_UPLOAD_CHANNELS);
    if (!raw || raw->isBufferInvalid())
    {
        return nullptr;
    }

    constexpr std::size_t tight_row = static_cast<std::size_t>(TEXTURE_UPLOAD_RESIDENT_WIDTH) * TEXTURE_UPLOAD_CHANNELS;
    for (std::size_t source_y = 0; source_y < TEXTURE_UPLOAD_RESIDENT_HEIGHT; ++source_y)
    {
        const std::size_t destination_y = TEXTURE_UPLOAD_RESIDENT_HEIGHT - 1 - source_y;
        std::memcpy(raw->getData() + destination_y * tight_row,
                    inputs.mPixels.data() + source_y * TEXTURE_UPLOAD_ROW_PITCH, tight_row);
    }
    return raw;
}

void configureDrawState()
{
    constexpr std::array<GLenum, 14> disabled{ GL_BLEND,
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
                                               GL_SAMPLE_SHADING };
    for (GLenum capability : disabled)
    {
        glDisable(capability);
    }
    glDisable(GL_COLOR_LOGIC_OP);
    GLint clip_distance_count = 0;
    glGetIntegerv(GL_MAX_CLIP_DISTANCES, &clip_distance_count);
    for (GLint index = 0; index < clip_distance_count; ++index)
    {
        glDisable(static_cast<GLenum>(GL_CLIP_DISTANCE0 + index));
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#if !LL_DARWIN
    if (gGLManager.mGLVersion >= 4.49f && glClipControl)
    {
        glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
    }
#endif
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glViewport(0, 0, TEXTURE_UPLOAD_OUTPUT_WIDTH, TEXTURE_UPLOAD_OUTPUT_HEIGHT);
}

class UniformRestore
{
public:
    UniformRestore(GLuint program, GLint location)
        : mProgram(program), mLocation(location)
    {
        glGetUniformiv(mProgram, mLocation, &mValue);
    }

    ~UniformRestore()
    {
        GLint current = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &current);
        glUseProgram(mProgram);
        glUniform1i(mLocation, mValue);
        glUseProgram(static_cast<GLuint>(current));
    }

private:
    GLuint mProgram = 0;
    GLint  mLocation = -1;
    GLint  mValue = 0;
};

bool drawSample(const Prepared& prepared)
{
    LLRenderTarget& output = *prepared.mOutput;
    output.bindTarget();
    configureDrawState();

    bindPreservingUniformDirty(*prepared.mShader);
    const GLint diffuse_location = glGetUniformLocation(prepared.mShader->mProgramObject, "diffuseMap");
    if (diffuse_location < 0)
    {
        output.flush();
        return false;
    }
    UniformRestore uniform_restore(prepared.mShader->mProgramObject, diffuse_location);
    glUniform1i(diffuse_location, 0);
    if (!gGL.getTexUnit(0)->bind(prepared.mReplacementImage, false, true))
    {
        output.flush();
        return false;
    }
    glBindSampler(0, prepared.mSampler->mName);
    prepared.mScreenTriangle->setBuffer();
    prepared.mScreenTriangle->drawArrays(LLRender::TRIANGLES, 0, 3);
    const bool success = noGlError();
    output.flush();
    return success && noGlError();
}

bool waitForCompletion()
{
    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    if (!sync)
    {
        noGlError();
        return false;
    }
    glFlush();
    const GLenum status = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, COMPLETION_TIMEOUT_NS);
    glDeleteSync(sync);
    return (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) && noGlError();
}

bool readMipmaps(const LLImageGL& image, ExecutionResult& result)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image.getTexName());
    for (std::uint32_t level = 0; level < TEXTURE_UPLOAD_MIP_LEVELS; ++level)
    {
        result.mMipRGBA8[level].resize(TEXTURE_UPLOAD_MIP_BYTE_SIZES[level]);
        glGetTexImage(GL_TEXTURE_2D, static_cast<GLint>(level), GL_RGBA, GL_UNSIGNED_BYTE,
                      result.mMipRGBA8[level].data());
    }
    return noGlError();
}

bool readOutput(LLRenderTarget& output, ExecutionResult& result)
{
    result.mSampledRGBA8.resize(TEXTURE_UPLOAD_OUTPUT_BYTE_COUNT);
    output.bindTarget();
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, TEXTURE_UPLOAD_OUTPUT_WIDTH, TEXTURE_UPLOAD_OUTPUT_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE,
                 result.mSampledRGBA8.data());
    const bool success = noGlError();
    output.flush();
    return success && noGlError();
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

bool verifiesUploadedContent(const ExecutionResult& before, const ExecutionResult& completed,
                            const LLImageRaw& normalized_source)
{
    const std::size_t source_size = static_cast<std::size_t>(normalized_source.getWidth()) *
                                    normalized_source.getHeight() * normalized_source.getComponents();
    if (completed.mMipRGBA8[0].size() != source_size ||
        !std::equal(completed.mMipRGBA8[0].begin(), completed.mMipRGBA8[0].end(), normalized_source.getData()))
    {
        return false;
    }
    for (std::size_t mip = 0; mip < TEXTURE_UPLOAD_MIP_LEVELS; ++mip)
    {
        if (!hasMultipleChangedTexels(before.mMipRGBA8[mip], completed.mMipRGBA8[mip]))
        {
            return false;
        }
    }
    return hasMultipleChangedTexels(before.mSampledRGBA8, completed.mSampledRGBA8) &&
           completed.mSampledRGBA8 == completed.mMipRGBA8[1];
}

} // namespace

bool execute(const LLRenderContract::FrameSnapshot& frame, Registry& registry, ExecutionResult& result,
             std::string* error)
{
    if (error)
    {
        error->clear();
    }
    const auto fail = [error](const char* reason)
    {
        if (error)
        {
            *error = reason;
        }
        return false;
    };

    if (!on_main_thread() || !gGLManager.mInited || gGLManager.mGLVersion < 4.09f || gGLManager.mIsDisabled ||
        LLGLSLShader::sProfileEnabled)
    {
        return fail("packet_registry_or_context_preflight");
    }

    const auto prepared = prepare(frame, registry);
    if (!prepared || !noGlError())
    {
        return fail("packet_registry_or_context_preflight");
    }

    GLStateRestore state;
    if (!state.valid())
    {
        return fail("gl_state_capture");
    }
    state.isolateViewerCaches();
    if (!noGlError())
    {
        return fail("gl_cache_isolation");
    }
    std::string live_error;
    if (!livePreflight(*prepared, &live_error))
    {
        if (error)
        {
            *error = "live_gl_preflight " + live_error;
        }
        return false;
    }

    // Owned normalization occurs only after every packet and live invariant has
    // passed. Poison padding never reaches the production upload path.
    LLPointer<LLImageRaw> raw = normalizeSource(prepared->mInputs);
    if (!raw)
    {
        return fail("source_normalization");
    }

    state.setTightPixelTransfer();
    ExecutionResult before;
    if (!readMipmaps(*prepared->mReplacementImage, before) || !readOutput(*prepared->mOutput, before))
    {
        return fail("pre_upload_readback");
    }

    state.setTightPixelTransfer();
    prepared->mReplacementImage->setAllowCompression(false);
    prepared->mReplacementImage->setExplicitFormat(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, false);
    const GLuint replacement_name = prepared->mReplacementImage->getTexName();
    if (!prepared->mReplacementImage->createGLTexture(static_cast<S32>(prepared->mInputs.mResidentDiscard), raw.get()) ||
        prepared->mReplacementImage->getTexName() != replacement_name || !noGlError() ||
        !matchesImageMetadata(*prepared->mReplacementImage) ||
        !matchesTextureLevels(replacement_name, TEXTURE_UPLOAD_RESIDENT_WIDTH, TEXTURE_UPLOAD_RESIDENT_HEIGHT,
                              TEXTURE_UPLOAD_MIP_LEVELS, true) || !drawSample(*prepared) ||
        !waitForCompletion())
    {
        return fail("upload_draw_or_completion");
    }

    state.setTightPixelTransfer();
    ExecutionResult completed = makeTextureUploadArtifact();
    completed.mPriorRevision = prepared->mLifecycle->mLastRevision;
    completed.mRevision = prepared->mInputs.mRevision;
    completed.mCompletedDestination = prepared->mInputs.mHandles.mReplacementImage;
    completed.mCompletedRevision = prepared->mInputs.mRevision;
    completed.mCompletedFrame = prepared->mInputs.mFrame;
    completed.mRetiredResource = prepared->mInputs.mHandles.mOldImage;
    completed.mRetirementFrame = prepared->mInputs.mFrame;
    completed.mOldResolvableBefore = registry.isResolvable(prepared->mInputs.mHandles.mOldImage);

    if (!readMipmaps(*prepared->mReplacementImage, completed) || !readOutput(*prepared->mOutput, completed))
    {
        return fail("readback");
    }

    LifecycleLedger next = *prepared->mLifecycle;
    next.mCurrentImage = prepared->mInputs.mHandles.mReplacementImage;
    next.mLastRevision = prepared->mInputs.mRevision;
    next.mCompletionPending = false;
    ++next.mCompletionCount;
    next.mCompletedDestination = prepared->mInputs.mHandles.mReplacementImage;
    next.mCompletedRevision = prepared->mInputs.mRevision;
    next.mCompletedFrame = prepared->mInputs.mFrame;
    ++next.mRetirementCount;
    next.mRetiredResource = prepared->mInputs.mHandles.mOldImage;
    next.mRetirementFrame = prepared->mInputs.mFrame;

    completed.mCompletionCount = next.mCompletionCount;
    completed.mRetirementCount = next.mRetirementCount;
    completed.mOldResolvableAfter = false;
    completed.mReplacementResolvableAfter = true;

    if (!validateTextureUploadArtifact(completed) || !verifiesUploadedContent(before, completed, *raw))
    {
        return fail("artifact_or_content_validation");
    }

    if (!state.restore())
    {
        return fail("gl_state_restore");
    }

    // Publication is the sole externally visible ownership mutation and occurs
    // after upload, synchronization, readback, validation, and clean GL restore.
    *prepared->mLifecycle = next;
    result = std::move(completed);
    return true;
}

} // namespace LLRenderGLTextureUpload
