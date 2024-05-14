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
#include "../llviewertexturelist.h"

using namespace LL::GLTF;

namespace LL
{
    namespace GLTF
    {
        template <typename T, typename U>
        void copy(const std::vector<T>& src, std::vector<U>& dst)
        {
            dst.resize(src.size());
            for (U32 i = 0; i < src.size(); ++i)
            {
                copy(src[i], dst[i]);
            }
        }

        void copy(const Node& src, tinygltf::Node& dst)
        {
            if (src.mMatrixValid)
            {
                if (!src.mMatrix.asMatrix4().isIdentity())
                {
                    dst.matrix.resize(16);
                    for (U32 i = 0; i < 16; ++i)
                    {
                        dst.matrix[i] = src.mMatrix.getF32ptr()[i];
                    }
                }
            }
            else if (src.mTRSValid)
            {
                if (!src.mRotation.equals(glh::quaternionf::identity(), FLT_EPSILON))
                {
                    dst.rotation.resize(4);
                    for (U32 i = 0; i < 4; ++i)
                    {
                        dst.rotation[i] = src.mRotation.get_value()[i];
                    }
                }   
                
                if (src.mTranslation != glh::vec3f(0.f, 0.f, 0.f))
                {
                    dst.translation.resize(3);
                    for (U32 i = 0; i < 3; ++i)
                    {
                        dst.translation[i] = src.mTranslation.v[i];
                    }
                }
                
                if (src.mScale != glh::vec3f(1.f, 1.f, 1.f))
                {
                    dst.scale.resize(3);
                    for (U32 i = 0; i < 3; ++i)
                    {
                        dst.scale[i] = src.mScale.v[i];
                    }
                }
            }

            dst.children = src.mChildren;
            dst.mesh = src.mMesh;
            dst.skin = src.mSkin;
            dst.name = src.mName;
        }

        void copy(const Scene& src, tinygltf::Scene& dst)
        {
            dst.nodes = src.mNodes;
            dst.name = src.mName;
        }

        void copy(const Primitive& src, tinygltf::Primitive& dst)
        {
            for (auto& attrib : src.mAttributes)
            {
                dst.attributes[attrib.first] = attrib.second;
            }
            dst.indices = src.mIndices;
            dst.material = src.mMaterial;
            dst.mode = src.mMode;
        }

        void copy(const Mesh& src, tinygltf::Mesh& mesh)
        {
            copy(src.mPrimitives, mesh.primitives);
            mesh.weights = src.mWeights;
            mesh.name = src.mName;
        }

        void copy(const Material::TextureInfo& src, tinygltf::TextureInfo& dst)
        {
            dst.index = src.mIndex;
            dst.texCoord = src.mTexCoord;
        }

        void copy(const Material::OcclusionTextureInfo& src, tinygltf::OcclusionTextureInfo& dst)
        {
            dst.index = src.mIndex;
            dst.texCoord = src.mTexCoord;
            dst.strength = src.mStrength;
        }

        void copy(const Material::NormalTextureInfo& src, tinygltf::NormalTextureInfo& dst)
        {
            dst.index = src.mIndex;
            dst.texCoord = src.mTexCoord;
            dst.scale = src.mScale;
        }

        void copy(const Material::PbrMetallicRoughness& src, tinygltf::PbrMetallicRoughness& dst)
        {
            dst.baseColorFactor = { src.mBaseColorFactor.v[0], src.mBaseColorFactor.v[1], src.mBaseColorFactor.v[2], src.mBaseColorFactor.v[3] };
            copy(src.mBaseColorTexture, dst.baseColorTexture);
            dst.metallicFactor = src.mMetallicFactor;
            dst.roughnessFactor = src.mRoughnessFactor;
            copy(src.mMetallicRoughnessTexture, dst.metallicRoughnessTexture);
        }

        void copy(const Material& src, tinygltf::Material& material)
        {
            material.name = src.mName;

            material.emissiveFactor = { src.mEmissiveFactor.v[0], src.mEmissiveFactor.v[1], src.mEmissiveFactor.v[2] };
            copy(src.mPbrMetallicRoughness, material.pbrMetallicRoughness);
            copy(src.mNormalTexture, material.normalTexture);
            copy(src.mEmissiveTexture, material.emissiveTexture);
        }

        void copy(const Texture& src, tinygltf::Texture& texture)
        {
            texture.sampler = src.mSampler;
            texture.source = src.mSource;
            texture.name = src.mName;
        }

        void copy(const Sampler& src, tinygltf::Sampler& sampler)
        {
            sampler.magFilter = src.mMagFilter;
            sampler.minFilter = src.mMinFilter;
            sampler.wrapS = src.mWrapS;
            sampler.wrapT = src.mWrapT;
            sampler.name = src.mName;
        }

        void copy(const Skin& src, tinygltf::Skin& skin)
        {
            skin.joints = src.mJoints;
            skin.inverseBindMatrices = src.mInverseBindMatrices;
            skin.skeleton = src.mSkeleton;
            skin.name = src.mName;
        }

        void copy(const Accessor& src, tinygltf::Accessor& accessor)
        {
            accessor.bufferView = src.mBufferView;
            accessor.byteOffset = src.mByteOffset;
            accessor.componentType = src.mComponentType;
            accessor.minValues = src.mMin;
            accessor.maxValues = src.mMax;
            
            accessor.count = src.mCount;
            accessor.type = src.mType;
            accessor.normalized = src.mNormalized;
            accessor.name = src.mName;
        }

        void copy(const Animation::Sampler& src, tinygltf::AnimationSampler& sampler)
        {
            sampler.input = src.mInput;
            sampler.output = src.mOutput;
            sampler.interpolation = src.mInterpolation;
        }

        void copy(const Animation::Channel& src, tinygltf::AnimationChannel& channel)
        {
            channel.sampler = src.mSampler;
            channel.target_node = src.mTarget.mNode;
            channel.target_path = src.mTarget.mPath;
        }

        void copy(const Animation& src, tinygltf::Animation& animation)
        {
            animation.name = src.mName;

            copy(src.mSamplers, animation.samplers);

            U32 channel_count = src.mRotationChannels.size() + src.mTranslationChannels.size() + src.mScaleChannels.size();

            animation.channels.resize(channel_count);

            U32 idx = 0;
            for (U32 i = 0; i < src.mTranslationChannels.size(); ++i)
            {
                copy(src.mTranslationChannels[i], animation.channels[idx++]);
            }

            for (U32 i = 0; i < src.mRotationChannels.size(); ++i)
            {
                copy(src.mRotationChannels[i], animation.channels[idx++]);
            }

            for (U32 i = 0; i < src.mScaleChannels.size(); ++i)
            {
                copy(src.mScaleChannels[i], animation.channels[idx++]);
            }
        }

        void copy(const Buffer& src, tinygltf::Buffer& buffer)
        {
            buffer.uri = src.mUri;
            buffer.data = src.mData;
            buffer.name = src.mName;
        }

        void copy(const BufferView& src, tinygltf::BufferView& bufferView)
        {
            bufferView.buffer = src.mBuffer;
            bufferView.byteOffset = src.mByteOffset;
            bufferView.byteLength = src.mByteLength;
            bufferView.byteStride = src.mByteStride;
            bufferView.target = src.mTarget;
            bufferView.name = src.mName;
        }

        void copy(const Image& src, tinygltf::Image& image)
        {
            image.name = src.mName;
            image.width = src.mWidth;
            image.height = src.mHeight;
            image.component = src.mComponent;
            image.bits = src.mBits;
            image.pixel_type = src.mPixelType;

            image.image = src.mData;
            image.bufferView = src.mBufferView;
            image.mimeType = src.mMimeType;
            image.uri = src.mUri;
        }

        void copy(const Asset & src, tinygltf::Model& dst)
        {
            dst.defaultScene = src.mDefaultScene;
            dst.asset.copyright = src.mCopyright;
            dst.asset.version = src.mVersion;
            dst.asset.minVersion = src.mMinVersion;
            dst.asset.generator = "Linden Lab Experimental GLTF Export";
            dst.asset.extras = src.mExtras;

            copy(src.mScenes, dst.scenes);
            copy(src.mNodes, dst.nodes);
            copy(src.mMeshes, dst.meshes);
            copy(src.mMaterials, dst.materials);
            copy(src.mBuffers, dst.buffers);
            copy(src.mBufferViews, dst.bufferViews);
            copy(src.mTextures, dst.textures);
            copy(src.mSamplers, dst.samplers);
            copy(src.mImages, dst.images);
            copy(src.mAccessors, dst.accessors);
            copy(src.mAnimations, dst.animations);
            copy(src.mSkins, dst.skins);
        }
    }
}
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

    llassert(mMatrixValid);
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

    llassert(mTRSValid);
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
        mMatrixValid = true;
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
    mVersion = src.asset.version;
    mMinVersion = src.asset.minVersion;
    mGenerator = src.asset.generator;
    mCopyright = src.asset.copyright;
    mExtras = src.asset.extras;

    mDefaultScene = src.defaultScene;
    

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

void Asset::save(tinygltf::Model& dst)
{
    LL::GLTF::copy(*this, dst);
}

void Asset::decompose(const std::string& filename)
{
    // get folder path
    std::string folder = gDirUtilp->getDirName(filename);

    // decompose images
    for (auto& image : mImages)
    {
        image.decompose(*this, folder);
    }
}

void Asset::eraseBufferView(S32 bufferView)
{
    mBufferViews.erase(mBufferViews.begin() + bufferView);

    for (auto& accessor : mAccessors)
    {
        if (accessor.mBufferView > bufferView)
        {
            accessor.mBufferView--;
        }
    }

    for (auto& image : mImages)
    {
        if (image.mBufferView > bufferView)
        {
            image.mBufferView--;
        }
    }

}

void Image::decompose(Asset& asset, const std::string& folder)
{
    std::string name = mName;
    if (name.empty())
    {
        S32 idx = this - asset.mImages.data();
        name = llformat("image_%d", idx);
    }

    if (mBufferView != INVALID_INDEX)
    {
        // save original image
        BufferView& bufferView = asset.mBufferViews[mBufferView];
        Buffer& buffer = asset.mBuffers[bufferView.mBuffer];
        
        std::string extension;

        if (mMimeType == "image/jpeg")
        {
            extension = ".jpg";
        }
        else if (mMimeType == "image/png")
        {
            extension = ".png";
        }
        else
        {
            extension = ".bin";
        }

        std::string filename = folder + "/" + name + "." + extension;

        // set URI to non-j2c file for now, but later we'll want to reference the j2c hash
        mUri = name + "." + extension;

        std::ofstream file(filename, std::ios::binary);
        file.write((const char*)buffer.mData.data() + bufferView.mByteOffset, bufferView.mByteLength);
        
        buffer.erase(asset, bufferView.mByteOffset, bufferView.mByteLength);

        asset.eraseBufferView(mBufferView);
    }

    if (!mData.empty())
    {
        // save j2c image
        std::string filename = folder + "/" + name + ".j2c";

        LLPointer<LLImageRaw> raw = new LLImageRaw(mWidth, mHeight, mComponent);
        U8* data = raw->allocateData();
        llassert(mData.size() == raw->getDataSize());
        memcpy(data, mData.data(), mData.size());

        LLViewerTextureList::createUploadFile(raw, filename, 4096);

        mData.clear();
    }

    mWidth = -1;
    mHeight = -1;
    mComponent = -1;
    mBits = -1;
    mPixelType = -1;
    mMimeType = "";

}

const Material::TextureInfo& Material::TextureInfo::operator=(const tinygltf::TextureInfo& src)
{
    mIndex = src.index;
    mTexCoord = src.texCoord;
    return *this;
}

const Material::OcclusionTextureInfo& Material::OcclusionTextureInfo::operator=(const tinygltf::OcclusionTextureInfo& src)
{
    mIndex = src.index;
    mTexCoord = src.texCoord;
    mStrength = src.strength;
    return *this;
}

const Material::NormalTextureInfo& Material::NormalTextureInfo::operator=(const tinygltf::NormalTextureInfo& src)
{
    mIndex = src.index;
    mTexCoord = src.texCoord;
    mScale = src.scale;
    return *this;
}

const Material::PbrMetallicRoughness& Material::PbrMetallicRoughness::operator=(const tinygltf::PbrMetallicRoughness& src)
{
    if (src.baseColorFactor.size() == 4)
    {
        mBaseColorFactor.set_value(src.baseColorFactor[0], src.baseColorFactor[1], src.baseColorFactor[2], src.baseColorFactor[3]);
    }
    
    mBaseColorTexture = src.baseColorTexture;
    mMetallicFactor = src.metallicFactor;
    mRoughnessFactor = src.roughnessFactor;
    mMetallicRoughnessTexture = src.metallicRoughnessTexture;

    return *this;
}
const Material& Material::operator=(const tinygltf::Material& src)
{
    mName = src.name;
    
    if (src.emissiveFactor.size() == 3)
    {
        mEmissiveFactor.set_value(src.emissiveFactor[0], src.emissiveFactor[1], src.emissiveFactor[2]);
    }

    mPbrMetallicRoughness = src.pbrMetallicRoughness;
    mNormalTexture = src.normalTexture;
    mOcclusionTexture = src.occlusionTexture;
    mEmissiveTexture = src.emissiveTexture;

    mAlphaMode = src.alphaMode;
    mAlphaCutoff = src.alphaCutoff;
    mDoubleSided = src.doubleSided;

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
        GL_FALSE,
        (GLfloat*)glmp.data());
}

