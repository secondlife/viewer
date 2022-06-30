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

#include "tinygltf/tiny_gltf.h"
#include <strstream>

LLGLTFMaterialList gGLTFMaterialList;

static LLColor4 get_color(const std::vector<double>& in)
{
    LLColor4 out;
    for (S32 i = 0; i < llmin((S32)in.size(), 4); ++i)
    {
        out.mV[i] = in[i];
    }

    return out;
}

static void set_from_model(LLGLTFMaterial* mat, tinygltf::Model& model)
{
    
    S32 index;
    
    auto& material_in = model.materials[0];

    // get albedo texture
    index = material_in.pbrMetallicRoughness.baseColorTexture.index;
    if (index >= 0)
    {
        mat->mAlbedoId.set(model.images[index].uri);
    }
    else
    {
        mat->mAlbedoId.setNull();
    }

    // get normal map
    index = material_in.normalTexture.index;
    if (index >= 0)
    {
        mat->mNormalId.set(model.images[index].uri);
    }
    else
    {
        mat->mNormalId.setNull();
    }

    // get metallic-roughness texture
    index = material_in.pbrMetallicRoughness.metallicRoughnessTexture.index;
    if (index >= 0)
    {
        mat->mMetallicRoughnessId.set(model.images[index].uri);
    }
    else
    {
        mat->mMetallicRoughnessId.setNull();
    }

    // get emissive texture
    index = material_in.emissiveTexture.index;
    if (index >= 0)
    {
        mat->mEmissiveId.set(model.images[index].uri);
    }
    else
    {
        mat->mEmissiveId.setNull();
    }

    mat->setAlphaMode(material_in.alphaMode);
    mat->mAlphaCutoff = material_in.alphaCutoff;

    mat->mAlbedoColor = get_color(material_in.pbrMetallicRoughness.baseColorFactor);
    mat->mEmissiveColor = get_color(material_in.emissiveFactor);

    mat->mMetallicFactor = material_in.pbrMetallicRoughness.metallicFactor;
    mat->mRoughnessFactor = material_in.pbrMetallicRoughness.roughnessFactor;

    mat->mDoubleSided = material_in.doubleSided;
}

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

                std::vector<char> buffer;
                buffer.resize(file.getSize());
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
                                    set_from_model(mat, model_in);
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

