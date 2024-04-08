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


using namespace LL::GLTF;

void Scene::updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview)
{
    for (auto& nodeIndex : mNodes)
    {
        Node& node = asset.mNodes[nodeIndex];
        node.updateRenderTransforms(asset, modelview);
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
        const Accessor& accessor = asset.mAccessors[it.second];
        const BufferView& bufferView = asset.mBufferViews[accessor.mBufferView];
        const Buffer& buffer = asset.mBuffers[bufferView.mBuffer];

        // load vertex data
        if (attribName == "POSITION")
        {
            // load position data
            const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

            LLStrider<LLVector4a> dst;
            mVertexBuffer->getVertexStrider(dst);

            if (accessor.mType == TINYGLTF_TYPE_VEC3)
            {
                
                // load vec3 position data
                for (U32 i = 0; i < numVertices; ++i)
                {
                    S32 stride = 0;
                    switch (accessor.mComponentType)
                    {
                    case TINYGLTF_COMPONENT_TYPE_FLOAT:
                        (dst++)->load3((F32*)src);
                        stride = sizeof(F32) * 3;
                        break;
                    default:
                        LL_ERRS("GLTF") << "Unsupported component type for POSITION attribute" << LL_ENDL;
                    }

                    if (bufferView.mByteStride == 0)
                    {
                        src += stride;
                    }
                    else
                    {
                        src += bufferView.mByteStride;
                    }
                }
            }
            else
            {
                LL_ERRS("GLTF") << "Unsupported type for POSITION attribute" << LL_ENDL;
            }
        }
        else if (attribName == "NORMAL")
        {
            needs_normal = false;
            // load normal data
            const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

            LLStrider<LLVector4a> dst;
            mVertexBuffer->getNormalStrider(dst);

            if (accessor.mType == TINYGLTF_TYPE_VEC3)
            {
                // load vec3 normal data
                for (U32 i = 0; i < numVertices; ++i)
                {
                    S32 stride = 0;
                    switch (accessor.mComponentType)
                    {
                    case TINYGLTF_COMPONENT_TYPE_FLOAT:
                        (dst++)->load3((F32*)src);
                        stride = sizeof(F32) * 3;
                        break;
                    default:
                        LL_ERRS("GLTF") << "Unsupported component type for NORMAL attribute" << LL_ENDL;
                    }

                    if (bufferView.mByteStride == 0)
                    {
                        src += stride;
                    }
                    else
                    {
                        src += bufferView.mByteStride;
                    }
                }
            }
            else
            {
                LL_ERRS("GLTF") << "Unsupported type for NORMAL attribute" << LL_ENDL;
            }
        }
        else if (attribName == "TANGENT")
        {
            needs_tangent = false;
            // load tangent data
            const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

            LLStrider<LLVector4a> dst;
            mVertexBuffer->getTangentStrider(dst);

            if (accessor.mType == TINYGLTF_TYPE_VEC4)
            {
                // load vec4 tangent data
                for (U32 i = 0; i < numVertices; ++i)
                {
                    S32 stride = 0;
                    switch (accessor.mComponentType)
                    {
                    case TINYGLTF_COMPONENT_TYPE_FLOAT:
                        (dst++)->loadua((F32*)src);
                        stride = sizeof(F32) * 4;
                        break;
                    default:
                        LL_ERRS("GLTF") << "Unsupported component type for TANGENT attribute" << LL_ENDL;
                    }

                    if (bufferView.mByteStride == 0)
                    {
                        src += stride;
                    }
                    else
                    {
                        src += bufferView.mByteStride;
                    }
                }
            }
            else
            {
                LL_ERRS("GLTF") << "Unsupported type for TANGENT attribute" << LL_ENDL;
            }
        }
        else if (attribName == "COLOR_0")
        {
            needs_color = false;
            // load color data
            const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

            LLStrider<LLColor4U> dst;
            mVertexBuffer->getColorStrider(dst);

            if (accessor.mType == TINYGLTF_TYPE_VEC4)
            {
                // load vec4 color data
                for (U32 i = 0; i < numVertices; ++i)
                {
                    S32 stride = 0;
                    switch (accessor.mComponentType)
                    {
                    case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {
                        LLColor4 c((F32*)src);
                        *(dst++) = c;
                        stride = sizeof(F32) * 4;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    {
                        U16* c = (U16*)src;
                        *(dst++) = LLColor4U(c[0], c[1], c[2], c[3]);
                        stride = sizeof(U16) * 4;
                    }
                    break;
                    default:
                        LL_ERRS("GLTF") << "Unsupported component type for COLOR_0 attribute" << LL_ENDL;
                    }

                    if (bufferView.mByteStride == 0)
                    {
                        src += stride;
                    }
                    else
                    {
                        src += bufferView.mByteStride;
                    }
                }
            }
            else
            {
                LL_ERRS("GLTF") << "Unsupported type for COLOR_0 attribute" << LL_ENDL;
            }
        }
        else if (attribName == "TEXCOORD_0")
        {
            needs_texcoord = false;
            // load texcoord data
            const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

            LLStrider<LLVector2> dst;
            mVertexBuffer->getTexCoord0Strider(dst);

            if (accessor.mType == TINYGLTF_TYPE_VEC2)
            {
                // load vec2 texcoord data
                for (U32 i = 0; i < numVertices; ++i)
                {
                    S32 stride = 0;
                    switch (accessor.mComponentType)
                    {
                    case TINYGLTF_COMPONENT_TYPE_FLOAT:
                        dst->set((F32*)src);
                        // convert to OpenGL UV space
                        dst->mV[1] = 1.f - dst->mV[1];
                        dst++;
                        stride = sizeof(F32) * 2;
                        break;
                    default:
                        LL_ERRS("GLTF") << "Unsupported component type for TEXCOORD_0 attribute" << LL_ENDL;
                    }

                    if (bufferView.mByteStride == 0)
                    {
                        src += stride;
                    }
                    else
                    {
                        src += bufferView.mByteStride;
                    }
                }
            }
            else if (accessor.mType == TINYGLTF_TYPE_SCALAR)
            {
                // load scalar texcoord data
                for (U32 i = 0; i < numVertices; ++i)
                {
                    S32 stride = 0;
                    switch (accessor.mComponentType)
                    {
                    case TINYGLTF_COMPONENT_TYPE_FLOAT:
                        (dst++)->set(*(F32*)src, 0.0f);
                        stride = sizeof(F32);
                        break;
                    default:
                        LL_ERRS("GLTF") << "Unsupported component type for TEXCOORD_0 attribute" << LL_ENDL;
                    }

                    if (bufferView.mByteStride == 0)
                    {
                        src += stride;
                    }
                    else
                    {
                        src += bufferView.mByteStride;
                    }
                }
            }
            else
            {
                LL_ERRS("GLTF") << "Unsupported type for TEXCOORD_0 attribute" << LL_ENDL;
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
    {  // TODO -- generate tangents
        LLStrider<LLVector4a> dst;
        mVertexBuffer->getTangentStrider(dst);
        for (U32 i = 0; i < numVertices; ++i)
        {
            *(dst++) = LLVector4a(1.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    mVertexBuffer->unmapBuffer();
}