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

LLReflectionMapManager::LLReflectionMapManager()
{

}

struct CompareReflectionMapDistance
{
    
};


struct CompareProbeDistance
{
    bool operator()(const LLReflectionMap& lhs, const LLReflectionMap& rhs)
    {
        return lhs.mDistance < rhs.mDistance;
    }
};

// helper class to seed octree with probes
void LLReflectionMapManager::update()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;

    // naively drop probes every 16m as we move the camera around for now
    // later, use LLSpatialPartition to manage probes
    const F32 PROBE_SPACING = 16.f;
    const U32 MAX_PROBES = 8;

    LLVector4a camera_pos;
    camera_pos.load3(LLViewerCamera::instance().getOrigin().mV);

    for (auto& probe : mProbes)
    {
        LLVector4a d;
        d.setSub(camera_pos, probe.mOrigin);
        probe.mDistance = d.getLength3().getF32();
    }

    if (mProbes.empty() || mProbes[0].mDistance > PROBE_SPACING)
    {
        addProbe(LLViewerCamera::instance().getOrigin());
    }

    // update distance to camera for all probes
    std::sort(mProbes.begin(), mProbes.end(), CompareProbeDistance());

    if (mProbes.size() > MAX_PROBES)
    {
        mProbes.resize(MAX_PROBES);
    }
}

void LLReflectionMapManager::addProbe(const LLVector3& pos)
{
    LLReflectionMap probe;
    probe.update(pos, 1024);
    mProbes.push_back(probe);
}

void LLReflectionMapManager::getReflectionMaps(std::vector<LLReflectionMap*>& maps)
{
    // just null out for now
    U32 i = 0;
    for (i = 0; i < maps.size() && i < mProbes.size(); ++i)
    {
        maps[i] = &(mProbes[i]);
    }

    for (++i; i < maps.size(); ++i)
    {
        maps[i] = nullptr;
    }
}

