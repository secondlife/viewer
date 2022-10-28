/**
 * @file   llgltfmateriallist.h
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

#include "llfetchedgltfmaterial.h"
#include "llgltfmaterial.h"
#include "llpointer.h"

#include <unordered_map>

class LLFetchedGLTFMaterial;

class LLGLTFMaterialList
{
public:
    static const LLUUID BLANK_MATERIAL_ASSET_ID;

    LLGLTFMaterialList() {}


    LLGLTFMaterial* getMaterial(const LLUUID& id);

    void addMaterial(const LLUUID& id, LLFetchedGLTFMaterial* material);
    void removeMaterial(const LLUUID& id);

    void flushMaterials();

    static void registerCallbacks();

    // apply given override data via given cap url
    //  cap_url -- should be gAgent.getRegionCapability("ModifyMaterialParams")
    //  overrides -- LLSD map in the format
    //    "object_id": LLUUID - object to be modified
    //    "side": integer - index of face to be modified
    //    "gltf_json" : string - GLTF compliant json of override data (optional, if omitted any existing override data will be cleared)
    static void modifyMaterialCoro(std::string cap_url, LLSD overrides);
    // save an override update for later (for example, if an override arrived for an unknown object)
    void queueOverrideUpdate(const LLUUID& id, S32 side, LLGLTFMaterial* override_data);

    void applyQueuedOverrides(LLViewerObject* obj);

private:
    typedef std::unordered_map<LLUUID, LLPointer<LLFetchedGLTFMaterial > > uuid_mat_map_t;
    uuid_mat_map_t mList;

    typedef std::vector<LLPointer<LLGLTFMaterial> > override_list_t;
    typedef std::unordered_map<LLUUID, override_list_t > queued_override_map_t;
    queued_override_map_t mQueuedOverrides;

    LLUUID mLastUpdateKey;
};

extern LLGLTFMaterialList gGLTFMaterialList;


