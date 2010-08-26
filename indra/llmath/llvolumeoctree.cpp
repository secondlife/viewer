/** 

 * @file llvolumeoctree.cpp
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

#include "llvolumeoctree.h"
#include "llvector4a.h"

BOOL LLLineSegmentBoxIntersect(const LLVector4a& start, const LLVector4a& end, const LLVector4a& center, const LLVector4a& size)
{
	LLVector4a fAWdU;
	LLVector4a dir;
	LLVector4a diff;

	dir.setSub(end, start);
	dir.mul(0.5f);

	diff.setAdd(end,start);
	diff.mul(0.5f);
	diff.sub(center);
	fAWdU.setAbs(dir); 

	LLVector4a rhs;
	rhs.setAdd(size, fAWdU);

	LLVector4a lhs;
	lhs.setAbs(diff);

	U32 grt = lhs.greaterThan(rhs).getGatheredBits();

	if (grt & 0x7)
	{
		return false;
	}
	
	LLVector4a f;
	f.setCross3(dir, diff);
	f.setAbs(f);

	LLVector4a v0, v1;

	v0 = _mm_shuffle_ps(size, size,_MM_SHUFFLE(3,0,0,1));
	v1 = _mm_shuffle_ps(fAWdU, fAWdU, _MM_SHUFFLE(3,1,2,2));
	lhs.setMul(v0, v1);

	v0 = _mm_shuffle_ps(size, size, _MM_SHUFFLE(3,1,2,2));
	v1 = _mm_shuffle_ps(fAWdU, fAWdU, _MM_SHUFFLE(3,0,0,1));
	rhs.setMul(v0, v1);
	rhs.add(lhs);
	
	grt = f.greaterThan(rhs).getGatheredBits();

	return (grt & 0x7) ? false : true;
}


LLVolumeOctreeListener::LLVolumeOctreeListener(LLOctreeNode<LLVolumeTriangle>* node)
{
	node->addListener(this);

	mBounds = (LLVector4a*) ll_aligned_malloc_16(sizeof(LLVector4a)*4);
	mExtents = mBounds+2;
}

LLVolumeOctreeListener::~LLVolumeOctreeListener()
{
	ll_aligned_free_16(mBounds);
}
	
void LLVolumeOctreeListener::handleChildAddition(const LLOctreeNode<LLVolumeTriangle>* parent, 
	LLOctreeNode<LLVolumeTriangle>* child)
{
	new LLVolumeOctreeListener(child);
}


LLOctreeTriangleRayIntersect::LLOctreeTriangleRayIntersect(const LLVector4a& start, const LLVector4a& dir, 
							   const LLVolumeFace* face, F32* closest_t,
							   LLVector3* intersection,LLVector2* tex_coord, LLVector3* normal, LLVector3* bi_normal)
   : mFace(face),
     mStart(start),
	 mDir(dir),
	 mIntersection(intersection),
	 mTexCoord(tex_coord),
	 mNormal(normal),
	 mBinormal(bi_normal),
	 mClosestT(closest_t),
	 mHitFace(false)
{
	mEnd.setAdd(mStart, mDir);
}

void LLOctreeTriangleRayIntersect::traverse(const LLOctreeNode<LLVolumeTriangle>* node)
{
	LLVolumeOctreeListener* vl = (LLVolumeOctreeListener*) node->getListener(0);

	/*const F32* start = mStart.getF32();
	const F32* end = mEnd.getF32();
	const F32* center = vl->mBounds[0].getF32();
	const F32* size = vl->mBounds[1].getF32();*/

	//if (LLLineSegmentBoxIntersect(mStart.getF32(), mEnd.getF32(), vl->mBounds[0].getF32(), vl->mBounds[1].getF32()))
	if (LLLineSegmentBoxIntersect(mStart, mEnd, vl->mBounds[0], vl->mBounds[1]))
	{
		node->accept(this);
		for (S32 i = 0; i < node->getChildCount(); ++i)
		{
			traverse(node->getChild(i));
		}
	}
}

void LLOctreeTriangleRayIntersect::visit(const LLOctreeNode<LLVolumeTriangle>* node)
{
	for (LLOctreeNode<LLVolumeTriangle>::const_element_iter iter = 
			node->getData().begin(); iter != node->getData().end(); ++iter)
	{
		const LLVolumeTriangle* tri = *iter;

		F32 a, b, t;
		
		if (LLTriangleRayIntersect(*tri->mV[0], *tri->mV[1], *tri->mV[2],
				mStart, mDir, a, b, t))
		{
			if ((t >= 0.f) &&      // if hit is after start
				(t <= 1.f) &&      // and before end
				(t < *mClosestT))   // and this hit is closer
			{
				*mClosestT = t;
				mHitFace = true;

				if (mIntersection != NULL)
				{
					LLVector4a intersect = mDir;
					intersect.mul(*mClosestT);
					intersect.add(mStart);
					mIntersection->set(intersect.getF32ptr());
				}


				if (mTexCoord != NULL)
				{
					LLVector2* tc = (LLVector2*) mFace->mTexCoords;
					*mTexCoord = ((1.f - a - b)  * tc[tri->mIndex[0]] +
						a              * tc[tri->mIndex[1]] +
						b              * tc[tri->mIndex[2]]);

				}

				if (mNormal != NULL)
				{
					LLVector4* norm = (LLVector4*) mFace->mNormals;

					*mNormal    = ((1.f - a - b)  * LLVector3(norm[tri->mIndex[0]]) + 
						a              * LLVector3(norm[tri->mIndex[1]]) +
						b              * LLVector3(norm[tri->mIndex[2]]));
				}

				if (mBinormal != NULL)
				{
					LLVector4* binormal = (LLVector4*) mFace->mBinormals;
					*mBinormal = ((1.f - a - b)  * LLVector3(binormal[tri->mIndex[0]]) + 
							a              * LLVector3(binormal[tri->mIndex[1]]) +
							b              * LLVector3(binormal[tri->mIndex[2]]));
				}
			}
		}
	}
}

const LLVector4a& LLVolumeTriangle::getPositionGroup() const
{
	return *mPositionGroup;
}

const F32& LLVolumeTriangle::getBinRadius() const
{
	return mRadius;
}


