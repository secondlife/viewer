/**
 * @file llreflectionmapmanager.h
 * @brief LLReflectionMapManager class declaration
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

#pragma once

#include "llreflectionmap.h"
#include "llrendertarget.h"
#include "llcubemaparray.h"
#include "llcubemap.h"

class LLSpatialGroup;
class LLViewerObject;

// number of reflection probes to keep in vram
#define LL_MAX_REFLECTION_PROBE_COUNT 256

// reflection probe resolution
#define LL_IRRADIANCE_MAP_RESOLUTION 16

// reflection probe mininum scale
#define LL_REFLECTION_PROBE_MINIMUM_SCALE 1.f;

void renderReflectionProbe(LLReflectionMap* probe);

class alignas(16) LLReflectionMapManager
{
    LL_ALIGN_NEW
public:
    enum class DetailLevel
    {
        STATIC_ONLY = 0,
        STATIC_AND_DYNAMIC,
        REALTIME = 2
    };

    // General guidance for UBOs is to statically allocate all of these fields to make your life ever so slightly easier.
    // Then set a "max" value for the number of probes you'll ever have, and use that to index into the arrays.
    // We do this with refmapCount.  The shaders will just pick up on it there.
    // This data structure should _always_ match what's in class3/deferred/reflectionProbeF.glsl.
    // The shader can and will break otherwise.
    // -Geenz 2025-03-10
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

        GLint refBucket[256][4]; // lookup table for which index to start with for the given Z depth
        // numbrer of active refmaps
        GLint refmapCount;

        GLint heroShape;
        GLint heroMipCount;
        GLint heroProbeCount;
    };

    // allocate an environment map of the given resolution
    LLReflectionMapManager();

    // release any GL state
    void cleanup();

    // maintain reflection probes
    void update();

    // add a probe for the given spatial group
    LLReflectionMap* addProbe(LLSpatialGroup* group = nullptr);

    // Populate "maps" with the N most relevant Reflection Maps where N is no more than maps.size()
    // If less than maps.size() ReflectionMaps are available, will assign trailing elements to nullptr.
    //  maps -- presized array of Reflection Map pointers
    void getReflectionMaps(std::vector<LLReflectionMap*>& maps);

    // called by LLSpatialGroup constructor
    // If spatial group should receive a Reflection Probe, will create one for the specified spatial group
    LLReflectionMap* registerSpatialGroup(LLSpatialGroup* group);

    // presently hacked into LLViewerObject::setTE
    // Used by LLViewerObjects that are Reflection Probes
    // vobj must not be null
    // Guaranteed to not return null
    LLReflectionMap* registerViewerObject(LLViewerObject* vobj);

    // reset all state on the next update
    void reset();

    // pause all updates other than the default probe
    // duration - number of seconds to pause (default 10)
    void pause(F32 duration = 10.f);

    // unpause (see pause)
    void resume();

    // called on region crossing to "shift" probes into new coordinate frame
    void shift(const LLVector4a& offset);

    // debug display, called from llspatialpartition if reflection
    // probe debug display is active
    void renderDebug();

    // call once at startup to allocate cubemap arrays
    void initReflectionMaps();

    // True if currently updating a radiance map, false if currently updating an irradiance map
    bool isRadiancePass() { return mRadiancePass; }

    // perform occlusion culling on all active reflection probes
    void doOcclusion();

    // *HACK: "cull" all reflection probes except the default one. Only call
    // this if you don't intend to call updateUniforms directly. Call again
    // with false when done.
    void forceDefaultProbeAndUpdateUniforms(bool force = true);

    U32 probeCount();
    U32 probeMemory();

private:
    friend class LLPipeline;
    friend class LLHeroProbeManager;

    // initialize mCubeFree array to default values
    void initCubeFree();

    // Just does a bulk clear of all of the cubemaps.
    void clearCubeMaps();

    // delete the probe with the given index in mProbes
    void deleteProbe(U32 i);

    // get a free cube index
    // returns -1 if allocation failed
    S32 allocateCubeIndex();

    // update the neighbors of the given probe
    void updateNeighbors(LLReflectionMap* probe);

    // update UBO used for rendering (call only once per render pipe flush)
    void updateUniforms();

    // bind UBO used for rendering
    void setUniforms();

    // render target for cube snapshots
    // used to generate mipmaps without doing a copy-to-texture
    LLRenderTarget mRenderTarget;

    std::vector<LLRenderTarget> mMipChain;

    // storage for reflection probe radiance maps (plus two scratch space cubemaps)
    LLPointer<LLCubeMapArray> mTexture;

    // vertex buffer for pushing verts to filter shaders
    LLPointer<LLVertexBuffer> mVertexBuffer;

    // storage for reflection probe irradiance maps
    LLPointer<LLCubeMapArray> mIrradianceMaps;

    // list of free cubemap indices
    std::list<S32> mCubeFree;

    // perform an update on the currently updating Probe
    void doProbeUpdate();

    // update the specified face of the specified probe
    void updateProbeFace(LLReflectionMap* probe, U32 face);

    // list of active reflection maps
    std::vector<LLPointer<LLReflectionMap> > mProbes;

    // list of reflection maps to kill
    std::vector<LLPointer<LLReflectionMap> > mKillList;

    // list of reflection maps to create
    std::vector<LLPointer<LLReflectionMap> > mCreateList;

    // handle to UBO
    U32 mUBO = 0;

    // list of maps being used for rendering
    std::vector<LLReflectionMap*> mReflectionMaps;

    LLReflectionMap* mUpdatingProbe = nullptr;
    U32 mUpdatingFace = 0;

    // if true, we're generating the radiance map for the current probe, otherwise we're generating the irradiance map.
    // Update sequence should be to generate the irradiance map from render of the world that has no irradiance,
    // then generate the radiance map from a render of the world that includes irradiance.
    // This should avoid feedback loops and ensure that the colors in the radiance maps match the colors in the environment.
    bool mRadiancePass = false;

    // same as above, but for the realtime probe.
    // Realtime probes should update all six sides of the irradiance map on "odd" frames and all six sides of the
    // radiance map on "even" frames.
    bool mRealtimeRadiancePass = false;

    LLPointer<LLReflectionMap> mDefaultProbe;  // default reflection probe to fall back to for pixels with no probe influences (should always be at cube index 0)

    // number of reflection probes to use for rendering
    U32 mReflectionProbeCount;

    U32 mDynamicProbeCount;

    // resolution of reflection probes
    U32 mProbeResolution = 128;

    // maximum LoD of reflection probes (mip levels - 1)
    F32 mMaxProbeLOD = 6.f;

    // amount to scale local lights during an irradiance map update (set during updateProbeFace and used by LLPipeline)
    F32 mLightScale = 1.f;

    // if true, reset all probe render state on the next update (for teleports and sky changes)
    bool mReset = false;

    float mResetFade = 1.f;
    float mGlobalFadeTarget = 1.f;

    // if true, only update the default probe
    bool mPaused = false;
    F32 mResumeTime = 0.f;

    ReflectionProbeData mProbeData;
};

