#pragma once

/**
 * @file buffer_util.inl
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

// inline template implementations for copying data out of GLTF buffers
// DO NOT include from header files to avoid the need to rebuild the whole project
// whenever we add support for more types

#ifdef _MSC_VER
#define LL_FUNCSIG __FUNCSIG__
#else
#define LL_FUNCSIG __PRETTY_FUNCTION__
#endif

#include "accessor.h"

#include <algorithm>
#include <cmath>

namespace LL
{
    namespace GLTF
    {

        using string_view = boost::json::string_view;

        // members present in the json that failed validation
        // points at Asset::mInvalidFields for the duration of Asset::operator=
        inline thread_local std::vector<std::string>* sInvalidFields = nullptr;

        // records "<member>: <reason>, got <json kind> <value>"
        inline void copy_failed(string_view member, const Value& value, const char* reason)
        {
            if (sInvalidFields == nullptr) {
                return;
            }

            std::string got = boost::json::serialize(value);

            if (got.size() > 64) {
                got.resize(64);
            }

            std::string kind(boost::json::to_string(value.kind()));
            sInvalidFields->push_back(std::string(member) + ": " + reason + ", got " + kind + " " + got);
        }

        // value validators, passed as the last argument to copy(src, member, dst, validator)

        // value must fall within [mMin, mMax]
        template<typename T>
        struct RangeValidator {
            T mMin;
            T mMax;

            bool operator()(const T& value) const {
                return value >= mMin && value <= mMax;
            }
        };

        // value must be one of mValues; a single specific value is a list of one
        template<typename T>
        struct ValueValidator {
            std::vector<T> mValues;

            bool operator()(const T& value) const {
                return std::find(mValues.begin(), mValues.end(), value) != mValues.end();
            }
        };

        // strings and string enums are lists of allowed strings
        using StringValidator = ValueValidator<std::string>;
        using EnumValidator = ValueValidator<std::string>;

        // copy one Scalar from src to dst
        template<class S, class T>
        inline bool copyScalar(S* src, T& dst)
        {
            return false;
        }

        // copy one vec2 from src to dst
        template<class S, class T>
        inline bool copyVec2(S* src, T& dst)
        {
            return false;
        }

        // copy one vec3 from src to dst
        template<class S, class T>
        inline bool copyVec3(S* src, T& dst)
        {
            return false;
        }

        // copy one vec4 from src to dst
        template<class S, class T>
        inline bool copyVec4(S* src, T& dst)
        {
            return false;
        }

        // copy one mat2 from src to dst
        template<class S, class T>
        inline bool copyMat2(S* src, T& dst)
        {
            return false;
        }

        // copy one mat3 from src to dst
        template<class S, class T>
        inline bool copyMat3(S* src, T& dst)
        {
            return false;
        }

        // copy one mat4 from src to dst
        template<class S, class T>
        inline bool copyMat4(S* src, T& dst)
        {
            return false;
        }

        //=========================================================================================================
        // concrete implementations for different types of source and destination
        //=========================================================================================================

        template<>
        inline bool copyScalar<F32, F32>(F32* src, F32& dst)
        {
            dst = *src;
            return true;
        }

        template<>
        inline bool copyScalar<U32, U32>(U32* src, U32& dst)
        {
            dst = *src;
            return true;
        }

        template<>
        inline bool copyScalar<U32, U16>(U32* src, U16& dst)
        {
            dst = *src;
            return true;
        }

        template<>
        inline bool copyScalar<U16, U16>(U16* src, U16& dst)
        {
            dst = *src;
            return true;
        }

        template<>
        inline bool copyScalar<U16, U32>(U16* src, U32& dst)
        {
            dst = *src;
            return true;
        }

        template<>
        inline bool copyScalar<U8, U16>(U8* src, U16& dst)
        {
            dst = *src;
            return true;
        }

        template<>
        inline bool copyScalar<U8, U32>(U8* src, U32& dst)
        {
            dst = *src;
            return true;
        }

        template<>
        inline bool copyScalar<U32, LLVector4a>(U32* src, LLVector4a& dst)
        {
            dst.set((F32)*src, 0.f, 0.f, 0.f);
            return true;
        }

        template<>
        inline bool copyScalar<U16, LLVector4a>(U16* src, LLVector4a& dst)
        {
            dst.set((F32)*src, 0.f, 0.f, 0.f);
            return true;
        }

        template<>
        inline bool copyScalar<U8, LLVector4a>(U8* src, LLVector4a& dst)
        {
            dst.set((F32)*src, 0.f, 0.f, 0.f);
            return true;
        }

        template<>
        inline bool copyScalar<U32, LLVector2>(U32* src, LLVector2& dst)
        {
            dst.set((F32)*src, 0.f);
            return true;
        }

        template<>
        inline bool copyScalar<U16, LLVector2>(U16* src, LLVector2& dst)
        {
            dst.set((F32)*src, 0.f);
            return true;
        }

        template<>
        inline bool copyScalar<U8, LLVector2>(U8* src, LLVector2& dst)
        {
            dst.set((F32)*src, 0.f);
            return true;
        }

        template<>
        inline bool copyVec2<F32, LLVector2>(F32* src, LLVector2& dst)
        {
            dst.set(src[0], src[1]);
            return true;
        }

        template<>
        inline bool copyVec3<F32, LLVector2>(F32* src, LLVector2& dst)
        {
            dst.set(src[0], src[1]);
            return true;
        }

        template<>
        inline bool copyVec3<F32, vec3>(F32* src, vec3& dst)
        {
            dst = vec3(src[0], src[1], src[2]);
            return true;
        }

        template<>
        inline bool copyVec3<F32, LLVector4a>(F32* src, LLVector4a& dst)
        {
            dst.load3(src);
            return true;
        }

        template<>
        inline bool copyVec3<F32, LLColor4U>(F32* src, LLColor4U& dst)
        {
            dst.set((U8)(src[0] * 255.f), (U8)(src[1] * 255.f), (U8)(src[2] * 255.f), 255);
            return true;
        }

        template<>
        inline bool copyVec3<U16, LLColor4U>(U16* src, LLColor4U& dst)
        {
            dst.set((U8)(src[0]), (U8)(src[1]), (U8)(src[2]), 255);
            return true;
        }

        template<>
        inline bool copyVec4<U8, LLColor4U>(U8* src, LLColor4U& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
            return true;
        }

        template<>
        inline bool copyVec4<U16, U64>(U16* src, U64& dst)
        {
            U16* data = (U16*)&dst;
            data[0] = src[0];
            data[1] = src[1];
            data[2] = src[2];
            data[3] = src[3];
            return true;
        }

        template<>
        inline bool copyVec4<U8, U64>(U8* src, U64& dst)
        {
            U8* data = (U8*)&dst;
            data[0] = src[0];
            data[1] = src[1];
            data[2] = src[2];
            data[3] = src[3];
            return true;
        }

        template<>
        inline bool copyVec4<U16, LLColor4U>(U16* src, LLColor4U& dst)
        {
            dst.set((U8)(src[0]), (U8)(src[1]), (U8)(src[2]), ((U8)src[3]));
            return true;
        }

        template<>
        inline bool copyVec4<F32, LLColor4U>(F32* src, LLColor4U& dst)
        {
            dst.set((U8)(src[0]*255.f), (U8)(src[1]*255.f), (U8)(src[2]*255.f), (U8)(src[3]*255.f));
            return true;
        }

        template<>
        inline bool copyVec4<F32, LLVector4a>(F32* src, LLVector4a& dst)
        {
            dst.loadua(src);
            return true;
        }

        template<>
        inline bool copyVec4<U32, LLVector4a>(U32* src, LLVector4a& dst)
        {
            dst.set((F32)src[0], (F32)src[1], (F32)src[2], (F32)src[3]);
            return true;
        }

        template<>
        inline bool copyVec4<U16, LLVector4a>(U16* src, LLVector4a& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
            return true;
        }

        template<>
        inline bool copyVec4<U8, LLVector4a>(U8* src, LLVector4a& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
            return true;
        }

        template<>
        inline bool copyVec4<F32, quat>(F32* src, quat& dst)
        {
            dst.x = src[0];
            dst.y = src[1];
            dst.z = src[2];
            dst.w = src[3];
            return true;
        }

        template<>
        inline bool copyMat4<F32, mat4>(F32* src, mat4& dst)
        {
            dst = glm::make_mat4(src);
            return true;
        }

        //=========================================================================================================

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline bool copyScalar(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                if (!copyScalar(src, *dst))
                {
                    return false;
                }
                dst++;
                src = (S*)((U8*)src + stride);
            }
            return true;
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline bool copyVec2(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                if (!copyVec2(src, *dst))
                {
                    return false;
                }
                dst++;
                src = (S*)((U8*)src + stride);
            }
            return true;
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline bool copyVec3(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                if (!copyVec3(src, *dst))
                {
                    return false;
                }
                dst++;
                src = (S*)((U8*)src + stride);
            }
            return true;
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline bool copyVec4(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                if (!copyVec4(src, *dst))
                {
                    return false;
                }
                dst++;
                src = (S*)((U8*)src + stride);
            }
            return true;
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline bool copyMat2(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                if (!copyMat2(src, *dst))
                {
                    return false;
                }
                dst++;
                src = (S*)((U8*)src + stride);
            }
            return true;
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline bool copyMat3(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                if (!copyMat3(src, *dst))
                {
                    return false;
                }
                dst++;
                src = (S*)((U8*)src + stride);
            }
            return true;
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline bool copyMat4(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                if (!copyMat4(src, *dst))
                {
                    return false;
                }
                dst++;
                src = (S*)((U8*)src + stride);
            }
            return true;
        }

        template<class S, class T>
        inline bool copy(Asset& asset, Accessor& accessor, const S* src, LLStrider<T>& dst, S32 byteStride)
        {
            if (accessor.mType == Accessor::Type::SCALAR)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 1 : byteStride;
                return copyScalar((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::VEC2)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 2 : byteStride;
                return copyVec2((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::VEC3)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 3 : byteStride;
                return copyVec3((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::VEC4)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 4 : byteStride;
                return copyVec4((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::MAT2)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 4 : byteStride;
                return copyMat2((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::MAT3)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 9 : byteStride;
                return copyMat3((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::MAT4)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 16 : byteStride;
                return copyMat4((S*)src, dst, stride, accessor.mCount);
            }
            else
            {
                LL_WARNS("GLTF") << "Unsupported accessor type " << (S32)accessor.mType << LL_ENDL;
                return false;
            }
        }

        // copy data from accessor to strider
        template<class T>
        inline bool copy(Asset& asset, Accessor& accessor, LLStrider<T>& dst)
        {
            if (accessor.mBufferView < 0
                || accessor.mBufferView >= asset.mBufferViews.size())
            {
                LL_WARNS("GLTF") << "Invalid buffer" << LL_ENDL;
                return false;
            }
            const BufferView& bufferView = asset.mBufferViews[accessor.mBufferView];
            if (bufferView.mBuffer < 0
                || bufferView.mBuffer >= asset.mBuffers.size())
            {
                LL_WARNS("GLTF") << "Invalid buffer view" << LL_ENDL;
                return false;
            }
            const Buffer& buffer = asset.mBuffers[bufferView.mBuffer];

            S64 componentSize = 0;
            switch (accessor.mComponentType)
            {
            case Accessor::ComponentType::BYTE:
            case Accessor::ComponentType::UNSIGNED_BYTE:
                componentSize = 1;
                break;
            case Accessor::ComponentType::SHORT:
            case Accessor::ComponentType::UNSIGNED_SHORT:
                componentSize = 2;
                break;
            case Accessor::ComponentType::UNSIGNED_INT:
            case Accessor::ComponentType::FLOAT:
                componentSize = 4;
                break;
            default:
                LL_WARNS("GLTF") << "Invalid component type" << LL_ENDL;
                return false;
            }

            S64 componentCount = 0;
            switch (accessor.mType)
            {
            case Accessor::Type::SCALAR:
                componentCount = 1;
                break;
            case Accessor::Type::VEC2:
                componentCount = 2;
                break;
            case Accessor::Type::VEC3:
                componentCount = 3;
                break;
            case Accessor::Type::VEC4:
            case Accessor::Type::MAT2:
                componentCount = 4;
                break;
            case Accessor::Type::MAT3:
                componentCount = 9;
                break;
            case Accessor::Type::MAT4:
                componentCount = 16;
                break;
            default:
                LL_WARNS("GLTF") << "Invalid accessor type" << LL_ENDL;
                return false;
            }

            S64 elementSize = componentSize * componentCount;
            S64 stride = bufferView.mByteStride;
            if (stride == 0)
            {
                stride = elementSize;
            }

            if (accessor.mCount < 0
                || accessor.mByteOffset < 0
                || bufferView.mByteOffset < 0
                || stride < elementSize)
            {
                LL_WARNS("GLTF") << "Accessor out of range" << LL_ENDL;
                return false;
            }

            if (accessor.mCount == 0)
            {
                return true;
            }

            S64 requiredBytes = ((S64)accessor.mCount - 1) * stride + elementSize;
            S64 startOffset = (S64)bufferView.mByteOffset + (S64)accessor.mByteOffset;
            if (startOffset + requiredBytes > (S64)buffer.mData.size())
            {
                LL_WARNS("GLTF") << "Accessor out of range" << LL_ENDL;
                return false;
            }

            const U8* src = buffer.mData.data() + startOffset;

            switch (accessor.mComponentType)
            {
            case Accessor::ComponentType::FLOAT:
                return copy(asset, accessor, (const F32*)src, dst, bufferView.mByteStride);
            case Accessor::ComponentType::UNSIGNED_INT:
                return copy(asset, accessor, (const U32*)src, dst, bufferView.mByteStride);
            case Accessor::ComponentType::SHORT:
                return copy(asset, accessor, (const S16*)src, dst, bufferView.mByteStride);
            case Accessor::ComponentType::UNSIGNED_SHORT:
                return copy(asset, accessor, (const U16*)src, dst, bufferView.mByteStride);
            case Accessor::ComponentType::BYTE:
                return copy(asset, accessor, (const S8*)src, dst, bufferView.mByteStride);
            case Accessor::ComponentType::UNSIGNED_BYTE:
                return copy(asset, accessor, (const U8*)src, dst, bufferView.mByteStride);
            default:
                LL_WARNS("GLTF") << "Invalid component type" << LL_ENDL;
                return false;
            }
        }

        // copy data from accessor to vector
        template<class T>
        inline bool copy(Asset& asset, Accessor& accessor, std::vector<T>& dst)
        {
            if (accessor.mCount < 0
                || accessor.mBufferView < 0
                || accessor.mBufferView >= asset.mBufferViews.size())
            {
                return false;
            }

            const BufferView& bufferView = asset.mBufferViews[accessor.mBufferView];

            if (bufferView.mBuffer < 0 || bufferView.mBuffer >= asset.mBuffers.size()) {
                return false;
            }

            // every element takes at least one byte, so a larger count is never valid
            if ((size_t)accessor.mCount > asset.mBuffers[bufferView.mBuffer].mData.size()) {
                return false;
            }

            dst.resize(accessor.mCount);
            LLStrider<T> strider = dst.data();
            return copy(asset, accessor, strider);
        }


        //=========================================================================================================
        // boost::json copying utilities
        // ========================================================================================================

        //====================== unspecialized base template, single value ===========================

        // to/from Value
        template<typename T>
        inline bool copy(const Value& src, T& dst)
        {
            if (!src.is_object()) {
                return false;
            }

            dst = src;
            return true;
        }

        // extras may hold any json value
        template<>
        inline bool copy(const Value& src, Value& dst)
        {
            dst = src;
            return true;
        }

        template<>
        inline bool copy(const Value& src, F32& dst);

        template<>
        inline bool copy(const Value& src, S32& dst);

        template<>
        inline bool copy(const Value& src, std::string& dst);

        template<typename T>
        inline bool write(const T& src, Value& dst)
        {
            dst = boost::json::object();
            src.serialize(dst.as_object());
            return true;
        }

        template<typename T>
        inline bool copy(const Value& src, std::unordered_map<std::string, T>& dst)
        {
            if (src.is_object())
            {
                const boost::json::object& obj = src.as_object();
                bool success = true;

                for (const auto& [key, value] : obj)
                {
                    if (!copy<T>(value, dst[key])) {
                        success = false;
                    }
                }

                return success;
            }
            return false;
        }

        template<typename T>
        inline bool write(const std::unordered_map<std::string, T>& src, Value& dst)
        {
            boost::json::object obj;
            for (const auto& [key, value] : src)
            {
                Value v;
                if (write<T>(value, v))
                {
                    obj[key] = v;
                }
                else
                {
                    return false;
                }
            }
            dst = obj;
            return true;
        }

        // to/from array
        template<typename T>
        inline bool copy(const Value& src, std::vector<T>& dst)
        {
            if (src.is_array())
            {
                const boost::json::array& arr = src.get_array();
                dst.resize(arr.size());
                bool success = true;

                for (size_t i = 0; i < arr.size(); ++i)
                {
                    if (!copy(arr[i], dst[i])) {
                        success = false;
                    }
                }

                return success;
            }

            return false;
        }

        template<typename T>
        inline bool write(const std::vector<T>& src, Value& dst)
        {
            boost::json::array arr;
            for (const T& t : src)
            {
                Value v;
                if (write(t, v))
                {
                    arr.push_back(v);
                }
                else
                {
                    return false;
                }
            }
            dst = arr;
            return true;
        }

        // to/from object member
        template<typename T>
        inline bool copy(const boost::json::object& src, string_view member, T& dst)
        {
            auto it = src.find(member);
            if (it != src.end())
            {
                if (copy(it->value(), dst)) {
                    return true;
                }

                copy_failed(member, it->value(), "wrong type or out of range");
            }
            return false;
        }

        // as above, and the copied value must also pass the validator
        template<typename T, typename V>
        inline bool copy(const boost::json::object& src, string_view member, T& dst, const V& validator)
        {
            auto it = src.find(member);

            if (it == src.end()) {
                return false;
            }

            T value = dst;

            if (!copy(it->value(), value)) {
                copy_failed(member, it->value(), "wrong type or out of range");
                return false;
            }

            if (!validator(value)) {
                copy_failed(member, it->value(), "value not allowed");
                return false;
            }

            dst = value;
            return true;
        }

        // always write a member to an object without checking default
        template<typename T>
        inline bool write_always(const T& src, string_view member, boost::json::object& dst)
        {
            Value& v = dst[member];
            if (!write(src, v))
            {
                dst.erase(member);
                return false;
            }
            return true;
        }


        // to/from extension

        // for internal use only, use copy_extensions instead
        template<typename T>
        inline void _copy_extension(bool& success, const boost::json::object& extensions, std::string_view member, T* dst)
        {
            if (extensions.contains(member))
            {
                if (copy(extensions.at(member), *dst))
                {
                    success = true;
                }
                else
                {
                    copy_failed(member, extensions.at(member), "wrong type or out of range");
                }
            }
        }

        // for internal use only, use copy_extensions instead
        template<typename T, class... Types>
        inline void _copy_extension(bool& success, const boost::json::object& extensions, std::string_view member, T* dst, Types... args)
        {
            _copy_extension(success, extensions, member, dst);
            _copy_extension(success, extensions, args...);
        }

        // Copy all extensions from src.extensions to provided destinations
        // Usage:
        //  copy_extensions(src,
        //                  "KHR_materials_unlit", &mUnlit,
        //                  "KHR_materials_pbrSpecularGlossiness", &mPbrSpecularGlossiness);
        // returns true if any of the extensions are copied
        template<typename... Types>
        inline bool copy_extensions(const boost::json::value& src, Types... args)
        {
            // extract the extensions object (don't assume it exists and verify that it is an object)
            bool success = false;

            if (src.is_object())
            {
                boost::json::object obj = src.get_object();
                if (obj.contains("extensions"))
                {
                    const boost::json::value& extensions = obj.at("extensions");
                    if (extensions.is_object())
                    {
                        const boost::json::object& ext_obj = extensions.as_object();
                        _copy_extension(success, ext_obj, args...);
                    }
                    else
                    {
                        copy_failed("extensions", extensions, "not an object");
                    }
                }
            }

            return success;
        }

        // internal use only, use write_extensions instead
        template<typename T>
        inline void _write_extension(bool &success, boost::json::object& extensions, const T* src, string_view member)
        {
            if (src->mPresent)
            {
                Value v;
                if (write(*src, v))
                {
                    extensions[member] = v;
                    success = true;
                }
            }
        }

        // for internal use only, use write_extensions instead
        template<typename T, class... Types>
        inline void _write_extension(bool &success, boost::json::object& extensions, const T* src, string_view member, Types... args)
        {
            _write_extension(success, extensions, src, member);
            _write_extension(success, extensions, args...);
        }

        // Write all extensions to dst.extensions
        // Usage:
        //  write_extensions(dst,
        //                   &mUnlit, "KHR_materials_unlit",
        //                   &mPbrSpecularGlossiness, "KHR_materials_pbrSpecularGlossiness");
        // returns true if any of the extensions are written
        template<class... Types>
        inline bool write_extensions(boost::json::object& dst, Types... args)
        {
            bool success = false;

            boost::json::object extensions;
            _write_extension(success, extensions, args...);

            if (success)
            {
                dst["extensions"] = extensions;
            }

            return success;
        }

        // conditionally write a member to an object if the member
        // is not the default value
        template<typename T>
        inline bool write(const T& src, string_view member, boost::json::object& dst, const T& default_value = T())
        {
            if (src != default_value)
            {
                return write_always(src, member, dst);
            }
            return false;
        }

        template<typename T>
        inline bool write(const std::unordered_map<std::string, T>& src, string_view member, boost::json::object& dst, const std::unordered_map<std::string, T>& default_value = std::unordered_map<std::string, T>())
        {
            if (!src.empty())
            {
                Value v;
                if (write<T>(src, v))
                {
                    dst[member] = v;
                    return true;
                }
            }
            return false;
        }

        template<typename T>
        inline bool write(const std::vector<T>& src, string_view member, boost::json::object& dst, const std::vector<T>& deafault_value = std::vector<T>())
        {
            if (!src.empty())
            {
                Value v;
                if (write(src, v))
                {
                    dst[member] = v;
                    return true;
                }
            }
            return false;
        }

        template<typename T>
        inline bool copy(const Value& src, string_view member, T& dst)
        {
            if (src.is_object())
            {
                const boost::json::object& obj = src.as_object();
                return copy(obj, member, dst);
            }

            return false;
        }

        template<typename T, typename V>
        inline bool copy(const Value& src, string_view member, T& dst, const V& validator)
        {
            if (src.is_object())
            {
                const boost::json::object& obj = src.as_object();
                return copy(obj, member, dst, validator);
            }

            return false;
        }

        // Accessor::ComponentType
        template<>
        inline bool copy(const Value& src, Accessor::ComponentType& dst)
        {
            static const ValueValidator<S32> valid = {
                {
                    (S32)Accessor::ComponentType::BYTE,
                    (S32)Accessor::ComponentType::UNSIGNED_BYTE,
                    (S32)Accessor::ComponentType::SHORT,
                    (S32)Accessor::ComponentType::UNSIGNED_SHORT,
                    (S32)Accessor::ComponentType::UNSIGNED_INT,
                    (S32)Accessor::ComponentType::FLOAT
                }
            };

            S32 value = 0;

            if (copy(src, value) && valid(value))
            {
                dst = (Accessor::ComponentType)value;
                return true;
            }
            return false;
        }

        template<>
        inline bool write(const Accessor::ComponentType& src, Value& dst)
        {
            dst = (S32)src;
            return true;
        }

        //Primitive::Mode
        template<>
        inline bool copy(const Value& src, Primitive::Mode& dst)
        {
            static const RangeValidator<S32> valid = { (S32)Primitive::Mode::POINTS, (S32)Primitive::Mode::TRIANGLE_FAN };

            S32 value = 0;

            if (copy(src, value) && valid(value))
            {
                dst = (Primitive::Mode)value;
                return true;
            }
            return false;
        }

        template<>
        inline bool write(const Primitive::Mode& src, Value& dst)
        {
            dst = (S32)src;
            return true;
        }

        // vec4
        template<>
        inline bool copy(const Value& src, vec4& dst)
        {
            if (src.is_array())
            {
                const boost::json::array& arr = src.as_array();
                if (arr.size() == 4)
                {
                    vec4 v;

                    if (!copy(arr[0], v.x) || !copy(arr[1], v.y) || !copy(arr[2], v.z) || !copy(arr[3], v.w)) {
                        return false;
                    }

                    dst = v;

                    return true;
                }
            }
            return false;
        }

        template<>
        inline bool write(const vec4& src, Value& dst)
        {
            dst = boost::json::array();
            boost::json::array& arr = dst.get_array();
            arr.resize(4);
            arr[0] = src.x;
            arr[1] = src.y;
            arr[2] = src.z;
            arr[3] = src.w;
            return true;
        }

        // quat
        template<>
        inline bool copy(const Value& src, quat& dst)
        {
            if (src.is_array())
            {
                const boost::json::array& arr = src.as_array();
                if (arr.size() == 4)
                {
                    quat q;

                    if (!copy(arr[0], q.x) || !copy(arr[1], q.y) || !copy(arr[2], q.z) || !copy(arr[3], q.w)) {
                        return false;
                    }

                    dst = q;
                    return true;
                }
            }
            return false;
        }

        template<>
        inline bool write(const quat& src, Value& dst)
        {
            dst = boost::json::array();
            boost::json::array& arr = dst.get_array();
            arr.resize(4);
            arr[0] = src.x;
            arr[1] = src.y;
            arr[2] = src.z;
            arr[3] = src.w;
            return true;
        }


        // vec3
        template<>
        inline bool copy(const Value& src, vec3& dst)
        {
            if (src.is_array())
            {
                const boost::json::array& arr = src.as_array();
                if (arr.size() == 3)
                {
                    vec3 t;

                    if (!copy(arr[0], t.x) || !copy(arr[1], t.y) || !copy(arr[2], t.z)) {
                        return false;
                    }

                    dst = t;
                    return true;
                }
            }
            return false;
        }

        template<>
        inline bool write(const vec3& src, Value& dst)
        {
            dst = boost::json::array();
            boost::json::array& arr = dst.as_array();
            arr.resize(3);
            arr[0] = src.x;
            arr[1] = src.y;
            arr[2] = src.z;
            return true;
        }

        // vec2
        template<>
        inline bool copy(const Value& src, vec2& dst)
        {
            if (src.is_array())
            {
                const boost::json::array& arr = src.as_array();
                if (arr.size() == 2)
                {
                    vec2 t;

                    if (!copy(arr[0], t.x) || !copy(arr[1], t.y)) {
                        return false;
                    }

                    dst = t;
                    return true;
                }
            }
            return false;
        }

        template<>
        inline bool write(const vec2& src, Value& dst)
        {
            dst = boost::json::array();
            boost::json::array& arr = dst.as_array();
            arr.resize(2);
            arr[0] = src.x;
            arr[1] = src.y;

            return true;
        }

        // bool
        template<>
        inline bool copy(const Value& src, bool& dst)
        {
            if (src.is_bool())
            {
                dst = src.get_bool();
                return true;
            }
            return false;
        }

        template<>
        inline bool write(const bool& src, Value& dst)
        {
            dst = src;
            return true;
        }

        // F32
        template<>
        inline bool copy(const Value& src, F32& dst)
        {
            std::error_code ec;
            F64 t = src.to_number<F64>(ec);

            // F32_MAX written as json text can read back a hair above it; anything short of
            // halfway to the next float step still rounds to F32_MAX
            constexpr F64 F32_ROUNDING_LIMIT = 0x1.ffffffp+127;

            if (ec || t <= -F32_ROUNDING_LIMIT || t >= F32_ROUNDING_LIMIT) {
                return false;
            }

            dst = (F32)llclamp(t, -(F64)F32_MAX, (F64)F32_MAX);
            return true;
        }

        template<>
        inline bool write(const F32& src, Value& dst)
        {
            dst = src;
            return true;
        }


        // U32
        template<>
        // glTF allows an integer to be written as 0.0 or 0e0, which boost::json stores as a double
        inline bool copy(const Value& src, U32& dst)
        {
            std::error_code ec;
            F64 t = src.to_number<F64>(ec);

            if (ec || t != std::floor(t) || t < 0.0 || t > (F64)U32_MAX) {
                return false;
            }

            dst = (U32)t;
            return true;
        }

        template<>
        inline bool write(const U32& src, Value& dst)
        {
            dst = src;
            return true;
        }

        // F64
        template<>
        inline bool copy(const Value& src, F64& dst)
        {
            std::error_code ec;
            F64 t = src.to_number<F64>(ec); if (ec) return false;
            dst = t;
            return true;
        }

        template<>
        inline bool write(const F64& src, Value& dst)
        {
            dst = src;
            return true;
        }

        // Accessor::Type
        template<>
        inline bool copy(const Value& src, Accessor::Type& dst)
        {
            static const EnumValidator valid = { { "SCALAR", "VEC2", "VEC3", "VEC4", "MAT2", "MAT3", "MAT4" } };

            std::string type;

            if (copy(src, type) && valid(type))
            {
                dst = gltf_type_to_enum(type);
                return true;
            }
            return false;
        }

        template<>
        inline bool write(const Accessor::Type& src, Value& dst)
        {
            dst = enum_to_gltf_type(src);
            return true;
        }

        // S32
        template<>
        inline bool copy(const Value& src, S32& dst)
        {
            std::error_code ec;
            F64 t = src.to_number<F64>(ec);

            if (ec || t != std::floor(t) || t < (F64)S32_MIN || t > (F64)S32_MAX) {
                return false;
            }

            dst = (S32)t;
            return true;
        }

        template<>
        inline bool write(const S32& src, Value& dst)
        {
            dst = src;
            return true;
        }


        // std::string
        template<>
        inline bool copy(const Value& src, std::string& dst)
        {
            if (!src.is_string()) {
                return false;
            }

            const boost::json::string& text = src.get_string();

            // a C string conversion would silently drop everything after an embedded NUL
            if (std::find(text.begin(), text.end(), '\0') != text.end()) {
                return false;
            }

            dst.assign(text.data(), text.size());
            return true;
        }

        template<>
        inline bool write(const std::string& src, Value& dst)
        {
            dst = src;
            return true;
        }

        // mat4
        template<>
        inline bool copy(const Value& src, mat4& dst)
        {
            if (src.is_array())
            {
                const boost::json::array& arr = src.get_array();
                if (arr.size() == 16)
                {
                    // populate a temporary local in case
                    // we hit an error in the middle of the array
                    // (don't partially write a matrix)
                    mat4 t;
                    F32* p = glm::value_ptr(t);

                    for (U32 i = 0; i < arr.size(); ++i)
                    {
                        if (!copy(arr[i], p[i]))
                        {
                            return false;
                        }
                    }

                    dst = t;
                    return true;
                }
            }

            return false;
        }

        template<>
        inline bool write(const mat4& src, Value& dst)
        {
            dst = boost::json::array();
            boost::json::array& arr = dst.get_array();
            arr.resize(16);
            const F32* p = glm::value_ptr(src);
            for (U32 i = 0; i < 16; ++i)
            {
                arr[i] = p[i];
            }
            return true;
        }

        // Material::AlphaMode
        template<>
        inline bool copy(const Value& src, Material::AlphaMode& dst)
        {
            static const EnumValidator valid = { { "OPAQUE", "MASK", "BLEND" } };

            std::string alpha_mode;

            if (copy(src, alpha_mode) && valid(alpha_mode))
            {
                dst = gltf_alpha_mode_to_enum(alpha_mode);
                return true;
            }
            return false;
        }

        template<>
        inline bool write(const Material::AlphaMode& src, Value& dst)
        {
            dst = enum_to_gltf_alpha_mode(src);
            return true;
        }

        //
        // ========================================================================================================

    }
}




