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
#include "llmd5.h"

#include <string>

namespace tinygltf
{
    class Model;
}

class LLGLTFMaterial : public LLRefCount
{
public:

    struct TextureTransform
    {
        LLVector2 mOffset = { 0.f, 0.f };
        LLVector2 mScale = { 1.f, 1.f };
        F32 mRotation = 0.f;
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

    LLUUID mBaseColorId;
    LLUUID mNormalId;
    LLUUID mMetallicRoughnessId;
    LLUUID mEmissiveId;

    LLColor4 mBaseColor = LLColor4(1, 1, 1, 1);
    LLColor3 mEmissiveColor = LLColor3(0, 0, 0);

    F32 mMetallicFactor = 0.f;
    F32 mRoughnessFactor = 0.f;
    F32 mAlphaCutoff = 0.f;

    bool mDoubleSided = false;
    AlphaMode mAlphaMode = ALPHA_MODE_OPAQUE;

    // get a UUID based on a hash of this LLGLTFMaterial
    LLUUID getHash() const
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
        LLMD5 md5;
        md5.update((unsigned char*)this, sizeof(this));
        md5.finalize();
        LLUUID id;
        md5.raw_digest(id.mData);
        // *TODO: Hash the overrides
        return id;
    }

    enum TextureInfo : U32
    {
        GLTF_TEXTURE_INFO_BASE_COLOR,
        GLTF_TEXTURE_INFO_NORMAL,
        GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS,
        GLTF_TEXTURE_INFO_EMISSIVE,

        GLTF_TEXTURE_INFO_COUNT
    };

    std::array<TextureTransform, GLTF_TEXTURE_INFO_COUNT> mTextureTransform;

    //setters for various members (will clamp to acceptable ranges)

    void setBaseColorId(const LLUUID& id);
    void setNormalId(const LLUUID& id);
    void setMetallicRoughnessId(const LLUUID& id);
    void setEmissiveId(const LLUUID& id);

    void setBaseColorFactor(const LLColor3& baseColor, F32 transparency);
    void setAlphaCutoff(F32 cutoff);
    void setEmissiveColorFactor(const LLColor3& emissiveColor);
    void setMetallicFactor(F32 metallic);
    void setRoughnessFactor(F32 roughness);
    void setAlphaMode(S32 mode);
    void setDoubleSided(bool double_sided);

    void setTextureOffset(TextureInfo texture_info, const LLVector2& offset);
    void setTextureScale(TextureInfo texture_info, const LLVector2& scale);
    void setTextureRotation(TextureInfo texture_info, float rotation);

    // Default value accessors
    static LLUUID getDefaultBaseColorId();
    static LLUUID getDefaultNormalId();
    static LLUUID getDefaultEmissiveId();
    static LLUUID getDefaultMetallicRoughnessId();
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

    // set mAlphaMode from string.
    // Anything otherthan "MASK" or "BLEND" sets mAlphaMode to ALPHA_MODE_OPAQUE
    void setAlphaMode(const std::string& mode)
    {
        if (mode == "MASK")
        {
            mAlphaMode = ALPHA_MODE_MASK;
        }
        else if (mode == "BLEND")
        {
            mAlphaMode = ALPHA_MODE_BLEND;
        }
        else
        {
            mAlphaMode = ALPHA_MODE_OPAQUE;
        }
    }

    const char* getAlphaMode() const
    {
        switch (mAlphaMode)
        {
        case ALPHA_MODE_MASK: return "MASK";
        case ALPHA_MODE_BLEND: return "BLEND";
        default: return "OPAQUE";
        }
    }

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

    // calculate the fields in this material that differ from a base material and write them out to a given tinygltf::Model
    void writeOverridesToModel(tinygltf::Model& model, S32 mat_index, LLGLTFMaterial const* base_material) const;

    void applyOverride(const LLGLTFMaterial& override_mat);

private:

    template<typename T>
    void setFromTexture(const tinygltf::Model& model, const T& texture_info, TextureInfo texture_info_id, LLUUID& texture_id_out);

    template<typename T>
    void writeToTexture(tinygltf::Model& model, T& texture_info, TextureInfo texture_info_id, const LLUUID& texture_id, bool is_override, const LLUUID& base_texture_id) const;
};

