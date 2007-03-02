/** 
 * @file lldrawpoolalpha.h
 * @brief LLDrawPoolAlpha class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRAWPOOLALPHA_H
#define LL_LLDRAWPOOLALPHA_H

#include "lldrawpool.h"
#include "llframetimer.h"

class LLFace;
class LLColor4;

class LLDrawPoolAlpha: public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_COLOR |
							LLVertexBuffer::MAP_TEXCOORD
	};
	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	LLDrawPoolAlpha(U32 type = LLDrawPool::POOL_ALPHA);
	/*virtual*/ ~LLDrawPoolAlpha();

	/*virtual*/ void beginRenderPass(S32 pass = 0);
	virtual void render(S32 pass = 0);
	void render(std::vector<LLSpatialGroup*>& groups);
	/*virtual*/ void prerender();

	void renderGroupAlpha(LLSpatialGroup* group, U32 type, U32 mask, BOOL texture = TRUE);
	void renderAlpha(U32 mask, std::vector<LLSpatialGroup*>& groups);
	void renderAlphaHighlight(U32 mask, std::vector<LLSpatialGroup*>& groups);
	
	static BOOL sShowDebugAlpha;
};

class LLDrawPoolAlphaPostWater : public LLDrawPoolAlpha
{
public:
	LLDrawPoolAlphaPostWater();
	virtual void render(S32 pass = 0);
};

#endif // LL_LLDRAWPOOLALPHA_H
