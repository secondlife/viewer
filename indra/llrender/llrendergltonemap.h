/**
 * @file llrendergltonemap.h
 * @brief Narrow OpenGL registry and executor for tonemap packets.
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

#ifndef LL_LLRENDERGLTONEMAP_H
#define LL_LLRENDERGLTONEMAP_H

#include "lltonemapcontract.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class LLGLSLShader;
class LLRenderTarget;
class LLVertexBuffer;

namespace LLRenderGLTonemap
{

enum class Sampler
{
    Point,
    Linear
};

// The registry borrows existing viewer objects for one synchronous replay.
class Registry
{
public:
    bool addBuffer(LLRenderContract::BufferHandle handle, LLVertexBuffer* buffer)
    {
        return add(mBuffers, handle, buffer);
    }

    bool addImage(LLRenderContract::ImageHandle handle, LLRenderTarget* image)
    {
        return add(mImages, handle, image);
    }

    bool addSampler(LLRenderContract::SamplerHandle handle, Sampler sampler)
    {
        if (!handle || hasIndex(mSamplers, handle.mIndex))
        {
            return false;
        }
        mSamplers.push_back({ handle, sampler });
        return true;
    }

    bool addPipeline(LLRenderContract::PipelineHandle handle, LLRenderContract::ShaderProgramKey program, LLGLSLShader* shader)
    {
        if (!handle || !shader || program.mName.empty() || hasIndex(mPipelines, handle.mIndex))
        {
            return false;
        }
        mPipelines.push_back({ handle, std::move(program), shader });
        return true;
    }

    LLVertexBuffer* resolve(LLRenderContract::BufferHandle handle) const
    {
        return resolveObject(mBuffers, handle);
    }

    LLRenderTarget* resolve(LLRenderContract::ImageHandle handle) const
    {
        return resolveObject(mImages, handle);
    }

    const Sampler* resolve(LLRenderContract::SamplerHandle handle) const
    {
        const auto found = std::find_if(mSamplers.begin(), mSamplers.end(),
                                        [handle](const SamplerEntry& entry) { return entry.mHandle == handle; });
        return found == mSamplers.end() ? nullptr : &found->mSampler;
    }

    LLGLSLShader* resolve(LLRenderContract::PipelineHandle handle, const LLRenderContract::ShaderProgramKey& program) const
    {
        const auto found = std::find_if(mPipelines.begin(), mPipelines.end(),
                                        [handle, &program](const PipelineEntry& entry)
                                        {
                                            return entry.mHandle == handle && entry.mProgram.mName == program.mName &&
                                                   entry.mProgram.mVariant == program.mVariant;
                                        });
        return found == mPipelines.end() ? nullptr : found->mShader;
    }

private:
    template<typename HandleType, typename ObjectType>
    struct ObjectEntry
    {
        HandleType  mHandle;
        ObjectType* mObject = nullptr;
    };

    struct SamplerEntry
    {
        LLRenderContract::SamplerHandle mHandle;
        Sampler                         mSampler = Sampler::Linear;
    };

    struct PipelineEntry
    {
        LLRenderContract::PipelineHandle mHandle;
        LLRenderContract::ShaderProgramKey mProgram;
        LLGLSLShader* mShader = nullptr;
    };

    template<typename Entry>
    static bool hasIndex(const std::vector<Entry>& entries, std::uint32_t index)
    {
        return std::any_of(entries.begin(), entries.end(),
                           [index](const Entry& entry) { return entry.mHandle.mIndex == index; });
    }

    template<typename HandleType, typename ObjectType>
    static bool add(std::vector<ObjectEntry<HandleType, ObjectType>>& entries, HandleType handle, ObjectType* object)
    {
        if (!handle || !object || hasIndex(entries, handle.mIndex))
        {
            return false;
        }
        entries.push_back({ handle, object });
        return true;
    }

    template<typename HandleType, typename ObjectType>
    static ObjectType* resolveObject(const std::vector<ObjectEntry<HandleType, ObjectType>>& entries, HandleType handle)
    {
        const auto found = std::find_if(entries.begin(), entries.end(),
                                        [handle](const ObjectEntry<HandleType, ObjectType>& entry) { return entry.mHandle == handle; });
        return found == entries.end() ? nullptr : found->mObject;
    }

    std::vector<ObjectEntry<LLRenderContract::BufferHandle, LLVertexBuffer>> mBuffers;
    std::vector<ObjectEntry<LLRenderContract::ImageHandle, LLRenderTarget>> mImages;
    std::vector<SamplerEntry> mSamplers;
    std::vector<PipelineEntry> mPipelines;
};

// Performs all validation and resolution before binding an FBO or changing GL state.
bool execute(const LLRenderContract::FrameSnapshot& frame, const Registry& registry);

}

#endif
