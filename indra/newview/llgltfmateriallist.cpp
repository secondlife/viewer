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
#include "llcorehttputil.h"
#include "tinygltf/tiny_gltf.h"
#include <strstream>

#include "json/reader.h"
#include "json/value.h"

#include <unordered_set>

LLGLTFMaterialList gGLTFMaterialList;

const LLUUID LLGLTFMaterialList::BLANK_MATERIAL_ASSET_ID("968cbad0-4dad-d64e-71b5-72bf13ad051a");

namespace
{
    class LLGLTFMaterialOverrideDispatchHandler : public LLDispatchHandler
    {
        LOG_CLASS(LLGLTFMaterialOverrideDispatchHandler);
    public:
        LLGLTFMaterialOverrideDispatchHandler() = default;
        ~LLGLTFMaterialOverrideDispatchHandler() override = default;

        bool operator()(const LLDispatcher* dispatcher, const std::string& key, const LLUUID& invoice, const sparam_t& strings) override
        {
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

            LLUUID object_id = message["object_id"].asUUID();
            
            LLViewerObject * obj = gObjectList.findObject(object_id); // NOTE: null object here does NOT mean nothing to do, parse message and queue results for later
            bool clear_all = true;

            if (message.has("sides") && message.has("gltf_json"))
            {
                LLSD& sides = message["sides"];
                LLSD& gltf_json = message["gltf_json"];

                if (sides.isArray() && gltf_json.isArray() &&
                    sides.size() != 0 &&
                    sides.size() == gltf_json.size())
                {
                    clear_all = false;

                    // message should be interpreted thusly:
                    ///  sides is a list of face indices
                    //   gltf_json is a list of corresponding json
                    //   any side not represented in "sides" has no override

                    // parse json
                    std::unordered_set<S32> side_set;

                    for (int i = 0; i < sides.size(); ++i)
                    {
                        LLPointer<LLGLTFMaterial> override_data = new LLGLTFMaterial();
                        
                        std::string gltf_json = message["gltf_json"][i].asString();

                        std::string warn_msg, error_msg;
                        
                        bool success = override_data->fromJSON(gltf_json, warn_msg, error_msg);

                        if (!success)
                        {
                            LL_WARNS() << "failed to parse GLTF override data.  errors: " << error_msg << " | warnings: " << warn_msg << LL_ENDL;
                        }
                        else
                        {
                            S32 side = sides[i].asInteger();
                            // flag this side to not be nulled out later
                            side_set.insert(sides[i]);

                            if (!obj || !obj->setTEGLTFMaterialOverride(side, override_data))
                            {
                                // object not ready to receive override data, queue for later
                                gGLTFMaterialList.queueOverrideUpdate(object_id, side, override_data);
                            }
                            else if (obj && obj->isAnySelected())
                            {
                                // Might want to cause a full selection
                                // update here instead of just an editor
                                LLMaterialEditor::updateLive();
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
                            }
                        }
                        if (obj->isAnySelected())
                        {
                            // Might want to cause a full selection
                            // update here instead of just an editor
                            LLMaterialEditor::updateLive();
                        }
                    }
                }
                else
                {
                    LL_WARNS() << "Malformed GLTF override message data: " << message << LL_ENDL;
                }
            }
            
            if (clear_all && obj)
            { // override list was empty or an error occurred, null out all overrides for this object
                for (int i = 0; i < obj->getNumTEs(); ++i)
                {
                    obj->setTEGLTFMaterialOverride(i, nullptr);
                }
                if (obj->isAnySelected())
                {
                    // Might want to cause a full selection
                    // update here instead of just an editor
                    LLMaterialEditor::updateLive();
                }
            }
            return true;
        }
    };
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
    const LLUUID& id = obj->getID();
    auto iter = mQueuedOverrides.find(id);

    if (iter != mQueuedOverrides.end())
    {
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
                if (obj->isAnySelected())
                {
                    // Might want to cause a full selection
                    // update here instead of just an editor
                    LLMaterialEditor::updateLive();
                }
            }
        }

        mQueuedOverrides.erase(iter);
    }
}

LLGLTFMaterial* LLGLTFMaterialList::getMaterial(const LLUUID& id)
{
    uuid_mat_map_t::iterator iter = mList.find(id);
    if (iter == mList.end())
    {
        LLFetchedGLTFMaterial* mat = new LLFetchedGLTFMaterial();
        mList[id] = mat;

        if (!mat->mFetching)
        {
            // if we do multiple getAssetData calls,
            // some will get distched, messing ref counter
            // Todo: get rid of mat->ref()
            mat->mFetching = true;
            mat->ref();

            gAssetStorage->getAssetData(id, LLAssetType::AT_MATERIAL,
                [=](const LLUUID& id, LLAssetType::EType asset_type, void* user_data, S32 status, LLExtStat ext_status)
            {
                if (status)
                {
                    LL_WARNS() << "Error getting material asset data: " << LLAssetStorage::getErrorString(status) << " (" << status << ")" << LL_ENDL;
                }

                LLFileSystem file(id, asset_type, LLFileSystem::READ);
                auto size = file.getSize();
                if (!size)
                {
                    LL_DEBUGS() << "Zero size material." << LL_ENDL;
                    mat->mFetching = false;
                    mat->unref();
                    return;
                }

                std::vector<char> buffer;
                buffer.resize(size);
                file.read((U8*)&buffer[0], buffer.size());

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

                                if (!mat->fromJSON(data, warn_msg, error_msg))
                                {
                                    LL_WARNS() << "Failed to decode material asset: " << LL_ENDL;
                                    LL_WARNS() << warn_msg << LL_ENDL;
                                    LL_WARNS() << error_msg << LL_ENDL;
                                }
                            }
                        }
                    }
                }
                else
                {
                    LL_WARNS() << "Failed to deserialize material LLSD" << LL_ENDL;
                }

                mat->mFetching = false;
                mat->unref();
            }, nullptr);
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
}

// static
void LLGLTFMaterialList::registerCallbacks()
{
    gGenericDispatcher.addHandler("GLTFMaterialOverride", &handle_gltf_override_message);
}

// static
void LLGLTFMaterialList::modifyMaterialCoro(std::string cap_url, LLSD overrides)
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

    if (!status)
    {
        LL_WARNS() << "Failed to modify material." << LL_ENDL;
    }
    else if (!result["success"].asBoolean())
    {
        LL_WARNS() << "Failed to modify material: " << result["message"] << LL_ENDL;
    }
}
