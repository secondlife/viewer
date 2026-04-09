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
    mActiveHeroes.clear();
}

// helper class to seed octree with probes
void LLHeroProbeManager::update()
{
    if (!LLPipeline::RenderMirrors || !LLPipeline::sReflectionProbesEnabled || gTeleportDisplay || LLStartUp::getStartupState() < STATE_PRECACHE)
    {
        return;
    }

    // Part of a hacky workaround to fix #3331.
    // For some reason clearing shaders will cause mirrors to actually work.
    // There's likely some deeper state issue that needs to be resolved.
    // - Geenz 2025-02-25
    if (!mInitialized && LLStartUp::getStartupState() > STATE_PRECACHE)
    {
        LLViewerShaderMgr::instance()->clearShaderCache();
        LLViewerShaderMgr::instance()->setShaders();
        mInitialized = true;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LL_PROFILE_GPU_ZONE("hero manager update");
    llassert(!gCubeSnapshot); // assert a snapshot is not in progress
    if (LLAppViewer::instance()->logoutRequestSent())
    {
        return;
    }

    initReflectionMaps();

    static LLCachedControl<bool> render_hdr(gSavedSettings, "RenderHDREnabled", true);

    if (!mRenderTarget.isComplete())
    {
        U32 color_fmt = render_hdr ? GL_RGBA16F : GL_RGBA8;
        mRenderTarget.allocate(mProbeResolution, mProbeResolution, color_fmt, true);
    }

    if (mMipChain.empty())
    {
        U32 res = mProbeResolution;
        U32 count = (U32)(log2((F32)res) + 0.5f);

        mMipChain.resize(count);
        for (U32 i = 0; i < count; ++i)
        {
            mMipChain[i].allocate(res, res, render_hdr ? GL_RGBA16F : GL_RGBA8);
            res /= 2;
        }
    }

    llassert(mProbes[0] == mDefaultProbe);

    LLVector3 camera_pos = LLViewerCamera::instance().mOrigin;
    LLQuaternion cameraOrientation = LLViewerCamera::instance().getQuaternion();
    LLVector3    cameraDirection   = LLVector3::z_axis * cameraOrientation;

    S32 probeCount = (S32)mReflectionProbeCount;

    // --- Probe 0: System water mirror probe ---
    {
        F32 waterHeight = LLEnvironment::instance().getWaterHeight();
        LLVector3 waterPos(camera_pos.mV[VX], camera_pos.mV[VY], waterHeight);
        LLVector3 waterNormal(0.f, 0.f, 1.f);

        LLVector3 offset = camera_pos - waterPos;
        LLVector3 project = waterNormal * (offset * waterNormal);
        LLVector3 reject  = offset - project;
        LLVector3 point   = (reject - project) + waterPos;

        LLVector4a probe_pos;
        probe_pos.load3(point.mV);

        mProbes[0]->mOrigin = probe_pos;
        mProbes[0]->mRadius = 256.0f * 0.5f * sqrtf(2.0f);

        mCurrentClipPlane.setVec(waterPos, waterNormal);
        mIsPlanar = true;

        LLVector3 camFwd = LLViewerCamera::instance().getAtAxis();
        LLVector3 camUp  = LLViewerCamera::instance().getUpAxis();
        mPlanarLookDir = camFwd - 2.0f * (camFwd * waterNormal) * waterNormal;
        mPlanarUpDir   = camUp  - 2.0f * (camUp  * waterNormal) * waterNormal;
        mPlanarLookDir.normalize();
        mPlanarUpDir.normalize();
    }

    // --- User probes (indices 1..N-1) ---
    mActiveHeroes.clear();

    if (mHeroVOList.size() > 0 && probeCount > 1)
    {
        // Build sorted candidate list by distance
        struct HeroCandidate
        {
            LLPointer<LLVOVolume> vo;
            float distance;
        };
        std::vector<HeroCandidate> candidates;

        for (auto vo : mHeroVOList)
        {
            if (vo && !vo->isDead() && vo->mDrawable.notNull() && vo->isReflectionProbe() && vo->getReflectionProbeIsBox())
            {
                float distance = (camera_pos - vo->getPositionAgent()).magVec();

                if (distance > LLViewerCamera::instance().getFar())
                    continue;

                LLVector4a center;
                center.load3(vo->getPositionAgent().mV);
                LLVector4a size;
                size.load3(vo->getScale().mV);

                if (!LLViewerCamera::instance().AABBInFrustum(center, size))
                    continue;

                candidates.push_back({ vo, distance });
            }
            else
            {
                unregisterViewerObject(vo);
            }
        }

        // Sort by distance, nearest first
        std::sort(candidates.begin(), candidates.end(),
            [](const HeroCandidate& a, const HeroCandidate& b) { return a.distance < b.distance; });

        // Pick up to N-1 nearest user probes
        S32 maxUserProbes = probeCount - 1;
        for (S32 i = 0; i < (S32)candidates.size() && i < maxUserProbes; ++i)
        {
            mActiveHeroes.push_back(candidates[i].vo);
        }

        // Set up each user probe
        for (S32 i = 0; i < (S32)mActiveHeroes.size(); ++i)
        {
            S32 probeIdx = i + 1; // probe 0 is water
            LLVOVolume* hero = mActiveHeroes[i];

            LLVector3 hero_pos = hero->getPositionAgent();
            LLVector3 face_normal = LLVector3(0, 0, 1);
            face_normal *= hero->mDrawable->getWorldRotation();
            face_normal.normalize();

            bool isPlanar = hero->getScale().mV[VZ] < 0.02f;

            LLVector4a probe_pos;
            if (isPlanar)
            {
                // Planar mirrors render from reflected camera position
                LLVector3 offset = camera_pos - hero_pos;
                LLVector3 project = face_normal * (offset * face_normal);
                LLVector3 reject  = offset - project;
                LLVector3 point   = (reject - project) + hero_pos;
                probe_pos.load3(point.mV);
            }
            else
            {
                // Non-planar probes render from the hero object's center
                probe_pos.load3(hero_pos.mV);
            }

            mProbes[probeIdx]->mOrigin = probe_pos;
            mProbes[probeIdx]->mRadius = hero->getScale().magVec() * 0.5f;
            mProbes[probeIdx]->mViewerObject = hero;
        }
    }

    // Set backward compat mMirrorPosition/mMirrorNormal from nearest user probe (for clipPlane uniform)
    if (!mActiveHeroes.empty())
    {
        LLVOVolume* nearest = mActiveHeroes[0];
        LLVector3 face_normal = LLVector3(0, 0, 1);
        face_normal *= nearest->mDrawable->getWorldRotation();
        face_normal.normalize();
        mMirrorPosition = nearest->getPositionAgent();
        mMirrorNormal   = face_normal;
    }
    else
    {
        // Fall back to water plane
        F32 waterHeight = LLEnvironment::instance().getWaterHeight();
        mMirrorPosition = LLVector3(camera_pos.mV[VX], camera_pos.mV[VY], waterHeight);
        mMirrorNormal   = LLVector3(0.f, 0.f, 1.f);
    }

    mHeroProbeStrength = 1;
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
    if (!gTeleportDisplay && !gDisconnected && !LLAppViewer::instance()->logoutRequestSent())
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("hpmu - realtime");

        bool radiance_pass = gPipeline.mReflectionMapManager.isRadiancePass();

        gPipeline.mReflectionMapManager.mRadiancePass = true;
        mRenderingMirror = true;

        // Reset shadow tracking for all probes
        for (S32 i = 0; i < LL_MAX_HERO_PROBE_COUNT; ++i)
        {
            mHeroShadowsComplete[i] = false;
        }

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

        S32 activeCount = 1 + (S32)mActiveHeroes.size(); // water probe + user probes

        for (S32 probeIdx = 0; probeIdx < activeCount; ++probeIdx)
        {
            if (probeIdx >= (S32)mProbes.size() || mProbes[probeIdx].isNull() || mProbes[probeIdx]->mOccluded)
                continue;

            mCurrentRenderingProbeIdx = probeIdx;

            // Set per-probe active state and mirror plane for applySpecial()
            if (probeIdx == 0)
            {
                // Water probe — always planar
                F32 waterHeight = LLEnvironment::instance().getWaterHeight();
                LLVector3 camera_pos = LLViewerCamera::instance().mOrigin;

                LLVector3 clipPos(camera_pos.mV[VX], camera_pos.mV[VY], waterHeight + 0.0001f);
                LLVector3 waterNormal(0.f, 0.f, 1.f);

                mCurrentClipPlane.setVec(clipPos, waterNormal);
                mMirrorPosition = clipPos;
                mMirrorNormal   = waterNormal;
                mIsPlanar = true;

                LLVector3 camFwd = LLViewerCamera::instance().getAtAxis();
                LLVector3 camUp  = LLViewerCamera::instance().getUpAxis();
                mPlanarLookDir = camFwd - 2.0f * (camFwd * waterNormal) * waterNormal;
                mPlanarUpDir   = camUp  - 2.0f * (camUp  * waterNormal) * waterNormal;
                mPlanarLookDir.normalize();
                mPlanarUpDir.normalize();
            }
            else
            {
                // User probe
                LLVOVolume* hero = mActiveHeroes[probeIdx - 1];
                LLVector3 hero_pos = hero->getPositionAgent();
                LLVector3 face_normal = LLVector3(0, 0, 1);
                face_normal *= hero->mDrawable->getWorldRotation();
                face_normal.normalize();

                mCurrentClipPlane.setVec(hero_pos, face_normal);
                mMirrorPosition = hero_pos;
                mMirrorNormal   = face_normal;
                mIsPlanar = hero->getScale().mV[VZ] < 0.02f;

                if (mIsPlanar)
                {
                    LLVector3 camFwd = LLViewerCamera::instance().getAtAxis();
                    LLVector3 camUp  = LLViewerCamera::instance().getUpAxis();
                    mPlanarLookDir = camFwd - 2.0f * (camFwd * face_normal) * face_normal;
                    mPlanarUpDir   = camUp  - 2.0f * (camUp  * face_normal) * face_normal;
                    mPlanarLookDir.normalize();
                    mPlanarUpDir.normalize();
                }
            }

            bool dynamic = false;
            if (probeIdx == 0)
            {
                dynamic = sDetail() > 0;
            }
            else
            {
                dynamic = mActiveHeroes[probeIdx - 1]->getReflectionProbeIsDynamic() && sDetail() > 0;
            }

            if (mIsPlanar)
            {
                updateProbeFace(mProbes[probeIdx], 0, dynamic, near_clip);
            }
            else
            {
                // Non-planar probes capture full environment from object center.
                // Disable mirror clipping so mirrorClip() doesn't discard geometry.
                mRenderingMirror = false;
                for (U32 i = 0; i < 6; ++i)
                {
                    if ((gFrameCount % rate) == (i % rate))
                    {
                        updateProbeFace(mProbes[probeIdx], i, dynamic, near_clip);
                    }
                }
                mRenderingMirror = true;
            }

            generateRadiance(mProbes[probeIdx]);
        }

        mCurrentRenderingProbeIdx = -1;
        mRenderingMirror = false;

        gPipeline.mReflectionMapManager.mRadiancePass = radiance_pass;
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

    if (mIsPlanar)
    {
        probe->update(mRenderTarget.getWidth(), face, is_dynamic, near_clip,
                      true, mCurrentClipPlane, &mPlanarLookDir, &mPlanarUpDir);
    }
    else
    {
        probe->update(mRenderTarget.getWidth(), face, is_dynamic, near_clip);
    }

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
    LLVector4a oa;
    oa.set(0, 0, 0, 0);

    // Zero out the hero data
    memset(&mHeroData, 0, sizeof(mHeroData));

    S32 activeCount = 1 + (S32)mActiveHeroes.size(); // water + user probes
    mHeroData.heroProbeCount = activeCount;

    LLVector3 camera_pos = LLViewerCamera::instance().mOrigin;

    for (S32 pi = 0; pi < activeCount; ++pi)
    {
        if (pi >= (S32)mProbes.size() || mProbes[pi].isNull())
            continue;

        // heroParams: x=shape, y=cubeIndex
        mHeroData.heroParams[pi][1] = mProbes[pi]->mCubeIndex;

        if (pi == 0)
        {
            // Water probe — use reflected origin for sphere
            modelview.affineTransform(mProbes[pi]->mOrigin, oa);
            mHeroData.heroSphere[pi].set(oa.getF32ptr());
            mHeroData.heroSphere[pi].mV[3] = mProbes[pi]->mRadius;

            // Water probe — always planar
            mHeroData.heroParams[pi][0] = 2; // planar shape

            F32 waterHeight = LLEnvironment::instance().getWaterHeight();
            LLVector3 waterNormal(0.f, 0.f, 1.f);

            LLVector3 camFwd = LLViewerCamera::instance().getAtAxis();
            LLVector3 camUp  = LLViewerCamera::instance().getUpAxis();
            LLVector3 lookDir = camFwd - 2.0f * (camFwd * waterNormal) * waterNormal;
            LLVector3 upDir   = camUp  - 2.0f * (camUp  * waterNormal) * waterNormal;
            lookDir.normalize();
            upDir.normalize();

            LLVector3 reflRight = lookDir % upDir;
            reflRight.normalize();

            mHeroData.heroPlaneMatrix[pi].initRows(
                LLVector4(lookDir.mV[VX], -upDir.mV[VX], -reflRight.mV[VX], 0),
                LLVector4(lookDir.mV[VY], -upDir.mV[VY], -reflRight.mV[VY], 0),
                LLVector4(lookDir.mV[VZ], -upDir.mV[VZ], -reflRight.mV[VZ], 0),
                LLVector4(0, 0, 0, 1));

            // Clip plane set just above water to avoid Z-fighting with underwater fog.
            // Computed camera-relative to avoid float32 precision loss at large world coords.
            glm::mat4 mat = glm::make_mat4(gGLModelView);
            glm::mat3 R(mat);

            F32 clipHeight = waterHeight + 0.0001f;
            glm::vec3 relPos(0.f, 0.f, clipHeight - camera_pos.mV[VZ]);

            glm::vec3 enorm = glm::normalize(R * glm::vec3(0.f, 0.f, 1.f));
            glm::vec3 ep = R * relPos;
            mHeroData.heroClipPlane[pi].set(enorm.x, enorm.y, enorm.z, -glm::dot(ep, enorm));

            // Box transform in eye space — 256x256x0.01 volume, no world-space rotation.
            {
                glm::vec3 halfScale(128.f, 128.f, 0.1f);
                glm::mat4 boxMat = glm::inverse(glm::mat4(R) * glm::translate(glm::mat4(1.0f), relPos) * glm::scale(glm::mat4(1.0f), halfScale));
                mHeroData.heroBox[pi] = LLMatrix4(glm::value_ptr(boxMat));
            }
        }
        else
        {
            // User probe
            LLVOVolume* hero = mActiveHeroes[pi - 1];

            if (hero->getReflectionProbeIsBox())
            {
                LLVector3 s = hero->getScale().scaledVec(LLVector3(0.5f, 0.5f, 0.5f));
                mProbes[pi]->mRadius = s.magVec();
            }
            else
            {
                mProbes[pi]->mRadius = hero->getScale().mV[0] * 0.5f;
            }

            // Use hero object's actual position for sphere origin (not reflected camera position).
            // This matches develop where autoAdjustOrigin() reset the origin before updateUniforms().
            LLVector4a heroObjOrigin;
            heroObjOrigin.load3(hero->getPositionAgent().mV);
            modelview.affineTransform(heroObjOrigin, oa);
            mHeroData.heroSphere[pi].set(oa.getF32ptr());
            mHeroData.heroSphere[pi].mV[3] = mProbes[pi]->mRadius;

            mHeroData.heroParams[pi][0] = 0; // box shape
            if (!mProbes[pi]->getBox(mHeroData.heroBox[pi]))
            {
                mHeroData.heroParams[pi][0] = 1; // sphere shape
            }

            LLVector3 hero_pos = hero->getPositionAgent();
            LLVector3 face_normal = LLVector3(0, 0, 1);
            face_normal *= hero->mDrawable->getWorldRotation();
            face_normal.normalize();

            bool isPlanar = hero->getScale().mV[VZ] < 0.02f;

            if (isPlanar)
            {
                mHeroData.heroParams[pi][0] = 2; // planar shape

                LLVector3 camFwd = LLViewerCamera::instance().getAtAxis();
                LLVector3 camUp  = LLViewerCamera::instance().getUpAxis();
                LLVector3 lookDir = camFwd - 2.0f * (camFwd * face_normal) * face_normal;
                LLVector3 upDir   = camUp  - 2.0f * (camUp  * face_normal) * face_normal;
                lookDir.normalize();
                upDir.normalize();

                LLVector3 reflRight = lookDir % upDir;
                reflRight.normalize();

                mHeroData.heroPlaneMatrix[pi].initRows(
                    LLVector4(lookDir.mV[VX], -upDir.mV[VX], -reflRight.mV[VX], 0),
                    LLVector4(lookDir.mV[VY], -upDir.mV[VY], -reflRight.mV[VY], 0),
                    LLVector4(lookDir.mV[VZ], -upDir.mV[VZ], -reflRight.mV[VZ], 0),
                    LLVector4(0, 0, 0, 1));
            }

            // Clip plane in eye space
            glm::mat4 mat = glm::make_mat4(gGLModelView);
            glm::mat4 invtrans = glm::transpose(glm::inverse(mat));
            invtrans[0][3] = invtrans[1][3] = invtrans[2][3] = 0.f;

            glm::vec3 enorm = glm::normalize(glm::vec3(invtrans * glm::vec4(face_normal.mV[VX], face_normal.mV[VY], face_normal.mV[VZ], 0.f)));
            glm::vec3 ep = glm::vec3(mat * glm::vec4(hero_pos.mV[VX], hero_pos.mV[VY], hero_pos.mV[VZ], 1.f));

            mHeroData.heroClipPlane[pi].set(enorm.x, enorm.y, enorm.z, -glm::dot(ep, enorm));
        }
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
    S32 count = llclamp(LLPipeline::RenderMirrorCount, 1, LL_MAX_HERO_PROBE_COUNT);

    if ((mTexture.isNull() || mReflectionProbeCount != (U32)count || mReset) && LLPipeline::RenderMirrors)
    {
        if (mReset || mReflectionProbeCount != (U32)count)
        {
            cleanup();
        }

        mReset = false;
        mReflectionProbeCount = count;
        mProbeResolution      = gSavedSettings.getS32("RenderHeroProbeResolution");
        mMaxProbeLOD = log2f((F32)mProbeResolution) - 1.f; // number of mips - 1

        mTexture = new LLCubeMapArray();

        static LLCachedControl<bool> render_hdr(gSavedSettings, "RenderHDREnabled", true);

        // store count+2 cube maps (count probes + 2 scratch/staging)
        mTexture->allocate(mProbeResolution, 3, mReflectionProbeCount + 2, true, render_hdr);

        // Create all probes
        for (S32 i = 0; i < count; ++i)
        {
            LLPointer<LLReflectionMap> probe = new LLReflectionMap();
            probe->mCubeIndex = i;
            probe->mCubeArray = mTexture;
            probe->mDistance  = gSavedSettings.getF32("RenderHeroProbeDistance");
            probe->mRadius = 4096.f;
            probe->mProbeIndex = i;
            touch_default_probe(probe);
            mProbes.push_back(probe);
        }

        mDefaultProbe = mProbes[0];
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
