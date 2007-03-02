/** 
 * @file lldrawpooltree.h
 * @brief LLDrawPoolTree class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRAWPOOLTREE_H
#define LL_LLDRAWPOOLTREE_H

#include "lldrawpool.h"

class LLDrawPoolTree : public LLFacePool
{
	LLPointer<LLViewerImage> mTexturep;
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD							
	};

	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	LLDrawPoolTree(LLViewerImage *texturep);

	/*virtual*/ LLDrawPool *instancePool();

	/*virtual*/ void prerender();
	/*virtual*/ void beginRenderPass( S32 pass );
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void endRenderPass( S32 pass );
	/*virtual*/ void renderForSelect();
	/*virtual*/ BOOL verify() const;
	/*virtual*/ LLViewerImage *getTexture();
	/*virtual*/ LLViewerImage *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	virtual S32 getMaterialAttribIndex();

	static S32 sDiffTex;

private:
	void renderTree(BOOL selecting = FALSE);
};

#endif // LL_LLDRAWPOOLTREE_H
