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

#include <boost/json.hpp>

const char* const LLGLTFMaterial::ASSET_VERSION = "1.1";
const char* const LLGLTFMaterial::ASSET_TYPE = "GLTF 2.0";
const std::array<std::string, 2> LLGLTFMaterial::ACCEPTED_ASSET_VERSIONS = { "1.0", "1.1" };

const char* const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSFORM = "KHR_texture_transform";
const char* const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSFORM_SCALE = "scale";
const char* const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSFORM_OFFSET = "offset";
const char* const LLGLTFMaterial::GLTF_FILE_EXTENSION_TRANSFORM_ROTATION = "rotation";

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
    mEmissiveStrength = 1.f;
    mSpecularFactor = 1.f;
    mSpecularColorFactor.set(1.f, 1.f, 1.f);
    mIOR = 1.5f;
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

void LLGLTFMaterial::TextureTransform::getPacked(Pack& packed) const
{
    packed[0] = mScale.mV[VX];
    packed[1] = mScale.mV[VY];
    packed[2] = mRotation;
    packed[4] = mOffset.mV[VX];
    packed[5] = mOffset.mV[VY];
    // Not used but nonetheless zeroed for proper hashing. HB
    packed[3] = packed[6] = packed[7] = 0.f;
}

void LLGLTFMaterial::TextureTransform::getPackedTight(PackTight& packed) const
{
    packed[0] = mScale.mV[VX];
    packed[1] = mScale.mV[VY];
    packed[2] = mRotation;
    packed[3] = mOffset.mV[VX];
    packed[4] = mOffset.mV[VY];
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
    mEmissiveStrength = rhs.mEmissiveStrength;
    mSpecularFactor = rhs.mSpecularFactor;
    mSpecularColorFactor = rhs.mSpecularColorFactor;
    mIOR = rhs.mIOR;

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

        mMetallicFactor == rhs.mMetallicFactor &&
        mRoughnessFactor == rhs.mRoughnessFactor &&
        mAlphaCutoff == rhs.mAlphaCutoff &&
        mEmissiveStrength == rhs.mEmissiveStrength &&
        mSpecularFactor == rhs.mSpecularFactor &&
        mSpecularColorFactor == rhs.mSpecularColorFactor &&
        mIOR == rhs.mIOR &&

        mDoubleSided == rhs.mDoubleSided &&
        mAlphaMode == rhs.mAlphaMode &&

        mOverrideDoubleSided == rhs.mOverrideDoubleSided &&
        mOverrideAlphaMode == rhs.mOverrideAlphaMode;
}

bool LLGLTFMaterial::fromJSON(const std::string& json, std::string& warn_msg, std::string& error_msg)
{
    LL_PROFILE_ZONE_SCOPED;
    try
    {
        boost::json::value doc = boost::json::parse(json);
        return setFromDocument(doc, 0);
    }
    catch (const boost::system::system_error& e)
    {
        error_msg = e.what();
        return false;
    }
}

std::string LLGLTFMaterial::asJSON(bool prettyprint) const
{
    LL_PROFILE_ZONE_SCOPED;
    boost::json::value doc = writeDocument();
    return boost::json::serialize(doc);
}

// static
std::string LLGLTFMaterial::getTextureURI(const boost::json::value& doc, S32 texture_index)
{
    if (texture_index < 0) return "";

    const auto* textures = doc.if_object() ? doc.as_object().if_contains("textures") : nullptr;
    if (!textures || !textures->is_array()) return "";

    const auto& tex_arr = textures->as_array();
    if ((size_t)texture_index >= tex_arr.size()) return "";

    const auto& tex = tex_arr[(size_t)texture_index];
    if (!tex.is_object()) return "";

    const auto* source_val = tex.as_object().if_contains("source");
    if (!source_val || !source_val->is_int64()) return "";

    S32 source_idx = (S32)source_val->as_int64();
    if (source_idx < 0) return "";

    const auto* images = doc.as_object().if_contains("images");
    if (!images || !images->is_array()) return "";

    const auto& img_arr = images->as_array();
    if ((size_t)source_idx >= img_arr.size()) return "";

    const auto& img = img_arr[(size_t)source_idx];
    if (!img.is_object()) return "";

    const auto* uri_val = img.as_object().if_contains("uri");
    if (!uri_val || !uri_val->is_string()) return "";

    return std::string(uri_val->as_string());
}

// static
void LLGLTFMaterial::readTextureInfo(const boost::json::value& doc, const boost::json::object& tex_info, LLUUID& texture_id, TextureTransform& transform)
{
    LL_PROFILE_ZONE_SCOPED;
    const auto* index_val = tex_info.if_contains("index");
    if (!index_val || !index_val->is_int64()) return;

    S32 tex_idx = (S32)index_val->as_int64();
    std::string uri = getTextureURI(doc, tex_idx);
    texture_id.set(uri);

    const auto* extensions_val = tex_info.if_contains("extensions");
    if (extensions_val && extensions_val->is_object())
    {
        const auto& extensions = extensions_val->as_object();
        const auto* transform_val = extensions.if_contains(GLTF_FILE_EXTENSION_TRANSFORM);
        if (transform_val && transform_val->is_object())
        {
            const auto& transform_obj = transform_val->as_object();
            transform.mOffset = vec2FromJson(transform_obj, GLTF_FILE_EXTENSION_TRANSFORM_OFFSET, getDefaultTextureOffset());
            transform.mScale = vec2FromJson(transform_obj, GLTF_FILE_EXTENSION_TRANSFORM_SCALE, getDefaultTextureScale());
            transform.mRotation = floatFromJson(transform_obj, GLTF_FILE_EXTENSION_TRANSFORM_ROTATION, getDefaultTextureRotation());
        }
    }
}

void LLGLTFMaterial::readTextureInfo(const boost::json::value& doc, const boost::json::object& tex_info, TextureInfo id)
{
    readTextureInfo(doc, tex_info, mTextureId[id], mTextureTransform[id]);
}

bool LLGLTFMaterial::setFromDocument(const boost::json::value& doc, S32 mat_index)
{
    LL_PROFILE_ZONE_SCOPED;
    if (!doc.is_object()) return false;

    const auto& root = doc.as_object();
    const auto* materials_val = root.if_contains("materials");
    if (!materials_val || !materials_val->is_array()) return false;

    const auto& materials = materials_val->as_array();
    if ((size_t)mat_index >= materials.size()) return false;

    const auto& mat = materials[(size_t)mat_index];
    if (!mat.is_object()) return false;

    const auto& mat_obj = mat.as_object();

    // PBR metallic roughness
    const auto* pbr_val = mat_obj.if_contains("pbrMetallicRoughness");
    if (pbr_val && pbr_val->is_object())
    {
        const auto& pbr = pbr_val->as_object();

        const auto* base_color_tex = pbr.if_contains("baseColorTexture");
        if (base_color_tex && base_color_tex->is_object())
        {
            readTextureInfo(doc, base_color_tex->as_object(), GLTF_TEXTURE_INFO_BASE_COLOR);
        }

        const auto* mr_tex = pbr.if_contains("metallicRoughnessTexture");
        if (mr_tex && mr_tex->is_object())
        {
            readTextureInfo(doc, mr_tex->as_object(), GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS);
        }

        const auto* bcf = pbr.if_contains("baseColorFactor");
        if (bcf && bcf->is_array())
        {
            const auto& arr = bcf->as_array();
            if (arr.size() >= 4)
            {
                mBaseColor.set(
                    (F32)arr[0].to_number<double>(),
                    (F32)arr[1].to_number<double>(),
                    (F32)arr[2].to_number<double>(),
                    (F32)arr[3].to_number<double>());
            }
        }

        const auto* mf = pbr.if_contains("metallicFactor");
        if (mf && mf->is_number())
        {
            mMetallicFactor = llclamp((F32)mf->to_number<double>(), 0.f, 1.f);
        }

        const auto* rf = pbr.if_contains("roughnessFactor");
        if (rf && rf->is_number())
        {
            mRoughnessFactor = llclamp((F32)rf->to_number<double>(), 0.f, 1.f);
        }
    }

    // Normal texture
    const auto* normal_tex = mat_obj.if_contains("normalTexture");
    if (normal_tex && normal_tex->is_object())
    {
        readTextureInfo(doc, normal_tex->as_object(), GLTF_TEXTURE_INFO_NORMAL);
    }

    // Emissive texture
    const auto* emissive_tex = mat_obj.if_contains("emissiveTexture");
    if (emissive_tex && emissive_tex->is_object())
    {
        readTextureInfo(doc, emissive_tex->as_object(), GLTF_TEXTURE_INFO_EMISSIVE);
    }

    // Alpha mode
    const auto* am = mat_obj.if_contains("alphaMode");
    if (am && am->is_string())
    {
        setAlphaMode(std::string(am->as_string()));
    }

    // Alpha cutoff
    const auto* ac = mat_obj.if_contains("alphaCutoff");
    if (ac && ac->is_number())
    {
        mAlphaCutoff = llclamp((F32)ac->to_number<double>(), 0.f, 1.f);
    }

    // Emissive factor
    const auto* ef = mat_obj.if_contains("emissiveFactor");
    if (ef && ef->is_array())
    {
        const auto& arr = ef->as_array();
        if (arr.size() >= 3)
        {
            mEmissiveColor.set(
                (F32)arr[0].to_number<double>(),
                (F32)arr[1].to_number<double>(),
                (F32)arr[2].to_number<double>());
        }
    }

    // Double sided
    const auto* ds = mat_obj.if_contains("doubleSided");
    if (ds && ds->is_bool())
    {
        mDoubleSided = ds->as_bool();
    }

    // Material extensions
    const auto* extensions_val = mat_obj.if_contains("extensions");
    if (extensions_val && extensions_val->is_object())
    {
        const auto& extensions = extensions_val->as_object();

        // KHR_materials_emissive_strength
        const auto* emissive_strength_ext = extensions.if_contains("KHR_materials_emissive_strength");
        if (emissive_strength_ext && emissive_strength_ext->is_object())
        {
            const auto& es = emissive_strength_ext->as_object();
            const auto* es_val = es.if_contains("emissiveStrength");
            if (es_val && es_val->is_number())
            {
                mEmissiveStrength = llmax((F32)es_val->to_number<double>(), 0.f);
            }
        }

        // KHR_materials_specular
        const auto* specular_ext = extensions.if_contains("KHR_materials_specular");
        if (specular_ext && specular_ext->is_object())
        {
            const auto& spec = specular_ext->as_object();

            const auto* sf = spec.if_contains("specularFactor");
            if (sf && sf->is_number())
            {
                mSpecularFactor = llclamp((F32)sf->to_number<double>(), 0.f, 1.f);
            }

            const auto* scf = spec.if_contains("specularColorFactor");
            if (scf && scf->is_array())
            {
                const auto& arr = scf->as_array();
                if (arr.size() >= 3)
                {
                    mSpecularColorFactor.set(
                        (F32)arr[0].to_number<double>(),
                        (F32)arr[1].to_number<double>(),
                        (F32)arr[2].to_number<double>());
                }
            }

            const auto* spec_tex = spec.if_contains("specularTexture");
            if (spec_tex && spec_tex->is_object())
            {
                readTextureInfo(doc, spec_tex->as_object(), GLTF_TEXTURE_INFO_SPECULAR);
            }

            const auto* spec_color_tex = spec.if_contains("specularColorTexture");
            if (spec_color_tex && spec_color_tex->is_object())
            {
                readTextureInfo(doc, spec_color_tex->as_object(), GLTF_TEXTURE_INFO_SPECULAR);
            }
        }

        // KHR_materials_ior
        const auto* ior_ext = extensions.if_contains("KHR_materials_ior");
        if (ior_ext && ior_ext->is_object())
        {
            const auto& ior_obj = ior_ext->as_object();
            const auto* ior_val = ior_obj.if_contains("ior");
            if (ior_val && ior_val->is_number())
            {
                mIOR = llmax((F32)ior_val->to_number<double>(), 0.f);
            }
        }
    }

    // Extras
    const auto* extras_val = mat_obj.if_contains("extras");
    if (extras_val && extras_val->is_object())
    {
        const auto& extras = extras_val->as_object();
        const auto* override_am = extras.if_contains("override_alpha_mode");
        if (override_am && override_am->is_bool())
        {
            mOverrideAlphaMode = override_am->as_bool();
        }

        const auto* override_ds = extras.if_contains("override_double_sided");
        if (override_ds && override_ds->is_bool())
        {
            mOverrideDoubleSided = override_ds->as_bool();
        }
    }

    return true;
}

// static
LLVector2 LLGLTFMaterial::vec2FromJson(const boost::json::object& obj, const char* key, const LLVector2& default_value)
{
    const auto* val = obj.if_contains(key);
    if (!val || !val->is_array()) return default_value;

    const auto& arr = val->as_array();
    if (arr.size() < LENGTHOFVECTOR2) return default_value;

    LLVector2 result;
    for (U32 i = 0; i < LENGTHOFVECTOR2; ++i)
    {
        if (!arr[i].is_number()) return default_value;
        result.mV[i] = (F32)arr[i].to_number<double>();
    }
    return result;
}

// static
F32 LLGLTFMaterial::floatFromJson(const boost::json::object& obj, const char* key, F32 default_value)
{
    const auto* val = obj.if_contains(key);
    if (!val || !val->is_number()) return default_value;
    return (F32)val->to_number<double>();
}

// static
void LLGLTFMaterial::writeTextureEntry(boost::json::array& images, boost::json::array& textures, boost::json::object& dst, const char* key, const LLUUID& texture_id, const TextureTransform& transform, bool force_write)
{
    LL_PROFILE_ZONE_SCOPED;
    const bool is_blank_transform = transform == sDefault.mTextureTransform[0];
    if (!force_write && texture_id.isNull() && is_blank_transform)
    {
        return;
    }

    // Add image entry
    S32 image_idx = (S32)images.size();
    boost::json::object image_obj;
    image_obj["uri"] = texture_id.asString();
    images.push_back(image_obj);

    // Add texture entry
    S32 texture_idx = (S32)textures.size();
    boost::json::object tex_obj;
    tex_obj["source"] = image_idx;
    textures.push_back(tex_obj);

    // Build texture info
    boost::json::object tex_info;
    tex_info["index"] = texture_idx;

    if (!is_blank_transform)
    {
        boost::json::object transform_obj;
        transform_obj[GLTF_FILE_EXTENSION_TRANSFORM_OFFSET] = boost::json::array({
            boost::json::value(transform.mOffset.mV[VX]),
            boost::json::value(transform.mOffset.mV[VY])
        });
        transform_obj[GLTF_FILE_EXTENSION_TRANSFORM_SCALE] = boost::json::array({
            boost::json::value(transform.mScale.mV[VX]),
            boost::json::value(transform.mScale.mV[VY])
        });
        transform_obj[GLTF_FILE_EXTENSION_TRANSFORM_ROTATION] = transform.mRotation;

        boost::json::object extensions;
        extensions[GLTF_FILE_EXTENSION_TRANSFORM] = transform_obj;
        tex_info["extensions"] = extensions;
    }

    dst[key] = tex_info;
}

boost::json::value LLGLTFMaterial::writeDocument() const
{
    LL_PROFILE_ZONE_SCOPED;
    boost::json::object root;

    // Asset info
    boost::json::object asset;
    asset["version"] = "2.0";
    root["asset"] = asset;

    boost::json::array images;
    boost::json::array textures;

    // Build material
    boost::json::object mat_obj;

    // PBR metallic roughness
    boost::json::object pbr;
    bool has_pbr = false;

    // Base color texture
    writeTextureEntry(images, textures, pbr, "baseColorTexture",
        mTextureId[GLTF_TEXTURE_INFO_BASE_COLOR], mTextureTransform[GLTF_TEXTURE_INFO_BASE_COLOR]);
    if (pbr.contains("baseColorTexture")) has_pbr = true;

    // Metallic roughness texture
    writeTextureEntry(images, textures, pbr, "metallicRoughnessTexture",
        mTextureId[GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS], mTextureTransform[GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS]);
    if (pbr.contains("metallicRoughnessTexture")) has_pbr = true;

    // Base color factor
    boost::json::array bcf = {
        boost::json::value((double)mBaseColor.mV[VRED]),
        boost::json::value((double)mBaseColor.mV[VGREEN]),
        boost::json::value((double)mBaseColor.mV[VBLUE]),
        boost::json::value((double)mBaseColor.mV[VALPHA])
    };
    if (mBaseColor != getDefaultBaseColor())
    {
        pbr["baseColorFactor"] = bcf;
        has_pbr = true;
    }

    if (mMetallicFactor != getDefaultMetallicFactor())
    {
        pbr["metallicFactor"] = (double)mMetallicFactor;
        has_pbr = true;
    }

    if (mRoughnessFactor != getDefaultRoughnessFactor())
    {
        pbr["roughnessFactor"] = (double)mRoughnessFactor;
        has_pbr = true;
    }

    if (has_pbr)
    {
        // Always write base color factor if we have PBR at all
        if (!pbr.contains("baseColorFactor"))
        {
            pbr["baseColorFactor"] = bcf;
        }
        if (!pbr.contains("metallicFactor"))
        {
            pbr["metallicFactor"] = (double)mMetallicFactor;
        }
        if (!pbr.contains("roughnessFactor"))
        {
            pbr["roughnessFactor"] = (double)mRoughnessFactor;
        }
        mat_obj["pbrMetallicRoughness"] = pbr;
    }

    // Normal texture
    boost::json::object normal_dst;
    writeTextureEntry(images, textures, normal_dst, "normalTexture_tmp",
        mTextureId[GLTF_TEXTURE_INFO_NORMAL], mTextureTransform[GLTF_TEXTURE_INFO_NORMAL]);
    if (normal_dst.contains("normalTexture_tmp"))
    {
        mat_obj["normalTexture"] = normal_dst["normalTexture_tmp"];
    }

    // Emissive texture
    boost::json::object emissive_dst;
    writeTextureEntry(images, textures, emissive_dst, "emissiveTexture_tmp",
        mTextureId[GLTF_TEXTURE_INFO_EMISSIVE], mTextureTransform[GLTF_TEXTURE_INFO_EMISSIVE]);
    if (emissive_dst.contains("emissiveTexture_tmp"))
    {
        mat_obj["emissiveTexture"] = emissive_dst["emissiveTexture_tmp"];
    }

    // Occlusion texture (GLTF compliance for ORM)
    boost::json::object occlusion_dst;
    writeTextureEntry(images, textures, occlusion_dst, "occlusionTexture_tmp",
        mTextureId[GLTF_TEXTURE_INFO_OCCLUSION], mTextureTransform[GLTF_TEXTURE_INFO_OCCLUSION]);
    if (occlusion_dst.contains("occlusionTexture_tmp"))
    {
        mat_obj["occlusionTexture"] = occlusion_dst["occlusionTexture_tmp"];
    }

    mat_obj["alphaMode"] = getAlphaMode();
    mat_obj["alphaCutoff"] = (double)mAlphaCutoff;
    mat_obj["doubleSided"] = mDoubleSided;

    if (mEmissiveColor != getDefaultEmissiveColor())
    {
        mat_obj["emissiveFactor"] = boost::json::array({
            boost::json::value((double)mEmissiveColor.mV[0]),
            boost::json::value((double)mEmissiveColor.mV[1]),
            boost::json::value((double)mEmissiveColor.mV[2])
        });
    }

    // Extras
    boost::json::object extras;
    bool write_extras = false;
    if (mOverrideAlphaMode && mAlphaMode == getDefaultAlphaMode())
    {
        extras["override_alpha_mode"] = true;
        write_extras = true;
    }
    if (mOverrideDoubleSided && mDoubleSided == getDefaultDoubleSided())
    {
        extras["override_double_sided"] = true;
        write_extras = true;
    }
    if (write_extras)
    {
        mat_obj["extras"] = extras;
    }

    // Material extensions
    boost::json::object mat_extensions;

    // KHR_materials_emissive_strength
    if (mEmissiveStrength != getDefaultEmissiveStrength())
    {
        boost::json::object es_ext;
        es_ext["emissiveStrength"] = (double)mEmissiveStrength;
        mat_extensions["KHR_materials_emissive_strength"] = es_ext;
    }

    // KHR_materials_specular
    {
        bool has_specular = false;
        boost::json::object specular_ext;

        if (mSpecularFactor != getDefaultSpecularFactor())
        {
            specular_ext["specularFactor"] = (double)mSpecularFactor;
            has_specular = true;
        }

        if (mSpecularColorFactor != getDefaultSpecularColorFactor())
        {
            specular_ext["specularColorFactor"] = boost::json::array({
                boost::json::value((double)mSpecularColorFactor.mV[0]),
                boost::json::value((double)mSpecularColorFactor.mV[1]),
                boost::json::value((double)mSpecularColorFactor.mV[2])
            });
            has_specular = true;
        }

        boost::json::object spec_tex_dst;
        writeTextureEntry(images, textures, spec_tex_dst, "specularTexture_tmp",
            mTextureId[GLTF_TEXTURE_INFO_SPECULAR], mTextureTransform[GLTF_TEXTURE_INFO_SPECULAR]);
        if (spec_tex_dst.contains("specularTexture_tmp"))
        {
            specular_ext["specularTexture"] = spec_tex_dst["specularTexture_tmp"];
            specular_ext["specularColorTexture"] = spec_tex_dst["specularTexture_tmp"];
            has_specular = true;
        }

        if (has_specular)
        {
            mat_extensions["KHR_materials_specular"] = specular_ext;
        }
    }

    // KHR_materials_ior
    if (mIOR != getDefaultIOR())
    {
        boost::json::object ior_ext;
        ior_ext["ior"] = (double)mIOR;
        mat_extensions["KHR_materials_ior"] = ior_ext;
    }

    if (!mat_extensions.empty())
    {
        mat_obj["extensions"] = mat_extensions;
    }

    boost::json::array materials;
    materials.push_back(mat_obj);
    root["materials"] = materials;

    if (!images.empty())
    {
        root["images"] = images;
    }
    if (!textures.empty())
    {
        root["textures"] = textures;
    }

    return root;
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

void LLGLTFMaterial::setSpecularId(const LLUUID& id, bool for_override)
{
    setTextureId(GLTF_TEXTURE_INFO_SPECULAR, id, for_override);
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

void LLGLTFMaterial::setEmissiveStrength(F32 strength, bool for_override)
{
    mEmissiveStrength = llmax(strength, 0.f);
    if (for_override)
    {
        if (mEmissiveStrength == getDefaultEmissiveStrength())
        {
            mEmissiveStrength -= FLT_EPSILON;
        }
    }
}

void LLGLTFMaterial::setSpecularFactor(F32 factor, bool for_override)
{
    mSpecularFactor = llclamp(factor, 0.f, for_override ? 1.f - FLT_EPSILON : 1.f);
}

void LLGLTFMaterial::setIOR(F32 ior, bool for_override)
{
    mIOR = llmax(ior, 0.f);
    if (for_override && mIOR == getDefaultIOR())
    {
        mIOR -= FLT_EPSILON;
    }
}

void LLGLTFMaterial::setSpecularColorFactor(const LLColor3& color, bool for_override)
{
    mSpecularColorFactor = color;
    mSpecularColorFactor.clamp();

    if (for_override)
    {
        if (mSpecularColorFactor == getDefaultSpecularColorFactor())
        {
            mSpecularColorFactor.mV[0] -= FLT_EPSILON;
        }
    }
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

F32 LLGLTFMaterial::getDefaultEmissiveStrength()
{
    return sDefault.mEmissiveStrength;
}

F32 LLGLTFMaterial::getDefaultSpecularFactor()
{
    return sDefault.mSpecularFactor;
}

LLColor3 LLGLTFMaterial::getDefaultSpecularColorFactor()
{
    return sDefault.mSpecularColorFactor;
}

F32 LLGLTFMaterial::getDefaultIOR()
{
    return sDefault.mIOR;
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

    if (override_mat.mEmissiveStrength != getDefaultEmissiveStrength())
    {
        mEmissiveStrength = override_mat.mEmissiveStrength;
    }

    if (override_mat.mSpecularFactor != getDefaultSpecularFactor())
    {
        mSpecularFactor = override_mat.mSpecularFactor;
    }

    if (override_mat.mSpecularColorFactor != getDefaultSpecularColorFactor())
    {
        mSpecularColorFactor = override_mat.mSpecularColorFactor;
    }

    if (override_mat.mIOR != getDefaultIOR())
    {
        mIOR = override_mat.mIOR;
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

void LLGLTFMaterial::getOverrideLLSD(const LLGLTFMaterial& override_mat, LLSD& data) const
{
    LL_PROFILE_ZONE_SCOPED;
    llassert(data.isUndefined());

    // make every effort to shave bytes here

    for (U32 i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        const LLUUID& texture_id = mTextureId[i];
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

    if (override_mat.mEmissiveStrength != getDefaultEmissiveStrength())
    {
        data["es"] = override_mat.mEmissiveStrength;
    }

    if (override_mat.mSpecularFactor != getDefaultSpecularFactor())
    {
        data["sf"] = override_mat.mSpecularFactor;
    }

    if (override_mat.mSpecularColorFactor != getDefaultSpecularColorFactor())
    {
        data["sc"] = override_mat.mSpecularColorFactor.getValue();
    }

    if (override_mat.mIOR != getDefaultIOR())
    {
        data["ior"] = override_mat.mIOR;
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

    const LLSD& mf = data["mf"];
    if (mf.isReal())
    {
        mMetallicFactor = (F32)mf.asReal();
        if (mMetallicFactor == getDefaultMetallicFactor())
        {
            // HACK -- nudge by epsilon if we receive a default value (indicates override to default)
            mMetallicFactor -= FLT_EPSILON;
        }
    }

    const LLSD& rf = data["rf"];
    if (rf.isReal())
    {
        mRoughnessFactor = (F32)rf.asReal();
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
        mAlphaCutoff = (F32)ac.asReal();
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

    const LLSD& es = data["es"];
    if (es.isReal())
    {
        mEmissiveStrength = (F32)es.asReal();
        if (mEmissiveStrength == getDefaultEmissiveStrength())
        {
            mEmissiveStrength -= FLT_EPSILON;
        }
    }

    const LLSD& sf = data["sf"];
    if (sf.isReal())
    {
        mSpecularFactor = (F32)sf.asReal();
        if (mSpecularFactor == getDefaultSpecularFactor())
        {
            mSpecularFactor -= FLT_EPSILON;
        }
    }

    const LLSD& sc = data["sc"];
    if (sc.isDefined())
    {
        mSpecularColorFactor.setValue(sc);
        if (mSpecularColorFactor == getDefaultSpecularColorFactor())
        {
            mSpecularColorFactor.mV[0] -= FLT_EPSILON;
        }
    }

    const LLSD& ior = data["ior"];
    if (ior.isReal())
    {
        mIOR = (F32)ior.asReal();
        if (mIOR == getDefaultIOR())
        {
            mIOR -= FLT_EPSILON;
        }
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
                mTextureTransform[i].mRotation = (F32)r.asReal();
            }
        }
    }
}

LLUUID LLGLTFMaterial::getHash() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    // Hash each field explicitly rather than hashing raw object bytes.
    // Raw byte hashing is fragile across compilers/platforms due to
    // struct padding and std::map layout side effects.
    HBXXH128 hash;

    hash.update(&mLocalTexDataDigest, sizeof(mLocalTexDataDigest));

    for (U32 i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        hash.update(mTextureId[i].mData, UUID_BYTES);
    }

    for (U32 i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        TextureTransform::Pack packed;
        mTextureTransform[i].getPacked(packed);
        hash.update(packed, sizeof(packed));
    }

    hash.update(mBaseColor.mV, sizeof(mBaseColor.mV));
    hash.update(mEmissiveColor.mV, sizeof(mEmissiveColor.mV));

    hash.update(&mMetallicFactor, sizeof(mMetallicFactor));
    hash.update(&mRoughnessFactor, sizeof(mRoughnessFactor));
    hash.update(&mAlphaCutoff, sizeof(mAlphaCutoff));
    hash.update(&mEmissiveStrength, sizeof(mEmissiveStrength));
    hash.update(&mSpecularFactor, sizeof(mSpecularFactor));
    hash.update(mSpecularColorFactor.mV, sizeof(mSpecularColorFactor.mV));
    hash.update(&mIOR, sizeof(mIOR));

    hash.update(&mAlphaMode, sizeof(mAlphaMode));
    hash.update(&mDoubleSided, sizeof(mDoubleSided));
    hash.update(&mOverrideDoubleSided, sizeof(mOverrideDoubleSided));
    hash.update(&mOverrideAlphaMode, sizeof(mOverrideAlphaMode));

    hash.finalize();
    return hash.digest();
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

// Test cases:
// Case 1.
// Input: scale 1.0,1.0; Offset horizontal 0.0, Offset vertical 0.0 Rotation 0.349066;
// Expected output: scale 1.0,1.0; Offset horizontal 0.201, Offset vertical -0.141 Rotation -0.349066;
// Case 2.
// Input: scale 1.0,1.0; Offset horizontal 0.5, Offset vertical 0.1 Rotation 0;
// Expected output: scale 1.0,1.0; Offset horizontal 0.5, Offset vertical -0.1 Rotation -0;
// Case 3.
// Input: scale 1.0,1.0; Offset horizontal 0.1, Offset vertical 0.2 Rotation 0.349066;
// Expected output: scale 1.0,1.0; Offset horizontal 0.295, Offset vertical -0.345 Rotation -0.349066;
// Case 4.
// Input: scale 1.0,1.0; Offset horizontal 0.5, Offset vertical 0.0 Rotation 0.349066;
// Expected output: scale 1.0,1.0; Offset horizontal 0.701, Offset vertical -0.141 Rotation -0.349066;
//
// Legacy offsets are right to left and top to bottom.
// PBR offsets are right to left and bottom to top.
//
// Legacy rotation is relative to face's center counter clockwise,
// PBR rotation is relative to top-left corner, clockwise
void LLGLTFMaterial::convertTextureTransformToPBR(
    F32 tex_scale_s,
    F32 tex_scale_t,
    F32 tex_offset_s,
    F32 tex_offset_t,
    F32 tex_rotation,
    LLVector2& pbr_scale,
    LLVector2& pbr_offset,
    F32& pbr_rotation)
{
    pbr_scale.set(tex_scale_s, tex_scale_t);
    pbr_rotation = -tex_rotation;

    // Center of the tile
    const F32 center_s = 0.5f;
    const F32 center_t = 0.5f;

    // Center adjustment for scale
    F32 center_adjust_s = 0.5f * (1.0f - tex_scale_s);
    F32 center_adjust_t = 0.5f * (1.0f - tex_scale_t);

    // 2. Offset from center
    F32 pos_s = center_adjust_s - center_s;
    F32 pos_t = center_adjust_t - center_t;

    // 3. Rotate around center (clockwise, as per GLTF spec)
    F32 c = cosf(pbr_rotation);
    F32 s = sinf(pbr_rotation);
    F32 rot_s = pos_s * c + pos_t * s;
    F32 rot_t = -pos_s * s + pos_t * c;

    // 4. Move back to top-left and apply offset
    pbr_offset.set(rot_s + center_s + tex_offset_s, rot_t + center_t - tex_offset_t);
}

// Convert PBR transform values back to legacy TE transform values.
// This is the reverse of convertTextureTransformToPBR.
void LLGLTFMaterial::convertPBRTransformToTexture(
    const LLVector2& pbr_scale,
    const LLVector2& pbr_offset,
    F32 pbr_rotation,
    F32& tex_scale_s,
    F32& tex_scale_t,
    F32& tex_offset_s,
    F32& tex_offset_t,
    F32& tex_rotation)
{
    tex_scale_s = pbr_scale.mV[0];
    tex_scale_t = pbr_scale.mV[1];
    tex_rotation = -pbr_rotation;

    // Center of the tile
    const F32 center_s = 0.5f;
    const F32 center_t = 0.5f;

    // Center adjustment for scale
    F32 center_adjust_s = 0.5f * (1.0f - tex_scale_s);
    F32 center_adjust_t = 0.5f * (1.0f - tex_scale_t);

    // 2. Offset from center
    F32 pos_s = center_adjust_s - center_s;
    F32 pos_t = center_adjust_t - center_t;

    // 3. Rotate around center (clockwise, as per GLTF spec)
    F32 c = cosf(pbr_rotation);
    F32 s = sinf(pbr_rotation);
    F32 rot_s = pos_s * c + pos_t * s;
    F32 rot_t = -pos_s * s + pos_t * c;

    // 3. Recover legacy offset
    tex_offset_s = pbr_offset.mV[0] - rot_s - center_s;
    tex_offset_t = -(pbr_offset.mV[1] - rot_t - center_t);
}
