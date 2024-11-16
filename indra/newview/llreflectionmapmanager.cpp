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

#include <vector>

#include "llviewercamera.h"
#include "llspatialpartition.h"
#include "llviewerregion.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llviewercontrol.h"
#include "llenvironment.h"
#include "llstartup.h"
#include "llviewermenufile.h"
#include "llnotificationsutil.h"

#if LL_WINDOWS
#pragma warning (push)
#pragma warning (disable : 4702) // compiler complains unreachable code
#endif
#define TINYEXR_USE_MINIZ 0
#include "zlib.h"
#define TINYEXR_IMPLEMENTATION
#include "tinyexr/tinyexr.h"
#if LL_WINDOWS
#pragma warning (pop)
#endif

LLPointer<LLImageGL> gEXRImage;

void load_exr(const std::string& filename)
{
    // reset reflection maps when previewing a new HDRI
    gPipeline.mReflectionMapManager.reset();
    gPipeline.mReflectionMapManager.initReflectionMaps();

    float* out; // width * height * RGBA
    int width;
    int height;
    const char* err = NULL; // or nullptr in C++11

    int ret =  LoadEXRWithLayer(&out, &width, &height, filename.c_str(), /* layername */ nullptr, &err);
    if (ret == TINYEXR_SUCCESS)
    {
        U32 texName = 0;
        LLImageGL::generateTextures(1, &texName);

        gEXRImage = new LLImageGL(texName, 4, GL_TEXTURE_2D, GL_RGB16F, GL_RGB16F, GL_FLOAT, LLTexUnit::TAM_CLAMP);
        gEXRImage->setHasMipMaps(true);
        gEXRImage->setUseMipMaps(true);
        gEXRImage->setFilteringOption(LLTexUnit::TFO_TRILINEAR);

        gGL.getTexUnit(0)->bind(gEXRImage);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGBA, GL_FLOAT, out);

        LLImageGLMemory::alloc_tex_image(width, height, GL_RGB16F, 1);

        free(out); // release memory of image data

        glGenerateMipmap(GL_TEXTURE_2D);

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

    }
    else
    {
        LLSD notif_args;
        notif_args["WHAT"] = filename;
        notif_args["REASON"] = "Unknown";
        if (err)
        {
            notif_args["REASON"] = std::string(err);
            FreeEXRErrorMessage(err); // release memory of error message.
        }
        LLNotificationsUtil::add("CannotLoad", notif_args);
    }
}

void hdri_preview()
{
    LLFilePickerReplyThread::startPicker(
        [](const std::vector<std::string>& filenames, LLFilePicker::ELoadFilter load_filter, LLFilePicker::ESaveFilter save_filter)
        {
            if (LLAppViewer::instance()->quitRequested())
            {
                return;
            }
            if (filenames.size() > 0)
            {
                load_exr(filenames[0]);
            }
        },
        LLFilePicker::FFLOAD_HDRI,
        true);
}

extern bool gCubeSnapshot;
extern bool gTeleportDisplay;

static U32 sUpdateCount = 0;

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

LLReflectionMapManager::LLReflectionMapManager()
{
    initCubeFree();
}

void LLReflectionMapManager::initCubeFree()
{
    // start at 1 because index 0 is reserved for mDefaultProbe
    for (int i = 1; i < LL_MAX_REFLECTION_PROBE_COUNT; ++i)
    {
        mCubeFree.push_back(i);
    }
}

struct CompareProbeDistance
{
    LLReflectionMap* mDefaultProbe;

    bool operator()(const LLPointer<LLReflectionMap>& lhs, const LLPointer<LLReflectionMap>& rhs)
    {
        return lhs->mDistance < rhs->mDistance;
    }
};

static F32 update_score(LLReflectionMap* p)
{
    return gFrameTimeSeconds - p->mLastUpdateTime  - p->mDistance*0.1f;
}

// return true if a is higher priority for an update than b
static bool check_priority(LLReflectionMap* a, LLReflectionMap* b)
{
    if (a->mCubeIndex == -1)
    { // not a candidate for updating
        return false;
    }
    else if (b->mCubeIndex == -1)
    { // b is not a candidate for updating, a is higher priority by default
        return true;
    }
    else if (!a->mComplete && !b->mComplete)
    { //neither probe is complete, use distance
        return a->mDistance < b->mDistance;
    }
    else if (a->mComplete && b->mComplete)
    { //both probes are complete, use update_score metric
        return update_score(a) > update_score(b);
    }

    // a or b is not complete,
    if (sUpdateCount % 3 == 0)
    { // every third update, allow complete probes to cut in line in front of non-complete probes to avoid spammy probe generators from deadlocking scheduler (SL-20258))
        return !b->mComplete;
    }

    // prioritize incomplete probe
    return b->mComplete;
}

// helper class to seed octree with probes
void LLReflectionMapManager::update()
{
    if (!LLPipeline::sReflectionProbesEnabled || gTeleportDisplay || LLStartUp::getStartupState() < STATE_PRECACHE)
    {
        return;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LL_PROFILE_GPU_ZONE("reflection manager update");
    llassert(!gCubeSnapshot); // assert a snapshot is not in progress
    if (LLAppViewer::instance()->logoutRequestSent())
    {
        return;
    }

    if (mPaused && gFrameTimeSeconds > mResumeTime)
    {
        resume();
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
        U32 count = (U32)(log2((F32)res) + 0.5f);

        mMipChain.resize(count);
        for (U32 i = 0; i < count; ++i)
        {
            mMipChain[i].allocate(res, res, GL_RGB16F);
            res /= 2;
        }
    }

    llassert(mProbes[0] == mDefaultProbe);

    LLVector4a camera_pos;
    camera_pos.load3(LLViewerCamera::instance().getOrigin().mV);

    // process kill list
    for (auto& probe : mKillList)
    {
        auto const & iter = std::find(mProbes.begin(), mProbes.end(), probe);
        if (iter != mProbes.end())
        {
            deleteProbe((U32)(iter - mProbes.begin()));
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
    static LLCachedControl<S32> sLevel(gSavedSettings, "RenderReflectionProbeLevel", 3);

    bool realtime = sDetail >= (S32)LLReflectionMapManager::DetailLevel::REALTIME;

    LLReflectionMap* closestDynamic = nullptr;

    LLReflectionMap* oldestProbe = nullptr;
    LLReflectionMap* oldestOccluded = nullptr;

    if (mUpdatingProbe != nullptr)
    {
        did_update = true;
        doProbeUpdate();
    }

    // update distance to camera for all probes
    std::sort(mProbes.begin()+1, mProbes.end(), CompareProbeDistance());
    llassert(mProbes[0] == mDefaultProbe);
    llassert(mProbes[0]->mCubeArray == mTexture);
    llassert(mProbes[0]->mCubeIndex == 0);

    // make sure we're assigning cube slots to the closest probes

    // first free any cube indices for distant probes
    for (U32 i = mReflectionProbeCount; i < mProbes.size(); ++i)
    {
        LLReflectionMap* probe = mProbes[i];
        llassert(probe != nullptr);

        if (probe->mCubeIndex != -1 && mUpdatingProbe != probe)
        { // free this index
            mCubeFree.push_back(probe->mCubeIndex);

            probe->mCubeArray = nullptr;
            probe->mCubeIndex = -1;
            probe->mComplete = false;
        }
    }

    // next distribute the free indices
    U32 count = llmin(mReflectionProbeCount, (U32)mProbes.size());

    for (U32 i = 1; i < count && !mCubeFree.empty(); ++i)
    {
        // find the closest probe that needs a cube index
        LLReflectionMap* probe = mProbes[i];

        if (probe->mCubeIndex == -1)
        {
            S32 idx = allocateCubeIndex();
            llassert(idx > 0); //if we're still in this loop, mCubeFree should not be empty and allocateCubeIndex should be returning good indices
            probe->mCubeArray = mTexture;
            probe->mCubeIndex = idx;
        }
    }

    for (unsigned int i = 0; i < mProbes.size(); ++i)
    {
        LLReflectionMap* probe = mProbes[i];
        if (probe->getNumRefs() == 1)
        { // no references held outside manager, delete this probe
            deleteProbe(i);
            --i;
            continue;
        }

        if (probe != mDefaultProbe &&
            (!probe->isRelevant() || mPaused))
        { // skip irrelevant probes (or all non-default probes if paused)
            continue;
        }

        LLVector4a d;

        if (probe != mDefaultProbe)
        {
            if (probe->mViewerObject) //make sure probes track the viewer objects they are attached to
            {
                probe->mOrigin.load3(probe->mViewerObject->getPositionAgent().mV);
            }
            d.setSub(camera_pos, probe->mOrigin);
            probe->mDistance = d.getLength3().getF32() - probe->mRadius;
        }
        else if (probe->mComplete)
        {
            // make default probe have a distance of 64m for the purposes of prioritization (if it's already been generated once)
            probe->mDistance = 64.f;
        }
        else
        {
            probe->mDistance = -4096.f; //boost priority of default probe when it's not complete
        }

        if (probe->mComplete)
        {
            probe->autoAdjustOrigin();
            probe->mFadeIn = llmin((F32) (probe->mFadeIn + gFrameIntervalSeconds), 1.f);
        }
        if (probe->mOccluded && probe->mComplete)
        {
            if (oldestOccluded == nullptr)
            {
                oldestOccluded = probe;
            }
            else if (probe->mLastUpdateTime < oldestOccluded->mLastUpdateTime)
            {
                oldestOccluded = probe;
            }
        }
        else
        {
            if (!did_update &&
                i < mReflectionProbeCount &&
                (oldestProbe == nullptr ||
                    check_priority(probe, oldestProbe)))
            {
               oldestProbe = probe;
            }
        }

        if (realtime &&
            closestDynamic == nullptr &&
            probe->mCubeIndex != -1 &&
            probe->getIsDynamic())
        {
            closestDynamic = probe;
        }
    }

    if (realtime && closestDynamic != nullptr)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("rmmu - realtime");
        // update the closest dynamic probe realtime
        // should do a full irradiance pass on "odd" frames and a radiance pass on "even" frames
        closestDynamic->autoAdjustOrigin();

        // store and override the value of "isRadiancePass" -- parts of the render pipe rely on "isRadiancePass" to set
        // lighting values etc
        bool radiance_pass = isRadiancePass();
        mRadiancePass = mRealtimeRadiancePass;
        for (U32 i = 0; i < 6; ++i)
        {
            updateProbeFace(closestDynamic, i);
        }
        mRealtimeRadiancePass = !mRealtimeRadiancePass;

        // restore "isRadiancePass"
        mRadiancePass = radiance_pass;
    }

    static LLCachedControl<F32> sUpdatePeriod(gSavedSettings, "RenderDefaultProbeUpdatePeriod", 2.f);
    if ((gFrameTimeSeconds - mDefaultProbe->mLastUpdateTime) < sUpdatePeriod)
    {
        if (sLevel == 0)
        { // when probes are disabled don't update the default probe more often than the prescribed update period
            oldestProbe = nullptr;
        }
    }
    else if (sLevel > 0)
    { // when probes are enabled don't update the default probe less often than the prescribed update period
      oldestProbe = mDefaultProbe;
    }

    // switch to updating the next oldest probe
    if (!did_update && oldestProbe != nullptr)
    {
        LLReflectionMap* probe = oldestProbe;
        llassert(probe->mCubeIndex != -1);

        probe->autoAdjustOrigin();

        sUpdateCount++;
        mUpdatingProbe = probe;
        doProbeUpdate();
    }

    if (oldestOccluded)
    {
        // as far as this occluded probe is concerned, an origin/radius update is as good as a full update
        oldestOccluded->autoAdjustOrigin();
        oldestOccluded->mLastUpdateTime = gFrameTimeSeconds;
    }
}

LLReflectionMap* LLReflectionMapManager::addProbe(LLSpatialGroup* group)
{
    if (gGLManager.mGLVersion < 4.05f)
    {
        return nullptr;
    }

    LLReflectionMap* probe = new LLReflectionMap();
    probe->mGroup = group;

    if (mDefaultProbe.isNull())
    {  //safety check to make sure default probe is always first probe added
        mDefaultProbe = new LLReflectionMap();
        mProbes.push_back(mDefaultProbe);
    }

    llassert(mProbes[0] == mDefaultProbe);

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

struct CompareProbeDepth
{
    bool operator()(const LLReflectionMap* lhs, const LLReflectionMap* rhs)
    {
        return lhs->mMinDepth < rhs->mMinDepth;
    }
};

void LLReflectionMapManager::getReflectionMaps(std::vector<LLReflectionMap*>& maps)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;

    LLMatrix4a modelview;
    modelview.loadu(gGLModelView);
    LLVector4a oa; // scratch space for transformed origin

    U32 count = 0;
    U32 lastIdx = 0;
    for (U32 i = 0; count < maps.size() && i < mProbes.size(); ++i)
    {
        mProbes[i]->mLastBindTime = gFrameTimeSeconds; // something wants to use this probe, indicate it's been requested
        if (mProbes[i]->mCubeIndex != -1)
        {
            if (!mProbes[i]->mOccluded && mProbes[i]->mComplete)
            {
                maps[count++] = mProbes[i];
                modelview.affineTransform(mProbes[i]->mOrigin, oa);
                mProbes[i]->mMinDepth = -oa.getF32ptr()[2] - mProbes[i]->mRadius;
                mProbes[i]->mMaxDepth = -oa.getF32ptr()[2] + mProbes[i]->mRadius;
            }
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

    if (count > 1)
    {
        std::sort(maps.begin(), maps.begin() + count, CompareProbeDepth());
    }

    for (U32 i = 0; i < count; ++i)
    {
        maps[i]->mProbeIndex = i;
    }

    // null terminate list
    if (count < maps.size())
    {
        maps[count] = nullptr;
    }
}

LLReflectionMap* LLReflectionMapManager::registerSpatialGroup(LLSpatialGroup* group)
{
    if (!group)
    {
        return nullptr;
    }
    LLSpatialPartition* part = group->getSpatialPartition();
    if (!part || part->mPartitionType != LLViewerRegion::PARTITION_VOLUME)
    {
        return nullptr;
    }
    OctreeNode* node = group->getOctreeNode();
    F32 size = node->getSize().getF32ptr()[0];
    if (size < 15.f || size > 17.f)
    {
        return nullptr;
    }
    return addProbe(group);
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
    if (!mCubeFree.empty())
    {
        S32 ret = mCubeFree.front();
        mCubeFree.pop_front();
        return ret;
    }

    return -1;
}

void LLReflectionMapManager::deleteProbe(U32 i)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LLReflectionMap* probe = mProbes[i];

    llassert(probe != mDefaultProbe);

    if (probe->mCubeIndex != -1)
    { // mark the cube index used by this probe as being free
        mCubeFree.push_back(probe->mCubeIndex);
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

    bool debug_updates = gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_PROBE_UPDATES) && mUpdatingProbe->mViewerObject;

    if (++mUpdatingFace == 6)
    {
        if (debug_updates)
        {
            mUpdatingProbe->mViewerObject->setDebugText(llformat("%.1f", (F32)gFrameTimeSeconds), LLColor4(1, 1, 1, 1));
        }
        updateNeighbors(mUpdatingProbe);
        mUpdatingFace = 0;
        if (isRadiancePass())
        {
            mUpdatingProbe->mComplete = true;
            mUpdatingProbe = nullptr;
            mRadiancePass = false;
        }
        else
        {
            mRadiancePass = true;
        }
    }
    else if (debug_updates)
    {
        mUpdatingProbe->mViewerObject->setDebugText(llformat("%.1f", (F32)gFrameTimeSeconds), LLColor4(1, 1, 0, 1));
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
void LLReflectionMapManager::updateProbeFace(LLReflectionMap* probe, U32 face)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LL_PROFILE_GPU_ZONE("probe update");
    // hacky hot-swap of camera specific render targets
    gPipeline.mRT = &gPipeline.mAuxillaryRT;

    mLightScale = 1.f;
    static LLCachedControl<F32> max_local_light_ambiance(gSavedSettings, "RenderReflectionProbeMaxLocalLightAmbiance", 8.f);
    if (!isRadiancePass() && probe->getAmbiance() > max_local_light_ambiance)
    {
        mLightScale = max_local_light_ambiance / probe->getAmbiance();
    }

    if (probe == mDefaultProbe)
    {
        touch_default_probe(probe);

        gPipeline.pushRenderTypeMask();

        //only render sky, water, terrain, and clouds
        gPipeline.andRenderTypeMask(LLPipeline::RENDER_TYPE_SKY, LLPipeline::RENDER_TYPE_WL_SKY,
            LLPipeline::RENDER_TYPE_WATER, LLPipeline::RENDER_TYPE_VOIDWATER, LLPipeline::RENDER_TYPE_CLOUDS, LLPipeline::RENDER_TYPE_TERRAIN, LLPipeline::END_RENDER_TYPES);

        probe->update(mRenderTarget.getWidth(), face);

        gPipeline.popRenderTypeMask();
    }
    else
    {
        probe->update(mRenderTarget.getWidth(), face);
    }

    gPipeline.mRT = &gPipeline.mMainRT;

    S32 sourceIdx = mReflectionProbeCount;

    if (probe != mUpdatingProbe)
    { // this is the "realtime" probe that's updating every frame, use the secondary scratch space channel
        sourceIdx += 1;
    }

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


        S32 mips = (S32)(log2((F32)mProbeResolution) + 0.5f);

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

            GLint mip = i - (static_cast<GLint>(mMipChain.size()) - mips);

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

        if (isRadiancePass())
        {
            //generate radiance map (even if this is not the irradiance map, we need the mip chain for the irradiance map)
            gRadianceGenProgram.bind();
            mVertexBuffer->setBuffer();

            S32 channel = gRadianceGenProgram.enableTexture(LLShaderMgr::REFLECTION_PROBES, LLTexUnit::TT_CUBE_MAP_ARRAY);
            mTexture->bind(channel);
            gRadianceGenProgram.uniform1i(sSourceIdx, sourceIdx);
            gRadianceGenProgram.uniform1f(LLShaderMgr::REFLECTION_PROBE_MAX_LOD, mMaxProbeLOD);
            gRadianceGenProgram.uniform1f(LLShaderMgr::REFLECTION_PROBE_STRENGTH, 1.f);

            U32 res = mMipChain[0].getWidth();

            for (int i = 0; i < mMipChain.size(); ++i)
            {
                LL_PROFILE_GPU_ZONE("probe radiance gen");
                static LLStaticHashedString sMipLevel("mipLevel");
                static LLStaticHashedString sRoughness("roughness");
                static LLStaticHashedString sWidth("u_width");

                gRadianceGenProgram.uniform1f(sRoughness, (F32)i / (F32)(mMipChain.size() - 1));
                gRadianceGenProgram.uniform1f(sMipLevel, (GLfloat)i);
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
        else
        {
            //generate irradiance map
            gIrradianceGenProgram.bind();
            S32 channel = gIrradianceGenProgram.enableTexture(LLShaderMgr::REFLECTION_PROBES, LLTexUnit::TT_CUBE_MAP_ARRAY);
            mTexture->bind(channel);

            gIrradianceGenProgram.uniform1i(sSourceIdx, sourceIdx);
            gIrradianceGenProgram.uniform1f(LLShaderMgr::REFLECTION_PROBE_MAX_LOD, mMaxProbeLOD);

            mVertexBuffer->setBuffer();
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
        }

        mMipChain[0].flush();

        gIrradianceGenProgram.unbind();
    }
}

void LLReflectionMapManager::reset()
{
    mReset = true;
}

void LLReflectionMapManager::pause(F32 duration)
{
    mPaused = true;
    mResumeTime = gFrameTimeSeconds + duration;
}

void LLReflectionMapManager::resume()
{
    mPaused = false;
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
    if (probe->isRelevant())
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("rmmun - search");
        for (auto& other : mProbes)
        {
            if (other != mDefaultProbe && other != probe)
            {
                if (other->isRelevant() && probe->intersects(other))
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
    LL_PROFILE_GPU_ZONE("rmmu - uniforms")

    // structure for packing uniform buffer object
    // see class3/deferred/reflectionProbeF.glsl
    struct ReflectionProbeData
    {
        // for box probes, matrix that transforms from camera space to a [-1, 1] cube representing the bounding box of
        // the box probe
        LLMatrix4 refBox[LL_MAX_REFLECTION_PROBE_COUNT];

        LLMatrix4 heroBox;

        // for sphere probes, origin (xyz) and radius (w) of refmaps in clip space
        LLVector4 refSphere[LL_MAX_REFLECTION_PROBE_COUNT];

        // extra parameters
        //  x - irradiance scale
        //  y - radiance scale
        //  z - fade in
        //  w - znear
        LLVector4 refParams[LL_MAX_REFLECTION_PROBE_COUNT];

        LLVector4 heroSphere;

        // indices used by probe:
        //  [i][0] - cubemap array index for this probe
        //  [i][1] - index into "refNeighbor" for probes that intersect this probe
        //  [i][2] - number of probes  that intersect this probe, or -1 for no neighbors
        //  [i][3] - priority (probe type stored in sign bit - positive for spheres, negative for boxes)
        GLint refIndex[LL_MAX_REFLECTION_PROBE_COUNT][4];

        // list of neighbor indices
        GLint refNeighbor[4096];

        GLint refBucket[256][4]; //lookup table for which index to start with for the given Z depth
        // numbrer of active refmaps
        GLint refmapCount;

        GLint     heroShape;
        GLint     heroMipCount;
        GLint     heroProbeCount;
    };

    mReflectionMaps.resize(mReflectionProbeCount);
    getReflectionMaps(mReflectionMaps);

    ReflectionProbeData rpd;

    F32 minDepth[256];

    for (int i = 0; i < 256; ++i)
    {
        rpd.refBucket[i][0] = mReflectionProbeCount;
        rpd.refBucket[i][1] = mReflectionProbeCount;
        rpd.refBucket[i][2] = mReflectionProbeCount;
        rpd.refBucket[i][3] = mReflectionProbeCount;
        minDepth[i] = FLT_MAX;
    }

    // load modelview matrix into matrix 4a
    LLMatrix4a modelview;
    modelview.loadu(gGLModelView);
    LLVector4a oa; // scratch space for transformed origin

    S32 count = 0;
    U32 nc = 0; // neighbor "cursor" - index into refNeighbor to start writing the next probe's list of neighbors

    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t psky = environment.getCurrentSky();

    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", true);
    F32 minimum_ambiance = psky->getReflectionProbeAmbiance(should_auto_adjust);

    bool is_ambiance_pass = gCubeSnapshot && !isRadiancePass();
    F32 ambscale = is_ambiance_pass ? 0.f : 1.f;
    F32 radscale = is_ambiance_pass ? 0.5f : 1.f;

    for (auto* refmap : mReflectionMaps)
    {
        if (refmap == nullptr)
        {
            break;
        }

        if (refmap != mDefaultProbe)
        {
            // bucket search data
            // theory of operation:
            //      1. Determine minimum and maximum depth of each influence volume and store in mDepth (done in getReflectionMaps)
            //      2. Sort by minimum depth
            //      3. Prepare a bucket for each 1m of depth out to 256m
            //      4. For each bucket, store the index of the nearest probe that might influence pixels in that bucket
            //      5. In the shader, lookup the bucket for the pixel depth to get the index of the first probe that could possibly influence
            //          the current pixel.
            unsigned int depth_min = llclamp(llfloor(refmap->mMinDepth), 0, 255);
            unsigned int depth_max = llclamp(llfloor(refmap->mMaxDepth), 0, 255);
            for (U32 i = depth_min; i <= depth_max; ++i)
            {
                if (refmap->mMinDepth < minDepth[i])
                {
                    minDepth[i] = refmap->mMinDepth;
                    rpd.refBucket[i][0] = refmap->mProbeIndex;
                }
            }
        }

        llassert(refmap->mProbeIndex == count);
        llassert(mReflectionMaps[refmap->mProbeIndex] == refmap);

        llassert(refmap->mCubeIndex >= 0); // should always be  true, if not, getReflectionMaps is bugged

        {
            if (refmap->mViewerObject && refmap->mViewerObject->getVolume())
            { // have active manual probes live-track the object they're associated with
                LLVOVolume* vobj = (LLVOVolume*)refmap->mViewerObject;

                refmap->mOrigin.load3(vobj->getPositionAgent().mV);

                if (vobj->getReflectionProbeIsBox())
                {
                    LLVector3 s = vobj->getScale().scaledVec(LLVector3(0.5f, 0.5f, 0.5f));
                    refmap->mRadius = s.magVec();
                }
                else
                {
                    refmap->mRadius = refmap->mViewerObject->getScale().mV[0] * 0.5f;
                }
            }
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

        rpd.refParams[count].set(
            llmax(minimum_ambiance, refmap->getAmbiance())*ambscale, // ambiance scale
            radscale, // radiance scale
            refmap->mFadeIn, // fade in weight
            oa.getF32ptr()[2] - refmap->mRadius); // z near

        S32 ni = nc; // neighbor ("index") - index into refNeighbor to write indices for current reflection probe's neighbors
        {
            //LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("rmmsu - refNeighbors");
            //pack neghbor list
            const U32 max_neighbors = 64;
            U32 neighbor_count = 0;

            for (auto& neighbor : refmap->mNeighbors)
            {
                if (ni >= 4096)
                { // out of space
                    break;
                }

                GLint idx = neighbor->mProbeIndex;
                if (idx == -1 || neighbor->mOccluded || neighbor->mCubeIndex == -1)
                {
                    continue;
                }

                // this neighbor may be sampled
                rpd.refNeighbor[ni++] = idx;

                neighbor_count++;
                if (neighbor_count >= max_neighbors)
                {
                    break;
                }
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

#if 0
    {
        // fill in gaps in refBucket
        S32 probe_idx = mReflectionProbeCount;

        for (int i = 0; i < 256; ++i)
        {
            if (i < count)
            { // for debugging, store depth of mReflectionsMaps[i]
                rpd.refBucket[i][1] = (S32) (mReflectionMaps[i]->mDepth * 10);
            }

            if (rpd.refBucket[i][0] == mReflectionProbeCount)
            {
                rpd.refBucket[i][0] = probe_idx;
            }
            else
            {
                probe_idx = rpd.refBucket[i][0];
            }
        }
    }
#endif

    rpd.refmapCount = count;

    gPipeline.mHeroProbeManager.updateUniforms();

    // Get the hero data.

    rpd.heroBox = gPipeline.mHeroProbeManager.mHeroData.heroBox;
    rpd.heroSphere = gPipeline.mHeroProbeManager.mHeroData.heroSphere;
    rpd.heroShape  = gPipeline.mHeroProbeManager.mHeroData.heroShape;
    rpd.heroMipCount = gPipeline.mHeroProbeManager.mHeroData.heroMipCount;
    rpd.heroProbeCount = gPipeline.mHeroProbeManager.mHeroData.heroProbeCount;

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
    glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_REFLECTION_PROBES, mUBO);
}


void renderReflectionProbe(LLReflectionMap* probe)
{
    if (probe->isRelevant())
    {
        F32* po = probe->mOrigin.getF32ptr();

        //draw orange line from probe to neighbors
        gGL.flush();
        gGL.diffuseColor4f(1, 0.5f, 0, 1);
        gGL.begin(gGL.LINES);
        for (auto& neighbor : probe->mNeighbors)
        {
            if (probe->mViewerObject && neighbor->mViewerObject)
            {
                continue;
            }

            gGL.vertex3fv(po);
            gGL.vertex3fv(neighbor->mOrigin.getF32ptr());
        }
        gGL.end();
        gGL.flush();

        gGL.diffuseColor4f(1, 1, 0, 1);
        gGL.begin(gGL.LINES);
        for (auto& neighbor : probe->mNeighbors)
        {
            if (probe->mViewerObject && neighbor->mViewerObject)
            {
                gGL.vertex3fv(po);
                gGL.vertex3fv(neighbor->mOrigin.getF32ptr());
            }
        }
        gGL.end();
        gGL.flush();
    }

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
    U32 count = LL_MAX_REFLECTION_PROBE_COUNT;

    static LLCachedControl<U32> ref_probe_res(gSavedSettings, "RenderReflectionProbeResolution", 128U);
    U32 probe_resolution = nhpo2(llclamp(ref_probe_res(), (U32)64, (U32)512));
    if (mTexture.isNull() || mReflectionProbeCount != count || mProbeResolution != probe_resolution || mReset)
    {
        if(mProbeResolution != probe_resolution)
        {
            mRenderTarget.release();
            mMipChain.clear();
        }

        gEXRImage = nullptr;

        mReset = false;
        mReflectionProbeCount = count;
        mProbeResolution = probe_resolution;
        mMaxProbeLOD = log2f((F32)mProbeResolution) - 1.f; // number of mips - 1

        if (mTexture.isNull() ||
            mTexture->getWidth() != mProbeResolution ||
            mReflectionProbeCount + 2 != mTexture->getCount())
        {
            mTexture = new LLCubeMapArray();

            // store mReflectionProbeCount+2 cube maps, final two cube maps are used for render target and radiance map generation source)
            mTexture->allocate(mProbeResolution, 3, mReflectionProbeCount + 2);

            mIrradianceMaps = new LLCubeMapArray();
            mIrradianceMaps->allocate(LL_IRRADIANCE_MAP_RESOLUTION, 3, mReflectionProbeCount, false);
        }

        // reset probe state
        mUpdatingFace = 0;
        mUpdatingProbe = nullptr;
        mRadiancePass = false;
        mRealtimeRadiancePass = false;

        // if default probe already exists, remember whether or not it's complete (SL-20498)
        bool default_complete = mDefaultProbe.isNull() ? false : mDefaultProbe->mComplete;

        for (auto& probe : mProbes)
        {
            probe->mLastUpdateTime = 0.f;
            probe->mComplete = false;
            probe->mProbeIndex = -1;
            probe->mCubeArray = nullptr;
            probe->mCubeIndex = -1;
            probe->mNeighbors.clear();
        }

        mCubeFree.clear();
        initCubeFree();

        if (mDefaultProbe.isNull())
        {
            llassert(mProbes.empty()); // default probe MUST be the first probe created
            mDefaultProbe = new LLReflectionMap();
            mProbes.push_back(mDefaultProbe);
        }

        llassert(mProbes[0] == mDefaultProbe);

        mDefaultProbe->mCubeIndex = 0;
        mDefaultProbe->mCubeArray = mTexture;
        mDefaultProbe->mDistance = 64.f;
        mDefaultProbe->mRadius = 4096.f;
        mDefaultProbe->mProbeIndex = 0;
        mDefaultProbe->mComplete = default_complete;

        touch_default_probe(mDefaultProbe);
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

void LLReflectionMapManager::cleanup()
{
    mVertexBuffer = nullptr;
    mRenderTarget.release();

    mMipChain.clear();

    mTexture = nullptr;
    mIrradianceMaps = nullptr;

    mProbes.clear();
    mKillList.clear();
    mCreateList.clear();

    mReflectionMaps.clear();
    mUpdatingFace = 0;

    mDefaultProbe = nullptr;
    mUpdatingProbe = nullptr;

    glDeleteBuffers(1, &mUBO);
    mUBO = 0;

    // note: also called on teleport (not just shutdown), so make sure we're in a good "starting" state
    initCubeFree();
}

void LLReflectionMapManager::doOcclusion()
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

void LLReflectionMapManager::forceDefaultProbeAndUpdateUniforms(bool force)
{
    static std::vector<bool> mProbeWasOccluded;

    if (force)
    {
        llassert(mProbeWasOccluded.empty());

        for (size_t i = 0; i < mProbes.size(); ++i)
        {
            auto& probe = mProbes[i];
            mProbeWasOccluded.push_back(probe->mOccluded);
            if (probe != nullptr && probe != mDefaultProbe)
            {
                probe->mOccluded = true;
            }
        }

        updateUniforms();
    }
    else
    {
        llassert(mProbes.size() == mProbeWasOccluded.size());

        const size_t n = llmin(mProbes.size(), mProbeWasOccluded.size());
        for (size_t i = 0; i < n; ++i)
        {
            auto& probe = mProbes[i];
            llassert(probe->mOccluded == (probe != mDefaultProbe));
            probe->mOccluded = mProbeWasOccluded[i];
        }
        mProbeWasOccluded.clear();
        mProbeWasOccluded.shrink_to_fit();
    }
}
