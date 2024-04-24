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
#include "../llviewershadermgr.h"
#include "../llviewercontrol.h"

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
        //if (node.mMesh != INVALID_INDEX)
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

void Node::setTranslation(const glh::vec3f& t)
{
    makeTRSValid();
    mTranslation = t;
    mMatrixValid = false;
}

void Node::setScale(const glh::vec3f& s)
{
    makeTRSValid();
    mScale = s;
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
    mSkin = src.skin;
    mName = src.name;

    return *this;
}

void Asset::render(bool opaque, bool rigged)
{
    if (rigged)
    {
        gGL.loadIdentity();
    }

    for (auto& node : mNodes)
    {
        if (node.mSkin != INVALID_INDEX)
        {
            if (rigged)
            {
                Skin& skin = mSkins[node.mSkin];
                skin.uploadMatrixPalette(*this, node);
            }
            else
            {
                //skip static nodes if we're rendering rigged
                continue;
            }
        }
        else if (rigged)
        {
            // skip rigged nodes if we're not rendering rigged
            continue;
        }


        if (node.mMesh != INVALID_INDEX)
        {
            Mesh& mesh = mMeshes[node.mMesh];
            for (auto& primitive : mesh.mPrimitives)
            {
                if (!rigged)
                {
                    gGL.loadMatrix((F32*)node.mRenderMatrix.mMatrix);
                }
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
            static LLCachedControl<U32> anim_idx(gSavedSettings, "GLTFAnimationIndex", 0);
            static LLCachedControl<F32> anim_speed(gSavedSettings, "GLTFAnimationSpeed", 1.f);

            U32 idx = llclamp(anim_idx(), 0U, mAnimations.size() - 1);
            mAnimations[idx].update(*this, dt*anim_speed);
        }

        updateTransforms();
    }
}

void Asset::allocateGLResources(const std::string& filename, const tinygltf::Model& model)
{
    // do images first as materials may depend on images
    for (auto& image : mImages)
    {
        image.allocateGLResources();
    }

    // do materials before meshes as meshes may depend on materials
    for (U32 i = 0; i < mMaterials.size(); ++i)
    {
        mMaterials[i].allocateGLResources(*this);
        LLTinyGLTFHelper::getMaterialFromModel(filename, model, i, mMaterials[i].mMaterial, mMaterials[i].mName, true);
    }

    for (auto& mesh : mMeshes)
    {
        mesh.allocateGLResources(*this);
    }

    for (auto& animation : mAnimations)
    {
        animation.allocateGLResources(*this);
    }

    for (auto& skin : mSkins)
    {
        skin.allocateGLResources(*this);
    }
}

const Asset& Asset::operator=(const tinygltf::Model& src)
{
    mScenes.resize(src.scenes.size());
    for (U32 i = 0; i < src.scenes.size(); ++i)
    {
        mScenes[i] = src.scenes[i];
    }

    mNodes.resize(src.nodes.size());
    for (U32 i = 0; i < src.nodes.size(); ++i)
    {
        mNodes[i] = src.nodes[i];
    }

    mMeshes.resize(src.meshes.size());
    for (U32 i = 0; i < src.meshes.size(); ++i)
    {
        mMeshes[i] = src.meshes[i];
    }

    mMaterials.resize(src.materials.size());
    for (U32 i = 0; i < src.materials.size(); ++i)
    {
        mMaterials[i] = src.materials[i];
    }

    mBuffers.resize(src.buffers.size());
    for (U32 i = 0; i < src.buffers.size(); ++i)
    {
        mBuffers[i] = src.buffers[i];
    }

    mBufferViews.resize(src.bufferViews.size());
    for (U32 i = 0; i < src.bufferViews.size(); ++i)
    {
        mBufferViews[i] = src.bufferViews[i];
    }

    mTextures.resize(src.textures.size());
    for (U32 i = 0; i < src.textures.size(); ++i)
    {
        mTextures[i] = src.textures[i];
    }

    mSamplers.resize(src.samplers.size());
    for (U32 i = 0; i < src.samplers.size(); ++i)
    {
        mSamplers[i] = src.samplers[i];
    }

    mImages.resize(src.images.size());
    for (U32 i = 0; i < src.images.size(); ++i)
    {
        mImages[i] = src.images[i];
    }

    mAccessors.resize(src.accessors.size());
    for (U32 i = 0; i < src.accessors.size(); ++i)
    {
        mAccessors[i] = src.accessors[i];
    }

    mAnimations.resize(src.animations.size());
    for (U32 i = 0; i < src.animations.size(); ++i)
    {
        mAnimations[i] = src.animations[i];
    }

    mSkins.resize(src.skins.size());
    for (U32 i = 0; i < src.skins.size(); ++i)
    {
        mSkins[i] = src.skins[i];
    }
 
    return *this;
}

const Material& Material::operator=(const tinygltf::Material& src)
{
    mName = src.name;
    return *this;
}

void Material::allocateGLResources(Asset& asset)
{
    // allocate material
    mMaterial = new LLFetchedGLTFMaterial();
}

const Mesh& Mesh::operator=(const tinygltf::Mesh& src)
{
    mPrimitives.resize(src.primitives.size());
    for (U32 i = 0; i < src.primitives.size(); ++i)
    {
        mPrimitives[i] = src.primitives[i];
    }

    mWeights = src.weights;
    mName = src.name;

    return *this;
}

void Mesh::allocateGLResources(Asset& asset)
{
    for (auto& primitive : mPrimitives)
    {
        primitive.allocateGLResources(asset);
    }
}

const Scene& Scene::operator=(const tinygltf::Scene& src)
{
    mNodes = src.nodes;
    mName = src.name;

    return *this;
}

const Texture& Texture::operator=(const tinygltf::Texture& src)
{
    mSampler = src.sampler;
    mSource = src.source;
    mName = src.name;

    return *this;
}

const Sampler& Sampler::operator=(const tinygltf::Sampler& src)
{
    mMagFilter = src.magFilter;
    mMinFilter = src.minFilter;
    mWrapS = src.wrapS;
    mWrapT = src.wrapT;
    mName = src.name;

    return *this;
}

void Skin::uploadMatrixPalette(Asset& asset, Node& node)
{
    // prepare matrix palette

    // modelview will be applied by the shader, so assume matrix palette is in asset space
    std::vector<glh::matrix4f> t_mp;

    t_mp.resize(mJoints.size());

    for (U32 i = 0; i < mJoints.size(); ++i)
    {
        Node& joint = asset.mNodes[mJoints[i]];
        
        //t_mp[i].set_value(joint.mRenderMatrix.getF32ptr());
        //t_mp[i] = t_mp[i] * mInverseBindMatricesData[i];

        //t_mp[i].set_value(joint.mRenderMatrix.getF32ptr());
        //t_mp[i] = mInverseBindMatricesData[i] * t_mp[i];

        t_mp[i].set_value(joint.mRenderMatrix.getF32ptr());
        t_mp[i] = t_mp[i] * mInverseBindMatricesData[i];

    }

    std::vector<F32> glmp;

    glmp.resize(mJoints.size() * 12);

    F32* mp = glmp.data();

    for (U32 i = 0; i < mJoints.size(); ++i)
    {
        F32* m = (F32*)t_mp[i].m;

        U32 idx = i * 12;

        mp[idx + 0] = m[0];
        mp[idx + 1] = m[1];
        mp[idx + 2] = m[2];
        mp[idx + 3] = m[12];

        mp[idx + 4] = m[4];
        mp[idx + 5] = m[5];
        mp[idx + 6] = m[6];
        mp[idx + 7] = m[13];

        mp[idx + 8] = m[8];
        mp[idx + 9] = m[9];
        mp[idx + 10] = m[10];
        mp[idx + 11] = m[14];
    }

    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX,
        mJoints.size(),
        FALSE,
        (GLfloat*)glmp.data());
}

