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
#include "buffer_util.h"

#include "../lltinygltfhelper.h"

using namespace LL::GLTF;

void Primitive::allocateGLResources(Asset& asset)
{
    // allocate vertex buffer
    // We diverge from the intent of the GLTF format here to work with our existing render pipeline
    // GLTF wants us to copy the buffer views into GPU storage as is and build render commands that source that data.
    // For our engine, though, it's better to rearrange the buffers at load time into a layout that's more consistent.
    // The GLTF native approach undoubtedly works well if you can count on VAOs, but VAOs perform much worse with our scenes.

    // load vertex data
    for (auto& it : mAttributes)
    {
        const std::string& attribName = it.first;
        Accessor& accessor = asset.mAccessors[it.second];

        // load vertex data
        if (attribName == "POSITION")
        {
            copy(asset, accessor, mPositions);
        }
        else if (attribName == "NORMAL")
        {
            copy(asset, accessor, mNormals);
        }
        else if (attribName == "TANGENT")
        {
            copy(asset, accessor, mTangents);
        }
        else if (attribName == "COLOR_0")
        {
            copy(asset, accessor, mColors);
        }
        else if (attribName == "TEXCOORD_0")
        {
            copy(asset, accessor, mTexCoords);
        }
        else if (attribName == "JOINTS_0")
        {
            copy(asset, accessor, mJoints);
        }
        else if (attribName == "WEIGHTS_0")
        {
            copy(asset, accessor, mWeights);
        }
    }

    // copy index buffer
    if (mIndices != INVALID_INDEX)
    {
        Accessor& accessor = asset.mAccessors[mIndices];
        copy(asset, accessor, mIndexArray);
    }

    U32 mask = ATTRIBUTE_MASK;
    
    if (!mWeights.empty())
    {
        mask |= LLVertexBuffer::MAP_WEIGHT4;
    }

    mVertexBuffer = new LLVertexBuffer(mask);
    mVertexBuffer->allocateBuffer(mPositions.size(), mIndexArray.size()*2); // double the size of the index buffer for 32-bit indices

    mVertexBuffer->setBuffer();
    mVertexBuffer->setPositionData(mPositions.data());

    if (!mIndexArray.empty())
    {
        mVertexBuffer->setIndexData(mIndexArray.data());
    }

    if (mTexCoords.empty())
    {
        mTexCoords.resize(mPositions.size());
    }

    // flip texcoord y, upload, then flip back (keep the off-spec data in vram only)
    for (auto& tc : mTexCoords)
    {
        tc[1] = 1.f - tc[1];
    }
    mVertexBuffer->setTexCoordData(mTexCoords.data());

    for (auto& tc : mTexCoords)
    {
        tc[1] = 1.f - tc[1];
    }

    if (mColors.empty())
    {
        mColors.resize(mPositions.size(), LLColor4U::white);
    }
    
    // bake material basecolor into color array
    if (mMaterial != INVALID_INDEX)
    {
        const Material& material = asset.mMaterials[mMaterial];
        LLColor4 baseColor = material.mMaterial->mBaseColor;
        for (auto& dst : mColors)
        {
            dst = LLColor4U(baseColor * LLColor4(dst));
        }
    }

    mVertexBuffer->setColorData(mColors.data());

    if (mNormals.empty())
    {
        mNormals.resize(mPositions.size(), LLVector4a(0, 0, 1, 0));
    }
    
    mVertexBuffer->setNormalData(mNormals.data());

    if (mTangents.empty())
    {
        // TODO: generate tangents if needed
        mTangents.resize(mPositions.size(), LLVector4a(1, 0, 0, 1));
    }

    mVertexBuffer->setTangentData(mTangents.data());

    if (!mWeights.empty())
    {
        std::vector<LLVector4a> weight_data;
        weight_data.resize(mWeights.size());

        F32 max_weight = 1.f - FLT_EPSILON*100.f;
        LLVector4a maxw(max_weight, max_weight, max_weight, max_weight);
        for (U32 i = 0; i < mWeights.size(); ++i)
        {
            LLVector4a& w = weight_data[i];
            w.setMin(mWeights[i], maxw);
            w.add(mJoints[i]);
        };

        mVertexBuffer->setWeight4Data(weight_data.data());
    }
    
    createOctree();
    
    mVertexBuffer->unbind();
}

void initOctreeTriangle(LLVolumeTriangle* tri, F32 scaler, S32 i0, S32 i1, S32 i2, const LLVector4a& v0, const LLVector4a& v1, const LLVector4a& v2)
{
    //store pointers to vertex data
    tri->mV[0] = &v0;
    tri->mV[1] = &v1;
    tri->mV[2] = &v2;

    //store indices
    tri->mIndex[0] = i0;
    tri->mIndex[1] = i1;
    tri->mIndex[2] = i2;

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
}

void Primitive::createOctree()
{
    // create octree
    mOctree = new LLVolumeOctree();

    F32 scaler = 0.25f;

    if (mMode == TINYGLTF_MODE_TRIANGLES)
    {
        const U32 num_triangles = mVertexBuffer->getNumIndices() / 3;
        // Initialize all the triangles we need
        mOctreeTriangles.resize(num_triangles);

        for (U32 triangle_index = 0; triangle_index < num_triangles; ++triangle_index)
        { //for each triangle
            const U32 index = triangle_index * 3;
            LLVolumeTriangle* tri = &mOctreeTriangles[triangle_index];
            S32 i0 = mIndexArray[index];
            S32 i1 = mIndexArray[index + 1];
            S32 i2 = mIndexArray[index + 2];

            const LLVector4a& v0 = mPositions[i0];
            const LLVector4a& v1 = mPositions[i1];
            const LLVector4a& v2 = mPositions[i2];
            
            initOctreeTriangle(tri, scaler, i0, i1, i2, v0, v1, v2);
            
            //insert
            mOctree->insert(tri);
        }
    }
    else if (mMode == TINYGLTF_MODE_TRIANGLE_STRIP)
    {
        const U32 num_triangles = mVertexBuffer->getNumIndices() - 2;
        // Initialize all the triangles we need
        mOctreeTriangles.resize(num_triangles);

        for (U32 triangle_index = 0; triangle_index < num_triangles; ++triangle_index)
        { //for each triangle
            const U32 index = triangle_index + 2;
            LLVolumeTriangle* tri = &mOctreeTriangles[triangle_index];
            S32 i0 = mIndexArray[index];
            S32 i1 = mIndexArray[index - 1];
            S32 i2 = mIndexArray[index - 2];

            const LLVector4a& v0 = mPositions[i0];
            const LLVector4a& v1 = mPositions[i1];
            const LLVector4a& v2 = mPositions[i2];

            initOctreeTriangle(tri, scaler, i0, i1, i2, v0, v1, v2);

            //insert
            mOctree->insert(tri);
        }
    }
    else if (mMode == TINYGLTF_MODE_TRIANGLE_FAN)
    {
        const U32 num_triangles = mVertexBuffer->getNumIndices() - 2;
        // Initialize all the triangles we need
        mOctreeTriangles.resize(num_triangles);

        for (U32 triangle_index = 0; triangle_index < num_triangles; ++triangle_index)
        { //for each triangle
            const U32 index = triangle_index + 2;
            LLVolumeTriangle* tri = &mOctreeTriangles[triangle_index];
            S32 i0 = mIndexArray[0];
            S32 i1 = mIndexArray[index - 1];
            S32 i2 = mIndexArray[index - 2];

            const LLVector4a& v0 = mPositions[i0];
            const LLVector4a& v1 = mPositions[i1];
            const LLVector4a& v2 = mPositions[i2];

            initOctreeTriangle(tri, scaler, i0, i1, i2, v0, v1, v2);

            //insert
            mOctree->insert(tri);
        }
    }
    else if (mMode == TINYGLTF_MODE_POINTS ||
            mMode == TINYGLTF_MODE_LINE ||
        mMode == TINYGLTF_MODE_LINE_LOOP ||
        mMode == TINYGLTF_MODE_LINE_STRIP)
    {
        // nothing to do, no volume... maybe add some collision geometry around these primitive types?
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
    face.mIndices = nullptr; // unreferenced

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


const Primitive& Primitive::operator=(const tinygltf::Primitive& src)
{
    // load material
    mMaterial = src.material;

    // load mode
    mMode = src.mode;

    // load indices
    mIndices = src.indices;

    // load attributes
    for (auto& it : src.attributes)
    {
        mAttributes[it.first] = it.second;
    }

    switch (mMode)
    {
    case TINYGLTF_MODE_POINTS:
        mGLMode = LLRender::POINTS;
        break;
    case TINYGLTF_MODE_LINE:
        mGLMode = LLRender::LINES;
        break;
    case TINYGLTF_MODE_LINE_LOOP:
        mGLMode = LLRender::LINE_LOOP;
        break;
    case TINYGLTF_MODE_LINE_STRIP:
        mGLMode = LLRender::LINE_STRIP;
        break;
    case TINYGLTF_MODE_TRIANGLES:
        mGLMode = LLRender::TRIANGLES;
        break;
    case TINYGLTF_MODE_TRIANGLE_STRIP:
        mGLMode = LLRender::TRIANGLE_STRIP;
        break;
    case TINYGLTF_MODE_TRIANGLE_FAN:
        mGLMode = LLRender::TRIANGLE_FAN;
        break;
    default:
        mGLMode = GL_TRIANGLES;
    }

    return *this;
}
