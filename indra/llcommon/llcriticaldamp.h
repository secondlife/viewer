/** 
 * @file llcriticaldamp.h
 * @brief A lightweight class that calculates critical damping constants once
 * per frame.  
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLCRITICALDAMP_H
#define LL_LLCRITICALDAMP_H

#include <vector>

#include "llframetimer.h"
#include "llunits.h"

class LL_COMMON_API LLSmoothInterpolation 
{
public:
    LLSmoothInterpolation();

    // MANIPULATORS
    static void updateInterpolants();

    // ACCESSORS
    static F32 getInterpolant(F32SecondsImplicit time_constant, bool use_cache = true);

    template<typename T> 
    static T lerp(T a, T b, F32SecondsImplicit time_constant, bool use_cache = true)
    {
        F32 interpolant = getInterpolant(time_constant, use_cache);
        return ((a * (1.f - interpolant)) 
                + (b * interpolant));
    }

protected:
    static F32 calcInterpolant(F32 time_constant);

    struct CompareTimeConstants;
    static LLFrameTimer sInternalTimer; // frame timer for calculating deltas

    struct Interpolant
    {
        F32 mTimeScale;
        F32 mInterpolant;
    };
    typedef std::vector<Interpolant> interpolant_vec_t;
    static interpolant_vec_t    sInterpolants;
    static F32                  sTimeDelta;
};

typedef LLSmoothInterpolation LLCriticalDamp;

#endif  // LL_LLCRITICALDAMP_H
