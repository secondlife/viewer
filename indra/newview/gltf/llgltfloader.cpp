/**
 * @file LLGLTFLoader.cpp
 * @brief LLGLTFLoader class implementation
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#include "llgltfloader.h"
#include "meshoptimizer.h"

// Import & define single-header gltf import/export lib
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14  // default is C++ 11

// tinygltf by default loads image files using STB
#define STB_IMAGE_IMPLEMENTATION
// to use our own image loading:
// 1. replace this definition with TINYGLTF_NO_STB_IMAGE
// 2. provide image loader callback with TinyGLTF::SetImageLoader(LoadimageDataFunction LoadImageData, void *user_data)

// tinygltf saves image files using STB
#define STB_IMAGE_WRITE_IMPLEMENTATION
// similarly, can override with TINYGLTF_NO_STB_IMAGE_WRITE and TinyGLTF::SetImageWriter(fxn, data)

// Additionally, disable inclusion of STB header files entirely with
// TINYGLTF_NO_INCLUDE_STB_IMAGE
// TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "tinygltf/tiny_gltf.h"


// TODO: includes inherited from dae loader.  Validate / prune

#include "llsdserialize.h"
#include "lljoint.h"

#include "llmatrix4a.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>

static const std::string lod_suffix[LLModel::NUM_LODS] =
{
    "_LOD0",
    "_LOD1",
    "_LOD2",
    "",
    "_PHYS",
};


LLGLTFLoader::LLGLTFLoader(std::string filename,
    S32                                 lod,
    LLModelLoader::load_callback_t      load_cb,
    LLModelLoader::joint_lookup_func_t  joint_lookup_func,
    LLModelLoader::texture_load_func_t  texture_load_func,
    LLModelLoader::state_callback_t     state_cb,
    void *                              opaque_userdata,
    JointTransformMap &                 jointTransformMap,
    JointNameSet &                      jointsFromNodes,
    std::map<std::string, std::string> &jointAliasMap,
    U32                                 maxJointsPerMesh,
    U32                                 modelLimit) //,
    //bool                                preprocess)
    : LLModelLoader( filename,
                     lod,
                     load_cb,
                     joint_lookup_func,
                     texture_load_func,
                     state_cb,
                     opaque_userdata,
                     jointTransformMap,
                     jointsFromNodes,
                     jointAliasMap,
                     maxJointsPerMesh ),
    //mPreprocessGLTF(preprocess),
    mMeshesLoaded(false),
    mMaterialsLoaded(false)
{
}

LLGLTFLoader::~LLGLTFLoader() {}

bool LLGLTFLoader::OpenFile(const std::string &filename)
{
    tinygltf::TinyGLTF loader;
    std::string        error_msg;
    std::string        warn_msg;
    std::string filename_lc(filename);
    LLStringUtil::toLower(filename_lc);

    mGltfLoaded = mGLTFAsset.load(filename);

    if (!mGltfLoaded)
    {
        if (!warn_msg.empty())
            LL_WARNS("GLTF_IMPORT") << "gltf load warning: " << warn_msg.c_str() << LL_ENDL;
        if (!error_msg.empty())
            LL_WARNS("GLTF_IMPORT") << "gltf load error: " << error_msg.c_str() << LL_ENDL;
        return false;
    }

    mMeshesLoaded = parseMeshes();
    if (mMeshesLoaded) uploadMeshes();

    /*
    mMaterialsLoaded = parseMaterials();
    if (mMaterialsLoaded) uploadMaterials();
    */

    setLoadState(DONE);

    return (mMeshesLoaded);
}

bool LLGLTFLoader::parseMeshes()
{
    if (!mGltfLoaded) return false;

    // 2022-04 DJH Volume params from dae example. TODO understand PCODE
    LLVolumeParams volume_params;
    volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);

    for (auto node : mGLTFAsset.mNodes)
    {
        LLMatrix4    transform;
        material_map mats;
        auto meshidx = node.mMesh;

        if (meshidx >= 0)
        {
            LLModel* pModel = new LLModel(volume_params, 0.f);
            auto mesh = mGLTFAsset.mMeshes[meshidx];

            if (populateModelFromMesh(pModel, mesh, mats) && (LLModel::NO_ERRORS == pModel->getStatus()) && validate_model(pModel))
            {
                mModelList.push_back(pModel);
                LLVector3 mesh_scale_vector = LLVector3(node.mScale);
                LLVector3 mesh_translation_vector = LLVector3(node.mTranslation);

                LLMatrix4 mesh_translation;
                mesh_translation.setTranslation(mesh_translation_vector);
                mesh_translation *= transform;
                transform = LLMatrix4((float*)&node.mAssetMatrix[0][0]);
                /*
                LLMatrix4 mesh_scale;
                mesh_scale.initScale(mesh_scale_vector);
                mesh_scale *= transform;
                transform = mesh_scale;
                */

                mScene[transform].push_back(LLModelInstance(pModel, node.mName, transform, mats));
            }
            else
            {
                setLoadState(ERROR_MODEL + pModel->getStatus());
                delete (pModel);
                return false;
            }
        }
    }

    return true;
}
 
bool LLGLTFLoader::populateModelFromMesh(LLModel* pModel, const LL::GLTF::Mesh &mesh, material_map &mats)
{
    pModel->mLabel = mesh.mName;
    pModel->ClearFacesAndMaterials();

    auto prims = mesh.mPrimitives;
    for (auto prim : prims)
    {
        // Unfortunately, SLM does not support 32 bit indices.  Filter out anything that goes beyond 16 bit.
        if (prim.getVertexCount() < USHRT_MAX)
        {
            // So primitives already have all of the data we need for a given face in SL land.
            // Primitives may only ever have a single material assigned to them - as the relation is 1:1 in terms of intended draw call
            // count. Just go ahead and populate faces direct from the GLTF primitives here. -Geenz 2025-04-07
            LLVolumeFace                          face;
            LLVolumeFace::VertexMapData::PointMap point_map;

            std::vector<GLTFVertex> vertices;
            std::vector<U16>        indices;

            LLImportMaterial impMat;

            LL::GLTF::Material* material = nullptr;

            if (prim.mMaterial >= 0)
                material = &mGLTFAsset.mMaterials[prim.mMaterial];

            impMat.mDiffuseColor = LLColor4::white;

            for (U32 i = 0; i < prim.getVertexCount(); i++)
            {
                GLTFVertex vert;
                vert.position = glm::vec3(prim.mPositions[i][0], prim.mPositions[i][1], prim.mPositions[i][2]);
                vert.normal   = glm::vec3(prim.mNormals[i][0], prim.mNormals[i][1], prim.mNormals[2][i]);
                vert.uv0      = glm::vec2(prim.mTexCoords0[i][0], prim.mTexCoords0[i][1]);
                vertices.push_back(vert);
            }

            for (U32 i = 0; i < prim.getIndexCount(); i++)
            {
                indices.push_back(prim.mIndexArray[i]);
            }

            std::vector<LLVolumeFace::VertexData> faceVertices;

            for (U32 i = 0; i < vertices.size(); i++)
            {
                LLVolumeFace::VertexData vert;
                LLVector4a               position = LLVector4a(vertices[i].position.x, vertices[i].position.y, vertices[i].position.z);
                LLVector4a               normal   = LLVector4a(vertices[i].normal.x, vertices[i].normal.y, vertices[i].normal.z);
                vert.setPosition(position);
                vert.setNormal(normal);
                vert.mTexCoord = LLVector2(vertices[i].uv0.x, vertices[i].uv0.y);
                faceVertices.push_back(vert);
            }

            face.fillFromLegacyData(faceVertices, indices);

            pModel->getVolumeFaces().push_back(face);
            pModel->getMaterialList().push_back("mat" + std::to_string(prim.mMaterial));
            mats["mat" + std::to_string(prim.mMaterial)] = impMat;
        }
    }

    return true;
}
/*
LLModel::EModelStatus loadFaceFromGLTFModel(LLModel* pModel, const LL::GLTF::Mesh& mesh, material_map& mats, LLSD& log_msg)
{
    LLVolumeFace                          face;
    std::vector<LLVolumeFace::VertexData> verts;
    std::vector<U16>                      indices;

    S32 pos_offset  = -1;
    S32 tc_offset   = -1;
    S32 norm_offset = -1;

    auto pos_source = mesh.mPrimitives[0].mPositions;
    auto tc_source  = mesh.mPrimitives[0].mNormals;
    auto norm_source = mesh.mPrimitives[0].mTexCoords0;

    S32 idx_stride = 0;

    if (pos_source.size() > USHRT_MAX)
    {
        LL_WARNS() << "Unable to process mesh due to 16-bit index limits; invalid model;  invalid model." << LL_ENDL;
        LLSD args;
        args["Message"] = "ParsingErrorBadElement";
        log_msg.append(args);
        return LLModel::BAD_ELEMENT;

    }

    std::vector<U32> idx = mesh.mPrimitives[0].mIndexArray;

    domListOfFloats  dummy;
    domListOfFloats& v  = pos_source ? pos_source->getFloat_array()->getValue() : dummy;
    domListOfFloats& tc = tc_source ? tc_source->getFloat_array()->getValue() : dummy;
    domListOfFloats& n  = norm_source ? norm_source->getFloat_array()->getValue() : dummy;

    if (pos_source.size() == 0)
    {
        return LLModel::BAD_ELEMENT;
    }

    // VFExtents change
    face.mExtents[0].set(pos_source[0][0], pos_source[0][1], pos_source[0][2]);
    face.mExtents[1].set(pos_source[0][0], pos_source[0][1], pos_source[0][2]);

    LLVolumeFace::VertexMapData::PointMap point_map;

    if (idx_stride <= 0 || (pos_source && pos_offset >= idx_stride) || (tc_source && tc_offset >= idx_stride) ||
        (norm_source && norm_offset >= idx_stride))
    {
        // Looks like these offsets should fit inside idx_stride
        // Might be good idea to also check idx.getCount()%idx_stride != 0
        LL_WARNS() << "Invalid pos_offset " << pos_offset << ", tc_offset " << tc_offset << " or norm_offset " << norm_offset << LL_ENDL;
        return LLModel::BAD_ELEMENT;
    }

    for (U32 i = 0; i < idx.getCount(); i += idx_stride)
    {
        LLVolumeFace::VertexData cv;
        if (pos_source)
        {
            cv.setPosition(
                LLVector4a((F32)v[idx[i + pos_offset] * 3 + 0], (F32)v[idx[i + pos_offset] * 3 + 1], (F32)v[idx[i + pos_offset] * 3 + 2]));
        }

        if (tc_source)
        {
            cv.mTexCoord.setVec((F32)tc[idx[i + tc_offset] * 2 + 0], (F32)tc[idx[i + tc_offset] * 2 + 1]);
        }

        if (norm_source)
        {
            cv.setNormal(LLVector4a((F32)n[idx[i + norm_offset] * 3 + 0],
                                    (F32)n[idx[i + norm_offset] * 3 + 1],
                                    (F32)n[idx[i + norm_offset] * 3 + 2]));
        }

        bool found = false;

        LLVolumeFace::VertexMapData::PointMap::iterator point_iter;
        point_iter = point_map.find(LLVector3(cv.getPosition().getF32ptr()));

        if (point_iter != point_map.end())
        {
            for (U32 j = 0; j < point_iter->second.size(); ++j)
            {
                // We have a matching loc
                //
                if ((point_iter->second)[j] == cv)
                {
                    U16 shared_index = (point_iter->second)[j].mIndex;

                    // Don't share verts within the same tri, degenerate
                    //
                    U32 indx_size     = static_cast<U32>(indices.size());
                    U32 verts_new_tri = indx_size % 3;
                    if ((verts_new_tri < 1 || indices[indx_size - 1] != shared_index) &&
                        (verts_new_tri < 2 || indices[indx_size - 2] != shared_index))
                    {
                        found = true;
                        indices.push_back(shared_index);
                    }
                    break;
                }
            }
        }

        if (!found)
        {
            // VFExtents change
            update_min_max(face.mExtents[0], face.mExtents[1], cv.getPosition());
            verts.push_back(cv);
            if (verts.size() >= 65535)
            {
                // llerrs << "Attempted to write model exceeding 16-bit index buffer limitation." << LL_ENDL;
                return LLModel::VERTEX_NUMBER_OVERFLOW;
            }
            U16 index = (U16)(verts.size() - 1);
            indices.push_back(index);

            LLVolumeFace::VertexMapData d;
            d.setPosition(cv.getPosition());
            d.mTexCoord = cv.mTexCoord;
            d.setNormal(cv.getNormal());
            d.mIndex = index;
            if (point_iter != point_map.end())
            {
                point_iter->second.push_back(d);
            }
            else
            {
                point_map[LLVector3(d.getPosition().getF32ptr())].push_back(d);
            }
        }

        if (indices.size() % 3 == 0 && verts.size() >= 65532)
        {
            std::string material;

            if (tri->getMaterial())
            {
                material = std::string(tri->getMaterial());
            }

            materials.push_back(material);
            face_list.push_back(face);
            face_list.rbegin()->fillFromLegacyData(verts, indices);
            LLVolumeFace& new_face = *face_list.rbegin();
            if (!norm_source)
            {
                // ll_aligned_free_16(new_face.mNormals);
                new_face.mNormals = NULL;
            }

            if (!tc_source)
            {
                // ll_aligned_free_16(new_face.mTexCoords);
                new_face.mTexCoords = NULL;
            }

            face = LLVolumeFace();
            // VFExtents change
            face.mExtents[0].set((F32)v[0], (F32)v[1], (F32)v[2]);
            face.mExtents[1].set((F32)v[0], (F32)v[1], (F32)v[2]);

            verts.clear();
            indices.clear();
            point_map.clear();
        }
    }

    if (!verts.empty())
    {
        std::string material;

        if (tri->getMaterial())
        {
            material = std::string(tri->getMaterial());
        }

        materials.push_back(material);
        face_list.push_back(face);

        face_list.rbegin()->fillFromLegacyData(verts, indices);
        LLVolumeFace& new_face = *face_list.rbegin();
        if (!norm_source)
        {
            // ll_aligned_free_16(new_face.mNormals);
            new_face.mNormals = NULL;
        }

        if (!tc_source)
        {
            // ll_aligned_free_16(new_face.mTexCoords);
            new_face.mTexCoords = NULL;
        }
    }

    return LLModel::NO_ERRORS;
}
*/
bool LLGLTFLoader::parseMaterials()
{
    return true;
    /*
    if (!mGltfLoaded) return false;

    // fill local texture data structures
    mSamplers.clear();
    for (auto in_sampler : mGltfModel.samplers)
    {
        gltf_sampler sampler;
        sampler.magFilter = in_sampler.magFilter > 0 ? in_sampler.magFilter : GL_LINEAR;
        sampler.minFilter = in_sampler.minFilter > 0 ? in_sampler.minFilter : GL_LINEAR;;
        sampler.wrapS     = in_sampler.wrapS;
        sampler.wrapT     = in_sampler.wrapT;
        sampler.name      = in_sampler.name; // unused
        mSamplers.push_back(sampler);
    }

    mImages.clear();
    for (auto in_image : mGltfModel.images)
    {
        gltf_image image;
        image.numChannels     = in_image.component;
        image.bytesPerChannel = in_image.bits >> 3;     // Convert bits to bytes
        image.pixelType       = in_image.pixel_type;    // Maps exactly, i.e. TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE == GL_UNSIGNED_BYTE, etc
        image.size            = static_cast<U32>(in_image.image.size());
        image.height          = in_image.height;
        image.width           = in_image.width;
        image.data            = in_image.image.data();

        if (in_image.as_is)
        {
            LL_WARNS("GLTF_IMPORT") << "Unsupported image encoding" << LL_ENDL;
            return false;
        }

        if (image.size != image.height * image.width * image.numChannels * image.bytesPerChannel)
        {
            LL_WARNS("GLTF_IMPORT") << "Image size error" << LL_ENDL;
            return false;
        }

        mImages.push_back(image);
    }

    mTextures.clear();
    for (auto in_tex : mGltfModel.textures)
    {
        gltf_texture tex;
        tex.imageIdx   = in_tex.source;
        tex.samplerIdx = in_tex.sampler;
        tex.imageUuid.setNull();

        if (tex.imageIdx >= mImages.size() || tex.samplerIdx >= mSamplers.size())
        {
            LL_WARNS("GLTF_IMPORT") << "Texture sampler/image index error" << LL_ENDL;
            return false;
        }

        mTextures.push_back(tex);
    }

    // parse each material
    for (tinygltf::Material gltf_material : mGltfModel.materials)
    {
        gltf_render_material mat;
        mat.name = gltf_material.name;

        tinygltf::PbrMetallicRoughness& pbr = gltf_material.pbrMetallicRoughness;
        mat.hasPBR = true;  // Always true, for now

        mat.baseColor.set(pbr.baseColorFactor.data());
        mat.hasBaseTex = pbr.baseColorTexture.index >= 0;
        mat.baseColorTexIdx = pbr.baseColorTexture.index;
        mat.baseColorTexCoords = pbr.baseColorTexture.texCoord;

        mat.metalness = pbr.metallicFactor;
        mat.roughness = pbr.roughnessFactor;
        mat.hasMRTex = pbr.metallicRoughnessTexture.index >= 0;
        mat.metalRoughTexIdx = pbr.metallicRoughnessTexture.index;
        mat.metalRoughTexCoords = pbr.metallicRoughnessTexture.texCoord;

        mat.normalScale = gltf_material.normalTexture.scale;
        mat.hasNormalTex = gltf_material.normalTexture.index >= 0;
        mat.normalTexIdx = gltf_material.normalTexture.index;
        mat.normalTexCoords = gltf_material.normalTexture.texCoord;

        mat.occlusionScale = gltf_material.occlusionTexture.strength;
        mat.hasOcclusionTex = gltf_material.occlusionTexture.index >= 0;
        mat.occlusionTexIdx = gltf_material.occlusionTexture.index;
        mat.occlusionTexCoords = gltf_material.occlusionTexture.texCoord;

        mat.emissiveColor.set(gltf_material.emissiveFactor.data());
        mat.hasEmissiveTex = gltf_material.emissiveTexture.index >= 0;
        mat.emissiveTexIdx = gltf_material.emissiveTexture.index;
        mat.emissiveTexCoords = gltf_material.emissiveTexture.texCoord;

        mat.alphaMode = gltf_material.alphaMode;
        mat.alphaMask = gltf_material.alphaCutoff;

        if ((mat.hasNormalTex    && (mat.normalTexIdx     >= mTextures.size())) ||
            (mat.hasOcclusionTex && (mat.occlusionTexIdx  >= mTextures.size())) ||
            (mat.hasEmissiveTex  && (mat.emissiveTexIdx   >= mTextures.size())) ||
            (mat.hasBaseTex      && (mat.baseColorTexIdx  >= mTextures.size())) ||
            (mat.hasMRTex        && (mat.metalRoughTexIdx >= mTextures.size())))
        {
            LL_WARNS("GLTF_IMPORT") << "Texture resource index error" << LL_ENDL;
            return false;
        }

        if ((mat.hasNormalTex    && (mat.normalTexCoords      > 2)) ||    // mesh can have up to 3 sets of UV
            (mat.hasOcclusionTex && (mat.occlusionTexCoords   > 2)) ||
            (mat.hasEmissiveTex  && (mat.emissiveTexCoords    > 2)) ||
            (mat.hasBaseTex      && (mat.baseColorTexCoords   > 2)) ||
            (mat.hasMRTex        && (mat.metalRoughTexCoords  > 2)))
        {
            LL_WARNS("GLTF_IMPORT") << "Image texcoord index error" << LL_ENDL;
            return false;
        }

        mMaterials.push_back(mat);
    }

    return true;
    */
}

// TODO: convert raw vertex buffers to UUIDs
void LLGLTFLoader::uploadMeshes()
{
    //llassert(0);
}

// convert raw image buffers to texture UUIDs & assemble into a render material
void LLGLTFLoader::uploadMaterials()
{
    for (gltf_render_material mat : mMaterials) // Initially 1 material per gltf file, but design for multiple
    {
        if (mat.hasBaseTex)
        {
            gltf_texture& gtex = mTextures[mat.baseColorTexIdx];
            if (gtex.imageUuid.isNull())
            {
                gtex.imageUuid = imageBufferToTextureUUID(gtex);
            }
        }

        if (mat.hasMRTex)
        {
            gltf_texture& gtex = mTextures[mat.metalRoughTexIdx];
            if (gtex.imageUuid.isNull())
            {
                gtex.imageUuid = imageBufferToTextureUUID(gtex);
            }
        }

        if (mat.hasNormalTex)
        {
            gltf_texture& gtex = mTextures[mat.normalTexIdx];
            if (gtex.imageUuid.isNull())
            {
                gtex.imageUuid = imageBufferToTextureUUID(gtex);
            }
        }

        if (mat.hasOcclusionTex)
        {
            gltf_texture& gtex = mTextures[mat.occlusionTexIdx];
            if (gtex.imageUuid.isNull())
            {
                gtex.imageUuid = imageBufferToTextureUUID(gtex);
            }
        }

        if (mat.hasEmissiveTex)
        {
            gltf_texture& gtex = mTextures[mat.emissiveTexIdx];
            if (gtex.imageUuid.isNull())
            {
                gtex.imageUuid = imageBufferToTextureUUID(gtex);
            }
        }
    }
}

LLUUID LLGLTFLoader::imageBufferToTextureUUID(const gltf_texture& tex)
{
    //gltf_image& image = mImages[tex.imageIdx];
    //gltf_sampler& sampler = mSamplers[tex.samplerIdx];

    // fill an LLSD container with image+sampler data

    // upload texture

    // retrieve UUID

    return LLUUID::null;
}
