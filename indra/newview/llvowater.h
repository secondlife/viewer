/** 
 * @file llvowater.h
 * @brief Description of LLVOWater class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_VOWATER_H
#define LL_VOWATER_H

#include "llviewerobject.h"
#include "llviewerimage.h"
#include "v2math.h"
#include "llfft.h"

#include "llwaterpatch.h"

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
							(1 << LLVertexBuffer::TYPE_TEXCOORD) 
	}
	eVertexDataMask;

	LLVOWater(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual ~LLVOWater() {}

	/*virtual*/ void markDead();

	// Initialize data that's only inited once per class.
	static void initClass();
	static void cleanupClass();

	/*virtual*/ BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);
	/*virtual*/ void		updateSpatialExtents(LLVector3& newMin, LLVector3& newMax);

	/*virtual*/ void updateTextures(LLAgent &agent);
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area

	virtual U32 getPartitionType() const;

	/*virtual*/ BOOL isActive() const; // Whether this object needs to do an idleUpdate.

	void setUseTexture(const BOOL use_texture);

protected:
	BOOL mUseTexture;
};

#endif // LL_VOSURFACEPATCH_H
