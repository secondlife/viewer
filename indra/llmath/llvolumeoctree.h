/** 
 * @file llvolumeoctree.h
 * @brief LLVolume octree classes.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLVOLUME_OCTREE_H
#define LL_LLVOLUME_OCTREE_H

#include "linden_common.h"
#include "llmemory.h"

#include "lloctree.h"
#include "llvolume.h"
#include "llvector4a.h"

class LLVolumeOctreeListener : public LLOctreeListener<LLVolumeTriangle>
{
public:
	
	LLVolumeOctreeListener(LLOctreeNode<LLVolumeTriangle>* node);
	~LLVolumeOctreeListener();
	
	LLVolumeOctreeListener(const LLVolumeOctreeListener& rhs)
	{
		*this = rhs;
	}

	const LLVolumeOctreeListener& operator=(const LLVolumeOctreeListener& rhs)
	{
		llerrs << "Illegal operation!" << llendl;
		return *this;
	}

	 //LISTENER FUNCTIONS
	virtual void handleChildAddition(const LLOctreeNode<LLVolumeTriangle>* parent, 
		LLOctreeNode<LLVolumeTriangle>* child);
	virtual void handleStateChange(const LLTreeNode<LLVolumeTriangle>* node) { }
	virtual void handleChildRemoval(const LLOctreeNode<LLVolumeTriangle>* parent, 
			const LLOctreeNode<LLVolumeTriangle>* child) {	}
	virtual void handleInsertion(const LLTreeNode<LLVolumeTriangle>* node, LLVolumeTriangle* tri) { }
	virtual void handleRemoval(const LLTreeNode<LLVolumeTriangle>* node, LLVolumeTriangle* tri) { }
	virtual void handleDestruction(const LLTreeNode<LLVolumeTriangle>* node) { }
	

public:
	LLVector4a* mBounds; // bounding box (center, size) of this node and all its children (tight fit to objects)
	LLVector4a* mExtents; // extents (min, max) of this node and all its children
};

class LLOctreeTriangleRayIntersect : public LLOctreeTraveler<LLVolumeTriangle>
{
public:
	const LLVolumeFace* mFace;
	LLVector4a mStart;
	LLVector4a mDir;
	LLVector4a mEnd;
	LLVector3* mIntersection;
	LLVector2* mTexCoord;
	LLVector3* mNormal;
	LLVector3* mBinormal;
	F32* mClosestT;
	bool mHitFace;

	LLOctreeTriangleRayIntersect(const LLVector4a& start, const LLVector4a& dir, 
								   const LLVolumeFace* face, F32* closest_t,
								   LLVector3* intersection,LLVector2* tex_coord, LLVector3* normal, LLVector3* bi_normal);

	void traverse(const LLOctreeNode<LLVolumeTriangle>* node);

	virtual void visit(const LLOctreeNode<LLVolumeTriangle>* node);
};

class LLVolumeTriangle : public LLRefCount
{
public:
	LLVolumeTriangle()
	{
		mPositionGroup = (LLVector4a*) ll_aligned_malloc_16(16);
	}

	LLVolumeTriangle(const LLVolumeTriangle& rhs)
	{
		*this = rhs;
	}

	const LLVolumeTriangle& operator=(const LLVolumeTriangle& rhs)
	{
		llerrs << "Illegal operation!" << llendl;
		return *this;
	}

	~LLVolumeTriangle()
	{
		ll_aligned_free_16(mPositionGroup);
	}

	const LLVector4a* mV[3];
	U16 mIndex[3];

	LLVector4a* mPositionGroup;

	F32 mRadius;

	virtual const LLVector4a& getPositionGroup() const;
	virtual const F32& getBinRadius() const;
};

class LLVolumeOctreeValidate : public LLOctreeTraveler<LLVolumeTriangle>
{
	virtual void visit(const LLOctreeNode<LLVolumeTriangle>* branch);
};

#endif
