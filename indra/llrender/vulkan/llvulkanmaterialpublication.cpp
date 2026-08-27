/**
 * @file llvulkanmaterialpublication.cpp
 * @brief API-neutral publication lifecycle for owned material shader bytes.
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

#include "llvulkanmaterialpublication.h"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace LLRenderContract
{
namespace
{

    constexpr std::uint32_t LEGACY_NORMSPEC_SHADER_INDEX = 1;

    bool sameProgram(const ShaderProgramKey& left, const ShaderProgramKey& right) noexcept
    {
        return left.mName == right.mName && left.mVariant == right.mVariant;
    }

} // namespace

std::optional<ShaderHandle> LegacyNormSpecShaderPublication::publish(const LoadedShaderProgram& program) noexcept
{
    if (!validLegacyNormSpecProductionShaderProgram(program))
    {
        return std::nullopt;
    }

    try
    {
        auto immutable = std::make_shared<const LoadedShaderProgram>(program);
        if (!mCurrent)
        {
            mCurrent.emplace(Generation{ { LEGACY_NORMSPEC_SHADER_INDEX, 1 }, std::move(immutable), 0 });
            return mCurrent->mHandle;
        }

        const auto next_handle = nextHandleGeneration(mCurrent->mHandle);
        if (!next_handle)
        {
            return std::nullopt;
        }

        Generation replacement{ *next_handle, std::move(immutable), 0 };
        mPending.push_back(*mCurrent);
        static_assert(std::is_nothrow_move_assignable_v<Generation>);
        *mCurrent = std::move(replacement);
        return mCurrent->mHandle;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<ShaderHandle> LegacyNormSpecShaderPublication::current(const ShaderProgramKey& program) const noexcept
{
    if (!mCurrent || !sameProgram(mCurrent->mProgram->mProgram, program))
    {
        return std::nullopt;
    }
    return mCurrent->mHandle;
}

std::optional<ShaderGenerationLease> LegacyNormSpecShaderPublication::resolveForFrame(ShaderHandle handle, std::uint64_t frame) noexcept
{
    if (!mCurrent || handle != mCurrent->mHandle || frame == 0 || frame <= mCompletedThrough || frame < mLastRecordedFrame)
    {
        return std::nullopt;
    }

    mCurrent->mLastFrame = std::max(mCurrent->mLastFrame, frame);
    mLastRecordedFrame   = frame;
    return ShaderGenerationLease{ handle, frame, mCurrent->mProgram };
}

std::optional<std::vector<ShaderGenerationRetirement>> LegacyNormSpecShaderPublication::completeThrough(
    std::uint64_t completed_frame) noexcept
{
    if (completed_frame == 0 || completed_frame < mCompletedThrough)
    {
        return std::nullopt;
    }

    try
    {
        std::vector<ShaderGenerationRetirement> retired;
        std::vector<Generation>                 remaining;
        retired.reserve(mPending.size());
        remaining.reserve(mPending.size());
        for (const Generation& generation : mPending)
        {
            if (generation.mLastFrame <= completed_frame)
            {
                retired.push_back({ generation.mHandle, generation.mLastFrame });
            }
            else
            {
                remaining.push_back(generation);
            }
        }

        mPending          = std::move(remaining);
        mCompletedThrough = completed_frame;
        return retired;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

} // namespace LLRenderContract
