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
#include "../llviewershadermgr.h"

#include "mikktspace/mikktspace.hh"

#include "meshoptimizer/meshoptimizer.h"


using namespace LL::GLTF;
using namespace boost::json;


// Mesh data useful for Mikktspace tangent generation (and flat normal generation)
struct MikktMesh
{
    std::vector<LLVector3> p;
    std::vector<LLVector3> n;
    std::vector<LLVector2> tc;
    std::vector<LLVector4> w;
    std::vector<LLVector4> t;
    std::vector<LLColor4U> c;
    std::vector<U64> j;

    // initialize from src primitive and make an unrolled triangle list
    // returns false if the Primitive cannot be converted to a triangle list
    bool copy(const Primitive* prim)
    {
        bool indexed = !prim->mIndexArray.empty();
        U32 vert_count = indexed ? prim->mIndexArray.size() : prim->mPositions.size();

        if (prim->mMode != Primitive::Mode::TRIANGLES)
        {
            LL_WARNS("GLTF") << "Unsupported primitive mode for conversion to triangles: " << (S32) prim->mMode << LL_ENDL;
            return false;
        }

        p.resize(vert_count);
        n.resize(vert_count);
        tc.resize(vert_count);
        c.resize(vert_count);

        bool has_normals = !prim->mNormals.empty();
        if (has_normals)
        {
            n.resize(vert_count);
        }
        bool has_tangents = !prim->mTangents.empty();
        if (has_tangents)
        {
            t.resize(vert_count);
        }
        bool rigged = !prim->mWeights.empty();
        if (rigged)
        {
            w.resize(vert_count);
            j.resize(vert_count);
        }

        for (int i = 0; i < vert_count; ++i)
        {
            U32 idx = indexed ? prim->mIndexArray[i] : i;

            p[i].set(prim->mPositions[idx].getF32ptr());
            tc[i].set(prim->mTexCoords[idx]);
            c[i] = prim->mColors[idx];

            if (has_normals)
            {
                n[i].set(prim->mNormals[idx].getF32ptr());
            }

            if (rigged)
            {
                w[i].set(prim->mWeights[idx].getF32ptr());
                j[i] = prim->mJoints[idx];
            }
        }

        return true;
    }

    void genNormals()
    {
        U32 tri_count = p.size() / 3;
        for (U32 i = 0; i < tri_count; ++i)
        {
            LLVector3 v0 = p[i * 3];
            LLVector3 v1 = p[i * 3 + 1];
            LLVector3 v2 = p[i * 3 + 2];

            LLVector3 normal = (v1 - v0) % (v2 - v0);
            normal.normalize();

            n[i * 3] = normal;
            n[i * 3 + 1] = normal;
            n[i * 3 + 2] = normal;
        }
    }

    void genTangents()
    {
        t.resize(p.size());
        mikk::Mikktspace ctx(*this);
        ctx.genTangSpace();
    }

    // write to target primitive as an indexed triangle list
    // Only modifies runtime data, does not modify the original GLTF data
    void write(Primitive* prim) const
    {
        //re-weld
        meshopt_Stream mos[] =
        {
            { &p[0], sizeof(LLVector3), sizeof(LLVector3) },
            { &n[0], sizeof(LLVector3), sizeof(LLVector3) },
            { &t[0], sizeof(LLVector4), sizeof(LLVector4) },
            { &tc[0], sizeof(LLVector2), sizeof(LLVector2) },
            { &c[0], sizeof(LLColor4U), sizeof(LLColor4U) },
            { w.empty() ? nullptr : &w[0], sizeof(LLVector4), sizeof(LLVector4) },
            { j.empty() ? nullptr : &j[0], sizeof(U64), sizeof(U64) }
        };

        std::vector<U32> remap;
        remap.resize(p.size());

        U32 stream_count = w.empty() ? 5 : 7;

        size_t vert_count = meshopt_generateVertexRemapMulti(&remap[0], nullptr, p.size(), p.size(), mos, stream_count);

        prim->mTexCoords.resize(vert_count);
        prim->mNormals.resize(vert_count);
        prim->mTangents.resize(vert_count);
        prim->mPositions.resize(vert_count);
        prim->mColors.resize(vert_count);
        if (!w.empty())
        {
            prim->mWeights.resize(vert_count);
            prim->mJoints.resize(vert_count);
        }
        
        prim->mIndexArray.resize(remap.size());

        for (int i = 0; i < remap.size(); ++i)
        {
            U32 src_idx = i;
            U32 dst_idx = remap[i];

            prim->mIndexArray[i] = dst_idx;

            prim->mPositions[dst_idx].load3(p[src_idx].mV);
            prim->mNormals[dst_idx].load3(n[src_idx].mV);
            prim->mTexCoords[dst_idx] = tc[src_idx];
            prim->mTangents[dst_idx].loadua(t[src_idx].mV);
            prim->mColors[dst_idx] = c[src_idx];

            if (!w.empty())
            {
                prim->mWeights[dst_idx].loadua(w[src_idx].mV);
                prim->mJoints[dst_idx] = j[src_idx];
            }
        }

        prim->mGLMode = LLRender::TRIANGLES;
    }

    uint32_t GetNumFaces()
    {
        return uint32_t(p.size()/3);
    }

    uint32_t GetNumVerticesOfFace(const uint32_t face_num)
    {
        return 3;
    }

    mikk::float3 GetPosition(const uint32_t face_num, const uint32_t vert_num)
    {
        F32* v = p[face_num * 3 + vert_num].mV;
        return mikk::float3(v);
    }

    mikk::float3 GetTexCoord(const uint32_t face_num, const uint32_t vert_num)
    {
        F32* uv = tc[face_num * 3 + vert_num].mV;
        return mikk::float3(uv[0], uv[1], 1.0f);
    }

    mikk::float3 GetNormal(const uint32_t face_num, const uint32_t vert_num)
    {
        F32* normal = n[face_num * 3 + vert_num].mV;
        return mikk::float3(normal);
    }

    void SetTangentSpace(const uint32_t face_num, const uint32_t vert_num, mikk::float3 T, bool orientation)
    {
        S32 i = face_num * 3 + vert_num;
        t[i].set(T.x, T.y, T.z, orientation ? 1.0f : -1.0f);
    }
};


bool Primitive::prep(Asset& asset)
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

        for (auto& idx : mIndexArray)
        {
            if (idx >= mPositions.size())
            {
                LL_WARNS("GLTF") << "Invalid index array" << LL_ENDL;
                return false;
            }
        }
    }

    U32 mask = LLVertexBuffer::MAP_VERTEX;

    if (!mWeights.empty())
    {
        mask |= LLVertexBuffer::MAP_WEIGHT4;
        mask |= LLVertexBuffer::MAP_JOINT;
    }

    if (mTexCoords.empty())
    {
        mTexCoords.resize(mPositions.size());
    }

    // TODO: support more than one texcoord set (or no texcoords)
    mask |= LLVertexBuffer::MAP_TEXCOORD0;

    if (mColors.empty())
    {
        mColors.resize(mPositions.size(), LLColor4U::white);
    }

    // TODO: support colorless vertex buffers
    mask |= LLVertexBuffer::MAP_COLOR;

    mShaderVariant = 0;

    bool unlit = false;

    // bake material basecolor into color array
    if (mMaterial != INVALID_INDEX)
    {
        const Material& material = asset.mMaterials[mMaterial];
        LLColor4 baseColor(glm::value_ptr(material.mPbrMetallicRoughness.mBaseColorFactor));
        for (auto& dst : mColors)
        {
            dst = LLColor4U(baseColor * LLColor4(dst));
        }

        if (material.mUnlit.mPresent)
        { // material uses KHR_materials_unlit
            mShaderVariant |= LLGLSLShader::GLTFVariant::UNLIT;
            unlit = true;
        }
    }

    if (mNormals.empty() && !unlit)
    {
        mTangents.clear();

        if (mMode == Mode::POINTS || mMode == Mode::LINES || mMode == Mode::LINE_LOOP || mMode == Mode::LINE_STRIP)
        { //no normals and no surfaces, this primitive is unlit
            mTangents.clear();
            mShaderVariant |= LLGLSLShader::GLTFVariant::UNLIT;
            unlit = true;
        }
        else
        {
            // unroll into non-indexed array of flat shaded triangles
            MikktMesh data;
            if (!data.copy(this))
            {
                return false;
            }

            data.genNormals();
            data.genTangents();
            data.write(this);
        }
    }

    if (mTangents.empty() && !unlit)
    { // NOTE: must be done last because tangent generation rewrites the other arrays
        // adapted from usage of Mikktspace in llvolume.cpp
        if (mMode == Mode::POINTS || mMode == Mode::LINES || mMode == Mode::LINE_LOOP || mMode == Mode::LINE_STRIP)
        {
            // for points and lines, just make sure tangent is perpendicular to normal
            mTangents.resize(mNormals.size());
            LLVector4a up(0.f, 0.f, 1.f, 0.f);
            LLVector4a left(1.f, 0.f, 0.f, 0.f);
            for (U32 i = 0; i < mNormals.size(); ++i)
            {
                if (fabsf(mNormals[i].getF32ptr()[2]) < 0.999f)
                {
                    mTangents[i] = up.cross3(mNormals[i]);
                }
                else
                {
                    mTangents[i] = left.cross3(mNormals[i]);
                }

                mTangents[i].getF32ptr()[3] = 1.f;
            }
        }
        else
        {
            MikktMesh data;
            if (!data.copy(this))
            {
                return false;
            }

            data.genTangents();
            data.write(this);
        }
    }

    if (!mNormals.empty())
    {
        mask |= LLVertexBuffer::MAP_NORMAL;
    }

    if (!mTangents.empty())
    {
        mask |= LLVertexBuffer::MAP_TANGENT;
    }

    if (LLGLSLShader::sCurBoundShaderPtr == nullptr)
    { // make sure a shader is bound to satisfy mVertexBuffer->setBuffer
        gDebugProgram.bind();
    }

    mVertexBuffer = new LLVertexBuffer(mask);
    mVertexBuffer->allocateBuffer(mPositions.size(), mIndexArray.size() * 2); // double the size of the index buffer for 32-bit indices

    mVertexBuffer->setBuffer();
    mVertexBuffer->setPositionData(mPositions.data());
    mVertexBuffer->setColorData(mColors.data());

    if (!mNormals.empty())
    {
        mVertexBuffer->setNormalData(mNormals.data());
    }
    if (!mTangents.empty())
    {
        mVertexBuffer->setTangentData(mTangents.data());
    }

    if (!mWeights.empty())
    {
        mShaderVariant |= LLGLSLShader::GLTFVariant::RIGGED;
        mVertexBuffer->setWeight4Data(mWeights.data());
        mVertexBuffer->setJointData(mJoints.data());
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

    if (!mIndexArray.empty())
    {
        mVertexBuffer->setIndexData(mIndexArray.data());
    }

    createOctree();

    mVertexBuffer->unbind();

    Material& material = asset.mMaterials[mMaterial];
    if (material.mAlphaMode == Material::AlphaMode::BLEND)
    {
        mShaderVariant |= LLGLSLShader::GLTFVariant::ALPHA_BLEND;
    }

    return true;
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

    if (mMode == Mode::TRIANGLES)
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
    else if (mMode == Mode::TRIANGLE_STRIP)
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
    else if (mMode == Mode::TRIANGLE_FAN)
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
    else if (mMode == Mode::POINTS ||
            mMode == Mode::LINES ||
        mMode == Mode::LINE_LOOP ||
        mMode == Mode::LINE_STRIP)
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

LLRender::eGeomModes gltf_mode_to_gl_mode(Primitive::Mode mode)
{
    switch (mode)
    {
    case Primitive::Mode::POINTS:
        return LLRender::POINTS;
    case Primitive::Mode::LINES:
        return LLRender::LINES;
    case Primitive::Mode::LINE_LOOP:
        return LLRender::LINE_LOOP;
    case Primitive::Mode::LINE_STRIP:
        return LLRender::LINE_STRIP;
    case Primitive::Mode::TRIANGLES:
        return LLRender::TRIANGLES;
    case Primitive::Mode::TRIANGLE_STRIP:
        return LLRender::TRIANGLE_STRIP;
    case Primitive::Mode::TRIANGLE_FAN:
        return LLRender::TRIANGLE_FAN;
    default:
        return LLRender::TRIANGLES;
    }
}

void Primitive::serialize(boost::json::object& dst) const
{
    write(mMaterial, "material", dst, -1);
    write(mMode, "mode", dst, Primitive::Mode::TRIANGLES);
    write(mIndices, "indices", dst, INVALID_INDEX);
    write(mAttributes, "attributes", dst);
}

const Primitive& Primitive::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "material", mMaterial);
        copy(src, "mode", mMode);
        copy(src, "indices", mIndices);
        copy(src, "attributes", mAttributes);

        mGLMode = gltf_mode_to_gl_mode(mMode);
    }
    return *this;
}

