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

// special UUID that indicates a null UUID in override data
const LLUUID LLGLTFMaterial::GLTF_OVERRIDE_NULL_UUID = LLUUID("ffffffff-ffff-ffff-ffff-ffffffffffff");

void LLGLTFMaterial::TextureTransform::getPacked(F32 (&packed)[8]) const
{
    packed[0] = mScale.mV[VX];
    packed[1] = mScale.mV[VY];
    packed[2] = mRotation;
    // packed[3] = unused
    packed[4] = mOffset.mV[VX];
    packed[5] = mOffset.mV[VY];
    // packed[6] = unused
    // packed[7] = unused
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

    mMetallicFactor = rhs.mMetallicFactor;
    mRoughnessFactor = rhs.mRoughnessFactor;
    mAlphaCutoff = rhs.mAlphaCutoff;

    mDoubleSided = rhs.mDoubleSided;
    mAlphaMode = rhs.mAlphaMode;

    mOverrideDoubleSided = rhs.mOverrideDoubleSided;
    mOverrideAlphaMode = rhs.mOverrideAlphaMode;

    mLocalTextureIds = rhs.mLocalTextureIds;
    mLocalTextureTrackingIds = rhs.mLocalTextureTrackingIds;

    updateTextureTracking();

    return *this;
}

bool LLGLTFMaterial::operator==(const LLGLTFMaterial& rhs) const
{
    return mTextureId == rhs.mTextureId &&

        mTextureTransform == rhs.mTextureTransform &&

        mBaseColor == rhs.mBaseColor &&
        mEmissiveColor == rhs.mEmissiveColor &&

        mMetallicFactor == rhs.mMetallicFactor &&
        mRoughnessFactor == rhs.mRoughnessFactor &&
        mAlphaCutoff == rhs.mAlphaCutoff &&

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

    // Apply base color texture
    setFromTexture(model, material_in.pbrMetallicRoughness.baseColorTexture, GLTF_TEXTURE_INFO_BASE_COLOR);
    // Apply normal map
    setFromTexture(model, material_in.normalTexture, GLTF_TEXTURE_INFO_NORMAL);
    // Apply metallic-roughness texture
    setFromTexture(model, material_in.pbrMetallicRoughness.metallicRoughnessTexture, GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS);
    // Apply emissive texture
    setFromTexture(model, material_in.emissiveTexture, GLTF_TEXTURE_INFO_EMISSIVE);

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

void LLGLTFMaterial::writeToModel(tinygltf::Model& model, S32 mat_index) const
{
    LL_PROFILE_ZONE_SCOPED;
    if (model.materials.size() < mat_index+1)
    {
        model.materials.resize(mat_index + 1);
    }

    tinygltf::Material& material_out = model.materials[mat_index];

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


    material_out.alphaMode = getAlphaMode();
    material_out.alphaCutoff = mAlphaCutoff;

    mBaseColor.write(material_out.pbrMetallicRoughness.baseColorFactor);

    if (mEmissiveColor != LLGLTFMaterial::getDefaultEmissiveColor())
    {
        material_out.emissiveFactor.resize(3);
        mEmissiveColor.write(material_out.emissiveFactor);
    }

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

void LLGLTFMaterial::setMetallicFactor(F32 metallic, bool for_override)
{
    mMetallicFactor = llclamp(metallic, 0.f, for_override ? 1.f - FLT_EPSILON : 1.f);
}

void LLGLTFMaterial::setRoughnessFactor(F32 roughness, bool for_override)
{
    mRoughnessFactor = llclamp(roughness, 0.f, for_override ? 1.f - FLT_EPSILON : 1.f);
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

LLColor4 LLGLTFMaterial::getDefaultBaseColor()
{
    return sDefault.mBaseColor;
}

LLColor3 LLGLTFMaterial::getDefaultEmissiveColor()
{
    return sDefault.mEmissiveColor;
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

    for (int i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
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

    for (int i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
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
}

void LLGLTFMaterial::getOverrideLLSD(const LLGLTFMaterial& override_mat, LLSD& data)
{
    LL_PROFILE_ZONE_SCOPED;
    llassert(data.isUndefined());

    // make every effort to shave bytes here

    for (int i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
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

    if (override_mat.mMetallicFactor != getDefaultMetallicFactor())
    {
        data["mf"] = override_mat.mMetallicFactor;
    }

    if (override_mat.mRoughnessFactor != getDefaultRoughnessFactor())
    {
        data["rf"] = override_mat.mRoughnessFactor;
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

    for (int i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
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
    }

    const LLSD& ec = data["ec"];
    if (ec.isDefined())
    {
        mEmissiveColor.setValue(ec);
    }

    const LLSD& mf = data["mf"];
    if (mf.isReal())
    {
        mMetallicFactor = mf.asReal();
    }

    const LLSD& rf = data["rf"];
    if (rf.isReal())
    {
        mRoughnessFactor = rf.asReal();
    }

    const LLSD& am = data["am"];
    if (am.isInteger())
    {
        mAlphaMode = (AlphaMode) am.asInteger();
    }

    const LLSD& ac = data["ac"];
    if (ac.isReal())
    {
        mAlphaCutoff = ac.asReal();
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
        for (int i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
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
    // HACK - hash the bytes of this object but don't include the ref count
    LLUUID hash;
    HBXXH128::digest(hash, (unsigned char*)this + sizeof(LLRefCount), sizeof(*this) - sizeof(LLRefCount));
    return hash;
}

void LLGLTFMaterial::addTextureEntry(LLTextureEntry* te)
{
    mTextureEntires.insert(te);
}
void LLGLTFMaterial::removeTextureEntry(LLTextureEntry* te)
{
    mTextureEntires.erase(te);
}

void LLGLTFMaterial::addLocalTextureTracking(const LLUUID& tracking_id, const LLUUID& tex_id)
{
    mLocalTextureTrackingIds.insert(tracking_id);
    mLocalTextureIds.insert(tex_id);
}

void LLGLTFMaterial::removeLocalTextureTracking(const LLUUID& tracking_id, const LLUUID& tex_id)
{
    mLocalTextureTrackingIds.erase(tracking_id);
    mLocalTextureIds.erase(tex_id);
}

bool LLGLTFMaterial::replaceLocalTexture(const LLUUID& old_id, const LLUUID& new_id)
{
    bool res = false;

    for (int i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        if (mTextureId[i] == old_id)
        {
            mTextureId[i] = new_id;
            res = true;
        }
    }

    mLocalTextureIds.erase(old_id);
    if (res)
    {
        mLocalTextureIds.insert(new_id);
    }

    return res;
}

void LLGLTFMaterial::updateTextureTracking()
{
    if (mLocalTextureTrackingIds.size() > 0)
    {
        LL_WARNS() << "copied a material with local textures, but tracking not implemented" << LL_ENDL;
    }
}
