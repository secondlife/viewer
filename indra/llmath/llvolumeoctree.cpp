/** 

 * @file llvolumeoctree.cpp
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
}

LLVolumeOctreeListener::~LLVolumeOctreeListener()
{

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

	//if (LLLineSegmentBoxIntersect(mStart, mEnd, vl->mBounds[0], vl->mBounds[1]))
	if (LLLineSegmentBoxIntersect(mStart.getF32ptr(), mEnd.getF32ptr(), vl->mBounds[0].getF32ptr(), vl->mBounds[1].getF32ptr()))
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
	return mPositionGroup;
}

const F32& LLVolumeTriangle::getBinRadius() const
{
	return mRadius;
}


//TEST CODE

void LLVolumeOctreeValidate::visit(const LLOctreeNode<LLVolumeTriangle>* branch)
{
	LLVolumeOctreeListener* node = (LLVolumeOctreeListener*) branch->getListener(0);

	//make sure bounds matches extents
	LLVector4a& min = node->mExtents[0];
	LLVector4a& max = node->mExtents[1];

	LLVector4a& center = node->mBounds[0];
	LLVector4a& size = node->mBounds[1];

	LLVector4a test_min, test_max;
	test_min.setSub(center, size);
	test_max.setAdd(center, size);

	if (!test_min.equals3(min, 0.001f) ||
		!test_max.equals3(max, 0.001f))
	{
		llerrs << "Bad bounding box data found." << llendl;
	}

	test_min.sub(LLVector4a(0.001f));
	test_max.add(LLVector4a(0.001f));

	for (U32 i = 0; i < branch->getChildCount(); ++i)
	{
		LLVolumeOctreeListener* child = (LLVolumeOctreeListener*) branch->getChild(i)->getListener(0);

		//make sure all children fit inside this node
		if (child->mExtents[0].lessThan(test_min).areAnySet(LLVector4Logical::MASK_XYZ) ||
			child->mExtents[1].greaterThan(test_max).areAnySet(LLVector4Logical::MASK_XYZ))
		{
			llerrs << "Child protrudes from bounding box." << llendl;
		}
	}

	//children fit, check data
	for (LLOctreeNode<LLVolumeTriangle>::const_element_iter iter = branch->getData().begin(); 
			iter != branch->getData().end(); ++iter)
	{
		const LLVolumeTriangle* tri = *iter;

		//validate triangle
		for (U32 i = 0; i < 3; i++)
		{
			if (tri->mV[i]->greaterThan(test_max).areAnySet(LLVector4Logical::MASK_XYZ) ||
				tri->mV[i]->lessThan(test_min).areAnySet(LLVector4Logical::MASK_XYZ))
			{
				llerrs << "Triangle protrudes from node." << llendl;
			}
		}
	}
}


