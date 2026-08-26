/**
 * @file lltonemapcontract.h
 * @brief Backend-neutral description of the viewer tonemap pass.
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

#ifndef LL_LLTONEMAPCONTRACT_H
#define LL_LLTONEMAPCONTRACT_H

#include "llrendercontract.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace LLRenderContract
{

enum class TonemapVariant : std::uint64_t
{
    Deferred                  = 0,
    NoPost                    = 1,
    GammaCorrect              = 2,
    NoPostGammaCorrect        = 3,
    LegacyGammaCorrect        = 6,
    NoPostLegacyGammaCorrect  = 7
};

struct TonemapParameters
{
    float         mExposure   = 1.f;
    float         mTonemapMix = 1.f;
    std::uint32_t mTonemapType = 0;
    float         mGamma      = 1.f;

    friend constexpr bool operator==(const TonemapParameters&, const TonemapParameters&) = default;
};

static_assert(sizeof(TonemapParameters) == 16, "tonemap parameters must remain a four-word packet");
static_assert(offsetof(TonemapParameters, mExposure) == 0);
static_assert(offsetof(TonemapParameters, mTonemapMix) == 4);
static_assert(offsetof(TonemapParameters, mTonemapType) == 8);
static_assert(offsetof(TonemapParameters, mGamma) == 12);

struct TonemapHandles
{
    BufferHandle   mScreenTriangle{ 1, 1 };
    ImageHandle    mScene{ 1, 1 };
    ImageHandle    mExposure{ 2, 1 };
    ImageHandle    mDestination{ 3, 1 };
    SamplerHandle  mPointSampler{ 1, 1 };
    SamplerHandle  mLinearSampler{ 2, 1 };
    PipelineHandle mPipeline{ 1, 1 };
    PassId         mPass{ 1 };

    friend constexpr bool operator==(const TonemapHandles&, const TonemapHandles&) = default;
};

struct TonemapInputs
{
    std::uint64_t     mFrame = 0;
    TonemapHandles    mHandles;
    Extent2D          mSourceExtent;
    Extent2D          mDestinationExtent;
    PixelFormat       mDestinationFormat = PixelFormat::RGBA16Float;
    TonemapVariant    mVariant = TonemapVariant::Deferred;
    TonemapParameters mParameters;
};

bool validTonemapVariant(TonemapVariant variant) noexcept;

// Returns no packet when policy supplied an invalid or non-finite input.
std::optional<FrameSnapshot> buildTonemapFrame(const TonemapInputs& inputs);

// Accepts only the canonical tonemap packet shape and returns owned values.
std::optional<TonemapInputs> decodeTonemapFrame(const FrameSnapshot& frame);

}

#endif
