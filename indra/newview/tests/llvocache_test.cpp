/**
 * @file llvocache_.cpp
 * @author Brad
 * @date 2023-03-01
 * @brief Test Viewer Object Cache functionality
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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
#include "../llviewerprecompiledheaders.h"
#include "../test/lltut.h"

#include "../llvocache.h"

#include "lldir.h"
#include "../llhudobject.h"
#include "llregionhandle.h"
#include "llsdutil.h"
#include "llsdserialize.h"

#include "../llviewerobjectlist.h"
#include "../llviewerregion.h"

#include "lldir_stub.cpp"
#include "llvieweroctree_stub.cpp"

namespace
{

}


//----------------------------------------------------------------------------
// Mock objects for the dependencies of the code we're testing
S32 LLVOCachePartition::cull(LLCamera &camera, bool do_occlusion) { return 0; }

LLViewerObjectList::LLViewerObjectList() = default;
LLViewerObjectList::~LLViewerObjectList() = default;
LLDebugBeacon::~LLDebugBeacon() = default;
LLViewerObjectList gObjectList{};
LLViewerCamera::eCameraID LLViewerCamera::sCurCameraID{};
void LLViewerObject::unpackUUID(LLDataPackerBinaryBuffer *dp, LLUUID &value, std::string name) {}

bool LLViewerRegion::addVisibleGroup(LLViewerOctreeGroup*) { return false; }
U32 LLViewerRegion::getNumOfVisibleGroups() const { return 0; }
LLVector3 LLViewerRegion::getOriginAgent() const { return LLVector3::zero; }
S32 LLViewerRegion::sLastCameraUpdated{};

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------
namespace tut
{
    // Test wrapper declaration
    struct vocacheTest
    {
        vocacheTest() = default;
        ~vocacheTest() = default;

        static const std::string override_llsd_text[];
    };

    const std::string vocacheTest::override_llsd_text[]{
        // sample override contents captured from traffic
        R"(<? llsd/notation ?>
        {'gltf_json':['{"asset":{"version":"2.0"},"images":[{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"}],"materials":[{"emissiveTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":3},"normalTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":1},"occlusionTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":4},"pbrMetallicRoughness":{"baseColorTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":0},"metallicRoughnessTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":2}}}],"textures":[{"source":0},{"source":1},{"source":2},{"source":3},{"source":4}]}\n','{"asset":{"version":"2.0"},"images":[{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"}],"materials":[{"emissiveTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":3},"normalTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":1},"occlusionTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":4},"pbrMetallicRoughness":{"baseColorTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":0},"metallicRoughnessTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":2}}}],"textures":[{"source":0},{"source":1},{"source":2},{"source":3},{"source":4}]}\n','{"asset":{"version":"2.0"},"images":[{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"}],"materials":[{"emissiveTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":3},"normalTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":1},"occlusionTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":4},"pbrMetallicRoughness":{"baseColorTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":0},"metallicRoughnessTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":2}}}],"textures":[{"source":0},{"source":1},{"source":2},{"source":3},{"source":4}]}\n','{"asset":{"version":"2.0"},"images":[{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"}],"materials":[{"emissiveTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":3},"normalTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":1},"occlusionTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":4},"pbrMetallicRoughness":{"baseColorTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":0},"metallicRoughnessTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":2}}}],"textures":[{"source":0},{"source":1},{"source":2},{"source":3},{"source":4}]}\n','{"asset":{"version":"2.0"},"images":[{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"}],"materials":[{"emissiveTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":3},"normalTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":1},"occlusionTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":4},"pbrMetallicRoughness":{"baseColorTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":0},"metallicRoughnessTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":2}}}],"textures":[{"source":0},{"source":1},{"source":2},{"source":3},{"source":4}]}\n','{"asset":{"version":"2.0"},"images":[{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"}],"materials":[{"emissiveTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":3},"normalTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":1},"occlusionTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":4},"pbrMetallicRoughness":{"baseColorTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":0},"metallicRoughnessTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":2}}}],"textures":[{"source":0},{"source":1},{"source":2},{"source":3},{"source":4}]}\n','{"asset":{"version":"2.0"},"images":[{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"},{"uri":"00000000-0000-0000-0000-000000000000"}],"materials":[{"emissiveTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":3},"normalTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":1},"occlusionTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":4},"pbrMetallicRoughness":{"baseColorTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":0},"metallicRoughnessTexture":{"extensions":{"KHR_texture_transform":{"offset":[0.0,0.0],"rotation":0.0,"scale":[2.0,2.0]}},"index":2}}}],"textures":[{"source":0},{"source":1},{"source":2},{"source":3},{"source":4}]}\n'],'object_id':u8b357110-4845-1d40-1cf0-cc4a2c22a4f1,'sides':[i0,i1,i2,i3,i4,i5,i6]})"
    };

    // Tut templating thingamagic: test group, object and test instance
    struct vocacheTestFactory : public test_group<vocacheTest>
    {
        vocacheTestFactory()
            : test_group<vocacheTest>("LLVOCache")
        {
            ll_init_apr();

            const bool READ_ONLY = false;
            const U32 INDRA_OBJECT_CACHE_VERSION = 15; // see LLAppViewer::getObjectCacheVersion()
            const U32 CACHE_NUMBER_OF_REGIONS = 128;   // see setting CacheNumberOfRegionsForObjects

            LLVOCache &instance = LLVOCache::initParamSingleton(READ_ONLY);
            instance.initCache(LL_PATH_CACHE, CACHE_NUMBER_OF_REGIONS, INDRA_OBJECT_CACHE_VERSION);
        }

        ~vocacheTestFactory()
        {
            LLVOCache::deleteSingleton();
            ll_cleanup_apr();
        }

    };

    typedef vocacheTestFactory::object vocacheTestObject;
    tut::vocacheTestFactory tut_test;

    // ---------------------------------------------------------------------------------------
    // Test functions
    // ---------------------------------------------------------------------------------------

    template<> template<>
    void vocacheTestObject::test<1>()
    {
        LLGLTFOverrideCacheEntry entry{};
        LLSD entry_llsd;

        std::istringstream in_llsd(override_llsd_text[0]);
        bool success = LLSDSerialize::deserialize(entry_llsd, in_llsd, override_llsd_text[0].length());

        ensure("llsd deserialize succeeds", success);

        entry.fromLLSD(entry_llsd);
        ensure_equals("entry object_id match", entry.mObjectId, LLUUID("8b357110-4845-1d40-1cf0-cc4a2c22a4f1"));
        ensure_equals("sides count", entry.mSides.size(), 7);

        //std::cout << other << std::endl;
        ensure_equals("toLLSD() match", entry.toLLSD(), entry_llsd);
    }

    template<> template<>
    void vocacheTestObject::test<2>()
    {
        LLVOCacheEntry::vocache_gltf_overrides_map_t extras;

        U64 region_handle = to_region_handle(140, 81);
        LLUUID region_id = LLUUID::generateNewID();

        LLVOCache::instance().readGenericExtrasFromCache(region_handle, region_id, extras);
    }
}
