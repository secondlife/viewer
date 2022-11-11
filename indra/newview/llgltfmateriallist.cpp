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

#include "llassetstorage.h"
#include "lldispatcher.h"
#include "llfetchedgltfmaterial.h"
#include "llfilesystem.h"
#include "llmaterialeditor.h"
#include "llsdserialize.h"
#include "lltinygltfhelper.h"
#include "llviewercontrol.h"
#include "llviewergenericmessage.h"
#include "llviewerobjectlist.h"
#include "llviewerstats.h"
#include "llcorehttputil.h"
#include "llagent.h"

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
        if (it != strings.end()) {
            const std::string& llsdRaw = *it++;
            std::istringstream llsdData(llsdRaw);
            if (!LLSDSerialize::deserialize(message, llsdData, llsdRaw.length()))
            {
                LL_WARNS() << "LLGLTFMaterialOverrideDispatchHandler: Attempted to read parameter data into LLSD but failed:" << llsdRaw << LL_ENDL;
            }
        }
        
        LL::WorkQueue::ptr_t main_queue = LL::WorkQueue::getInstance("mainloop");
        LL::WorkQueue::ptr_t general_queue = LL::WorkQueue::getInstance("General");

        struct ReturnData
        {
        public:
            std::vector<LLPointer<LLGLTFMaterial> > mMaterialVector;
            std::vector<bool> mResults;
        };

        // fromJson() is performance heavy offload to a thread.
        main_queue->postTo(
            general_queue,
            [message]() // Work done on general queue
        {
            ReturnData result;

            if (message.has("sides") && message.has("gltf_json"))
            {
                LLSD& sides = message.get("sides");
                LLSD& gltf_json = message.get("gltf_json");

                if (sides.isArray() && gltf_json.isArray() &&
                    sides.size() != 0 &&
                    sides.size() == gltf_json.size())
                {
                    // message should be interpreted thusly:
                    ///  sides is a list of face indices
                    //   gltf_json is a list of corresponding json
                    //   any side not represented in "sides" has no override
                    result.mResults.resize(sides.size());
                    result.mMaterialVector.resize(sides.size());

                    // parse json
                    for (int i = 0; i < sides.size(); ++i)
                    {
                        LLPointer<LLGLTFMaterial> override_data = new LLGLTFMaterial();

                        std::string gltf_json_str = gltf_json[i].asString();

                        std::string warn_msg, error_msg;

                        bool success = override_data->fromJSON(gltf_json_str, warn_msg, error_msg);

                        result.mResults[i] = success;

                        if (success)
                        {
                            result.mMaterialVector[i] = override_data;
                        }
                        else
                        {
                            LL_WARNS() << "failed to parse GLTF override data.  errors: " << error_msg << " | warnings: " << warn_msg << LL_ENDL;
                        }
                    }
                }
            }
            return result;
        },
            [message](ReturnData result) // Callback to main thread
            mutable {

            LLUUID object_id = message["object_id"].asUUID();
            LLSD& sides = message["sides"];
            LLViewerObject * obj = gObjectList.findObject(object_id);
            std::unordered_set<S32> side_set;

            if (result.mResults.size() > 0 )
            {
                for (int i = 0; i < result.mResults.size(); ++i)
                {
                    if (result.mResults[i])
                    {
                        S32 side = sides[i].asInteger();
                        // flag this side to not be nulled out later
                        side_set.insert(sides[i]);

                        if (!obj || !obj->setTEGLTFMaterialOverride(side, result.mMaterialVector[i]))
                        {
                            // object not ready to receive override data, queue for later
                            gGLTFMaterialList.queueOverrideUpdate(object_id, side, result.mMaterialVector[i]);
                        }
                        else if (obj && obj->isAnySelected())
                        {
                            LLMaterialEditor::updateLive(object_id, side);
                        }
                    }
                    else
                    {
                        // unblock material editor
                        if (obj && obj->isAnySelected())
                        {
                            LLMaterialEditor::updateLive(object_id, sides[i].asInteger());
                        }
                    }
                }

                if (obj && side_set.size() != obj->getNumTEs())
                { // object exists and at least one texture entry needs to have its override data nulled out
                    bool object_has_selection = obj->isAnySelected();
                    for (int i = 0; i < obj->getNumTEs(); ++i)
                    {
                        if (side_set.find(i) == side_set.end())
                        {
                            obj->setTEGLTFMaterialOverride(i, nullptr);
                            if (object_has_selection)
                            {
                                LLMaterialEditor::updateLive(object_id, i);
                            }
                        }
                    }
                }
            }
            else if (obj)
            { // override list was empty or an error occurred, null out all overrides for this object
                bool object_has_selection = obj->isAnySelected();
                for (int i = 0; i < obj->getNumTEs(); ++i)
                {
                    obj->setTEGLTFMaterialOverride(i, nullptr);
                    if (object_has_selection)
                    {
                        LLMaterialEditor::updateLive(obj->getID(), i);
                    }
                }
            }
        });

        return true;
    }
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
        bool object_has_selection = obj->isAnySelected();
        override_list_t& overrides = iter->second;
        for (int i = 0; i < overrides.size(); ++i)
        {
            if (overrides[i].notNull())
            {
                if (!obj->getTE(i)->getGLTFMaterial())
                { // object doesn't have its base GLTF material yet, don't apply override (yet)
                    return;
                }
                obj->setTEGLTFMaterialOverride(i, overrides[i]);
                if (object_has_selection)
                {
                    LLMaterialEditor::updateLive(id, i);
                }
            }
        }

        mQueuedOverrides.erase(iter);
    }
}

void LLGLTFMaterialList::queueModify(const LLUUID& id, S32 side, const LLGLTFMaterial* mat)
{
    if (mat == nullptr)
    {
        sModifyQueue.push_back({ id, side, LLGLTFMaterial(), false });
    }
    else
    {
        sModifyQueue.push_back({ id, side, *mat, true});
    }
}

void LLGLTFMaterialList::queueApply(const LLUUID& object_id, S32 side, const LLUUID& asset_id)
{
    sApplyQueue.push_back({ object_id, side, asset_id});
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

    for (auto& e : sModifyQueue)
    {
        data[i]["object_id"] = e.object_id;
        data[i]["side"] = e.side;
         
        if (e.has_override)
        {
            data[i]["gltf_json"] = e.override_data.asJSON();
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
        data[i]["gltf_json"] = ""; // null out any existing overrides when applying a material asset

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
        LL_WARNS() << "Error getting material asset data: " << LLAssetStorage::getErrorString(status) << " (" << status << ")" << LL_ENDL;
        asset_data->mMaterial->mFetching = false;
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
                    if (asset.has("version") && asset["version"] == "1.0")
                    {
                        if (asset.has("type") && asset["type"].asString() == "GLTF 2.0")
                        {
                            if (asset.has("data") && asset["data"].isString())
                            {
                                std::string data = asset["data"];

                                std::string warn_msg, error_msg;

                                LL_PROFILE_ZONE_SCOPED;
                                tinygltf::TinyGLTF gltf;

                                if (!gltf.LoadASCIIFromString(&asset_data->mModelIn, &error_msg, &warn_msg, data.c_str(), data.length(), ""))
                                {
                                    LL_WARNS() << "Failed to decode material asset: "
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
                    LL_WARNS() << "Failed to deserialize material LLSD" << LL_ENDL;
                }
            }

            return false;
        },
            [id, asset_data](bool result) // Callback to main thread
            mutable {

            if (result)
            {
                asset_data->mMaterial->setFromModel(asset_data->mModelIn, 0/*only one index*/);
            }
            else
            {
                LL_DEBUGS() << "Failed to get material " << id << LL_ENDL;
            }
            asset_data->mMaterial->mFetching = false;
            delete asset_data;
        });
    }
}

LLGLTFMaterial* LLGLTFMaterialList::getMaterial(const LLUUID& id)
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
            mat->mFetching = true;

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

    LL_DEBUGS() << "Applying override via ModifyMaterialParams cap: " << overrides << LL_ENDL;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, cap_url, overrides, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    bool success = true;
    if (!status)
    {
        LL_WARNS() << "Failed to modify material." << LL_ENDL;
        success = false;
    }
    else if (!result["success"].asBoolean())
    {
        LL_WARNS() << "Failed to modify material: " << result["message"] << LL_ENDL;
        success = false;
    }

    if (done_callback)
    {
        done_callback(success);
    }
}
