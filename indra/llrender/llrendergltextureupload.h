/**
 * @file llrendergltextureupload.h
 * @brief Narrow OpenGL registry and executor for one streamed texture upload.
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

#ifndef LL_LLRENDERGLTEXTUREUPLOAD_H
#define LL_LLRENDERGLTEXTUREUPLOAD_H

#include "llgltypes.h"
#include "lltextureuploaddiagnostic.h"

#include <cstdint>

class LLGLSLShader;
class LLImageGL;
class LLRenderTarget;
class LLVertexBuffer;

namespace LLRenderGLTextureUpload
{

struct Sampler
{
    LLGLuint                      mName          = 0;
    LLRenderContract::Filter      mMinFilter     = LLRenderContract::Filter::Linear;
    LLRenderContract::Filter      mMagFilter     = LLRenderContract::Filter::Linear;
    LLRenderContract::MipFilter   mMipFilter     = LLRenderContract::MipFilter::Linear;
    LLRenderContract::AddressMode mAddressU      = LLRenderContract::AddressMode::Clamp;
    LLRenderContract::AddressMode mAddressV      = LLRenderContract::AddressMode::Clamp;
    float                         mMaxAnisotropy = 1.f;

    friend constexpr bool operator==(const Sampler&, const Sampler&) = default;
};

// GL object destruction may happen later through LLImageGL's normal deferred
// deletion path. Publication itself uses the backend-neutral ledger.
using LifecycleLedger = LLRenderContract::StreamingUploadLifecycle;

using ExecutionResult = LLRenderContract::TextureUploadArtifact;

// The registry borrows all viewer and ledger objects for one synchronous
// replay. Registration and resolution never dereference a viewer pointer.
class Registry
{
public:
    bool addScreenTriangle(LLRenderContract::BufferHandle handle, LLVertexBuffer* buffer);
    bool addImageGenerations(LLRenderContract::ImageHandle old_handle, LLImageGL* old_image,
                             LLRenderContract::ImageHandle replacement_handle, LLImageGL* replacement_image);
    bool addOutput(LLRenderContract::ImageHandle handle, LLRenderTarget* output);
    bool addSampler(LLRenderContract::SamplerHandle handle, Sampler sampler);
    bool addPipeline(LLRenderContract::PipelineHandle handle, LLRenderContract::ShaderProgramKey program,
                     LLGLSLShader* shader);
    bool addLifecycle(LifecycleLedger* ledger);

    LLVertexBuffer* resolve(LLRenderContract::BufferHandle handle) const;
    LLImageGL* resolveRegisteredImage(LLRenderContract::ImageHandle handle) const;
    LLRenderTarget* resolveOutput(LLRenderContract::ImageHandle handle) const;
    const Sampler* resolve(LLRenderContract::SamplerHandle handle) const;
    LLGLSLShader* resolve(LLRenderContract::PipelineHandle handle,
                         const LLRenderContract::ShaderProgramKey& program) const;
    LifecycleLedger* lifecycle() const;

    // Logical resolution follows the publication ledger, not the lifetime of a
    // deferred GL name.
    bool isResolvable(LLRenderContract::ImageHandle handle) const;

private:
    LLRenderContract::BufferHandle mScreenHandle;
    LLVertexBuffer*                mScreenTriangle = nullptr;

    LLRenderContract::ImageHandle mOldHandle;
    LLImageGL*                     mOldImage = nullptr;
    LLRenderContract::ImageHandle mReplacementHandle;
    LLImageGL*                     mReplacementImage = nullptr;

    LLRenderContract::ImageHandle mOutputHandle;
    LLRenderTarget*                mOutput = nullptr;

    LLRenderContract::SamplerHandle mSamplerHandle;
    Sampler                         mSampler;

    LLRenderContract::PipelineHandle   mPipelineHandle;
    LLRenderContract::ShaderProgramKey mProgram;
    LLGLSLShader*                      mShader = nullptr;

    LifecycleLedger* mLifecycle = nullptr;
};

// Decodes the strict packet and preflights every packet, registry, ledger, and
// live GL invariant before uploading. On failure the result and ledger are left
// unchanged. On success result contains raw BottomLeft RGBA8 readback. The
// optional error identifies the failed gate without exposing driver data.
bool execute(const LLRenderContract::FrameSnapshot& frame, Registry& registry, ExecutionResult& result,
             std::string* error = nullptr);

} // namespace LLRenderGLTextureUpload

#endif // LL_LLRENDERGLTEXTUREUPLOAD_H
