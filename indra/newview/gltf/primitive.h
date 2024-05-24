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
#include "boost/json.hpp"

// LL GLTF Implementation
namespace LL
{
    namespace GLTF
    {
        using Value = boost::json::value;
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
            enum class Mode : U8
            {
                POINTS,
                LINES,
                LINE_LOOP,
                LINE_STRIP,
                TRIANGLES,
                TRIANGLE_STRIP,
                TRIANGLE_FAN
            };

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
            Mode mMode = Mode::TRIANGLES; // default to triangles
            LLRender::eGeomModes mGLMode = LLRender::TRIANGLES; // for use with LLRender
            S32 mIndices = -1;
            std::unordered_map<std::string, S32> mAttributes;

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

            void serialize(boost::json::object& obj) const;
            const Primitive& operator=(const Value& src);

            bool prep(Asset& asset);
        };
    }
}
