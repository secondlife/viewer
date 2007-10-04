/** 
 * @file llwind.h
 * @brief LLWind class header file
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

#ifndef LL_LLWIND_H
#define LL_LLWIND_H

//#include "vmath.h"
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
	LLVector3 getCloudVelocity(const LLVector3 &location); // "location" is region-local
	LLVector3 getVelocityNoisy(const LLVector3 &location, const F32 dim);	// "location" is region-local

	void decompress(LLBitPack &bitpack, LLGroupHeader *group_headerp);
	LLVector3 getAverage();
	void setCloudDensityPointer(F32 *densityp);

	void setOriginGlobal(const LLVector3d &origin_global);
private:
	S32 mSize;
	F32 * mVelX;
	F32 * mVelY;
	F32 * mCloudVelX;
	F32 * mCloudVelY;
	F32 * mCloudDensityp;

	LLVector3d mOriginGlobal;
	void init();

};

#endif
