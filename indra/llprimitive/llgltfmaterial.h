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

#include <array>
#include <string>
#include <map>

namespace tinygltf
{
    class Model;
    struct TextureInfo;
    class Value;
    struct Material;
}

class LLTextureEntry;

class LLGLTFMaterial : public LLRefCount
{
public:

    // default material for reference
    static const LLGLTFMaterial sDefault;

    static const char* const ASSET_VERSION;
    static const char* const ASSET_TYPE;
    // Max allowed size of a GLTF material asset or override, when serialized
    // as a minified JSON string
    static constexpr size_t MAX_ASSET_LENGTH = 2048;
    static const std::array<std::string, 2> ACCEPTED_ASSET_VERSIONS;
    static bool isAcceptedVersion(const std::string& version) { return std::find(ACCEPTED_ASSET_VERSIONS.cbegin(), ACCEPTED_ASSET_VERSIONS.cend(), version) != ACCEPTED_ASSET_VERSIONS.cend(); }

    struct TextureTransform
    {
        LLVector2 mOffset = { 0.f, 0.f };
        LLVector2 mScale = { 1.f, 1.f };
        F32 mRotation = 0.f;

        void getPacked(F32 (&packed)[8]) const;

        bool operator==(const TextureTransform& other) const;
        bool operator!=(const TextureTransform& other) const { return !(*this == other); }
    };

    enum AlphaMode
    {
        ALPHA_MODE_OPAQUE = 0,
        ALPHA_MODE_BLEND,
        ALPHA_MODE_MASK
    };

    LLGLTFMaterial();
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
        GLTF_TEXTURE_INFO_TRANSMISSION_TEXTURE,
        // Geenz: The GLTF spec for KHR_materials_transmission states that the red channel of the transmission texture defines the
        // transmission factor, but the spec does not define what the green and blue channels are for. We are using the green channel to
        // define the thickness of the object, as the spec defines the green channel of the volume extension as thickness in KHR_materials_volume.
        GLTF_TEXTURE_INFO_THICKNESS_TEXTURE = GLTF_TEXTURE_INFO_TRANSMISSION_TEXTURE,

        GLTF_TEXTURE_INFO_COUNT
    };

    static const char* const GLTF_FILE_EXTENSION_TRANSFORM;
    static const char* const GLTF_FILE_EXTENSION_TRANSFORM_SCALE;
    static const char* const GLTF_FILE_EXTENSION_TRANSFORM_OFFSET;
    static const char* const GLTF_FILE_EXTENSION_TRANSFORM_ROTATION;
    static const LLUUID GLTF_OVERRIDE_NULL_UUID;
    
    static const char *const GLTF_FILE_EXTENSION_EMISSIVE_STRENGTH;
    static const char *const GLTF_FILE_EXTENSION_EMISSIVE_STRENGTH_EMISSIVE_STRENGTH;

    static const char *const GLTF_FILE_EXTENSION_TRANSMISSION;
    static const char *const GLTF_FILE_EXTENSION_TRANSMISSION_TRANSMISSION_FACTOR;
    static const char *const GLTF_FILE_EXTENSION_TRANSMISSION_TEXTURE;
    
    static const char *const GLTF_FILE_EXTENSION_IOR;
    static const char *const GLTF_FILE_EXTENSION_IOR_IOR;
    
    static const char *const GLTF_FILE_EXTENSION_VOLUME;
    static const char *const GLTF_FILE_EXTENSION_VOLUME_THICKNESS_FACTOR;
    static const char *const GLTF_FILE_EXTENSION_VOLUME_THICKNESS_TEXTURE;
    static const char *const GLTF_FILE_EXTENSION_VOLUME_ATTENUATION_DISTANCE;
    static const char *const GLTF_FILE_EXTENSION_VOLUME_ATTENUATION_COLOR;

    static const char *const GLTF_FILE_EXTENSION_DISPERSION;
    static const char *const GLTF_FILE_EXTENSION_DISPERSION_DISPERSION;

    // *TODO: If/when we implement additional GLTF extensions, they may not be
    // compatible with our GLTF terrain implementation. We may want to disallow
    // materials with some features from being set on terrain, if their
    // implementation on terrain is not compliant with the spec:
    //     - KHR_materials_transmission: Probably OK?
    //     - KHR_materials_ior: Probably OK?
    //     - KHR_materials_volume: Likely incompatible, as our terrain
    //       heightmaps cannot currently be described as finite enclosed
    //       volumes.
    // See also LLPanelRegionTerrainInfo::validateMaterials

    bool mDoubleSided = false;
    typedef std::map<LLUUID, LLUUID> local_tex_map_t;
    local_tex_map_t mTrackingIdToLocalTexture;

public:

    // get a UUID based on a hash of this LLGLTFMaterial
    LLUUID getHash() const;

    //setters for various members (will clamp to acceptable ranges)
    // for_override - set to true if this value is being set as part of an override (important for handling override to default value)x1

    void setTextureId(TextureInfo texture_info, const LLUUID& id, bool for_override = false);

    void setBaseColorId(const LLUUID& id, bool for_override = false);
    void setNormalId(const LLUUID& id, bool for_override = false);
    void setOcclusionRoughnessMetallicId(const LLUUID& id, bool for_override = false);
    void setEmissiveId(const LLUUID &id, bool for_override = false);
    void setTransmissionId(const LLUUID &id, bool for_override = false);

    void setBaseColorFactor(const LLColor4& baseColor, bool for_override = false);
    void setAlphaCutoff(F32 cutoff, bool for_override = false);
    void setEmissiveColorFactor(const LLColor3& emissiveColor, bool for_override = false);
    void setEmissiveStrength(F32 emissiveStrength, bool for_override = false);
    void setMetallicFactor(F32 metallic, bool for_override = false);
    void setRoughnessFactor(F32 roughness, bool for_override = false);
    void setTransmissionFactor(F32 transmission, bool for_override = false);
    void setIOR(F32 ior, bool for_override = false);
    void setThicknessFactor(F32 thickness, bool for_override = false);
    void setAttenuationDistance(F32 attenuationDistance, bool for_override = false);
    void setAttenuationColor(const LLColor3& attenuationColor, bool for_override = false);
    void setDispersion(F32 dispersion, bool for_override = false);
    void setAlphaMode(S32 mode, bool for_override = false);
    void setDoubleSided(bool double_sided, bool for_override = false);

    // *NOTE: texture offsets only exist in overrides, so "for_override" is not needed

    void setTextureOffset(TextureInfo texture_info, const LLVector2& offset);
    void setTextureScale(TextureInfo texture_info, const LLVector2& scale);
    void setTextureRotation(TextureInfo texture_info, float rotation);

    // Default value accessors
    static F32 getDefaultAlphaCutoff();
    static S32 getDefaultAlphaMode();
    static F32 getDefaultMetallicFactor();
    static F32 getDefaultRoughnessFactor();
    static F32 getDefaultTransmissionFactor();
    static F32 getDefaultIOR();
    static F32 getDefaultThicknessFactor();
    static F32 getDefaultAttenuationDistance();
    static LLColor3 getDefaultAttenuationColor();
    static F32 getDefaultDispersion();
    static LLColor4 getDefaultBaseColor();
    static LLColor3 getDefaultEmissiveColor();
    static F32 getDefaultEmissiveStrength();
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
    // if unsuccessful, the contents of this LLGLTFMaterial should be left unchanged and false is returned
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

    virtual void applyOverride(const LLGLTFMaterial& override_mat);
    
    // apply the given LLSD override data
    void applyOverrideLLSD(const LLSD& data);

    // Get the given override on this LLGLTFMaterial as LLSD
    // override_mat -- the override source data
    // data -- output LLSD object (should be passed in empty)
    void getOverrideLLSD(const LLGLTFMaterial& override_mat, LLSD& data);

    // For base materials only (i.e. assets). Clears transforms to
    // default since they're not supported in assets yet.
    void sanitizeAssetMaterial();

    // For material overrides only. Clears most properties to
    // default/fallthrough, but preserves the transforms.
    bool setBaseMaterial();
    void setBaseMaterial(const LLGLTFMaterial& old_override_mat);
    // True if setBaseMaterial() was just called
    bool isClearedForBaseMaterial() const;

    // For local materials, they have to keep track of where
    // they are assigned to for full updates
    virtual void addTextureEntry(LLTextureEntry* te) {};
    virtual void removeTextureEntry(LLTextureEntry* te) {};

    // For local textures so that editor will know to track changes
    void addLocalTextureTracking(const LLUUID& tracking_id, const LLUUID &tex_id);
    void removeLocalTextureTracking(const LLUUID& tracking_id);
    bool hasLocalTextures() { return !mTrackingIdToLocalTexture.empty(); }
    virtual bool replaceLocalTexture(const LLUUID& tracking_id, const LLUUID &old_id, const LLUUID& new_id);
    virtual void updateTextureTracking();

    static void writeFloatToMaterialWithExtension(tinygltf::Material &material, const char *extension, const char *key, F32 value);
    static void setFloatFromModelWithExtension(const tinygltf::Material &model, const char *extension, const char *key, F32 &value, F32 defaultValue);

    static void writeColor3ToMaterialWithExtension(tinygltf::Material &material, const char *extension, const char *key, const LLColor3 &color);
    static void setColor3FromMaterialWithExtension(const tinygltf::Material &model, const char *extension, const char *key, LLColor3 &color,
                                                   LLColor3 defaultColor);

    static void writeTextureToMaterialWithExtension(tinygltf::Material &material, const char *extension, const char *key,
                                                    const tinygltf::TextureInfo &texture_info);
    static void setTextureFromMaterialWithExtension(const tinygltf::Material &model, const char *extension, const char *key,
                                                    tinygltf::TextureInfo &texture_info);

protected:
    static LLVector2 vec2FromJson(const std::map<std::string, tinygltf::Value>& object, const char* key, const LLVector2& default_value);
    static LLColor3 color3FromJson(const std::map<std::string, tinygltf::Value>& object, const char* key, const LLColor3& default_value);
    static F32 floatFromJson(const std::map<std::string, tinygltf::Value>& object, const char* key, const F32 default_value);

    template<typename T>
    static void allocateTextureImage(tinygltf::Model& model, T& texture_info, const std::string& uri);

    template<typename T>
    void setFromTexture(const tinygltf::Model& model, const T& texture_info, TextureInfo texture_info_id);
    template<typename T>
    static void setFromTexture(const tinygltf::Model &model, const T &texture_info, LLUUID &texture_id, TextureTransform &transform);

    template<typename T>
    void writeToTexture(tinygltf::Model& model, T& texture_info, TextureInfo texture_info_id, bool force_write = false) const;
    template<typename T>
    static void writeToTexture(tinygltf::Model& model, T& texture_info, const LLUUID& texture_id, const TextureTransform& transform, bool force_write = false);

    // Used to update the digest of the mTrackingIdToLocalTexture map each time
    // it is changed; this way, that digest can be used by the fast getHash()
    // method intsead of having to hash all individual keys and values. HB
    void updateLocalTexDataDigest();

public:
    // These fields are local to viewer and are a part of local bitmap support
    // IMPORTANT: do not move this member down (and do not move
    // mLocalTexDataDigest either): the getHash() method does rely on the
    // current ordering. HB

    // Used to store a digest of mTrackingIdToLocalTexture when the latter is
    // not empty, or zero otherwise. HB
    U64 mLocalTexDataDigest;

    std::array<LLUUID, GLTF_TEXTURE_INFO_COUNT> mTextureId;
    std::array<TextureTransform, GLTF_TEXTURE_INFO_COUNT> mTextureTransform;

    // NOTE: initialize values to defaults according to the GLTF spec
    // NOTE: these values should be in linear color space
    LLColor4 mBaseColor;
    LLColor3 mEmissiveColor;
    F32      mEmissiveStrength = 1.0f;

    F32 mMetallicFactor;
    F32 mRoughnessFactor;
    F32 mAlphaCutoff;

    F32 mTransmissionFactor = 0.0f;
    F32 mIOR                = 1.5f;
    F32 mThicknessFactor    = 0.0f;
    F32 mAttenuationDistance = INFINITY;
    LLColor3 mAttenuationColor    = LLColor3::white;
    F32      mDispersion          = 0.0f;

    AlphaMode mAlphaMode;

    // Override specific flags for state that can't use off-by-epsilon or UUID
    // hack
    bool mOverrideDoubleSided = false;
    bool mOverrideAlphaMode = false;
};
