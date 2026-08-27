/**
 * @file llmaterialparametercontract.h
 * @brief Pure materialization of legacy material shader parameters.
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

#ifndef LL_LLMATERIALPARAMETERCONTRACT_H
#define LL_LLMATERIALPARAMETERCONTRACT_H

#include "lldrawpacketcontract.h"
#include "llmaterialcontract.h"

#include <array>
#include <cstdint>
#include <optional>

namespace LLRenderContract
{

struct LegacyNormSpecWorldParameterContext
{
    std::uint64_t mFrame = 0;
    PassId        mPass;
    // Matrices use the viewer's contiguous column-major convention.
    DrawMatrix4          mBaseModelview = DRAW_IDENTITY_MATRIX4;
    DrawMatrix4          mProjection    = DRAW_IDENTITY_MATRIX4;
    std::array<float, 4> mEyeClipPlane{};

    friend constexpr bool operator==(const LegacyNormSpecWorldParameterContext&, const LegacyNormSpecWorldParameterContext&) = default;
};

// Produces the complete owned parameter block for a coherent production draw.
// The composed modelview must be affine; the projection may be projective.
std::optional<MaterialParameters> materializeLegacyNormSpecWorldParameters(const LegacyNormSpecDrawPacket&            packet,
                                                                           const LegacyNormSpecWorldParameterContext& context) noexcept;

} // namespace LLRenderContract

#endif // LL_LLMATERIALPARAMETERCONTRACT_H
