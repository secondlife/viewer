/** 
 * @file llvotextbubble.h
 * @brief Description of LLVORock class, which a derivation of LLViewerObject
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOTEXTBUBBLE_H
#define LL_LLVOTEXTBUBBLE_H

#include "llviewerobject.h"
#include "llframetimer.h"

class LLVOTextBubble : public LLAlphaObject
{
public:
	LLVOTextBubble(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	/*virtual*/ BOOL    isActive() const; // Whether this object needs to do an idleUpdate.
	/*virtual*/ BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);

	/*virtual*/ void updateTextures(LLAgent &agent);
	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);
	/*virtual*/ BOOL		updateLOD();
	/*virtual*/ void		updateFaceSize(S32 idx);
	
	/*virtual*/ void		getGeometry(S32 idx,
								LLStrider<LLVector3>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<U32>& indicesp);

	virtual U32 getPartitionType() const;

	LLColor4 mColor;
	S32 mLOD;
	BOOL mVolumeChanged;

protected:
	~LLVOTextBubble();
	BOOL setVolume(const LLVolumeParams &volume_params);
	LLFrameTimer mUpdateTimer;
};

#endif // LL_VO_TEXT_BUBBLE
