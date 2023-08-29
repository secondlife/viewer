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

extern BOOL gCubeSnapshot;
extern BOOL gTeleportDisplay;

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

// helper class to seed octree with probes
void LLHeroProbeManager::update()
{
    if (!LLPipeline::sReflectionProbesEnabled || gTeleportDisplay || LLStartUp::getStartupState() < STATE_PRECACHE)
    {
        return;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(!gCubeSnapshot); // assert a snapshot is not in progress
    if (LLAppViewer::instance()->logoutRequestSent())
    {
        return;
    }

    initReflectionMaps();

    if (!mRenderTarget.isComplete())
    {
        U32 color_fmt = GL_RGB16F;
        U32 targetRes = mProbeResolution * 4; // super sample
        mRenderTarget.allocate(targetRes, targetRes, color_fmt, true);
    }

    if (mMipChain.empty())
    {
        U32 res = mProbeResolution;
        U32 count = log2((F32)res) + 0.5f;
        
        mMipChain.resize(count);
        for (int i = 0; i < count; ++i)
        {
            mMipChain[i].allocate(res, res, GL_RGB16F);
            res /= 2;
        }
    }

    llassert(mProbes[0] == mDefaultProbe);
    
    LLVector4a probe_pos;
    LLVector3 camera_pos = LLViewerCamera::instance().mOrigin;
    {
        // Get the nearest hero.
        float distance = F32_MAX;
        
        if (mNearestHero.notNull())
        {
            distance = mNearestHero->mDistanceWRTCamera;
        }
        
        for (auto drawable : mHeroList)
        {
            if (drawable.notNull() && drawable != mNearestHero)
            {
                if (drawable->mDistanceWRTCamera < distance)
                {
                    mIsInTransition = true;
                    mNearestHero = drawable;
                }
            }
        }
    }
    
    if (mIsInTransition && mHeroProbeStrength > 0.f)
    {
        mHeroProbeStrength -= 0.05f;
    }
    else
    {
        if (mNearestHero.notNull())
            probe_pos.set(mNearestHero->mXform.getPosition().mV[0], mNearestHero->mXform.getPosition().mV[1], camera_pos.mV[2]);
        
        
        if (mHeroProbeStrength < 1.f)
            mHeroProbeStrength += 0.05f;
    }
    
    static LLCachedControl<S32> sDetail(gSavedSettings, "RenderHeroReflectionProbeDetail", -1);
    static LLCachedControl<S32> sLevel(gSavedSettings, "RenderHeroReflectionProbeLevel", 3);

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("hpmu - realtime");
        // Probe 0 is always our mirror probe.
        mProbes[0]->mOrigin = probe_pos;
        
        bool radiance_pass = gPipeline.mReflectionMapManager.isRadiancePass();
        
        gPipeline.mReflectionMapManager.mRadiancePass = true;
        
        for (U32 j = 0; j < mProbes.size(); j++)
        {
            for (U32 i = 0; i < 6; ++i)
            {
                updateProbeFace(mProbes[j], i);
            }
        }
        
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
void LLHeroProbeManager::updateProbeFace(LLReflectionMap* probe, U32 face)
{
    // hacky hot-swap of camera specific render targets
    gPipeline.mRT = &gPipeline.mAuxillaryRT;

    probe->update(mRenderTarget.getWidth(), face, true);
    
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

        LLRenderTarget* screen_rt = &gPipeline.mAuxillaryRT.screen;

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
        }


        S32 mips = log2((F32)mProbeResolution) + 0.5f;

        gReflectionMipProgram.bind();
        S32 diffuseChannel = gReflectionMipProgram.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, LLTexUnit::TT_TEXTURE);

        for (int i = 0; i < mMipChain.size(); ++i)
        {
            LL_PROFILE_GPU_ZONE("probe mip");
            mMipChain[i].bindTarget();
            if (i == 0)
            {
                gGL.getTexUnit(diffuseChannel)->bind(screen_rt);
            }
            else
            {
                gGL.getTexUnit(diffuseChannel)->bind(&(mMipChain[i - 1]));
            }

            
            gReflectionMipProgram.uniform1f(resScale, 1.f/(mProbeResolution*2));
            
            gPipeline.mScreenTriangleVB->setBuffer();
            gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            
            res /= 2;

            S32 mip = i - (mMipChain.size() - mips);

            if (mip >= 0)
            {
                LL_PROFILE_GPU_ZONE("probe mip copy");
                mTexture->bind(0);
                //glCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, 0, 0, probe->mCubeIndex * 6 + face, 0, 0, res, res);
                glCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, 0, 0, sourceIdx * 6 + face, 0, 0, res, res);
                //if (i == 0)
                //{
                    //glCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, 0, 0, probe->mCubeIndex * 6 + face, 0, 0, res, res);
                //}
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

    if (face == 5)
    {
        mMipChain[0].bindTarget();
        static LLStaticHashedString sSourceIdx("sourceIdx");

        {
            //generate radiance map (even if this is not the irradiance map, we need the mip chain for the irradiance map)
            gRadianceGenProgram.bind();
            mVertexBuffer->setBuffer();

            S32 channel = gRadianceGenProgram.enableTexture(LLShaderMgr::REFLECTION_PROBES, LLTexUnit::TT_CUBE_MAP_ARRAY);
            mTexture->bind(channel);
            gRadianceGenProgram.uniform1i(sSourceIdx, sourceIdx);
            gRadianceGenProgram.uniform1f(LLShaderMgr::REFLECTION_PROBE_MAX_LOD, mMaxProbeLOD);
            gRadianceGenProgram.uniform1f(LLShaderMgr::REFLECTION_PROBE_STRENGTH, mHeroProbeStrength);
            
            U32 res = mMipChain[0].getWidth();

            for (int i = 0; i < mMipChain.size(); ++i)
            {
                LL_PROFILE_GPU_ZONE("probe radiance gen");
                static LLStaticHashedString sMipLevel("mipLevel");
                static LLStaticHashedString sRoughness("roughness");
                static LLStaticHashedString sWidth("u_width");

                gRadianceGenProgram.uniform1f(sRoughness, (F32)i / (F32)(mMipChain.size() - 1));
                gRadianceGenProgram.uniform1f(sMipLevel, i);
                gRadianceGenProgram.uniform1i(sWidth, mProbeResolution);

                for (int cf = 0; cf < 6; ++cf)
                { // for each cube face
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

            gRadianceGenProgram.unbind();
        }

        mMipChain[0].flush();
    }
}

void LLHeroProbeManager::updateUniforms()
{
    if (!LLPipeline::sReflectionProbesEnabled)
    {
        return;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    
    struct HeroProbeData
    {
        LLVector4 heroPosition[1];
        GLint heroProbeCount = 1;
    };
    
    HeroProbeData hpd;
    
    LLMatrix4a modelview;
    modelview.loadu(gGLModelView);
    LLVector4a oa; // scratch space for transformed origin
    oa.set(0, 0, 0, 0);
    hpd.heroProbeCount = 1;
    modelview.affineTransform(mProbes[0]->mOrigin, oa);
    hpd.heroPosition[0].set(oa.getF32ptr());

    //copy rpd into uniform buffer object
    if (mUBO == 0)
    {
        glGenBuffers(1, &mUBO);
    }

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("rmmsu - update buffer");
        glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(HeroProbeData), &hpd, GL_STREAM_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    
#if 0
    if (!gCubeSnapshot)
    {
        for (auto& probe : mProbes)
        {
            LLViewerObject* vobj = probe->mViewerObject;
            if (vobj)
            {
                F32 time = (F32)gFrameTimeSeconds - probe->mLastUpdateTime;
                vobj->setDebugText(llformat("%d/%d/%d/%.1f - %.1f/%.1f", probe->mCubeIndex, probe->mProbeIndex, (U32) probe->mNeighbors.size(), probe->mMinDepth, probe->mMaxDepth, time), time > 1.f ? LLColor4::white : LLColor4::green);
            }
        }
    }
#endif
}

void LLHeroProbeManager::setUniforms()
{
    if (!LLPipeline::sReflectionProbesEnabled)
    {
        return;
    }

    if (mUBO == 0)
    { 
        updateUniforms();
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUBO);
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
    U32 count = LL_MAX_REFLECTION_PROBE_COUNT;

    if (mTexture.isNull() || mReflectionProbeCount != count || mReset)
    {
        mReset = false;
        mReflectionProbeCount = count;
        mProbeResolution = nhpo2(1024);
        mMaxProbeLOD = log2f(mProbeResolution) - 1.f; // number of mips - 1

        mTexture = new LLCubeMapArray();

        // store mReflectionProbeCount+2 cube maps, final two cube maps are used for render target and radiance map generation source)
        mTexture->allocate(mProbeResolution, 3, mReflectionProbeCount + 2);

        mIrradianceMaps = new LLCubeMapArray();
        mIrradianceMaps->allocate(LL_IRRADIANCE_MAP_RESOLUTION, 3, mReflectionProbeCount, FALSE);

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
        mDefaultProbe->mDistance = 12.f;
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
    mHeroRenderTarget.release();

    mMipChain.clear();

    mTexture = nullptr;

    mProbes.clear();

    mReflectionMaps.clear();
    
    mDefaultProbe = nullptr;
    mUpdatingProbe = nullptr;

    glDeleteBuffers(1, &mUBO);
    mUBO = 0;
    
    mHeroList.clear();
    mNearestHero = nullptr;
}

void LLHeroProbeManager::doOcclusion()
{
    LLVector4a eye;
    eye.load3(LLViewerCamera::instance().getOrigin().mV);

    for (auto& probe : mProbes)
    {
        if (probe != nullptr && probe != mDefaultProbe)
        {
            probe->doOcclusion(eye);
        }
    }
}

void LLHeroProbeManager::registerHeroDrawable(LLDrawable* drawablep)
{
    if (mHeroList.find(drawablep) == mHeroList.end())
    {
        mHeroList.insert(drawablep);
    }
}

void LLHeroProbeManager::unregisterHeroDrawable(LLDrawable *drawablep)
{
    if (mHeroList.find(drawablep) != mHeroList.end())
    {
        mHeroList.erase(drawablep);
    }
}
