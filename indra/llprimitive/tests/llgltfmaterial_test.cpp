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
#include "../test/lldoctest.h"

#include <set>

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

TEST_SUITE("llgltfmaterial") {

TEST_CASE("test_1")
{

#if ADDRESS_SIZE != 32
#if LL_WINDOWS
        // If any fields are added/changed, these tests should be updated (consider also updating ASSET_VERSION in LLGLTFMaterial)
        // This test result will vary between compilers, so only test a single platform
        ensure_equals("fields supported for GLTF (sizeof check)", sizeof(LLGLTFMaterial), 232);
#endif
#endif
        ensure_equals("LLGLTFMaterial texture info count", (U32)LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT, 4);
    
}

TEST_CASE("test_2")
{

        CHECK_MESSAGE(LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS == LLGLTFMaterial::GLTF_TEXTURE_INFO_OCCLUSION, "LLGLTFMaterial occlusion does not differ from metallic roughness");
    
}

TEST_CASE("test_3")
{

        const bool doubleSideds[] { false, true 
}

TEST_CASE("test_4")
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

TEST_CASE("test_5")
{

        {
            const LLGLTFMaterial material;
            const std::string material_json = material.asJSON();
            ensure_gltf_material_trimmed(material_json, "pbrMetallicRoughness");
            ensure_gltf_material_trimmed(material_json, "normalTexture");
            ensure_gltf_material_trimmed(material_json, "emissiveTexture");
            ensure_gltf_material_trimmed(material_json, "occlusionTexture");
        
}

TEST_CASE("test_6")
{

        {
            const LLGLTFMaterial full_material = create_test_material();
            ensure_gltf_material_serialize("full material", full_material);
        
}

TEST_CASE("test_7")
{

        const LLGLTFMaterial material_asset = create_test_material();
        LLGLTFMaterial render_material = material_asset;
        render_material.applyOverride(LLGLTFMaterial::sDefault);
        CHECK_MESSAGE(material_asset == render_material, "LLGLTFMaterial: sDefault is a no-op override");
    
}

TEST_CASE("test_8")
{

        LLGLTFMaterial override_material;
        apply_test_material_texture_transforms(override_material);
        LLGLTFMaterial render_material;
        render_material.applyOverride(override_material);
        CHECK_MESSAGE(render_material == override_material, "LLGLTFMaterial: transform overrides");
    
}

TEST_CASE("test_9")
{

        {
            LLGLTFMaterial override_material;
            override_material.setAlphaMode(LLGLTFMaterial::ALPHA_MODE_BLEND, true);
            override_material.setDoubleSided(true, true);

            LLGLTFMaterial render_material;

            render_material.applyOverride(override_material);

            CHECK_MESSAGE(render_material == override_material, "LLGLTFMaterial: extra overrides with non-default values applied over default");
        
}

TEST_CASE("test_10")
{

        const U32 texture_count = 2;
        const LLUUID override_textures[texture_count] = { LLUUID::null, LLUUID::generateNewID() 
}

TEST_CASE("test_11")
{

        const S32 non_default_alpha_modes[] = { LLGLTFMaterial::ALPHA_MODE_BLEND, LLGLTFMaterial::ALPHA_MODE_MASK 
}

TEST_CASE("test_12")
{

        // *NOTE: Due to direct manipulation of the fields of materials
        // throughout this test, the resulting modified materials may not be
        // compliant or properly serializable.

        // Ensure all fields of source_mat are set to values that differ from
        // LLGLTFMaterial::sDefault, even if that would result in an invalid
        // material object.
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

} // TEST_SUITE

