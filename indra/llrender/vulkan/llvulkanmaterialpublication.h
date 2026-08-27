/**
 * @file llvulkanmaterialpublication.h
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

#ifndef LL_LLVULKANMATERIALPUBLICATION_H
#define LL_LLVULKANMATERIALPUBLICATION_H

#include "llvulkanmaterialartifact.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace LLRenderContract
{

struct ShaderGenerationLease
{
    ShaderHandle                               mHandle;
    std::uint64_t                              mFrame = 0;
    std::shared_ptr<const LoadedShaderProgram> mProgram;
};

struct ShaderGenerationRetirement
{
    ShaderHandle  mHandle;
    std::uint64_t mLastFrame = 0;

    friend constexpr bool operator==(const ShaderGenerationRetirement&, const ShaderGenerationRetirement&) = default;
};

// One externally sequenced owner for the canonical legacy norm-spec program.
// Every recorded frame must acquire its own lease. Reusing a lease for a later
// frame would bypass last-use tracking and is outside this contract. Handles
// are relative to one owner instance and must never cross between owners. Old
// and replacement leases may coexist within one frame; both remain owned until
// that frame completes.
class LegacyNormSpecShaderPublication
{
public:
    LegacyNormSpecShaderPublication()  = default;
    ~LegacyNormSpecShaderPublication() = default;

    LegacyNormSpecShaderPublication(const LegacyNormSpecShaderPublication&)            = delete;
    LegacyNormSpecShaderPublication& operator=(const LegacyNormSpecShaderPublication&) = delete;
    LegacyNormSpecShaderPublication(LegacyNormSpecShaderPublication&&)                 = delete;
    LegacyNormSpecShaderPublication& operator=(LegacyNormSpecShaderPublication&&)      = delete;

    std::optional<ShaderHandle> publish(const LoadedShaderProgram& program) noexcept;
    std::optional<ShaderHandle> current(const ShaderProgramKey& program) const noexcept;

    std::optional<ShaderGenerationLease>                   resolveForFrame(ShaderHandle handle, std::uint64_t frame) noexcept;
    std::optional<std::vector<ShaderGenerationRetirement>> completeThrough(std::uint64_t completed_frame) noexcept;

    std::uint64_t completedThrough() const noexcept { return mCompletedThrough; }

private:
    struct Generation
    {
        ShaderHandle                               mHandle;
        std::shared_ptr<const LoadedShaderProgram> mProgram;
        std::uint64_t                              mLastFrame = 0;
    };

    std::optional<Generation> mCurrent;
    std::vector<Generation>   mPending;
    std::uint64_t             mCompletedThrough  = 0;
    std::uint64_t             mLastRecordedFrame = 0;
};

} // namespace LLRenderContract

#endif // LL_LLVULKANMATERIALPUBLICATION_H
