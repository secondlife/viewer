/** 
 * @file llvowater.h
 * @brief Description of LLVOWater class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_VOWATER_H
#define LL_VOWATER_H

#include "llviewerobject.h"
#include "llviewertexture.h"
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

	/*virtual*/ BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);
	/*virtual*/ void		updateSpatialExtents(LLVector3& newMin, LLVector3& newMax);

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
};

#endif // LL_VOSURFACEPATCH_H
