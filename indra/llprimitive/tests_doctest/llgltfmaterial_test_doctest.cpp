/**
 * @file llgltfmaterial_test.cpp
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"

#include <set>

#include "../llgltfmaterial.h"
#include "lluuid.cpp"

// Import & define single-header gltf import/export lib
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14

// tinygltf by default loads image files using STB
#define STB_IMAGE_IMPLEMENTATION

// tinygltf saves image files using STB
#define STB_IMAGE_WRITE_IMPLEMENTATION

// Disable reading external images to prevent warnings and speed up the tests.
// We don't need this for the tests, but still need the filesystem
// implementation to be defined in order for llprimitive to link correctly.
#define TINYGLTF_NO_EXTERNAL_IMAGE 1

#include "tinygltf/tiny_gltf.h"

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::ensure_equals;
    using tut_compat::ensure_not_equals;

    struct llgltfmaterial
    {
    };

    // A positive 32-bit float with a long string representation
    constexpr F32 test_fraction = 1.09045365e-32;
    // A larger positive 32-bit float for values that get zeroed if below a threshold
    constexpr F32 test_fraction_big = 0.109045;

    void apply_test_material_texture_ids(LLGLTFMaterial& material)
    {
        material.setBaseColorId(LLUUID::generateNewID());
        material.setNormalId(LLUUID::generateNewID());
        material.setOcclusionRoughnessMetallicId(LLUUID::generateNewID());
        material.setEmissiveId(LLUUID::generateNewID());
    }

    void apply_test_material_texture_transforms(LLGLTFMaterial& material)
    {
        LLGLTFMaterial::TextureTransform test_transform;
        test_transform.mOffset.mV[VX] = test_fraction;
        test_transform.mOffset.mV[VY] = test_fraction;
        test_transform.mScale.mV[VX] = test_fraction;
        test_transform.mScale.mV[VY] = test_fraction;
        test_transform.mRotation = test_fraction;
        for (LLGLTFMaterial::TextureInfo i = LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR;
             i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT;
             i = LLGLTFMaterial::TextureInfo((U32)i + 1))
        {
            material.setTextureOffset(i, test_transform.mOffset);
            material.setTextureScale(i, test_transform.mScale);
            material.setTextureRotation(i, test_transform.mRotation);
        }
    }

    void apply_test_material_factors(LLGLTFMaterial& material)
    {
        material.setBaseColorFactor(LLColor4(test_fraction_big, test_fraction_big, test_fraction_big, test_fraction_big));
        material.setEmissiveColorFactor(LLColor3(test_fraction_big, test_fraction_big, test_fraction_big));
        material.setMetallicFactor(test_fraction);
        material.setRoughnessFactor(test_fraction);
    }

    LLGLTFMaterial create_test_material()
    {
        LLGLTFMaterial material;

        apply_test_material_texture_ids(material);
        apply_test_material_texture_transforms(material);
        apply_test_material_factors(material);

        material.setAlphaCutoff(test_fraction);
        material.setAlphaMode(LLGLTFMaterial::ALPHA_MODE_OPAQUE, true);
        material.setDoubleSided(false, true);

        return material;
    }

    void ensure_gltf_material_serialize(const std::string& ensure_suffix, const LLGLTFMaterial& material_in)
    {
        const std::string json_in = material_in.asJSON();
        LLGLTFMaterial material_out;
        std::string warn_msg;
        std::string error_msg;
        bool serialize_success = material_out.fromJSON(json_in, warn_msg, error_msg);
        ensure_equals("LLGLTFMaterial serialization has no warnings: " + ensure_suffix, "", warn_msg);
        ensure_equals("LLGLTFMaterial serialization has no errors: " + ensure_suffix, "", error_msg);
        ensure("LLGLTFMaterial serializes successfully: " + ensure_suffix, serialize_success);
        ensure("LLGLTFMaterial is preserved when deserialized: " + ensure_suffix, material_in == material_out);
        const std::string json_out = material_out.asJSON();
        ensure_equals("LLGLTFMaterial is preserved when serialized: " + ensure_suffix, json_in, json_out);
    }

    void ensure_gltf_material_trimmed(const std::string& material_json, const std::string& must_not_contain)
    {
        ensure("LLGLTFMaterial serialization trims property '" + must_not_contain + "'",
               material_json.find(must_not_contain) == std::string::npos);
    }

    template<typename T>
    void ensure_material_hash_pre(LLGLTFMaterial& material, T& material_field, const T new_value, const std::string& field_name)
    {
        ensure("LLGLTFMaterial: Hash: Test field " + field_name + " is part of the test material object", (
                    size_t(&material_field) >= size_t(&material) &&
                    (size_t(&material_field) + sizeof(material_field)) <= (size_t(&material) + sizeof(material))));
        ensure("LLGLTFMaterial: Hash: " + field_name + " differs and will cause a perturbation worth hashing",
               material_field != new_value);
    }

    template<typename T>
    void ensure_material_hash_not_changed(LLGLTFMaterial& material, T& material_field, const T new_value, const std::string& field_name)
    {
        ensure_material_hash_pre(material, material_field, new_value, field_name);

        const LLGLTFMaterial old_material = material;
        material_field = new_value;
        ensure_equals(("LLGLTFMaterial: Hash: Perturbing " + field_name + " to new value does NOT change the hash").c_str(),
                      material.getHash(), old_material.getHash());
    }

    template<typename T>
    void ensure_material_hash_changed(LLGLTFMaterial& material, T& material_field, const T new_value, const std::string& field_name)
    {
        ensure_material_hash_pre(material, material_field, new_value, field_name);

        const LLGLTFMaterial old_material = material;
        material_field = new_value;
        ensure_not_equals(("LLGLTFMaterial: Hash: Perturbing " + field_name + " to new value changes the hash").c_str(),
                          material.getHash(), old_material.getHash());
    }
} // namespace tut

#define ENSURE_HASH_NOT_CHANGED(HASH_MAT, SOURCE_MAT, FIELD) ensure_material_hash_not_changed(HASH_MAT, HASH_MAT.FIELD, SOURCE_MAT.FIELD, #FIELD)
#define ENSURE_HASH_CHANGED(HASH_MAT, SOURCE_MAT, FIELD) ensure_material_hash_changed(HASH_MAT, HASH_MAT.FIELD, SOURCE_MAT.FIELD, #FIELD)

TUT_SUITE("llgltfmaterial")
{
    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_1")
    {
        using namespace tut;
#if ADDRESS_SIZE != 32
#if LL_WINDOWS
        ensure_equals("fields supported for GLTF (sizeof check)", sizeof(LLGLTFMaterial), 232);
#endif
#endif
        ensure_equals("LLGLTFMaterial texture info count", (U32)LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT, 4);
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_2")
    {
        using namespace tut;
        ensure_equals("LLGLTFMaterial occlusion does not differ from metallic roughness",
                      LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS,
                      LLGLTFMaterial::GLTF_TEXTURE_INFO_OCCLUSION);
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_3")
    {
        using namespace tut;
        const bool doubleSideds[] { false, true };
        const LLGLTFMaterial::AlphaMode alphaModes[] { LLGLTFMaterial::ALPHA_MODE_OPAQUE, LLGLTFMaterial::ALPHA_MODE_BLEND, LLGLTFMaterial::ALPHA_MODE_MASK };
        const bool forOverrides[] { false, true };

        for (bool doubleSided : doubleSideds)
        {
            for (bool forOverride : forOverrides)
            {
                LLGLTFMaterial material;
                material.setDoubleSided(doubleSided, forOverride);
                const bool overrideBit = (doubleSided == false) && forOverride;
                ensure_equals("LLGLTFMaterial: double sided = " + std::to_string(doubleSided) +
                              " override bit when forOverride = " + std::to_string(forOverride),
                              material.mOverrideDoubleSided, overrideBit);
                ensure_gltf_material_serialize("double sided = " + std::to_string(doubleSided), material);
            }
        }

        for (LLGLTFMaterial::AlphaMode alphaMode : alphaModes)
        {
            for (bool forOverride : forOverrides)
            {
                LLGLTFMaterial material;
                material.setAlphaMode(alphaMode, forOverride);
                const bool overrideBit = (alphaMode == LLGLTFMaterial::ALPHA_MODE_OPAQUE) && forOverride;
                ensure_equals("LLGLTFMaterial: alpha mode = " + std::to_string(alphaMode) +
                              " override bit when forOverride = " + std::to_string(forOverride),
                              material.mOverrideAlphaMode, overrideBit);
                ensure_gltf_material_serialize("alpha mode = " + std::to_string(alphaMode), material);
            }
        }
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_4")
    {
        using namespace tut;
        LLGLTFMaterial material;
        LLGLTFMaterial::TextureTransform& transform = material.mTextureTransform[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR];
        transform.mOffset[VX] = 1.f;
        transform.mOffset[VY] = 2.f;
        transform.mScale[VX] = 0.05f;
        transform.mScale[VY] = 100.f;
        transform.mRotation = 1.571f;
        ensure_gltf_material_serialize("material with transform", material);
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_5")
    {
        using namespace tut;
        {
            const LLGLTFMaterial material;
            const std::string material_json = material.asJSON();
            ensure_gltf_material_trimmed(material_json, "pbrMetallicRoughness");
            ensure_gltf_material_trimmed(material_json, "normalTexture");
            ensure_gltf_material_trimmed(material_json, "emissiveTexture");
            ensure_gltf_material_trimmed(material_json, "occlusionTexture");
        }

        {
            LLGLTFMaterial metallic_factor_material;
            metallic_factor_material.setMetallicFactor(0.5);
            const std::string metallic_factor_material_json = metallic_factor_material.asJSON();
            ensure_gltf_material_trimmed(metallic_factor_material_json, "baseColorTexture");
            ensure_gltf_material_trimmed(metallic_factor_material_json, "metallicRoughnessTexture");
        }
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_6")
    {
        using namespace tut;
        {
            const LLGLTFMaterial full_material = create_test_material();
            ensure_gltf_material_serialize("full material", full_material);
        }

        {
            LLGLTFMaterial texture_ids_only_material;
            apply_test_material_texture_ids(texture_ids_only_material);
            ensure_gltf_material_serialize("material with texture IDs only", texture_ids_only_material);
        }

        {
            LLGLTFMaterial texture_transforms_only_material;
            apply_test_material_texture_ids(texture_transforms_only_material);
            ensure_gltf_material_serialize("material with texture transforms only", texture_transforms_only_material);
        }

        {
            LLGLTFMaterial factors_only_material;
            apply_test_material_factors(factors_only_material);
            ensure_gltf_material_serialize("material with scaling/tint factors only", factors_only_material);
        }
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_7")
    {
        using namespace tut;
        const LLGLTFMaterial material_asset = create_test_material();
        LLGLTFMaterial render_material = material_asset;
        render_material.applyOverride(LLGLTFMaterial::sDefault);
        ensure("LLGLTFMaterial: sDefault is a no-op override", material_asset == render_material);
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_8")
    {
        using namespace tut;
        LLGLTFMaterial override_material;
        apply_test_material_texture_transforms(override_material);
        LLGLTFMaterial render_material;
        render_material.applyOverride(override_material);
        ensure("LLGLTFMaterial: transform overrides", render_material == override_material);
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_9")
    {
        using namespace tut;
        {
            LLGLTFMaterial override_material;
            override_material.setAlphaMode(LLGLTFMaterial::ALPHA_MODE_BLEND, true);
            override_material.setDoubleSided(true, true);

            LLGLTFMaterial render_material;
            render_material.applyOverride(override_material);
            ensure("LLGLTFMaterial: extra overrides with non-default values applied over default", render_material == override_material);
        }
        {
            LLGLTFMaterial override_material;
            override_material.setAlphaMode(LLGLTFMaterial::ALPHA_MODE_OPAQUE, true);
            override_material.setDoubleSided(false, true);

            LLGLTFMaterial render_material;
            override_material.setAlphaMode(LLGLTFMaterial::ALPHA_MODE_BLEND, false);
            override_material.setDoubleSided(true, false);

            render_material.applyOverride(override_material);
            override_material.mOverrideDoubleSided = false;
            override_material.mOverrideAlphaMode = false;

            ensure("LLGLTFMaterial: extra overrides with default values applied over non-default", render_material == override_material);
        }
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_10")
    {
        using namespace tut;
        const U32 texture_count = 2;
        const LLUUID override_textures[texture_count] = { LLUUID::null, LLUUID::generateNewID() };
        const LLUUID asset_textures[texture_count] = { LLUUID::generateNewID(), LLUUID::null };
        for (U32 i = 0; i < texture_count; ++i)
        {
            LLGLTFMaterial override_material;
            const LLUUID& override_texture = override_textures[i];
            for (LLGLTFMaterial::TextureInfo j = LLGLTFMaterial::TextureInfo(0);
                 j < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT;
                 j = LLGLTFMaterial::TextureInfo(U32(j) + 1))
            {
                override_material.setTextureId(j, override_texture, true);
            }

            LLGLTFMaterial render_material;
            const LLUUID& asset_texture = asset_textures[i];
            for (LLGLTFMaterial::TextureInfo j = LLGLTFMaterial::TextureInfo(0);
                 j < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT;
                 j = LLGLTFMaterial::TextureInfo(U32(j) + 1))
            {
                render_material.setTextureId(j, asset_texture, false);
            }

            render_material.applyOverride(override_material);

            for (LLGLTFMaterial::TextureInfo j = LLGLTFMaterial::TextureInfo(0);
                 j < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT;
                 j = LLGLTFMaterial::TextureInfo(U32(j) + 1))
            {
                const LLUUID& render_texture = render_material.mTextureId[j];
                ensure_equals("LLGLTFMaterial: Override texture ID " + override_texture.asString() +
                              " replaces underlying texture ID " + asset_texture.asString(),
                              render_texture, override_texture);
            }
        }
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_11")
    {
        using namespace tut;
        const S32 non_default_alpha_modes[] = { LLGLTFMaterial::ALPHA_MODE_BLEND, LLGLTFMaterial::ALPHA_MODE_MASK };
        for (S32 non_default_alpha_mode : non_default_alpha_modes)
        {
            LLGLTFMaterial material;
            material.setAlphaMode(LLGLTFMaterial::ALPHA_MODE_OPAQUE, true);
            ensure_equals("LLGLTFMaterial: alpha mode override flag set", material.mOverrideAlphaMode, true);
            material.setAlphaMode(non_default_alpha_mode, true);
            ensure_equals("LLGLTFMaterial: alpha mode override flag unset", material.mOverrideAlphaMode, false);
        }

        {
            LLGLTFMaterial material;
            material.setDoubleSided(false, true);
            ensure_equals("LLGLTFMaterial: double sided override flag set", material.mOverrideDoubleSided, true);
            material.setDoubleSided(true, true);
            ensure_equals("LLGLTFMaterial: double sided override flag unset", material.mOverrideDoubleSided, false);
        }
    }

    TUT_CASE("llgltfmaterial::llgltfmaterial_object_t_test_12")
    {
        using namespace tut;
        LLGLTFMaterial source_mat = create_test_material();
        source_mat.mTrackingIdToLocalTexture[LLUUID::generateNewID()] = LLUUID::generateNewID();
        source_mat.mLocalTexDataDigest = 1;
        source_mat.mAlphaMode = LLGLTFMaterial::ALPHA_MODE_MASK;
        source_mat.mDoubleSided = true;

        LLGLTFMaterial hash_mat;

        ENSURE_HASH_NOT_CHANGED(hash_mat, source_mat, mTrackingIdToLocalTexture);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mLocalTexDataDigest);

        ENSURE_HASH_CHANGED(hash_mat, source_mat, mTextureId);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mTextureTransform);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mBaseColor);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mEmissiveColor);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mMetallicFactor);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mRoughnessFactor);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mAlphaCutoff);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mAlphaMode);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mDoubleSided);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mOverrideDoubleSided);
        ENSURE_HASH_CHANGED(hash_mat, source_mat, mOverrideAlphaMode);
    }
}

#undef ENSURE_HASH_NOT_CHANGED
#undef ENSURE_HASH_CHANGED
