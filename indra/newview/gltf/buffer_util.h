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

namespace LL
{
    namespace GLTF
    {

        using string_view = boost::json::string_view;

        // copy one Scalar from src to dst
        template<class S, class T>
        inline void copyScalar(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one vec2 from src to dst
        template<class S, class T>
        inline void copyVec2(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one vec3 from src to dst
        template<class S, class T>
        inline void copyVec3(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one vec4 from src to dst
        template<class S, class T>
        inline void copyVec4(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one mat2 from src to dst
        template<class S, class T>
        inline void copyMat2(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one mat3 from src to dst
        template<class S, class T>
        inline void copyMat3(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one mat4 from src to dst
        template<class S, class T>
        inline void copyMat4(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        //=========================================================================================================
        // concrete implementations for different types of source and destination
        //=========================================================================================================

        template<>
        inline void copyScalar<F32, F32>(F32* src, F32& dst)
        {
            dst = *src;
        }

        template<>
        inline void copyScalar<U32, U32>(U32* src, U32& dst)
        {
            dst = *src;
        }

        template<>
        inline void copyScalar<U32, U16>(U32* src, U16& dst)
        {
            dst = *src;
        }

        template<>
        inline void copyScalar<U16, U16>(U16* src, U16& dst)
        {
            dst = *src;
        }

        template<>
        inline void copyScalar<U16, U32>(U16* src, U32& dst)
        {
            dst = *src;
        }

        template<>
        inline void copyScalar<U8, U16>(U8* src, U16& dst)
        {
            dst = *src;
        }

        template<>
        inline void copyScalar<U8, U32>(U8* src, U32& dst)
        {
            dst = *src;
        }

        template<>
        inline void copyVec2<F32, LLVector2>(F32* src, LLVector2& dst)
        {
            dst.set(src[0], src[1]);
        }

        template<>
        inline void copyVec3<F32, vec3>(F32* src, vec3& dst)
        {
            dst = vec3(src[0], src[1], src[2]);
        }

        template<>
        inline void copyVec3<F32, LLVector4a>(F32* src, LLVector4a& dst)
        {
            dst.load3(src);
        }

        template<>
        inline void copyVec3<U16, LLColor4U>(U16* src, LLColor4U& dst)
        {
            dst.set(src[0], src[1], src[2], 255);
        }

        template<>
        inline void copyVec4<U8, LLColor4U>(U8* src, LLColor4U& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
        }

        template<>
        inline void copyVec4<U16, LLColor4U>(U16* src, LLColor4U& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
        }

        template<>
        inline void copyVec4<F32, LLColor4U>(F32* src, LLColor4U& dst)
        {
            dst.set(src[0]*255, src[1]*255, src[2]*255, src[3]*255);
        }

        template<>
        inline void copyVec4<F32, LLVector4a>(F32* src, LLVector4a& dst)
        {
            dst.loadua(src);
        }

        template<>
        inline void copyVec4<U16, LLVector4a>(U16* src, LLVector4a& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
        }

        template<>
        inline void copyVec4<U8, LLVector4a>(U8* src, LLVector4a& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
        }

        template<>
        inline void copyVec4<F32, quat>(F32* src, quat& dst)
        {
            dst.x = src[0];
            dst.y = src[1];
            dst.z = src[2];
            dst.w = src[3];
        }

        template<>
        inline void copyMat4<F32, mat4>(F32* src, mat4& dst)
        {
            dst = glm::make_mat4(src);
        }

        //=========================================================================================================

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline void copyScalar(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                copyScalar(src, *dst);
                dst++;
                src = (S*)((U8*)src + stride);
            }
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline void copyVec2(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                copyVec2(src, *dst);
                dst++;
                src = (S*)((U8*)src + stride);
            }
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline void copyVec3(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                copyVec3(src, *dst);
                dst++;
                src = (S*)((U8*)src + stride);
            }
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline void copyVec4(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                copyVec4(src, *dst);
                dst++;
                src = (S*)((U8*)src + stride);
            }
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline void copyMat2(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                copyMat2(src, *dst);
                dst++;
                src = (S*)((U8*)src + stride);
            }
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline void copyMat3(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                copyMat3(src, *dst);
                dst++;
                src = (S*)((U8*)src + stride);
            }
        }

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        inline void copyMat4(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                copyMat4(src, *dst);
                dst++;
                src = (S*)((U8*)src + stride);
            }
        }

        template<class S, class T>
        inline void copy(Asset& asset, Accessor& accessor, const S* src, LLStrider<T>& dst, S32 byteStride)
        {
            if (accessor.mType == Accessor::Type::SCALAR)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 1 : byteStride;
                copyScalar((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::VEC2)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 2 : byteStride;
                copyVec2((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::VEC3)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 3 : byteStride;
                copyVec3((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::VEC4)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 4 : byteStride;
                copyVec4((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::MAT2)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 4 : byteStride;
                copyMat2((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::MAT3)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 9 : byteStride;
                copyMat3((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == Accessor::Type::MAT4)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 16 : byteStride;
                copyMat4((S*)src, dst, stride, accessor.mCount);
            }
            else
            {
                LL_ERRS("GLTF") << "Unsupported accessor type" << LL_ENDL;
            }
        }

        // copy data from accessor to strider
        template<class T>
        inline void copy(Asset& asset, Accessor& accessor, LLStrider<T>& dst)
        {
            const BufferView& bufferView = asset.mBufferViews[accessor.mBufferView];
            const Buffer& buffer = asset.mBuffers[bufferView.mBuffer];
            const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

            if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                LL::GLTF::copy(asset, accessor, (const F32*)src, dst, bufferView.mByteStride);
            }
            else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                LL::GLTF::copy(asset, accessor, (const U16*)src, dst, bufferView.mByteStride);
            }
            else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                LL::GLTF::copy(asset, accessor, (const U32*)src, dst, bufferView.mByteStride);
            }
            else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                LL::GLTF::copy(asset, accessor, (const U8*)src, dst, bufferView.mByteStride);
            }
            else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_SHORT)
            {
                LL::GLTF::copy(asset, accessor, (const S16*)src, dst, bufferView.mByteStride);
            }
            else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_BYTE)
            {
                LL::GLTF::copy(asset, accessor, (const S8*)src, dst, bufferView.mByteStride);
            }
            else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_DOUBLE)
            {
                LL::GLTF::copy(asset, accessor, (const F64*)src, dst, bufferView.mByteStride);
            }
            else
            {
                LL_ERRS("GLTF") << "Unsupported component type" << LL_ENDL;
            }
        }

        // copy data from accessor to vector
        template<class T>
        inline void copy(Asset& asset, Accessor& accessor, std::vector<T>& dst)
        {
            dst.resize(accessor.mCount);
            LLStrider<T> strider = dst.data();
            copy(asset, accessor, strider);
        }


        //=========================================================================================================
        // boost::json copying utilities
        // ========================================================================================================
        
        //====================== unspecialized base template, single value ===========================

        // to/from Value
        template<typename T>
        inline bool copy(const Value& src, T& dst)
        {
            dst = src;
            return true;
        }

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
                for (const auto& [key, value] : obj)
                {
                    copy<T>(value, dst[key]);
                }
                return true;
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
                for (size_t i = 0; i < arr.size(); ++i)
                {
                    copy(arr[i], dst[i]);
                }
                return true;
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
                return copy(it->value(), dst);
            }
            return false;
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

        // vec4
        template<>
        inline bool copy(const Value& src, vec4& dst)
        {
            if (src.is_array())
            {
                const boost::json::array& arr = src.as_array();
                if (arr.size() == 4)
                {
                    if (arr[0].is_double() &&
                        arr[1].is_double() &&
                        arr[2].is_double() &&
                        arr[3].is_double())
                    {
                        dst = vec4(arr[0].get_double(), arr[1].get_double(), arr[2].get_double(), arr[3].get_double());
                        return true;
                    }
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
                    if (arr[0].is_double() &&
                        arr[1].is_double() &&
                        arr[2].is_double() &&
                        arr[3].is_double())
                    {
                        dst.x = arr[0].get_double();
                        dst.y = arr[1].get_double();
                        dst.z = arr[2].get_double();
                        dst.w = arr[3].get_double();
                        return true;
                    }
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
                    if (arr[0].is_double() &&
                        arr[1].is_double() &&
                        arr[2].is_double())
                    {
                        dst = vec3(arr[0].get_double(), arr[1].get_double(), arr[2].get_double());
                    }
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
            if (src.is_double())
            {
                dst = src.get_double();
                return true;
            }
            return false;
        }

        template<>
        inline bool write(const F32& src, Value& dst)
        {
            dst = src;
            return true;
        }


        // U32
        template<>
        inline bool copy(const Value& src, U32& dst)
        {
            if (src.is_int64())
            {
                dst = src.get_int64();
                return true;
            }
            return false;
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
            if (src.is_double())
            {
                dst = src.get_double();
                return true;
            }
            return false;
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
            if (src.is_string())
            {
                dst = gltf_type_to_enum(src.get_string().c_str());
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
            if (src.is_int64())
            {
                dst = src.get_int64();
                return true;
            }
            return false;
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
            if (src.is_string())
            {
                dst = src.get_string().c_str();
                return true;
            }
            return false;
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
                        if (arr[i].is_double())
                        {
                            p[i] = arr[i].get_double();
                        }
                        else
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

        // ========================================================================================================

    }
}




