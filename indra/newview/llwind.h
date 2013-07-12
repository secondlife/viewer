/** 
 * @file llwind.h
 * @brief LLWind class header file
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

#ifndef LL_LLWIND_H
#define LL_LLWIND_H

#include "llmath.h"
#include "v3math.h"
#include "v3dmath.h"

class LLVector3;
class LLBitPack;
class LLGroupHeader;


class LLWind  
{
public:
	LLWind();
	~LLWind();
	void renderVectors();
	LLVector3 getVelocity(const LLVector3 &location); // "location" is region-local
	LLVector3 getVelocityNoisy(const LLVector3 &location, const F32 dim);	// "location" is region-local

	void decompress(LLBitPack &bitpack, LLGroupHeader *group_headerp);
	LLVector3 getAverage();

	void setOriginGlobal(const LLVector3d &origin_global);
private:
	S32 mSize;
	F32 * mVelX;
	F32 * mVelY;

	LLVector3d mOriginGlobal;
	void init();

	LOG_CLASS(LLWind);
};

#endif
