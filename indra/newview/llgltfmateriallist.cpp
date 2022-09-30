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
#include "llfilesystem.h"
#include "llsdserialize.h"
#include "lltinygltfhelper.h"

#include "tinygltf/tiny_gltf.h"
#include <strstream>

LLGLTFMaterialList gGLTFMaterialList;

LLGLTFMaterial* LLGLTFMaterialList::getMaterial(const LLUUID& id)
{
    List::iterator iter = mList.find(id);
    if (iter == mList.end())
    {
        LLGLTFMaterial* mat = new LLGLTFMaterial();
        mList[id] = mat;

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

                                tinygltf::TinyGLTF gltf;
                                tinygltf::TinyGLTF loader;
                                std::string        error_msg;
                                std::string        warn_msg;

                                tinygltf::Model model_in;

                                if (loader.LoadASCIIFromString(&model_in, &error_msg, &warn_msg, data.c_str(), data.length(), ""))
                                {
                                    LLTinyGLTFHelper::setFromModel(mat, model_in, 0);
                                }
                                else
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

                mat->unref();
            }, nullptr);
        
        return mat;
    }

    return iter->second;
}

void LLGLTFMaterialList::addMaterial(const LLUUID& id, LLGLTFMaterial* material)
{
    mList[id] = material;
}

void LLGLTFMaterialList::removeMaterial(const LLUUID& id)
{
    mList.erase(id);
}

