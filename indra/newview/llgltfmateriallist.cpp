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
#include "llsdserialize.h"
#include "lltinygltfhelper.h"
#include "llviewercontrol.h"
#include "llviewergenericmessage.h"

#include "tinygltf/tiny_gltf.h"
#include <strstream>

namespace
{
    class LLGLTFOverrideDispatchHandler : public LLDispatchHandler
    {
    public:
        LLGLTFOverrideDispatchHandler() = default;
        ~LLGLTFOverrideDispatchHandler() override = default;

        bool operator()(const LLDispatcher* dispatcher, const std::string& key, const LLUUID& invoice, const sparam_t& strings) override
        {
            for (std::string const & s : strings) {
                LL_DEBUGS() << "received override: " << s << LL_ENDL;

#if 0
                // for now messages are coming in llsd
                LLSD override_data;
                std::istringstream input(s);
                LLSDSerialize::deserialize(override_data, input, s.length());
                LL_DEBUGS() << "deserialized override: " << override_data << LL_ENDL;
#else
                std::string warn_msg, error_msg;
                LLGLTFMaterial override_data;
                override_data.fromJSON(s, warn_msg, error_msg);
#endif
            }

            return true;
        }
    };
    LLGLTFOverrideDispatchHandler handle_gltf_override_message;
}

LLGLTFMaterialList gGLTFMaterialList;

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
    gGenericDispatcher.addHandler("GLTF", &handle_gltf_override_message);
}
