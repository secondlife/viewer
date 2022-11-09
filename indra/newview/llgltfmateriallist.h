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

    // Queue an override update that we want to send to the simulator.  Call "flushUpdates" to flush pending updates.
    //  id - ID of object to modify
    //  side - TexureEntry index to modify, or -1 for all sides
    //  mat - material to apply as override, or nullptr to remove existing overrides and revert to asset
    //
    // NOTE: do not use to revert to asset when applying a new asset id, use queueApplyMaterialAsset below
    static void queueModifyMaterial(const LLUUID& id, S32 side, const LLGLTFMaterial* mat);

    // Queue an application of a material asset we want to send to the simulator.  Call "flushUpdates" to flush pending updates.
    //  object_id - ID of object to apply material asset to
    //  side - TextureEntry index to apply material to, or -1 for all sides
    //  asset_id - ID of material asset to apply, or LLUUID::null to disassociate current material asset
    //
    // NOTE: implicitly removes any override data if present
    static void queueApplyMaterialAsset(const LLUUID& object_id, S32 side, const LLUUID& asset_id);

    // flush pending material updates to the simulator
    static void  flushUpdates(void(*done_callback)(bool) = nullptr);
    
    // apply given override data via given cap url
    //  cap_url -- should be gAgent.getRegionCapability("ModifyMaterialParams")
    //  overrides -- LLSD map (or array of maps) in the format:
    //      object_id   UUID(required)      id of object
    //      side        integer(required)   TE index of face to set, or -1 for all faces
    //      gltf_json   string(optional)    override data to set, empty string nulls out override data, omissions of this parameter keeps existing data
    //      asset_id    UUID(optional)      id of material asset to set, omission of this parameter keeps existing material asset id
    //    
    // NOTE: if you're calling this from outside of flushUpdates, you're probably doing it wrong.  Use the "queue"/"flush" API above.
    // If the queue/flush API is insufficient, extend it.
    static void modifyMaterialCoro(std::string cap_url, LLSD overrides, void(*done_callback)(bool));

    void applyQueuedOverrides(LLViewerObject* obj);

private:
    friend class LLGLTFMaterialOverrideDispatchHandler;
    // save an override update that we got from the simulator for later (for example, if an override arrived for an unknown object)
    // NOTE: this is NOT for applying overrides from the UI, see queueModifyMaterial above
    void queueOverrideUpdate(const LLUUID& id, S32 side, LLGLTFMaterial* override_data);

    typedef std::unordered_map<LLUUID, LLPointer<LLFetchedGLTFMaterial > > uuid_mat_map_t;
    uuid_mat_map_t mList;

    typedef std::vector<LLPointer<LLGLTFMaterial> > override_list_t;
    typedef std::unordered_map<LLUUID, override_list_t > queued_override_map_t;
    queued_override_map_t mQueuedOverrides;

    LLUUID mLastUpdateKey;

    struct ModifyMaterialData
    {
        LLUUID object_id;
        S32 side = -1;
        LLGLTFMaterial override_data;

        bool has_override = false;
    };

    typedef std::list<ModifyMaterialData> modify_queue_t;
    static modify_queue_t sModifyQueue;

    struct ApplyMaterialAssetData
    {
        LLUUID object_id;
        S32 side = -1;
        LLUUID asset_id;
    };

    typedef std::list<ApplyMaterialAssetData> apply_queue_t;
    static apply_queue_t sApplyQueue;

};

extern LLGLTFMaterialList gGLTFMaterialList;


