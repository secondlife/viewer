/** 
 * @file llrendersphere.h
 * @brief interface for the LLRenderSphere class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLRENDERSPHERE_H
#define LL_LLRENDERSPHERE_H

#include "llmath.h"
#include "v3math.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"
#include "v4color.h"
#include "llgl.h"

void lat2xyz(LLVector3 * result, F32 lat, F32 lon);			// utility routine

class LLRenderSphere  
{
public:
	LLGLuint	mDList[5];

	void prerender();
	void cleanupGL();
	void render(F32 pixel_area);		// of a box of size 1.0 at that position
	void render();						// render at highest LOD
};

extern LLRenderSphere gSphere;
#endif
