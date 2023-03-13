/**
 * @file llgltfmaterial.h
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

#pragma once

#include "llrefcount.h"
#include "llmemory.h"
#include "v4color.h"
#include "v3color.h"
#include "v2math.h"
#include "lluuid.h"
#include "hbxxh.h"

#include <string>

namespace tinygltf
{
    class Model;
}

class LLGLTFMaterial : public LLRefCount
{
public:

    // default material for reference
    static const LLGLTFMaterial sDefault;

    static const char* const ASSET_VERSION;
    static const char* const ASSET_TYPE;
    static const std::array<std::string, 2> ACCEPTED_ASSET_VERSIONS;
    static bool isAcceptedVersion(const std::string& version) { return std::find(ACCEPTED_ASSET_VERSIONS.cbegin(), ACCEPTED_ASSET_VERSIONS.cend(), version) != ACCEPTED_ASSET_VERSIONS.cend(); }

    struct TextureTransform
    {
        LLVector2 mOffset = { 0.f, 0.f };
        LLVector2 mScale = { 1.f, 1.f };
        F32 mRotation = 0.f;

        void getPacked(F32 (&packed)[8]);

        bool operator==(const TextureTransform& other) const;
    };

    enum AlphaMode
    {
        ALPHA_MODE_OPAQUE = 0,
        ALPHA_MODE_BLEND,
        ALPHA_MODE_MASK
    };

    LLGLTFMaterial() {}
    LLGLTFMaterial(const LLGLTFMaterial& rhs);

    LLGLTFMaterial& operator=(const LLGLTFMaterial& rhs);
    bool operator==(const LLGLTFMaterial& rhs) const;
    bool operator!=(const LLGLTFMaterial& rhs) const { return !(*this == rhs); }

    enum TextureInfo : U32
    {
        GLTF_TEXTURE_INFO_BASE_COLOR,
        GLTF_TEXTURE_INFO_NORMAL,
        GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS,
        // *NOTE: GLTF_TEXTURE_INFO_OCCLUSION is currently ignored, in favor of
        // the values specified with GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS.
        // Currently, only ORM materials are supported (materials which define
        // occlusion, roughness, and metallic in the same texture).
        // -Cosmic,2023-01-26
        GLTF_TEXTURE_INFO_OCCLUSION = GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS,
        GLTF_TEXTURE_INFO_EMISSIVE,

        GLTF_TEXTURE_INFO_COUNT
    };

    std::array<LLUUID, GLTF_TEXTURE_INFO_COUNT> mTextureId;

    std::array<TextureTransform, GLTF_TEXTURE_INFO_COUNT> mTextureTransform;

    // NOTE: initialize values to defaults according to the GLTF spec
    // NOTE: these values should be in linear color space
    LLColor4 mBaseColor = LLColor4(1, 1, 1, 1);
    LLColor3 mEmissiveColor = LLColor3(0, 0, 0);

    F32 mMetallicFactor = 1.f;
    F32 mRoughnessFactor = 1.f;
    F32 mAlphaCutoff = 0.5f;

    bool mDoubleSided = false;
    AlphaMode mAlphaMode = ALPHA_MODE_OPAQUE;

    // override specific flags for state that can't use off-by-epsilon or UUID hack
    bool mOverrideDoubleSided = false;
    bool mOverrideAlphaMode = false;

    // get a UUID based on a hash of this LLGLTFMaterial
    LLUUID getHash() const
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
        // HACK - hash the bytes of this object but don't include the ref count
        LLUUID hash;
        HBXXH128::digest(hash, (unsigned char*)this + sizeof(S32), sizeof(this) - sizeof(S32));
        return hash;
    }

    //setters for various members (will clamp to acceptable ranges)
    // for_override - set to true if this value is being set as part of an override (important for handling override to default value)

    void setTextureId(TextureInfo texture_info, const LLUUID& id, bool for_override = false);

    void setBaseColorId(const LLUUID& id, bool for_override = false);
    void setNormalId(const LLUUID& id, bool for_override = false);
    void setOcclusionRoughnessMetallicId(const LLUUID& id, bool for_override = false);
    void setEmissiveId(const LLUUID& id, bool for_override = false);

    void setBaseColorFactor(const LLColor4& baseColor, bool for_override = false);
    void setAlphaCutoff(F32 cutoff, bool for_override = false);
    void setEmissiveColorFactor(const LLColor3& emissiveColor, bool for_override = false);
    void setMetallicFactor(F32 metallic, bool for_override = false);
    void setRoughnessFactor(F32 roughness, bool for_override = false);
    void setAlphaMode(S32 mode, bool for_override = false);
    void setDoubleSided(bool double_sided, bool for_override = false);

    //NOTE: texture offsets only exist in overrides, so "for_override" is not needed

    void setTextureOffset(TextureInfo texture_info, const LLVector2& offset);
    void setTextureScale(TextureInfo texture_info, const LLVector2& scale);
    void setTextureRotation(TextureInfo texture_info, float rotation);

    // Default value accessors
    static F32 getDefaultAlphaCutoff();
    static S32 getDefaultAlphaMode();
    static F32 getDefaultMetallicFactor();
    static F32 getDefaultRoughnessFactor();
    static LLColor4 getDefaultBaseColor();
    static LLColor3 getDefaultEmissiveColor();
    static bool getDefaultDoubleSided();
    static LLVector2 getDefaultTextureOffset();
    static LLVector2 getDefaultTextureScale();
    static F32 getDefaultTextureRotation();


    static void hackOverrideUUID(LLUUID& id);
    static void applyOverrideUUID(LLUUID& dst_id, const LLUUID& override_id);

    // set mAlphaMode from string.
    // Anything otherthan "MASK" or "BLEND" sets mAlphaMode to ALPHA_MODE_OPAQUE
    void setAlphaMode(const std::string& mode, bool for_override = false);

    const char* getAlphaMode() const;
    
    // set the contents of this LLGLTFMaterial from the given json
    // returns true if successful
    // json - the json text to load from
    // warn_msg - warning message from TinyGLTF if any
    // error_msg - error_msg from TinyGLTF if any
    bool fromJSON(const std::string& json, std::string& warn_msg, std::string& error_msg);

    // get the contents of this LLGLTFMaterial as a json string
    std::string asJSON(bool prettyprint = false) const;

    // initialize from given tinygltf::Model
    // model - the model to reference
    // mat_index - index of material in model's material array
    void setFromModel(const tinygltf::Model& model, S32 mat_index);

    // write to given tinygltf::Model
    void writeToModel(tinygltf::Model& model, S32 mat_index) const;

    void applyOverride(const LLGLTFMaterial& override_mat);

    // For base materials only (i.e. assets). Clears transforms to
    // default since they're not supported in assets yet.
    void sanitizeAssetMaterial();

    // For material overrides only. Clears most properties to
    // default/fallthrough, but preserves the transforms.
    bool setBaseMaterial();
    // True if setBaseMaterial() was just called
    bool isClearedForBaseMaterial();

private:
    template<typename T>
    void setFromTexture(const tinygltf::Model& model, const T& texture_info, TextureInfo texture_info_id);

    template<typename T>
    void writeToTexture(tinygltf::Model& model, T& texture_info, TextureInfo texture_info_id, bool force_write = false) const;

    void setBaseMaterial(const LLGLTFMaterial& old_override_mat);
};

