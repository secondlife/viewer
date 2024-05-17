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
#include "../pipeline.h"
#include "buffer_util.h"

using namespace LL::GLTF;
using namespace boost::json;

namespace LL
{
    namespace GLTF
    {
        Material::AlphaMode gltf_alpha_mode_to_enum(const std::string& alpha_mode)
        {
            if (alpha_mode == "OPAQUE")
            {
                return Material::AlphaMode::OPAQUE;
            }
            else if (alpha_mode == "MASK")
            {
                return Material::AlphaMode::MASK;
            }
            else if (alpha_mode == "BLEND")
            {
                return Material::AlphaMode::BLEND;
            }
            else
            {
                return Material::AlphaMode::OPAQUE;
            }
        }

        std::string enum_to_gltf_alpha_mode(Material::AlphaMode alpha_mode)
        {
            switch (alpha_mode)
            {
            case Material::AlphaMode::OPAQUE:
                return "OPAQUE";
            case Material::AlphaMode::MASK:
                return "MASK";
            case Material::AlphaMode::BLEND:
                return "BLEND";
            default:
                return "OPAQUE";
            }
        }

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
                if (src.mMatrix != glm::identity<mat4>())
                {
                    dst.matrix.resize(16);
                    const F32* m = glm::value_ptr(src.mMatrix);
                    for (U32 i = 0; i < 16; ++i)
                    {
                        dst.matrix[i] = m[i];
                    }
                }
            }
            else if (src.mTRSValid)
            {
                if (src.mRotation != glm::identity<quat>())
                {
                    dst.rotation.resize(4);
                    dst.rotation[0] = src.mRotation.x;
                    dst.rotation[1] = src.mRotation.y;
                    dst.rotation[2] = src.mRotation.z;
                    dst.rotation[3] = src.mRotation.w;
                }
                
                if (src.mTranslation != vec3(0.f, 0.f, 0.f))
                {
                    dst.translation.resize(3);
                    dst.translation[0] = src.mTranslation.x;
                    dst.translation[1] = src.mTranslation.y;
                    dst.translation[2] = src.mTranslation.z;
                }
                
                if (src.mScale != vec3(1.f, 1.f, 1.f))
                {
                    dst.scale.resize(3);
                    dst.scale[0] = src.mScale.x;
                    dst.scale[1] = src.mScale.y;
                    dst.scale[2] = src.mScale.z;
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
            dst.baseColorFactor = { src.mBaseColorFactor.r, src.mBaseColorFactor.g, src.mBaseColorFactor.b, src.mBaseColorFactor.a };
            copy(src.mBaseColorTexture, dst.baseColorTexture);
            dst.metallicFactor = src.mMetallicFactor;
            dst.roughnessFactor = src.mRoughnessFactor;
            copy(src.mMetallicRoughnessTexture, dst.metallicRoughnessTexture);
        }

        void copy(const Material& src, tinygltf::Material& material)
        {
            material.name = src.mName;

            material.emissiveFactor = { src.mEmissiveFactor.r, src.mEmissiveFactor.g, src.mEmissiveFactor.b };
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
            accessor.type = (S32) src.mType;
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

            // NOTE: extras are lost in the conversion for now

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
    mat4 identity = glm::identity<mat4>();
    
    for (auto& nodeIndex : mNodes)
    {
        Node& node = asset.mNodes[nodeIndex];
        node.updateTransforms(asset, identity);
    }
}

void Scene::updateRenderTransforms(Asset& asset, const mat4& modelview)
{
    for (auto& nodeIndex : mNodes)
    {
        Node& node = asset.mNodes[nodeIndex];
        node.updateRenderTransforms(asset, modelview);
    }
}

void Node::updateRenderTransforms(Asset& asset, const mat4& modelview)
{
    mRenderMatrix = modelview * mMatrix;

    for (auto& childIndex : mChildren)
    {
        Node& child = asset.mNodes[childIndex];
        child.updateRenderTransforms(asset, mRenderMatrix);
    }
}

void Node::updateTransforms(Asset& asset, const mat4& parentMatrix)
{
    makeMatrixValid();
    mAssetMatrix = parentMatrix * mMatrix;
    
    mAssetMatrixInv = glm::inverse(mAssetMatrix);
    
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

void Asset::updateRenderTransforms(const mat4& modelview)
{
    // use mAssetMatrix to update render transforms from node list
    for (auto& node : mNodes)
    {
        node.mRenderMatrix = modelview * node.mAssetMatrix;
    }
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

            LLMatrix4a ami;
            ami.loadu(glm::value_ptr(node.mAssetMatrixInv));
            // transform start and end to this node's local space
            ami.affineTransform(start, local_start);
            ami.affineTransform(asset_end, local_end);

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
                LLMatrix4a am;
                am.loadu(glm::value_ptr(node.mAssetMatrix));
                // shorten line segment on hit
                am.affineTransform(p, asset_end); 

                // transform results back to asset space
                if (intersection)
                {
                    *intersection = asset_end;
                }

                if (normal || tangent)
                {
                    mat4 normalMatrix = glm::transpose(node.mAssetMatrixInv);

                    LLMatrix4a norm_mat;
                    norm_mat.loadu(glm::value_ptr(normalMatrix));

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
        mMatrix = glm::recompose(mScale, mRotation, mTranslation, vec3(0,0,0), vec4(0,0,0,1));
        mMatrixValid = true;
    }

    llassert(mMatrixValid);
}

void Node::makeTRSValid()
{
    if (!mTRSValid && mMatrixValid)
    {
        vec3 skew;
        vec4 perspective;
        glm::decompose(mMatrix, mScale, mRotation, mTranslation, skew, perspective);
        
        mTRSValid = true;
    }

    llassert(mTRSValid);
}

void Node::setRotation(const quat& q)
{
    makeTRSValid();
    mRotation = q;
    mMatrixValid = false;
}

void Node::setTranslation(const vec3& t)
{
    makeTRSValid();
    mTranslation = t;
    mMatrixValid = false;
}

void Node::setScale(const vec3& s)
{
    makeTRSValid();
    mScale = s;
    mMatrixValid = false;
}

void Node::serialize(object& dst) const
{
    write(mName, "name", dst);
    write(mMatrix, "matrix", dst, glm::identity<mat4>());
    write(mRotation, "rotation", dst);
    write(mTranslation, "translation", dst);
    write(mScale, "scale", dst, vec3(1.f,1.f,1.f));
    write(mChildren, "children", dst);
    write(mMesh, "mesh", dst, INVALID_INDEX);
    write(mSkin, "skin", dst, INVALID_INDEX);
}

const Node& Node::operator=(const Value& src)
{
    copy(src, "name", mName);
    mMatrixValid = copy(src, "matrix", mMatrix);

    copy(src, "rotation", mRotation);
    copy(src, "translation", mTranslation);
    copy(src, "scale", mScale);
    copy(src, "children", mChildren);
    copy(src, "mesh", mMesh);
    copy(src, "skin", mSkin);

    if (!mMatrixValid)
    {
        mTRSValid = true;
    }
    
    return *this;
}

const Node& Node::operator=(const tinygltf::Node& src)
{
    F32* dstMatrix = glm::value_ptr(mMatrix);

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
            mRotation = quat((F32)src.rotation[3], (F32)src.rotation[0], (F32)src.rotation[1], (F32)src.rotation[2]);
        }

        if (src.translation.size() == 3)
        {
            mTranslation = vec3((F32)src.translation[0], (F32)src.translation[1], (F32)src.translation[2]);
        }

        if (src.scale.size() == 3)
        {
            mScale = vec3((F32)src.scale[0], (F32)src.scale[1], (F32)src.scale[2]);
        }
        else
        {
            mScale = vec3(1.f, 1.f, 1.f);
        }

        mTRSValid = true;
    }
    else
    {
        // node specifies no transformation, set to identity
        mMatrix = glm::identity<mat4>();
        mMatrixValid = true;
    }

    mChildren = src.children;
    mMesh = src.mesh;
    mSkin = src.skin;
    mName = src.name;

    return *this;
}

void Image::serialize(object& dst) const
{
    write(mUri, "uri", dst);
    write(mMimeType, "mimeType", dst);
    write(mBufferView, "bufferView", dst, INVALID_INDEX);
    write(mName, "name", dst);
    write(mWidth, "width", dst, -1);
    write(mHeight, "height", dst, -1);
    write(mComponent, "component", dst, -1);
    write(mBits, "bits", dst, -1);
    write(mPixelType, "pixelType", dst, -1);
}

const Image& Image::operator=(const Value& src)
{
    copy(src, "uri", mUri);
    copy(src, "mimeType", mMimeType);
    copy(src, "bufferView", mBufferView);
    copy(src, "name", mName);
    copy(src, "width", mWidth);
    copy(src, "height", mHeight);
    copy(src, "component", mComponent);
    copy(src, "bits", mBits);
    copy(src, "pixelType", mPixelType);

    return *this;
}

const Image& Image::operator=(const tinygltf::Image& src)
{
    mName = src.name;
    mWidth = src.width;
    mHeight = src.height;
    mComponent = src.component;
    mBits = src.bits;
    mPixelType = src.pixel_type;
    mUri = src.uri;
    mBufferView = src.bufferView;
    mMimeType = src.mimeType;
    mData = src.image;
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
                    gGL.loadMatrix((F32*)glm::value_ptr(node.mRenderMatrix));
                }
                bool cull = true;
                if (primitive.mMaterial != INVALID_INDEX)
                {
                    Material& material = mMaterials[primitive.mMaterial];
                    bool mat_opaque = material.mAlphaMode != Material::AlphaMode::BLEND;

                    if (mat_opaque != opaque)
                    {
                        continue;
                    }

                    if (mMaterials[primitive.mMaterial].mMaterial.notNull())
                    {
                        material.mMaterial->bind();
                    }
                    else
                    {
                        material.bind(*this);
                    }
                    cull = !material.mDoubleSided;
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
    if (!filename.empty())
    {
        for (U32 i = 0; i < mMaterials.size(); ++i)
        {
            // HACK: local preview mode, load material from model for now
            mMaterials[i].allocateGLResources(*this);
            LLTinyGLTFHelper::getMaterialFromModel(filename, model, i, mMaterials[i].mMaterial, mMaterials[i].mName, true);
        }
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

Asset::Asset(const tinygltf::Model& src)
{
    *this = src;
}

Asset::Asset(const Value& src)
{
    *this = src;
}

const Asset& Asset::operator=(const tinygltf::Model& src)
{
    mVersion = src.asset.version;
    mMinVersion = src.asset.minVersion;
    mGenerator = src.asset.generator;
    mCopyright = src.asset.copyright;

    // note: extras are lost in the conversion for now

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

const Asset& Asset::operator=(const Value& src)
{
    if (src.is_object())
    {
        const object& obj = src.as_object();

        const auto it = obj.find("asset");

        if (it != obj.end())
        {
            const Value& asset = it->value();

            copy(asset, "version", mVersion);
            copy(asset, "minVersion", mMinVersion);
            copy(asset, "generator", mGenerator);
            copy(asset, "copyright", mCopyright);
            copy(asset, "extras", mExtras);
        }

        copy(obj, "defaultScene", mDefaultScene);
        copy(obj, "scenes", mScenes);
        copy(obj, "nodes", mNodes);
        copy(obj, "meshes", mMeshes);
        copy(obj, "materials", mMaterials);
        copy(obj, "buffers", mBuffers);
        copy(obj, "bufferViews", mBufferViews);
        copy(obj, "textures", mTextures);
        copy(obj, "samplers", mSamplers);
        copy(obj, "images", mImages);
        copy(obj, "accessors", mAccessors);
        copy(obj, "animations", mAnimations);
        copy(obj, "skins", mSkins);
    }

    return *this;
}

void Asset::save(tinygltf::Model& dst)
{
    LL::GLTF::copy(*this, dst);
}

void Asset::serialize(object& dst) const
{
    write(mVersion, "version", dst);
    write(mMinVersion, "minVersion", dst, std::string());
    write(mGenerator, "generator", dst);
    write(mDefaultScene, "defaultScene", dst, 0);
    
    write(mScenes, "scenes", dst);
    write(mNodes, "nodes", dst);
    write(mMeshes, "meshes", dst);
    write(mMaterials, "materials", dst);
    write(mBuffers, "buffers", dst);
    write(mBufferViews, "bufferViews", dst);
    write(mTextures, "textures", dst);
    write(mSamplers, "samplers", dst);
    write(mImages, "images", dst);
    write(mAccessors, "accessors", dst);
    write(mAnimations, "animations", dst);
    write(mSkins, "skins", dst);
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

LLViewerFetchedTexture* fetch_texture(const LLUUID& id);

void Image::allocateGLResources()
{
    LLUUID id;
    if (LLUUID::parseUUID(mUri, &id) && id.notNull())
    {
        mTexture = fetch_texture(id);
    }
}


void Image::clearData(Asset& asset)
{
    if (mBufferView != INVALID_INDEX)
    {
        // remove data from buffer
        BufferView& bufferView = asset.mBufferViews[mBufferView];
        Buffer& buffer = asset.mBuffers[bufferView.mBuffer];

        buffer.erase(asset, bufferView.mByteOffset, bufferView.mByteLength);

        asset.eraseBufferView(mBufferView);
    }

    mData.clear();
    mBufferView = INVALID_INDEX;
    mWidth = -1;
    mHeight = -1;
    mComponent = -1;
    mBits = -1;
    mPixelType = -1;
    mMimeType = "";
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
    }

#if 0
    if (!mData.empty())
    {
        // save j2c image
        std::string filename = folder + "/" + name + ".j2c";

        LLPointer<LLImageRaw> raw = new LLImageRaw(mWidth, mHeight, mComponent);
        U8* data = raw->allocateData();
        llassert_always(mData.size() == raw->getDataSize());
        memcpy(data, mData.data(), mData.size());

        LLViewerTextureList::createUploadFile(raw, filename, 4096);

        mData.clear();
    }
#endif


    clearData(asset);
}

void Material::TextureInfo::serialize(object& dst) const
{
    write(mIndex, "index", dst, INVALID_INDEX);
    write(mTexCoord, "texCoord", dst, 0);
}

const Material::TextureInfo& Material::TextureInfo::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "index", mIndex);
        copy(src, "texCoord", mTexCoord);
    }

    return *this;
}

bool Material::TextureInfo::operator==(const Material::TextureInfo& rhs) const
{
    return mIndex == rhs.mIndex && mTexCoord == rhs.mTexCoord;
}

bool Material::TextureInfo::operator!=(const Material::TextureInfo& rhs) const
{
    return !(*this == rhs);
}

const Material::TextureInfo& Material::TextureInfo::operator=(const tinygltf::TextureInfo& src)
{
    mIndex = src.index;
    mTexCoord = src.texCoord;
    return *this;
}

void Material::OcclusionTextureInfo::serialize(object& dst) const
{
    write(mIndex, "index", dst, INVALID_INDEX);
    write(mTexCoord, "texCoord", dst, 0);
    write(mStrength, "strength", dst, 1.f);
}

const Material::OcclusionTextureInfo& Material::OcclusionTextureInfo::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "index", mIndex);
        copy(src, "texCoord", mTexCoord);
        copy(src, "strength", mStrength);
    }

    return *this;
}

const Material::OcclusionTextureInfo& Material::OcclusionTextureInfo::operator=(const tinygltf::OcclusionTextureInfo& src)
{
    mIndex = src.index;
    mTexCoord = src.texCoord;
    mStrength = src.strength;
    return *this;
}

void Material::NormalTextureInfo::serialize(object& dst) const
{
    write(mIndex, "index", dst, INVALID_INDEX);
    write(mTexCoord, "texCoord", dst, 0);
    write(mScale, "scale", dst, 1.f);
}

const Material::NormalTextureInfo& Material::NormalTextureInfo::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "index", mIndex);
        copy(src, "texCoord", mTexCoord);
        copy(src, "scale", mScale);
    }

    return *this;
}
const Material::NormalTextureInfo& Material::NormalTextureInfo::operator=(const tinygltf::NormalTextureInfo& src)
{
    mIndex = src.index;
    mTexCoord = src.texCoord;
    mScale = src.scale;
    return *this;
}

const Material::PbrMetallicRoughness& Material::PbrMetallicRoughness::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "baseColorFactor", mBaseColorFactor);
        copy(src, "baseColorTexture", mBaseColorTexture);
        copy(src, "metallicFactor", mMetallicFactor);
        copy(src, "roughnessFactor", mRoughnessFactor);
        copy(src, "metallicRoughnessTexture", mMetallicRoughnessTexture);
    }

    return *this;
}

void Material::PbrMetallicRoughness::serialize(object& dst) const
{
    write(mBaseColorFactor, "baseColorFactor", dst, vec4(1.f, 1.f, 1.f, 1.f));
    write(mBaseColorTexture, "baseColorTexture", dst);
    write(mMetallicFactor, "metallicFactor", dst, 1.f);
    write(mRoughnessFactor, "roughnessFactor", dst, 1.f);
    write(mMetallicRoughnessTexture, "metallicRoughnessTexture", dst);
}

bool Material::PbrMetallicRoughness::operator==(const Material::PbrMetallicRoughness& rhs) const
{
    return mBaseColorFactor == rhs.mBaseColorFactor &&
        mBaseColorTexture == rhs.mBaseColorTexture &&
        mMetallicFactor == rhs.mMetallicFactor &&
        mRoughnessFactor == rhs.mRoughnessFactor &&
        mMetallicRoughnessTexture == rhs.mMetallicRoughnessTexture;
}

bool Material::PbrMetallicRoughness::operator!=(const Material::PbrMetallicRoughness& rhs) const
{
    return !(*this == rhs);
}

const Material::PbrMetallicRoughness& Material::PbrMetallicRoughness::operator=(const tinygltf::PbrMetallicRoughness& src)
{
    if (src.baseColorFactor.size() == 4)
    {
        mBaseColorFactor = vec4(src.baseColorFactor[0], src.baseColorFactor[1], src.baseColorFactor[2], src.baseColorFactor[3]);
    }
    
    mBaseColorTexture = src.baseColorTexture;
    mMetallicFactor = src.metallicFactor;
    mRoughnessFactor = src.roughnessFactor;
    mMetallicRoughnessTexture = src.metallicRoughnessTexture;

    return *this;
}

static void bindTexture(Asset& asset, S32 uniform, Material::TextureInfo& info, LLViewerTexture* fallback)
{
    if (info.mIndex != INVALID_INDEX)
    {
        LLViewerTexture* tex = asset.mImages[asset.mTextures[info.mIndex].mSource].mTexture;
        if (tex)
        {
            tex->addTextureStats(2048.f * 2048.f);
            LLGLSLShader::sCurBoundShaderPtr->bindTexture(uniform, tex);
        }
        else
        {
            LLGLSLShader::sCurBoundShaderPtr->bindTexture(uniform, fallback);
        }
    }
    else
    {
        LLGLSLShader::sCurBoundShaderPtr->bindTexture(uniform, fallback);
    }
}

void Material::bind(Asset& asset)
{
    // bind for rendering (derived from LLFetchedGLTFMaterial::bind)
    // glTF 2.0 Specification 3.9.4. Alpha Coverage
    // mAlphaCutoff is only valid for LLGLTFMaterial::ALPHA_MODE_MASK
    F32 min_alpha = -1.0;

    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

    if (!LLPipeline::sShadowRender || (mAlphaMode == Material::AlphaMode::BLEND))
    {
        if (mAlphaMode == Material::AlphaMode::MASK)
        {
            // dividing the alpha cutoff by transparency here allows the shader to compare against
            // the alpha value of the texture without needing the transparency value
            if (mPbrMetallicRoughness.mBaseColorFactor.a > 0.f)
            {
                min_alpha = mAlphaCutoff / mPbrMetallicRoughness.mBaseColorFactor.a;
            }
            else
            {
                min_alpha = 1024.f;
            }
        }
        shader->uniform1f(LLShaderMgr::MINIMUM_ALPHA, min_alpha);
    }

    bindTexture(asset, LLShaderMgr::DIFFUSE_MAP, mPbrMetallicRoughness.mBaseColorTexture, LLViewerFetchedTexture::sWhiteImagep);

    F32 base_color_packed[8];
    //mTextureTransform[GLTF_TEXTURE_INFO_BASE_COLOR].getPacked(base_color_packed);
    LLGLTFMaterial::sDefault.mTextureTransform[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR].getPacked(base_color_packed);
    shader->uniform4fv(LLShaderMgr::TEXTURE_BASE_COLOR_TRANSFORM, 2, (F32*)base_color_packed);

    if (!LLPipeline::sShadowRender)
    {
        bindTexture(asset, LLShaderMgr::BUMP_MAP, mNormalTexture, LLViewerFetchedTexture::sFlatNormalImagep);      
        bindTexture(asset, LLShaderMgr::SPECULAR_MAP, mPbrMetallicRoughness.mMetallicRoughnessTexture, LLViewerFetchedTexture::sWhiteImagep);
        bindTexture(asset, LLShaderMgr::EMISSIVE_MAP, mEmissiveTexture, LLViewerFetchedTexture::sWhiteImagep);
        
        // NOTE: base color factor is baked into vertex stream

        shader->uniform1f(LLShaderMgr::ROUGHNESS_FACTOR, mPbrMetallicRoughness.mRoughnessFactor);
        shader->uniform1f(LLShaderMgr::METALLIC_FACTOR, mPbrMetallicRoughness.mMetallicFactor);
        shader->uniform3fv(LLShaderMgr::EMISSIVE_COLOR, 1, glm::value_ptr(mEmissiveFactor));

        F32 normal_packed[8];
        //mTextureTransform[GLTF_TEXTURE_INFO_NORMAL].getPacked(normal_packed);
        LLGLTFMaterial::sDefault.mTextureTransform[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL].getPacked(normal_packed);
        shader->uniform4fv(LLShaderMgr::TEXTURE_NORMAL_TRANSFORM, 2, (F32*)normal_packed);

        F32 metallic_roughness_packed[8];
        //mTextureTransform[GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS].getPacked(metallic_roughness_packed);
        LLGLTFMaterial::sDefault.mTextureTransform[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS].getPacked(metallic_roughness_packed);
        shader->uniform4fv(LLShaderMgr::TEXTURE_METALLIC_ROUGHNESS_TRANSFORM, 2, (F32*)metallic_roughness_packed);

        F32 emissive_packed[8];
        //mTextureTransform[GLTF_TEXTURE_INFO_EMISSIVE].getPacked(emissive_packed);
        LLGLTFMaterial::sDefault.mTextureTransform[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE].getPacked(emissive_packed);
        shader->uniform4fv(LLShaderMgr::TEXTURE_EMISSIVE_TRANSFORM, 2, (F32*)emissive_packed);
    }
}

void Material::serialize(object& dst) const
{
    write(mName, "name", dst);
    write(mEmissiveFactor, "emissiveFactor", dst, vec3(0.f, 0.f, 0.f));
    write(mPbrMetallicRoughness, "pbrMetallicRoughness", dst);
    write(mNormalTexture, "normalTexture", dst);
    write(mOcclusionTexture, "occlusionTexture", dst);
    write(mEmissiveTexture, "emissiveTexture", dst);
    write(mAlphaMode, "alphaMode", dst, Material::AlphaMode::OPAQUE);
    write(mAlphaCutoff, "alphaCutoff", dst, 0.5f);
    write(mDoubleSided, "doubleSided", dst, false);
}

const Material& Material::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "name", mName);
        copy(src, "emissiveFactor", mEmissiveFactor);
        copy(src, "pbrMetallicRoughness", mPbrMetallicRoughness);
        copy(src, "normalTexture", mNormalTexture);
        copy(src, "occlusionTexture", mOcclusionTexture);
        copy(src, "emissiveTexture", mEmissiveTexture);
        copy(src, "alphaMode", mAlphaMode);
        copy(src, "alphaCutoff", mAlphaCutoff);
        copy(src, "doubleSided", mDoubleSided);
    }
    return *this;
}


const Material& Material::operator=(const tinygltf::Material& src)
{
    mName = src.name;
    
    if (src.emissiveFactor.size() == 3)
    {
        mEmissiveFactor = vec3(src.emissiveFactor[0], src.emissiveFactor[1], src.emissiveFactor[2]);
    }

    mPbrMetallicRoughness = src.pbrMetallicRoughness;
    mNormalTexture = src.normalTexture;
    mOcclusionTexture = src.occlusionTexture;
    mEmissiveTexture = src.emissiveTexture;

    mAlphaMode = gltf_alpha_mode_to_enum(src.alphaMode);
    mAlphaCutoff = src.alphaCutoff;
    mDoubleSided = src.doubleSided;

    return *this;
}

void Material::allocateGLResources(Asset& asset)
{
    // HACK: allocate an LLFetchedGLTFMaterial for now
    // later we'll render directly from the GLTF Images
    // and BufferViews
    mMaterial = new LLFetchedGLTFMaterial();
}

void Mesh::serialize(object& dst) const
{
    write(mPrimitives, "primitives", dst);
    write(mWeights, "weights", dst);
    write(mName, "name", dst);
}

const Mesh& Mesh::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "primitives", mPrimitives);
        copy(src, "weights", mWeights);
        copy(src, "name", mName);
    }

    return *this;

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

void Scene::serialize(object& dst) const
{
    write(mNodes, "nodes", dst);
    write(mName, "name", dst);
}

const Scene& Scene::operator=(const Value& src)
{
    copy(src, "nodes", mNodes);
    copy(src, "name", mName);
    
    return *this;
}

const Scene& Scene::operator=(const tinygltf::Scene& src)
{
    mNodes = src.nodes;
    mName = src.name;

    return *this;
}

void Texture::serialize(object& dst) const
{
    write(mSampler, "sampler", dst, INVALID_INDEX);
    write(mSource, "source", dst, INVALID_INDEX);
    write(mName, "name", dst);
}

const Texture& Texture::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "sampler", mSampler);
        copy(src, "source", mSource);
        copy(src, "name", mName);
    }

    return *this;
}

const Texture& Texture::operator=(const tinygltf::Texture& src)
{
    mSampler = src.sampler;
    mSource = src.source;
    mName = src.name;

    return *this;
}


void Sampler::serialize(object& dst) const
{
    write(mMagFilter, "magFilter", dst, LINEAR);
    write(mMinFilter, "minFilter", dst, LINEAR_MIPMAP_LINEAR);
    write(mWrapS, "wrapS", dst, REPEAT);
    write(mWrapT, "wrapT", dst, REPEAT);
    write(mName, "name", dst);
}

const Sampler& Sampler::operator=(const Value& src)
{
    copy(src, "magFilter", mMagFilter);
    copy(src, "minFilter", mMinFilter);
    copy(src, "wrapS", mWrapS);
    copy(src, "wrapT", mWrapT);
    copy(src, "name", mName);

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
    std::vector<mat4> t_mp;

    t_mp.resize(mJoints.size());

    for (U32 i = 0; i < mJoints.size(); ++i)
    {
        Node& joint = asset.mNodes[mJoints[i]];
        t_mp[i] = joint.mRenderMatrix * mInverseBindMatricesData[i];
    }

    std::vector<F32> glmp;

    glmp.resize(mJoints.size() * 12);

    F32* mp = glmp.data();

    for (U32 i = 0; i < mJoints.size(); ++i)
    {
        F32* m = glm::value_ptr(t_mp[i]);

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

