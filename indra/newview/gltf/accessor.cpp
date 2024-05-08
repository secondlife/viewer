/**
 * @file accessor.cpp
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

#include "../llviewerprecompiledheaders.h"

#include "asset.h"
#include "buffer_util.h"

using namespace LL::GLTF;
using namespace boost::json;

namespace LL
{
    namespace GLTF
    {
        Accessor::Type gltf_type_to_enum(const std::string& type)
        {
            if (type == "SCALAR")
            {
                return Accessor::Type::SCALAR;
            }
            else if (type == "VEC2")
            {
                return Accessor::Type::VEC2;
            }
            else if (type == "VEC3")
            {
                return Accessor::Type::VEC3;
            }
            else if (type == "VEC4")
            {
                return Accessor::Type::VEC4;
            }
            else if (type == "MAT2")
            {
                return Accessor::Type::MAT2;
            }
            else if (type == "MAT3")
            {
                return Accessor::Type::MAT3;
            }
            else if (type == "MAT4")
            {
                return Accessor::Type::MAT4;
            }

            LL_WARNS("GLTF") << "Unknown accessor type: " << type << LL_ENDL;
            llassert(false);

            return Accessor::Type::SCALAR;
        }

        std::string enum_to_gltf_type(Accessor::Type type)
        {
            switch (type)
            {
            case Accessor::Type::SCALAR:
                return "SCALAR";
            case Accessor::Type::VEC2:
                return "VEC2";
            case Accessor::Type::VEC3:
                return "VEC3";
            case Accessor::Type::VEC4:
                return "VEC4";
            case Accessor::Type::MAT2:
                return "MAT2";
            case Accessor::Type::MAT3:
                return "MAT3";
            case Accessor::Type::MAT4:
                return "MAT4";
            }

            LL_WARNS("GLTF") << "Unknown accessor type: " << (S32)type << LL_ENDL;
            llassert(false);

            return "SCALAR";
        }
    }
}

void Buffer::erase(Asset& asset, S32 offset, S32 length)
{
    S32 idx = this - &asset.mBuffers[0];

    mData.erase(mData.begin() + offset, mData.begin() + offset + length);

    for (BufferView& view : asset.mBufferViews)
    {
        if (view.mBuffer == idx)
        {
            if (view.mByteOffset >= offset)
            {
                view.mByteOffset -= length;
            }
        }
    }
}

void Buffer::serialize(object& dst) const
{
    write(mName, "name", dst);
    write(mUri, "uri", dst);
    write_always(mByteLength, "byteLength", dst);
};

const Buffer& Buffer::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "name", mName);
        copy(src, "uri", mUri);
        
        // NOTE: DO NOT attempt to handle the uri here. 
        // The uri is a reference to a file that is not loaded until
        // after the json document is parsed
    }
    return *this;
}

const Buffer& Buffer::operator=(const tinygltf::Buffer& src)
{
    mData = src.data;
    mName = src.name;
    mUri = src.uri;
    return *this;
}


void BufferView::serialize(object& dst) const
{
    write_always(mBuffer, "buffer", dst);
    write_always(mByteLength, "byteLength", dst);
    write(mByteOffset, "byteOffset", dst, 0);
    write(mByteStride, "byteStride", dst, 0);
    write(mTarget, "target", dst, -1);
    write(mName, "name", dst);
}

const BufferView& BufferView::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "buffer", mBuffer);
        copy(src, "byteLength", mByteLength);
        copy(src, "byteOffset", mByteOffset);
        copy(src, "byteStride", mByteStride);
        copy(src, "target", mTarget);
        copy(src, "name", mName);
    }
    return *this;
}

const BufferView& BufferView::operator=(const tinygltf::BufferView& src)
{
    mBuffer = src.buffer;
    mByteLength = src.byteLength;
    mByteOffset = src.byteOffset;
    mByteStride = src.byteStride;
    mTarget = src.target;
    mName = src.name;
    return *this;
}

Accessor::Type tinygltf_type_to_enum(S32 type)
{
    switch (type)
    {
    case TINYGLTF_TYPE_SCALAR:
        return Accessor::Type::SCALAR;
    case TINYGLTF_TYPE_VEC2:
        return Accessor::Type::VEC2;
    case TINYGLTF_TYPE_VEC3:
        return Accessor::Type::VEC3;
    case TINYGLTF_TYPE_VEC4:
        return Accessor::Type::VEC4;
    case TINYGLTF_TYPE_MAT2:
        return Accessor::Type::MAT2;
    case TINYGLTF_TYPE_MAT3:
        return Accessor::Type::MAT3;
    case TINYGLTF_TYPE_MAT4:
        return Accessor::Type::MAT4;
    }

    LL_WARNS("GLTF") << "Unknown tinygltf accessor type: " << type << LL_ENDL;
    llassert(false);

    return Accessor::Type::SCALAR;
}

void Accessor::serialize(object& dst) const
{
    write(mName, "name", dst);
    write(mBufferView, "bufferView", dst, INVALID_INDEX);
    write(mByteOffset, "byteOffset", dst, 0);
    write_always(mComponentType, "componentType", dst);
    write_always(mCount, "count", dst);
    write_always(enum_to_gltf_type(mType), "type", dst);
    write(mNormalized, "normalized", dst, false);
    write(mMax, "max", dst);
    write(mMin, "min", dst);
}

const Accessor& Accessor::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "name", mName);
        copy(src, "bufferView", mBufferView);
        copy(src, "byteOffset", mByteOffset);
        copy(src, "componentType", mComponentType);
        copy(src, "count", mCount);
        copy(src, "type", mType);
        copy(src, "normalized", mNormalized);
        copy(src, "max", mMax);
        copy(src, "min", mMin);
    }
    return *this;
}

const Accessor& Accessor::operator=(const tinygltf::Accessor& src)
{
    mBufferView = src.bufferView;
    mByteOffset = src.byteOffset;
    mComponentType = src.componentType;
    mCount = src.count;
    mType = tinygltf_type_to_enum(src.type);
    mNormalized = src.normalized;
    mName = src.name;
    mMax = src.maxValues;
    mMin = src.minValues;

    return *this;
}

