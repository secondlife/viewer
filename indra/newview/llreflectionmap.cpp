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
#include "llworld.h"
#include "llshadermgr.h"

extern F32SecondsImplicit gFrameTimeSeconds;

extern U32 get_box_fan_indices(LLCamera* camera, const LLVector4a& center);

LLReflectionMap::LLReflectionMap()
{
}

LLReflectionMap::~LLReflectionMap()
{
    if (mOcclusionQuery)
    {
        glDeleteQueries(1, &mOcclusionQuery);
    }
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

        if (mGroup->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_VOLUME)
        {
            mPriority = 0;
            // cast a ray towards 8 corners of bounding box
            // nudge origin towards center of empty space

            if (!node)
            {
                return;
            }

            mOrigin = bounds[0];

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

            // make sure origin isn't under ground
            F32* fp = mOrigin.getF32ptr();
            LLVector3 origin(fp);
            F32 height = LLWorld::instance().resolveLandHeightAgent(origin) + 2.f;
            fp[2] = llmax(fp[2], height);
            
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
    }
    else if (mViewerObject)
    {
        mPriority = 1;
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

bool LLReflectionMap::isActive()
{
    return mCubeIndex != -1;
}

void LLReflectionMap::doOcclusion(const LLVector4a& eye)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
#if 1
    // super sloppy, but we're doing an occlusion cull against a bounding cube of
    // a bounding sphere, pad radius so we assume if the eye is within
    // the bounding sphere of the bounding cube, the node is not culled
    F32 dist = mRadius * F_SQRT3 + 1.f;

    LLVector4a o;
    o.setSub(mOrigin, eye);

    bool do_query = false;

    if (o.getLength3().getF32() < dist)
    { // eye is inside radius, don't attempt to occlude
        mOccluded = false;
        return;
    }
    
    if (mOcclusionQuery == 0)
    { // no query was previously issued, allocate one and issue
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("rmdo - glGenQueries");
        glGenQueries(1, &mOcclusionQuery);
        do_query = true;
    }
    else
    { // query was previously issued, check it and only issue a new query
        // if previous query is available
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("rmdo - glGetQueryObject");
        GLuint result = 0;
        glGetQueryObjectuiv(mOcclusionQuery, GL_QUERY_RESULT_AVAILABLE, &result);

        if (result > 0)
        {
            do_query = true;
            glGetQueryObjectuiv(mOcclusionQuery, GL_QUERY_RESULT, &result);
            mOccluded = result == 0;
            mOcclusionPendingFrames = 0;
        }
        else
        {
            mOcclusionPendingFrames++;
        }
    }

    if (do_query)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("rmdo - push query");
        glBeginQuery(GL_ANY_SAMPLES_PASSED, mOcclusionQuery);

        LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

        shader->uniform3fv(LLShaderMgr::BOX_CENTER, 1, mOrigin.getF32ptr());
        shader->uniform3f(LLShaderMgr::BOX_SIZE, mRadius, mRadius, mRadius);

        gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, get_box_fan_indices(LLViewerCamera::getInstance(), mOrigin));

        glEndQuery(GL_ANY_SAMPLES_PASSED);
    }
#endif
}
