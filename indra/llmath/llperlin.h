/** 
 * @file llperlin.h
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
