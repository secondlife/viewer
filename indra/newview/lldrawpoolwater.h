/** 
 * @file lldrawpoolwater.h
 * @brief LLDrawPoolWater class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRAWPOOLWATER_H
#define LL_LLDRAWPOOLWATER_H

#include "lldrawpool.h"


class LLFace;
class LLHeavenBody;
class LLWaterSurface;

class LLDrawPoolWater: public LLDrawPool
{
protected:
	LLPointer<LLViewerImage> mHBTex[2];
	LLPointer<LLViewerImage> mWaterImagep;
	LLPointer<LLViewerImage> mWaterNormp;

	const LLWaterSurface *mWaterSurface;
public:
	enum
	{
		SHADER_LEVEL_RIPPLE = 2,
	};
	
	LLDrawPoolWater();
	/*virtual*/ ~LLDrawPoolWater();

	/*virtual*/ LLDrawPool *instancePool();
	static void restoreGL();

	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void renderFaceSelected(LLFace *facep, LLImageGL *image, const LLColor4 &color,
										const S32 index_offset = 0, const S32 index_count = 0);
	/*virtual*/ void prerender();
	/*virtual*/ void renderForSelect();

	/*virtual*/ LLViewerImage *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	void renderReflection(const LLFace* face);
	void shade();
	void renderShaderSimple();

	virtual S32 getMaterialAttribIndex() { return 0; }
};

void cgErrorCallback();

#endif // LL_LLDRAWPOOLWATER_H
