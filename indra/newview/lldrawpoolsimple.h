/** 
 * @file lldrawpoolsimple.h
 * @brief LLDrawPoolSimple class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRAWPOOLSIMPLE_H
#define LL_LLDRAWPOOLSIMPLE_H

#include "lldrawpool.h"

class LLFRInfo
{
public:
	U32 mPrimType;
	U32 mGeomIndex;
	U32 mGeomIndexEnd;
	U32 mNumIndices;
	U32 mIndicesStart;

	LLFRInfo()
	{
	}

	LLFRInfo(const U32 pt, const U32 gi, const U32 gc, const U32 ni, const U32 is) :
		mPrimType(pt),
		mGeomIndex(gi),
		mGeomIndexEnd(gi+gc),
		mNumIndices(ni),
		mIndicesStart(is)
	{
	}
};

class LLDrawPoolSimple : public LLDrawPool
{
	LLPointer<LLViewerImage> mTexturep;
public:
	enum
	{
		SHADER_LEVEL_LOCAL_LIGHTS = 2
	};
	
	LLDrawPoolSimple(LLViewerImage *texturep);

	/*virtual*/ LLDrawPool *instancePool();

	/*virtual*/ void beginRenderPass(S32 pass);
	/*virtual*/ void endRenderPass(S32 pass);
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void renderFaceSelected(LLFace *facep, 
									LLImageGL *image, 
									const LLColor4 &color,
									const S32 index_offset = 0, const S32 index_count = 0);
	/*virtual*/ void prerender();
	/*virtual*/ void renderForSelect();
	/*virtual*/ void dirtyTexture(const LLViewerImage *texturep);
	/*virtual*/ LLViewerImage *getTexture();
	/*virtual*/ LLViewerImage *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display
	/*virtual*/ BOOL match(LLFace* last_face, LLFace* facep);

	/*virtual*/ void enableShade();
	/*virtual*/ void disableShade();
	/*virtual*/ void setShade(F32 shade);

	virtual S32 getMaterialAttribIndex();

	static S32 sDiffTex;
};

#endif // LL_LLDRAWPOOLSIMPLE_H
