/** 
 * @file llcriticaldamp.cpp
 * @brief Implementation of the critical damping functionality.
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

#include "linden_common.h"

#include "llcriticaldamp.h"
#include <algorithm>

//-----------------------------------------------------------------------------
// static members
//-----------------------------------------------------------------------------
LLFrameTimer LLSmoothInterpolation::sInternalTimer;
std::vector<LLSmoothInterpolation::Interpolant> LLSmoothInterpolation::sInterpolants;
F32 LLSmoothInterpolation::sTimeDelta;

// helper functors
struct LLSmoothInterpolation::CompareTimeConstants
{
    bool operator()(const F32& a, const LLSmoothInterpolation::Interpolant& b) const
    {
        return a < b.mTimeScale;
    }

    bool operator()(const LLSmoothInterpolation::Interpolant& a, const F32& b) const
    {
        return a.mTimeScale < b; // bottom of a is higher than bottom of b
    }

    bool operator()(const LLSmoothInterpolation::Interpolant& a, const LLSmoothInterpolation::Interpolant& b) const
    {
        return a.mTimeScale < b.mTimeScale; // bottom of a is higher than bottom of b
    }
};

//-----------------------------------------------------------------------------
// LLSmoothInterpolation()
//-----------------------------------------------------------------------------
LLSmoothInterpolation::LLSmoothInterpolation()
{
    sTimeDelta = 0.f;
}

// static
//-----------------------------------------------------------------------------
// updateInterpolants()
//-----------------------------------------------------------------------------
void LLSmoothInterpolation::updateInterpolants()
{
    sTimeDelta = sInternalTimer.getElapsedTimeAndResetF32();

    for (S32 i = 0; i < sInterpolants.size(); i++)
    {
        Interpolant& interp = sInterpolants[i];
        interp.mInterpolant = calcInterpolant(interp.mTimeScale);
    }
} 

//-----------------------------------------------------------------------------
// getInterpolant()
//-----------------------------------------------------------------------------
F32 LLSmoothInterpolation::getInterpolant(F32SecondsImplicit time_constant, bool use_cache)
{
    if (time_constant == 0.f)
    {
        return 1.f;
    }

    if (use_cache)
    {
        interpolant_vec_t::iterator find_it = std::lower_bound(sInterpolants.begin(), sInterpolants.end(), time_constant.value(), CompareTimeConstants());
        if (find_it != sInterpolants.end() && find_it->mTimeScale == time_constant) 
        {
            return find_it->mInterpolant;
        }
        else
        {
            Interpolant interp;
            interp.mTimeScale = time_constant.value();
            interp.mInterpolant = calcInterpolant(time_constant.value());
            sInterpolants.insert(find_it, interp);
            return interp.mInterpolant;
        }
    }
    else
    {
        return calcInterpolant(time_constant.value());

    }
}

//-----------------------------------------------------------------------------
// calcInterpolant()
//-----------------------------------------------------------------------------
F32 LLSmoothInterpolation::calcInterpolant(F32 time_constant)
{
    return llclamp(1.f - powf(2.f, -sTimeDelta / time_constant), 0.f, 1.f);
}

