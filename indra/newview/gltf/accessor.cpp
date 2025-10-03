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
#include "llfilesystem.h"

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
    S32 idx = (S32)(this - &asset.mBuffers[0]);

    mData.erase(mData.begin() + offset, mData.begin() + offset + length);

    llassert(mData.size() <= size_t(INT_MAX));
    mByteLength = S32(mData.size());

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

bool Buffer::prep(Asset& asset)
{
    if (mByteLength == 0)
    {
        return false;
    }

    LLUUID id;
    if (mUri.size() == UUID_STR_SIZE && LLUUID::parseUUID(mUri, &id) && id.notNull())
    { // loaded from an asset, fetch the buffer data from the asset store
        LLFileSystem file(id, LLAssetType::AT_GLTF_BIN, LLFileSystem::READ);

        if (mByteLength > file.getSize())
        {
            LL_WARNS("GLTF") << "Unexpected glbin size: " << id << " is " << file.getSize() << " bytes, expected " << mByteLength << LL_ENDL;
            return false;
        }

        mData.resize(mByteLength);
        if (!file.read((U8*)mData.data(), mByteLength))
        {
            LL_WARNS("GLTF") << "Failed to load buffer data from asset: " << id << LL_ENDL;
            return false;
        }
    }
    else if (mUri.find("data:") == 0)
    { // loaded from a data URI, load the texture from the data
        LL_WARNS() << "Data URIs not yet supported" << LL_ENDL;
        return false;
    }
    else if (!asset.mFilename.empty() &&
        !mUri.empty()) // <-- uri could be empty if we're loading from .glb
    {
        std::string dir = gDirUtilp->getDirName(asset.mFilename);
        std::string bin_file = dir + gDirUtilp->getDirDelimiter() + mUri;

        llifstream file(bin_file.c_str(), std::ios::binary);
        if (!file.is_open())
        {
            LL_WARNS("GLTF") << "Failed to open file: " << bin_file << LL_ENDL;
            return false;
        }

        file.seekg(0, std::ios::end);
        if (mByteLength > file.tellg())
        {
            LL_WARNS("GLTF") << "Unexpected file size: " << bin_file << " is " << file.tellg() << " bytes, expected " << mByteLength << LL_ENDL;
            return false;
        }
        file.seekg(0, std::ios::beg);

        mData.resize(mByteLength);
        file.read((char*)mData.data(), mData.size());
    }

    // POSTCONDITION: on success, mData.size == mByteLength
    llassert(mData.size() == mByteLength);
    return true;
}

bool Buffer::save(Asset& asset, const std::string& folder)
{
    if (mUri.substr(0, 5) == "data:")
    {
        LL_WARNS("GLTF") << "Data URIs not yet supported" << LL_ENDL;
        return false;
    }

    std::string bin_file = folder + gDirUtilp->getDirDelimiter();

    if (mUri.empty())
    {
        if (mName.empty())
        {
            S32 idx = (S32)(this - &asset.mBuffers[0]);
            mUri = llformat("buffer_%d.bin", idx);
        }
        else
        {
            mUri = mName + ".bin";
        }
    }

    bin_file += mUri;

    std::ofstream file(bin_file, std::ios::binary);
    if (!file.is_open())
    {
        LL_WARNS("GLTF") << "Failed to open file: " << bin_file << LL_ENDL;
        return false;
    }

    file.write((char*)mData.data(), mData.size());

    return true;
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
        copy(src, "byteLength", mByteLength);

        // NOTE: DO NOT attempt to handle the uri here.
        // The uri is a reference to a file that is not loaded until
        // after the json document is parsed
    }
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

