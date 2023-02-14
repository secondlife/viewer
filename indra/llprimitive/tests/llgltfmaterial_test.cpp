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

#include "linden_common.h"
#include "lltut.h"

#include "../llgltfmaterial.h"
#include "lluuid.cpp"

// Import & define single-header gltf import/export lib
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14  // default is C++ 11

// tinygltf by default loads image files using STB
#define STB_IMAGE_IMPLEMENTATION
// to use our own image loading:
// 1. replace this definition with TINYGLTF_NO_STB_IMAGE
// 2. provide image loader callback with TinyGLTF::SetImageLoader(LoadimageDataFunction LoadImageData, void *user_data)

// tinygltf saves image files using STB
#define STB_IMAGE_WRITE_IMPLEMENTATION
// similarly, can override with TINYGLTF_NO_STB_IMAGE_WRITE and TinyGLTF::SetImageWriter(fxn, data)

// Disable reading external images to prevent warnings and speed up the tests.
// We don't need this for the tests, but still need the filesystem
// implementation to be defined in order for llprimitive to link correctly.
#define TINYGLTF_NO_EXTERNAL_IMAGE 1

#include "tinygltf/tiny_gltf.h"

namespace tut
{
    struct llgltfmaterial
    {
    };
    typedef test_group<llgltfmaterial> llgltfmaterial_t;
    typedef llgltfmaterial_t::object llgltfmaterial_object_t;
    tut::llgltfmaterial_t tut_llgltfmaterial("llgltfmaterial");

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
        for (LLGLTFMaterial::TextureInfo i = LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; i = LLGLTFMaterial::TextureInfo((U32)i + 1))
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
        // Because this is the default value, it should append to the extras field to mark it as an override
        material.setAlphaMode(LLGLTFMaterial::ALPHA_MODE_OPAQUE);
        // Because this is the default value, it should append to the extras field to mark it as an override
        material.setDoubleSided(false);

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
        ensure("LLGLTFMaterial serialization trims property '" + must_not_contain + "'", material_json.find(must_not_contain) == std::string::npos);
    }

    // Test that GLTF material fields have not changed since these tests were written
    template<> template<>
    void llgltfmaterial_object_t::test<1>()
    {
#if ADDRESS_SIZE != 32
#if LL_WINDOWS
        // If any fields are added/changed, these tests should be updated (consider also updating ASSET_VERSION in LLGLTFMaterial)
        // This test result will vary between compilers, so only test a single platform
        ensure_equals("fields supported for GLTF (sizeof check)", sizeof(LLGLTFMaterial), 216);
#endif
#endif
        ensure_equals("LLGLTFMaterial texture info count", (U32)LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT, 4);
    }

    // Test that occlusion and metallicRoughness are the same (They are different for asset validation. See lluploadmaterial.cpp)
    template<> template<>
    void llgltfmaterial_object_t::test<2>()
    {
        ensure_equals("LLGLTFMaterial occlusion does not differ from metallic roughness", LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS, LLGLTFMaterial::GLTF_TEXTURE_INFO_OCCLUSION);
    }

    // Ensure double sided and alpha mode overrides serialize as expected
    template<> template<>
    void llgltfmaterial_object_t::test<3>()
    {
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
                ensure_equals("LLGLTFMaterial: double sided = " + std::to_string(doubleSided) + " override bit when forOverride = " + std::to_string(forOverride), material.mOverrideDoubleSided, overrideBit);
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
                ensure_equals("LLGLTFMaterial: alpha mode = " + std::to_string(alphaMode) + " override bit when forOverride = " + std::to_string(forOverride), material.mOverrideAlphaMode, overrideBit);
                ensure_gltf_material_serialize("alpha mode = " + std::to_string(alphaMode), material);
            }
        }
    }

    // Test that a GLTF material's transform components serialize as expected
    template<> template<>
    void llgltfmaterial_object_t::test<4>()
    {
        LLGLTFMaterial material;
        LLGLTFMaterial::TextureTransform& transform = material.mTextureTransform[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR];
        transform.mOffset[VX] = 1.f;
        transform.mOffset[VY] = 2.f;
        transform.mScale[VX] = 0.05f;
        transform.mScale[VY] = 100.f;
        transform.mRotation = 1.571f;
        ensure_gltf_material_serialize("material with transform", material);
    }

    // Test that a GLTF material avoids serializing a material unnecessarily
    template<> template<>
    void llgltfmaterial_object_t::test<5>()
    {
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

    // Test that a GLTF material preserves values on serialization
    template<> template<>
    void llgltfmaterial_object_t::test<6>()
    {
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

    // Test that sDefault is a no-op override
    template<> template<>
    void llgltfmaterial_object_t::test<7>()
    {
        const LLGLTFMaterial material_asset = create_test_material();
        LLGLTFMaterial render_material = material_asset;
        render_material.applyOverride(LLGLTFMaterial::sDefault);
        ensure("LLGLTFMaterial: sDefault is a no-op override", material_asset == render_material);
    }

    // Test application of transform overrides
    template<> template<>
    void llgltfmaterial_object_t::test<8>()
    {
        LLGLTFMaterial override_material;
        apply_test_material_texture_transforms(override_material);
        LLGLTFMaterial render_material;
        render_material.applyOverride(override_material);
        ensure("LLGLTFMaterial: transform overrides", render_material == override_material);
    }

    // Test application of flag-based overrides
    template<> template<>
    void llgltfmaterial_object_t::test<9>()
    {
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
            // Not interested in these flags for equality comparison
            override_material.mOverrideDoubleSided = false;
            override_material.mOverrideAlphaMode = false;

            ensure("LLGLTFMaterial: extra overrides with default values applied over non-default", render_material == override_material);
        }
    }

    // Test application of texture overrides
    template<> template<>
    void llgltfmaterial_object_t::test<10>()
    {
        const U32 texture_count = 2;
        const LLUUID override_textures[texture_count] = { LLUUID::null, LLUUID::generateNewID() };
        const LLUUID asset_textures[texture_count] = { LLUUID::generateNewID(), LLUUID::null };
        for (U32 i = 0; i < texture_count; ++i)
        {
            LLGLTFMaterial override_material;
            const LLUUID& override_texture = override_textures[i];
            for (LLGLTFMaterial::TextureInfo j = LLGLTFMaterial::TextureInfo(0); j  < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; j = LLGLTFMaterial::TextureInfo(U32(j) + 1))
            {
                override_material.setTextureId(j, override_texture, true);
            }

            LLGLTFMaterial render_material;
            const LLUUID& asset_texture = asset_textures[i];
            for (LLGLTFMaterial::TextureInfo j = LLGLTFMaterial::TextureInfo(0); j  < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; j = LLGLTFMaterial::TextureInfo(U32(j) + 1))
            {
                render_material.setTextureId(j, asset_texture, false);
            }

            render_material.applyOverride(override_material);

            for (LLGLTFMaterial::TextureInfo j = LLGLTFMaterial::TextureInfo(0); j  < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; j = LLGLTFMaterial::TextureInfo(U32(j) + 1))
            {
                const LLUUID& render_texture = render_material.mTextureId[j];
                ensure_equals("LLGLTFMaterial: Override texture ID " + override_texture.asString() + " replaces underlying texture ID " + asset_texture.asString(), render_texture, override_texture);
            }
        }
    }

    // Test non-persistence of default value flags in overrides
    template<> template<>
    void llgltfmaterial_object_t::test<11>()
    {
        const S32 non_default_alpha_modes[] = { LLGLTFMaterial::ALPHA_MODE_BLEND, LLGLTFMaterial::ALPHA_MODE_MASK };
        for (S32 non_default_alpha_mode : non_default_alpha_modes)
        {
            LLGLTFMaterial material;
            // Set default alpha mode
            material.setAlphaMode(LLGLTFMaterial::ALPHA_MODE_OPAQUE, true);
            ensure_equals("LLGLTFMaterial: alpha mode override flag set", material.mOverrideAlphaMode, true);
            // Set non-default alpha mode
            material.setAlphaMode(non_default_alpha_mode, true);
            ensure_equals("LLGLTFMaterial: alpha mode override flag unset", material.mOverrideAlphaMode, false);
        }

        {
            // Set default double sided
            LLGLTFMaterial material;
            material.setDoubleSided(false, true);
            ensure_equals("LLGLTFMaterial: double sided override flag set", material.mOverrideDoubleSided, true);
            // Set non-default double sided
            material.setDoubleSided(true, true);
            ensure_equals("LLGLTFMaterial: double sided override flag unset", material.mOverrideDoubleSided, false);
        }
    }
}
