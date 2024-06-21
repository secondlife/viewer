/**
 * @file llgltfmaterial_templates.h
 * @brief Material template definition
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

#pragma once

#include "llgltfmaterial.h"

// Use templates here as workaround for the different similar texture info classes in tinygltf
// Includer must first include tiny_gltf.h with the desired flags

template<typename T>
std::string gltf_get_texture_image(const tinygltf::Model& model, const T& texture_info)
{
    const S32 texture_idx = texture_info.index;
    if (texture_idx < 0 || texture_idx >= model.textures.size())
    {
        return "";
    }
    const tinygltf::Texture& texture = model.textures[texture_idx];

    // Ignore texture.sampler for now

    const S32 image_idx = texture.source;
    if (image_idx < 0 || image_idx >= model.images.size())
    {
        return "";
    }
    const tinygltf::Image& image = model.images[image_idx];

    return image.uri;
}

template<typename T>
void LLGLTFMaterial::setFromTexture(const tinygltf::Model& model, const T& texture_info, TextureInfo texture_info_id)
{
    setFromTexture(model, texture_info, mTextureId[texture_info_id], mTextureTransform[texture_info_id]);
    const std::string uri = gltf_get_texture_image(model, texture_info);
}

// static
template<typename T>
void LLGLTFMaterial::setFromTexture(const tinygltf::Model& model, const T& texture_info, LLUUID& texture_id, TextureTransform& transform)
{
    LL_PROFILE_ZONE_SCOPED;
    const std::string uri = gltf_get_texture_image(model, texture_info);
    texture_id.set(uri);

    const tinygltf::Value::Object& extensions_object = texture_info.extensions;
    const auto transform_it = extensions_object.find(GLTF_FILE_EXTENSION_TRANSFORM);
    if (transform_it != extensions_object.end())
    {
        const tinygltf::Value& transform_json = std::get<1>(*transform_it);
        if (transform_json.IsObject())
        {
            const tinygltf::Value::Object& transform_object = transform_json.Get<tinygltf::Value::Object>();
            transform.mOffset = vec2FromJson(transform_object, GLTF_FILE_EXTENSION_TRANSFORM_OFFSET, getDefaultTextureOffset());
            transform.mScale = vec2FromJson(transform_object, GLTF_FILE_EXTENSION_TRANSFORM_SCALE, getDefaultTextureScale());
            transform.mRotation = floatFromJson(transform_object, GLTF_FILE_EXTENSION_TRANSFORM_ROTATION, getDefaultTextureRotation());
        }
    }
}

// static
template<typename T>
void LLGLTFMaterial::allocateTextureImage(tinygltf::Model& model, T& texture_info, const std::string& uri)
{
    const S32 image_idx = static_cast<S32>(model.images.size());
    model.images.emplace_back();
    model.images[image_idx].uri = uri;

    // The texture, not to be confused with the texture info
    const S32 texture_idx = static_cast<S32>(model.textures.size());
    model.textures.emplace_back();
    tinygltf::Texture& texture = model.textures[texture_idx];
    texture.source = image_idx;

    texture_info.index = texture_idx;
}

// static
template<typename T>
void LLGLTFMaterial::writeToTexture(tinygltf::Model& model, T& texture_info, TextureInfo texture_info_id, bool force_write) const
{
    writeToTexture(model, texture_info, mTextureId[texture_info_id], mTextureTransform[texture_info_id], force_write);
}

// static
template<typename T>
void LLGLTFMaterial::writeToTexture(tinygltf::Model& model, T& texture_info, const LLUUID& texture_id, const TextureTransform& transform, bool force_write)
{
    LL_PROFILE_ZONE_SCOPED;
    const bool is_blank_transform = transform == sDefault.mTextureTransform[0];
    // Check if this material matches all the fallback values, and if so, then
    // skip including it to reduce material size
    if (!force_write && texture_id.isNull() && is_blank_transform)
    {
        return;
    }

    // tinygltf will discard this texture info if there is no valid texture,
    // causing potential loss of information for overrides, so ensure one is
    // defined. -Cosmic,2023-01-30
    allocateTextureImage(model, texture_info, texture_id.asString());

    if (!is_blank_transform)
    {
        tinygltf::Value::Object transform_map;
        transform_map[GLTF_FILE_EXTENSION_TRANSFORM_OFFSET] = tinygltf::Value(tinygltf::Value::Array({
            tinygltf::Value(transform.mOffset.mV[VX]),
            tinygltf::Value(transform.mOffset.mV[VY])
        }));
        transform_map[GLTF_FILE_EXTENSION_TRANSFORM_SCALE] = tinygltf::Value(tinygltf::Value::Array({
            tinygltf::Value(transform.mScale.mV[VX]),
            tinygltf::Value(transform.mScale.mV[VY])
        }));
        transform_map[GLTF_FILE_EXTENSION_TRANSFORM_ROTATION] = tinygltf::Value(transform.mRotation);
        texture_info.extensions[GLTF_FILE_EXTENSION_TRANSFORM] = tinygltf::Value(transform_map);
    }
}
