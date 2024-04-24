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

// LL GLTF Implementation
namespace LL
{
    namespace GLTF
    {
        class Asset;

        constexpr S32 INVALID_INDEX = -1;

        class Buffer
        {
        public:
            std::vector<U8> mData;
            std::string mName;
            std::string mUri;

            const Buffer& operator=(const tinygltf::Buffer& src);
        };

        class BufferView
        {
        public:
            S32 mBuffer = INVALID_INDEX;
            S32 mByteLength;
            S32 mByteOffset;
            S32 mByteStride;
            S32 mTarget;
            S32 mComponentType;

            std::string mName;

            const BufferView& operator=(const tinygltf::BufferView& src);
            
        };
        
        class Accessor
        {
        public:
            S32 mBufferView = INVALID_INDEX;
            S32 mByteOffset;
            S32 mComponentType;
            S32 mCount;
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

            S32 mType;
            bool mNormalized;
            std::string mName;

            const Accessor& operator=(const tinygltf::Accessor& src);
        };
    }
}
