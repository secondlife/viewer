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
#include "llviewerimage.h"
#include "llframetimer.h"

class LLFace;
class LLColor4;

class LLDrawPoolAlpha: public LLDrawPool
{
public:
	LLDrawPoolAlpha();
	/*virtual*/ ~LLDrawPoolAlpha();

	/*virtual*/ LLDrawPool *instancePool();

	/*virtual*/ void beginRenderPass(S32 pass = 0);
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void renderFaceSelected(LLFace *facep, LLImageGL *image, const LLColor4 &color,
										const S32 index_offset = 0, const S32 index_count = 0);
	/*virtual*/ void prerender();
	/*virtual*/ void renderForSelect();

	/*virtual*/ void enqueue(LLFace *face);
	/*virtual*/ BOOL removeFace(LLFace *face);
	/*virtual*/ void resetDrawOrders();

	/*virtual*/ void enableShade();
	/*virtual*/ void disableShade();
	/*virtual*/ void setShade(F32 shade);


	virtual S32 getMaterialAttribIndex();

	BOOL mRebuiltLastFrame;
	enum
	{
		NUM_ALPHA_BINS = 1024
	};

	/*virtual*/ BOOL verify() const;
	/*virtual*/ LLViewerImage *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	static BOOL sShowDebugAlpha;
protected:
	F32 mMinDistance;
	F32 mMaxDistance;
	F32 mInvBinSize;

	LLDynamicArray<LLFace*> mDistanceBins[NUM_ALPHA_BINS];
};

#endif // LL_LLDRAWPOOLALPHA_H
