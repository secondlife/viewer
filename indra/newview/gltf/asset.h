#pragma once

/**
 * @file asset.h
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
#include "../lltinygltfhelper.h"
#include "accessor.h"
#include "primitive.h"
#include "animation.h"

extern F32SecondsImplicit       gFrameTimeSeconds;

// LL GLTF Implementation
namespace LL
{
    namespace GLTF
    {
        class Asset;

        class Material
        {
        public:
            // use LLFetchedGLTFMaterial for now, but eventually we'll want to use
            // a more flexible GLTF material implementation instead of the fixed packing
            // version we use for sharable GLTF material assets
            LLPointer<LLFetchedGLTFMaterial> mMaterial;
            std::string mName;

            const Material& operator=(const tinygltf::Material& src);

            void allocateGLResources(Asset& asset);
        };

        class Mesh
        {
        public:
            std::vector<Primitive> mPrimitives;
            std::vector<double> mWeights;
            std::string mName;

            const Mesh& operator=(const tinygltf::Mesh& src);

            void allocateGLResources(Asset& asset);
        };

        class Node
        {
        public:
            LLMatrix4a mMatrix; //local transform
            LLMatrix4a mRenderMatrix; //transform for rendering
            LLMatrix4a mAssetMatrix; //transform from local to asset space
            LLMatrix4a mAssetMatrixInv; //transform from asset to local space

            glh::vec3f mTranslation;
            glh::quaternionf mRotation;
            glh::vec3f mScale;

            // if true, mMatrix is valid and up to date
            bool mMatrixValid = false;

            // if true, translation/rotation/scale are valid and up to date
            bool mTRSValid = false;

            bool mNeedsApplyMatrix = false;

            std::vector<S32> mChildren;
            S32 mParent = INVALID_INDEX;

            S32 mMesh = INVALID_INDEX;
            S32 mSkin = INVALID_INDEX;

            std::string mName;

            const Node& operator=(const tinygltf::Node& src);

            // Set mRenderMatrix to a transform that can be used for the current render pass
            // modelview -- parent's render matrix
            void updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview);

            // update mAssetMatrix and mAssetMatrixInv
            void updateTransforms(Asset& asset, const LLMatrix4a& parentMatrix);

            // ensure mMatrix is valid -- if mMatrixValid is false and mTRSValid is true, will update mMatrix to match Translation/Rotation/Scale
            void makeMatrixValid();

            // ensure Translation/Rotation/Scale are valid -- if mTRSValid is false and mMatrixValid is true, will update Translation/Rotation/Scale to match mMatrix
            void makeTRSValid();

            // Set rotation of this node
            // SIDE EFFECT: invalidates mMatrix
            void setRotation(const glh::quaternionf& rotation);

            // Set translation of this node
            // SIDE EFFECT: invalidates mMatrix
            void setTranslation(const glh::vec3f& translation);

            // Set scale of this node
            // SIDE EFFECT: invalidates mMatrix
            void setScale(const glh::vec3f& scale);
        };

        class Skin
        {
        public:
            S32 mInverseBindMatrices = INVALID_INDEX;
            S32 mSkeleton = INVALID_INDEX;
            std::vector<S32> mJoints;
            std::string mName;
            std::vector<glh::matrix4f> mInverseBindMatricesData;

            void allocateGLResources(Asset& asset);
            void uploadMatrixPalette(Asset& asset, Node& node);

            const Skin& operator=(const tinygltf::Skin& src);
        };

        class Scene
        {
        public:
            std::vector<S32> mNodes;
            std::string mName;

            const Scene& operator=(const tinygltf::Scene& src);

            void updateTransforms(Asset& asset);
            void updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview);
        };

        class Texture
        {
        public:
            S32 mSampler = INVALID_INDEX;
            S32 mSource = INVALID_INDEX;
            std::string mName;

            const Texture& operator=(const tinygltf::Texture& src);
        };

        class Sampler
        {
        public:
            S32 mMagFilter;
            S32 mMinFilter;
            S32 mWrapS;
            S32 mWrapT;
            std::string mName;

            const Sampler& operator=(const tinygltf::Sampler& src);
        };

        class Image
        {
        public:
            std::string mName;
            std::string mUri;
            std::string mMimeType;
            std::vector<U8> mData;
            S32 mWidth;
            S32 mHeight;
            S32 mComponent;
            S32 mBits;
            LLPointer<LLViewerFetchedTexture> mTexture;

            const Image& operator=(const tinygltf::Image& src)
            {
                mName = src.name;
                mUri = src.uri;
                mMimeType = src.mimeType;
                mData = src.image;
                mWidth = src.width;
                mHeight = src.height;
                mComponent = src.component;
                mBits = src.bits;

                return *this;
            }

            void allocateGLResources()
            {
                // allocate texture

            }
        };

        // C++ representation of a GLTF Asset
        class Asset : public LLRefCount
        {
        public:
            std::vector<Scene> mScenes;
            std::vector<Node> mNodes;
            std::vector<Mesh> mMeshes;
            std::vector<Material> mMaterials;
            std::vector<Buffer> mBuffers;
            std::vector<BufferView> mBufferViews;
            std::vector<Texture> mTextures;
            std::vector<Sampler> mSamplers;
            std::vector<Image> mImages;
            std::vector<Accessor> mAccessors;
            std::vector<Animation> mAnimations;
            std::vector<Skin> mSkins;

            // the last time update() was called according to gFrameTimeSeconds
            F32 mLastUpdateTime = gFrameTimeSeconds;

            // prepare the asset for rendering
            void allocateGLResources(const std::string& filename, const tinygltf::Model& model);

            // Called periodically (typically once per frame)
            // Any ongoing work (such as animations) should be handled here
            // NOT guaranteed to be called every frame
            // MAY be called more than once per frame
            // Upon return, all Node Matrix transforms should be up to date
            void update();

            // update asset-to-node and node-to-asset transforms
            void updateTransforms();

            // update node render transforms
            void updateRenderTransforms(const LLMatrix4a& modelview);

            void render(bool opaque, bool rigged = false);
            void renderOpaque();
            void renderTransparent();

            // return the index of the node that the line segment intersects with, or -1 if no hit
            // input and output values must be in this asset's local coordinate frame
            S32 lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
                LLVector4a* intersection = nullptr,         // return the intersection point
                LLVector2* tex_coord = nullptr,            // return the texture coordinates of the intersection point
                LLVector4a* normal = nullptr,               // return the surface normal at the intersection point
                LLVector4a* tangent = nullptr,             // return the surface tangent at the intersection point
                S32* primitive_hitp = nullptr           // return the index of the primitive that was hit
            );

            const Asset& operator=(const tinygltf::Model& src);

        };
    }
}
