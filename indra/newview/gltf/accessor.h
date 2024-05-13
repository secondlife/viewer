#pragma once

/**
 * @file asset.h
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

#include "../lltinygltfhelper.h"
#include "llstrider.h"
#include "boost/json.hpp"

#include "common.h"

// LL GLTF Implementation
namespace LL
{
    namespace GLTF
    {
        constexpr S32 INVALID_INDEX = -1;

        class Buffer
        {
        public:
            std::vector<U8> mData;
            std::string mName;
            std::string mUri;
            S32 mByteLength = 0;

            // erase the given range from this buffer.
            // also updates all buffer views in given asset that reference this buffer
            void erase(Asset& asset, S32 offset, S32 length);

            void serialize(boost::json::object& obj) const;
            const Buffer& operator=(const Value& value);
            const Buffer& operator=(const tinygltf::Buffer& src);
        };

        class BufferView
        {
        public:
            S32 mBuffer = INVALID_INDEX;
            S32 mByteLength = 0;
            S32 mByteOffset = 0;
            S32 mByteStride = 0;
            S32 mTarget = -1;

            std::string mName;

            void serialize(boost::json::object& obj) const;
            const BufferView& operator=(const Value& value);
            const BufferView& operator=(const tinygltf::BufferView& src);
        };
        
        class Accessor
        {
        public:
            S32 mBufferView = INVALID_INDEX;
            S32 mByteOffset = 0;
            S32 mComponentType = 0;
            S32 mCount = 0;
            std::vector<double> mMax;
            std::vector<double> mMin;

            enum class Type : S32
            {
                SCALAR = TINYGLTF_TYPE_SCALAR,
                VEC2 = TINYGLTF_TYPE_VEC2,
                VEC3 = TINYGLTF_TYPE_VEC3,
                VEC4 = TINYGLTF_TYPE_VEC4,
                MAT2 = TINYGLTF_TYPE_MAT2,
                MAT3 = TINYGLTF_TYPE_MAT3,
                MAT4 = TINYGLTF_TYPE_MAT4
            };

            Type mType = Type::SCALAR;
            bool mNormalized = false;
            std::string mName;

            void serialize(boost::json::object& obj) const;
            const Accessor& operator=(const Value& value);
            const Accessor& operator=(const tinygltf::Accessor& src);
        };

        // convert from "SCALAR", "VEC2", etc to Accessor::Type
        Accessor::Type gltf_type_to_enum(const std::string& type);

        // convert from Accessor::Type to "SCALAR", "VEC2", etc
        std::string enum_to_gltf_type(Accessor::Type type);
    }
}
