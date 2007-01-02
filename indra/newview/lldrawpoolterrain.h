/** 
 * @file lldrawpoolterrain.h
 * @brief LLDrawPoolTerrain class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRAWPOOLTERRAIN_H
#define LL_LLDRAWPOOLTERRAIN_H

#include "lldrawpool.h"

class LLDrawPoolTerrain : public LLDrawPool
{
	LLPointer<LLViewerImage> mTexturep;
public:
	LLDrawPoolTerrain(LLViewerImage *texturep);
	virtual ~LLDrawPoolTerrain();

	/*virtual*/ LLDrawPool *instancePool();


	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void prerender();
	/*virtual*/ void renderForSelect();
	/*virtual*/ void dirtyTexture(const LLViewerImage *texturep);
	/*virtual*/ LLViewerImage *getTexture();
	/*virtual*/ LLViewerImage *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	LLPointer<LLViewerImage> mAlphaRampImagep;
	LLPointer<LLViewerImage> m2DAlphaRampImagep;
	LLPointer<LLViewerImage> mAlphaNoiseImagep;

	virtual S32 getMaterialAttribIndex();

	static S32 sDetailMode;
	static F32 sDetailScale; // meters per texture
protected:
	void renderSimple();
	void renderOwnership();

	void renderFull2TU();
	void renderFull4TU();
	void renderFull4TUShader();
};

#endif // LL_LLDRAWPOOLSIMPLE_H
