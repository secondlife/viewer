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
#include <boost/url.hpp>
#include "llimagejpeg.h"
#include "../llskinningutil.h"

using namespace LL::GLTF;
using namespace boost::json;


namespace LL
{
    namespace GLTF
    {
        static std::unordered_set<std::string> ExtensionsSupported = {
            "KHR_materials_unlit",
            "KHR_texture_transform"
        };

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

void Node::updateTransforms(Asset& asset, const mat4& parentMatrix)
{
    makeMatrixValid();
    mAssetMatrix = parentMatrix * mMatrix;

    mAssetMatrixInv = glm::inverse(mAssetMatrix);

    S32 my_index = (S32)(this - &asset.mNodes[0]);

    for (auto& childIndex : mChildren)
    {
        Node& child = asset.mNodes[childIndex];
        child.mParent = my_index;
        child.updateTransforms(asset, mAssetMatrix);
    }
}

void Asset::updateTransforms()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_GLTF;
    for (auto& scene : mScenes)
    {
        scene.updateTransforms(*this);
    }

    uploadTransforms();
}

void Asset::uploadTransforms()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_GLTF;
    // prepare matrix palette
    U32 max_nodes = LLSkinningUtil::getMaxGLTFJointCount();

    size_t node_count = llmin<size_t>(max_nodes, mNodes.size());

    std::vector<mat4> t_mp;

    t_mp.resize(node_count);

    for (U32 i = 0; i < node_count; ++i)
    {
        Node& node = mNodes[i];
        // build matrix palette in asset space
        t_mp[i] = node.mAssetMatrix;
    }

    std::vector<F32> glmp;

    glmp.resize(node_count * 12);

    F32* mp = glmp.data();

    for (U32 i = 0; i < node_count; ++i)
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

    if (mNodesUBO == 0)
    {
        glGenBuffers(1, &mNodesUBO);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, mNodesUBO);
    glBufferData(GL_UNIFORM_BUFFER, glmp.size() * sizeof(F32), glmp.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Asset::uploadMaterials()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_GLTF;
    // see pbrmetallicroughnessV.glsl for the layout of the material UBO
    std::vector<vec4> md;

    U32 material_size = sizeof(vec4) * 12;
    U32 max_materials = gGLManager.mMaxUniformBlockSize / material_size;

    U32 mat_count = (U32)mMaterials.size();
    mat_count = llmin(mat_count, max_materials);

    md.resize(mat_count * 12);

    for (U32 i = 0; i < mat_count*12; i += 12)
    {
        Material& material = mMaterials[i/12];

        // add texture transforms and UV indices
        material.mPbrMetallicRoughness.mBaseColorTexture.mTextureTransform.getPacked(&md[i+0]);
        md[i + 1].g = (F32)material.mPbrMetallicRoughness.mBaseColorTexture.getTexCoord();
        material.mNormalTexture.mTextureTransform.getPacked(&md[i + 2]);
        md[i + 3].g = (F32)material.mNormalTexture.getTexCoord();
        material.mPbrMetallicRoughness.mMetallicRoughnessTexture.mTextureTransform.getPacked(&md[i+4]);
        md[i + 5].g = (F32)material.mPbrMetallicRoughness.mMetallicRoughnessTexture.getTexCoord();
        material.mEmissiveTexture.mTextureTransform.getPacked(&md[i + 6]);
        md[i + 7].g = (F32)material.mEmissiveTexture.getTexCoord();
        material.mOcclusionTexture.mTextureTransform.getPacked(&md[i + 8]);
        md[i + 9].g = (F32)material.mOcclusionTexture.getTexCoord();

        // add material properties
        F32 min_alpha = material.mAlphaMode == Material::AlphaMode::MASK ? material.mAlphaCutoff : -1.0f;
        md[i + 10] = vec4(material.mEmissiveFactor, 1.f);
        md[i + 11] = vec4(0.f,
            material.mPbrMetallicRoughness.mRoughnessFactor,
            material.mPbrMetallicRoughness.mMetallicFactor,
            min_alpha);
    }

    if (mMaterialsUBO == 0)
    {
        glGenBuffers(1, &mMaterialsUBO);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, mMaterialsUBO);
    glBufferData(GL_UNIFORM_BUFFER, md.size() * sizeof(vec4), md.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
                    node_hit = (S32)(&node - &mNodes[0]);
                    llassert(&mNodes[node_hit] == &node);

                    //pointer math to get the primitive index
                    primitive_hit = (S32)(&primitive - &mesh.mPrimitives[0]);
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
    write(mRotation, "rotation", dst, glm::identity<quat>());
    write(mTranslation, "translation", dst, glm::vec3(0.f, 0.f, 0.f));
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

void Asset::update()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_GLTF;
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

        for (auto& skin : mSkins)
        {
            skin.uploadMatrixPalette(*this);
        }

        uploadMaterials();

        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_GLTF("gltf - addTextureStats");

            for (auto& image : mImages)
            {
                if (image.mTexture.notNull())
                { // HACK - force texture to be loaded full rez
                    // TODO: calculate actual vsize
                    image.mTexture->addTextureStats(2048.f * 2048.f);
                    image.mTexture->setBoostLevel(LLViewerTexture::BOOST_HIGH);
                }
            }
        }
    }
}

bool Asset::prep()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_GLTF;
    // check required extensions and fail if not supported
    bool unsupported = false;
    for (auto& extension : mExtensionsRequired)
    {
        if (ExtensionsSupported.find(extension) == ExtensionsSupported.end())
        {
            LL_WARNS() << "Unsupported extension: " << extension << LL_ENDL;
            unsupported = true;
        }
    }

    if (unsupported)
    {
        return false;
    }

    // do buffers first as other resources depend on them
    for (auto& buffer : mBuffers)
    {
        if (!buffer.prep(*this))
        {
            return false;
        }
    }

    for (auto& image : mImages)
    {
        if (!image.prep(*this))
        {
            return false;
        }
    }

    for (auto& mesh : mMeshes)
    {
        if (!mesh.prep(*this))
        {
            return false;
        }
    }

    for (auto& animation : mAnimations)
    {
        if (!animation.prep(*this))
        {
            return false;
        }
    }

    for (auto& skin : mSkins)
    {
        if (!skin.prep(*this))
        {
            return false;
        }
    }

    // prepare vertex buffers

    // material count is number of materials + 1 for default material
    U32 mat_count = (U32) mMaterials.size() + 1;

    if (LLGLSLShader::sCurBoundShaderPtr == nullptr)
    { // make sure a shader is bound to satisfy mVertexBuffer->setBuffer
        gDebugProgram.bind();
    }

    for (S32 double_sided = 0; double_sided < 2; ++double_sided)
    {
        RenderData& rd = mRenderData[double_sided];
        for (U32 i = 0; i < LLGLSLShader::NUM_GLTF_VARIANTS; ++i)
        {
            rd.mBatches[i].resize(mat_count);
        }

        // for each material
        for (S32 mat_id = -1; mat_id < (S32)mMaterials.size(); ++mat_id)
        {
            // for each shader variant
            U32 vertex_count[LLGLSLShader::NUM_GLTF_VARIANTS] = { 0 };
            U32 index_count[LLGLSLShader::NUM_GLTF_VARIANTS] = { 0 };

            S32 ds_mat = mat_id == -1 ? 0 : mMaterials[mat_id].mDoubleSided;
            if (ds_mat != double_sided)
            {
                continue;
            }

            for (U32 variant = 0; variant < LLGLSLShader::NUM_GLTF_VARIANTS; ++variant)
            {
                U32 attribute_mask = 0;
                // for each mesh
                for (auto& mesh : mMeshes)
                {
                    // for each primitive
                    for (auto& primitive : mesh.mPrimitives)
                    {
                        if (primitive.mMaterial == mat_id && primitive.mShaderVariant == variant)
                        {
                            // accumulate vertex and index counts
                            primitive.mVertexOffset = vertex_count[variant];
                            primitive.mIndexOffset = index_count[variant];

                            vertex_count[variant] += primitive.getVertexCount();
                            index_count[variant] += primitive.getIndexCount();

                            // all primitives of a given variant and material should all have the same attribute mask
                            llassert(attribute_mask == 0 || primitive.mAttributeMask == attribute_mask);
                            attribute_mask |= primitive.mAttributeMask;
                        }
                    }
                }

                // allocate vertex buffer and pack it
                if (vertex_count[variant] > 0)
                {
                    U32 mat_idx = mat_id + 1;
                    LLVertexBuffer* vb = new LLVertexBuffer(attribute_mask);

                    rd.mBatches[variant][mat_idx].mVertexBuffer = vb;
                    vb->allocateBuffer(vertex_count[variant],
                        index_count[variant] * 2); // hack double index count... TODO: find a better way to indicate 32-bit indices will be used
                    vb->setBuffer();

                    for (auto& mesh : mMeshes)
                    {
                        for (auto& primitive : mesh.mPrimitives)
                        {
                            if (primitive.mMaterial == mat_id && primitive.mShaderVariant == variant)
                            {
                                primitive.upload(vb);
                            }
                        }
                    }

                    vb->unmapBuffer();

                    vb->unbind();
                }
            }
        }
    }

    // sanity check that all primitives have a vertex buffer
    for (auto& mesh : mMeshes)
    {
        for (auto& primitive : mesh.mPrimitives)
        {
            llassert(primitive.mVertexBuffer.notNull());
        }
    }

    // build render batches
    for (S32 node_id = 0; node_id < mNodes.size(); ++node_id)
    {
        Node& node = mNodes[node_id];

        if (node.mMesh != INVALID_INDEX)
        {
            auto& mesh = mMeshes[node.mMesh];

            S32 mat_idx = mesh.mPrimitives[0].mMaterial + 1;

            S32 double_sided = mat_idx == 0 ? 0 : mMaterials[mat_idx - 1].mDoubleSided;

            for (S32 j = 0; j < mesh.mPrimitives.size(); ++j)
            {
                auto& primitive = mesh.mPrimitives[j];

                S32 variant = primitive.mShaderVariant;

                RenderData& rd = mRenderData[double_sided];
                RenderBatch& rb = rd.mBatches[variant][mat_idx];

                rb.mPrimitives.push_back({ j, node_id });
            }
        }
    }
    return true;
}

Asset::Asset(const Value& src)
{
    *this = src;
}

bool Asset::load(std::string_view filename)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_GLTF;
    mFilename = filename;
    std::string ext = gDirUtilp->getExtension(mFilename);

    std::ifstream file(filename.data(), std::ios::binary);
    if (file.is_open())
    {
        std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        if (ext == "gltf")
        {
            Value val = parse(str);
            *this = val;
            return prep();
        }
        else if (ext == "glb")
        {
            return loadBinary(str);
        }
        else
        {
            LL_WARNS() << "Unsupported file type: " << ext << LL_ENDL;
            return false;
        }
    }
    else
    {
        LL_WARNS() << "Failed to open file: " << filename << LL_ENDL;
        return false;
    }

    return false;
}

bool Asset::loadBinary(const std::string& data)
{
    // load from binary gltf
    const U8* ptr = (const U8*)data.data();
    const U8* end = ptr + data.size();

    if (end - ptr < 12)
    {
        LL_WARNS("GLTF") << "GLB file too short" << LL_ENDL;
        return false;
    }

    U32 magic = *(U32*)ptr;
    ptr += 4;

    if (magic != 0x46546C67)
    {
        LL_WARNS("GLTF") << "Invalid GLB magic" << LL_ENDL;
        return false;
    }

    U32 version = *(U32*)ptr;
    ptr += 4;

    if (version != 2)
    {
        LL_WARNS("GLTF") << "Unsupported GLB version" << LL_ENDL;
        return false;
    }

    U32 length = *(U32*)ptr;
    ptr += 4;

    if (length != data.size())
    {
        LL_WARNS("GLTF") << "GLB length mismatch" << LL_ENDL;
        return false;
    }

    U32 chunkLength = *(U32*)ptr;
    ptr += 4;

    if (end - ptr < chunkLength + 8)
    {
        LL_WARNS("GLTF") << "GLB chunk too short" << LL_ENDL;
        return false;
    }

    U32 chunkType = *(U32*)ptr;
    ptr += 4;

    if (chunkType != 0x4E4F534A)
    {
        LL_WARNS("GLTF") << "Invalid GLB chunk type" << LL_ENDL;
        return false;
    }

    Value val = parse(std::string_view((const char*)ptr, chunkLength));
    *this = val;

    if (mBuffers.size() > 0 && mBuffers[0].mUri.empty())
    {
        // load binary chunk
        ptr += chunkLength;

        if (end - ptr < 8)
        {
            LL_WARNS("GLTF") << "GLB chunk too short" << LL_ENDL;
            return false;
        }

        chunkLength = *(U32*)ptr;
        ptr += 4;

        chunkType = *(U32*)ptr;
        ptr += 4;

        if (chunkType != 0x004E4942)
        {
            LL_WARNS("GLTF") << "Invalid GLB chunk type" << LL_ENDL;
            return false;
        }

        auto& buffer = mBuffers[0];

        if (ptr + buffer.mByteLength <= end)
        {
            buffer.mData.resize(buffer.mByteLength);
            memcpy(buffer.mData.data(), ptr, buffer.mByteLength);
            ptr += buffer.mByteLength;
        }
        else
        {
            LL_WARNS("GLTF") << "Buffer too short" << LL_ENDL;
            return false;
        }
    }

    return prep();
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

        copy(obj, "scene", mScene);
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
        copy(obj, "extensionsUsed", mExtensionsUsed);
        copy(obj, "extensionsRequired", mExtensionsRequired);
    }

    return *this;
}

void Asset::serialize(object& dst) const
{
    static const std::string sGenerator = "Linden Lab GLTF Prototype v0.1";

    dst["asset"] = object{};
    object& asset = dst["asset"].get_object();

    write(mVersion, "version", asset);
    write(mMinVersion, "minVersion", asset, std::string());
    write(sGenerator, "generator", asset);
    write(mScene, "scene", dst, INVALID_INDEX);
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
    write(mExtensionsUsed, "extensionsUsed", dst);
    write(mExtensionsRequired, "extensionsRequired", dst);
}

bool Asset::save(const std::string& filename)
{
    // get folder path
    std::string folder = gDirUtilp->getDirName(filename);

    // save images
    for (auto& image : mImages)
    {
        if (!image.save(*this, folder))
        {
            return false;
        }
    }

    // save buffers
    // NOTE: save buffers after saving images as saving images
    // may remove image data from buffers
    for (auto& buffer : mBuffers)
    {
        if (!buffer.save(*this, folder))
        {
            return false;
        }
    }

    // save .gltf
    object obj;
    serialize(obj);
    std::string buffer = boost::json::serialize(obj, {});
    std::ofstream file(filename, std::ios::binary);
    file.write(buffer.c_str(), buffer.size());

    return true;
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

bool Image::prep(Asset& asset)
{
    LLUUID id;
    if (mUri.size() == UUID_STR_SIZE && LLUUID::parseUUID(mUri, &id) && id.notNull())
    { // loaded from an asset, fetch the texture from the asset system
        mTexture = fetch_texture(id);
    }
    else if (mUri.find("data:") == 0)
    { // embedded in a data URI, load the texture from the URI
        LL_WARNS() << "Data URIs not yet supported" << LL_ENDL;
        return false;
    }
    else if (mBufferView != INVALID_INDEX)
    { // embedded in a buffer, load the texture from the buffer
        BufferView& bufferView = asset.mBufferViews[mBufferView];
        Buffer& buffer = asset.mBuffers[bufferView.mBuffer];

        U8* data = buffer.mData.data() + bufferView.mByteOffset;

        mTexture = LLViewerTextureManager::getFetchedTextureFromMemory(data, bufferView.mByteLength, mMimeType);

        if (mTexture.isNull())
        {
            LL_WARNS("GLTF") << "Failed to load image from buffer:" << LL_ENDL;
            LL_WARNS("GLTF") << "  image: " << mName << LL_ENDL;
            LL_WARNS("GLTF") << "  mimeType: " << mMimeType << LL_ENDL;

            return false;
        }
    }
    else if (!asset.mFilename.empty() && !mUri.empty())
    { // loaded locally and not embedded, load the texture as a local preview
        std::string dir = gDirUtilp->getDirName(asset.mFilename);
        std::string img_file = dir + gDirUtilp->getDirDelimiter() + mUri;

        LLUUID tracking_id = LLLocalBitmapMgr::getInstance()->addUnit(img_file);
        if (tracking_id.notNull())
        {
            LLUUID world_id = LLLocalBitmapMgr::getInstance()->getWorldID(tracking_id);
            mTexture = LLViewerTextureManager::getFetchedTexture(world_id);
        }
        else
        {
            LL_WARNS("GLTF") << "Failed to load image from file:" << LL_ENDL;
            LL_WARNS("GLTF") << "  image: " << mName << LL_ENDL;
            LL_WARNS("GLTF") << "  file: " << img_file << LL_ENDL;

            return false;
        }
    }
    else
    {
        LL_WARNS("GLTF") << "Failed to load image: " << mName << LL_ENDL;
        return false;
    }

    if (!asset.mFilename.empty())
    { // local preview, boost image so it doesn't discard and force to save raw image in case we save out or upload
        mTexture->setBoostLevel(LLViewerTexture::BOOST_PREVIEW);
        mTexture->forceToSaveRawImage(0, F32_MAX);
    }

    return true;
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

    mBufferView = INVALID_INDEX;
    mWidth = -1;
    mHeight = -1;
    mComponent = -1;
    mBits = -1;
    mPixelType = -1;
    mMimeType = "";
}

bool Image::save(Asset& asset, const std::string& folder)
{
    // NOTE:  this *MUST* be a lossless save
    // Artists use this to save their work repeatedly, so
    // adding any compression artifacts here will degrade
    // images over time.
    std::string name = mName;
    std::string error;
    const std::string& delim = gDirUtilp->getDirDelimiter();
    if (name.empty())
    {
        S32 idx = (S32)(this - asset.mImages.data());
        name = llformat("image_%d", idx);
    }

    if (mBufferView != INVALID_INDEX)
    {
        // we have the bytes of the original image, save that out in its
        // original format
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
            error = "Unknown mime type, saved as .bin";
            extension = ".bin";
        }

        std::string filename = folder + delim + name + extension;

        // set URI to non-j2c file for now, but later we'll want to reference the j2c hash
        mUri = name + extension;

        std::ofstream file(filename, std::ios::binary);
        file.write((const char*)buffer.mData.data() + bufferView.mByteOffset, bufferView.mByteLength);
    }
    else if (mTexture.notNull())
    {
        auto bitmapmgr = LLLocalBitmapMgr::getInstance();
        if (bitmapmgr->isLocal(mTexture->getID()))
        {
            LLUUID tracking_id = bitmapmgr->getTrackingID(mTexture->getID());
            if (tracking_id.notNull())
            { // copy original file to destination folder
                std::string source = bitmapmgr->getFilename(tracking_id);
                if (gDirUtilp->fileExists(source))
                {
                    std::string filename = gDirUtilp->getBaseFileName(source);
                    std::string dest = folder + delim + filename;

                    LLFile::copy(source, dest);
                    mUri = filename;
                }
                else
                {
                    error = "File not found: " + source;
                }
            }
            else
            {
                error = "Local image missing.";
            }
        }
        else if (!mUri.empty())
        {
            std::string from_dir = gDirUtilp->getDirName(asset.mFilename);
            std::string base_filename = gDirUtilp->getBaseFileName(mUri);
            std::string filename = from_dir + delim + base_filename;
            if (gDirUtilp->fileExists(filename))
            {
                std::string dest = folder + delim + base_filename;
                LLFile::copy(filename, dest);
                mUri = base_filename;
            }
            else
            {
                error = "Original image file not found: " + filename;
            }
        }
        else
        {
            error = "Image is not a local image and has no uri, cannot save.";
        }
    }

    if (!error.empty())
    {
        LL_WARNS("GLTF") << "Failed to save " << name << ": " << error << LL_ENDL;
        return false;
    }

    clearData(asset);

    return true;
}

void TextureInfo::serialize(object& dst) const
{
    write(mIndex, "index", dst, INVALID_INDEX);
    write(mTexCoord, "texCoord", dst, 0);
    write_extensions(dst, &mTextureTransform, "KHR_texture_transform");
}

S32 TextureInfo::getTexCoord() const
{
    if (mTextureTransform.mPresent && mTextureTransform.mTexCoord != INVALID_INDEX)
    {
        return mTextureTransform.mTexCoord;
    }
    return mTexCoord;
}

bool Material::isMultiUV() const
{
    return mPbrMetallicRoughness.mBaseColorTexture.getTexCoord() != 0 ||
        mPbrMetallicRoughness.mMetallicRoughnessTexture.getTexCoord() != 0 ||
        mNormalTexture.getTexCoord() != 0 ||
        mOcclusionTexture.getTexCoord() != 0 ||
        mEmissiveTexture.getTexCoord() != 0;
}

const TextureInfo& TextureInfo::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "index", mIndex);
        copy(src, "texCoord", mTexCoord);
        copy_extensions(src, "KHR_texture_transform", &mTextureTransform);
    }

    return *this;
}

bool TextureInfo::operator==(const TextureInfo& rhs) const
{
    return mIndex == rhs.mIndex && mTexCoord == rhs.mTexCoord;
}

bool TextureInfo::operator!=(const TextureInfo& rhs) const
{
    return !(*this == rhs);
}

void OcclusionTextureInfo::serialize(object& dst) const
{
    TextureInfo::serialize(dst);
    write(mStrength, "strength", dst, 1.f);
}

const OcclusionTextureInfo& OcclusionTextureInfo::operator=(const Value& src)
{
    TextureInfo::operator=(src);

    if (src.is_object())
    {
        copy(src, "strength", mStrength);
    }

    return *this;
}

void NormalTextureInfo::serialize(object& dst) const
{
    TextureInfo::serialize(dst);
    write(mScale, "scale", dst, 1.f);
}

const NormalTextureInfo& NormalTextureInfo::operator=(const Value& src)
{
    TextureInfo::operator=(src);
    if (src.is_object())
    {
        copy(src, "index", mIndex);
        copy(src, "texCoord", mTexCoord);
        copy(src, "scale", mScale);
    }

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

const Material::Unlit& Material::Unlit::operator=(const Value& src)
{
    mPresent = true;
    return *this;
}

void Material::Unlit::serialize(object& dst) const
{
    // no members and object has already been created, nothing to do
}

void TextureTransform::getPacked(vec4* packed) const
{
    packed[0] = vec4(mScale.x, mScale.y, mRotation, mOffset.x);
    packed[1] = vec4(mOffset.y, 0.f, 0.f, 0.f);
}

const TextureTransform& TextureTransform::operator=(const Value& src)
{
    mPresent = true;
    if (src.is_object())
    {
        copy(src, "offset", mOffset);
        copy(src, "rotation", mRotation);
        copy(src, "scale", mScale);
        copy(src, "texCoord", mTexCoord);
    }

    return *this;
}

void TextureTransform::serialize(object& dst) const
{
    write(mOffset, "offset", dst, vec2(0.f, 0.f));
    write(mRotation, "rotation", dst, 0.f);
    write(mScale, "scale", dst, vec2(1.f, 1.f));
    write(mTexCoord, "texCoord", dst, -1);
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
    write_extensions(dst, &mUnlit, "KHR_materials_unlit");
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
        copy_extensions(src,
            "KHR_materials_unlit", &mUnlit );
    }
    return *this;
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

bool Mesh::prep(Asset& asset)
{
    for (auto& primitive : mPrimitives)
    {
        if (!primitive.prep(asset))
        {
            return false;
        }
    }

    return true;
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


