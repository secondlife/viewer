/** 
 * @file llwind.h
 * @brief LLWind class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
