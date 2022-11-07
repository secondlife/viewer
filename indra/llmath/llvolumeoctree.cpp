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


LLVolumeOctreeListener::LLVolumeOctreeListener(LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* node)
{
    node->addListener(this);
}

LLVolumeOctreeListener::~LLVolumeOctreeListener()
{

}
    
void LLVolumeOctreeListener::handleChildAddition(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* parent, 
    LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* child)
{
    new LLVolumeOctreeListener(child);
}

LLOctreeTriangleRayIntersect::LLOctreeTriangleRayIntersect(const LLVector4a& start, const LLVector4a& dir, 
                               const LLVolumeFace* face, F32* closest_t,
                               LLVector4a* intersection,LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent)
   : mFace(face),
     mStart(start),
     mDir(dir),
     mIntersection(intersection),
     mTexCoord(tex_coord),
     mNormal(normal),
     mTangent(tangent),
     mClosestT(closest_t),
     mHitFace(false)
{
    mEnd.setAdd(mStart, mDir);
}

void LLOctreeTriangleRayIntersect::traverse(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* node)
{
    LLVolumeOctreeListener* vl = (LLVolumeOctreeListener*) node->getListener(0);

    if (LLLineSegmentBoxIntersect(mStart, mEnd, vl->mBounds[0], vl->mBounds[1]))
    {
        node->accept(this);
        for (S32 i = 0; i < node->getChildCount(); ++i)
        {
            traverse(node->getChild(i));
        }
    }
}

void LLOctreeTriangleRayIntersect::visit(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* node)
{
    for (typename LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>::const_element_iter iter =
            node->getDataBegin(); iter != node->getDataEnd(); ++iter)
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
                    *mIntersection = intersect;
                }

                U32 idx0 = tri->mIndex[0];
                U32 idx1 = tri->mIndex[1];
                U32 idx2 = tri->mIndex[2];

                if (mTexCoord != NULL)
                {
                    LLVector2* tc = (LLVector2*) mFace->mTexCoords;
                    *mTexCoord = ((1.f - a - b)  * tc[idx0] +
                        a              * tc[idx1] +
                        b              * tc[idx2]);

                }

                if (mNormal != NULL)
                {
                    LLVector4a* norm = mFace->mNormals;
                                
                    LLVector4a n1,n2,n3;
                    n1 = norm[idx0];
                    n1.mul(1.f-a-b);
                                
                    n2 = norm[idx1];
                    n2.mul(a);
                                
                    n3 = norm[idx2];
                    n3.mul(b);

                    n1.add(n2);
                    n1.add(n3);
                                
                    *mNormal        = n1; 
                }

                if (mTangent != NULL)
                {
                    LLVector4a* tangents = mFace->mTangents;
                                
                    LLVector4a t1,t2,t3;
                    t1 = tangents[idx0];
                    t1.mul(1.f-a-b);
                                
                    t2 = tangents[idx1];
                    t2.mul(a);
                                
                    t3 = tangents[idx2];
                    t3.mul(b);

                    t1.add(t2);
                    t1.add(t3);
                                
                    *mTangent = t1; 
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

void LLVolumeOctreeValidate::visit(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* branch)
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
        LL_ERRS() << "Bad bounding box data found." << LL_ENDL;
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
            LL_ERRS() << "Child protrudes from bounding box." << LL_ENDL;
        }
    }

    //children fit, check data
    for (typename LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>::const_element_iter iter = branch->getDataBegin();
            iter != branch->getDataEnd(); ++iter)
    {
        const LLVolumeTriangle* tri = *iter;

        //validate triangle
        for (U32 i = 0; i < 3; i++)
        {
            if (tri->mV[i]->greaterThan(test_max).areAnySet(LLVector4Logical::MASK_XYZ) ||
                tri->mV[i]->lessThan(test_min).areAnySet(LLVector4Logical::MASK_XYZ))
            {
                LL_ERRS() << "Triangle protrudes from node." << LL_ENDL;
            }
        }
    }
}

