/** 
 * @file llperlin.h
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
