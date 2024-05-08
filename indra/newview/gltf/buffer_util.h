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
            if (accessor.mType == (S32)Accessor::Type::SCALAR)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 1 : byteStride;
                copyScalar((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == (S32)Accessor::Type::VEC2)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 2 : byteStride;
                copyVec2((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == (S32)Accessor::Type::VEC3)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 3 : byteStride;
                copyVec3((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == (S32)Accessor::Type::VEC4)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 4 : byteStride;
                copyVec4((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == (S32)Accessor::Type::MAT2)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 4 : byteStride;
                copyMat2((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == (S32)Accessor::Type::MAT3)
            {
                S32 stride = byteStride == 0 ? sizeof(S) * 9 : byteStride;
                copyMat3((S*)src, dst, stride, accessor.mCount);
            }
            else if (accessor.mType == (S32)Accessor::Type::MAT4)
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
    }
}

