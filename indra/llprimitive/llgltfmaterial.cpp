/**
 * @file llgltfmaterial.cpp
 * @brief Material definition
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

#include "linden_common.h"

#include "llgltfmaterial.h"

#include "tiny_gltf.h"

bool LLGLTFMaterial::fromJSON(const std::string& json, std::string& warn_msg, std::string& error_msg)
{
    tinygltf::TinyGLTF gltf;
    
    tinygltf::Model model_in;

    if (gltf.LoadASCIIFromString(&model_in, &error_msg, &warn_msg, json.c_str(), json.length(), ""))
    {
        setFromModel(model_in, 0);

        //DEBUG generate json and print
        LL_INFOS() << asJSON(true) << LL_ENDL;

        return true;
    }

    return false;
}

std::string LLGLTFMaterial::asJSON(bool prettyprint) const
{
    tinygltf::TinyGLTF gltf;
    tinygltf::Model model_out;

    std::ostringstream str;

    writeToModel(model_out, 0);

    gltf.WriteGltfSceneToStream(&model_out, str, prettyprint, false);

    return str.str();
}

void LLGLTFMaterial::setFromModel(const tinygltf::Model& model, S32 mat_index)
{
    if (model.materials.size() <= mat_index)
    {
        return;
    }

    const tinygltf::Material& material_in = model.materials[mat_index];

    // get base color texture
    S32 tex_index = material_in.pbrMetallicRoughness.baseColorTexture.index;
    if (tex_index >= 0)
    {
        mBaseColorId.set(model.images[tex_index].uri);
    }
    else
    {
        mBaseColorId.setNull();
    }

    // get normal map
    tex_index = material_in.normalTexture.index;
    if (tex_index >= 0)
    {
        mNormalId.set(model.images[tex_index].uri);
    }
    else
    {
        mNormalId.setNull();
    }

    // get metallic-roughness texture
    tex_index = material_in.pbrMetallicRoughness.metallicRoughnessTexture.index;
    if (tex_index >= 0)
    {
        mMetallicRoughnessId.set(model.images[tex_index].uri);
    }
    else
    {
        mMetallicRoughnessId.setNull();
    }

    // get emissive texture
    tex_index = material_in.emissiveTexture.index;
    if (tex_index >= 0)
    {
        mEmissiveId.set(model.images[tex_index].uri);
    }
    else
    {
        mEmissiveId.setNull();
    }

    setAlphaMode(material_in.alphaMode);
    mAlphaCutoff = llclamp((F32)material_in.alphaCutoff, 0.f, 1.f);

    mBaseColor.set(material_in.pbrMetallicRoughness.baseColorFactor);
    mEmissiveColor.set(material_in.emissiveFactor);

    mMetallicFactor = llclamp((F32)material_in.pbrMetallicRoughness.metallicFactor, 0.f, 1.f);
    mRoughnessFactor = llclamp((F32)material_in.pbrMetallicRoughness.roughnessFactor, 0.f, 1.f);

    mDoubleSided = material_in.doubleSided;
}

void LLGLTFMaterial::writeToModel(tinygltf::Model& model, S32 mat_index) const
{
    if (model.materials.size() < mat_index+1)
    {
        model.materials.resize(mat_index + 1);
    }

    tinygltf::Material& material_out = model.materials[mat_index];

    // set base color texture
    if (mBaseColorId.notNull())
    {
        U32 idx = model.images.size();
        model.images.resize(idx + 1);
        model.textures.resize(idx + 1);
        
        material_out.pbrMetallicRoughness.baseColorTexture.index = idx;
        model.textures[idx].source = idx;
        model.images[idx].uri = mBaseColorId.asString();
    }

    // set normal texture
    if (mNormalId.notNull())
    {
        U32 idx = model.images.size();
        model.images.resize(idx + 1);
        model.textures.resize(idx + 1);

        material_out.normalTexture.index = idx;
        model.textures[idx].source = idx;
        model.images[idx].uri = mNormalId.asString();
    }
    
    // set metallic-roughness texture
    if (mMetallicRoughnessId.notNull())
    {
        U32 idx = model.images.size();
        model.images.resize(idx + 1);
        model.textures.resize(idx + 1);

        material_out.pbrMetallicRoughness.metallicRoughnessTexture.index = idx;
        model.textures[idx].source = idx;
        model.images[idx].uri = mMetallicRoughnessId.asString();
    }

    // set emissive texture
    if (mEmissiveId.notNull())
    {
        U32 idx = model.images.size();
        model.images.resize(idx + 1);
        model.textures.resize(idx + 1);

        material_out.emissiveTexture.index = idx;
        model.textures[idx].source = idx;
        model.images[idx].uri = mEmissiveId.asString();
    }

    material_out.alphaMode = getAlphaMode();
    material_out.alphaCutoff = mAlphaCutoff;
    
    mBaseColor.write(material_out.pbrMetallicRoughness.baseColorFactor);
    mEmissiveColor.write(material_out.emissiveFactor);

    material_out.pbrMetallicRoughness.metallicFactor = mMetallicFactor;
    material_out.pbrMetallicRoughness.roughnessFactor = mRoughnessFactor;

    material_out.doubleSided = mDoubleSided;
}

