/** 
 * @file llvotreenew.h
 * @brief LLVOTreeNew class header file
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LL_LLVOTREENEW_H
#define LL_LLVOTREENEW_H

#include "llviewerobject.h"
#include "lldarray.h"
#include "xform.h"

#include "lltreeparams.h"
#include "llstrider.h"
#include "v2math.h"
#include "v3math.h"
#include "llviewertexture.h"

class LLFace;
class LLDrawPool;

// number of static arrays created
const U8 MAX_SPECIES = 16;	// max species of trees
const U8 MAX_PARTS = 15;	// trunk, 2 or 3 branches per species?
const U8 MAX_RES = 6;		// max # cross sections for a branch curve
const U8 MAX_FLARE = 6;		// max # cross sections for flare of trunk
const U8 MAX_LEVELS = 3;

// initial vertex array allocations
const U32 NUM_INIT_VERTS = 5000;			// number of vertices/normals/texcoords
const U32 NUM_INIT_INDICES = 15000;			// number of indices to vert array (3 vertices per triangle, roughly 3x)
const U32 NUM_TIMES_TO_DOUBLE = 2;			// if we go over initial allocations, num times to double each step

// for finding the closest parts...

// the parts are searched based on:
const F32 MAX_LOBES_DIFF = 2;
const F32 MAX_LOBEDEPTH_DIFF = .3f;
const F32 MAX_CURVEBACK_DIFF = 20.0f;
const F32 MAX_CURVE_DIFF = 15.0f;
const F32 MAX_CURVE_V_DIFF = 20.0f;

const F32 CURVEV_DIVIDER = 10.0f;	// curveV/CURVEV_DIVIDER = # branch variances...
const U8 MAX_VARS = 3;				// max number of variations of branches

const U8 MAX_RAND_NUMS = 100;		// max number of rand numbers to pregenerate and store

// texture params
const F32 WIDTH_OF_BARK = .48f;

class LLVOTreeNew : public LLViewerObject
{
public:

	// Some random number generators using the pre-generated random numbers
	// return +- negPos
	static S32 llrand_signed(S32 negPos)
	{
		return (ll_rand((U32)negPos * 2) - negPos);
	};

	static S32 llrand_signed(S32 negPos, U32 index)
	{
		return lltrunc((sRandNums[index % MAX_RAND_NUMS] * (negPos * 2.0f) - negPos));
	};
	
	static S32 llrand_unsigned(S32 pos, U32 index)
	{
		return lltrunc((sRandNums[index % MAX_RAND_NUMS] * pos));
	};

	// return +- negPos
	static F32 llfrand_signed(F32 negPos)
	{
		return (ll_frand(negPos * 2.0f) - negPos);
	};

	static F32 llfrand_signed(F32 negPos, U32 index)
	{
		return (sRandNums[index % MAX_RAND_NUMS] * negPos * 2.0f) - negPos;
	};

	static F32 llfrand_unsigned(F32 pos, U32 index)
	{
		return sRandNums[index % MAX_RAND_NUMS] * pos;
	};

	// return between 0-pos
	static F32 llfrand_unsigned(F32 pos)
	{
		return ll_frand(pos);
	};

	static void cleanupTextures() {};	// not needed anymore

	struct TreePart
	{
		F32 mRadius;		// scale x/y
		F32 mLength;		// scale z
		F32 mCurve;
		F32 mCurveV;
		F32 mCurveRes;
		F32 mCurveBack;
		U8 mLobes;
		F32 mLobeDepth;
		U8 mLevel;
		U32 mNumTris;
		U8 mVertsPerSection;
		U8 mNumVariants;

		// first index into the drawpool arrays for this particular branch
		U32 mIndiceIndex[MAX_VARS];
		U32 mOffsets[MAX_VARS][MAX_RES];		// offsets for the partial branch pieces
		// local section frames for this branch
		LLMatrix4 mFrames[MAX_VARS][(MAX_RES*(MAX_RES + 1))/2];	// (0...n) + (1...n) + ... + (n-1..n)
		LLDynamicArray<LLVector3> mFaceNormals;

	};

	LLVOTreeNew(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual ~LLVOTreeNew();

	/*virtual*/ 
	U32 processUpdateMessage(LLMessageSystem *mesgsys,
											void **user_data,
											U32 block_num, const EObjectUpdateType update_type,
											LLDataPacker *dp);

	/*virtual*/ BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);

	/*virtual*/ void render(LLAgent &agent);
	/*virtual*/ void updateTextures(LLAgent &agent);

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);

	F32 CalcZStep(TreePart *part, U8 section);

	void createPart(U8 level, F32 length, F32 radius, LLStrider<LLVector3> &vertices, LLStrider<LLVector3> &normals, 
							LLStrider<LLVector2> &tex_coords, U32 *indices, 
							U32 &curVertexIndex, U32 &curTexCoordIndex,
							U32 &curNormalIndex, U32 &curIndiceIndex);

	S32 findSimilarPart(U8 level);

	F32 CalculateSectionRadius(U8 level, F32 y, F32 stemLength, F32 stemRadius);
	//F32 CalculateVerticalAttraction(U8 level, LLMatrix4 &sectionFrame);

	void createSection(LLMatrix4 &frame, TreePart *part, F32 sectionRadius, F32 stemZ, 
							  LLStrider<LLVector3> &vertices, LLStrider<LLVector2> &tex_coords, U32 *indices, 
							  U32 &curVertexIndex, U32 &curTexCoordIndex, U32 &curIndiceIndex, U8 curSection, BOOL firstBranch);

	void genIndicesAndFaceNormalsForLastSection(TreePart *part, U8 numVerts, LLStrider<LLVector3> &vertices, U32 curVertexIndex, U32 *indices, U32 &curIndiceIndex, BOOL firstBranch);

	void genVertexNormals(TreePart *part, LLStrider<LLVector3> &normals, U8 numSections, U32 curNormalOffset);

	void drawTree(LLDrawPool &draw_pool, const LLMatrix4 &frame, U8 level, F32 offsetChild, F32 curLength, F32 parentLength, F32 curRadius, F32 parentRadius, U8 part, U8 variant, U8 startSection);
	void drawTree(LLDrawPool &draw_pool);

	
	//LLTreeParams mParams;
	U8 mSpecies;
	LLPointer<LLViewerTexture> mTreeImagep;
	LLMatrix4 mTrunkFlareFrames[MAX_FLARE];
	F32 mSegSplitsError[3];
	U32 mRandOffset[MAX_LEVELS];

	U32 mNumTrisDrawn;
	U32 mTotalIndices;
	U32 mTotalVerts;

	static void initClass();

	// tree params
	static LLTreeParams sParameters;

	// next indexes used to drawpool arrays
	static U32 sNextVertexIndex[MAX_SPECIES];
	static U32 sNextIndiceIndex[MAX_SPECIES];

	// tree parts
	static U32 sNextPartIndex[MAX_PARTS];
	static TreePart sTreeParts[MAX_SPECIES][MAX_PARTS];

	// species images
	static LLUUID sTreeImageIDs[MAX_SPECIES];

	// random numbers
	static F32 sRandNums[MAX_RAND_NUMS];

	// usage data
	static U32 sTreePartsUsed[MAX_SPECIES][MAX_PARTS][MAX_VARS];
	
	
};

#endif
