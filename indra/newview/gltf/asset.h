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


// LL GLTF Implementation
namespace LL
{
    namespace GLTF
    {
        constexpr S32 INVALID_INDEX = -1;

        class Asset;

        constexpr U32 ATTRIBUTE_MASK =
            LLVertexBuffer::MAP_VERTEX |
            LLVertexBuffer::MAP_NORMAL |
            LLVertexBuffer::MAP_TEXCOORD0 |
            LLVertexBuffer::MAP_TANGENT |
            LLVertexBuffer::MAP_COLOR;

        class Buffer
        {
        public:
            std::vector<U8> mData;
            std::string mName;
            std::string mUri;

            const Buffer& operator=(const tinygltf::Buffer& src)
            {
                mData = src.data;
                mName = src.name;
                mUri = src.uri;
                return *this;
            }
        };

        class BufferView
        {
        public:
            S32 mBuffer = INVALID_INDEX;
            S32 mByteLength;
            S32 mByteOffset;
            S32 mByteStride;
            S32 mTarget;
            S32 mComponentType;

            std::string mName;

            const BufferView& operator=(const tinygltf::BufferView& src)
            {
                mBuffer = src.buffer;
                mByteLength = src.byteLength;
                mByteOffset = src.byteOffset;
                mByteStride = src.byteStride;
                mTarget = src.target;
                mName = src.name;
                return *this;
            }
        };

        class Accessor
        {
        public:
            S32 mBufferView = INVALID_INDEX;
            S32 mByteOffset;
            S32 mComponentType;
            S32 mCount;
            std::vector<double> mMax;
            std::vector<double> mMin;
            S32 mType;
            bool mNormalized;
            std::string mName;

            const Accessor& operator=(const tinygltf::Accessor& src)
            {
                mBufferView = src.bufferView;
                mByteOffset = src.byteOffset;
                mComponentType = src.componentType;
                mCount = src.count;
                mType = src.type;
                mNormalized = src.normalized;
                mName = src.name;
                mMax = src.maxValues;
                mMin = src.maxValues;

                return *this;
            }
        };

        class Material
        {
        public:
            // use LLFetchedGLTFMaterial for now, but eventually we'll want to use
            // a more flexible GLTF material implementation instead of the fixed packing
            // version we use for sharable GLTF material assets
            LLPointer<LLFetchedGLTFMaterial> mMaterial;
            std::string mName;

            const Material& operator=(const tinygltf::Material& src)
            {
                mName = src.name;
                return *this;
            }

            void allocateGLResources(Asset& asset)
            {
                // allocate material
                mMaterial = new LLFetchedGLTFMaterial();
            }
        };

        class Primitive
        {
        public:
            LLPointer<LLVertexBuffer> mVertexBuffer;
            LLPointer<LLVolumeOctree> mOctree;

            S32 mMaterial = INVALID_INDEX;
            U32 mMode = TINYGLTF_MODE_TRIANGLES; // default to triangles
            U32 mGLMode = LLRender::TRIANGLES;
            S32 mIndices = -1;
            std::unordered_map<std::string, int> mAttributes;

            // copy the attribute in the given BufferView to the given destination
            // assumes destination has enough storage for the attribute
            template<class T>
            void copyAttribute(Asset& asset, S32 bufferViewIdx, LLStrider<T>& dst);
            
            const Primitive& operator=(const tinygltf::Primitive& src)
            {
                // load material
                mMaterial = src.material;

                // load mode
                mMode = src.mode;

                // load indices
                mIndices = src.indices;

                // load attributes
                for (auto& it : src.attributes)
                {
                    mAttributes[it.first] = it.second;
                }

                switch (mMode)
                {
                case TINYGLTF_MODE_POINTS:
                    mGLMode = LLRender::POINTS;
                    break;
                case TINYGLTF_MODE_LINE:
                    mGLMode = LLRender::LINES;
                    break;
                case TINYGLTF_MODE_LINE_LOOP:
                    mGLMode = LLRender::LINE_LOOP;
                    break;
                case TINYGLTF_MODE_LINE_STRIP:
                    mGLMode = LLRender::LINE_STRIP;
                    break;
                case TINYGLTF_MODE_TRIANGLES:
                    mGLMode = LLRender::TRIANGLES;
                    break;
                case TINYGLTF_MODE_TRIANGLE_STRIP:
                    mGLMode = LLRender::TRIANGLE_STRIP;
                    break;
                case TINYGLTF_MODE_TRIANGLE_FAN:
                    mGLMode = LLRender::TRIANGLE_FAN;
                    break;
                default:
                    mGLMode = GL_TRIANGLES;
                }

                return *this;
            }

            void allocateGLResources(Asset& asset);
        };

        class Mesh
        {
        public:
            std::vector<Primitive> mPrimitives;
            std::vector<double> mWeights;
            std::string mName;

            const Mesh& operator=(const tinygltf::Mesh& src)
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

            void allocateGLResources(Asset& asset)
            {
                for (auto& primitive : mPrimitives)
                {
                    primitive.allocateGLResources(asset);
                }
            }

        };

        class Node
        {
        public:
            LLMatrix4a mMatrix;
            LLMatrix4a mRenderMatrix;
            std::vector<S32> mChildren;
            S32 mMesh = INVALID_INDEX;
            std::string mName;

            const Node& operator=(const tinygltf::Node& src)
            {
                F32* dstMatrix = mMatrix.getF32ptr();

                if (src.matrix.size() != 16)
                {
                    mMatrix.setIdentity();
                }
                else
                {
                    for (U32 i = 0; i < 16; ++i)
                    {
                        dstMatrix[i] = (F32)src.matrix[i];
                    }
                }
                
                mChildren = src.children;
                mMesh = src.mesh;
                mName = src.name;

                return *this;
            }

            // Set mRenderMatrix to a transform that can be used for the current render pass
            // modelview -- parent's render matrix
            void updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview);
        };

        class Scene
        {
        public:
            std::vector<S32> mNodes;
            std::string mName;

            const Scene& operator=(const tinygltf::Scene& src)
            {
                mNodes = src.nodes;
                mName = src.name;

                return *this;
            }

            void updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview);
            
        };

        class Texture
        {
        public:
            S32 mSampler = INVALID_INDEX;
            S32 mSource = INVALID_INDEX;
            std::string mName;

            const Texture& operator=(const tinygltf::Texture& src)
            {
                mSampler = src.sampler;
                mSource = src.source;
                mName = src.name;

                return *this;
            }
        };

        class Sampler
        {
        public:
            S32 mMagFilter;
            S32 mMinFilter;
            S32 mWrapS;
            S32 mWrapT;
            std::string mName;

            const Sampler& operator=(const tinygltf::Sampler& src)
            {
                mMagFilter = src.magFilter;
                mMinFilter = src.minFilter;
                mWrapS = src.wrapS;
                mWrapT = src.wrapT;
                mName = src.name;

                return *this;
            }
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
        class Asset
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

            void allocateGLResources(const std::string& filename, const tinygltf::Model& model)
            {
                for (auto& mesh : mMeshes)
                {
                    mesh.allocateGLResources(*this);
                }

                for (auto& image : mImages)
                {
                    image.allocateGLResources();
                }

                for (U32 i = 0; i < mMaterials.size(); ++i)
                {
                    mMaterials[i].allocateGLResources(*this);
                    LLTinyGLTFHelper::getMaterialFromModel(filename, model, i, mMaterials[i].mMaterial, mMaterials[i].mName, true);
                }
            }

            void updateRenderTransforms(const LLMatrix4a& modelview)
            {
                for (auto& scene : mScenes)
                {
                    scene.updateRenderTransforms(*this, modelview);
                }
            }

            void renderOpaque()
            {
                for (auto& node : mNodes)
                {
                    if (node.mMesh != INVALID_INDEX)
                    {
                        Mesh& mesh = mMeshes[node.mMesh];
                        for (auto& primitive : mesh.mPrimitives)
                        {
                            gGL.loadMatrix((F32*)node.mRenderMatrix.mMatrix);
                            if (primitive.mMaterial != INVALID_INDEX)
                            {
                                Material& material = mMaterials[primitive.mMaterial];
                                material.mMaterial->bind();
                            }
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
            const Asset& operator=(const tinygltf::Model& src)
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

                return *this;
            }
        };
    }
}
