/** 
 * @file llviewerlayer.h
 * @brief LLViewerLayer class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERLAYER_H
#define LL_LLVIEWERLAYER_H

// Viewer-side representation of a layer...

class LLViewerLayer
{
public:
	LLViewerLayer(const S32 width, const F32 scale = 1.f);
	virtual ~LLViewerLayer();

	F32 getValueScaled(const F32 x, const F32 y) const;
protected:
	F32 getValue(const S32 x, const S32 y) const;
protected:
	S32 mWidth;
	F32 mScale;
	F32 mScaleInv;
	F32 *mDatap;
};

#endif // LL_LLVIEWERLAYER_H
