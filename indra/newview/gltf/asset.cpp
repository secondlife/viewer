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

#include "../llviewerprecompiledheaders.h"

#include "asset.h"
#include "llvolumeoctree.h"

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

LLMatrix4a inverse(const LLMatrix4a& mat);

void Node::updateTransforms(Asset& asset, const LLMatrix4a& parentMatrix)
{
    matMul(mMatrix, parentMatrix, mAssetMatrix);
    mAssetMatrixInv = inverse(mAssetMatrix);
    
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
    LLVector4a* tangent,             // return the surface tangent at the intersection point
    S32* primitive_hitp
)
{
    S32 node_hit = -1;
    S32 primitive_hit = -1;

    LLVector4a local_start;
    LLVector4a asset_end = end;
    LLVector4a local_end;
    LLVector4a p;


    for (auto& node : mNodes)
    {
        if (node.mMesh != INVALID_INDEX)
        {
            
            bool newHit = false;

            // transform start and end to this node's local space
            node.mAssetMatrixInv.affineTransform(start, local_start);
            node.mAssetMatrixInv.affineTransform(asset_end, local_end);

            Mesh& mesh = mMeshes[node.mMesh];
            for (auto& primitive : mesh.mPrimitives)
            {
                const LLVolumeTriangle* tri = primitive.lineSegmentIntersect(local_start, local_end, &p, tex_coord, normal, tangent);
                if (tri)
                {
                    newHit = true;
                    local_end = p;

                    // pointer math to get the node index
                    node_hit = &node - &mNodes[0];
                    llassert(&mNodes[node_hit] == &node);

                    //pointer math to get the primitive index
                    primitive_hit = &primitive - &mesh.mPrimitives[0];
                    llassert(&mesh.mPrimitives[primitive_hit] == &primitive);
                }
            }

            if (newHit)
            {
                // shorten line segment on hit
                node.mAssetMatrix.affineTransform(p, asset_end); 

                // transform results back to asset space
                if (intersection)
                {
                    *intersection = asset_end;
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

    if (node_hit != -1)
    {
        if (primitive_hitp)
        {
            *primitive_hitp = primitive_hit;
        }
    }

    return node_hit;
}


