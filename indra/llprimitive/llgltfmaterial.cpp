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

#include "tinygltf/tiny_gltf.h"

LLGLTFMaterial::LLGLTFMaterial(const LLGLTFMaterial& rhs)
{
    *this = rhs;
}

LLGLTFMaterial& LLGLTFMaterial::operator=(const LLGLTFMaterial& rhs)
{
    //have to do a manual operator= because of LLRefCount 
    mBaseColorId = rhs.mBaseColorId;
    mNormalId = rhs.mNormalId;
    mMetallicRoughnessId = rhs.mMetallicRoughnessId;
    mEmissiveId = rhs.mEmissiveId;

    mBaseColor = rhs.mBaseColor;
    mEmissiveColor = rhs.mEmissiveColor;

    mMetallicFactor = rhs.mMetallicFactor;
    mRoughnessFactor = rhs.mRoughnessFactor;
    mAlphaCutoff = rhs.mAlphaCutoff;

    mDoubleSided = rhs.mDoubleSided;
    mAlphaMode = rhs.mAlphaMode;

    for (S32 i = 0; i < 3; ++i)
    {
        mTextureTransform[i] = rhs.mTextureTransform[i];
    }

    return *this;
}

bool LLGLTFMaterial::fromJSON(const std::string& json, std::string& warn_msg, std::string& error_msg)
{
#if 1
    tinygltf::TinyGLTF gltf;

    tinygltf::Model model_in;

    if (gltf.LoadASCIIFromString(&model_in, &error_msg, &warn_msg, json.c_str(), json.length(), ""))
    {
        setFromModel(model_in, 0);

        //DEBUG generate json and print
        LL_INFOS() << asJSON(true) << LL_ENDL;

        return true;
    }
#endif
    return false;
}

std::string LLGLTFMaterial::asJSON(bool prettyprint) const
{
#if 1
    tinygltf::TinyGLTF gltf;
    tinygltf::Model model_out;

    std::ostringstream str;

    writeToModel(model_out, 0);

    gltf.WriteGltfSceneToStream(&model_out, str, prettyprint, false);

    return str.str();
#else
    return "";
#endif
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
    if (model.materials.size() < mat_index + 1)
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

    model.asset.version = "2.0";
}


void LLGLTFMaterial::setBaseColorId(const LLUUID& id)
{
    mBaseColorId = id;
}

void LLGLTFMaterial::setNormalId(const LLUUID& id)
{
    mNormalId = id;
}

void LLGLTFMaterial::setMetallicRoughnessId(const LLUUID& id)
{
    mMetallicRoughnessId = id;
}

void LLGLTFMaterial::setEmissiveId(const LLUUID& id)
{
    mEmissiveId = id;
}

void LLGLTFMaterial::setBaseColorFactor(const LLColor3& baseColor, F32 transparency)
{
    mBaseColor.set(baseColor, transparency);
    mBaseColor.clamp();
}

void LLGLTFMaterial::setAlphaCutoff(F32 cutoff)
{
    mAlphaCutoff = llclamp(cutoff, 0.f, 1.f);
}

void LLGLTFMaterial::setEmissiveColorFactor(const LLColor3& emissiveColor)
{
    mEmissiveColor = emissiveColor;
    mEmissiveColor.clamp();
}

void LLGLTFMaterial::setMetallicFactor(F32 metallic)
{
    mMetallicFactor = llclamp(metallic, 0.f, 1.f);
}

void LLGLTFMaterial::setRoughnessFactor(F32 roughness)
{
    mRoughnessFactor = llclamp(roughness, 0.f, 1.f);
}

void LLGLTFMaterial::setAlphaMode(S32 mode)
{
    mAlphaMode = (AlphaMode)llclamp(mode, (S32)ALPHA_MODE_OPAQUE, (S32)ALPHA_MODE_MASK);
}

void LLGLTFMaterial::setDoubleSided(bool double_sided)
{
    // sure, no clamping will ever be needed for a bool, but include the 
    // setter for consistency with the clamping API
    mDoubleSided = double_sided;
}

// Default value accessors

LLUUID LLGLTFMaterial::getDefaultBaseColorId()
{
    return LLUUID::null;
}

LLUUID LLGLTFMaterial::getDefaultNormalId()
{
    return LLUUID::null;
}

LLUUID LLGLTFMaterial::getDefaultEmissiveId()
{
    return LLUUID::null;
}

LLUUID LLGLTFMaterial::getDefaultMetallicRoughnessId()
{
    return LLUUID::null;
}

F32 LLGLTFMaterial::getDefaultAlphaCutoff()
{
    return 0.f;
}

S32 LLGLTFMaterial::getDefaultAlphaMode()
{
    return (S32)ALPHA_MODE_OPAQUE;
}

F32 LLGLTFMaterial::getDefaultMetallicFactor()
{
    return 0.f;
}

F32 LLGLTFMaterial::getDefaultRoughnessFactor()
{
    return 0.f;
}

LLColor4 LLGLTFMaterial::getDefaultBaseColor()
{
    return LLColor4::white;
}

LLColor3 LLGLTFMaterial::getDefaultEmissiveColor()
{
    return LLColor3::black;
}

bool LLGLTFMaterial::getDefaultDoubleSided()
{
    return false;
}

void LLGLTFMaterial::writeOverridesToModel(tinygltf::Model& model, S32 mat_index, LLGLTFMaterial const* base_material) const
{
    if (model.materials.size() < mat_index + 1)
    {
        model.materials.resize(mat_index + 1);
    }

    tinygltf::Material& material_out = model.materials[mat_index];

    // TODO - fix handling of resetting to null/default values

    // set base color texture
    if (mBaseColorId.notNull() && mBaseColorId != base_material->mBaseColorId)
    {
        U32 idx = model.images.size();
        model.images.resize(idx + 1);
        model.textures.resize(idx + 1);

        material_out.pbrMetallicRoughness.baseColorTexture.index = idx;
        model.textures[idx].source = idx;
        model.images[idx].uri = mBaseColorId.asString();
    }

    // set normal texture
    if (mNormalId.notNull() && mNormalId != base_material->mNormalId)
    {
        U32 idx = model.images.size();
        model.images.resize(idx + 1);
        model.textures.resize(idx + 1);

        material_out.normalTexture.index = idx;
        model.textures[idx].source = idx;
        model.images[idx].uri = mNormalId.asString();
    }

    // set metallic-roughness texture
    if (mMetallicRoughnessId.notNull() && mMetallicRoughnessId != base_material->mMetallicRoughnessId)
    {
        U32 idx = model.images.size();
        model.images.resize(idx + 1);
        model.textures.resize(idx + 1);

        material_out.pbrMetallicRoughness.metallicRoughnessTexture.index = idx;
        model.textures[idx].source = idx;
        model.images[idx].uri = mMetallicRoughnessId.asString();
    }

    // set emissive texture
    if (mEmissiveId.notNull() && mEmissiveId != base_material->mEmissiveId)
    {
        U32 idx = model.images.size();
        model.images.resize(idx + 1);
        model.textures.resize(idx + 1);

        material_out.emissiveTexture.index = idx;
        model.textures[idx].source = idx;
        model.images[idx].uri = mEmissiveId.asString();
    }

    if (mAlphaMode != base_material->mAlphaMode)
    {
        material_out.alphaMode = getAlphaMode();
    }

    if (mAlphaCutoff != base_material->mAlphaCutoff)
    {
        material_out.alphaCutoff = mAlphaCutoff;
    }

    if (mBaseColor != base_material->mBaseColor)
    {
        mBaseColor.write(material_out.pbrMetallicRoughness.baseColorFactor);
    }

    if (mEmissiveColor != base_material->mEmissiveColor)
    {
        mEmissiveColor.write(material_out.emissiveFactor);
    }

    if (mMetallicFactor != base_material->mMetallicFactor)
    {
        material_out.pbrMetallicRoughness.metallicFactor = mMetallicFactor;
    }

    if (mRoughnessFactor != base_material->mRoughnessFactor)
    {
        material_out.pbrMetallicRoughness.roughnessFactor = mRoughnessFactor;
    }

    if (mDoubleSided != base_material->mDoubleSided)
    {
        material_out.doubleSided = mDoubleSided;
    }
}

void LLGLTFMaterial::applyOverride(const LLGLTFMaterial& override_mat)
{
    if (override_mat.mBaseColorId != getDefaultBaseColorId())
    {
        mBaseColorId = override_mat.mBaseColorId;
    }

    if (override_mat.mNormalId != getDefaultNormalId())
    {
        mNormalId = override_mat.mNormalId;
    }

    if (override_mat.mMetallicRoughnessId != getDefaultMetallicRoughnessId())
    {
        mMetallicRoughnessId = override_mat.mMetallicRoughnessId;
    }

    if (override_mat.mEmissiveId != getDefaultEmissiveId())
    {
        mEmissiveId = override_mat.mEmissiveId;
    }

    //TODO -- implement non texture parameters
}
