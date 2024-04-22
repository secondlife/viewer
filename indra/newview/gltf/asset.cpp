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
    makeMatrixValid();
    matMul(mMatrix, parentMatrix, mAssetMatrix);
    mAssetMatrixInv = inverse(mAssetMatrix);
    
    S32 my_index = this - &asset.mNodes[0];

    for (auto& childIndex : mChildren)
    {
        Node& child = asset.mNodes[childIndex];
        child.mParent = my_index;
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


void Node::makeMatrixValid()
{
    if (!mMatrixValid && mTRSValid)
    {
        glh::matrix4f rot;
        mRotation.get_value(rot);

        glh::matrix4f trans;
        trans.set_translate(mTranslation);

        glh::matrix4f sc;
        sc.set_scale(mScale);

        glh::matrix4f t;
        //t = sc * rot * trans; 
        //t = trans * rot * sc; // best so far, still wrong on negative scale
        //t = sc * trans * rot;
        t = trans * sc * rot;

        mMatrix.loadu(t.m);
        mMatrixValid = true;
    }
}

void Node::makeTRSValid()
{
    if (!mTRSValid && mMatrixValid)
    {
        glh::matrix4f t(mMatrix.getF32ptr());

        glh::vec4f p = t.get_column(3);
        mTranslation.set_value(p.v[0], p.v[1], p.v[2]);

        mScale.set_value(t.get_column(0).length(), t.get_column(1).length(), t.get_column(2).length());
        mRotation.set_value(t);
        mTRSValid = true;
    }
}

void Node::setRotation(const glh::quaternionf& q)
{
    makeTRSValid();
    mRotation = q;
    mMatrixValid = false;
}

const Node& Node::operator=(const tinygltf::Node& src)
{
    F32* dstMatrix = mMatrix.getF32ptr();

    if (src.matrix.size() == 16)
    {
        // Node has a transformation matrix, just copy it
        for (U32 i = 0; i < 16; ++i)
        {
            dstMatrix[i] = (F32)src.matrix[i];
        }

        mMatrixValid = true;
    }
    else if (!src.rotation.empty() || !src.translation.empty() || !src.scale.empty())
    {
        // node has rotation/translation/scale, convert to matrix
        if (src.rotation.size() == 4)
        {
            mRotation = glh::quaternionf((F32)src.rotation[0], (F32)src.rotation[1], (F32)src.rotation[2], (F32)src.rotation[3]);
        }

        if (src.translation.size() == 3)
        {
            mTranslation = glh::vec3f((F32)src.translation[0], (F32)src.translation[1], (F32)src.translation[2]);
        }

        glh::vec3f scale;
        if (src.scale.size() == 3)
        {
            mScale = glh::vec3f((F32)src.scale[0], (F32)src.scale[1], (F32)src.scale[2]);
        }
        else
        {
            mScale.set_value(1.f, 1.f, 1.f);
        }

        mTRSValid = true;
    }
    else
    {
        // node specifies no transformation, set to identity
        mMatrix.setIdentity();
    }

    mChildren = src.children;
    mMesh = src.mesh;
    mName = src.name;

    return *this;
}

void Asset::render(bool opaque)
{
    for (auto& node : mNodes)
    {
        if (node.mMesh != INVALID_INDEX)
        {
            Mesh& mesh = mMeshes[node.mMesh];
            for (auto& primitive : mesh.mPrimitives)
            {
                gGL.loadMatrix((F32*)node.mRenderMatrix.mMatrix);
                bool cull = true;
                if (primitive.mMaterial != INVALID_INDEX)
                {
                    Material& material = mMaterials[primitive.mMaterial];

                    if ((material.mMaterial->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_BLEND) == opaque)
                    {
                        continue;
                    }
                    material.mMaterial->bind();
                    cull = !material.mMaterial->mDoubleSided;
                }
                else
                {
                    if (!opaque)
                    {
                        continue;
                    }
                    LLFetchedGLTFMaterial::sDefault.bind();
                }

                LLGLDisable cull_face(!cull ? GL_CULL_FACE : 0);

                primitive.mVertexBuffer->setBuffer();
                if (primitive.mVertexBuffer->getNumIndices() > 0)
                {
                    primitive.mVertexBuffer->draw(primitive.mGLMode, primitive.mVertexBuffer->getNumIndices(), 0);
                }
                else
                {
                    primitive.mVertexBuffer->drawArrays(primitive.mGLMode, 0, primitive.mVertexBuffer->getNumVerts());
                }

            }
        }
    }
}

void Asset::renderOpaque()
{
    render(true);
}

void Asset::renderTransparent()
{
    render(false);
}

void Asset::update()
{
    F32 dt = gFrameTimeSeconds - mLastUpdateTime;

    if (dt > 0.f)
    {
        mLastUpdateTime = gFrameTimeSeconds;
        if (mAnimations.size() > 0)
        {
            mAnimations[0].update(*this, dt);
        }

        updateTransforms();
    }
}
