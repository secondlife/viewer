/**
 * @file llreflectionmapmanager.cpp
 * @brief LLReflectionMapManager class implementation
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

#include "llreflectionmapmanager.h"
#include "llviewercamera.h"
#include "llspatialpartition.h"
#include "llviewerregion.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llviewercontrol.h"
#include "llenvironment.h"
#include "llstartup.h"

extern BOOL gCubeSnapshot;
extern BOOL gTeleportDisplay;

LLReflectionMapManager::LLReflectionMapManager()
{
    for (int i = 1; i < LL_MAX_REFLECTION_PROBE_COUNT; ++i)
    {
        mCubeFree[i] = true;
    }

    // cube index 0 is reserved for the fallback probe
    mCubeFree[0] = false;
}

struct CompareReflectionMapDistance
{

};


struct CompareProbeDistance
{
    bool operator()(const LLPointer<LLReflectionMap>& lhs, const LLPointer<LLReflectionMap>& rhs)
    {
        return lhs->mDistance < rhs->mDistance;
    }
};

// helper class to seed octree with probes
void LLReflectionMapManager::update()
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

    // =============== TODO -- move to an init function  =================
    initReflectionMaps();

    if (!mRenderTarget.isComplete())
    {
        U32 color_fmt = GL_RGB16F;
        const bool use_depth_buffer = true;
        const bool use_stencil_buffer = false;
        U32 targetRes = LL_REFLECTION_PROBE_RESOLUTION * 2; // super sample
        mRenderTarget.allocate(targetRes, targetRes, color_fmt, use_depth_buffer, use_stencil_buffer, LLTexUnit::TT_RECT_TEXTURE);
    }

    if (mMipChain.empty())
    {
        U32 res = LL_REFLECTION_PROBE_RESOLUTION;
        U32 count = log2((F32)res) + 0.5f;
        
        mMipChain.resize(count);
        for (int i = 0; i < count; ++i)
        {
            mMipChain[i].allocate(res, res, GL_RGBA16F, false, false, LLTexUnit::TT_RECT_TEXTURE);
            res /= 2;
        }
    }


    if (mDefaultProbe.isNull())
    {
        mDefaultProbe = addProbe();
        mDefaultProbe->mDistance = -4096.f; // hack to make sure the default probe is always first in sort order
        mDefaultProbe->mRadius = 4096.f;
    }

    mDefaultProbe->mOrigin.load3(LLViewerCamera::getInstance()->getOrigin().mV);
    
    LLVector4a camera_pos;
    camera_pos.load3(LLViewerCamera::instance().getOrigin().mV);

    // process kill list
    for (auto& probe : mKillList)
    {
        auto const & iter = std::find(mProbes.begin(), mProbes.end(), probe);
        if (iter != mProbes.end())
        {
            deleteProbe(iter - mProbes.begin());
        }
    }

    mKillList.clear();
    
    // process create list
    for (auto& probe : mCreateList)
    {
        mProbes.push_back(probe);
    }

    mCreateList.clear();

    if (mProbes.empty())
    {
        return;
    }

    bool did_update = false;
    
    static LLCachedControl<S32> sDetail(gSavedSettings, "RenderReflectionProbeDetail", -1);
    bool realtime = sDetail >= (S32)LLReflectionMapManager::DetailLevel::REALTIME;
    
    LLReflectionMap* closestDynamic = nullptr;

    LLReflectionMap* oldestProbe = nullptr;

    if (mUpdatingProbe != nullptr)
    {
        did_update = true;
        doProbeUpdate();
    }

    for (int i = 0; i < mProbes.size(); ++i)
    {
        LLReflectionMap* probe = mProbes[i];
        if (probe->getNumRefs() == 1)
        { // no references held outside manager, delete this probe
            deleteProbe(i);
            --i;
            continue;
        }
        
        probe->mProbeIndex = i;

        LLVector4a d;
        
        if (!did_update && 
            i < mReflectionProbeCount &&
            (oldestProbe == nullptr || probe->mLastUpdateTime < oldestProbe->mLastUpdateTime))
        {
            oldestProbe = probe;
        }

        if (realtime && 
            closestDynamic == nullptr && 
            probe->mCubeIndex != -1 &&
            probe->getIsDynamic())
        {
            closestDynamic = probe;
        }

        if (probe != mDefaultProbe)
        {
            d.setSub(camera_pos, probe->mOrigin);
            probe->mDistance = d.getLength3().getF32() - probe->mRadius;
        }
    }

    if (realtime && closestDynamic != nullptr)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("rmmu - realtime");
        // update the closest dynamic probe realtime
        closestDynamic->autoAdjustOrigin();
        for (U32 i = 0; i < 6; ++i)
        {
            updateProbeFace(closestDynamic, i);
        }
    }

    // switch to updating the next oldest probe
    if (!did_update && oldestProbe != nullptr)
    {
        LLReflectionMap* probe = oldestProbe;
        if (probe->mCubeIndex == -1)
        {
            probe->mCubeArray = mTexture;

            probe->mCubeIndex = probe == mDefaultProbe ? 0 : allocateCubeIndex();
        }

        probe->autoAdjustOrigin();

        mUpdatingProbe = probe;
        doProbeUpdate();
    }

    // update distance to camera for all probes
    std::sort(mProbes.begin(), mProbes.end(), CompareProbeDistance());
}

LLReflectionMap* LLReflectionMapManager::addProbe(LLSpatialGroup* group)
{
    LLReflectionMap* probe = new LLReflectionMap();
    probe->mGroup = group;

    if (group)
    {
        probe->mOrigin = group->getOctreeNode()->getCenter();
    }

    if (gCubeSnapshot)
    { //snapshot is in progress, mProbes is being iterated over, defer insertion until next update
        mCreateList.push_back(probe);
    }
    else
    {
        mProbes.push_back(probe);
    }

    return probe;
}

void LLReflectionMapManager::getReflectionMaps(std::vector<LLReflectionMap*>& maps)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;

    U32 count = 0;
    U32 lastIdx = 0;
    for (U32 i = 0; count < maps.size() && i < mProbes.size(); ++i)
    {
        mProbes[i]->mLastBindTime = gFrameTimeSeconds; // something wants to use this probe, indicate it's been requested
        if (mProbes[i]->mCubeIndex != -1)
        {
            mProbes[i]->mProbeIndex = count;
            maps[count++] = mProbes[i];
        }
        else
        {
            mProbes[i]->mProbeIndex = -1;
        }
        lastIdx = i;
    }

    // set remaining probe indices to -1
    for (U32 i = lastIdx+1; i < mProbes.size(); ++i)
    {
        mProbes[i]->mProbeIndex = -1;
    }

    // null terminate list
    if (count < maps.size())
    {
        maps[count] = nullptr;
    }
}

LLReflectionMap* LLReflectionMapManager::registerSpatialGroup(LLSpatialGroup* group)
{
#if 1
    if (group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_VOLUME)
    {
        OctreeNode* node = group->getOctreeNode();
        F32 size = node->getSize().getF32ptr()[0];
        if (size >= 15.f && size <= 17.f)
        {
            return addProbe(group);
        }
    }
#endif

#if 0
    if (group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_TERRAIN)
    {
        OctreeNode* node = group->getOctreeNode();
        F32 size = node->getSize().getF32ptr()[0];
        if (size >= 15.f && size <= 17.f)
        {
            return addProbe(group);
        }
    }
#endif
    return nullptr;
}

LLReflectionMap* LLReflectionMapManager::registerViewerObject(LLViewerObject* vobj)
{
    llassert(vobj != nullptr);

    LLReflectionMap* probe = new LLReflectionMap();
    probe->mViewerObject = vobj;
    probe->mOrigin.load3(vobj->getPositionAgent().mV);

    if (gCubeSnapshot)
    { //snapshot is in progress, mProbes is being iterated over, defer insertion until next update
        mCreateList.push_back(probe);
    }
    else
    {
        mProbes.push_back(probe);
    }

    return probe;
}


S32 LLReflectionMapManager::allocateCubeIndex()
{
    for (int i = 0; i < mReflectionProbeCount; ++i)
    {
        if (mCubeFree[i])
        {
            mCubeFree[i] = false;
            return i;
        }
    }

    // no cubemaps free, steal one from the back of the probe list
    for (int i = mProbes.size() - 1; i >= mReflectionProbeCount; --i)
    {
        if (mProbes[i]->mCubeIndex != -1)
        {
            S32 ret = mProbes[i]->mCubeIndex;
            mProbes[i]->mCubeIndex = -1;
            mProbes[i]->mCubeArray = nullptr;
            return ret;
        }
    }

    llassert(false); // should never fail to allocate, something is probably wrong with mCubeFree
    return -1;
}

void LLReflectionMapManager::deleteProbe(U32 i)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LLReflectionMap* probe = mProbes[i];

    llassert(probe != mDefaultProbe);

    if (probe->mCubeIndex != -1)
    { // mark the cube index used by this probe as being free
        mCubeFree[probe->mCubeIndex] = true;
    }
    if (mUpdatingProbe == probe)
    {
        mUpdatingProbe = nullptr;
        mUpdatingFace = 0;
    }

    // remove from any Neighbors lists
    for (auto& other : probe->mNeighbors)
    {
        auto const & iter = std::find(other->mNeighbors.begin(), other->mNeighbors.end(), probe);
        llassert(iter != other->mNeighbors.end());
        other->mNeighbors.erase(iter);
    }

    mProbes.erase(mProbes.begin() + i);
}


void LLReflectionMapManager::doProbeUpdate()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(mUpdatingProbe != nullptr);

    updateProbeFace(mUpdatingProbe, mUpdatingFace);
    
    if (++mUpdatingFace == 6)
    {
        updateNeighbors(mUpdatingProbe);
        mUpdatingProbe = nullptr;
        mUpdatingFace = 0;
    }
}

void LLReflectionMapManager::updateProbeFace(LLReflectionMap* probe, U32 face)
{
    // hacky hot-swap of camera specific render targets
    gPipeline.mRT = &gPipeline.mAuxillaryRT;
    probe->update(mRenderTarget.getWidth(), face);
    gPipeline.mRT = &gPipeline.mMainRT;

    S32 targetIdx = mReflectionProbeCount;

    if (probe != mUpdatingProbe)
    { // this is the "realtime" probe that's updating every frame, use the secondary scratch space channel
        targetIdx += 1;
    }

    gGL.setColorMask(true, true);

    // downsample to placeholder map
    {
        LLGLDepthTest depth(GL_FALSE, GL_FALSE);
        LLGLDisable cull(GL_CULL_FACE);

        gReflectionMipProgram.bind();

        gGL.matrixMode(gGL.MM_MODELVIEW);
        gGL.pushMatrix();
        gGL.loadIdentity();

        gGL.matrixMode(gGL.MM_PROJECTION);
        gGL.pushMatrix();
        gGL.loadIdentity();

        gGL.flush();
        U32 res = LL_REFLECTION_PROBE_RESOLUTION * 2;

        S32 mips = log2((F32)LL_REFLECTION_PROBE_RESOLUTION) + 0.5f;

        S32 diffuseChannel = gReflectionMipProgram.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, LLTexUnit::TT_RECT_TEXTURE);
        S32 depthChannel = gReflectionMipProgram.enableTexture(LLShaderMgr::DEFERRED_DEPTH, LLTexUnit::TT_RECT_TEXTURE);

        LLRenderTarget* screen_rt = &gPipeline.mAuxillaryRT.screen;
        LLRenderTarget* depth_rt = &gPipeline.mAuxillaryRT.deferredScreen;

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

            gGL.getTexUnit(depthChannel)->bind(depth_rt, true);

            static LLStaticHashedString resScale("resScale");
            static LLStaticHashedString znear("znear");
            static LLStaticHashedString zfar("zfar");
            
            gReflectionMipProgram.uniform1f(resScale, (F32) (1 << i));
            gReflectionMipProgram.uniform1f(znear, probe->getNearClip());
            gReflectionMipProgram.uniform1f(zfar, MAX_FAR_CLIP);

            gGL.begin(gGL.QUADS);

            gGL.texCoord2f(0, 0);
            gGL.vertex2f(-1, -1);

            gGL.texCoord2f(res, 0);
            gGL.vertex2f(1, -1);

            gGL.texCoord2f(res, res);
            gGL.vertex2f(1, 1);

            gGL.texCoord2f(0, res);
            gGL.vertex2f(-1, 1);
            gGL.end();
            gGL.flush();

            res /= 2;

            S32 mip = i - (mMipChain.size() - mips);

            if (mip >= 0)
            {
                LL_PROFILE_GPU_ZONE("probe mip copy");
                mTexture->bind(0);
                //glCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, 0, 0, probe->mCubeIndex * 6 + face, 0, 0, res, res);
                glCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, 0, 0, targetIdx * 6 + face, 0, 0, res, res);
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

        gGL.getTexUnit(diffuseChannel)->unbind(LLTexUnit::TT_RECT_TEXTURE);
        gGL.getTexUnit(depthChannel)->unbind(LLTexUnit::TT_RECT_TEXTURE);
        gReflectionMipProgram.unbind();
    }

    if (face == 5)
    {
        //generate radiance map
        gRadianceGenProgram.bind();
        mVertexBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX);

        S32 channel = gRadianceGenProgram.enableTexture(LLShaderMgr::REFLECTION_PROBES, LLTexUnit::TT_CUBE_MAP_ARRAY);
        mTexture->bind(channel);
        static LLStaticHashedString sSourceIdx("sourceIdx");
        gRadianceGenProgram.uniform1i(sSourceIdx, targetIdx);

        mMipChain[0].bindTarget();
        U32 res = mMipChain[0].getWidth();

        for (int i = 0; i < mMipChain.size(); ++i)
        {
            LL_PROFILE_GPU_ZONE("probe radiance gen");
            static LLStaticHashedString sMipLevel("mipLevel");
            static LLStaticHashedString sRoughness("roughness");
            static LLStaticHashedString sWidth("u_width");

            gRadianceGenProgram.uniform1f(sRoughness, (F32)i / (F32)(mMipChain.size() - 1));
            gRadianceGenProgram.uniform1f(sMipLevel, i);
            gRadianceGenProgram.uniform1i(sWidth, mMipChain[i].getWidth());

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

        //generate irradiance map
        gIrradianceGenProgram.bind();
        channel = gIrradianceGenProgram.enableTexture(LLShaderMgr::REFLECTION_PROBES, LLTexUnit::TT_CUBE_MAP_ARRAY);
        mTexture->bind(channel);

        gIrradianceGenProgram.uniform1i(sSourceIdx, targetIdx);
        mVertexBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX);
        int start_mip = 0;
        // find the mip target to start with based on irradiance map resolution
        for (start_mip = 0; start_mip < mMipChain.size(); ++start_mip)
        {
            if (mMipChain[start_mip].getWidth() == LL_IRRADIANCE_MAP_RESOLUTION)
            {
                break;
            }
        }

        //for (int i = start_mip; i < mMipChain.size(); ++i)
        {
            int i = start_mip;
            LL_PROFILE_GPU_ZONE("probe irradiance gen");
            glViewport(0, 0, mMipChain[i].getWidth(), mMipChain[i].getHeight());
            for (int cf = 0; cf < 6; ++cf)
            { // for each cube face
                LLCoordFrame frame;
                frame.lookAt(LLVector3(0, 0, 0), LLCubeMapArray::sClipToCubeLookVecs[cf], LLCubeMapArray::sClipToCubeUpVecs[cf]);

                F32 mat[16];
                frame.getOpenGLRotation(mat);
                gGL.loadMatrix(mat);

                mVertexBuffer->drawArrays(gGL.TRIANGLE_STRIP, 0, 4);

                S32 res = mMipChain[i].getWidth();
                mIrradianceMaps->bind(channel);
                glCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, i - start_mip, 0, 0, probe->mCubeIndex * 6 + cf, 0, 0, res, res);
                mTexture->bind(channel);
            }
        }

        mMipChain[0].flush();

        gIrradianceGenProgram.unbind();
    }
}

void LLReflectionMapManager::rebuild()
{
    for (auto& probe : mProbes)
    {
        probe->mLastUpdateTime = 0.f;
    }
}

void LLReflectionMapManager::shift(const LLVector4a& offset)
{
    for (auto& probe : mProbes)
    {
        probe->mOrigin.add(offset);
    }
}

void LLReflectionMapManager::updateNeighbors(LLReflectionMap* probe)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    if (mDefaultProbe == probe)
    {
        return;
    }

    //remove from existing neighbors
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("rmmun - clear");
    
        for (auto& other : probe->mNeighbors)
        {
            auto const & iter = std::find(other->mNeighbors.begin(), other->mNeighbors.end(), probe);
            llassert(iter != other->mNeighbors.end()); // <--- bug davep if this ever happens, something broke badly
            other->mNeighbors.erase(iter);
        }

        probe->mNeighbors.clear();
    }

    // search for new neighbors
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("rmmun - search");
        for (auto& other : mProbes)
        {
            if (other != mDefaultProbe && other != probe)
            {
                if (probe->intersects(other))
                {
                    probe->mNeighbors.push_back(other);
                    other->mNeighbors.push_back(probe);
                }
            }
        }
    }
}

void LLReflectionMapManager::updateUniforms()
{
    if (!LLPipeline::sReflectionProbesEnabled)
    {
        return;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;

    // structure for packing uniform buffer object
    // see class3/deferred/reflectionProbeF.glsl
    struct ReflectionProbeData
    {
        LLMatrix4 refBox[LL_MAX_REFLECTION_PROBE_COUNT]; // object bounding box as needed
        LLVector4 refSphere[LL_MAX_REFLECTION_PROBE_COUNT]; //origin and radius of refmaps in clip space
        LLVector4 refParams[LL_MAX_REFLECTION_PROBE_COUNT]; //extra parameters (currently only ambiance)
        GLint refIndex[LL_MAX_REFLECTION_PROBE_COUNT][4];
        GLint refNeighbor[4096];
        GLint refmapCount;
    };

    mReflectionMaps.resize(mReflectionProbeCount);
    getReflectionMaps(mReflectionMaps);

    ReflectionProbeData rpd;

    // load modelview matrix into matrix 4a
    LLMatrix4a modelview;
    modelview.loadu(gGLModelView);
    LLVector4a oa; // scratch space for transformed origin

    S32 count = 0;
    U32 nc = 0; // neighbor "cursor" - index into refNeighbor to start writing the next probe's list of neighbors

    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t psky = environment.getCurrentSky();

    F32 minimum_ambiance = psky->getReflectionProbeAmbiance();

    for (auto* refmap : mReflectionMaps)
    {
        if (refmap == nullptr)
        {
            break;
        }

        llassert(refmap->mProbeIndex == count);
        llassert(mReflectionMaps[refmap->mProbeIndex] == refmap);

        llassert(refmap->mCubeIndex >= 0); // should always be  true, if not, getReflectionMaps is bugged

        {
            //LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("rmmsu - refSphere");

            modelview.affineTransform(refmap->mOrigin, oa);
            rpd.refSphere[count].set(oa.getF32ptr());
            rpd.refSphere[count].mV[3] = refmap->mRadius;
        }

        rpd.refIndex[count][0] = refmap->mCubeIndex;
        llassert(nc % 4 == 0);
        rpd.refIndex[count][1] = nc / 4;
        rpd.refIndex[count][3] = refmap->mPriority;

        // for objects that are reflection probes, use the volume as the influence volume of the probe
        // only possibile influence volumes are boxes and spheres, so detect boxes and treat everything else as spheres
        if (refmap->getBox(rpd.refBox[count]))
        { // negate priority to indicate this probe has a box influence volume
            rpd.refIndex[count][3] = -rpd.refIndex[count][3];
        }

        rpd.refParams[count].set(llmax(minimum_ambiance, refmap->getAmbiance()), 0.f, 0.f, 0.f);

        S32 ni = nc; // neighbor ("index") - index into refNeighbor to write indices for current reflection probe's neighbors
        {
            //LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("rmmsu - refNeighbors");
            //pack neghbor list
            for (auto& neighbor : refmap->mNeighbors)
            {
                if (ni >= 4096)
                { // out of space
                    break;
                }

                GLint idx = neighbor->mProbeIndex;
                if (idx == -1)
                {
                    continue;
                }

                // this neighbor may be sampled
                rpd.refNeighbor[ni++] = idx;
            }
        }

        if (nc == ni)
        {
            //no neighbors, tag as empty
            rpd.refIndex[count][1] = -1;
        }
        else
        {
            rpd.refIndex[count][2] = ni - nc;

            // move the cursor forward
            nc = ni;
            if (nc % 4 != 0)
            { // jump to next power of 4 for compatibility with ivec4
                nc += 4 - (nc % 4);
            }
        }


        count++;
    }

    rpd.refmapCount = count;

    //copy rpd into uniform buffer object
    if (mUBO == 0)
    {
        glGenBuffers(1, &mUBO);
    }

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("rmmsu - update buffer");
        glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(ReflectionProbeData), &rpd, GL_STREAM_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void LLReflectionMapManager::setUniforms()
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


void renderReflectionProbe(LLReflectionMap* probe)
{
    F32* po = probe->mOrigin.getF32ptr();

    //draw orange line from probe to neighbors
    gGL.flush();
    gGL.diffuseColor4f(1, 0.5f, 0, 1);
    gGL.begin(gGL.LINES);
    for (auto& neighbor : probe->mNeighbors)
    {
        gGL.vertex3fv(po);
        gGL.vertex3fv(neighbor->mOrigin.getF32ptr());
    }
    gGL.end();
    gGL.flush();

#if 0
    LLSpatialGroup* group = probe->mGroup;
    if (group)
    { // draw lines from corners of object aabb to reflection probe

        const LLVector4a* bounds = group->getBounds();
        LLVector4a o = bounds[0];

        gGL.flush();
        gGL.diffuseColor4f(0, 0, 1, 1);
        F32* c = o.getF32ptr();

        const F32* bc = bounds[0].getF32ptr();
        const F32* bs = bounds[1].getF32ptr();

        // daaw blue lines from corners to center of node
        gGL.begin(gGL.LINES);
        gGL.vertex3fv(c);
        gGL.vertex3f(bc[0] + bs[0], bc[1] + bs[1], bc[2] + bs[2]);
        gGL.vertex3fv(c);
        gGL.vertex3f(bc[0] - bs[0], bc[1] + bs[1], bc[2] + bs[2]);
        gGL.vertex3fv(c);
        gGL.vertex3f(bc[0] + bs[0], bc[1] - bs[1], bc[2] + bs[2]);
        gGL.vertex3fv(c);
        gGL.vertex3f(bc[0] - bs[0], bc[1] - bs[1], bc[2] + bs[2]);

        gGL.vertex3fv(c);
        gGL.vertex3f(bc[0] + bs[0], bc[1] + bs[1], bc[2] - bs[2]);
        gGL.vertex3fv(c);
        gGL.vertex3f(bc[0] - bs[0], bc[1] + bs[1], bc[2] - bs[2]);
        gGL.vertex3fv(c);
        gGL.vertex3f(bc[0] + bs[0], bc[1] - bs[1], bc[2] - bs[2]);
        gGL.vertex3fv(c);
        gGL.vertex3f(bc[0] - bs[0], bc[1] - bs[1], bc[2] - bs[2]);
        gGL.end();

        //draw yellow line from center of node to reflection probe origin
        gGL.flush();
        gGL.diffuseColor4f(1, 1, 0, 1);
        gGL.begin(gGL.LINES);
        gGL.vertex3fv(c);
        gGL.vertex3fv(po);
        gGL.end();
        gGL.flush();
    }
#endif
}

void LLReflectionMapManager::renderDebug()
{
    gDebugProgram.bind();

    for (auto& probe : mProbes)
    {
        renderReflectionProbe(probe);
    }

    gDebugProgram.unbind();
}

void LLReflectionMapManager::initReflectionMaps()
{
    if (mTexture.isNull())
    {
        mReflectionProbeCount = llclamp(gSavedSettings.getS32("RenderReflectionProbeCount"), 1, LL_MAX_REFLECTION_PROBE_COUNT);

        mTexture = new LLCubeMapArray();

        // store mReflectionProbeCount+2 cube maps, final two cube maps are used for render target and radiance map generation source)
        mTexture->allocate(LL_REFLECTION_PROBE_RESOLUTION, 4, mReflectionProbeCount + 2);

        mIrradianceMaps = new LLCubeMapArray();
        mIrradianceMaps->allocate(LL_IRRADIANCE_MAP_RESOLUTION, 4, mReflectionProbeCount, FALSE);
    }

    if (mVertexBuffer.isNull())
    {
        U32 mask = LLVertexBuffer::MAP_VERTEX;
        LLPointer<LLVertexBuffer> buff = new LLVertexBuffer(mask, GL_STATIC_DRAW);
        buff->allocateBuffer(4, 0, TRUE);

        LLStrider<LLVector3> v;
        
        buff->getVertexStrider(v);
        
        v[0] = LLVector3(-1, -1, -1);
        v[1] = LLVector3(1, -1, -1);
        v[2] = LLVector3(-1, 1, -1);
        v[3] = LLVector3(1, 1, -1);

        buff->flush();

        mVertexBuffer = buff;
    }
}
