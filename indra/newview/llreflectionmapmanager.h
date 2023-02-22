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
#define LL_IRRADIANCE_MAP_RESOLUTION 64

// reflection probe mininum scale
#define LL_REFLECTION_PROBE_MINIMUM_SCALE 1.f;

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
    // Guaranteed to not return null
    LLReflectionMap* registerViewerObject(LLViewerObject* vobj);

    // force an update of all probes
    void rebuild();

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

private:
    friend class LLPipeline;

    // initialize mCubeFree array to default values
    void initCubeFree();

    // delete the probe with the given index in mProbes
    void deleteProbe(U32 i);

    // get a free cube index
    // if no cube indices are free, free one starting from the back of the probe list
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

    // array indicating if a particular cubemap is free
    bool mCubeFree[LL_MAX_REFLECTION_PROBE_COUNT];

    // start tracking the given spatial group
    void trackGroup(LLSpatialGroup* group);
    
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

    // number of reflection probes to use for rendering (based on saved setting RenderReflectionProbeCount)
    U32 mReflectionProbeCount;

    // resolution of reflection probes
    U32 mProbeResolution = 128;

    // maximum LoD of reflection probes (mip levels - 1)
    F32 mMaxProbeLOD = 6.f;
};

