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
#include <glm/gtc/packing.hpp>

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

    mGltfLoaded = mGLTFAsset.load(filename, false);

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

    mMaterialsLoaded = parseMaterials();
    if (mMaterialsLoaded) uploadMaterials();

    setLoadState(DONE);

    return (mMeshesLoaded);
}

bool LLGLTFLoader::parseMeshes()
{
    if (!mGltfLoaded) return false;

    // 2022-04 DJH Volume params from dae example. TODO understand PCODE
    LLVolumeParams volume_params;
    volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);

    mTransform.setIdentity();

    // Populate the joints from skins first.
    // There's not many skins - and you can pretty easily iterate through the nodes from that.
    for (auto& skin : mGLTFAsset.mSkins)
    {
        populateJointFromSkin(skin);
    }

    for (auto& node : mGLTFAsset.mNodes)
    {
        // Make node matrix valid for correct transformation
        node.makeMatrixValid();
    }

    // Process each node
    for (auto& node : mGLTFAsset.mNodes)
    {
        LLMatrix4    transformation;
        material_map mats;
        auto meshidx = node.mMesh;

        if (meshidx >= 0)
        {
            if (mGLTFAsset.mMeshes.size() > meshidx)
            {
                LLModel* pModel = new LLModel(volume_params, 0.f);
                auto mesh = mGLTFAsset.mMeshes[meshidx];
                if (populateModelFromMesh(pModel, mesh, node, mats) &&
                    (LLModel::NO_ERRORS == pModel->getStatus()) &&
                    validate_model(pModel))
                {
                    mModelList.push_back(pModel);

                    mTransform.setIdentity();
                    transformation = mTransform;

                    // adjust the transformation to compensate for mesh normalization
                    LLVector3 mesh_scale_vector;
                    LLVector3 mesh_translation_vector;
                    pModel->getNormalizedScaleTranslation(mesh_scale_vector, mesh_translation_vector);

                    LLMatrix4 mesh_translation;
                    mesh_translation.setTranslation(mesh_translation_vector);
                    mesh_translation *= transformation;
                    transformation = mesh_translation;

                    LLMatrix4 mesh_scale;
                    mesh_scale.initScale(mesh_scale_vector);
                    mesh_scale *= transformation;
                    transformation = mesh_scale;

                    if (transformation.determinant() < 0)
                    { // negative scales are not supported
                        LL_INFOS("GLTF") << "Negative scale detected, unsupported post-normalization transform.  domInstance_geometry: "
                                   << pModel->mLabel << LL_ENDL;
                        LLSD args;
                        args["Message"] = "NegativeScaleNormTrans";
                        args["LABEL"]   = pModel->mLabel;
                        mWarningsArray.append(args);
                    }

                    mScene[transformation].push_back(LLModelInstance(pModel, pModel->mLabel, transformation, mats));
                    stretch_extents(pModel, transformation);
                }
                else
                {
                    setLoadState(ERROR_MODEL + pModel->getStatus());
                    delete pModel;
                    return false;
                }
            }
        }
    }

    return true;
}

void LLGLTFLoader::computeCombinedNodeTransform(const LL::GLTF::Asset& asset, S32 node_index, glm::mat4& combined_transform)
{
    auto& node = asset.mNodes[node_index];

    // Start with this node's transform
    glm::mat4 node_transform = node.mMatrix;

    // Find parent node and apply its transform if it exists
    for (auto& other_node : asset.mNodes)
    {
        for (auto& child_index : other_node.mChildren)
        {
            if (child_index == node_index)
            {
                // Found a parent, recursively get its combined transform
                glm::mat4 parent_transform;
                computeCombinedNodeTransform(asset, static_cast<S32>(&other_node - &asset.mNodes[0]), parent_transform);

                // Apply parent transform to current node transform
                node_transform = parent_transform * node_transform;
                break;
            }
        }
    }

    combined_transform = node_transform;
}

bool LLGLTFLoader::populateModelFromMesh(LLModel* pModel, const LL::GLTF::Mesh& mesh, const LL::GLTF::Node& nodeno, material_map& mats)
{
    pModel->mLabel = mesh.mName;
    pModel->ClearFacesAndMaterials();

    S32 skinIdx = nodeno.mSkin;

    // Pre-compute coordinate system rotation matrix (GLTF Y-up to SL Z-up)
    static const glm::mat4 coord_system_rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Compute final combined transform matrix (hierarchy + coordinate rotation)
    S32 node_index = static_cast<S32>(&nodeno - &mGLTFAsset.mNodes[0]);
    glm::mat4 hierarchy_transform;
    computeCombinedNodeTransform(mGLTFAsset, node_index, hierarchy_transform);

    // Combine transforms: coordinate rotation applied to hierarchy transform
    const glm::mat4 final_transform = coord_system_rotation * hierarchy_transform;

    // Pre-compute normal transform matrix (transpose of inverse of upper-left 3x3)
    const glm::mat3 normal_transform = glm::transpose(glm::inverse(glm::mat3(final_transform)));

    // Mark unsuported joints with '-1' so that they won't get added into weights
    // GLTF maps all joints onto all meshes. Gather use count per mesh to cut unused ones.
    std::vector<S32> gltf_joint_index_use_count;
    if (skinIdx >= 0 && mGLTFAsset.mSkins.size() > skinIdx)
    {
        LL::GLTF::Skin& gltf_skin = mGLTFAsset.mSkins[skinIdx];

        size_t jointCnt = gltf_skin.mJoints.size();
        gltf_joint_index_use_count.resize(jointCnt);

        S32 replacement_index = 0;
        for (size_t i = 0; i < jointCnt; ++i)
        {
            // Process joint name and idnex
            S32 joint = gltf_skin.mJoints[i];
            LL::GLTF::Node& jointNode = mGLTFAsset.mNodes[joint];
            jointNode.makeMatrixValid();

            std::string legal_name(jointNode.mName);
            if (mJointMap.find(legal_name) == mJointMap.end())
            {
                gltf_joint_index_use_count[i] = -1; // mark as unsupported
            }
        }
    }

    auto prims = mesh.mPrimitives;
    for (auto prim : prims)
    {
        // Unfortunately, SLM does not support 32 bit indices.  Filter out anything that goes beyond 16 bit.
        if (prim.getVertexCount() < USHRT_MAX)
        {
            // So primitives already have all of the data we need for a given face in SL land.
            // Primitives may only ever have a single material assigned to them - as the relation is 1:1 in terms of intended draw call
            // count. Just go ahead and populate faces direct from the GLTF primitives here. -Geenz 2025-04-07
            LLVolumeFace face;
            std::vector<GLTFVertex> vertices;
            std::vector<U16> indices;

            LLImportMaterial impMat;
            impMat.mDiffuseColor = LLColor4::white; // Default color

            // Process material if available
            if (prim.mMaterial >= 0 && prim.mMaterial < mGLTFAsset.mMaterials.size())
            {
                LL::GLTF::Material* material = &mGLTFAsset.mMaterials[prim.mMaterial];

                // Set diffuse color from base color factor
                impMat.mDiffuseColor = LLColor4(
                    material->mPbrMetallicRoughness.mBaseColorFactor[0],
                    material->mPbrMetallicRoughness.mBaseColorFactor[1],
                    material->mPbrMetallicRoughness.mBaseColorFactor[2],
                    material->mPbrMetallicRoughness.mBaseColorFactor[3]
                );

                // Process base color texture if it exists
                if (material->mPbrMetallicRoughness.mBaseColorTexture.mIndex >= 0)
                {
                    S32 texIndex = material->mPbrMetallicRoughness.mBaseColorTexture.mIndex;
                    if (texIndex < mGLTFAsset.mTextures.size())
                    {
                        S32 sourceIndex = mGLTFAsset.mTextures[texIndex].mSource;
                        if (sourceIndex >= 0 && sourceIndex < mGLTFAsset.mImages.size())
                        {
                            LL::GLTF::Image& image = mGLTFAsset.mImages[sourceIndex];

                            // Use URI as texture file name
                            if (!image.mUri.empty())
                            {
                                // URI might be a remote URL or a local path
                                std::string filename = image.mUri;

                                // Extract just the filename from the URI
                                size_t pos = filename.find_last_of("/\\");
                                if (pos != std::string::npos)
                                {
                                    filename = filename.substr(pos + 1);
                                }

                                // Store the texture filename
                                impMat.mDiffuseMapFilename = filename;
                                impMat.mDiffuseMapLabel = material->mName.empty() ? filename : material->mName;

                                LL_INFOS("GLTF_IMPORT") << "Found texture: " << impMat.mDiffuseMapFilename << LL_ENDL;

                                // If the image has a texture loaded already, use it
                                if (image.mTexture.notNull())
                                {
                                    impMat.setDiffuseMap(image.mTexture->getID());
                                    LL_INFOS("GLTF_IMPORT") << "Using existing texture ID: " << image.mTexture->getID().asString() << LL_ENDL;
                                }
                                else
                                {
                                    // Let the model preview know we need to load this texture
                                    mNumOfFetchingTextures++;
                                    LL_INFOS("GLTF_IMPORT") << "Adding texture to load queue: " << impMat.mDiffuseMapFilename << LL_ENDL;
                                }
                            }
                            else if (image.mTexture.notNull())
                            {
                                // No URI but we have a texture, use it directly
                                impMat.setDiffuseMap(image.mTexture->getID());
                                LL_INFOS("GLTF_IMPORT") << "Using existing texture ID without URI: " << image.mTexture->getID().asString() << LL_ENDL;
                            }
                            else if (image.mBufferView >= 0)
                            {
                                // For embedded textures (no URI but has buffer data)
                                // Create a pseudo filename for the embedded texture
                                std::string pseudo_filename = "gltf_embedded_texture_" + std::to_string(sourceIndex) + ".png";
                                impMat.mDiffuseMapFilename = pseudo_filename;
                                impMat.mDiffuseMapLabel = material->mName.empty() ? pseudo_filename : material->mName;

                                // Mark for loading
                                mNumOfFetchingTextures++;
                                LL_INFOS("GLTF_IMPORT") << "Adding embedded texture to load queue: " << pseudo_filename << LL_ENDL;
                            }
                        }
                    }
                }
            }

            // Apply the global scale and center offset to all vertices
            for (U32 i = 0; i < prim.getVertexCount(); i++)
            {
                // Use pre-computed final_transform
                glm::vec4 pos(prim.mPositions[i][0], prim.mPositions[i][1], prim.mPositions[i][2], 1.0f);
                glm::vec4 transformed_pos = final_transform * pos;

                GLTFVertex vert;
                vert.position = glm::vec3(transformed_pos);

                // Use pre-computed normal_transform
                glm::vec3 normal_vec(prim.mNormals[i][0], prim.mNormals[i][1], prim.mNormals[i][2]);
                vert.normal = glm::normalize(normal_transform * normal_vec);

                vert.uv0 = glm::vec2(prim.mTexCoords0[i][0], -prim.mTexCoords0[i][1]);

                if (skinIdx >= 0)
                {
                    auto accessorIdx = prim.mAttributes["JOINTS_0"];
                    LL::GLTF::Accessor::ComponentType componentType = LL::GLTF::Accessor::ComponentType::UNSIGNED_BYTE;
                    if (accessorIdx >= 0)
                    {
                        auto accessor = mGLTFAsset.mAccessors[accessorIdx];
                        componentType = accessor.mComponentType;
                    }

                    // The GLTF spec allows for either an unsigned byte for joint indices, or an unsigned short.
                    // Detect and unpack accordingly.
                    if (componentType == LL::GLTF::Accessor::ComponentType::UNSIGNED_BYTE)
                    {
                        auto ujoint = glm::unpackUint4x8((U32)(prim.mJoints[i] & 0xFFFFFFFF));
                        vert.joints = glm::u16vec4(ujoint.x, ujoint.y, ujoint.z, ujoint.w);
                    }
                    else if (componentType == LL::GLTF::Accessor::ComponentType::UNSIGNED_SHORT)
                    {
                        vert.joints = glm::unpackUint4x16(prim.mJoints[i]);
                    }

                    vert.weights = glm::vec4(prim.mWeights[i]);
                }
                vertices.push_back(vert);
            }

            for (U32 i = 0; i < prim.getIndexCount(); i++)
            {
                indices.push_back(prim.mIndexArray[i]);
            }

            // Check for empty vertex array before processing
            if (vertices.empty())
            {
                LL_WARNS("GLTF_IMPORT") << "Empty vertex array for primitive" << LL_ENDL;
                continue; // Skip this primitive
            }

            std::vector<LLVolumeFace::VertexData> faceVertices;
            glm::vec3 min = glm::vec3(FLT_MAX);
            glm::vec3 max = glm::vec3(-FLT_MAX);

            for (U32 i = 0; i < vertices.size(); i++)
            {
                LLVolumeFace::VertexData vert;

                // Update min/max bounds
                if (i == 0)
                {
                    min = max = vertices[i].position;
                }
                else
                {
                    min.x = std::min(min.x, vertices[i].position.x);
                    min.y = std::min(min.y, vertices[i].position.y);
                    min.z = std::min(min.z, vertices[i].position.z);
                    max.x = std::max(max.x, vertices[i].position.x);
                    max.y = std::max(max.y, vertices[i].position.y);
                    max.z = std::max(max.z, vertices[i].position.z);
                }

                LLVector4a position = LLVector4a(vertices[i].position.x, vertices[i].position.y, vertices[i].position.z);
                LLVector4a normal = LLVector4a(vertices[i].normal.x, vertices[i].normal.y, vertices[i].normal.z);
                vert.setPosition(position);
                vert.setNormal(normal);
                vert.mTexCoord = LLVector2(vertices[i].uv0.x, vertices[i].uv0.y);
                faceVertices.push_back(vert);

                if (skinIdx >= 0)
                {
                    // create list of weights that influence this vertex
                    LLModel::weight_list weight_list;

                    // Drop joints that viewer doesn't support (negative in gltf_joint_index_use_count)
                    // don't reindex them yet, more indexes will be removed
                    // Also drop joints that have no weight. GLTF stores 4 per vertex, so there might be
                    // 'empty' ones
                    if (gltf_joint_index_use_count[vertices[i].joints.x] >= 0
                        && vertices[i].weights.x > 0.f)
                    {
                        weight_list.push_back(LLModel::JointWeight(vertices[i].joints.x, vertices[i].weights.x));
                        gltf_joint_index_use_count[vertices[i].joints.x]++;
                    }
                    if (gltf_joint_index_use_count[vertices[i].joints.y] >= 0
                        && vertices[i].weights.y > 0.f)
                    {
                        weight_list.push_back(LLModel::JointWeight(vertices[i].joints.y, vertices[i].weights.y));
                        gltf_joint_index_use_count[vertices[i].joints.y]++;
                    }
                    if (gltf_joint_index_use_count[vertices[i].joints.z] >= 0
                        && vertices[i].weights.z > 0.f)
                    {
                        weight_list.push_back(LLModel::JointWeight(vertices[i].joints.z, vertices[i].weights.z));
                        gltf_joint_index_use_count[vertices[i].joints.z]++;
                    }
                    if (gltf_joint_index_use_count[vertices[i].joints.w] >= 0
                        && vertices[i].weights.w > 0.f)
                    {
                        weight_list.push_back(LLModel::JointWeight(vertices[i].joints.w, vertices[i].weights.w));
                        gltf_joint_index_use_count[vertices[i].joints.w]++;
                    }

                    std::sort(weight_list.begin(), weight_list.end(), LLModel::CompareWeightGreater());

                    std::vector<LLModel::JointWeight> wght;
                    F32                               total = 0.f;

                    for (U32 j = 0; j < llmin((U32)4, (U32)weight_list.size()); ++j)
                    {
                        // take up to 4 most significant weights
                        // Ported from the DAE loader - however, GLTF right now only supports up to four weights per vertex.
                        wght.push_back(weight_list[j]);
                        total += weight_list[j].mWeight;
                    }

                    if (total != 0.f)
                    {
                        F32 scale = 1.f / total;
                        if (scale != 1.f)
                        { // normalize weights
                            for (U32 j = 0; j < wght.size(); ++j)
                            {
                                wght[j].mWeight *= scale;
                            }
                        }
                    }

                    if (wght.size() > 0)
                    {
                        pModel->mSkinWeights[LLVector3(vertices[i].position)] = wght;
                    }
                }
            }

            face.fillFromLegacyData(faceVertices, indices);
            face.mExtents[0] = LLVector4a(min.x, min.y, min.z, 0);
            face.mExtents[1] = LLVector4a(max.x, max.y, max.z, 0);

            pModel->getVolumeFaces().push_back(face);

            // Create a unique material name for this primitive
            std::string materialName;
            if (prim.mMaterial >= 0 && prim.mMaterial < mGLTFAsset.mMaterials.size())
            {
                LL::GLTF::Material* material = &mGLTFAsset.mMaterials[prim.mMaterial];
                materialName = material->mName;

                if (materialName.empty())
                {
                    materialName = "mat" + std::to_string(prim.mMaterial);
                }
            }
            else
            {
                materialName = "mat_default" + std::to_string(pModel->getNumVolumeFaces() - 1);
            }

            pModel->getMaterialList().push_back(materialName);
            mats[materialName] = impMat;
        }
        else {
            LL_INFOS() << "Unable to process mesh due to 16-bit index limits" << LL_ENDL;
            LLSD args;
            args["Message"] = "ParsingErrorBadElement";
            mWarningsArray.append(args);
            return false;
        }
    }

    // Fill joint names, bind matrices and prepare to remap weight indices
    if (skinIdx >= 0)
    {
        LL::GLTF::Skin& gltf_skin = mGLTFAsset.mSkins[skinIdx];
        LLMeshSkinInfo& skin_info = pModel->mSkinInfo;

        size_t jointCnt = gltf_skin.mJoints.size();
        if (gltf_skin.mInverseBindMatrices >= 0 && jointCnt != gltf_skin.mInverseBindMatricesData.size())
        {
            LL_INFOS("GLTF") << "Bind matrices count mismatch joints count" << LL_ENDL;
            LLSD args;
            args["Message"] = "InvBindCountMismatch";
            mWarningsArray.append(args);
        }

        std::vector<S32> gltfindex_to_joitindex_map;
        gltfindex_to_joitindex_map.resize(jointCnt);

        S32 replacement_index = 0;
        for (size_t i = 0; i < jointCnt; ++i)
        {
            // Process joint name and idnex
            S32 joint = gltf_skin.mJoints[i];
            if (gltf_joint_index_use_count[i] <= 0)
            {
                // Unused (0) or unsupported (-1) joint, drop it
                continue;
            }
            LL::GLTF::Node& jointNode = mGLTFAsset.mNodes[joint];
            jointNode.makeMatrixValid();

            std::string legal_name(jointNode.mName);
            if (mJointMap.find(legal_name) != mJointMap.end())
            {
                legal_name = mJointMap[legal_name];
            }
            else
            {
                llassert(false); // should have been stopped by gltf_joint_index_use_count[i] == -1
                continue;
            }
            gltfindex_to_joitindex_map[i] = replacement_index++;

            skin_info.mJointNames.push_back(legal_name);
            skin_info.mJointNums.push_back(-1);

            if (i < gltf_skin.mInverseBindMatricesData.size())
            {
                // Use pre-computed coord_system_rotation instead of recreating it
                LL::GLTF::mat4 gltf_mat = gltf_skin.mInverseBindMatricesData[i];

                glm::mat4 original_bind_matrix = glm::inverse(gltf_mat);
                glm::mat4 rotated_original = coord_system_rotation * original_bind_matrix;
                glm::mat4 rotated_inverse_bind_matrix = glm::inverse(rotated_original);

                LLMatrix4 gltf_transform = LLMatrix4(glm::value_ptr(rotated_inverse_bind_matrix));
                skin_info.mInvBindMatrix.push_back(LLMatrix4a(gltf_transform));

                LL_INFOS("GLTF_DEBUG") << "mInvBindMatrix name: " << legal_name << " val: " << gltf_transform << LL_ENDL;

                // For alternate bind matrix, use the ORIGINAL joint transform (before rotation)
                // Get the original joint node and use its matrix directly
                S32 joint = gltf_skin.mJoints[i];
                LL::GLTF::Node& jointNode = mGLTFAsset.mNodes[joint];
                jointNode.makeMatrixValid();
                LLMatrix4 original_joint_transform(glm::value_ptr(jointNode.mMatrix));

                LL_INFOS("GLTF_DEBUG") << "mAlternateBindMatrix name: " << legal_name << " val: " << original_joint_transform << LL_ENDL;
                skin_info.mAlternateBindMatrix.push_back(LLMatrix4a(original_joint_transform));
            }
        }

        // "Bind Shape Matrix" is supposed to transform the geometry of the skinned mesh
        // into the coordinate space of the joints.
        // In GLTF, this matrix is omitted, and it is assumed that this transform is either
        // premultiplied with the mesh data, or postmultiplied to the inverse bind matrices.
        LLMatrix4 bind_shape;
        bind_shape.setIdentity();
        skin_info.mBindShapeMatrix.loadu(bind_shape);

        // Remap indices for pModel->mSkinWeights
        for (auto& weights : pModel->mSkinWeights)
        {
            for (auto& weight : weights.second)
            {
                weight.mJointIdx = gltfindex_to_joitindex_map[weight.mJointIdx];
            }
        }
    }

    return true;
}

void LLGLTFLoader::populateJointFromSkin(const LL::GLTF::Skin& skin)
{
    // Create coordinate system rotation matrix - GLTF is Y-up, SL is Z-up
    glm::mat4 coord_system_rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    LL_INFOS("GLTF_DEBUG") << "populateJointFromSkin: Processing " << skin.mJoints.size() << " joints" << LL_ENDL;

    for (auto joint : skin.mJoints)
    {
        auto jointNode = mGLTFAsset.mNodes[joint];

        std::string legal_name(jointNode.mName);
        if (mJointMap.find(legal_name) != mJointMap.end())
        {
            legal_name = mJointMap[legal_name];
        }
        else
        {
            // ignore unrecognized joint
            LL_DEBUGS("GLTF") << "Ignoring joint: " << legal_name << LL_ENDL;
            continue;
        }

        jointNode.makeMatrixValid();

        // Debug: Log original joint matrix
        glm::mat4 gltf_joint_matrix = jointNode.mMatrix;
        LL_INFOS("GLTF_DEBUG") << "Joint '" << legal_name << "' original matrix:" << LL_ENDL;
        for(int i = 0; i < 4; i++)
        {
            LL_INFOS("GLTF_DEBUG") << "  [" << gltf_joint_matrix[i][0] << ", " << gltf_joint_matrix[i][1]
                                   << ", " << gltf_joint_matrix[i][2] << ", " << gltf_joint_matrix[i][3] << "]" << LL_ENDL;
        }

        // Apply coordinate system rotation to joint transform
        glm::mat4 rotated_joint_matrix = coord_system_rotation * gltf_joint_matrix;

        // Debug: Log rotated joint matrix
        LL_INFOS("GLTF_DEBUG") << "Joint '" << legal_name << "' rotated matrix:" << LL_ENDL;
        for(int i = 0; i < 4; i++)
        {
            LL_INFOS("GLTF_DEBUG") << "  [" << rotated_joint_matrix[i][0] << ", " << rotated_joint_matrix[i][1]
                                   << ", " << rotated_joint_matrix[i][2] << ", " << rotated_joint_matrix[i][3] << "]" << LL_ENDL;
        }

        LLMatrix4 gltf_transform = LLMatrix4(glm::value_ptr(rotated_joint_matrix));
        mJointList[legal_name] = gltf_transform;
        mJointsFromNode.push_front(legal_name);

        LL_INFOS("GLTF_DEBUG") << "mJointList name: " << legal_name << " val: " << gltf_transform << LL_ENDL;
    }
}

bool LLGLTFLoader::parseMaterials()
{
    if (!mGltfLoaded) return false;

    // fill local texture data structures
    mSamplers.clear();
    for (auto& in_sampler : mGLTFAsset.mSamplers)
    {
        gltf_sampler sampler;
        sampler.magFilter = in_sampler.mMagFilter > 0 ? in_sampler.mMagFilter : GL_LINEAR;
        sampler.minFilter = in_sampler.mMinFilter > 0 ? in_sampler.mMinFilter : GL_LINEAR;
        sampler.wrapS = in_sampler.mWrapS;
        sampler.wrapT = in_sampler.mWrapT;
        sampler.name = in_sampler.mName;
        mSamplers.push_back(sampler);
    }

    mImages.clear();
    for (auto& in_image : mGLTFAsset.mImages)
    {
        gltf_image image;
        image.numChannels = in_image.mComponent;
        image.bytesPerChannel = in_image.mBits >> 3;     // Convert bits to bytes
        image.pixelType = in_image.mPixelType;
        image.size = 0; // We'll calculate this below if we have valid dimensions

        // Get dimensions from the texture if available
        if (in_image.mTexture && in_image.mTexture->getDiscardLevel() >= 0)
        {
            image.height = in_image.mTexture->getHeight();
            image.width = in_image.mTexture->getWidth();
            // Since we don't have direct access to the raw data, we'll use the dimensions to calculate size
            if (image.height > 0 && image.width > 0 && image.numChannels > 0 && image.bytesPerChannel > 0)
            {
                image.size = static_cast<U32>(image.height * image.width * image.numChannels * image.bytesPerChannel);
            }
        }
        else
        {
            // Fallback to provided dimensions
            image.height = in_image.mHeight;
            image.width = in_image.mWidth;
            if (image.height > 0 && image.width > 0 && image.numChannels > 0 && image.bytesPerChannel > 0)
            {
                image.size = static_cast<U32>(image.height * image.width * image.numChannels * image.bytesPerChannel);
            }
        }

        // If we couldn't determine the size, skip this image
        if (image.size == 0)
        {
            LL_WARNS("GLTF_IMPORT") << "Image size could not be determined" << LL_ENDL;
            continue;
        }

        // We don't have direct access to the image data, so data pointer remains nullptr
        image.data = nullptr;
        mImages.push_back(image);
    }

    mTextures.clear();
    for (auto& in_tex : mGLTFAsset.mTextures)
    {
        gltf_texture tex;
        tex.imageIdx = in_tex.mSource;
        tex.samplerIdx = in_tex.mSampler;
        tex.imageUuid.setNull();

        if (tex.imageIdx >= mImages.size() || tex.samplerIdx >= mSamplers.size())
        {
            LL_WARNS("GLTF_IMPORT") << "Texture sampler/image index error" << LL_ENDL;
            return false;
        }

        mTextures.push_back(tex);
    }

    // parse each material
    mMaterials.clear();
    for (const auto& gltf_material : mGLTFAsset.mMaterials)
    {
        gltf_render_material mat;
        mat.name = gltf_material.mName;

        // PBR Metallic Roughness properties
        mat.hasPBR = true;

        // Base color factor
        mat.baseColor = LLColor4(
            gltf_material.mPbrMetallicRoughness.mBaseColorFactor[0],
            gltf_material.mPbrMetallicRoughness.mBaseColorFactor[1],
            gltf_material.mPbrMetallicRoughness.mBaseColorFactor[2],
            gltf_material.mPbrMetallicRoughness.mBaseColorFactor[3]
        );

        // Base color texture
        mat.hasBaseTex = gltf_material.mPbrMetallicRoughness.mBaseColorTexture.mIndex >= 0;
        mat.baseColorTexIdx = gltf_material.mPbrMetallicRoughness.mBaseColorTexture.mIndex;
        mat.baseColorTexCoords = gltf_material.mPbrMetallicRoughness.mBaseColorTexture.mTexCoord;

        // Metalness and roughness
        mat.metalness = gltf_material.mPbrMetallicRoughness.mMetallicFactor;
        mat.roughness = gltf_material.mPbrMetallicRoughness.mRoughnessFactor;

        // Metallic-roughness texture
        mat.hasMRTex = gltf_material.mPbrMetallicRoughness.mMetallicRoughnessTexture.mIndex >= 0;
        mat.metalRoughTexIdx = gltf_material.mPbrMetallicRoughness.mMetallicRoughnessTexture.mIndex;
        mat.metalRoughTexCoords = gltf_material.mPbrMetallicRoughness.mMetallicRoughnessTexture.mTexCoord;

        // Normal texture
        mat.normalScale = gltf_material.mNormalTexture.mScale;
        mat.hasNormalTex = gltf_material.mNormalTexture.mIndex >= 0;
        mat.normalTexIdx = gltf_material.mNormalTexture.mIndex;
        mat.normalTexCoords = gltf_material.mNormalTexture.mTexCoord;

        // Occlusion texture
        mat.occlusionScale = gltf_material.mOcclusionTexture.mStrength;
        mat.hasOcclusionTex = gltf_material.mOcclusionTexture.mIndex >= 0;
        mat.occlusionTexIdx = gltf_material.mOcclusionTexture.mIndex;
        mat.occlusionTexCoords = gltf_material.mOcclusionTexture.mTexCoord;

        // Emissive texture and color
        mat.emissiveColor = LLColor4(
            gltf_material.mEmissiveFactor[0],
            gltf_material.mEmissiveFactor[1],
            gltf_material.mEmissiveFactor[2],
            1.0f
        );
        mat.hasEmissiveTex = gltf_material.mEmissiveTexture.mIndex >= 0;
        mat.emissiveTexIdx = gltf_material.mEmissiveTexture.mIndex;
        mat.emissiveTexCoords = gltf_material.mEmissiveTexture.mTexCoord;

        // Convert AlphaMode enum to string
        switch (gltf_material.mAlphaMode)
        {
        case LL::GLTF::Material::AlphaMode::OPAQUE:
            mat.alphaMode = "OPAQUE";
            break;
        case LL::GLTF::Material::AlphaMode::MASK:
            mat.alphaMode = "MASK";
            break;
        case LL::GLTF::Material::AlphaMode::BLEND:
            mat.alphaMode = "BLEND";
            break;
        default:
            mat.alphaMode = "OPAQUE";
            break;
        }

        mat.alphaMask = gltf_material.mAlphaCutoff;

        // Verify that all referenced textures are valid
        if ((mat.hasNormalTex && (mat.normalTexIdx >= mTextures.size())) ||
            (mat.hasOcclusionTex && (mat.occlusionTexIdx >= mTextures.size())) ||
            (mat.hasEmissiveTex && (mat.emissiveTexIdx >= mTextures.size())) ||
            (mat.hasBaseTex && (mat.baseColorTexIdx >= mTextures.size())) ||
            (mat.hasMRTex && (mat.metalRoughTexIdx >= mTextures.size())))
        {
            LL_WARNS("GLTF_IMPORT") << "Texture resource index error" << LL_ENDL;
            return false;
        }

        // Verify texture coordinate sets are valid (mesh can have up to 3 sets of UV)
        if ((mat.hasNormalTex && (mat.normalTexCoords > 2)) ||
            (mat.hasOcclusionTex && (mat.occlusionTexCoords > 2)) ||
            (mat.hasEmissiveTex && (mat.emissiveTexCoords > 2)) ||
            (mat.hasBaseTex && (mat.baseColorTexCoords > 2)) ||
            (mat.hasMRTex && (mat.metalRoughTexCoords > 2)))
        {
            LL_WARNS("GLTF_IMPORT") << "Image texcoord index error" << LL_ENDL;
            return false;
        }

        mMaterials.push_back(mat);
    }

    return true;
}

// TODO: convert raw vertex buffers to UUIDs
void LLGLTFLoader::uploadMeshes()
{
    //llassert(0);
}

// convert raw image buffers to texture UUIDs & assemble into a render material
void LLGLTFLoader::uploadMaterials()
{
    LL_INFOS("GLTF_IMPORT") << "Uploading materials, count: " << mMaterials.size() << LL_ENDL;

    for (gltf_render_material& mat : mMaterials)
    {
        LL_INFOS("GLTF_IMPORT") << "Processing material: " << mat.name << LL_ENDL;

        // Process base color texture
        if (mat.hasBaseTex && mat.baseColorTexIdx < mTextures.size())
        {
            gltf_texture& gtex = mTextures[mat.baseColorTexIdx];
            if (gtex.imageUuid.isNull())
            {
                LL_INFOS("GLTF_IMPORT") << "Loading base color texture for material " << mat.name << LL_ENDL;
                gtex.imageUuid = imageBufferToTextureUUID(gtex);

                if (gtex.imageUuid.notNull())
                {
                    LL_INFOS("GLTF_IMPORT") << "Base color texture loaded, ID: " << gtex.imageUuid.asString() << LL_ENDL;
                }
                else
                {
                    LL_WARNS("GLTF_IMPORT") << "Failed to load base color texture for material " << mat.name << LL_ENDL;
                }
            }
        }

        // Process other textures similarly
        if (mat.hasMRTex && mat.metalRoughTexIdx < mTextures.size())
        {
            gltf_texture& gtex = mTextures[mat.metalRoughTexIdx];
            if (gtex.imageUuid.isNull())
            {
                gtex.imageUuid = imageBufferToTextureUUID(gtex);
            }
        }

        if (mat.hasNormalTex && mat.normalTexIdx < mTextures.size())
        {
            gltf_texture& gtex = mTextures[mat.normalTexIdx];
            if (gtex.imageUuid.isNull())
            {
                gtex.imageUuid = imageBufferToTextureUUID(gtex);
            }
        }

        if (mat.hasOcclusionTex && mat.occlusionTexIdx < mTextures.size())
        {
            gltf_texture& gtex = mTextures[mat.occlusionTexIdx];
            if (gtex.imageUuid.isNull())
            {
                gtex.imageUuid = imageBufferToTextureUUID(gtex);
            }
        }

        if (mat.hasEmissiveTex && mat.emissiveTexIdx < mTextures.size())
        {
            gltf_texture& gtex = mTextures[mat.emissiveTexIdx];
            if (gtex.imageUuid.isNull())
            {
                gtex.imageUuid = imageBufferToTextureUUID(gtex);
            }
        }
    }

    // Update material map for all model instances to ensure textures are properly associated
    // mScene is a std::map<LLMatrix4, model_instance_list>, not an array, so we need to iterate through it correctly
    for (auto& scene_entry : mScene)
    {
        for (LLModelInstance& instance : scene_entry.second)
        {
            LLModel* model = instance.mModel;

            if (model)
            {
                for (size_t i = 0; i < model->getMaterialList().size(); ++i)
                {
                    const std::string& matName = model->getMaterialList()[i];
                    if (!matName.empty())
                    {
                        // Ensure this material exists in the instance's material map
                        if (instance.mMaterial.find(matName) == instance.mMaterial.end())
                        {
                            // Find material in our render materials
                            for (const auto& renderMat : mMaterials)
                            {
                                if (renderMat.name == matName)
                                {
                                    // Create an import material from the render material
                                    LLImportMaterial impMat;
                                    impMat.mDiffuseColor = renderMat.baseColor;

                                    // Set diffuse texture if available
                                    if (renderMat.hasBaseTex && renderMat.baseColorTexIdx < mTextures.size())
                                    {
                                        const gltf_texture& gtex = mTextures[renderMat.baseColorTexIdx];
                                        if (!gtex.imageUuid.isNull())
                                        {
                                            impMat.setDiffuseMap(gtex.imageUuid);
                                            LL_INFOS("GLTF_IMPORT") << "Setting texture " << gtex.imageUuid.asString() << " for material " << matName << LL_ENDL;
                                        }
                                    }

                                    // Add material to instance's material map
                                    instance.mMaterial[matName] = impMat;
                                    LL_INFOS("GLTF_IMPORT") << "Added material " << matName << " to instance" << LL_ENDL;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

LLUUID LLGLTFLoader::imageBufferToTextureUUID(const gltf_texture& tex)
{
    if (tex.imageIdx >= mImages.size() || tex.samplerIdx >= mSamplers.size())
    {
        LL_WARNS("GLTF_IMPORT") << "Invalid texture indices in imageBufferToTextureUUID" << LL_ENDL;
        return LLUUID::null;
    }

    gltf_image& image = mImages[tex.imageIdx];
    gltf_sampler& sampler = mSamplers[tex.samplerIdx];

    S32 sourceIndex = tex.imageIdx;
    if (sourceIndex < 0 || sourceIndex >= mGLTFAsset.mImages.size())
    {
        LL_WARNS("GLTF_IMPORT") << "Invalid image index: " << sourceIndex << LL_ENDL;
        return LLUUID::null;
    }

    LL::GLTF::Image& source_image = mGLTFAsset.mImages[sourceIndex];

    // If the image already has a texture loaded, use it
    if (source_image.mTexture.notNull())
    {
        LL_INFOS("GLTF_IMPORT") << "Using already loaded texture ID: " << source_image.mTexture->getID().asString() << LL_ENDL;
        return source_image.mTexture->getID();
    }

    // Create an import material to pass to the texture load function
    LLImportMaterial material;

    // Try to get the texture filename from the URI
    if (!source_image.mUri.empty())
    {
        std::string filename = source_image.mUri;

        // Extract just the filename from the URI
        size_t pos = filename.find_last_of("/\\");
        if (pos != std::string::npos)
        {
            filename = filename.substr(pos + 1);
        }

        material.mDiffuseMapFilename = filename;
        material.mDiffuseMapLabel = filename;
    }
    else if (source_image.mBufferView >= 0)
    {
        // For embedded textures, create a pseudo-filename
        std::string pseudo_filename = "gltf_embedded_texture_" + std::to_string(sourceIndex) + ".png";
        material.mDiffuseMapFilename = pseudo_filename;
        material.mDiffuseMapLabel = pseudo_filename;
    }
    else
    {
        LL_WARNS("GLTF_IMPORT") << "No URI or buffer data for image" << LL_ENDL;
        return LLUUID::null;
    }

    // Create LLSD container with image and sampler data for texture upload
    LLSD texture_data = LLSD::emptyMap();

    // Image data
    texture_data["width"] = LLSD::Integer(image.width);
    texture_data["height"] = LLSD::Integer(image.height);
    texture_data["components"] = LLSD::Integer(image.numChannels);
    texture_data["bytes_per_component"] = LLSD::Integer(image.bytesPerChannel);
    texture_data["pixel_type"] = LLSD::Integer(image.pixelType);

    // Sampler data
    texture_data["min_filter"] = LLSD::Integer(sampler.minFilter);
    texture_data["mag_filter"] = LLSD::Integer(sampler.magFilter);
    texture_data["wrap_s"] = LLSD::Integer(sampler.wrapS);
    texture_data["wrap_t"] = LLSD::Integer(sampler.wrapT);

    // Add URI for reference
    if (!source_image.mUri.empty())
    {
        texture_data["uri"] = source_image.mUri;
    }

    // Check if we have a buffer view for embedded data
    if (source_image.mBufferView >= 0)
    {
        texture_data["has_embedded_data"] = LLSD::Boolean(true);
        texture_data["buffer_view"] = LLSD::Integer(source_image.mBufferView);

        // Extract embedded data for texture loading
        if (source_image.mBufferView < mGLTFAsset.mBufferViews.size())
        {
            const LL::GLTF::BufferView& buffer_view = mGLTFAsset.mBufferViews[source_image.mBufferView];
            if (buffer_view.mBuffer < mGLTFAsset.mBuffers.size())
            {
                const LL::GLTF::Buffer& buffer = mGLTFAsset.mBuffers[buffer_view.mBuffer];
                if (buffer_view.mByteOffset + buffer_view.mByteLength <= buffer.mData.size())
                {
                    // Add embedded data reference to texture_data
                    texture_data["buffer_index"] = LLSD::Integer(buffer_view.mBuffer);
                    texture_data["byte_offset"] = LLSD::Integer(buffer_view.mByteOffset);
                    texture_data["byte_length"] = LLSD::Integer(buffer_view.mByteLength);

                    LL_INFOS("GLTF_IMPORT") << "Found embedded texture data: offset=" << buffer_view.mByteOffset
                                           << " length=" << buffer_view.mByteLength << LL_ENDL;
                }
            }
        }
    }

    // Store the texture metadata in the binding field
    std::ostringstream ostr;
    LLSDSerialize::toXML(texture_data, ostr);
    material.mBinding = ostr.str();

    LL_INFOS("GLTF_IMPORT") << "Loading texture: " << material.mDiffuseMapFilename << LL_ENDL;

    // Flag to track if texture was loaded immediately
    bool texture_loaded = false;

    // Call texture loading function with our import material
    if (mTextureLoadFunc)
    {
        // Increment textures to fetch counter BEFORE calling load function
        mNumOfFetchingTextures++;

        U32 result = mTextureLoadFunc(material, mOpaqueData);

        // If result is 0, texture is being loaded asynchronously
        // If result is >0, texture was loaded immediately
        if (result > 0)
        {
            // Texture was loaded immediately, so decrement counter
            mNumOfFetchingTextures--;
            texture_loaded = true;

            if (material.getDiffuseMap().notNull())
            {
                LL_INFOS("GLTF_IMPORT") << "Texture loaded successfully, ID: " << material.getDiffuseMap().asString() << LL_ENDL;

                // Store the texture in the source image for future reference
                if (source_image.mTexture.isNull())
                {
                    // Create and store a texture object using the UUID
                    source_image.mTexture = LLViewerTextureManager::getFetchedTexture(material.getDiffuseMap());
                }

                return material.getDiffuseMap();
            }
        }
        else if (result == 0)
        {
            LL_INFOS("GLTF_IMPORT") << "Texture loading queued asynchronously for " << material.mDiffuseMapFilename << LL_ENDL;
        }
        else // result < 0, indicating error
        {
            // Texture loading failed, decrement counter
            mNumOfFetchingTextures--;
            LL_WARNS("GLTF_IMPORT") << "Texture loading failed for " << material.mDiffuseMapFilename << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS("GLTF_IMPORT") << "No texture loading function available" << LL_ENDL;
    }

    return LLUUID::null;
}

