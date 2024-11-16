/**
 * @file LLHeroProbeManager.cpp
 * @brief LLHeroProbeManager class implementation
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

#include "llheroprobemanager.h"
#include "llreflectionmapmanager.h"
#include "llviewercamera.h"
#include "llspatialpartition.h"
#include "llviewerregion.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llviewercontrol.h"
#include "llenvironment.h"
#include "llstartup.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewerwindow.h"
#include "llviewerjoystick.h"
#include "llviewermediafocus.h"

extern bool gCubeSnapshot;
extern bool gTeleportDisplay;

// get the next highest power of two of v (or v if v is already a power of two)
//defined in llvertexbuffer.cpp
extern U32 nhpo2(U32 v);

static void touch_default_probe(LLReflectionMap* probe)
{
    if (LLViewerCamera::getInstance())
    {
        LLVector3 origin = LLViewerCamera::getInstance()->getOrigin();
        origin.mV[2] += 64.f;

        probe->mOrigin.load3(origin.mV);
    }
}

LLHeroProbeManager::LLHeroProbeManager()
{
}

LLHeroProbeManager::~LLHeroProbeManager()
{
    cleanup();

    mHeroVOList.clear();
    mNearestHero = nullptr;
}

// helper class to seed octree with probes
void LLHeroProbeManager::update()
{
    if (!LLPipeline::RenderMirrors || !LLPipeline::sReflectionProbesEnabled || gTeleportDisplay || LLStartUp::getStartupState() < STATE_PRECACHE)
    {
        return;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LL_PROFILE_GPU_ZONE("hero manager update");
    llassert(!gCubeSnapshot); // assert a snapshot is not in progress
    if (LLAppViewer::instance()->logoutRequestSent())
    {
        return;
    }

    initReflectionMaps();

    if (!mRenderTarget.isComplete())
    {
        U32 color_fmt = GL_RGBA16F;
        mRenderTarget.allocate(mProbeResolution, mProbeResolution, color_fmt, true);
    }

    if (mMipChain.empty())
    {
        U32 res = mProbeResolution;
        U32 count = (U32)(log2((F32)res) + 0.5f);

        mMipChain.resize(count);
        for (U32 i = 0; i < count; ++i)
        {
            mMipChain[i].allocate(res, res, GL_RGBA16F);
            res /= 2;
        }
    }

    llassert(mProbes[0] == mDefaultProbe);

    LLVector4a probe_pos;
    LLVector3 camera_pos = LLViewerCamera::instance().mOrigin;
    bool       probe_present = false;
    LLQuaternion cameraOrientation = LLViewerCamera::instance().getQuaternion();
    LLVector3    cameraDirection   = LLVector3::z_axis * cameraOrientation;

    if (mHeroVOList.size() > 0)
    {
        // Find our nearest hero candidate.
        float last_distance = 99999.f;
        float camera_center_distance = 99999.f;
        mNearestHero = nullptr;
        for (auto vo : mHeroVOList)
        {
            if (vo && !vo->isDead() && vo->mDrawable.notNull() && vo->isReflectionProbe() && vo->getReflectionProbeIsBox())
            {
                float distance = (LLViewerCamera::instance().getOrigin() - vo->getPositionAgent()).magVec();
                float center_distance = cameraDirection * (vo->getPositionAgent() - camera_pos);

                if (distance > LLViewerCamera::instance().getFar())
                    continue;

                LLVector4a center;
                center.load3(vo->getPositionAgent().mV);
                LLVector4a size;

                size.load3(vo->getScale().mV);

                bool visible = LLViewerCamera::instance().AABBInFrustum(center, size);

                if (distance < last_distance && center_distance < camera_center_distance && visible)
                {
                    probe_present = true;
                    mNearestHero = vo;
                    last_distance = distance;
                    camera_center_distance = center_distance;
                }
            }
            else
            {
                unregisterViewerObject(vo);
            }
        }

        // Don't even try to do anything if we didn't find a single mirror present.
        if (!probe_present)
            return;

        if (mNearestHero != nullptr && !mNearestHero->isDead() && mNearestHero->mDrawable.notNull())
        {
            LLVector3 hero_pos = mNearestHero->getPositionAgent();
            LLVector3 face_normal = LLVector3(0, 0, 1);

            face_normal *= mNearestHero->mDrawable->getWorldRotation();
            face_normal.normalize();

            LLVector3 offset = camera_pos - hero_pos;
            LLVector3 project = face_normal * (offset * face_normal);
            LLVector3 reject  = offset - project;
            LLVector3 point   = (reject - project) + hero_pos;

            mCurrentClipPlane.setVec(hero_pos, face_normal);
            mMirrorPosition = hero_pos;
            mMirrorNormal   = face_normal;

            probe_pos.load3(point.mV);

            // Detect visible faces of a cube based on camera direction and distance

            // Define the cube faces
            static LLVector3 cubeFaces[6] = {
                LLVector3(1, 0, 0),
                LLVector3(-1, 0, 0),
                LLVector3(0, 1, 0),
                LLVector3(0, -1, 0),
                LLVector3(0, 0, 1),
                LLVector3(0, 0, -1)
            };

            mProbes[0]->mOrigin = probe_pos;
            mProbes[0]->mRadius = mNearestHero->getScale().magVec() * 0.5f;
        }
        else
        {
            mNearestHero = nullptr;
            mDefaultProbe->mViewerObject = nullptr;
        }

        mHeroProbeStrength = 1;
    }
    else
    {
        mNearestHero = nullptr;
        mDefaultProbe->mViewerObject = nullptr;
    }
}

void LLHeroProbeManager::renderProbes()
{
    if (!LLPipeline::RenderMirrors || !LLPipeline::sReflectionProbesEnabled || gTeleportDisplay ||
        LLStartUp::getStartupState() < STATE_PRECACHE)
    {
        return;
    }

    static LLCachedControl<S32> sDetail(gSavedSettings, "RenderHeroReflectionProbeDetail", -1);
    static LLCachedControl<S32> sLevel(gSavedSettings, "RenderHeroReflectionProbeLevel", 3);
    static LLCachedControl<S32> sUpdateRate(gSavedSettings, "RenderHeroProbeUpdateRate", 0);

    F32 near_clip = 0.01f;
    if (mNearestHero != nullptr &&
        !gTeleportDisplay && !gDisconnected && !LLAppViewer::instance()->logoutRequestSent())
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("hpmu - realtime");

        bool radiance_pass = gPipeline.mReflectionMapManager.isRadiancePass();

        gPipeline.mReflectionMapManager.mRadiancePass = true;
        mRenderingMirror = true;

        S32 rate = sUpdateRate;

        // rate must be divisor of 6 (1, 2, 3, or 6)
        if (rate < 1)
        {
            rate = 1;
        }
        else if (rate > 3)
        {
            rate = 6;
        }

        S32 face = gFrameCount % 6;

        if (!mProbes.empty() && !mProbes[0].isNull() && !mProbes[0]->mOccluded)
        {
            LL_PROFILE_ZONE_NUM(gFrameCount % rate);
            LL_PROFILE_ZONE_NUM(rate);

            for (U32 i = 0; i < 6; ++i)
            {
                if ((gFrameCount % rate) == (i % rate))
                { // update 6/rate faces per frame
                    LL_PROFILE_ZONE_NUM(i);
                    updateProbeFace(mProbes[0], i, mNearestHero->getReflectionProbeIsDynamic() && sDetail > 0, near_clip);
                }
            }
            generateRadiance(mProbes[0]);
        }

        mRenderingMirror = false;

        gPipeline.mReflectionMapManager.mRadiancePass = radiance_pass;

        mProbes[0]->mViewerObject = mNearestHero;
        mProbes[0]->autoAdjustOrigin();
    }
}

// Do the reflection map update render passes.
// For every 12 calls of this function, one complete reflection probe radiance map and irradiance map is generated
// First six passes render the scene with direct lighting only into a scratch space cube map at the end of the cube map array and generate
// a simple mip chain (not convolution filter).
// At the end of these passes, an irradiance map is generated for this probe and placed into the irradiance cube map array at the index for this probe
// The next six passes render the scene with both radiance and irradiance into the same scratch space cube map and generate a simple mip chain.
// At the end of these passes, a radiance map is generated for this probe and placed into the radiance cube map array at the index for this probe.
// In effect this simulates single-bounce lighting.
void LLHeroProbeManager::updateProbeFace(LLReflectionMap* probe, U32 face, bool is_dynamic, F32 near_clip)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LL_PROFILE_GPU_ZONE("hero probe update");

    // hacky hot-swap of camera specific render targets
    gPipeline.mRT = &gPipeline.mHeroProbeRT;

    probe->update(mRenderTarget.getWidth(), face, is_dynamic, near_clip);

    gPipeline.mRT = &gPipeline.mMainRT;

    S32 sourceIdx = mReflectionProbeCount;

    // Unlike the reflectionmap manager, all probes are considered "realtime" for hero probes.
    sourceIdx += 1;

    gGL.setColorMask(true, true);
    LLGLDepthTest depth(GL_FALSE, GL_FALSE);
    LLGLDisable cull(GL_CULL_FACE);
    LLGLDisable blend(GL_BLEND);

    // downsample to placeholder map
    {
        gGL.matrixMode(gGL.MM_MODELVIEW);
        gGL.pushMatrix();
        gGL.loadIdentity();

        gGL.matrixMode(gGL.MM_PROJECTION);
        gGL.pushMatrix();
        gGL.loadIdentity();

        gGL.flush();
        U32 res = mProbeResolution * 2;

        static LLStaticHashedString resScale("resScale");
        static LLStaticHashedString direction("direction");
        static LLStaticHashedString znear("znear");
        static LLStaticHashedString zfar("zfar");

        LLRenderTarget *screen_rt = &gPipeline.mHeroProbeRT.screen;
        LLRenderTarget *depth_rt  = &gPipeline.mHeroProbeRT.deferredScreen;

        // perform a gaussian blur on the super sampled render before downsampling
        {
            gGaussianProgram.bind();
            gGaussianProgram.uniform1f(resScale, 1.f / (mProbeResolution * 2));
            S32 diffuseChannel = gGaussianProgram.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, LLTexUnit::TT_TEXTURE);

            // horizontal
            gGaussianProgram.uniform2f(direction, 1.f, 0.f);
            gGL.getTexUnit(diffuseChannel)->bind(screen_rt);
            mRenderTarget.bindTarget();
            gPipeline.mScreenTriangleVB->setBuffer();
            gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            mRenderTarget.flush();

            // vertical
            gGaussianProgram.uniform2f(direction, 0.f, 1.f);
            gGL.getTexUnit(diffuseChannel)->bind(&mRenderTarget);
            screen_rt->bindTarget();
            gPipeline.mScreenTriangleVB->setBuffer();
            gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            screen_rt->flush();
            gGaussianProgram.unbind();
        }

        S32 mips = (S32)(log2((F32)mProbeResolution) + 0.5f);

        gReflectionMipProgram.bind();
        S32 diffuseChannel = gReflectionMipProgram.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, LLTexUnit::TT_TEXTURE);
        S32 depthChannel   = gReflectionMipProgram.enableTexture(LLShaderMgr::DEFERRED_DEPTH, LLTexUnit::TT_TEXTURE);

        for (int i = 0; i < mMipChain.size(); ++i)
        {
            LL_PROFILE_GPU_ZONE("hero probe mip");
            mMipChain[i].bindTarget();
            if (i == 0)
            {
                gGL.getTexUnit(diffuseChannel)->bind(screen_rt);
            }
            else
            {
                gGL.getTexUnit(diffuseChannel)->bind(&(mMipChain[i - 1]));
            }

            gGL.getTexUnit(depthChannel)->bind(depth_rt, true);

            gReflectionMipProgram.uniform1f(resScale, 1.f / (mProbeResolution * 2));
            gReflectionMipProgram.uniform1f(znear, probe->getNearClip());
            gReflectionMipProgram.uniform1f(zfar, MAX_FAR_CLIP);

            gPipeline.mScreenTriangleVB->setBuffer();
            gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

            res /= 2;

            llassert(mMipChain.size() <= size_t(S32_MAX));
            GLint mip = i - (S32(mMipChain.size()) - mips);

            if (mip >= 0)
            {
                LL_PROFILE_GPU_ZONE("hero probe mip copy");
                mTexture->bind(0);

                glCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, 0, 0, sourceIdx * 6 + face, 0, 0, res, res);

                mTexture->unbind();
            }
            mMipChain[i].flush();
        }

        gGL.popMatrix();
        gGL.matrixMode(gGL.MM_MODELVIEW);
        gGL.popMatrix();

        gGL.getTexUnit(diffuseChannel)->unbind(LLTexUnit::TT_TEXTURE);
        gReflectionMipProgram.unbind();
    }
}

// Separate out radiance generation as a separate stage.
// This is to better enable independent control over how we generate radiance vs. having it coupled with processing the final face of the probe.
// Useful when we may not always be rendering a full set of faces of the probe.
void LLHeroProbeManager::generateRadiance(LLReflectionMap* probe)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    S32 sourceIdx = mReflectionProbeCount;

    // Unlike the reflectionmap manager, all probes are considered "realtime" for hero probes.
    sourceIdx += 1;
    {
        mMipChain[0].bindTarget();
        static LLStaticHashedString sSourceIdx("sourceIdx");

        {
            // generate radiance map (even if this is not the irradiance map, we need the mip chain for the irradiance map)
            gHeroRadianceGenProgram.bind();
            mVertexBuffer->setBuffer();

            S32 channel = gHeroRadianceGenProgram.enableTexture(LLShaderMgr::REFLECTION_PROBES, LLTexUnit::TT_CUBE_MAP_ARRAY);
            mTexture->bind(channel);
            gHeroRadianceGenProgram.uniform1i(sSourceIdx, sourceIdx);
            gHeroRadianceGenProgram.uniform1f(LLShaderMgr::REFLECTION_PROBE_MAX_LOD, mMaxProbeLOD);
            gHeroRadianceGenProgram.uniform1f(LLShaderMgr::REFLECTION_PROBE_STRENGTH, mHeroProbeStrength);

            U32 res = mMipChain[0].getWidth();

            for (int i = 0; i < mMipChain.size() / 4; ++i)
            {
                LL_PROFILE_GPU_ZONE("hero probe radiance gen");
                static LLStaticHashedString sMipLevel("mipLevel");
                static LLStaticHashedString sRoughness("roughness");
                static LLStaticHashedString sWidth("u_width");
                static LLStaticHashedString sStrength("probe_strength");

                gHeroRadianceGenProgram.uniform1f(sRoughness, (F32) i / (F32) (mMipChain.size() - 1));
                gHeroRadianceGenProgram.uniform1f(sMipLevel, (GLfloat)i);
                gHeroRadianceGenProgram.uniform1i(sWidth, mProbeResolution);
                gHeroRadianceGenProgram.uniform1f(sStrength, 1);

                for (int cf = 0; cf < 6; ++cf)
                {  // for each cube face
                    LLCoordFrame frame;
                    frame.lookAt(LLVector3(0, 0, 0), LLCubeMapArray::sClipToCubeLookVecs[cf], LLCubeMapArray::sClipToCubeUpVecs[cf]);

                    F32 mat[16];
                    frame.getOpenGLRotation(mat);
                    gGL.loadMatrix(mat);

                    mVertexBuffer->drawArrays(gGL.TRIANGLE_STRIP, 0, 4);

                    glCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, i, 0, 0, probe->mCubeIndex * 6 + cf, 0, 0, res, res);
                }

                if (i != mMipChain.size() - 1)
                {
                    res /= 2;
                    glViewport(0, 0, res, res);
                }
            }

            gHeroRadianceGenProgram.unbind();
        }

        mMipChain[0].flush();
    }
}

void LLHeroProbeManager::updateUniforms()
{
    if (!gPipeline.RenderMirrors)
    {
        return;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LL_PROFILE_GPU_ZONE("hpmu - uniforms")

    LLMatrix4a modelview;
    modelview.loadu(gGLModelView);
    LLVector4a oa; // scratch space for transformed origin
    oa.set(0, 0, 0, 0);
    mHeroData.heroProbeCount = 1;

    if (mNearestHero != nullptr && !mNearestHero->isDead())
    {
        if (mNearestHero->getReflectionProbeIsBox())
        {
            LLVector3 s = mNearestHero->getScale().scaledVec(LLVector3(0.5f, 0.5f, 0.5f));
            mProbes[0]->mRadius = s.magVec();
        }
        else
        {
            mProbes[0]->mRadius = mNearestHero->getScale().mV[0] * 0.5f;
        }

        modelview.affineTransform(mProbes[0]->mOrigin, oa);
        mHeroData.heroShape = 0;
        if (!mProbes[0]->getBox(mHeroData.heroBox))
        {
            mHeroData.heroShape = 1;
        }

        mHeroData.heroSphere.set(oa.getF32ptr());
        mHeroData.heroSphere.mV[3] = mProbes[0]->mRadius;
    }

    llassert(mMipChain.size() <= size_t(S32_MAX));
    mHeroData.heroMipCount = S32(mMipChain.size());
}

void LLHeroProbeManager::renderDebug()
{
    gDebugProgram.bind();

    for (auto& probe : mProbes)
    {
        renderReflectionProbe(probe);
    }

    gDebugProgram.unbind();
}


void LLHeroProbeManager::initReflectionMaps()
{
    U32 count = LL_MAX_HERO_PROBE_COUNT;

    if ((mTexture.isNull() || mReflectionProbeCount != count || mReset) && LLPipeline::RenderMirrors)
    {

        if (mReset)
        {
            cleanup();
        }

        mReset = false;
        mReflectionProbeCount = count;
        mProbeResolution      = gSavedSettings.getS32("RenderHeroProbeResolution");
        mMaxProbeLOD = log2f((F32)mProbeResolution) - 1.f; // number of mips - 1

        mTexture = new LLCubeMapArray();

        // store mReflectionProbeCount+2 cube maps, final two cube maps are used for render target and radiance map generation source)
        mTexture->allocate(mProbeResolution, 3, mReflectionProbeCount + 2);

        if (mDefaultProbe.isNull())
        {
            llassert(mProbes.empty()); // default probe MUST be the first probe created
            mDefaultProbe = new LLReflectionMap();
            mProbes.push_back(mDefaultProbe);
        }

        llassert(mProbes[0] == mDefaultProbe);

        // For hero probes, we treat this as the main mirror probe.

        mDefaultProbe->mCubeIndex = 0;
        mDefaultProbe->mCubeArray = mTexture;
        mDefaultProbe->mDistance  = gSavedSettings.getF32("RenderHeroProbeDistance");
        mDefaultProbe->mRadius = 4096.f;
        mDefaultProbe->mProbeIndex = 0;
        touch_default_probe(mDefaultProbe);

        mProbes.push_back(mDefaultProbe);
    }

    if (mVertexBuffer.isNull())
    {
        U32 mask = LLVertexBuffer::MAP_VERTEX;
        LLPointer<LLVertexBuffer> buff = new LLVertexBuffer(mask);
        buff->allocateBuffer(4, 0);

        LLStrider<LLVector3> v;

        buff->getVertexStrider(v);

        v[0] = LLVector3(-1, -1, -1);
        v[1] = LLVector3(1, -1, -1);
        v[2] = LLVector3(-1, 1, -1);
        v[3] = LLVector3(1, 1, -1);

        buff->unmapBuffer();

        mVertexBuffer = buff;
    }
}

void LLHeroProbeManager::cleanup()
{
    mVertexBuffer = nullptr;
    mRenderTarget.release();

    mMipChain.clear();

    mTexture = nullptr;

    mProbes.clear();

    mDefaultProbe = nullptr;
}

void LLHeroProbeManager::doOcclusion()
{
    LLVector4a eye;
    eye.load3(LLViewerCamera::instance().getOrigin().mV);

    for (auto& probe : mProbes)
    {
        if (probe != nullptr)
        {
            probe->doOcclusion(eye);
        }
    }
}

void LLHeroProbeManager::reset()
{
    mReset = true;
}

bool LLHeroProbeManager::registerViewerObject(LLVOVolume* drawablep)
{
    llassert(drawablep != nullptr);

    if (std::find(mHeroVOList.begin(), mHeroVOList.end(), drawablep) == mHeroVOList.end())
    {
        // Probe isn't in our list for consideration.  Add it.
        mHeroVOList.push_back(drawablep);
        return true;
    }

    return false;
}

void LLHeroProbeManager::unregisterViewerObject(LLVOVolume* drawablep)
{
    std::vector<LLPointer<LLVOVolume>>::iterator found_itr = std::find(mHeroVOList.begin(), mHeroVOList.end(), drawablep);
    if (found_itr != mHeroVOList.end())
    {
        mHeroVOList.erase(found_itr);
    }
}
