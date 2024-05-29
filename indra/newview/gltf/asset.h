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
#include "accessor.h"
#include "primitive.h"
#include "animation.h"
#include "boost/json.hpp"
#include "common.h"
#include "../llviewertexture.h"

extern F32SecondsImplicit       gFrameTimeSeconds;

// wingdi defines OPAQUE, which conflicts with our enum
#if defined(OPAQUE)
#undef OPAQUE
#endif

// LL GLTF Implementation
namespace LL
{
    namespace GLTF
    {
        class Asset;

        class Material
        {
        public:

            enum class AlphaMode
            {
                OPAQUE,
                MASK,
                BLEND
            };

            class TextureInfo
            {
            public:
                S32 mIndex = INVALID_INDEX;
                S32 mTexCoord = 0;

                bool operator==(const TextureInfo& rhs) const;
                bool operator!=(const TextureInfo& rhs) const;

                const TextureInfo& operator=(const Value& src);
                void serialize(boost::json::object& dst) const;
            };

            class NormalTextureInfo : public TextureInfo
            {
            public:
                F32 mScale = 1.0f;

                const NormalTextureInfo& operator=(const Value& src);
                void serialize(boost::json::object& dst) const;
            };

            class OcclusionTextureInfo : public TextureInfo
            {
            public:
                F32 mStrength = 1.0f;

                const OcclusionTextureInfo& operator=(const Value& src);
                void serialize(boost::json::object& dst) const;
            };

            class PbrMetallicRoughness
            {
            public:
                vec4 mBaseColorFactor = vec4(1.f,1.f,1.f,1.f);
                TextureInfo mBaseColorTexture;
                F32 mMetallicFactor = 1.0f;
                F32 mRoughnessFactor = 1.0f;
                TextureInfo mMetallicRoughnessTexture;

                bool operator==(const PbrMetallicRoughness& rhs) const;
                bool operator!=(const PbrMetallicRoughness& rhs) const;
                const PbrMetallicRoughness& operator=(const Value& src);
                void serialize(boost::json::object& dst) const;
            };


            PbrMetallicRoughness mPbrMetallicRoughness;
            NormalTextureInfo mNormalTexture;
            OcclusionTextureInfo mOcclusionTexture;
            TextureInfo mEmissiveTexture;

            std::string mName;
            vec3 mEmissiveFactor = vec3(0.f, 0.f, 0.f);
            AlphaMode mAlphaMode = AlphaMode::OPAQUE;
            F32 mAlphaCutoff = 0.5f;
            bool mDoubleSided = false;

            const Material& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;
        };

        class Mesh
        {
        public:
            std::vector<Primitive> mPrimitives;
            std::vector<double> mWeights;
            std::string mName;

            const Mesh& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;

            bool prep(Asset& asset);
        };

        class Node
        {
        public:
            mat4 mMatrix = glm::identity<mat4>(); //local transform
            mat4 mRenderMatrix; //transform for rendering
            mat4 mAssetMatrix; //transform from local to asset space
            mat4 mAssetMatrixInv; //transform from asset to local space

            vec3  mTranslation = vec3(0,0,0);
            quat mRotation = glm::identity<quat>();
            vec3 mScale = vec3(1.f,1.f,1.f);

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

            const Node& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;

            // Set mRenderMatrix to a transform that can be used for the current render pass
            // modelview -- parent's render matrix
            void updateRenderTransforms(Asset& asset, const mat4& modelview);

            // update mAssetMatrix and mAssetMatrixInv
            void updateTransforms(Asset& asset, const mat4& parentMatrix);

            // ensure mMatrix is valid -- if mMatrixValid is false and mTRSValid is true, will update mMatrix to match Translation/Rotation/Scale
            void makeMatrixValid();

            // ensure Translation/Rotation/Scale are valid -- if mTRSValid is false and mMatrixValid is true, will update Translation/Rotation/Scale to match mMatrix
            void makeTRSValid();

            // Set rotation of this node
            // SIDE EFFECT: invalidates mMatrix
            void setRotation(const quat& rotation);

            // Set translation of this node
            // SIDE EFFECT: invalidates mMatrix
            void setTranslation(const vec3& translation);

            // Set scale of this node
            // SIDE EFFECT: invalidates mMatrix
            void setScale(const vec3& scale);
        };

        class Skin
        {
        public:
            ~Skin();

            S32 mInverseBindMatrices = INVALID_INDEX;
            S32 mSkeleton = INVALID_INDEX;

            U32 mUBO = 0;
            std::vector<S32> mJoints;
            std::string mName;
            std::vector<mat4> mInverseBindMatricesData;

            bool prep(Asset& asset);
            void uploadMatrixPalette(Asset& asset);

            const Skin& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;
        };

        class Scene
        {
        public:
            std::vector<S32> mNodes;
            std::string mName;

            const Scene& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;

            void updateTransforms(Asset& asset);
            void updateRenderTransforms(Asset& asset, const mat4& modelview);
        };

        class Texture
        {
        public:
            S32 mSampler = INVALID_INDEX;
            S32 mSource = INVALID_INDEX;
            std::string mName;

            const Texture& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;
        };

        class Sampler
        {
        public:
            S32 mMagFilter = LINEAR;
            S32 mMinFilter = LINEAR_MIPMAP_LINEAR;
            S32 mWrapS = REPEAT;
            S32 mWrapT = REPEAT;
            std::string mName;

            const Sampler& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;
        };

        class Image
        {
        public:
            std::string mName;
            std::string mUri;
            std::string mMimeType;

            S32 mBufferView = INVALID_INDEX;

            S32 mWidth = -1;
            S32 mHeight = -1;
            S32 mComponent = -1;
            S32 mBits = -1;
            S32 mPixelType = -1;

            LLPointer<LLViewerFetchedTexture> mTexture;

            const Image& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;

            // save image to disk
            // may remove image data from bufferviews and convert to
            // file uri if necessary
            bool save(Asset& asset, const std::string& filename);

            // erase the buffer view associated with this image
            // free any associated GLTF resources
            // preserve only uri and name
            void clearData(Asset& asset);

            bool prep(Asset& asset);
        };

        // C++ representation of a GLTF Asset
        class Asset
        {
        public:

            static const std::string minVersion_default;
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

            std::string mVersion;
            std::string mGenerator;
            std::string mMinVersion;
            std::string mCopyright;

            S32 mScene = INVALID_INDEX;
            Value mExtras;

            U32 mPendingBuffers = 0;

            // local file this asset was loaded from (if any)
            std::string mFilename;

            // the last time update() was called according to gFrameTimeSeconds
            F32 mLastUpdateTime = gFrameTimeSeconds;


            // prepare for first time use
            bool prep();

            // Called periodically (typically once per frame)
            // Any ongoing work (such as animations) should be handled here
            // NOT guaranteed to be called every frame
            // MAY be called more than once per frame
            // Upon return, all Node Matrix transforms should be up to date
            void update();

            // update asset-to-node and node-to-asset transforms
            void updateTransforms();

            // update node render transforms
            void updateRenderTransforms(const mat4& modelview);

            // return the index of the node that the line segment intersects with, or -1 if no hit
            // input and output values must be in this asset's local coordinate frame
            S32 lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
                LLVector4a* intersection = nullptr,         // return the intersection point
                LLVector2* tex_coord = nullptr,            // return the texture coordinates of the intersection point
                LLVector4a* normal = nullptr,               // return the surface normal at the intersection point
                LLVector4a* tangent = nullptr,             // return the surface tangent at the intersection point
                S32* primitive_hitp = nullptr           // return the index of the primitive that was hit
            );

            Asset() = default;
            Asset(const Value& src);

            // load from given file
            // accepts .gltf and .glb files
            // Any existing data will be lost
            // returns result of prep() on success
            bool load(std::string_view filename);

            // load .glb contents from memory
            // data - binary contents of .glb file
            // returns result of prep() on success
            bool loadBinary(const std::string& data);

            const Asset& operator=(const Value& src);
            void serialize(boost::json::object& dst) const;

            // save the asset to the given .gltf file
            // saves images and bins alongside the gltf file
            bool save(const std::string& filename);

            // remove the bufferview at the given index
            // updates all bufferview indices in this Asset as needed
            void eraseBufferView(S32 bufferView);
        };

        Material::AlphaMode gltf_alpha_mode_to_enum(const std::string& alpha_mode);
        std::string enum_to_gltf_alpha_mode(Material::AlphaMode alpha_mode);
    }
}
