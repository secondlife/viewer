/** 
 * @file llviewertextureanim.h
 * @brief LLViewerTextureAnim class header file
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERTEXTUREANIM_H
#define LL_LLVIEWERTEXTUREANIM_H

#include "lltextureanim.h"
#include "llframetimer.h"

class LLViewerTextureAnim : public LLTextureAnim
{
public:
	LLViewerTextureAnim();
	virtual ~LLViewerTextureAnim();

	/*virtual*/ void reset();

	S32 animateTextures(F32 &off_s, F32 &off_t, F32 &scale_s, F32 &scale_t, F32 &rotate);
	enum
	{
		TRANSLATE = 0x01 // Result code JUST for animateTextures
	};

	F32 mOffS;
	F32 mOffT;
	F32 mScaleS;
	F32 mScaleT;
	F32 mRot;

protected:
	LLFrameTimer mTimer;
	F64 mLastTime;
	F32 mLastFrame;
};
#endif
