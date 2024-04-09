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

void Scene::updateTransforms(Asset& asset)
{
    LLMatrix4a identity;
    identity.setIdentity();
    for (auto& nodeIndex : mNodes)
    {
        Node& node = asset.mNodes[nodeIndex];
        node.updateTransforms(asset, identity);
    }
}

void Scene::updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview)
{
    for (auto& nodeIndex : mNodes)
    {
        Node& node = asset.mNodes[nodeIndex];
        node.updateRenderTransforms(asset, modelview);
    }
}

void Node::updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview)
{
    matMul(mMatrix, modelview, mRenderMatrix);

    for (auto& childIndex : mChildren)
    {
        Node& child = asset.mNodes[childIndex];
        child.updateRenderTransforms(asset, mRenderMatrix);
    }
}

void Node::updateTransforms(Asset& asset, const LLMatrix4a& parentMatrix)
{
    matMul(mMatrix, parentMatrix, mAssetMatrix);

    glh::matrix4f m((F32*)&mAssetMatrix);
    m = m.inverse();
    mAssetMatrixInv.loadu((F32*)m.m);

    for (auto& childIndex : mChildren)
    {
        Node& child = asset.mNodes[childIndex];
        child.updateTransforms(asset, mAssetMatrix);
    }
}

void Asset::updateTransforms()
{
    for (auto& scene : mScenes)
    {
        scene.updateTransforms(*this);
    }
}

void Asset::updateRenderTransforms(const LLMatrix4a& modelview)
{
#if 0
    // traverse hierarchy and update render transforms from scratch
    for (auto& scene : mScenes)
    {
        scene.updateRenderTransforms(*this, modelview);
    }
#else
    // use mAssetMatrix to update render transforms from node list
    for (auto& node : mNodes)
    {
        if (node.mMesh != INVALID_INDEX)
        {
            matMul(node.mAssetMatrix, modelview, node.mRenderMatrix);
        }
    }

#endif

}

S32 Asset::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
    LLVector4a* intersection,         // return the intersection point
    LLVector2* tex_coord,            // return the texture coordinates of the intersection point
    LLVector4a* normal,               // return the surface normal at the intersection point
    LLVector4a* tangent             // return the surface tangent at the intersection point
)
{
    S32 hit = -1;
    for (auto& node : mNodes)
    {
        if (node.mMesh != INVALID_INDEX)
        {
            LLVector4a local_start;
            LLVector4a local_end;

            bool newHit = false;

            // transform start and end to this node's local space
            node.mAssetMatrixInv.affineTransform(start, local_start);
            node.mAssetMatrixInv.affineTransform(end, local_end);

            Mesh& mesh = mMeshes[node.mMesh];
            for (auto& primitive : mesh.mPrimitives)
            {
                const LLVolumeTriangle* tri = primitive.lineSegmentIntersect(start, end, intersection, tex_coord, normal, tangent);
                if (tri)
                {
                    newHit = true;
                    // pointer math to get the node index
                    hit = &node - &mNodes[0];
                }
            }

            if (newHit)
            {
                // transform results back to asset space
                if (intersection)
                {
                    node.mAssetMatrix.affineTransform(*intersection, *intersection);
                }

                if (normal || tangent)
                {
                    LLMatrix4 normalMatrix(node.mAssetMatrixInv.getF32ptr());

                    normalMatrix.transpose();

                    LLMatrix4a norm_mat;
                    norm_mat.loadu((F32*)normalMatrix.mMatrix);

                    if (normal)
                    {
                        LLVector4a n = *normal;
                        F32 w = n.getF32ptr()[3];
                        n.getF32ptr()[3] = 0.0f;

                        norm_mat.affineTransform(n, *normal);
                        normal->getF32ptr()[3] = w;
                    }

                    if (tangent)
                    {
                        LLVector4a t = *tangent;
                        F32 w = t.getF32ptr()[3];
                        t.getF32ptr()[3] = 0.0f;

                        norm_mat.affineTransform(t, *tangent);
                        tangent->getF32ptr()[3] = w;
                    }
                }
            }
        }
    }
    return hit;
}


