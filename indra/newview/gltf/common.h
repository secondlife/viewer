#pragma once

/**
 * @file common.h
 * @brief LL GLTF Implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include <boost/json.hpp>

// Common types and constants used in the GLTF implementation
namespace LL
{
    namespace GLTF
    {
        constexpr S32 INVALID_INDEX = -1;

        using Value = boost::json::value;

        using mat4 = glm::mat4;
        using vec4 = glm::vec4;
        using vec3 = glm::vec3;
        using vec2 = glm::vec2;
        using quat = glm::quat;

        constexpr S32 LINEAR = 9729;
        constexpr S32 NEAREST = 9728;
        constexpr S32 NEAREST_MIPMAP_NEAREST = 9984;
        constexpr S32 LINEAR_MIPMAP_NEAREST = 9985;
        constexpr S32 NEAREST_MIPMAP_LINEAR = 9986;
        constexpr S32 LINEAR_MIPMAP_LINEAR = 9987;
        constexpr S32 CLAMP_TO_EDGE = 33071;
        constexpr S32 MIRRORED_REPEAT = 33648;
        constexpr S32 REPEAT = 10497;


        class Asset;
        class Material;
        class TextureInfo;
        class NormalTextureInfo;
        class OcclusionTextureInfo;
        class Mesh;
        class Node;
        class Scene;
        class Texture;
        class Sampler;
        class Image;
        class Animation;
        class Skin;
        class Camera;
        class Light;
        class Primitive;
        class Accessor;
        class BufferView;
        class Buffer;

        enum class TextureType : U8
        {
            BASE_COLOR = 0,
            NORMAL,
            METALLIC_ROUGHNESS,
            OCCLUSION,
            EMISSIVE
        };

        constexpr U32 TEXTURE_TYPE_COUNT = 5;
    }
}

