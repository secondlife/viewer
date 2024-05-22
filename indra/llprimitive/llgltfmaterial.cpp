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

#include "llsdserialize.h"

// NOTE -- this should be the one and only place tiny_gltf.h is included
#include "tinygltf/tiny_gltf.h"
#include "llgltfmaterial_templates.h"

const char* const LLGLTFMaterial::ASSET_VERSION = "1.1";
const char* const LLGLTFMaterial::ASSET_TYPE = "GLTF 2.0";
const std::array<std::string, 2> LLGLTFMaterial::ACCEPTED_ASSET_VERSIONS = { "1.0", "1.1" };

const char* const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSFORM = "KHR_texture_transform";
const char* const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSFORM_SCALE = "scale";
const char* const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSFORM_OFFSET = "offset";
const char* const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSFORM_ROTATION = "rotation";


const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_EMISSIVE_STRENGTH = "KHR_materials_emissive_strength";
const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_EMISSIVE_STRENGTH_EMISSIVE_STRENGTH = "emissiveStrength";

const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSMISSION = "KHR_materials_transmission";
const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSMISSION_TRANSMISSION_FACTOR = "transmissionFactor";
const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSMISSION_TEXTURE             = "transmissionTexture";

const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_IOR = "KHR_materials_ior";
const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_IOR_IOR = "ior";

const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_VOLUME = "KHR_materials_volume";
const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_VOLUME_THICKNESS_FACTOR     = "thicknessFactor";
const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_VOLUME_THICKNESS_TEXTURE    = "thicknessTexture";
const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_VOLUME_ATTENUATION_DISTANCE = "attenuationDistance";
const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_VOLUME_ATTENUATION_COLOR = "attenuationColor";

const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_DISPERSION = "KHR_materials_dispersion";
const char *const LLGLTFMaterial::GLTF_FILE_EXTENSION_DISPERSION_DISPERSION = "dispersion";

// special UUID that indicates a null UUID in override data
const LLUUID LLGLTFMaterial::GLTF_OVERRIDE_NULL_UUID = LLUUID("ffffffff-ffff-ffff-ffff-ffffffffffff");

LLGLTFMaterial::LLGLTFMaterial()
{
    // IMPORTANT: since we use the hash of the member variables memory block of
    // this class to detect changes, we must ensure that all its padding bytes
    // have been zeroed out. But of course, we must leave the LLRefCount member
    // variable untouched (and skip it when hashing), and we cannot either
    // touch the local texture overrides map (else we destroy pointers, and
    // sundry private data, which would lead to a crash when using that map).
    // The variable members have therefore been arranged so that anything,
    // starting at mLocalTexDataDigest and up to the end of the members, can be
    // safely zeroed. HB
    const size_t offset = intptr_t(&mLocalTexDataDigest) - intptr_t(this);
    memset((void*)((const char*)this + offset), 0, sizeof(*this) - offset);

    // Now that we zeroed out our member variables, we can set the ones that
    // should not be zero to their default value. HB
    mBaseColor.set(1.f, 1.f, 1.f, 1.f);
    mMetallicFactor = mRoughnessFactor = 1.f;
    mAlphaCutoff = 0.5f;
    for (U32 i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        mTextureTransform[i].mScale.set(1.f, 1.f);
#if 0
        mTextureTransform[i].mOffset.clear();
        mTextureTransform[i].mRotation = 0.f;
#endif    
    }
#if 0
    mLocalTexDataDigest = 0;
    mAlphaMode = ALPHA_MODE_OPAQUE;    // This is 0
    mOverrideDoubleSided = mOverrideAlphaMode = false;
#endif
}

void LLGLTFMaterial::TextureTransform::getPacked(F32 (&packed)[8]) const
{
    packed[0] = mScale.mV[VX];
    packed[1] = mScale.mV[VY];
    packed[2] = mRotation;
    packed[4] = mOffset.mV[VX];
    packed[5] = mOffset.mV[VY];
    // Not used but nonetheless zeroed for proper hashing. HB
    packed[3] = packed[6] = packed[7] = 0.f;
}

bool LLGLTFMaterial::TextureTransform::operator==(const TextureTransform& other) const
{
    return mOffset == other.mOffset && mScale == other.mScale && mRotation == other.mRotation;
}

LLGLTFMaterial::LLGLTFMaterial(const LLGLTFMaterial& rhs)
{
    *this = rhs;
}

LLGLTFMaterial& LLGLTFMaterial::operator=(const LLGLTFMaterial& rhs)
{
    //have to do a manual operator= because of LLRefCount
    mTextureId = rhs.mTextureId;

    mTextureTransform = rhs.mTextureTransform;

    mBaseColor = rhs.mBaseColor;
    mEmissiveColor = rhs.mEmissiveColor;
    mEmissiveStrength = rhs.mEmissiveStrength;

    mTransmissionFactor = rhs.mTransmissionFactor;
    mIOR                = rhs.mIOR;
    mThicknessFactor = rhs.mThicknessFactor;
    mAttenuationDistance = rhs.mAttenuationDistance;
    mAttenuationColor    = rhs.mAttenuationColor;
    mDispersion          = rhs.mDispersion;

    mMetallicFactor = rhs.mMetallicFactor;
    mRoughnessFactor = rhs.mRoughnessFactor;
    mAlphaCutoff = rhs.mAlphaCutoff;

    mDoubleSided = rhs.mDoubleSided;
    mAlphaMode = rhs.mAlphaMode;

    mOverrideDoubleSided = rhs.mOverrideDoubleSided;
    mOverrideAlphaMode = rhs.mOverrideAlphaMode;

    if (rhs.mTrackingIdToLocalTexture.empty())
    {
        mTrackingIdToLocalTexture.clear();
        mLocalTexDataDigest = 0;
    }
    else
    {
        mTrackingIdToLocalTexture = rhs.mTrackingIdToLocalTexture;
        updateLocalTexDataDigest();
        updateTextureTracking();
    }

    return *this;
}

void LLGLTFMaterial::updateLocalTexDataDigest()
{
    mLocalTexDataDigest = 0;
    if (!mTrackingIdToLocalTexture.empty())
    {
        for (local_tex_map_t::const_iterator
                it = mTrackingIdToLocalTexture.begin(),
                end = mTrackingIdToLocalTexture.end();
             it != end; ++it)
        {
            mLocalTexDataDigest ^= it->first.getDigest64() ^
                                   it->second.getDigest64();
        }
    }
}

bool LLGLTFMaterial::operator==(const LLGLTFMaterial& rhs) const
{
    return mTextureId == rhs.mTextureId &&

        mTextureTransform == rhs.mTextureTransform &&

        mBaseColor == rhs.mBaseColor &&
        mEmissiveColor == rhs.mEmissiveColor &&

        mEmissiveStrength == rhs.mEmissiveStrength &&
        mMetallicFactor == rhs.mMetallicFactor &&
        mRoughnessFactor == rhs.mRoughnessFactor &&
        mAlphaCutoff == rhs.mAlphaCutoff &&

        mTransmissionFactor == rhs.mTransmissionFactor &&
        mIOR == rhs.mIOR &&
        mThicknessFactor == rhs.mThicknessFactor &&
        mAttenuationDistance == rhs.mAttenuationDistance &&
        mAttenuationColor == rhs.mAttenuationColor &&
        mDispersion == rhs.mDispersion &&

        mDoubleSided == rhs.mDoubleSided &&
        mAlphaMode == rhs.mAlphaMode &&

        mOverrideDoubleSided == rhs.mOverrideDoubleSided &&
        mOverrideAlphaMode == rhs.mOverrideAlphaMode;
}

bool LLGLTFMaterial::fromJSON(const std::string& json, std::string& warn_msg, std::string& error_msg)
{
    LL_PROFILE_ZONE_SCOPED;
    tinygltf::TinyGLTF gltf;

    tinygltf::Model model_in;

    if (gltf.LoadASCIIFromString(&model_in, &error_msg, &warn_msg, json.c_str(), json.length(), ""))
    {
        setFromModel(model_in, 0);

        return true;
    }

    return false;
}

std::string LLGLTFMaterial::asJSON(bool prettyprint) const
{
    LL_PROFILE_ZONE_SCOPED;
    tinygltf::TinyGLTF gltf;

    tinygltf::Model model_out;

    std::ostringstream str;

    writeToModel(model_out, 0);

    // To ensure consistency in asset upload, this should be the only reference
    // to WriteGltfSceneToStream in the viewer.
    gltf.WriteGltfSceneToStream(&model_out, str, prettyprint, false);

    return str.str();
}

void LLGLTFMaterial::setFromModel(const tinygltf::Model& model, S32 mat_index)
{
    LL_PROFILE_ZONE_SCOPED;
    if (model.materials.size() <= mat_index)
    {
        return;
    }

    const tinygltf::Material& material_in = model.materials[mat_index];

    tinygltf::TextureInfo transmissionMap = tinygltf::TextureInfo();
    TextureTransform transmissionMapTransform = TextureTransform();


    // Apply base color texture
    setFromTexture(model, material_in.pbrMetallicRoughness.baseColorTexture, GLTF_TEXTURE_INFO_BASE_COLOR);
    // Apply normal map
    setFromTexture(model, material_in.normalTexture, GLTF_TEXTURE_INFO_NORMAL);
    // Apply metallic-roughness texture
    setFromTexture(model, material_in.pbrMetallicRoughness.metallicRoughnessTexture, GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS);
    // Apply emissive texture
    setFromTexture(model, material_in.emissiveTexture, GLTF_TEXTURE_INFO_EMISSIVE);

    // We have to do this to populate the extension data for transmission and thickness maps.  Otherwise, when we call setFromTexture, it
    // will fail to populate the texture transform data.
    setTextureFromMaterialWithExtension(material_in, GLTF_FILE_EXTENSION_TRANSMISSION, GLTF_FILE_EXTENSION_TRANSMISSION_TEXTURE,
                                        transmissionMap);

    // Place holder to make tests happy.
    setFromTexture(model, transmissionMap, GLTF_TEXTURE_INFO_TRANSMISSION_TEXTURE);

    // KHR_materials_emissive_strength
    setFloatFromModelWithExtension(material_in, GLTF_FILE_EXTENSION_EMISSIVE_STRENGTH,
                                   GLTF_FILE_EXTENSION_EMISSIVE_STRENGTH_EMISSIVE_STRENGTH, mEmissiveStrength,
                                   getDefaultEmissiveStrength());

    // KHR_materials_transmission
    setFloatFromModelWithExtension(material_in, GLTF_FILE_EXTENSION_TRANSMISSION, GLTF_FILE_EXTENSION_TRANSMISSION_TRANSMISSION_FACTOR,
                                   mTransmissionFactor, getDefaultTransmissionFactor());

    // KHR_materials_ior
    setFloatFromModelWithExtension(material_in, GLTF_FILE_EXTENSION_IOR, GLTF_FILE_EXTENSION_IOR_IOR, mIOR, getDefaultIOR());

    // KHR_materials_volume
    setFloatFromModelWithExtension(material_in,
                                   GLTF_FILE_EXTENSION_VOLUME,
                                   GLTF_FILE_EXTENSION_VOLUME_THICKNESS_FACTOR,
                                   mThicknessFactor,
                                   getDefaultThicknessFactor());

    setFloatFromModelWithExtension(material_in,
                                   GLTF_FILE_EXTENSION_VOLUME,
                                   GLTF_FILE_EXTENSION_VOLUME_ATTENUATION_DISTANCE,
                                   mAttenuationDistance,
                                   getDefaultAttenuationDistance());

    setColor3FromMaterialWithExtension(material_in,
                                    GLTF_FILE_EXTENSION_VOLUME,
                                    GLTF_FILE_EXTENSION_VOLUME_ATTENUATION_COLOR,
                                    mAttenuationColor,
                                    getDefaultAttenuationColor());

    // KHR_materials_dispersion
    setFloatFromModelWithExtension(material_in,
                                   GLTF_FILE_EXTENSION_DISPERSION,
                                   GLTF_FILE_EXTENSION_DISPERSION_DISPERSION,
                                   mDispersion,
                                   getDefaultDispersion());


    setAlphaMode(material_in.alphaMode);
    mAlphaCutoff = llclamp((F32)material_in.alphaCutoff, 0.f, 1.f);

    mBaseColor.set(material_in.pbrMetallicRoughness.baseColorFactor);
    mEmissiveColor.set(material_in.emissiveFactor);

    mMetallicFactor = llclamp((F32)material_in.pbrMetallicRoughness.metallicFactor, 0.f, 1.f);
    mRoughnessFactor = llclamp((F32)material_in.pbrMetallicRoughness.roughnessFactor, 0.f, 1.f);

    mDoubleSided = material_in.doubleSided;

    if (material_in.extras.IsObject())
    {
        tinygltf::Value::Object extras = material_in.extras.Get<tinygltf::Value::Object>();
        const auto& alpha_mode = extras.find("override_alpha_mode");
        if (alpha_mode != extras.end())
        {
            mOverrideAlphaMode = alpha_mode->second.Get<bool>();
        }

        const auto& double_sided = extras.find("override_double_sided");
        if (double_sided != extras.end())
        {
            mOverrideDoubleSided = double_sided->second.Get<bool>();
        }
    }
}

// static
LLVector2 LLGLTFMaterial::vec2FromJson(const tinygltf::Value::Object& object, const char* key, const LLVector2& default_value)
{
    const auto it = object.find(key);
    if (it == object.end())
    {
        return default_value;
    }
    const tinygltf::Value& vec2_json = std::get<1>(*it);
    if (!vec2_json.IsArray() || vec2_json.ArrayLen() < LENGTHOFVECTOR2)
    {
        return default_value;
    }
    LLVector2 value;
    for (U32 i = 0; i < LENGTHOFVECTOR2; ++i)
    {
        const tinygltf::Value& real_json = vec2_json.Get(i);
        if (!real_json.IsReal())
        {
            return default_value;
        }
        value.mV[i] = (F32)real_json.Get<double>();
    }
    return value;
}

// static
LLColor3 LLGLTFMaterial::color3FromJson(const tinygltf::Value::Object &object, const char *key, const LLColor3 &default_value)
{
    const auto it = object.find(key);
    if (it == object.end())
    {
        return default_value;
    }
    const tinygltf::Value &color_json = std::get<1>(*it);
    if (!color_json.IsArray() || color_json.ArrayLen() < LENGTHOFCOLOR3)
    {
        return default_value;
    }
    LLColor3 value;
    for (U32 i = 0; i < LENGTHOFCOLOR3; ++i)
    {
        const tinygltf::Value &real_json = color_json.Get(i);
        if (!real_json.IsReal())
        {
            return default_value;
        }
        value.mV[i] = (F32) real_json.Get<double>();
    }
    return value;
}

// static
F32 LLGLTFMaterial::floatFromJson(const tinygltf::Value::Object& object, const char* key, const F32 default_value)
{
    const auto it = object.find(key);
    if (it == object.end())
    {
        return default_value;
    }
    const tinygltf::Value& real_json = std::get<1>(*it);
    if (!real_json.IsReal())
    {
        return default_value;
    }
    return (F32)real_json.GetNumberAsDouble();
}

// static
void LLGLTFMaterial::writeFloatToMaterialWithExtension(tinygltf::Material &material, const char *extension, const char *key, F32 value)
{
    tinygltf::Value::Object extension_object;
    tinygltf::Value val = tinygltf::Value(value);
    // See if we already have this extension.
    auto extension_it = material.extensions.find(extension);

    if (extension_it != material.extensions.end())
    {
        material.extensions[extension].Get<tinygltf::Value::Object>()[key] = val;
    }
    else
    {
        extension_object[key]          = val;
        material.extensions[extension] = tinygltf::Value(extension_object);
    }
}

// static
void LLGLTFMaterial::setFloatFromModelWithExtension(const tinygltf::Material &material, const char *extension, const char *key, F32 &value,
                                                    F32 defaultValue)
{
    LL_PROFILE_ZONE_SCOPED;

    const tinygltf::Value::Object &extensions_object = material.extensions;
    const auto                     extension_it       = extensions_object.find(extension);

    if (extension_it != extensions_object.end())
    {
        const tinygltf::Value &value_json = std::get<1>(*extension_it);
        if (value_json.IsObject())
        {
            const tinygltf::Value::Object &value_object = value_json.Get<tinygltf::Value::Object>();
            value                                       = floatFromJson(value_object, key, defaultValue);
        }
    }
}

// static
void LLGLTFMaterial::writeColor3ToMaterialWithExtension(tinygltf::Material& material, const char* extension, const char* key, const LLColor3& color)
{
    tinygltf::Value::Object extension_object;
    tinygltf::Value::Array  color_array;

    color_array.push_back(tinygltf::Value(color.mV[0]));
    color_array.push_back(tinygltf::Value(color.mV[1]));
    color_array.push_back(tinygltf::Value(color.mV[2]));

    // See if we already have this extension.
    auto extension_it = material.extensions.find(extension);
    if (extension_it != material.extensions.end())
    {
        material.extensions[extension].Get<tinygltf::Value::Object>()[key] = tinygltf::Value(color_array);
    }
    else
    {
        extension_object[key]          = tinygltf::Value(color_array);
        material.extensions[extension] = tinygltf::Value(extension_object);
    }
}

// static
void LLGLTFMaterial::setColor3FromMaterialWithExtension(const tinygltf::Material &material, const char *extension, const char *key,
                                                        LLColor3 &color, LLColor3 defaultColor)
{
	LL_PROFILE_ZONE_SCOPED;

	const tinygltf::Value::Object &extensions_object = material.extensions;
	const auto                     extension_it       = extensions_object.find(extension);

    if (extension_it != extensions_object.end())
    {
		const tinygltf::Value &value_json = std::get<1>(*extension_it);
        if (value_json.IsObject())
        {
			const tinygltf::Value::Object &value_object = value_json.Get<tinygltf::Value::Object>();
            color                                       = color3FromJson(value_object, key, defaultColor);
		}
	}
}

void LLGLTFMaterial::writeTextureToMaterialWithExtension(tinygltf::Material& material, const char* extension, const char* key,
    const tinygltf::TextureInfo& texture_info)
{
    tinygltf::Value::Object extension_object;

    tinygltf::Value::Object texture_object;
    texture_object["index"] = tinygltf::Value(texture_info.index);
    texture_object["extensions"] = tinygltf::Value(texture_info.extensions);
    
    // See if we already have this extension.
    auto extension_it = material.extensions.find(extension);
    if (extension_it != material.extensions.end())
    {
        material.extensions[extension].Get<tinygltf::Value::Object>()[key] = tinygltf::Value(texture_object);
    }
    else
    {
        extension_object[key]          = tinygltf::Value(texture_object);
        material.extensions[extension] = tinygltf::Value(extension_object);
    }
}

void LLGLTFMaterial::setTextureFromMaterialWithExtension(const tinygltf::Material &model, const char *extension, const char *key,
                                                         tinygltf::TextureInfo &texture_info)
{
	LL_PROFILE_ZONE_SCOPED;

	const tinygltf::Value::Object &extensions_object = model.extensions;
	const auto extension_it = extensions_object.find(extension);

    if (extension_it != extensions_object.end())
    {
        // We found the extension data.  Now we need to extract it.

        auto key_it = std::get<1>(*extension_it).Get<tinygltf::Value::Object>().find(key);
        if (key_it != std::get<1>(*extension_it).Get<tinygltf::Value::Object>().end())
        {
			const tinygltf::Value::Object &texture_object = key_it->second.Get<tinygltf::Value::Object>();
			texture_info.index = (U32)floatFromJson(texture_object, "index", 0);

            if (texture_object.find("extensions") != texture_object.end())
            {
				const tinygltf::Value::Object &extensions = texture_object.find("extensions")->second.Get<tinygltf::Value::Object>();
				texture_info.extensions = extensions;
			}
		}
	}
}

void LLGLTFMaterial::writeToModel(tinygltf::Model& model, S32 mat_index) const
{
    LL_PROFILE_ZONE_SCOPED;
    if (model.materials.size() < mat_index+1)
    {
        model.materials.resize(mat_index + 1);
    }

    tinygltf::Material& material_out = model.materials[mat_index];

    tinygltf::TextureInfo transmissionMap;
    tinygltf::TextureInfo thicknessMap;

    // set base color texture
    writeToTexture(model, material_out.pbrMetallicRoughness.baseColorTexture, GLTF_TEXTURE_INFO_BASE_COLOR);
    // set normal texture
    writeToTexture(model, material_out.normalTexture, GLTF_TEXTURE_INFO_NORMAL);
    // set metallic-roughness texture
    writeToTexture(model, material_out.pbrMetallicRoughness.metallicRoughnessTexture, GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS);
    // set emissive texture
    writeToTexture(model, material_out.emissiveTexture, GLTF_TEXTURE_INFO_EMISSIVE);
    // set occlusion texture
    // *NOTE: This is required for ORM materials for GLTF compliance.
    // See: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_occlusiontexture
    writeToTexture(model, material_out.occlusionTexture, GLTF_TEXTURE_INFO_OCCLUSION);

    // KHR_materials_transmission & KHR_materials_volume textures.  We pack these into a singular texture, but we need to get the locations
    // regardless even though they share the same texture.
    writeToTexture(model, transmissionMap, GLTF_TEXTURE_INFO_TRANSMISSION_TEXTURE);
    writeToTexture(model, thicknessMap, GLTF_TEXTURE_INFO_THICKNESS_TEXTURE);

    
    // We have to do some scuffed stuff here to make sure that the material has transmission and thickness maps.
    writeTextureToMaterialWithExtension(material_out, GLTF_FILE_EXTENSION_TRANSMISSION, GLTF_FILE_EXTENSION_TRANSMISSION_TEXTURE,
                                        transmissionMap);
    writeTextureToMaterialWithExtension(material_out, GLTF_FILE_EXTENSION_VOLUME, GLTF_FILE_EXTENSION_VOLUME_THICKNESS_TEXTURE,
                                        thicknessMap);

    material_out.alphaMode = getAlphaMode();
    material_out.alphaCutoff = mAlphaCutoff;

    mBaseColor.write(material_out.pbrMetallicRoughness.baseColorFactor);

    if (mEmissiveColor != LLGLTFMaterial::getDefaultEmissiveColor())
    {
        material_out.emissiveFactor.resize(3);
        mEmissiveColor.write(material_out.emissiveFactor);
    }

    writeFloatToMaterialWithExtension(material_out,
                                      GLTF_FILE_EXTENSION_EMISSIVE_STRENGTH,
                                      GLTF_FILE_EXTENSION_EMISSIVE_STRENGTH_EMISSIVE_STRENGTH,
                                      mEmissiveStrength);

    
    writeFloatToMaterialWithExtension(material_out, GLTF_FILE_EXTENSION_TRANSMISSION, GLTF_FILE_EXTENSION_TRANSMISSION_TRANSMISSION_FACTOR,
                                      mTransmissionFactor);

    writeFloatToMaterialWithExtension(material_out,
                                      GLTF_FILE_EXTENSION_IOR, GLTF_FILE_EXTENSION_IOR_IOR,
                                      mIOR);

    writeFloatToMaterialWithExtension(material_out,
                                      GLTF_FILE_EXTENSION_VOLUME,
                                      GLTF_FILE_EXTENSION_VOLUME_THICKNESS_FACTOR,
                                      mThicknessFactor);

    writeFloatToMaterialWithExtension(material_out,
                                      GLTF_FILE_EXTENSION_VOLUME,
                                      GLTF_FILE_EXTENSION_VOLUME_ATTENUATION_DISTANCE,
                                      mAttenuationDistance);

    writeColor3ToMaterialWithExtension(material_out,
        									   GLTF_FILE_EXTENSION_VOLUME,
        									   GLTF_FILE_EXTENSION_VOLUME_ATTENUATION_COLOR,
        									   mAttenuationColor);
    
    writeFloatToMaterialWithExtension(material_out,
                                      GLTF_FILE_EXTENSION_DISPERSION,
                                      GLTF_FILE_EXTENSION_DISPERSION_DISPERSION,
                                      mDispersion);

    material_out.pbrMetallicRoughness.metallicFactor = mMetallicFactor;
    material_out.pbrMetallicRoughness.roughnessFactor = mRoughnessFactor;

    material_out.doubleSided = mDoubleSided;

    // generate "extras" string
    tinygltf::Value::Object extras;
    bool write_extras = false;
    if (mOverrideAlphaMode && mAlphaMode == getDefaultAlphaMode())
    {
        extras["override_alpha_mode"] = tinygltf::Value(mOverrideAlphaMode);
        write_extras = true;
    }

    if (mOverrideDoubleSided && mDoubleSided == getDefaultDoubleSided())
    {
        extras["override_double_sided"] = tinygltf::Value(mOverrideDoubleSided);
        write_extras = true;
    }

    if (write_extras)
    {
        material_out.extras = tinygltf::Value(extras);
    }

    model.asset.version = "2.0";
}

void LLGLTFMaterial::sanitizeAssetMaterial()
{
    mTextureTransform = sDefault.mTextureTransform;
}

bool LLGLTFMaterial::setBaseMaterial()
{
    const LLGLTFMaterial old_override = *this;
    *this = sDefault;
    setBaseMaterial(old_override);
    return *this != old_override;
}

// For material overrides only. Copies transforms from the old override.
void LLGLTFMaterial::setBaseMaterial(const LLGLTFMaterial& old_override_mat)
{
    mTextureTransform = old_override_mat.mTextureTransform;
}

bool LLGLTFMaterial::isClearedForBaseMaterial() const
{
    LLGLTFMaterial cleared_override = sDefault;
    cleared_override.setBaseMaterial(*this);
    return *this == cleared_override;
}


// static
void LLGLTFMaterial::hackOverrideUUID(LLUUID& id)
{
    if (id == LLUUID::null)
    {
        id = GLTF_OVERRIDE_NULL_UUID;
    }
}

void LLGLTFMaterial::setTextureId(TextureInfo texture_info, const LLUUID& id, bool for_override)
{
    mTextureId[texture_info] = id;
    if (for_override)
    {
        hackOverrideUUID(mTextureId[texture_info]);
    }
}

void LLGLTFMaterial::setBaseColorId(const LLUUID& id, bool for_override)
{
    setTextureId(GLTF_TEXTURE_INFO_BASE_COLOR, id, for_override);
}

void LLGLTFMaterial::setNormalId(const LLUUID& id, bool for_override)
{
    setTextureId(GLTF_TEXTURE_INFO_NORMAL, id, for_override);
}

void LLGLTFMaterial::setOcclusionRoughnessMetallicId(const LLUUID& id, bool for_override)
{
    setTextureId(GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS, id, for_override);
}

void LLGLTFMaterial::setEmissiveId(const LLUUID& id, bool for_override)
{
    setTextureId(GLTF_TEXTURE_INFO_EMISSIVE, id, for_override);
}

void LLGLTFMaterial::setTransmissionId(const LLUUID &id, bool for_override)
{
    setTextureId(GLTF_TEXTURE_INFO_TRANSMISSION_TEXTURE, id, for_override);
}

void LLGLTFMaterial::setBaseColorFactor(const LLColor4& baseColor, bool for_override)
{
    mBaseColor.set(baseColor);
    mBaseColor.clamp();

    if (for_override)
    { // hack -- nudge off of default value
        if (mBaseColor == getDefaultBaseColor())
        {
            mBaseColor.mV[3] -= FLT_EPSILON;
        }
    }
}

void LLGLTFMaterial::setAlphaCutoff(F32 cutoff, bool for_override)
{
    mAlphaCutoff = llclamp(cutoff, 0.f, 1.f);
    if (for_override)
    { // hack -- nudge off of default value
        if (mAlphaCutoff == getDefaultAlphaCutoff())
        {
            mAlphaCutoff -= FLT_EPSILON;
        }
    }
}

void LLGLTFMaterial::setEmissiveColorFactor(const LLColor3& emissiveColor, bool for_override)
{
    mEmissiveColor = emissiveColor;
    mEmissiveColor.clamp();

    if (for_override)
    { // hack -- nudge off of default value
        if (mEmissiveColor == getDefaultEmissiveColor())
        {
            mEmissiveColor.mV[0] += FLT_EPSILON;
        }
    }
}

void LLGLTFMaterial::setEmissiveStrength(F32 emissiveStrength, bool for_override)
{
    // KHR_materials_emissive_strength only allows positive values.  There is no maximum value.
	mEmissiveStrength = llmax(emissiveStrength, 0.f);
	if (for_override)
	{ 
        if (mEmissiveStrength == getDefaultEmissiveStrength())
        {
			mEmissiveStrength -= FLT_EPSILON;
		}
    }
}

void LLGLTFMaterial::setMetallicFactor(F32 metallic, bool for_override)
{
    mMetallicFactor = llclamp(metallic, 0.f, for_override ? 1.f - FLT_EPSILON : 1.f);
}

void LLGLTFMaterial::setRoughnessFactor(F32 roughness, bool for_override)
{
    mRoughnessFactor = llclamp(roughness, 0.f, for_override ? 1.f - FLT_EPSILON : 1.f);
}

void LLGLTFMaterial::setTransmissionFactor(F32 transmission, bool for_override)
{
    mTransmissionFactor = llclamp(transmission, 0.f, 1.f);
    if (for_override)
    {
        if (mTransmissionFactor == getDefaultTransmissionFactor())
        {
            mTransmissionFactor -= FLT_EPSILON;
        }
    }
}

void LLGLTFMaterial::setIOR(F32 ior, bool for_override)
{
    mIOR = llmax(ior, 1.f);
    if (for_override)
    {
        if (mIOR == getDefaultIOR())
        {
            mIOR -= FLT_EPSILON;
        }
    }
}

void LLGLTFMaterial::setThicknessFactor(F32 thickness, bool for_override)
{
    mThicknessFactor = llmax(thickness, 0.f);
    if (for_override)
    {
        if (mThicknessFactor == getDefaultThicknessFactor())
        {
            mThicknessFactor -= FLT_EPSILON;
        }
    }
}

void LLGLTFMaterial::setAttenuationDistance(F32 attenuationDistance, bool for_override)
{
    mAttenuationDistance = llmax(attenuationDistance, 0.f);
    if (for_override)
    {
        if (mAttenuationDistance == getDefaultAttenuationDistance())
        {
            mAttenuationDistance -= FLT_EPSILON;
        }
    }
}

void LLGLTFMaterial::setAttenuationColor(const LLColor3& attenuationColor, bool for_override)
{
    mAttenuationColor = attenuationColor;
    mAttenuationColor.clamp();

    if (for_override)
    {  // hack -- nudge off of default value
        if (mAttenuationColor == getDefaultAttenuationColor())
        {
            mAttenuationColor.mV[0] += FLT_EPSILON;
        }
    }
}

void LLGLTFMaterial::setDispersion(F32 dispersion, bool for_override)
{
    mDispersion = llmax(dispersion, 0.f);
    if (for_override)
    {
        if (mDispersion == getDefaultDispersion())
        {
			mDispersion -= FLT_EPSILON;
		}
	}
}


void LLGLTFMaterial::setAlphaMode(const std::string& mode, bool for_override)
{
    S32 m = getDefaultAlphaMode();
    if (mode == "MASK")
    {
        m = ALPHA_MODE_MASK;
    }
    else if (mode == "BLEND")
    {
        m = ALPHA_MODE_BLEND;
    }

    setAlphaMode(m, for_override);
}

const char* LLGLTFMaterial::getAlphaMode() const
{
    switch (mAlphaMode)
    {
    case ALPHA_MODE_MASK: return "MASK";
    case ALPHA_MODE_BLEND: return "BLEND";
    default: return "OPAQUE";
    }
}

void LLGLTFMaterial::setAlphaMode(S32 mode, bool for_override)
{
    mAlphaMode = (AlphaMode) llclamp(mode, (S32) ALPHA_MODE_OPAQUE, (S32) ALPHA_MODE_MASK);
    mOverrideAlphaMode = for_override && mAlphaMode == getDefaultAlphaMode();
}

void LLGLTFMaterial::setDoubleSided(bool double_sided, bool for_override)
{
    // sure, no clamping will ever be needed for a bool, but include the
    // setter for consistency with the clamping API
    mDoubleSided = double_sided;
    mOverrideDoubleSided = for_override && mDoubleSided == getDefaultDoubleSided();
}

void LLGLTFMaterial::setTextureOffset(TextureInfo texture_info, const LLVector2& offset)
{
    mTextureTransform[texture_info].mOffset = offset;
}

void LLGLTFMaterial::setTextureScale(TextureInfo texture_info, const LLVector2& scale)
{
    mTextureTransform[texture_info].mScale = scale;
}

void LLGLTFMaterial::setTextureRotation(TextureInfo texture_info, float rotation)
{
    mTextureTransform[texture_info].mRotation = rotation;
}

// Default value accessors (NOTE: these MUST match the GLTF specification)

// Make a static default material for accessors
const LLGLTFMaterial LLGLTFMaterial::sDefault;

F32 LLGLTFMaterial::getDefaultAlphaCutoff()
{
    return sDefault.mAlphaCutoff;
}

S32 LLGLTFMaterial::getDefaultAlphaMode()
{
    return (S32) sDefault.mAlphaMode;
}

F32 LLGLTFMaterial::getDefaultMetallicFactor()
{
    return sDefault.mMetallicFactor;
}

F32 LLGLTFMaterial::getDefaultRoughnessFactor()
{
    return sDefault.mRoughnessFactor;
}

F32 LLGLTFMaterial::getDefaultTransmissionFactor()
{
    return sDefault.mTransmissionFactor;
}

F32 LLGLTFMaterial::getDefaultIOR()
{
    return sDefault.mIOR;
}

F32 LLGLTFMaterial::getDefaultThicknessFactor()
{
    return sDefault.mThicknessFactor;
}

F32 LLGLTFMaterial::getDefaultAttenuationDistance()
{
    return sDefault.mAttenuationDistance;
}

LLColor3 LLGLTFMaterial::getDefaultAttenuationColor()
{
    return sDefault.mAttenuationColor;
}

F32 LLGLTFMaterial::getDefaultDispersion()
{
    return sDefault.mDispersion;
}

LLColor4 LLGLTFMaterial::getDefaultBaseColor()
{
    return sDefault.mBaseColor;
}

LLColor3 LLGLTFMaterial::getDefaultEmissiveColor()
{
    return sDefault.mEmissiveColor;
}

F32 LLGLTFMaterial::getDefaultEmissiveStrength()
{
    return sDefault.mEmissiveStrength;
}

bool LLGLTFMaterial::getDefaultDoubleSided()
{
    return sDefault.mDoubleSided;
}

LLVector2 LLGLTFMaterial::getDefaultTextureOffset()
{
    return sDefault.mTextureTransform[0].mOffset;
}

LLVector2 LLGLTFMaterial::getDefaultTextureScale()
{
    return sDefault.mTextureTransform[0].mScale;
}

F32 LLGLTFMaterial::getDefaultTextureRotation()
{
    return sDefault.mTextureTransform[0].mRotation;
}

// static
void LLGLTFMaterial::applyOverrideUUID(LLUUID& dst_id, const LLUUID& override_id)
{
    if (override_id != GLTF_OVERRIDE_NULL_UUID)
    {
        if (override_id != LLUUID::null)
        {
            dst_id = override_id;
        }
    }
    else
    {
        dst_id = LLUUID::null;
    }
}

void LLGLTFMaterial::applyOverride(const LLGLTFMaterial& override_mat)
{
    LL_PROFILE_ZONE_SCOPED;

    for (U32 i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        LLUUID& texture_id = mTextureId[i];
        const LLUUID& override_texture_id = override_mat.mTextureId[i];
        applyOverrideUUID(texture_id, override_texture_id);
    }

    if (override_mat.mBaseColor != getDefaultBaseColor())
    {
        mBaseColor = override_mat.mBaseColor;
    }

    if (override_mat.mEmissiveColor != getDefaultEmissiveColor())
    {
        mEmissiveColor = override_mat.mEmissiveColor;
    }

    if (override_mat.mEmissiveStrength != getDefaultEmissiveStrength())
    {
		mEmissiveStrength = override_mat.mEmissiveStrength;
	}

    if (override_mat.mTransmissionFactor != getDefaultTransmissionFactor())
    {
		mTransmissionFactor = override_mat.mTransmissionFactor;
	}

    if (override_mat.mIOR != getDefaultIOR())
    {
        mIOR = override_mat.mIOR;
    }

    if (override_mat.mThicknessFactor != getDefaultThicknessFactor())
    {
		mThicknessFactor = override_mat.mThicknessFactor;
	}

    if (override_mat.mAttenuationDistance != getDefaultAttenuationDistance())
    {
        mAttenuationDistance = override_mat.mAttenuationDistance;
    }

    if (override_mat.mAttenuationColor != getDefaultAttenuationColor())
    {
		mAttenuationColor = override_mat.mAttenuationColor;
	}

    if (override_mat.mDispersion != getDefaultDispersion())
	{
        mDispersion = override_mat.mDispersion;
    }

    if (override_mat.mMetallicFactor != getDefaultMetallicFactor())
    {
        mMetallicFactor = override_mat.mMetallicFactor;
    }

    if (override_mat.mRoughnessFactor != getDefaultRoughnessFactor())
    {
        mRoughnessFactor = override_mat.mRoughnessFactor;
    }

    if (override_mat.mAlphaMode != getDefaultAlphaMode() || override_mat.mOverrideAlphaMode)
    {
        mAlphaMode = override_mat.mAlphaMode;
    }
    if (override_mat.mAlphaCutoff != getDefaultAlphaCutoff())
    {
        mAlphaCutoff = override_mat.mAlphaCutoff;
    }

    if (override_mat.mDoubleSided != getDefaultDoubleSided() || override_mat.mOverrideDoubleSided)
    {
        mDoubleSided = override_mat.mDoubleSided;
    }

    for (U32 i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        if (override_mat.mTextureTransform[i].mOffset != getDefaultTextureOffset())
        {
            mTextureTransform[i].mOffset = override_mat.mTextureTransform[i].mOffset;
        }

        if (override_mat.mTextureTransform[i].mScale != getDefaultTextureScale())
        {
            mTextureTransform[i].mScale = override_mat.mTextureTransform[i].mScale;
        }

        if (override_mat.mTextureTransform[i].mRotation != getDefaultTextureRotation())
        {
            mTextureTransform[i].mRotation = override_mat.mTextureTransform[i].mRotation;
        }
    }

    if (!override_mat.mTrackingIdToLocalTexture.empty())
    {
        auto it = override_mat.mTrackingIdToLocalTexture.begin();
        mTrackingIdToLocalTexture.insert(it, it);
        updateLocalTexDataDigest();
        updateTextureTracking();
    }
}

void LLGLTFMaterial::getOverrideLLSD(const LLGLTFMaterial& override_mat, LLSD& data)
{
    LL_PROFILE_ZONE_SCOPED;
    llassert(data.isUndefined());

    // make every effort to shave bytes here

    for (U32 i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        LLUUID& texture_id = mTextureId[i];
        const LLUUID& override_texture_id = override_mat.mTextureId[i];
        if (override_texture_id.notNull() && override_texture_id != texture_id)
        {
            data["tex"][i] = LLSD::UUID(override_texture_id);
        }
    }

    if (override_mat.mBaseColor != getDefaultBaseColor())
    {
        data["bc"] = override_mat.mBaseColor.getValue();
    }

    if (override_mat.mEmissiveColor != getDefaultEmissiveColor())
    {
        data["ec"] = override_mat.mEmissiveColor.getValue();
    }
    
    if (override_mat.mEmissiveStrength != getDefaultEmissiveStrength())
    {
        data["es"] = override_mat.mEmissiveStrength;
    }

    if (override_mat.mMetallicFactor != getDefaultMetallicFactor())
    {
        data["mf"] = override_mat.mMetallicFactor;
    }

    if (override_mat.mRoughnessFactor != getDefaultRoughnessFactor())
    {
        data["rf"] = override_mat.mRoughnessFactor;
    }
    
    if (override_mat.mTransmissionFactor != getDefaultTransmissionFactor())
    {
        data["tf"] = override_mat.mTransmissionFactor;
    }

    if (override_mat.mIOR != getDefaultIOR())
    {
		data["ir"] = override_mat.mIOR;
	}

    if (override_mat.mThicknessFactor != getDefaultThicknessFactor())
    {
        data["th"] = override_mat.mThicknessFactor;
    }

    if (override_mat.mAttenuationDistance != getDefaultAttenuationDistance())
    {
		data["ad"] = override_mat.mAttenuationDistance;
	}

    if (override_mat.mAttenuationColor != getDefaultAttenuationColor())
    {
        data["atc"] = override_mat.mAttenuationColor.getValue();
    }

    if (override_mat.mDispersion != getDefaultDispersion())
    {
		data["di"] = override_mat.mDispersion;
	}

    if (override_mat.mAlphaMode != getDefaultAlphaMode() || override_mat.mOverrideAlphaMode)
    {
        data["am"] = override_mat.mAlphaMode;
    }

    if (override_mat.mAlphaCutoff != getDefaultAlphaCutoff())
    {
        data["ac"] = override_mat.mAlphaCutoff;
    }

    if (override_mat.mDoubleSided != getDefaultDoubleSided() || override_mat.mOverrideDoubleSided)
    {
        data["ds"] = override_mat.mDoubleSided;
    }

    for (U32 i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        if (override_mat.mTextureTransform[i].mOffset != getDefaultTextureOffset())
        {
            data["ti"][i]["o"] = override_mat.mTextureTransform[i].mOffset.getValue();
        }

        if (override_mat.mTextureTransform[i].mScale != getDefaultTextureScale())
        {
            data["ti"][i]["s"] = override_mat.mTextureTransform[i].mScale.getValue();
        }

        if (override_mat.mTextureTransform[i].mRotation != getDefaultTextureRotation())
        {
            data["ti"][i]["r"] = override_mat.mTextureTransform[i].mRotation;
        }
    }
}


void LLGLTFMaterial::applyOverrideLLSD(const LLSD& data)
{
    const LLSD& tex = data["tex"];

    if (tex.isArray())
    {
        for (int i = 0; i < tex.size(); ++i)
        {
            mTextureId[i] = tex[i].asUUID();
        }
    }

    const LLSD& bc = data["bc"];
    if (bc.isDefined())
    {
        mBaseColor.setValue(bc);
        if (mBaseColor == getDefaultBaseColor())
        {
            // HACK -- nudge by epsilon if we receive a default value (indicates override to default)
            mBaseColor.mV[3] -= FLT_EPSILON;
        }
    }

    const LLSD& ec = data["ec"];
    if (ec.isDefined())
    {
        mEmissiveColor.setValue(ec);
        if (mEmissiveColor == getDefaultEmissiveColor())
        {
            // HACK -- nudge by epsilon if we receive a default value (indicates override to default)
            mEmissiveColor.mV[0] += FLT_EPSILON;
        }
    }
    
    const LLSD &es = data["es"];
    if (es.isDefined())
    {
        mEmissiveStrength = es.asReal();
        if (mEmissiveStrength == getDefaultEmissiveStrength())
        {
            // HACK -- nudge by epsilon if we receive a default value (indicates override to default)
            mEmissiveStrength -= FLT_EPSILON;
        }
    }

    const LLSD &tf = data["tf"];
    if (tf.isDefined())
    {
		mTransmissionFactor = tf.asReal();
        if (mTransmissionFactor == getDefaultTransmissionFactor())
        {
			// HACK -- nudge by epsilon if we receive a default value (indicates override to default)
			mTransmissionFactor -= FLT_EPSILON;
		}
	}

    const LLSD &ir = data["ir"];
    if (ir.isDefined())
    {
		mIOR = ir.asReal();
        if (mIOR == getDefaultIOR())
        {
			// HACK -- nudge by epsilon if we receive a default value (indicates override to default)
			mIOR -= FLT_EPSILON;
		}
	}

    const LLSD &th = data["th"];
    if (th.isDefined())
    {
		mThicknessFactor = th.asReal();
        if (mThicknessFactor == getDefaultThicknessFactor())
        {
			// HACK -- nudge by epsilon if we receive a default value (indicates override to default)
			mThicknessFactor -= FLT_EPSILON;
		}
	}

    const LLSD &ad = data["ad"];
    if (ad.isDefined())
    {
		mAttenuationDistance = ad.asReal();
        if (mAttenuationDistance == getDefaultAttenuationDistance())
        {
			// HACK -- nudge by epsilon if we receive a default value (indicates override to default)
			mAttenuationDistance -= FLT_EPSILON;
		}
	}

    const LLSD &atc = data["atc"];
    if (atc.isDefined())
    {
        mAttenuationColor.setValue(atc);
        if (mAttenuationColor == getDefaultAttenuationColor())
        {
            // HACK -- nudge by epsilon if we receive a default value (indicates override to default)
            mAttenuationColor.mV[0] += FLT_EPSILON;
        }
    }

    const LLSD &di = data["di"];
    if (di.isDefined())
    {
        mDispersion = di.asReal();
        if (mDispersion == getDefaultDispersion())
        {
			// HACK -- nudge by epsilon if we receive a default value (indicates override to default)
			mDispersion -= FLT_EPSILON;
		}
    }

    const LLSD& mf = data["mf"];
    if (mf.isReal())
    {
        mMetallicFactor = mf.asReal();
        if (mMetallicFactor == getDefaultMetallicFactor())
        { 
            // HACK -- nudge by epsilon if we receive a default value (indicates override to default)
            mMetallicFactor -= FLT_EPSILON;
        }
    }

    const LLSD& rf = data["rf"];
    if (rf.isReal())
    {
        mRoughnessFactor = rf.asReal();
        if (mRoughnessFactor == getDefaultRoughnessFactor())
        { 
            // HACK -- nudge by epsilon if we receive a default value (indicates override to default)
            mRoughnessFactor -= FLT_EPSILON;
        }
    }

    const LLSD& am = data["am"];
    if (am.isInteger())
    {
        mAlphaMode = (AlphaMode) am.asInteger();
        mOverrideAlphaMode = true;
    }

    const LLSD& ac = data["ac"];
    if (ac.isReal())
    {
        mAlphaCutoff = ac.asReal();
        if (mAlphaCutoff == getDefaultAlphaCutoff())
        {
            // HACK -- nudge by epsilon if we receive a default value (indicates override to default)
            mAlphaCutoff -= FLT_EPSILON;
        }
    }

    const LLSD& ds = data["ds"];
    if (ds.isBoolean())
    {
        mDoubleSided = ds.asBoolean();
        mOverrideDoubleSided = true;
    }

    const LLSD& ti = data["ti"];
    if (ti.isArray())
    {
        for (U32 i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
        {
            const LLSD& o = ti[i]["o"];
            if (o.isDefined())
            {
                mTextureTransform[i].mOffset.setValue(o);
            }

            const LLSD& s = ti[i]["s"];
            if (s.isDefined())
            {
                mTextureTransform[i].mScale.setValue(s);
            }

            const LLSD& r = ti[i]["r"];
            if (r.isReal())
            {
                mTextureTransform[i].mRotation = r.asReal();
            }
        }
    }
}

LLUUID LLGLTFMaterial::getHash() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    // *HACK: hash the bytes of this object but do not include the ref count
    // neither the local texture overrides (which is a map, with pointers to
    // key/value pairs that would change from one LLGLTFMaterial instance to
    // the other, even though the key/value pairs could be the same, and stored
    // elsewhere in the memory heap or on the stack).
    // Note: this does work properly to compare two LLGLTFMaterial instances
    // only because the padding bytes between their member variables have been
    // dutifully zeroed in the constructor. HB
    const size_t offset = intptr_t(&mLocalTexDataDigest) - intptr_t(this);
    return HBXXH128::digest((const void*)((const char*)this + offset),
                            sizeof(*this) - offset);
}

void LLGLTFMaterial::addLocalTextureTracking(const LLUUID& tracking_id, const LLUUID& tex_id)
{
    mTrackingIdToLocalTexture[tracking_id] = tex_id;
    updateLocalTexDataDigest();
}

void LLGLTFMaterial::removeLocalTextureTracking(const LLUUID& tracking_id)
{
    mTrackingIdToLocalTexture.erase(tracking_id);
    updateLocalTexDataDigest();
}

bool LLGLTFMaterial::replaceLocalTexture(const LLUUID& tracking_id, const LLUUID& old_id, const LLUUID& new_id)
{
    bool res = false;

    for (U32 i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        if (mTextureId[i] == old_id)
        {
            mTextureId[i] = new_id;
            res = true;
        }
        else if (mTextureId[i] == new_id)
        {
            res = true;
        }
    }

    if (res)
    {
        mTrackingIdToLocalTexture[tracking_id] = new_id;
    }
    else
    {
        mTrackingIdToLocalTexture.erase(tracking_id);
    }
    updateLocalTexDataDigest();

    return res;
}

void LLGLTFMaterial::updateTextureTracking()
{
    // setTEGLTFMaterialOverride is responsible for tracking
    // for material overrides editor will set it
}
