/** 
 * @file llrand.cpp
 * @brief Global random generator.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "llrand.h"
#include "lluuid.h"

/**
 * Through analysis, we have decided that we want to take values which
 * are close enough to 1.0 to map back to 0.0.  We came to this
 * conclusion from noting that:
 *
 * [0.0, 1.0)
 *
 * when scaled to the integer set:
 *
 * [0, 4)
 *
 * there is some value close enough to 1.0 that when multiplying by 4,
 * gets truncated to 4. Therefore:
 *
 * [0,1-eps] => 0
 * [1,2-eps] => 1
 * [2,3-eps] => 2
 * [3,4-eps] => 3
 *
 * So 0 gets uneven distribution if we simply clamp. The actual
 * clamp utilized in this file is to map values out of range back
 * to 0 to restore uniform distribution.
 *
 * Also, for clamping floats when asking for a distribution from
 * [0.0,g) we have determined that for values of g < 0.5, then
 * rand*g=g, which is not the desired result. As above, we clamp to 0
 * to restore uniform distribution.
 */

// *NOTE: The system rand implementation is probably not correct.
#define LL_USE_SYSTEM_RAND 0

#if LL_USE_SYSTEM_RAND
#include <cstdlib>
#endif

#if LL_USE_SYSTEM_RAND
class LLSeedRand
{
public:
    LLSeedRand()
    {
#if LL_WINDOWS
        srand(LLUUID::getRandomSeed());
#else
        srand48(LLUUID::getRandomSeed());
#endif
    }
};
static LLSeedRand sRandomSeeder;
inline F64 ll_internal_random_double()
{
#if LL_WINDOWS
    return (F64)rand() / (F64)RAND_MAX; 
#else
    return drand48();
#endif
}
inline F32 ll_internal_random_float()
{
#if LL_WINDOWS
    return (F32)rand() / (F32)RAND_MAX; 
#else
    return (F32)drand48();
#endif
}
#else
static LLRandLagFib2281 gRandomGenerator(LLUUID::getRandomSeed());
inline F64 ll_internal_random_double()
{
    // *HACK: Through experimentation, we have found that dual core
    // CPUs (or at least multi-threaded processes) seem to
    // occasionally give an obviously incorrect random number -- like
    // 5^15 or something. Sooooo, clamp it as described above.
    F64 rv = gRandomGenerator();
    if(!((rv >= 0.0) && (rv < 1.0))) return fmod(rv, 1.0);
    return rv;
}

inline F32 ll_internal_random_float()
{
    // The clamping rules are described above.
    F32 rv = (F32)gRandomGenerator();
    if(!((rv >= 0.0f) && (rv < 1.0f))) return fmod(rv, 1.f);
    return rv;
}
#endif

S32 ll_rand()
{
    return ll_rand(RAND_MAX);
}

S32 ll_rand(S32 val)
{
    // The clamping rules are described above.
    S32 rv = (S32)(ll_internal_random_double() * val);
    if(rv == val) return 0;
    return rv;
}

F32 ll_frand()
{
    return ll_internal_random_float();
}

F32 ll_frand(F32 val)
{
    // The clamping rules are described above.
    F32 rv = ll_internal_random_float() * val;
    if(val > 0)
    {
        if(rv >= val) return 0.0f;
    }
    else
    {
        if(rv <= val) return 0.0f;
    }
    return rv;
}

F64 ll_drand()
{
    return ll_internal_random_double();
}

F64 ll_drand(F64 val)
{
    // The clamping rules are described above.
    F64 rv = ll_internal_random_double() * val;
    if(val > 0)
    {
        if(rv >= val) return 0.0;
    }
    else
    {
        if(rv <= val) return 0.0;
    }
    return rv;
}
