/** 
 * @file llvowater.h
 * @brief Description of LLVOWater class
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

#ifndef LL_VOWATER_H
#define LL_VOWATER_H

#include "llviewerobject.h"
#include "llviewertexture.h"
#include "pipeline.h"
#include "v2math.h"

const U32 N_RES	= 16; //32			// number of subdivisions of wave tile
const U8  WAVE_STEP		= 8;

class LLSurface;
class LLHeavenBody;
class LLVOSky;
class LLFace;

class LLVOWater : public LLStaticViewerObject
{
public:
	enum 
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD0) 
	};

	LLVOWater(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	/*virtual*/ void markDead();

	// Initialize data that's only inited once per class.
	static void initClass();
	static void cleanupClass();

	/*virtual*/ void idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);
	/*virtual*/ void		updateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax);

	/*virtual*/ void updateTextures();
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area

	virtual U32 getPartitionType() const;

	/*virtual*/ BOOL isActive() const; // Whether this object needs to do an idleUpdate.

	void setUseTexture(const BOOL use_texture);
	void setIsEdgePatch(const BOOL edge_patch);
	BOOL getUseTexture() const { return mUseTexture; }
	BOOL getIsEdgePatch() const { return mIsEdgePatch; }

protected:
	BOOL mUseTexture;
	BOOL mIsEdgePatch;
	S32  mRenderType; 
};

class LLVOVoidWater : public LLVOWater
{
public:
	LLVOVoidWater(LLUUID const& id, LLPCode pcode, LLViewerRegion* regionp) : LLVOWater(id, pcode, regionp)
	{
		mRenderType = LLPipeline::RENDER_TYPE_VOIDWATER;
	}

	/*virtual*/ U32 getPartitionType() const;
};


#endif // LL_VOSURFACEPATCH_H
