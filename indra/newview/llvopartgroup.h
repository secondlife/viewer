/** 
 * @file llvopartgroup.h
 * @brief Group of particle systems
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOPARTGROUP_H
#define LL_LLVOPARTGROUP_H

#include "llviewerobject.h"
#include "v3math.h"
#include "v3color.h"
#include "llframetimer.h"

class LLViewerPartGroup;

class LLVOPartGroup : public LLAlphaObject
{
public:
	enum 
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD) |
							(1 << LLVertexBuffer::TYPE_COLOR)
	}
	eVertexDataMask;

	LLVOPartGroup(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	/*virtual*/ BOOL    isActive() const; // Whether this object needs to do an idleUpdate.
	BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	BOOL isParticle();

	virtual F32 getBinRadius();
	virtual void updateSpatialExtents(LLVector3& newMin, LLVector3& newMax);
	virtual U32 getPartitionType() const;
	
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent);
	/*virtual*/ void updateTextures(LLAgent &agent);

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);
				void		getGeometry(S32 idx,
								LLStrider<LLVector3>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<U32>& indicesp);

	void updateFaceSize(S32 idx) { }
	F32 getPartSize(S32 idx);
	void setViewerPartGroup(LLViewerPartGroup *part_groupp)		{ mViewerPartGroupp = part_groupp; }
	LLViewerPartGroup* getViewerPartGroup()	{ return mViewerPartGroupp; }

protected:
	~LLVOPartGroup();

	LLViewerPartGroup *mViewerPartGroupp;
	LLVector3 mExtents[2];
	LLColor4 mDebugColor;
};

#endif // LL_LLVOPARTGROUP_H
