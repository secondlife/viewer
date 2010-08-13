/** 
 * @file llvoclouds.h
 * @brief Description of LLVOClouds class
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

#ifndef LL_LLVOCLOUDS_H
#define LL_LLVOCLOUDS_H

#include "llviewerobject.h"
#include "lltable.h"
#include "v4coloru.h"

class LLViewerTexture;
class LLViewerCloudGroup;

class LLCloudGroup;


class LLVOClouds : public LLAlphaObject
{
public:
	LLVOClouds(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp );

	// Initialize data that's only inited once per class.
	static void initClass();

	void updateDrawable(BOOL force_damped); 

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);
	/*virtual*/ void		getGeometry(S32 te, 
							LLStrider<LLVector3>& verticesp, 
							LLStrider<LLVector3>& normalsp, 
							LLStrider<LLVector2>& texcoordsp, 
							LLStrider<LLColor4U>& colorsp, 
							LLStrider<U16>& indicesp);

	/*virtual*/ BOOL    isActive() const; // Whether this object needs to do an idleUpdate.
	F32 getPartSize(S32 idx);

	/*virtual*/ void updateTextures();
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area
	
	void updateFaceSize(S32 idx) { }
	BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);

	virtual U32 getPartitionType() const;

	void setCloudGroup(LLCloudGroup *cgp)		{ mCloudGroupp = cgp; }
protected:
	virtual ~LLVOClouds();

	LLCloudGroup *mCloudGroupp;
};

extern LLUUID gCloudTextureID;

#endif // LL_VO_CLOUDS_H
