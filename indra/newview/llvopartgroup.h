/** 
 * @file llvopartgroup.h
 * @brief Group of particle systems
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

#ifndef LL_LLVOPARTGROUP_H
#define LL_LLVOPARTGROUP_H

#include "llviewerobject.h"
#include "v3math.h"
#include "v3color.h"
#include "llframetimer.h"
#include "llviewerpartsim.h"
#include "llvertexbuffer.h"

class LLViewerPartGroup;

class LLVOPartGroup : public LLAlphaObject
{
public:

	static void initClass();
	static void restoreGL();
	static void destroyGL();

	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR |
							LLVertexBuffer::MAP_EMISSIVE |
							LLVertexBuffer::MAP_TEXTURE_INDEX
	};

	LLVOPartGroup(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	/*virtual*/ BOOL    isActive() const; // Whether this object needs to do an idleUpdate.
	void idleUpdate(LLAgent &agent, const F64 &time);

	virtual F32 getBinRadius();
	virtual void updateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax);
	virtual U32 getPartitionType() const;
	
	/*virtual*/ BOOL lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
										  S32 face,
										  BOOL pick_transparent,
										  BOOL pick_rigged,
                                          BOOL pick_unselectable,
										  S32* face_hit,
										  LLVector4a* intersection,
										  LLVector2* tex_coord,
										  LLVector4a* normal,
										  LLVector4a* tangent);

	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent);
	/*virtual*/ void updateTextures();

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);
	void		getGeometry(const LLViewerPart& part,							
								LLStrider<LLVector4a>& verticesp);
				
				void		getGeometry(S32 idx,
								LLStrider<LLVector4a>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<LLColor4U>& emissivep,
								LLStrider<U16>& indicesp);

	void updateFaceSize(S32 idx) { }
	F32 getPartSize(S32 idx);
	void getBlendFunc(S32 idx, LLRender::eBlendFactor& src, LLRender::eBlendFactor& dst);
	LLUUID getPartOwner(S32 idx);
	LLUUID getPartSource(S32 idx);

	void setViewerPartGroup(LLViewerPartGroup *part_groupp)		{ mViewerPartGroupp = part_groupp; }
	LLViewerPartGroup* getViewerPartGroup()	{ return mViewerPartGroupp; }

protected:
	~LLVOPartGroup();

	LLViewerPartGroup *mViewerPartGroupp;

	virtual LLVector3 getCameraPosition() const;

};


class LLVOHUDPartGroup : public LLVOPartGroup
{
public:
	LLVOHUDPartGroup(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp) : 
	  LLVOPartGroup(id, pcode, regionp)   
	{
	}
protected:
	LLDrawable* createDrawable(LLPipeline *pipeline);
	U32 getPartitionType() const;
	virtual LLVector3 getCameraPosition() const;
};

#endif // LL_LLVOPARTGROUP_H
