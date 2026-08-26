/**
 * @file llbenchmarkdisplay.h
 * @brief Display-scale helpers for renderer benchmarks.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

#ifndef LL_LLBENCHMARKDISPLAY_H
#define LL_LLBENCHMARKDISPLAY_H

#include "stdtypes.h"

namespace LLBenchmarkDisplay
{
inline F32 configuredUIScale(F32 current_scale, F32 target_scale, F32 backing_scale)
{
    if (target_scale <= 0.f || backing_scale <= 0.f)
    {
        return current_scale;
    }
    return target_scale / backing_scale;
}
}

#endif
