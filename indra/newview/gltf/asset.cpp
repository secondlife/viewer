/**
 * @file asset.cpp
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

#include "asset.h"
#include "llvolumeoctree.h"

// disable optimizations for debugging
#pragma optimize("", off)

using namespace LL::GLTF;

void Scene::updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview)
{
    for (auto& nodeIndex : mNodes)
    {
        Node& node = asset.mNodes[nodeIndex];
        node.updateRenderTransforms(asset, modelview);
    }
}

#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

// copy one vec3 from src to dst
template<class S, class T>
void copyVec2(S* src, T& dst)
{
    LL_ERRS() << "TODO: implement " << __PRETTY_FUNCTION__ << LL_ENDL;
}

// copy one vec3 from src to dst
template<class S, class T>
void copyVec3(S* src, T& dst)
{
    LL_ERRS() << "TODO: implement " << __PRETTY_FUNCTION__ << LL_ENDL;
}

// copy one vec4 from src to dst
template<class S, class T>
void copyVec4(S* src, T& dst)
{
    LL_ERRS() << "TODO: implement " << __PRETTY_FUNCTION__ << LL_ENDL;
}

template<>
void copyVec2<F32, LLVector2>(F32* src, LLVector2& dst)
{
    dst.set(src[0], src[1]);
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
void copyVec4<F32, LLVector4a>(F32* src, LLVector4a& dst)
{
    dst.loadua(src);
}

// copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
template<class S, class T>
void copyVec2(S* src, LLStrider<T> dst, S32 stride, S32 count)
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
void copyVec3(S* src, LLStrider<T> dst, S32 stride, S32 count)
{
    for (S32 i = 0; i < count; ++i)
    {
        copyVec3(src, *dst);
        dst++;
        src = (S*) ((U8*)src + stride);
    }
}

// copy from src to dst, stride is the number of bytes between each element in src, count is number of elements to copy
template<class S, class T>
void copyVec4(S* src, LLStrider<T> dst, S32 stride, S32 count)
{
    for (S32 i = 0; i < count; ++i)
    {
        copyVec3(src, *dst);
        dst++;
        src = (S*)((U8*)src + stride);
    }
}

template<class S, class T>
void copyAttributeArray(Asset& asset, const Accessor& accessor, const S* src, LLStrider<T>& dst, S32 byteStride)
{
    if (accessor.mType == TINYGLTF_TYPE_VEC2)
    {
        S32 stride = byteStride == 0 ? sizeof(S) * 2 : byteStride;
        copyVec2((S*)src, dst, stride, accessor.mCount);
    }
    else if (accessor.mType == TINYGLTF_TYPE_VEC3)
    {
        S32 stride = byteStride == 0 ? sizeof(S) * 3 : byteStride;
        copyVec3((S*)src, dst, stride, accessor.mCount);
    }
    else if (accessor.mType == TINYGLTF_TYPE_VEC4)
    {
        S32 stride = byteStride == 0 ? sizeof(S) * 4 : byteStride;
        copyVec4((S*)src, dst, stride, accessor.mCount);
    }
    else
    {
        LL_ERRS("GLTF") << "Unsupported accessor type" << LL_ENDL;
    }
}

template <class T>
void Primitive::copyAttribute(Asset& asset, S32 accessorIdx, LLStrider<T>& dst)
{
    const Accessor& accessor = asset.mAccessors[accessorIdx];
    const BufferView& bufferView = asset.mBufferViews[accessor.mBufferView];
    const Buffer& buffer = asset.mBuffers[bufferView.mBuffer];
    const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

    if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
    {
        copyAttributeArray(asset, accessor, (const F32*) src, dst, bufferView.mByteStride);
    }
    else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    {
        copyAttributeArray(asset, accessor, (const U16*) src, dst, bufferView.mByteStride);
    }
    else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
    {
        copyAttributeArray(asset, accessor, (const U32*) src, dst, bufferView.mByteStride);
    }
    else
    
    {
        LL_ERRS() << "Unsupported component type" << LL_ENDL;
    }
}

void Primitive::allocateGLResources(Asset& asset)
{
    // allocate vertex buffer
    // We diverge from the intent of the GLTF format here to work with our existing render pipeline
    // GLTF wants us to copy the buffer views into GPU storage as is and build render commands that source that data.
    // For our engine, though, it's better to rearrange the buffers at load time into a layout that's more consistent.
    // The GLTF native approach undoubtedly works well if you can count on VAOs, but VAOs perform much worse with our scenes.

    // get the number of vertices
    U32 numVertices = 0;
    for (auto& it : mAttributes)
    {
        const Accessor& accessor = asset.mAccessors[it.second];
        numVertices = accessor.mCount;
        break;
    }

    // get the number of indices
    U32 numIndices = 0;
    if (mIndices != INVALID_INDEX)
    {
        const Accessor& accessor = asset.mAccessors[mIndices];
        numIndices = accessor.mCount;
    }

    // create vertex buffer
    mVertexBuffer = new LLVertexBuffer(ATTRIBUTE_MASK);
    mVertexBuffer->allocateBuffer(numVertices, numIndices);

    bool needs_color = true;
    bool needs_texcoord = true;
    bool needs_normal = true;
    bool needs_tangent = true;

    // load vertex data
    for (auto& it : mAttributes)
    {
        const std::string& attribName = it.first;
        
        // load vertex data
        if (attribName == "POSITION")
        {
            // load position data
            LLStrider<LLVector4a> dst;
            mVertexBuffer->getVertexStrider(dst);

            copyAttribute(asset, it.second, dst);
        }
        else if (attribName == "NORMAL")
        {
            needs_normal = false;
            // load normal data
            LLStrider<LLVector4a> dst;
            mVertexBuffer->getNormalStrider(dst);
            
            copyAttribute(asset, it.second, dst);
        }
        else if (attribName == "TANGENT")
        {
            needs_tangent = false;
            // load tangent data

            LLStrider<LLVector4a> dst;
            mVertexBuffer->getTangentStrider(dst);

            copyAttribute(asset, it.second, dst);
        }
        else if (attribName == "COLOR_0")
        {
            needs_color = false;
            // load color data

            LLStrider<LLColor4U> dst;
            mVertexBuffer->getColorStrider(dst);

            copyAttribute(asset, it.second, dst);
        }
        else if (attribName == "TEXCOORD_0")
        {
            needs_texcoord = false;
            // load texcoord data
            LLStrider<LLVector2> dst;
            mVertexBuffer->getTexCoord0Strider(dst);

            LLStrider<LLVector2> tc = dst;
            copyAttribute(asset, it.second, dst);

            // convert to OpenGL coordinate space
            for (U32 i = 0; i < numVertices; ++i)
            { 
                tc->mV[1] = 1.0f - tc->mV[1];;
                tc++;
            }
        }
    }

    // copy index buffer
    if (mIndices != INVALID_INDEX)
    {
        const Accessor& accessor = asset.mAccessors[mIndices];
        const BufferView& bufferView = asset.mBufferViews[accessor.mBufferView];
        const Buffer& buffer = asset.mBuffers[bufferView.mBuffer];

        const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

        LLStrider<U16> dst;
        mVertexBuffer->getIndexStrider(dst);

        if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
        {
            for (U32 i = 0; i < numIndices; ++i)
            {
                *(dst++) = (U16) * (U32*)src;
                src += sizeof(U32);
            }
        }
        else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
            for (U32 i = 0; i < numIndices; ++i)
            {
                *(dst++) = *(U16*)src;
                src += sizeof(U16);
            }
        }
        else
        {
            LL_ERRS("GLTF") << "Unsupported component type for indices" << LL_ENDL;
        }
    }

    // fill in default values for missing attributes
    if (needs_color)
    { // set default color
        LLStrider<LLColor4U> dst;
        mVertexBuffer->getColorStrider(dst);
        for (U32 i = 0; i < numVertices; ++i)
        {
            *(dst++) = LLColor4U(255, 255, 255, 255);
        }
    }

    if (needs_texcoord)
    { // set default texcoord
        LLStrider<LLVector2> dst;
        mVertexBuffer->getTexCoord0Strider(dst);
        for (U32 i = 0; i < numVertices; ++i)
        {
            *(dst++) = LLVector2(0.0f, 0.0f);
        }
    }

    if (needs_normal)
    { // set default normal
        LLStrider<LLVector4a> dst;
        mVertexBuffer->getNormalStrider(dst);
        for (U32 i = 0; i < numVertices; ++i)
        {
            *(dst++) = LLVector4a(0.0f, 0.0f, 1.0f, 0.0f);
        }
    }

    if (needs_tangent)
    {  // TODO -- generate tangents if normals and texture coordinates are present
        LLStrider<LLVector4a> dst;
        mVertexBuffer->getTangentStrider(dst);
        for (U32 i = 0; i < numVertices; ++i)
        {
            *(dst++) = LLVector4a(1.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    mVertexBuffer->unmapBuffer();
}