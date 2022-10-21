/**
 * @file llreflectionmap.cpp
 * @brief LLReflectionMap class implementation
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llreflectionmap.h"
#include "pipeline.h"
#include "llviewerwindow.h"
#include "llviewerregion.h"

extern F32SecondsImplicit gFrameTimeSeconds;

LLReflectionMap::LLReflectionMap()
{
}

void LLReflectionMap::update(U32 resolution, U32 face)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    mLastUpdateTime = gFrameTimeSeconds;
    llassert(mCubeArray.notNull());
    llassert(mCubeIndex != -1);
    //llassert(LLPipeline::sRenderDeferred);
    
    // make sure we don't walk off the edge of the render target
    while (resolution > gPipeline.mRT->deferredScreen.getWidth() ||
        resolution > gPipeline.mRT->deferredScreen.getHeight())
    {
        resolution /= 2;
    }
    gViewerWindow->cubeSnapshot(LLVector3(mOrigin), mCubeArray, mCubeIndex, face, getNearClip(), getIsDynamic());
}

void LLReflectionMap::autoAdjustOrigin()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;

    if (mGroup)
    {
        const LLVector4a* bounds = mGroup->getBounds();
        auto* node = mGroup->getOctreeNode();

        if (mGroup->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_TERRAIN)
        {
            // for terrain, make probes float a couple meters above the highest point in the surface patch
            mOrigin = bounds[0];
            mOrigin.getF32ptr()[2] += bounds[1].getF32ptr()[2] + 3.f;

            // update radius to encompass bounding box
            LLVector4a d;
            d.setAdd(bounds[0], bounds[1]);
            d.sub(mOrigin);
            mRadius = d.getLength3().getF32();
            mPriority = 1;
        }
        else if (mGroup->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_VOLUME)
        {
            mPriority = 1;
            // cast a ray towards 8 corners of bounding box
            // nudge origin towards center of empty space

            if (!node)
            {
                return;
            }

            if (node->isLeaf() || node->getChildCount() > 1 || node->getElementCount() > 0)
            { // use center of object bounding box for leaf nodes or nodes with multiple child nodes
                mOrigin = bounds[0];

                LLVector4a start;
                LLVector4a end;

                LLVector4a size = bounds[1];

                LLVector4a corners[] =
                {
                    { 1, 1, 1 },
                    { -1, 1, 1 },
                    { 1, -1, 1 },
                    { -1, -1, 1 },
                    { 1, 1, -1 },
                    { -1, 1, -1 },
                    { 1, -1, -1 },
                    { -1, -1, -1 }
                };

                for (int i = 0; i < 8; ++i)
                {
                    corners[i].mul(size);
                    corners[i].add(bounds[0]);
                }

                LLVector4a extents[2];
                extents[0].setAdd(bounds[0], bounds[1]);
                extents[1].setSub(bounds[0], bounds[1]);

                bool hit = false;
                for (int i = 0; i < 8; ++i)
                {
                    int face = -1;
                    LLVector4a intersection;
                    LLDrawable* drawable = mGroup->lineSegmentIntersect(bounds[0], corners[i], true, false, true, &face, &intersection);
                    if (drawable != nullptr)
                    {
                        hit = true;
                        update_min_max(extents[0], extents[1], intersection);
                    }
                    else
                    {
                        update_min_max(extents[0], extents[1], corners[i]);
                    }
                }

                if (hit)
                {
                    mOrigin.setAdd(extents[0], extents[1]);
                    mOrigin.mul(0.5f);
                }

                // make sure radius encompasses all objects
                LLSimdScalar r2 = 0.0;
                for (int i = 0; i < 8; ++i)
                {
                    LLVector4a v;
                    v.setSub(corners[i], mOrigin);

                    LLSimdScalar d = v.dot3(v);

                    if (d > r2)
                    {
                        r2 = d;
                    }
                }

                mRadius = llmax(sqrtf(r2.getF32()), 8.f);
            }
            else
            {
                // user placed probe
                mPriority = 2;

                // use center of octree node volume for nodes that are just branches without data
                mOrigin = node->getCenter();

                // update radius to encompass entire octree node volume
                mRadius = node->getSize().getLength3().getF32();

                //mOrigin = bounds[0];
                //mRadius = bounds[1].getLength3().getF32();

            }
        }
    }
    else if (mViewerObject)
    {
        mPriority = 64;
        mOrigin.load3(mViewerObject->getPositionAgent().mV);
        mRadius = mViewerObject->getScale().mV[0]*0.5f;
    }
}

bool LLReflectionMap::intersects(LLReflectionMap* other)
{
    // TODO: incorporate getBox
    LLVector4a delta;
    delta.setSub(other->mOrigin, mOrigin);

    F32 dist = delta.dot3(delta).getF32();

    F32 r2 = mRadius + other->mRadius;

    r2 *= r2;

    return dist < r2;
}

extern LLControlGroup gSavedSettings;

F32 LLReflectionMap::getAmbiance()
{
    F32 ret = 0.f;
    if (mViewerObject && mViewerObject->getVolume())
    {
        ret = ((LLVOVolume*)mViewerObject)->getReflectionProbeAmbiance();
    }

    return ret;
}

F32 LLReflectionMap::getNearClip()
{
    const F32 MINIMUM_NEAR_CLIP = 0.1f;

    F32 ret = 0.f;

    if (mViewerObject && mViewerObject->getVolume())
    {
        ret = ((LLVOVolume*)mViewerObject)->getReflectionProbeNearClip();
    }

    return llmax(ret, MINIMUM_NEAR_CLIP);
}

bool LLReflectionMap::getIsDynamic()
{
    if (gSavedSettings.getS32("RenderReflectionProbeDetail") > (S32) LLReflectionMapManager::DetailLevel::STATIC_ONLY &&
        mViewerObject && 
        mViewerObject->getVolume())
    {
        return ((LLVOVolume*)mViewerObject)->getReflectionProbeIsDynamic();
    }

    return false;
}

bool LLReflectionMap::getBox(LLMatrix4& box)
{ 
    if (mViewerObject)
    {
        LLVolume* volume = mViewerObject->getVolume();
        if (volume)
        {
            LLVOVolume* vobjp = (LLVOVolume*)mViewerObject;

            if (vobjp->getReflectionProbeIsBox())
            {
                glh::matrix4f mv(gGLModelView);
                glh::matrix4f scale;
                LLVector3 s = vobjp->getScale().scaledVec(LLVector3(0.5f, 0.5f, 0.5f));
                mRadius = s.magVec();
                scale.set_scale(glh::vec3f(s.mV));
                if (vobjp->mDrawable != nullptr)
                {
                    glh::matrix4f rm((F32*)vobjp->mDrawable->getWorldMatrix().mMatrix);

                    glh::matrix4f rt((F32*)vobjp->getRelativeXform().mMatrix);

                    mv = mv * rm * scale; // *rt;
                    mv = mv.inverse();

                    box = LLMatrix4(mv.m);

                    return true;
                }
            }
        }
    }

    return false;
}
