#pragma once

/**
 * @file primitive.h
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

#include "llvertexbuffer.h"
#include "llvolumeoctree.h"

// LL GLTF Implementation
namespace LL
{
    namespace GLTF
    {
        class Asset;

        constexpr U32 ATTRIBUTE_MASK =
            LLVertexBuffer::MAP_VERTEX |
            LLVertexBuffer::MAP_NORMAL |
            LLVertexBuffer::MAP_TEXCOORD0 |
            LLVertexBuffer::MAP_TANGENT |
            LLVertexBuffer::MAP_COLOR;

        class Primitive
        {
        public:
            ~Primitive();

            // GPU copy of mesh data
            LLPointer<LLVertexBuffer> mVertexBuffer;

            // CPU copy of mesh data
            std::vector<LLVector2> mTexCoords;
            std::vector<LLVector4a> mNormals;
            std::vector<LLVector4a> mTangents;
            std::vector<LLVector4a> mPositions;
            std::vector<LLVector4a> mJoints;
            std::vector<LLVector4a> mWeights;
            std::vector<LLColor4U> mColors;
            std::vector<U32> mIndexArray;

            // raycast acceleration structure
            LLPointer<LLVolumeOctree> mOctree;
            std::vector<LLVolumeTriangle> mOctreeTriangles;

            S32 mMaterial = -1;
            U32 mMode = TINYGLTF_MODE_TRIANGLES; // default to triangles
            U32 mGLMode = LLRender::TRIANGLES;
            S32 mIndices = -1;
            std::unordered_map<std::string, int> mAttributes;

            // create octree based on vertex buffer
            // must be called before buffer is unmapped and after buffer is populated with good data
            void createOctree();

            //get the LLVolumeTriangle that intersects with the given line segment at the point
            //closest to start.  Moves end to the point of intersection.  Returns nullptr if no intersection.
            //Line segment must be in the same coordinate frame as this Primitive
            const LLVolumeTriangle* lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
                LLVector4a* intersection = NULL,         // return the intersection point
                LLVector2* tex_coord = NULL,            // return the texture coordinates of the intersection point
                LLVector4a* normal = NULL,               // return the surface normal at the intersection point
                LLVector4a* tangent = NULL             // return the surface tangent at the intersection point
            );

            const Primitive& operator=(const tinygltf::Primitive& src);

            void allocateGLResources(Asset& asset);
        };
    }
}
