/** 
 * @file llperlin.h
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_PERLIN_H
#define LL_PERLIN_H

#include "stdtypes.h"

// namespace wrapper
class LLPerlinNoise
{
public:
    static F32 noise1(F32 x);
    static F32 noise2(F32 x, F32 y);
    static F32 noise3(F32 x, F32 y, F32 z);
    static F32 turbulence2(F32 x, F32 y, F32 freq);
    static F32 turbulence3(F32 x, F32 y, F32 z, F32 freq);
    static F32 clouds3(F32 x, F32 y, F32 z, F32 freq);
private:
    static bool sInitialized;
    static void init(void);
};

#endif // LL_PERLIN_
