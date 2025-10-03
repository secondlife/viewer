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

#include "llassettype.h"
#include "llextendedstatus.h"
#include "llfetchedgltfmaterial.h"
#include "llgltfmaterial.h"
#include "llpointer.h"

#include <unordered_map>

class LLFetchedGLTFMaterial;
class LLGLTFOverrideCacheEntry;

class LLGLTFMaterialList
{
public:
    LLGLTFMaterialList() {}


    LLFetchedGLTFMaterial* getMaterial(const LLUUID& id);

    void addMaterial(const LLUUID& id, LLFetchedGLTFMaterial* material);
    void removeMaterial(const LLUUID& id);

    void flushMaterials();

    // Queue an modification of a material that we want to send to the simulator.  Call "flushUpdates" to flush pending updates.
    //  id - ID of object to modify
    //  side - TexureEntry index to modify, or -1 for all sides
    //  mat - material to apply as override, or nullptr to remove existing overrides and revert to asset
    //
    // NOTE: do not use to revert to asset when applying a new asset id, use queueApply below
    static void queueModify(const LLViewerObject* obj, S32 side, const LLGLTFMaterial* mat);

    // Queue an application of a material asset we want to send to the simulator.
    //  Call "flushUpdates" to flush pending updates immediately.
    //  Will be flushed automatically if queue is full.
    //  object_id - ID of object to apply material asset to
    //  side - TextureEntry index to apply material to, or -1 for all sides
    //  asset_id - ID of material asset to apply, or LLUUID::null to disassociate current material asset
    //
    // NOTE: Implicitly clears most override data if present
    static void queueApply(const LLViewerObject* obj, S32 side, const LLUUID& asset_id);
    static void queueApply(const LLViewerObject* obj, S32 side, const LLUUID& asset_id, const std::string& override_json);

    // Queue an application of a material asset we want to send to the simulator.
    //  Call "flushUpdates" to flush pending updates immediately.
    //  Will be flushed automatically if queue is full.
    //  object_id - ID of object to apply material asset to
    //  side - TextureEntry index to apply material to, or -1 for all sides
    //  asset_id - ID of material asset to apply, or LLUUID::null to disassociate current material asset
    //  mat - override material, if null, will clear most override data
    static void queueApply(const LLViewerObject* obj, S32 side, const LLUUID& asset_id, const LLGLTFMaterial* mat);

    // flush pending material updates to the simulator
    // Automatically called once per frame, but may be called explicitly
    // for cases that care about the done_callback forwarded to LLCoros::instance().launch
    static void flushUpdates(void(*done_callback)(bool) = nullptr);

    static void addSelectionUpdateCallback(void(*update_callback)(const LLUUID& object_id, S32 side));

    // Queue an explicit LLSD ModifyMaterialParams update apply given override data
    //  overrides -- LLSD map (or array of maps) in the format:
    //      object_id   UUID(required)      id of object
    //      side        integer(required)   TE index of face to set, or -1 for all faces
    //      gltf_json   string(optional)    override data to set, empty string nulls out override data, omissions of this parameter keeps existing data
    //      asset_id    UUID(optional)      id of material asset to set, omission of this parameter keeps existing material asset id
    //
    // NOTE: Unless you already have a gltf_json string you want to send, strongly prefer using queueModify
    // If the queue/flush API is insufficient, extend it.
    static void queueUpdate(const LLSD& data);

    // Called by batch builder to give LLGLTMaterialList an opportunity to apply
    // any override data that arrived before the object was ready to receive it
    void applyQueuedOverrides(LLViewerObject* obj);

    // Apply an override update with the given data
    void applyOverrideMessage(LLMessageSystem* msg, const std::string& data);

private:
    friend class LLGLTFMaterialOverrideDispatchHandler;
    // save an override update that we got from the simulator for later (for example, if an override arrived for an unknown object)
    // NOTE: this is NOT for applying overrides from the UI, see queueModifyMaterial above
    void queueOverrideUpdate(const LLUUID& id, S32 side, LLGLTFMaterial* override_data);


    class CallbackHolder
    {
    public:
        CallbackHolder(void(*done_callback)(bool))
            : mCallback(done_callback)
        {}
        ~CallbackHolder()
        {
            if (mCallback) mCallback(mSuccess);
        }
        std::function<void(bool)> mCallback = nullptr;
        bool mSuccess = true;
    };
    static void flushUpdatesOnce(std::shared_ptr<CallbackHolder> callback_holder);
    static void modifyMaterialCoro(std::string cap_url, LLSD overrides, std::shared_ptr<CallbackHolder> callback_holder);

protected:
    static void onAssetLoadComplete(
        const LLUUID& asset_uuid,
        LLAssetType::EType type,
        void* user_data,
        S32 status,
        LLExtStat ext_status);

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
        LLPointer<LLGLTFMaterial> override_data;
        std::string override_json;
    };

    typedef std::list<ApplyMaterialAssetData> apply_queue_t;
    static apply_queue_t sApplyQueue;

    // data to be flushed to ModifyMaterialParams capability
    static LLSD    sUpdates;
};

extern LLGLTFMaterialList gGLTFMaterialList;


