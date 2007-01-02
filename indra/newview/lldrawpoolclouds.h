/** 
 * @file lldrawpoolclouds.h
 * @brief LLDrawPoolClouds class definition
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRAWPOOLCLOUDS_H
#define LL_LLDRAWPOOLCLOUDS_H

#include "lldrawpool.h"

class LLDrawPoolClouds : public LLDrawPool
{
public:
	LLDrawPoolClouds();

	/*virtual*/ void prerender();
	/*virtual*/ LLDrawPool *instancePool();
	/*virtual*/ void enqueue(LLFace *face);
	/*virtual*/ void beginRenderPass(S32 pass);
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void renderForSelect();
	virtual S32 getMaterialAttribIndex() { return 0; }
};

#endif // LL_LLDRAWPOOLSKY_H
