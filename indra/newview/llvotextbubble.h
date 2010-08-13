/** 
 * @file llvotextbubble.h
 * @brief Description of LLVORock class, which a derivation of LLViewerObject
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
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

	/*virtual*/ void updateTextures();
	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);
	/*virtual*/ BOOL		updateLOD();
	/*virtual*/ void		updateFaceSize(S32 idx);
	
	/*virtual*/ void		getGeometry(S32 idx,
								LLStrider<LLVector3>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<U16>& indicesp);

	virtual U32 getPartitionType() const;

	LLColor4 mColor;
	S32 mLOD;
	BOOL mVolumeChanged;

protected:
	~LLVOTextBubble();
	BOOL setVolume(const LLVolumeParams &volume_params, const S32 detail, bool unique_volume = false);
	LLFrameTimer mUpdateTimer;
};

#endif // LL_VO_TEXT_BUBBLE
