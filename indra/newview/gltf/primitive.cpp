/**
 * @file primitive.cpp
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
#include "../lltinygltfhelper.h"

using namespace LL::GLTF;

#ifdef _MSC_VER
#define LL_FUNCSIG __FUNCSIG__ 
#else
#define LL_FUNCSIG __PRETTY_FUNCTION__
#endif

// copy one vec3 from src to dst
template<class S, class T>
void copyVec2(S* src, T& dst)
{
    LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
}

// copy one vec3 from src to dst
template<class S, class T>
void copyVec3(S* src, T& dst)
{
    LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
}

// copy one vec4 from src to dst
template<class S, class T>
void copyVec4(S* src, T& dst)
{
    LL_ERRS() << "TODO: implement " << LL_FUNCSIG << LL_ENDL;
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
        src = (S*)((U8*)src + stride);
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
        copyAttributeArray(asset, accessor, (const F32*)src, dst, bufferView.mByteStride);
    }
    else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    {
        copyAttributeArray(asset, accessor, (const U16*)src, dst, bufferView.mByteStride);
    }
    else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
    {
        copyAttributeArray(asset, accessor, (const U32*)src, dst, bufferView.mByteStride);
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
        mIndexArray.resize(numIndices);

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

        U16* idx = (U16*)mVertexBuffer->getMappedIndices();
        for (U32 i = 0; i < numIndices; ++i)
        {
            mIndexArray[i] = idx[i];
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
    {  // TODO: generate tangents if needed
        LLStrider<LLVector4a> dst;
        mVertexBuffer->getTangentStrider(dst);
        for (U32 i = 0; i < numVertices; ++i)
        {
            *(dst++) = LLVector4a(1.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    mPositions.resize(numVertices);
    mTexCoords.resize(numVertices);
    mNormals.resize(numVertices);
    mTangents.resize(numVertices);

    LLVector4a* pos = (LLVector4a*)(mVertexBuffer->getMappedData() + mVertexBuffer->getOffset(LLVertexBuffer::TYPE_VERTEX));
    LLVector2* tc = (LLVector2*)(mVertexBuffer->getMappedData() + mVertexBuffer->getOffset(LLVertexBuffer::TYPE_TEXCOORD0));
    LLVector4a* norm = (LLVector4a*)(mVertexBuffer->getMappedData() + mVertexBuffer->getOffset(LLVertexBuffer::TYPE_NORMAL));
    LLVector4a* tangent = (LLVector4a*)(mVertexBuffer->getMappedData() + mVertexBuffer->getOffset(LLVertexBuffer::TYPE_TANGENT));
    for (U32 i = 0; i < numVertices; ++i)
    {
        mPositions[i] = pos[i];
        mTexCoords[i] = tc[i];
        mNormals[i] = norm[i];
        mTangents[i] = tangent[i];
    }
    createOctree();
    
    mVertexBuffer->unmapBuffer();
}

void Primitive::createOctree()
{
    // create octree
    mOctree = new LLVolumeOctree();

    if (mMode == TINYGLTF_MODE_TRIANGLES)
    {
        F32 scaler = 0.25f;

        const U32 num_triangles = mVertexBuffer->getNumIndices() / 3;
        // Initialize all the triangles we need
        mOctreeTriangles.resize(num_triangles);

        LLVector4a* pos = (LLVector4a*)(mVertexBuffer->getMappedData() + mVertexBuffer->getOffset(LLVertexBuffer::TYPE_VERTEX));
        U16* indices = (U16*)mVertexBuffer->getMappedIndices();

        for (U32 triangle_index = 0; triangle_index < num_triangles; ++triangle_index)
        { //for each triangle
            const U32 index = triangle_index * 3;
            LLVolumeTriangle* tri = &mOctreeTriangles[triangle_index];
            const LLVector4a& v0 = pos[indices[index]];
            const LLVector4a& v1 = pos[indices[index + 1]];
            const LLVector4a& v2 = pos[indices[index + 2]];

            //store pointers to vertex data
            tri->mV[0] = &v0;
            tri->mV[1] = &v1;
            tri->mV[2] = &v2;

            //store indices
            tri->mIndex[0] = indices[index];
            tri->mIndex[1] = indices[index + 1];
            tri->mIndex[2] = indices[index + 2];

            //get minimum point
            LLVector4a min = v0;
            min.setMin(min, v1);
            min.setMin(min, v2);

            //get maximum point
            LLVector4a max = v0;
            max.setMax(max, v1);
            max.setMax(max, v2);

            //compute center
            LLVector4a center;
            center.setAdd(min, max);
            center.mul(0.5f);

            tri->mPositionGroup = center;

            //compute "radius"
            LLVector4a size;
            size.setSub(max, min);

            tri->mRadius = size.getLength3().getF32() * scaler;

            //insert
            mOctree->insert(tri);
        }
    }
    else
    {
        LL_ERRS() << "Unsupported Primitive mode" << LL_ENDL;
    }

    //remove unneeded octree layers
    while (!mOctree->balance()) {}

    //calculate AABB for each node
    LLVolumeOctreeRebound rebound;
    rebound.traverse(mOctree);
}

const LLVolumeTriangle* Primitive::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
    LLVector4a* intersection, LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent_out)
{
    if (mOctree.isNull())
    {
        return nullptr;
    }

    LLVector4a dir;
    dir.setSub(end, start);

    F32 closest_t = 2.f; // must be larger than 1

    //create a proxy LLVolumeFace for the raycast
    LLVolumeFace face;
    face.mPositions = mPositions.data();
    face.mTexCoords = mTexCoords.data();
    face.mNormals = mNormals.data();
    face.mTangents = mTangents.data();
    face.mIndices = mIndexArray.data();

    face.mNumIndices = mIndexArray.size();
    face.mNumVertices = mPositions.size();

    LLOctreeTriangleRayIntersect intersect(start, dir, &face, &closest_t, intersection, tex_coord, normal, tangent_out);
    intersect.traverse(mOctree);

    // null out proxy data so it doesn't get freed
    face.mPositions = face.mNormals = face.mTangents = nullptr;
    face.mIndices = nullptr;
    face.mTexCoords = nullptr;

    return intersect.mHitTriangle;
}

Primitive::~Primitive()
{
    mOctree = nullptr;
}

