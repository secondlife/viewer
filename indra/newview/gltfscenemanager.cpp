/**
 * @file gltfscenemanager.cpp
 * @brief Builds menus out of items.
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

#include "gltfscenemanager.h"
#include "llviewermenufile.h"
#include "llappviewer.h"
#include "lltinygltfhelper.h"
#include "llvertexbuffer.h"
#include "llselectmgr.h"
#include "llagent.h"
#include "llvoavatarself.h"

#pragma optimize("", off)

using namespace LL;

// temporary location of LL GLTF Implementation
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
            LLVector4a mMax;
            LLVector4a mMin;
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

                llassert(src.maxValues.size() <= 4);
                F32* dstMax = mMax.getF32ptr();
                for (U32 i = 0; i < src.maxValues.size(); ++i)
                {
                    dstMax[i] = (F32) src.maxValues[i];
                }

                llassert(src.minValues.size() <= 4);
                F32* dstMin = mMin.getF32ptr();
                for (U32 i = 0; i < src.minValues.size(); ++i)
                {
                    dstMin[i] = (F32) src.minValues[i];
                }

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

            const Material& operator=(const tinygltf::Material& src)
            {
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
            S32 mMaterial = INVALID_INDEX;
            U32 mMode = 4; // default to triangles
            U32 mGLMode = LLRender::TRIANGLES;
            S32 mIndices = -1;
            std::unordered_map<std::string, int> mAttributes;

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

            void allocateGLResources()
            {
                for (auto& mesh : mMeshes)
                {
                    mesh.allocateGLResources(*this);
                }

                for (auto& image : mImages)
                {
                    image.allocateGLResources();
                }

                for (auto& material : mMaterials)
                {
                    material.allocateGLResources(*this);
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

        void Scene::updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview)
        {
            for (auto& nodeIndex : mNodes)
            {
                Node& node = asset.mNodes[nodeIndex];
                node.updateRenderTransforms(asset, modelview);
            }
        }

        void Primitive::allocateGLResources(Asset& asset)
        {
            // allocate vertex buffer
            // We diverge from the intent of the GLTF format here to work with our existing render pipeline
            // GLTF wants us to copy the buffer views into GPU storage as is and build render commands that source that data.
            // For our engine, though, it's better to rearrange the buffers at load time into a layout that's more consistent.
            // The GLTF native approach undoubtedly works well if you can count on VAOs, but VAOs perform much worse with our scenes.

            // get the number of vertices
            U32 numVertices = 0;
            for (auto& it : mAttributes)
            {
                const Accessor& accessor = asset.mAccessors[it.second];
                numVertices = accessor.mCount;
                break;
            }

            // get the number of indices
            U32 numIndices = 0;
            if (mIndices != INVALID_INDEX)
            {
                const Accessor& accessor = asset.mAccessors[mIndices];
                numIndices = accessor.mCount;
            }

            // create vertex buffer
            mVertexBuffer = new LLVertexBuffer(ATTRIBUTE_MASK);
            mVertexBuffer->allocateBuffer(numVertices, numIndices);

            bool needs_color = true;
            bool needs_texcoord = true;
            bool needs_normal = true;
            bool needs_tangent = true;

            // load vertex data
            for (auto& it : mAttributes)
            {
                const std::string& attribName = it.first;
                const Accessor& accessor = asset.mAccessors[it.second];
                const BufferView& bufferView = asset.mBufferViews[accessor.mBufferView];
                const Buffer& buffer = asset.mBuffers[bufferView.mBuffer];

                // load vertex data
                if (attribName == "POSITION")
                {
                    // load position data
                    const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

                    LLStrider<LLVector4a> dst;
                    mVertexBuffer->getVertexStrider(dst);

                    if (accessor.mType == TINYGLTF_TYPE_VEC3)
                    {
                        // load vec3 position data
                        for (U32 i = 0; i < numVertices; ++i)
                        {
                            S32 stride = 0;
                            switch (accessor.mComponentType)
                            {
                            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                                (dst++)->load3((F32*)src);
                                stride = sizeof(F32) * 3;
                                break;
                            default:
                                LL_ERRS("GLTF") << "Unsupported component type for POSITION attribute" << LL_ENDL;
                            }

                            if (bufferView.mByteStride == 0)
                            {
                                src += stride;
                            }
                            else
                            {
                                src += bufferView.mByteStride;
                            }
                        }
                    }
                    else
                    {
                        LL_ERRS("GLTF") << "Unsupported type for POSITION attribute" << LL_ENDL;
                    }
                }
                else if (attribName == "NORMAL")
                {
                    needs_normal = false;
                    // load normal data
                    const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

                    LLStrider<LLVector4a> dst;
                    mVertexBuffer->getNormalStrider(dst);

                    if (accessor.mType == TINYGLTF_TYPE_VEC3)
                    {
                        // load vec3 normal data
                        for (U32 i = 0; i < numVertices; ++i)
                        {
                            S32 stride = 0;
                            switch (accessor.mComponentType)
                            {
                            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                                (dst++)->load3((F32*)src);
                                stride = sizeof(F32) * 3;
                                break;
                            default:
                                LL_ERRS("GLTF") << "Unsupported component type for NORMAL attribute" << LL_ENDL;
                            }

                            if (bufferView.mByteStride == 0)
                            {
                                src += stride;
                            }
                            else
                            {
                                src += bufferView.mByteStride;
                            }
                        }
                    }
                    else
                    {
                        LL_ERRS("GLTF") << "Unsupported type for NORMAL attribute" << LL_ENDL;
                    }
                }
                else if (attribName == "TANGENT")
                {
                    needs_tangent = false;
                    // load tangent data
                    const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

                    LLStrider<LLVector4a> dst;
                    mVertexBuffer->getTangentStrider(dst);

                    if (accessor.mType == TINYGLTF_TYPE_VEC4)
                    {
                        // load vec4 tangent data
                        for (U32 i = 0; i < numVertices; ++i)
                        {
                            S32 stride = 0;
                            switch (accessor.mComponentType)
                            {
                            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                                (dst++)->loadua((F32*)src);
                                stride = sizeof(F32) * 4;
                                break;
                            default:
                                LL_ERRS("GLTF") << "Unsupported component type for TANGENT attribute" << LL_ENDL;
                            }

                            if (bufferView.mByteStride == 0)
                            {
                                src += stride;
                            }
                            else
                            {
                                src += bufferView.mByteStride;
                            }
                        }
                    }
                    else
                    {
                        LL_ERRS("GLTF") << "Unsupported type for TANGENT attribute" << LL_ENDL;
                    }
                }
                else if (attribName == "COLOR_0")
                {
                    needs_color = false;
                    // load color data
                    const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

                    LLStrider<LLColor4U> dst;
                    mVertexBuffer->getColorStrider(dst);

                    if (accessor.mType == TINYGLTF_TYPE_VEC4)
                    {
                        // load vec4 color data
                        for (U32 i = 0; i < numVertices; ++i)
                        {
                            S32 stride = 0;
                            switch (accessor.mComponentType)
                            {
                            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                            {
                                LLColor4 c((F32*)src);
                                *(dst++) = c;
                                stride = sizeof(F32) * 4;
                            }
                            break;
                            default:
                                LL_ERRS("GLTF") << "Unsupported component type for COLOR_0 attribute" << LL_ENDL;
                            }

                            if (bufferView.mByteStride == 0)
                            {
                                src += stride;
                            }
                            else
                            {
                                src += bufferView.mByteStride;
                            }
                        }
                    }
                    else
                    {
                        LL_ERRS("GLTF") << "Unsupported type for COLOR_0 attribute" << LL_ENDL;
                    }
                }
                else if (attribName == "TEXCOORD_0")
                {
                    needs_texcoord = false;
                    // load texcoord data
                    const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

                    LLStrider<LLVector2> dst;
                    mVertexBuffer->getTexCoord0Strider(dst);

                    if (accessor.mType == TINYGLTF_TYPE_VEC2)
                    {
                        // load vec2 texcoord data
                        for (U32 i = 0; i < numVertices; ++i)
                        {
                            S32 stride = 0;
                            switch (accessor.mComponentType)
                            {
                            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                                (dst++)->set((F32*)src);
                                stride = sizeof(F32) * 2;
                                break;
                            default:
                                LL_ERRS("GLTF") << "Unsupported component type for TEXCOORD_0 attribute" << LL_ENDL;
                            }

                            if (bufferView.mByteStride == 0)
                            {
                                src += stride;
                            }
                            else
                            {
                                src += bufferView.mByteStride;
                            }
                        }
                    }
                    else
                    {
                        LL_ERRS("GLTF") << "Unsupported type for TEXCOORD_0 attribute" << LL_ENDL;
                    }
                }
            }

            // copy index buffer
            if (mIndices != INVALID_INDEX)
            {
                const Accessor& accessor = asset.mAccessors[mIndices];
                const BufferView& bufferView = asset.mBufferViews[accessor.mBufferView];
                const Buffer& buffer = asset.mBuffers[bufferView.mBuffer];

                const U8* src = buffer.mData.data() + bufferView.mByteOffset + accessor.mByteOffset;

                LLStrider<U16> dst;
                mVertexBuffer->getIndexStrider(dst);

                if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    for (U32 i = 0; i < numIndices; ++i)
                    {
                        *(dst++) = (U16) *(U32*)src;
                        src += sizeof(U32);
                    }
                }
                else if (accessor.mComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    for (U32 i = 0; i < numIndices; ++i)
                    {
                        *(dst++) = *(U16*)src;
                        src += sizeof(U16);
                    }
                }
                else
                {
                    LL_ERRS("GLTF") << "Unsupported component type for indices" << LL_ENDL;
                }
            }

            if (needs_color)
            { // set default color
                LLStrider<LLColor4U> dst;
                mVertexBuffer->getColorStrider(dst);
                for (U32 i = 0; i < numVertices; ++i)
                {
                    *(dst++) = LLColor4U(255, 255, 255, 255);
                }
            }

            if (needs_texcoord)
            { // set default texcoord
                LLStrider<LLVector2> dst;
                mVertexBuffer->getTexCoord0Strider(dst);
                for (U32 i = 0; i < numVertices; ++i)
                {
                    *(dst++) = LLVector2(0.0f, 0.0f);
                }
            }

            if (needs_normal)
            { // set default normal
                LLStrider<LLVector4a> dst;
                mVertexBuffer->getNormalStrider(dst);
                for (U32 i = 0; i < numVertices; ++i)
                {
                    *(dst++) = LLVector4a(0.0f, 0.0f, 1.0f, 0.0f);
                }
            }

            if (needs_tangent)
            {  // TODO -- generate tangents
                LLStrider<LLVector4a> dst;
                mVertexBuffer->getTangentStrider(dst);
                for (U32 i = 0; i < numVertices; ++i)
                {
                    *(dst++) = LLVector4a(1.0f, 0.0f, 0.0f, 1.0f);
                }
            }

            mVertexBuffer->unmapBuffer();
        }
    }
}

using namespace LL::GLTF;

Asset gGLTFAsset;
LLPointer<LLViewerObject> gGLTFObject;

void GLTFSceneManager::load()
{
    // Load a scene from disk
    LLFilePickerReplyThread::startPicker(
        [](const std::vector<std::string>& filenames, LLFilePicker::ELoadFilter load_filter, LLFilePicker::ESaveFilter save_filter)
        {
            if (LLAppViewer::instance()->quitRequested())
            {
                return;
            }
            if (filenames.size() > 0)
            {
                GLTFSceneManager::instance().load(filenames[0]);
            }
        },
        LLFilePicker::FFLOAD_GLTF,
        true);
}

void GLTFSceneManager::load(const std::string& filename)
{
    tinygltf::Model model;
    LLTinyGLTFHelper::loadModel(filename, model);

    gGLTFAsset = model;

    gGLTFAsset.allocateGLResources();

    // hang the asset off the currently selected object, or off of the avatar if no object is selected
    gGLTFObject = LLSelectMgr::instance().getSelection()->getFirstRootObject();

    if (gGLTFObject.isNull())
    { // assign to self avatar
        gGLTFObject = gAgentAvatarp;
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


void GLTFSceneManager::renderOpaque()
{
    // for debugging, just render the whole scene as opaque
    // by traversing the whole scenegraph
    // Assumes camera transform is already set and 
    // appropriate shader is already bound
    if (gGLTFObject.isNull())
    {
        return;
    }

    if (gGLTFObject->isDead())
    {
        gGLTFObject = nullptr;
        return;
    }

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();

    LLMatrix4a modelview;
    modelview.loadu(gGLModelView);

    LLMatrix4a root;
    root.loadu((F32*) gGLTFObject->getRenderMatrix().mMatrix);

    matMul(root, modelview, modelview);

    gGLTFAsset.updateRenderTransforms(modelview);

    gGLTFAsset.renderOpaque();

    gGL.popMatrix();
}


