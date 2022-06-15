/** 
 * @file llmaterialeditor.cpp
 * @brief Implementation of the notecard editor
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llmaterialeditor.h"
#include "llcombobox.h"

#include "tinygltf/tiny_gltf.h"

///----------------------------------------------------------------------------
/// Class LLPreviewNotecard
///----------------------------------------------------------------------------

// Default constructor
LLMaterialEditor::LLMaterialEditor(const LLSD& key)
    : LLFloater(key)
{
}

BOOL LLMaterialEditor::postBuild()
{
    childSetAction("save", boost::bind(&LLMaterialEditor::onClickSave, this));
	return LLFloater::postBuild();
}

LLUUID LLMaterialEditor::getAlbedoId()
{
    return childGetValue("albedo texture").asUUID();
}

LLColor4 LLMaterialEditor::getAlbedoColor()
{
    LLColor4 ret = LLColor4(childGetValue("albedo color"));
    ret.mV[3] = getTransparency();
    return ret;
}


F32 LLMaterialEditor::getTransparency()
{
    return childGetValue("transparency").asReal();
}

std::string LLMaterialEditor::getAlphaMode()
{
    return childGetValue("alpha mode").asString();
}

F32 LLMaterialEditor::getAlphaCutoff()
{
    return childGetValue("alpha cutoff").asReal();
}

LLUUID LLMaterialEditor::getMetallicRoughnessId()
{
    return childGetValue("metallic-roughness texture").asUUID();
}

F32 LLMaterialEditor::getMetalnessFactor()
{
    return childGetValue("metalness factor").asReal();
}

F32 LLMaterialEditor::getRoughnessFactor()
{
    return childGetValue("roughness factor").asReal();
}

LLUUID LLMaterialEditor::getEmissiveId()
{
    return childGetValue("emissive texture").asUUID();
}

LLColor4 LLMaterialEditor::getEmissiveColor()
{
    return LLColor4(childGetValue("emissive color"));
}

LLUUID LLMaterialEditor::getNormalId()
{
    return childGetValue("normal texture").asUUID();
}

bool LLMaterialEditor::getDoubleSided()
{
    return childGetValue("double sided").asBoolean();
}


static void write_color(const LLColor4& color, std::vector<double>& c)
{
    for (int i = 0; i < c.size(); ++i) // NOTE -- use c.size because some gltf colors are 3-component
    {
        c[i] = color.mV[i];
    }
}

static U32 write_texture(const LLUUID& id, tinygltf::Model& model)
{
    tinygltf::Image image;
    image.uri = id.asString();
    model.images.push_back(image);
    U32 image_idx = model.images.size() - 1;

    tinygltf::Texture texture;
    texture.source = image_idx;
    model.textures.push_back(texture);
    U32 texture_idx = model.textures.size() - 1;

    return texture_idx;
}

void LLMaterialEditor::onClickSave()
{
    tinygltf::Model model;
    model.materials.resize(1);
    tinygltf::PbrMetallicRoughness& pbrMaterial = model.materials[0].pbrMetallicRoughness;

    // write albedo
    LLColor4 albedo_color = getAlbedoColor();
    albedo_color.mV[3] = getTransparency();
    write_color(albedo_color, pbrMaterial.baseColorFactor);

    model.materials[0].alphaCutoff = getAlphaCutoff();
    model.materials[0].alphaMode = getAlphaMode();

    LLUUID albedo_id = getAlbedoId();
    
    if (albedo_id.notNull())
    {
        U32 texture_idx = write_texture(albedo_id, model);
        
        pbrMaterial.baseColorTexture.index = texture_idx;
    }

    // write metallic/roughness
    F32 metalness = getMetalnessFactor();
    F32 roughness = getRoughnessFactor();

    pbrMaterial.metallicFactor = metalness;
    pbrMaterial.roughnessFactor = roughness;
    
    LLUUID mr_id = getMetallicRoughnessId();
    if (mr_id.notNull())
    {
        U32 texture_idx = write_texture(mr_id, model);
        pbrMaterial.metallicRoughnessTexture.index = texture_idx;
    }
    
    //write emissive
    LLColor4 emissive_color = getEmissiveColor();
    model.materials[0].emissiveFactor.resize(3);
    write_color(emissive_color, model.materials[0].emissiveFactor);

    LLUUID emissive_id = getEmissiveId();
    if (emissive_id.notNull())
    {
        U32 idx = write_texture(emissive_id, model);
        model.materials[0].emissiveTexture.index = idx;
    }

    //write normal
    LLUUID normal_id = getNormalId();
    if (normal_id.notNull())
    {
        U32 idx = write_texture(normal_id, model);
        model.materials[0].normalTexture.index = idx;
    }

    //write doublesided
    model.materials[0].doubleSided = getDoubleSided();

    std::ostringstream str;

    tinygltf::TinyGLTF gltf;
    model.asset.version = "2.0";
    gltf.WriteGltfSceneToStream(&model, str, true, false);
    
    std::string dump = str.str();

    LL_INFOS() << dump << LL_ENDL;
}

