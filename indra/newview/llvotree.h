/** 
 * @file llvotree.h
 * @brief LLVOTree class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLVOTREE_H
#define LL_LLVOTREE_H

#include "llviewerobject.h"
#include "lldarray.h"
#include "xform.h"

class LLFace;
class LLDrawPool;
class LLViewerFetchedTexture;

class LLVOTree : public LLViewerObject
{
protected:
	~LLVOTree();

public:
	enum
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD0)
	};

	LLVOTree(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	// Initialize data that's only inited once per class.
	static void initClass();
	static void cleanupClass();
	static bool isTreeRenderingStopped();

	/*virtual*/ U32 processUpdateMessage(LLMessageSystem *mesgsys,
											void **user_data,
											U32 block_num, const EObjectUpdateType update_type,
											LLDataPacker *dp);
	/*virtual*/ void idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	
	// Graphical stuff for objects - maybe broken out into render class later?
	/*virtual*/ void render(LLAgent &agent);
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent);
	/*virtual*/ void updateTextures();

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);
	/*virtual*/ void		updateSpatialExtents(LLVector4a &min, LLVector4a &max);

	virtual U32 getPartitionType() const;

	void updateRadius();

	void calcNumVerts(U32& vert_count, U32& index_count, S32 trunk_LOD, S32 stop_level, U16 depth, U16 trunk_depth, F32 branches);

	void updateMesh();

	void appendMesh(LLStrider<LLVector3>& vertices, 
						 LLStrider<LLVector3>& normals, 
						 LLStrider<LLVector2>& tex_coords, 
						 LLStrider<U16>& indices,
						 U16& idx_offset,
						 LLMatrix4& matrix,
						 LLMatrix4& norm_mat,
						 S32 vertex_offset,
						 S32 vertex_count,
						 S32 index_count,
						 S32 index_offset);

	void genBranchPipeline(LLStrider<LLVector3>& vertices, 
								 LLStrider<LLVector3>& normals, 
								 LLStrider<LLVector2>& tex_coords, 
								 LLStrider<U16>& indices,
								 U16& index_offset,
								 LLMatrix4& matrix, 
								 S32 trunk_LOD, 
								 S32 stop_level, 
								 U16 depth, 
								 U16 trunk_depth,  
								 F32 scale, 
								 F32 twist, 
								 F32 droop,  
								 F32 branches, 
								 F32 alpha);

	U32 drawBranchPipeline(LLMatrix4& matrix, U16* indicesp, S32 trunk_LOD, S32 stop_level, U16 depth, U16 trunk_depth,  F32 scale, F32 twist, F32 droop,  F32 branches, F32 alpha);
 

	 /*virtual*/ BOOL lineSegmentIntersect(const LLVector3& start, const LLVector3& end, 
										  S32 face = -1,                        // which face to check, -1 = ALL_SIDES
										  BOOL pick_transparent = FALSE,
										  S32* face_hit = NULL,                 // which face was hit
										  LLVector3* intersection = NULL,       // return the intersection point
										  LLVector2* tex_coord = NULL,          // return the texture coordinates of the intersection point
										  LLVector3* normal = NULL,             // return the surface normal at the intersection point
										  LLVector3* bi_normal = NULL           // return the surface bi-normal at the intersection point
		);

	static S32 sMaxTreeSpecies;

	struct TreeSpeciesData
	{
		LLUUID mTextureID;
		
		F32				mBranchLength;	// Scale (length) of tree branches
		F32				mDroop;			// Droop from vertical (degrees) at each branch recursion
		F32				mTwist;			// Twist 
		F32				mBranches;		// Number of branches emitted at each recursion level 
		U8				mDepth;			// Number of recursions to tips of branches
		F32				mScaleStep;		// Multiplier for scale at each recursion level
		U8				mTrunkDepth;

		F32				mLeafScale;		// Scales leaf texture when rendering 
		F32				mTrunkLength;	// Scales branch diameters when rendering 
		F32				mBillboardScale; // Scales the billboard representation 
		F32				mBillboardRatio; // Height to width aspect ratio
		F32				mTrunkAspect;	
		F32				mBranchAspect;
		F32				mRandomLeafRotate;
		F32				mNoiseScale;	//  Scaling of noise function in perlin space (norm = 1.0)
		F32				mNoiseMag;		//  amount of perlin noise to deform by (0 = none)
		F32				mTaper;			//  amount of perlin noise to deform by (0 = none)
		F32				mRepeatTrunkZ;	//  Times to repeat the trunk texture vertically along trunk 
	};

	static F32 sTreeFactor;			// Tree level of detail factor
	static const S32 sMAX_NUM_TREE_LOD_LEVELS ;

	friend class LLDrawPoolTree;
protected:
	LLVector3		mTrunkBend;		// Accumulated wind (used for blowing trees)
	LLVector3		mWind;

	LLPointer<LLVertexBuffer> mReferenceBuffer; //reference geometry for generating tree mesh
	LLPointer<LLViewerFetchedTexture> mTreeImagep;	// Pointer to proper tree image

	U8				mSpecies;		// Species of tree
	F32				mBranchLength;	// Scale (length) of tree branches
	F32				mTrunkLength;	// Trunk length (first recursion)
	F32				mDroop;			// Droop from vertical (degrees) at each branch recursion
	F32				mTwist;			// Twist 
	F32				mBranches;		// Number of branches emitted at each recursion level 
	U8				mDepth;			// Number of recursions to tips of branches
	F32				mScaleStep;		// Multiplier for scale at each recursion level
	U8				mTrunkDepth;
	U32				mTrunkLOD;
	F32				mLeafScale;		// Scales leaf texture when rendering 

	F32				mBillboardScale;	//  How big to draw the billboard?
	F32				mBillboardRatio;	//  Height to width ratio of billboard
	F32				mTrunkAspect;		//  Ratio between width/length of trunk
	F32				mBranchAspect;	//  Ratio between width/length of branch
	F32				mRandomLeafRotate;	//	How much to randomly rotate leaves about arbitrary axis 

	// cache last position+rotation so we can detect the need for a
	// complete rebuild when not animating
	LLVector3 mLastPosition;
	LLQuaternion mLastRotation;

	U32 mFrameCount;

	typedef std::map<U32, TreeSpeciesData*> SpeciesMap;
	static SpeciesMap sSpeciesTable;

	static S32 sLODIndexOffset[4];
	static S32 sLODIndexCount[4];
	static S32 sLODVertexOffset[4];
	static S32 sLODVertexCount[4];
	static S32 sLODSlices[4];
	static F32 sLODAngles[4];
};

#endif
