/** 
 * @file llvosurfacepatch.h
 * @brief Description of LLVOSurfacePatch class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_VOSURFACEPATCH_H
#define LL_VOSURFACEPATCH_H

#include "llviewerobject.h"
#include "llstrider.h"

class LLSurfacePatch;
class LLDrawPool;
class LLVector2;

class LLVOSurfacePatch : public LLStaticViewerObject
{
public:
	enum 
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD2) |
							(1 << LLVertexBuffer::TYPE_COLOR) 
	}
	eVertexDataMask;

	LLVOSurfacePatch(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	/*virtual*/ void markDead();

	// Initialize data that's only inited once per class.
	static void initClass();

	virtual U32 getPartitionType() const;

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);
	/*virtual*/ BOOL		updateLOD();
	/*virtual*/ void		updateFaceSize(S32 idx);
	void getGeometry(LLStrider<LLVector3> &verticesp,
								LLStrider<LLVector3> &normalsp,
								LLStrider<LLColor4U> &colorsp,
								LLStrider<LLVector2> &texCoords0p,
								LLStrider<LLVector2> &texCoords1p,
								LLStrider<U32> &indicesp);

	/*virtual*/ void updateTextures(LLAgent &agent);
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area

	/*virtual*/ void updateSpatialExtents(LLVector3& newMin, LLVector3& newMax);
	/*virtual*/ BOOL isActive() const; // Whether this object needs to do an idleUpdate.

	void setPatch(LLSurfacePatch *patchp);
	LLSurfacePatch	*getPatch() const		{ return mPatchp; }

	void dirtyPatch();
	void dirtyGeom();

	BOOL			mDirtiedPatch;
protected:
	~LLVOSurfacePatch();

	LLFacePool		*mPool;
	LLFacePool		*getPool();
	S32				mBaseComp;
	LLSurfacePatch	*mPatchp;
	BOOL			mDirtyTexture;
	BOOL			mDirtyTerrain;

	S32				mLastNorthStride;
	S32				mLastEastStride;
	S32				mLastStride;
	S32				mLastLength;

	void calcColor(const LLVector3* vertex, const LLVector3* normal, LLColor4U* colorp);
	BOOL updateShadows(BOOL use_shadow_factor = FALSE);
	void getGeomSizesMain(const S32 stride, S32 &num_vertices, S32 &num_indices);
	void getGeomSizesNorth(const S32 stride, const S32 north_stride,
								  S32 &num_vertices, S32 &num_indices);
	void getGeomSizesEast(const S32 stride, const S32 east_stride,
								 S32 &num_vertices, S32 &num_indices);

	void updateMainGeometry(LLFace *facep,
					   LLStrider<LLVector3> &verticesp,
					   LLStrider<LLVector3> &normalsp,
					   LLStrider<LLColor4U> &colorsp,
					   LLStrider<LLVector2> &texCoords0p,
					   LLStrider<LLVector2> &texCoords1p,
					   LLStrider<U32> &indicesp,
					   U32 &index_offset);
	void updateNorthGeometry(LLFace *facep,
					   LLStrider<LLVector3> &verticesp,
					   LLStrider<LLVector3> &normalsp,
					   LLStrider<LLColor4U> &colorsp,
					   LLStrider<LLVector2> &texCoords0p,
					   LLStrider<LLVector2> &texCoords1p,
					   LLStrider<U32> &indicesp,
					   U32 &index_offset);
	void updateEastGeometry(LLFace *facep,
					   LLStrider<LLVector3> &verticesp,
					   LLStrider<LLVector3> &normalsp,
					   LLStrider<LLColor4U> &colorsp,
					   LLStrider<LLVector2> &texCoords0p,
					   LLStrider<LLVector2> &texCoords1p,
					   LLStrider<U32> &indicesp,
					   U32 &index_offset);
};

#endif // LL_VOSURFACEPATCH_H
