/** 
 * @file llvotree.h
 * @brief LLVOTree class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLVOTREE_H
#define LL_LLVOTREE_H

#include "llviewerobject.h"
#include "lldarray.h"
#include "xform.h"

class LLFace;
class LLDrawPool;


class LLVOTree : public LLViewerObject
{
protected:
	~LLVOTree();

public:
	enum
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD)
	};

	LLVOTree(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	// Initialize data that's only inited once per class.
	static void initClass();
	static void cleanupClass();

	/*virtual*/ U32 processUpdateMessage(LLMessageSystem *mesgsys,
											void **user_data,
											U32 block_num, const EObjectUpdateType update_type,
											LLDataPacker *dp);
	/*virtual*/ BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	
	// Graphical stuff for objects - maybe broken out into render class later?
	/*virtual*/ void render(LLAgent &agent);
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent);
	/*virtual*/ void updateTextures(LLAgent &agent);

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);
	/*virtual*/ void		updateSpatialExtents(LLVector3 &min, LLVector3 &max);

	virtual U32 getPartitionType() const;

	void updateRadius();

	U32 drawBranchPipeline(U32* indicesp, S32 trunk_LOD, S32 stop_level, U16 depth, U16 trunk_depth,  F32 scale, F32 twist, F32 droop,  F32 branches, F32 alpha);
 
	void drawBranch(S32 stop_level, U16 depth, U16 trunk_depth, F32 scale, F32 twist, F32 droop,  F32 branches, F32 alpha, BOOL draw_leaves);

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

	friend class LLDrawPoolTree;
protected:
	LLVector3		mTrunkBend;		// Accumulated wind (used for blowing trees)
	LLVector3		mTrunkVel;		// 
	LLVector3		mWind;

	LLPointer<LLViewerImage> mTreeImagep;	// Pointer to proper tree image

	U8				mSpecies;		// Species of tree
	F32				mBranchLength;	// Scale (length) of tree branches
	F32				mTrunkLength;	// Trunk length (first recursion)
	F32				mDroop;			// Droop from vertical (degrees) at each branch recursion
	F32				mTwist;			// Twist 
	F32				mBranches;		// Number of branches emitted at each recursion level 
	U8				mDepth;			// Number of recursions to tips of branches
	F32				mScaleStep;		// Multiplier for scale at each recursion level
	U8				mTrunkDepth;

	F32				mLeafScale;		// Scales leaf texture when rendering 

	F32				mBillboardScale;	//  How big to draw the billboard?
	F32				mBillboardRatio;	//  Height to width ratio of billboard
	F32				mTrunkAspect;		//  Ratio between width/length of trunk
	F32				mBranchAspect;	//  Ratio between width/length of branch
	F32				mRandomLeafRotate;	//	How much to randomly rotate leaves about arbitrary axis 
	
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
