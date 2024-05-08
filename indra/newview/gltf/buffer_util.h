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
        // copy one Scalar from src to dst
        template<class S, class T>
        static void copyScalar(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one vec2 from src to dst
        template<class S, class T>
        static void copyVec2(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one vec3 from src to dst
        template<class S, class T>
        static void copyVec3(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one vec4 from src to dst
        template<class S, class T>
        static void copyVec4(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one vec2 from src to dst
        template<class S, class T>
        static void copyMat2(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one vec3 from src to dst
        template<class S, class T>
        static void copyMat3(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        // copy one vec4 from src to dst
        template<class S, class T>
        static void copyMat4(S* src, T& dst)
        {
            LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
        }

        //=========================================================================================================
        // concrete implementations for different types of source and destination
        //=========================================================================================================

// suppress unused function warning -- clang complains here but these specializations are definitely used
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

        template<>
        void copyScalar<F32, F32>(F32* src, F32& dst)
        {
            dst = *src;
        }

        template<>
        void copyScalar<U32, U32>(U32* src, U32& dst)
        {
            dst = *src;
        }

        template<>
        void copyScalar<U32, U16>(U32* src, U16& dst)
        {
            dst = *src;
        }

        template<>
        void copyScalar<U16, U16>(U16* src, U16& dst)
        {
            dst = *src;
        }

        template<>
        void copyScalar<U16, U32>(U16* src, U32& dst)
        {
            dst = *src;
        }

        template<>
        void copyScalar<U8, U16>(U8* src, U16& dst)
        {
            dst = *src;
        }

        template<>
        void copyScalar<U8, U32>(U8* src, U32& dst)
        {
            dst = *src;
        }

        template<>
        void copyVec2<F32, LLVector2>(F32* src, LLVector2& dst)
        {
            dst.set(src[0], src[1]);
        }

        template<>
        void copyVec3<F32, glh::vec3f>(F32* src, glh::vec3f& dst)
        {
            dst.set_value(src[0], src[1], src[2]);
        }

        template<>
        void copyVec3<F32, LLVector4a>(F32* src, LLVector4a& dst)
        {
            dst.load3(src);
        }

        template<>
        void copyVec3<U16, LLColor4U>(U16* src, LLColor4U& dst)
        {
            dst.set(src[0], src[1], src[2], 255);
        }

        template<>
        void copyVec4<U8, LLColor4U>(U8* src, LLColor4U& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
        }

        template<>
        void copyVec4<U16, LLColor4U>(U16* src, LLColor4U& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
        }

        template<>
        void copyVec4<F32, LLColor4U>(F32* src, LLColor4U& dst)
        {
            dst.set(src[0]*255, src[1]*255, src[2]*255, src[3]*255);
        }

        template<>
        void copyVec4<F32, LLVector4a>(F32* src, LLVector4a& dst)
        {
            dst.loadua(src);
        }

        template<>
        void copyVec4<U16, LLVector4a>(U16* src, LLVector4a& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
        }

        template<>
        void copyVec4<U8, LLVector4a>(U8* src, LLVector4a& dst)
        {
            dst.set(src[0], src[1], src[2], src[3]);
        }

        template<>
        void copyVec4<F32, glh::quaternionf>(F32* src, glh::quaternionf& dst)
        {
            dst.set_value(src);
        }

        template<>
        void copyMat4<F32, glh::matrix4f>(F32* src, glh::matrix4f& dst)
        {
            dst.set_value(src);
        }

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

        //=========================================================================================================

        // copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
        template<class S, class T>
        static void copyScalar(S* src, LLStrider<T> dst, S32 stride, S32 count)
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
        static void copyVec2(S* src, LLStrider<T> dst, S32 stride, S32 count)
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
        static void copyVec3(S* src, LLStrider<T> dst, S32 stride, S32 count)
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
        static void copyVec4(S* src, LLStrider<T> dst, S32 stride, S32 count)
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
        static void copyMat2(S* src, LLStrider<T> dst, S32 stride, S32 count)
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
        static void copyMat3(S* src, LLStrider<T> dst, S32 stride, S32 count)
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
        static void copyMat4(S* src, LLStrider<T> dst, S32 stride, S32 count)
        {
            for (S32 i = 0; i < count; ++i)
            {
                copyMat4(src, *dst);
                dst++;
                src = (S*)((U8*)src + stride);
            }
        }

        template<class S, class T>
        static void copy(Asset& asset, Accessor& accessor, const S* src, LLStrider<T>& dst, S32 byteStride)
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
        static void copy(Asset& asset, Accessor& accessor, LLStrider<T>& dst)
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
        static void copy(Asset& asset, Accessor& accessor, std::vector<T>& dst)
        {
            dst.resize(accessor.mCount);
            LLStrider<T> strider = dst.data();
            copy(asset, accessor, strider);
        }


        //=========================================================================================================
        // boost::json copying utilities
        // ========================================================================================================
        
        //====================== unspecialized base template ===========================

        // to/from Value
        template<typename T>
        static bool copy(const Value& src, T& dst)
        {
            dst = src;
            return true;
        }

        template<typename T>
        static bool write(const T& src, Value& dst)
        {
            dst = boost::json::object();
            src.serialize(dst.as_object());
            return true;
        }


        // to/from object member
        template<typename T>
        static bool copy(const boost::json::object& src, std::string_view member, T& dst)
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
        static bool write_always(const T& src, std::string_view member, boost::json::object& dst)
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
        static bool write(const T& src, std::string_view member, boost::json::object& dst, const T& default_value = T())
        {
            if (src != default_value)
            {
                return write_always(src, member, dst);
            }
            return false;
        }
        

        template<typename T>
        static bool copy(const Value& src, std::string_view member, T& dst)
        {
            if (src.is_object())
            {
                const boost::json::object& obj = src.as_object();
                return copy(obj, member, dst);
            }

            return false;
        }


        // to/from array
        template<typename T>
        bool copy(const Value& src, std::vector<T>& dst)
        {
            if (src.is_array())
            {
                const array& arr = src.get_array();
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
        bool write(const std::vector<T>& src, Value& dst)
        {
            array arr;
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

        template<typename T>
        bool write(const std::vector<T>& src, std::string_view member, boost::json::object& dst)
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

        // to/from map
        template<typename T>
        bool copy(const Value& src, std::unordered_map<std::string, T>& dst)
        {
            if (src.is_object())
            {
                const boost::json::object& obj = src.as_object();
                for (const auto& [key, value] : obj)
                {
                    copy(value, dst[key]);
                }
                return true;
            }
            return false;
        }

        template<typename T>
        bool write(const std::unordered_map<std::string, T>& src, Value& dst)
        {
            boost::json::object obj;
            for (const auto& [key, value] : src)
            {
                Value v;
                if (write(value, v))
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

        template<typename T>
        bool write(const std::unordered_map<std::string, T>& src, std::string_view member, boost::json::object& dst)
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

        // ====================== specialized template implementations ===========================
        
        
        // glh::vec4f
        template<>
        bool copy(const Value& src, glh::vec4f& dst)
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
                        dst.set_value(arr[0].get_double(), arr[1].get_double(), arr[2].get_double(), arr[3].get_double());
                        return true;
                    }
                }
            }
            return false;
        }

        template<>
        bool write(const glh::vec4f& src, Value& dst)
        {
            boost::json::array arr;
            arr.push_back(src.v[0]);
            arr.push_back(src.v[1]);
            arr.push_back(src.v[2]);
            arr.push_back(src.v[3]);
            dst = arr;
            return true;
        }

        // glh::quaternionf
        template<>
        bool copy(const Value& src, glh::quaternionf& dst)
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
                        dst.set_value(arr[0].get_double(), arr[1].get_double(), arr[2].get_double(), arr[3].get_double());
                        return true;
                    }
                }
            }
            return false;
        }

        template<>
        bool write(const glh::quaternionf& src, Value& dst)
        {
            boost::json::array arr;
            arr.push_back(src[0]);
            arr.push_back(src[1]);
            arr.push_back(src[2]);
            arr.push_back(src[3]);
            dst = arr;
            return true;
        }


        // glh::vec3f
        template<>
        bool copy(const Value& src, glh::vec3f& dst)
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
                        dst.set_value(arr[0].get_double(), arr[1].get_double(), arr[2].get_double());
                    }
                    return true;
                }
            }
            return false;
        }

        template<>
        bool write(const glh::vec3f& src, Value& dst)
        {
            boost::json::array arr;
            arr.push_back(src[0]);
            arr.push_back(src[1]);
            arr.push_back(src[2]);
            dst = arr;
            return true;
        }

        // bool
        template<>
        bool copy(const Value& src, bool& dst)
        {
            if (src.is_bool())
            {
                dst = src.get_bool();
                return true;
            }
            return false;
        }

        template<>
        bool write(const bool& src, Value& dst)
        {
            dst = src;
            return true;
        }

        // F32
        template<>
        bool copy(const Value& src, F32& dst)
        {
            if (src.is_double())
            {
                dst = src.get_double();
                return true;
            }
            return false;
        }

        template<>
        bool write(const F32& src, Value& dst)
        {
            dst = src;
            return true;
        }


        // U32
        template<>
        bool copy(const Value& src, U32& dst)
        {
            if (src.is_int64())
            {
                dst = src.get_int64();
                return true;
            }
            return false;
        }

        template<>
        bool write(const U32& src, Value& dst)
        {
            dst = src;
            return true;
        }

        // F64
        template<>
        bool copy(const Value& src, F64& dst)
        {
            if (src.is_double())
            {
                dst = src.get_double();
                return true;
            }
            return false;
        }

        template<>
        bool write(const F64& src, Value& dst)
        {
            dst = src;
            return true;
        }

        // Accessor::Type
        template<>
        bool copy(const Value& src, Accessor::Type& dst)
        {
            if (src.is_string())
            {
                dst = gltf_type_to_enum(src.get_string().c_str());
                return true;
            }
            return false;
        }

        template<>
        bool write(const Accessor::Type& src, Value& dst)
        {
            dst = enum_to_gltf_type(src);
            return true;
        }

        // S32
        template<>
        bool copy(const Value& src, S32& dst)
        {
            if (src.is_int64())
            {
                dst = src.get_int64();
                return true;
            }
            return false;
        }

        template<>
        bool write(const S32& src, Value& dst)
        {
            dst = src;
            return true;
        }


        // std::string
        template<>
        bool copy(const Value& src, std::string& dst)
        {
            if (src.is_string())
            {
                dst = src.get_string();
                return true;
            }
            return false;
        }

        template<>
        bool write(const std::string& src, Value& dst)
        {
            dst = src;
            return true;
        }

        // LLMatrix4a
        template<>
        bool copy(const Value& src, LLMatrix4a& dst)
        {
            if (src.is_array())
            {
                const boost::json::array& arr = src.get_array();
                if (arr.size() == 16)
                {
                    // populate a temporary local in case
                    // we hit an error in the middle of the array
                    // (don't partially write a matrix)
                    LLMatrix4a t;
                    F32* p = t.getF32ptr();

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
        bool write(const LLMatrix4a& src, Value& dst)
        {
            boost::json::array arr;
            arr.resize(16);
            const F32* p = src.getF32ptr();
            for (U32 i = 0; i < 16; ++i)
            {
                arr[i] = p[i];
            }
            dst = arr;
            return true;
        }

        // ========================================================================================================
    }
}

