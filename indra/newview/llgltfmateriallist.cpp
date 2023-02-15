/**
 * @file   llgltfmateriallist.cpp
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

#include "llgltfmateriallist.h"

#include "llagent.h"
#include "llassetstorage.h"
#include "lldispatcher.h"
#include "llfetchedgltfmaterial.h"
#include "llfilesystem.h"
#include "llsdserialize.h"
#include "lltinygltfhelper.h"
#include "llviewercontrol.h"
#include "llviewergenericmessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llcorehttputil.h"
#include "llagent.h"
#include "llvocache.h"
#include "llworld.h"

#include "tinygltf/tiny_gltf.h"
#include <strstream>

#include "json/reader.h"
#include "json/value.h"

#include <unordered_set>

LLGLTFMaterialList gGLTFMaterialList;

LLGLTFMaterialList::modify_queue_t LLGLTFMaterialList::sModifyQueue;
LLGLTFMaterialList::apply_queue_t LLGLTFMaterialList::sApplyQueue;
LLSD LLGLTFMaterialList::sUpdates;

const LLUUID LLGLTFMaterialList::BLANK_MATERIAL_ASSET_ID("968cbad0-4dad-d64e-71b5-72bf13ad051a");

#ifdef SHOW_ASSERT
// return true if given data is (probably) valid update message for ModifyMaterialParams capability
static bool is_valid_update(const LLSD& data)
{
    llassert(data.isMap());

    U32 count = 0;

    if (data.has("object_id"))
    {
        if (!data["object_id"].isUUID())
        {
            LL_WARNS() << "object_id is not a UUID" << LL_ENDL;
            return false;
        }
        ++count;
    }
    else
    { 
        LL_WARNS() << "Missing required parameter: object_id" << LL_ENDL;
        return false;
    }

    if (data.has("side"))
    {
        if (!data["side"].isInteger())
        {
            LL_WARNS() << "side is not an integer" << LL_ENDL;
            return false;
        }

        if (data["side"].asInteger() < -1)
        {
            LL_WARNS() << "side is invalid" << LL_ENDL;
        }
        ++count;
    }
    else
    { 
        LL_WARNS() << "Missing required parameter: side" << LL_ENDL;
        return false;
    }

    if (data.has("gltf_json"))
    {
        if (!data["gltf_json"].isString())
        {
            LL_WARNS() << "gltf_json is not a string" << LL_ENDL;
            return false;
        }
        ++count;
    }

    if (data.has("asset_id"))
    {
        if (!data["asset_id"].isUUID())
        {
            LL_WARNS() << "asset_id is not a UUID" << LL_ENDL;
            return false;
        }
        ++count;
    }

    if (count < 3)
    { 
        LL_WARNS() << "Only specified object_id and side, update won't actually change anything and is just noise" << LL_ENDL;
        return false;
    }

    if (data.size() != count)
    {
        LL_WARNS() << "update data contains unrecognized parameters" << LL_ENDL;
        return false;
    }

    return true;
}
#endif

class LLGLTFMaterialOverrideDispatchHandler : public LLDispatchHandler
{
    LOG_CLASS(LLGLTFMaterialOverrideDispatchHandler);
public:
    LLGLTFMaterialOverrideDispatchHandler() = default;
    ~LLGLTFMaterialOverrideDispatchHandler() override = default;

    void addCallback(void(*callback)(const LLUUID& object_id, S32 side))
    {
        mSelectionCallbacks.push_back(callback);
    }

    bool operator()(const LLDispatcher* dispatcher, const std::string& key, const LLUUID& invoice, const sparam_t& strings) override
    {
        LL_PROFILE_ZONE_SCOPED;
        // receive override data from simulator via LargeGenericMessage
        // message should have:
        //  object_id - UUID of LLViewerObject
        //  side - S32 index of texture entry
        //  gltf_json - String of GLTF json for override data


        LLSD message;

        sparam_t::const_iterator it = strings.begin();
        if (it != strings.end())
        {
            const std::string& llsdRaw = *it++;
            std::istringstream llsdData(llsdRaw);
            if (!LLSDSerialize::deserialize(message, llsdData, llsdRaw.length()))
            {
                LL_WARNS() << "LLGLTFMaterialOverrideDispatchHandler: Attempted to read parameter data into LLSD but failed:" << llsdRaw << LL_ENDL;
            }
        }
        else
        {
            // malformed message, nothing we can do to handle it
            LL_DEBUGS("GLTF") << "Empty message" << LL_ENDL;
            return false;
        }

        LLGLTFOverrideCacheEntry object_override;
        if (!object_override.fromLLSD(message))
        {
            // malformed message, nothing we can do to handle it
            LL_DEBUGS("GLTF") << "Message without id:" << message << LL_ENDL;
            return false;
        }

        // Cache the data
        {
            LL_DEBUGS("GLTF") << "material overrides cache" << LL_ENDL;

            // default to main region if message doesn't specify
            LLViewerRegion * region = gAgent.getRegion();;

            if (object_override.mHasRegionHandle)
            {
                // TODO start requiring this once server sends this for all messages
                region = LLWorld::instance().getRegionFromHandle(object_override.mRegionHandle);
            }

            if (region)
            {
                region->cacheFullUpdateGLTFOverride(object_override);
            }
            else
            {
                LL_WARNS("GLTF") << "could not access region for material overrides message cache, region_handle: " << LL_ENDL;
            }
        }
        applyData(object_override);
        return true;
    }

    void doSelectionCallbacks(const LLUUID& object_id, S32 side)
    {
        for (auto& callback : mSelectionCallbacks)
        {
            callback(object_id, side);
        }
    }

    void applyData(const LLGLTFOverrideCacheEntry &object_override)
    {
        // Parse the data

        LL::WorkQueue::ptr_t main_queue = LL::WorkQueue::getInstance("mainloop");
        LL::WorkQueue::ptr_t general_queue = LL::WorkQueue::getInstance("General");

        struct ReturnData
        {
        public:
            LLPointer<LLGLTFMaterial> mMaterial;
            S32 mSide;
            bool mSuccess;
        };

        // fromJson() is performance heavy offload to a thread.
        main_queue->postTo(
            general_queue,
            [object_override]() // Work done on general queue
        {
            std::vector<ReturnData> results;

            if (!object_override.mSides.empty())
            {
                results.reserve(object_override.mSides.size());
                // parse json
                std::map<S32, std::string>::const_iterator iter = object_override.mSides.begin();
                std::map<S32, std::string>::const_iterator end = object_override.mSides.end();
                while (iter != end)
                {
                    LLPointer<LLGLTFMaterial> override_data = new LLGLTFMaterial();
                    std::string warn_msg, error_msg;

                    bool success = override_data->fromJSON(iter->second, warn_msg, error_msg);

                    ReturnData result;
                    result.mSuccess = success;
                    result.mSide = iter->first;

                    if (success)
                    {
                        result.mMaterial = override_data;
                    }
                    else
                    {
                        LL_WARNS("GLTF") << "failed to parse GLTF override data.  errors: " << error_msg << " | warnings: " << warn_msg << LL_ENDL;
                    }

                    results.push_back(result);
                    iter++;
                }
            }
            return results;
        },
            [object_override, this](std::vector<ReturnData> results) // Callback to main thread
            {

            LLViewerObject * obj = gObjectList.findObject(object_override.mObjectId);

            if (results.size() > 0 )
            {
                std::unordered_set<S32> side_set;

                for (int i = 0; i < results.size(); ++i)
                {
                    if (results[i].mSuccess)
                    {
                        // flag this side to not be nulled out later
                        side_set.insert(results[i].mSide);

                        if (!obj || !obj->setTEGLTFMaterialOverride(results[i].mSide, results[i].mMaterial))
                        {
                            // object not ready to receive override data, queue for later
                            gGLTFMaterialList.queueOverrideUpdate(object_override.mObjectId, results[i].mSide, results[i].mMaterial);
                        }
                        else if (obj && obj->getTE(results[i].mSide) && obj->getTE(results[i].mSide)->isSelected())
                        {
                            doSelectionCallbacks(object_override.mObjectId, results[i].mSide);
                        }
                    }
                    else
                    {
                        // unblock material editor
                        if (obj && obj->getTE(results[i].mSide) && obj->getTE(results[i].mSide)->isSelected())
                        {
                            doSelectionCallbacks(object_override.mObjectId, results[i].mSide);
                        }
                    }
                }

                if (obj && side_set.size() != obj->getNumTEs())
                { // object exists and at least one texture entry needs to have its override data nulled out
                    for (int i = 0; i < obj->getNumTEs(); ++i)
                    {
                        if (side_set.find(i) == side_set.end())
                        {
                            obj->setTEGLTFMaterialOverride(i, nullptr);
                            if (obj->getTE(i) && obj->getTE(i)->isSelected())
                            {
                                doSelectionCallbacks(object_override.mObjectId, i);
                            }
                        }
                    }
                }
            }
            else if (obj)
            { // override list was empty or an error occurred, null out all overrides for this object
                for (int i = 0; i < obj->getNumTEs(); ++i)
                {
                    obj->setTEGLTFMaterialOverride(i, nullptr);
                    if (obj->getTE(i) && obj->getTE(i)->isSelected())
                    {
                        doSelectionCallbacks(obj->getID(), i);
                    }
                }
            }
        });
    }

private:

    std::vector<void(*)(const LLUUID& object_id, S32 side)> mSelectionCallbacks;
};

namespace
{
    LLGLTFMaterialOverrideDispatchHandler handle_gltf_override_message;
}

void LLGLTFMaterialList::queueOverrideUpdate(const LLUUID& id, S32 side, LLGLTFMaterial* override_data)
{
    override_list_t& overrides = mQueuedOverrides[id];
    
    if (overrides.size() < side + 1)
    {
        overrides.resize(side + 1);
    }

    overrides[side] = override_data;
}

void LLGLTFMaterialList::applyQueuedOverrides(LLViewerObject* obj)
{
    LL_PROFILE_ZONE_SCOPED;
    const LLUUID& id = obj->getID();
    auto iter = mQueuedOverrides.find(id);

    if (iter != mQueuedOverrides.end())
    {
        override_list_t& overrides = iter->second;
        for (int i = 0; i < overrides.size(); ++i)
        {
            if (overrides[i].notNull())
            {
                if (!obj->getTE(i) || !obj->getTE(i)->getGLTFMaterial())
                { // object doesn't have its base GLTF material yet, don't apply override (yet)
                    return;
                }
                obj->setTEGLTFMaterialOverride(i, overrides[i]);
                if (obj->getTE(i)->isSelected())
                {
                    handle_gltf_override_message.doSelectionCallbacks(id, i);
                }
            }
        }

        mQueuedOverrides.erase(iter);
    }
}

void LLGLTFMaterialList::queueModify(const LLViewerObject* obj, S32 side, const LLGLTFMaterial* mat)
{
    if (obj && obj->getRenderMaterialID(side).notNull())
    {
        if (mat == nullptr)
        {
            sModifyQueue.push_back({ obj->getID(), side, LLGLTFMaterial(), false });
        }
        else
        {
            sModifyQueue.push_back({ obj->getID(), side, *mat, true });
        }
    }
}

void LLGLTFMaterialList::queueApply(const LLViewerObject* obj, S32 side, const LLUUID& asset_id)
{
    const LLGLTFMaterial* material_override = obj->getTE(side)->getGLTFMaterialOverride();
    if (material_override)
    {
        LLGLTFMaterial* cleared_override = new LLGLTFMaterial(*material_override);
        cleared_override->setBaseMaterial();
        sApplyQueue.push_back({ obj->getID(), side, asset_id, cleared_override });
    }
    else
    {
        sApplyQueue.push_back({ obj->getID(), side, asset_id, nullptr });
    }
}

void LLGLTFMaterialList::queueUpdate(const LLSD& data)
{
    llassert(is_valid_update(data));

    if (!sUpdates.isArray())
    {
        sUpdates = LLSD::emptyArray();
    }
    
    sUpdates[sUpdates.size()] = data;
}

void LLGLTFMaterialList::flushUpdates(void(*done_callback)(bool))
{
    LLSD& data = sUpdates;

    S32 i = data.size();

    for (ModifyMaterialData& e : sModifyQueue)
    {
#ifdef SHOW_ASSERT
        // validate object has a material id
        LLViewerObject* obj = gObjectList.findObject(e.object_id);
        llassert(obj && obj->getRenderMaterialID(e.side).notNull());
#endif

        data[i]["object_id"] = e.object_id;
        data[i]["side"] = e.side;
         
        if (e.has_override)
        {
            data[i]["gltf_json"] = e.override_data.asJSON();
        }
        else
        {
            // Clear all overrides
            data[i]["gltf_json"] = "";
        }

        llassert(is_valid_update(data[i]));
        ++i;
    }
    sModifyQueue.clear();

    for (auto& e : sApplyQueue)
    {
        data[i]["object_id"] = e.object_id;
        data[i]["side"] = e.side;
        data[i]["asset_id"] = e.asset_id;
        if (e.override_data)
        {
            data[i]["gltf_json"] = e.override_data->asJSON();
        }
        else
        {
            // Clear all overrides
            data[i]["gltf_json"] = "";
        }

        llassert(is_valid_update(data[i]));
        ++i;
    }
    sApplyQueue.clear();

#if 0 // debug output of data being sent to capability
    std::stringstream str;
    LLSDSerialize::serialize(data, str, LLSDSerialize::LLSD_NOTATION, LLSDFormatter::OPTIONS_PRETTY);
    LL_INFOS() << "\n" << str.str() << LL_ENDL;
#endif

    if (sUpdates.size() > 0)
    {
        LLCoros::instance().launch("modifyMaterialCoro",
            std::bind(&LLGLTFMaterialList::modifyMaterialCoro,
                gAgent.getRegionCapability("ModifyMaterialParams"),
                sUpdates,
                done_callback));

        sUpdates = LLSD::emptyArray();
    }
}

void LLGLTFMaterialList::addSelectionUpdateCallback(void(*update_callback)(const LLUUID& object_id, S32 side))
{
    handle_gltf_override_message.addCallback(update_callback);
}

class AssetLoadUserData
{
public:
    AssetLoadUserData() {}
    tinygltf::Model mModelIn;
    LLPointer<LLFetchedGLTFMaterial> mMaterial;
};

void LLGLTFMaterialList::onAssetLoadComplete(const LLUUID& id, LLAssetType::EType asset_type, void* user_data, S32 status, LLExtStat ext_status)
{
    LL_PROFILE_ZONE_NAMED("gltf asset callback");
    AssetLoadUserData* asset_data = (AssetLoadUserData*)user_data;

    if (status != LL_ERR_NOERR)
    {
        LL_WARNS("GLTF") << "Error getting material asset data: " << LLAssetStorage::getErrorString(status) << " (" << status << ")" << LL_ENDL;
        asset_data->mMaterial->materialComplete();
        delete asset_data;
    }
    else
    {

        LL::WorkQueue::ptr_t main_queue = LL::WorkQueue::getInstance("mainloop");
        LL::WorkQueue::ptr_t general_queue = LL::WorkQueue::getInstance("General");

        typedef std::pair<U32, tinygltf::Model> return_data_t;

        main_queue->postTo(
            general_queue,
            [id, asset_type, asset_data]() // Work done on general queue
        {
            std::vector<char> buffer;
            {
                LL_PROFILE_ZONE_NAMED("gltf read asset");
                LLFileSystem file(id, asset_type, LLFileSystem::READ);
                auto size = file.getSize();
                if (!size)
                {
                    return false;
                }

                buffer.resize(size);
                file.read((U8*)&buffer[0], buffer.size());
            }

            {
                LL_PROFILE_ZONE_NAMED("gltf deserialize asset");

                LLSD asset;

                // read file into buffer
                std::istrstream str(&buffer[0], buffer.size());

                if (LLSDSerialize::deserialize(asset, str, buffer.size()))
                {
                    if (asset.has("version") && LLGLTFMaterial::isAcceptedVersion(asset["version"].asString()))
                    {
                        if (asset.has("type") && asset["type"].asString() == LLGLTFMaterial::ASSET_TYPE)
                        {
                            if (asset.has("data") && asset["data"].isString())
                            {
                                std::string data = asset["data"];

                                std::string warn_msg, error_msg;

                                LL_PROFILE_ZONE_SCOPED;
                                tinygltf::TinyGLTF gltf;

                                if (!gltf.LoadASCIIFromString(&asset_data->mModelIn, &error_msg, &warn_msg, data.c_str(), data.length(), ""))
                                {
                                    LL_WARNS("GLTF") << "Failed to decode material asset: "
                                        << LL_NEWLINE
                                        << warn_msg
                                        << LL_NEWLINE
                                        << error_msg
                                        << LL_ENDL;
                                    return false;
                                }
                                return true;
                            }
                        }
                    }
                }
                else
                {
                    LL_WARNS("GLTF") << "Failed to deserialize material LLSD" << LL_ENDL;
                }
            }

            return false;
        },
            [id, asset_data](bool result) // Callback to main thread
            {

            if (result)
            {
                asset_data->mMaterial->setFromModel(asset_data->mModelIn, 0/*only one index*/);
            }
            else
            {
                LL_DEBUGS("GLTF") << "Failed to get material " << id << LL_ENDL;
            }

            asset_data->mMaterial->materialComplete();

            delete asset_data;
        });
    }
}

LLFetchedGLTFMaterial* LLGLTFMaterialList::getMaterial(const LLUUID& id)
{
    LL_PROFILE_ZONE_SCOPED;
    uuid_mat_map_t::iterator iter = mList.find(id);
    if (iter == mList.end())
    {
        LL_PROFILE_ZONE_NAMED("gltf fetch")
        LLFetchedGLTFMaterial* mat = new LLFetchedGLTFMaterial();
        mList[id] = mat;

        if (!mat->mFetching)
        {
            mat->materialBegin();

            AssetLoadUserData *user_data = new AssetLoadUserData();
            user_data->mMaterial = mat;

            gAssetStorage->getAssetData(id, LLAssetType::AT_MATERIAL, onAssetLoadComplete, (void*)user_data);
        }
        
        return mat;
    }

    return iter->second;
}

void LLGLTFMaterialList::addMaterial(const LLUUID& id, LLFetchedGLTFMaterial* material)
{
    mList[id] = material;
}

void LLGLTFMaterialList::removeMaterial(const LLUUID& id)
{
    mList.erase(id);
}

void LLGLTFMaterialList::flushMaterials()
{
    // Similar variant to what textures use
    static const S32 MIN_UPDATE_COUNT = gSavedSettings.getS32("TextureFetchUpdateMinCount");       // default: 32
    //update MIN_UPDATE_COUNT or 5% of materials, whichever is greater
    U32 update_count = llmax((U32)MIN_UPDATE_COUNT, (U32)mList.size() / 20);
    update_count = llmin(update_count, (U32)mList.size());

    const F64 MAX_INACTIVE_TIME = 30.f;
    F64 cur_time = LLTimer::getTotalSeconds();

    // advance iter one past the last key we updated
    uuid_mat_map_t::iterator iter = mList.find(mLastUpdateKey);
    if (iter != mList.end()) {
        ++iter;
    }

    while (update_count-- > 0)
    {
        if (iter == mList.end())
        {
            iter = mList.begin();
        }

        LLPointer<LLFetchedGLTFMaterial> material = iter->second;
        if (material->getNumRefs() == 2) // this one plus one from the list
        {

            if (!material->mActive
                && cur_time > material->mExpectedFlusTime)
            {
                iter = mList.erase(iter);
            }
            else
            {
                if (material->mActive)
                {
                    material->mExpectedFlusTime = cur_time + MAX_INACTIVE_TIME;
                    material->mActive = false;
                }
                ++iter;
            }
        }
        else
        {
            material->mActive = true;
            ++iter;
        }
    }

    if (iter != mList.end())
    {
        mLastUpdateKey = iter->first;
    }
    else
    {
        mLastUpdateKey.setNull();
    }

    {
        using namespace LLStatViewer;
        sample(NUM_MATERIALS, mList.size());
    }
}

// static
void LLGLTFMaterialList::registerCallbacks()
{
    gGenericDispatcher.addHandler("GLTFMaterialOverride", &handle_gltf_override_message);
}

// static
void LLGLTFMaterialList::modifyMaterialCoro(std::string cap_url, LLSD overrides, void(*done_callback)(bool) )
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("modifyMaterialCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    httpOpts->setFollowRedirects(true);

    LL_DEBUGS("GLTF") << "Applying override via ModifyMaterialParams cap: " << overrides << LL_ENDL;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, cap_url, overrides, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    bool success = true;
    if (!status)
    {
        LL_WARNS("GLTF") << "Failed to modify material." << LL_ENDL;
        success = false;
    }
    else if (!result["success"].asBoolean())
    {
        LL_WARNS("GLTF") << "Failed to modify material: " << result["message"] << LL_ENDL;
        success = false;
    }

    if (done_callback)
    {
        done_callback(success);
    }
}

void LLGLTFMaterialList::loadCacheOverrides(const LLGLTFOverrideCacheEntry& override)
{
    handle_gltf_override_message.applyData(override);
}
