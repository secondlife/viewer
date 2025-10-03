/**
 * @file llvolumeoctree.h
 * @brief LLVolume octree classes.
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

#ifndef LL_LLVOLUME_OCTREE_H
#define LL_LLVOLUME_OCTREE_H

#include "linden_common.h"
#include "llmemory.h"

#include "lloctree.h"
#include "llvolume.h"
#include "llvector4a.h"

class alignas(16) LLVolumeTriangle : public LLRefCount
{
    LL_ALIGN_NEW
public:
    LLVolumeTriangle()
    {
        mBinIndex = -1;
    }

    LLVolumeTriangle(const LLVolumeTriangle& rhs)
    {
        *this = rhs;
    }

    ~LLVolumeTriangle()
    {

    }

    LL_ALIGN_16(LLVector4a mPositionGroup);

    const LLVector4a* mV[3];
    U32 mIndex[3];

    F32 mRadius;
    mutable S32 mBinIndex;


    virtual const LLVector4a& getPositionGroup() const;
    virtual const F32& getBinRadius() const;

    S32 getBinIndex() const { return mBinIndex; }
    void setBinIndex(S32 idx) const { mBinIndex = idx; }


};

class alignas(16) LLVolumeOctreeListener : public LLOctreeListener<LLVolumeTriangle, LLVolumeTriangle*>
{
    LL_ALIGN_NEW
public:
    LLVolumeOctreeListener(LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* node);
    ~LLVolumeOctreeListener();

    LLVolumeOctreeListener(const LLVolumeOctreeListener& rhs)
    {
        *this = rhs;
    }

    const LLVolumeOctreeListener& operator=(const LLVolumeOctreeListener& rhs)
    {
        LL_ERRS() << "Illegal operation!" << LL_ENDL;
        return *this;
    }

     //LISTENER FUNCTIONS
    virtual void handleChildAddition(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* parent, LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* child);
    virtual void handleStateChange(const LLTreeNode<LLVolumeTriangle>* node) { }
    virtual void handleChildRemoval(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* parent, const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* child) { }
    virtual void handleInsertion(const LLTreeNode<LLVolumeTriangle>* node, LLVolumeTriangle* tri) { }
    virtual void handleRemoval(const LLTreeNode<LLVolumeTriangle>* node, LLVolumeTriangle* tri) { }
    virtual void handleDestruction(const LLTreeNode<LLVolumeTriangle>* node) { }


public:
    LL_ALIGN_16(LLVector4a mBounds[2]); // bounding box (center, size) of this node and all its children (tight fit to objects)
    LL_ALIGN_16(LLVector4a mExtents[2]); // extents (min, max) of this node and all its children
};

class LLOctreeTriangleRayIntersect : public LLOctreeTraveler<LLVolumeTriangle, LLVolumeTriangle*>
{
public:
    LLVector4a mStart;
    LLVector4a mDir;
    LLVector4a mEnd;
    LLVector4a* mIntersection;
    LLVector2* mTexCoord;
    LLVector4a* mNormal;
    LLVector4a* mTangent;
    F32* mClosestT;
    LLVolumeFace* mFace;
    bool mHitFace;
    const LLVolumeTriangle* mHitTriangle = nullptr;

    LLOctreeTriangleRayIntersect(const LLVector4a& start, const LLVector4a& dir,
                                    LLVolumeFace* face,
                                   F32* closest_t,
                                   LLVector4a* intersection,LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent);

    void traverse(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* node);

    virtual void visit(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* node);
};

class LLVolumeOctreeValidate : public LLOctreeTraveler<LLVolumeTriangle, LLVolumeTriangle*>
{
    virtual void visit(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* branch);
};

class LLVolumeOctreeRebound : public LLOctreeTravelerDepthFirst<LLVolumeTriangle, LLVolumeTriangle*>
{
public:
    LLVolumeOctreeRebound()
    {
    }

    virtual void visit(const LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>* branch)
    { //this is a depth first traversal, so it's safe to assum all children have complete
        //bounding data
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;

            LLVolumeOctreeListener* node = (LLVolumeOctreeListener*)branch->getListener(0);

        LLVector4a& min = node->mExtents[0];
        LLVector4a& max = node->mExtents[1];

        if (!branch->isEmpty())
        { //node has data, find AABB that binds data set
            const LLVolumeTriangle* tri = *(branch->getDataBegin());

            //initialize min/max to first available vertex
            min = *(tri->mV[0]);
            max = *(tri->mV[0]);

            for (LLOctreeNode<LLVolumeTriangle, LLVolumeTriangle*>::const_element_iter iter = branch->getDataBegin(); iter != branch->getDataEnd(); ++iter)
            { //for each triangle in node

                //stretch by triangles in node
                tri = *iter;

                min.setMin(min, *tri->mV[0]);
                min.setMin(min, *tri->mV[1]);
                min.setMin(min, *tri->mV[2]);

                max.setMax(max, *tri->mV[0]);
                max.setMax(max, *tri->mV[1]);
                max.setMax(max, *tri->mV[2]);
            }
        }
        else if (branch->getChildCount() > 0)
        { //no data, but child nodes exist
            LLVolumeOctreeListener* child = (LLVolumeOctreeListener*)branch->getChild(0)->getListener(0);

            //initialize min/max to extents of first child
            min = child->mExtents[0];
            max = child->mExtents[1];
        }
        else
        {
            llassert(!branch->isLeaf()); // Empty leaf
        }

        for (U32 i = 0; i < branch->getChildCount(); ++i)
        {  //stretch by child extents
            LLVolumeOctreeListener* child = (LLVolumeOctreeListener*)branch->getChild(i)->getListener(0);
            min.setMin(min, child->mExtents[0]);
            max.setMax(max, child->mExtents[1]);
        }

        node->mBounds[0].setAdd(min, max);
        node->mBounds[0].mul(0.5f);

        node->mBounds[1].setSub(max, min);
        node->mBounds[1].mul(0.5f);
    }
};

class LLVolumeOctree : public LLOctreeRoot<LLVolumeTriangle, LLVolumeTriangle*>, public LLRefCount
{
public:
    LLVolumeOctree(const LLVector4a& center, const LLVector4a& size)
        :
        LLOctreeRoot<LLVolumeTriangle, LLVolumeTriangle*>(center, size, nullptr),
        LLRefCount()
    {
        new LLVolumeOctreeListener(this);
    }

    LLVolumeOctree()
        : LLOctreeRoot<LLVolumeTriangle, LLVolumeTriangle*>(LLVector4a::getZero(), LLVector4a(1.f,1.f,1.f), nullptr),
        LLRefCount()
    {
        new LLVolumeOctreeListener(this);
    }
};

#endif
