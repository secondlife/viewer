/** 
 * @file llvosurfacepatch.h
 * @brief Description of LLVOSurfacePatch class
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
	static F32 sLODFactor;

	enum
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD2) |
							(1 << LLVertexBuffer::TYPE_COLOR) 
	};

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
								LLStrider<U16> &indicesp);

	/*virtual*/ void updateTextures(LLAgent &agent);
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area

	/*virtual*/ void updateSpatialExtents(LLVector3& newMin, LLVector3& newMax);
	/*virtual*/ BOOL isActive() const; // Whether this object needs to do an idleUpdate.

	void setPatch(LLSurfacePatch *patchp);
	LLSurfacePatch	*getPatch() const		{ return mPatchp; }

	void dirtyPatch();
	void dirtyGeom();

	/*virtual*/ BOOL lineSegmentIntersect(const LLVector3& start, const LLVector3& end, 
										  S32 face = -1,                        // which face to check, -1 = ALL_SIDES
										  BOOL pick_transparent = FALSE,
										  S32* face_hit = NULL,                 // which face was hit
										  LLVector3* intersection = NULL,       // return the intersection point
										  LLVector2* tex_coord = NULL,          // return the texture coordinates of the intersection point
										  LLVector3* normal = NULL,             // return the surface normal at the intersection point
										  LLVector3* bi_normal = NULL           // return the surface bi-normal at the intersection point
		);

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
					   LLStrider<U16> &indicesp,
					   U32 &index_offset);
	void updateNorthGeometry(LLFace *facep,
					   LLStrider<LLVector3> &verticesp,
					   LLStrider<LLVector3> &normalsp,
					   LLStrider<LLColor4U> &colorsp,
					   LLStrider<LLVector2> &texCoords0p,
					   LLStrider<LLVector2> &texCoords1p,
					   LLStrider<U16> &indicesp,
					   U32 &index_offset);
	void updateEastGeometry(LLFace *facep,
					   LLStrider<LLVector3> &verticesp,
					   LLStrider<LLVector3> &normalsp,
					   LLStrider<LLColor4U> &colorsp,
					   LLStrider<LLVector2> &texCoords0p,
					   LLStrider<LLVector2> &texCoords1p,
					   LLStrider<U16> &indicesp,
					   U32 &index_offset);
};

#endif // LL_VOSURFACEPATCH_H
