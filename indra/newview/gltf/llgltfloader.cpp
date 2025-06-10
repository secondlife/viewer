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

// Premade rotation matrix, GLTF is Y-up while SL is Z-up
static const glm::mat4 coord_system_rotation(
    1.f, 0.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, -1.f, 0.f, 0.f,
    0.f, 0.f, 0.f, 1.f
);


static const glm::mat4 coord_system_rotationxy(
    0.f, 1.f, 0.f, 0.f,
    -1.f, 0.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
);

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
    U32                                 modelLimit,
    joint_rest_map_t                    jointRestMatrices) //,
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
    mMaterialsLoaded(false),
    mGeneratedModelLimit(modelLimit),
    mJointRestMatrices(jointRestMatrices)
{
}

LLGLTFLoader::~LLGLTFLoader() {}

bool LLGLTFLoader::OpenFile(const std::string &filename)
{
    tinygltf::TinyGLTF loader;
    std::string filename_lc(filename);
    LLStringUtil::toLower(filename_lc);

    mGltfLoaded = mGLTFAsset.load(filename, false);

    if (!mGltfLoaded)
    {
        notifyUnsupportedExtension(true);
        return false;
    }

    notifyUnsupportedExtension(false);

    mMeshesLoaded = parseMeshes();
    if (mMeshesLoaded) uploadMeshes();

    mMaterialsLoaded = parseMaterials();
    if (mMaterialsLoaded) uploadMaterials();

    setLoadState(DONE);

    return (mMeshesLoaded);
}

void LLGLTFLoader::addModelToScene(
    LLModel* pModel,
    U32 submodel_limit,
    const LLMatrix4& transformation,
    const LLVolumeParams& volume_params,
    const material_map& mats)
{
    U32 volume_faces = pModel->getNumVolumeFaces();

    // Side-steps all manner of issues when splitting models
    // and matching lower LOD materials to base models
    //
    pModel->sortVolumeFacesByMaterialName();

    int submodelID = 0;

    // remove all faces that definitely won't fit into one model and submodel limit
    U32 face_limit = (submodel_limit + 1) * LL_SCULPT_MESH_MAX_FACES;
    if (face_limit < volume_faces)
    {
        pModel->setNumVolumeFaces(face_limit);
    }

    LLVolume::face_list_t remainder;
    std::vector<LLModel*> ready_models;
    LLModel* current_model = pModel;
    do
    {
        current_model->trimVolumeFacesToSize(LL_SCULPT_MESH_MAX_FACES, &remainder);

        volume_faces = static_cast<U32>(remainder.size());

        // Don't add to scene yet because weights and materials aren't ready.
        // Just save it
        ready_models.push_back(current_model);

        // If we have left-over volume faces, create another model
        // to absorb them.
        if (volume_faces)
        {
            LLModel* next = new LLModel(volume_params, 0.f);
            next->ClearFacesAndMaterials();
            next->mSubmodelID = ++submodelID;
            next->mLabel = pModel->mLabel + (char)((int)'a' + next->mSubmodelID) + lod_suffix[mLod];
            next->getVolumeFaces() = remainder;
            next->mNormalizedScale = current_model->mNormalizedScale;
            next->mNormalizedTranslation = current_model->mNormalizedTranslation;
            next->mSkinWeights = current_model->mSkinWeights;
            next->mPosition = current_model->mPosition;

            const LLMeshSkinInfo& current_skin_info = current_model->mSkinInfo;
            LLMeshSkinInfo& next_skin_info = next->mSkinInfo;
            next_skin_info.mJointNames = current_skin_info.mJointNames;
            next_skin_info.mJointNums = current_skin_info.mJointNums;
            next_skin_info.mBindShapeMatrix = current_skin_info.mBindShapeMatrix;
            next_skin_info.mInvBindMatrix = current_skin_info.mInvBindMatrix;
            next_skin_info.mAlternateBindMatrix = current_skin_info.mAlternateBindMatrix;
            next_skin_info.mPelvisOffset = current_skin_info.mPelvisOffset;


            if (current_model->mMaterialList.size() > LL_SCULPT_MESH_MAX_FACES)
            {
                next->mMaterialList.assign(current_model->mMaterialList.begin() + LL_SCULPT_MESH_MAX_FACES, current_model->mMaterialList.end());
                current_model->mMaterialList.resize(LL_SCULPT_MESH_MAX_FACES);
            }

            current_model = next;
        }

        remainder.clear();

    } while (volume_faces);

    for (auto model : ready_models)
    {
        // remove unused/redundant vertices
        model->remapVolumeFaces();
        // remove unused/redundant weights and joints
        model->remapSkinWeightsAndJoints();

        mModelList.push_back(model);

        std::map<std::string, LLImportMaterial> materials;
        for (U32 i = 0; i < (U32)model->mMaterialList.size(); ++i)
        {
            material_map::const_iterator found = mats.find(model->mMaterialList[i]);
            if (found != mats.end())
            {
                materials[model->mMaterialList[i]] = found->second;
            }
            else
            {
                materials[model->mMaterialList[i]] = LLImportMaterial();
            }
        }
        mScene[transformation].push_back(LLModelInstance(model, model->mLabel, transformation, materials));
        stretch_extents(model, transformation);
    }
}

bool LLGLTFLoader::parseMeshes()
{
    if (!mGltfLoaded) return false;

    // 2022-04 DJH Volume params from dae example. TODO understand PCODE
    LLVolumeParams volume_params;
    volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);

    mTransform.setIdentity();

    for (auto& node : mGLTFAsset.mNodes)
    {
        // Make node matrix valid for correct transformation
        node.makeMatrixValid();
    }

    if (mGLTFAsset.mSkins.size() > 0)
    {
        checkForXYrotation(mGLTFAsset.mSkins[0]);
    }

    // Populate the joints from skins first.
    // There's not many skins - and you can pretty easily iterate through the nodes from that.
    for (S32 i = 0; i < mGLTFAsset.mSkins.size(); i++)
    {
        populateJointFromSkin(i);
    }

    // Track how many times each mesh name has been used
    std::map<std::string, S32> mesh_name_counts;
    U32 submodel_limit = mGLTFAsset.mNodes.size() > 0 ? mGeneratedModelLimit / (U32)mGLTFAsset.mNodes.size() : 0;

    // Build parent mapping for efficient traversal
    std::vector<S32> node_parents(mGLTFAsset.mNodes.size(), -1);
    std::vector<bool> is_root(mGLTFAsset.mNodes.size(), true);

    // Build parent relationships
    for (size_t parent_idx = 0; parent_idx < mGLTFAsset.mNodes.size(); parent_idx++)
    {
        const auto& parent_node = mGLTFAsset.mNodes[parent_idx];
        for (S32 child_idx : parent_node.mChildren)
        {
            if (child_idx >= 0 && child_idx < static_cast<S32>(mGLTFAsset.mNodes.size()))
            {
                node_parents[child_idx] = static_cast<S32>(parent_idx);
                is_root[child_idx] = false;
            }
        }
    }

    // Process all root nodes and their hierarchies
    for (size_t node_idx = 0; node_idx < mGLTFAsset.mNodes.size(); node_idx++)
    {
        if (is_root[node_idx])
        {
            processNodeHierarchy(static_cast<S32>(node_idx), mesh_name_counts, submodel_limit, volume_params);
        }
    }

    return true;
}

void LLGLTFLoader::processNodeHierarchy(S32 node_idx, std::map<std::string, S32>& mesh_name_counts, U32 submodel_limit, const LLVolumeParams& volume_params)
{
    if (node_idx < 0 || node_idx >= static_cast<S32>(mGLTFAsset.mNodes.size()))
        return;

    auto& node = mGLTFAsset.mNodes[node_idx];

    // Process this node's mesh if it has one
    if (node.mMesh >= 0 && node.mMesh < mGLTFAsset.mMeshes.size())
    {
        LLMatrix4    transformation;
        material_map mats;

        LLModel* pModel = new LLModel(volume_params, 0.f);
        auto& mesh = mGLTFAsset.mMeshes[node.mMesh];

        // Get base mesh name and track usage
        std::string base_name = mesh.mName;
        if (base_name.empty())
        {
            base_name = "mesh_" + std::to_string(node.mMesh);
        }

        S32 instance_count = mesh_name_counts[base_name]++;

        if (populateModelFromMesh(pModel, mesh, node, mats, instance_count) &&
            (LLModel::NO_ERRORS == pModel->getStatus()) &&
            validate_model(pModel))
        {
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

            if (node.mSkin >= 0)
            {
                // "Bind Shape Matrix" is supposed to transform the geometry of the skinned mesh
                // into the coordinate space of the joints.
                // In GLTF, this matrix is omitted, and it is assumed that this transform is either
                // premultiplied with the mesh data, or postmultiplied to the inverse bind matrices.
                //
                // TODO: There appears to be missing rotation when joints rotate the model
                // or inverted bind matrices are missing inherited rotation
                // (based of values the 'bento shoes' mesh might be missing 90 degrees horizontaly
                // prior to skinning)

                pModel->mSkinInfo.mBindShapeMatrix.loadu(mesh_scale);
                LL_INFOS("GLTF_DEBUG") << "Model: " << pModel->mLabel << " mBindShapeMatrix: " << pModel->mSkinInfo.mBindShapeMatrix << LL_ENDL;
            }

            if (transformation.determinant() < 0)
            { // negative scales are not supported
                LL_INFOS("GLTF_IMPORT") << "Negative scale detected, unsupported post-normalization transform.  domInstance_geometry: "
                           << pModel->mLabel << LL_ENDL;
                LLSD args;
                args["Message"] = "NegativeScaleNormTrans";
                args["LABEL"]   = pModel->mLabel;
                mWarningsArray.append(args);
            }

            addModelToScene(pModel, submodel_limit, transformation, volume_params, mats);
            mats.clear();
        }
        else
        {
            setLoadState(ERROR_MODEL + pModel->getStatus());
            delete pModel;
            return;
        }
    }

    // Process all children recursively
    for (S32 child_idx : node.mChildren)
    {
        processNodeHierarchy(child_idx, mesh_name_counts, submodel_limit, volume_params);
    }
}

void LLGLTFLoader::computeCombinedNodeTransform(const LL::GLTF::Asset& asset, S32 node_index, glm::mat4& combined_transform) const
{
    if (node_index < 0 || node_index >= static_cast<S32>(asset.mNodes.size()))
    {
        combined_transform = glm::mat4(1.0f);
        return;
    }

    const auto& node = asset.mNodes[node_index];

    // Ensure the node's matrix is valid
    const_cast<LL::GLTF::Node&>(node).makeMatrixValid();

    // Start with this node's transform
    combined_transform = node.mMatrix;

    // Find and apply parent transform if it exists
    for (size_t i = 0; i < asset.mNodes.size(); ++i)
    {
        const auto& potential_parent = asset.mNodes[i];
        auto it = std::find(potential_parent.mChildren.begin(), potential_parent.mChildren.end(), node_index);

        if (it != potential_parent.mChildren.end())
        {
            // Found parent - recursively get its combined transform and apply it
            glm::mat4 parent_transform;
            computeCombinedNodeTransform(asset, static_cast<S32>(i), parent_transform);
            combined_transform = parent_transform * combined_transform;
            return; // Early exit - a node can only have one parent
        }
    }
}

bool LLGLTFLoader::populateModelFromMesh(LLModel* pModel, const LL::GLTF::Mesh& mesh, const LL::GLTF::Node& nodeno, material_map& mats, S32 instance_count)
{
    // Create unique model name
    std::string base_name = mesh.mName;
    if (base_name.empty())
    {
        S32 mesh_index = static_cast<S32>(&mesh - &mGLTFAsset.mMeshes[0]);
        base_name = "mesh_" + std::to_string(mesh_index);
    }

    LL_INFOS("GLTF_DEBUG") << "Processing model " << base_name << LL_ENDL;

    if (instance_count > 0)
    {
        pModel->mLabel = base_name + "_copy_" + std::to_string(instance_count);
    }
    else
    {
        pModel->mLabel = base_name;
    }

    pModel->ClearFacesAndMaterials();

    S32 skinIdx = nodeno.mSkin;

    // Compute final combined transform matrix (hierarchy + coordinate rotation)
    S32 node_index = static_cast<S32>(&nodeno - &mGLTFAsset.mNodes[0]);
    glm::mat4 hierarchy_transform;
    computeCombinedNodeTransform(mGLTFAsset, node_index, hierarchy_transform);

    // Combine transforms: coordinate rotation applied to hierarchy transform
    glm::mat4 final_transform = coord_system_rotation * hierarchy_transform;
    if (mApplyXYRotation)
    {
        final_transform = coord_system_rotationxy * final_transform;
    }

    // Check if we have a negative scale (flipped coordinate system)
    bool hasNegativeScale = glm::determinant(final_transform) < 0.0f;

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

            std::string legal_name(jointNode.mName);
            if (mJointMap.find(legal_name) == mJointMap.end())
            {
                gltf_joint_index_use_count[i] = -1; // mark as unsupported
            }
        }
    }

    for (const LL::GLTF::Primitive& prim : mesh.mPrimitives)
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

                if (!prim.mNormals.empty())
                {
                    // Use pre-computed normal_transform
                    glm::vec3 normal_vec(prim.mNormals[i][0], prim.mNormals[i][1], prim.mNormals[i][2]);
                    vert.normal = glm::normalize(normal_transform * normal_vec);
                }
                else
                {
                    // Use default normal (pointing up in model space)
                    vert.normal = glm::normalize(normal_transform * glm::vec3(0.0f, 0.0f, 1.0f));
                    LL_DEBUGS("GLTF_IMPORT") << "No normals found for primitive, using default normal." << LL_ENDL;
                }

                vert.uv0 = glm::vec2(prim.mTexCoords0[i][0], -prim.mTexCoords0[i][1]);

                if (skinIdx >= 0)
                {
                    vert.weights = glm::vec4(prim.mWeights[i]);

                    auto accessorIdx = prim.mAttributes.at("JOINTS_0");
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
                    else
                    {
                        vert.joints = glm::zero<glm::u16vec4>();
                        vert.weights = glm::zero<glm::vec4>();
                    }
                }
                vertices.push_back(vert);
            }

            if (prim.getIndexCount() % 3 != 0)
            {
                LL_WARNS("GLTF_IMPORT") << "Invalid primitive: index count " << prim.getIndexCount()
                                       << " is not divisible by 3. GLTF files must contain triangulated geometry." << LL_ENDL;
                LLSD args;
                args["Message"] = "InvalidGeometryNonTriangulated";
                mWarningsArray.append(args);
                continue; // Skip this primitive
            }

            // When processing indices, flip winding order if needed
            for (U32 i = 0; i < prim.getIndexCount(); i += 3)
            {
                if (hasNegativeScale)
                {
                    // Flip winding order for negative scale
                    indices.push_back(prim.mIndexArray[i]);
                    indices.push_back(prim.mIndexArray[i + 2]); // Swap these two
                    indices.push_back(prim.mIndexArray[i + 1]);
                }
                else
                {
                    indices.push_back(prim.mIndexArray[i]);
                    indices.push_back(prim.mIndexArray[i + 1]);
                    indices.push_back(prim.mIndexArray[i + 2]);
                }
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
            LL_INFOS("GLTF_IMPORT") << "Unable to process mesh due to 16-bit index limits" << LL_ENDL;
            LLSD args;
            args["Message"] = "ErrorIndexLimit";
            mWarningsArray.append(args);
            return false;
        }
    }

    // Call normalizeVolumeFacesAndWeights to compute proper extents
    pModel->normalizeVolumeFacesAndWeights();

    // Fill joint names, bind matrices and prepare to remap weight indices
    if (skinIdx >= 0)
    {
        LL::GLTF::Skin& gltf_skin = mGLTFAsset.mSkins[skinIdx];
        LLMeshSkinInfo& skin_info = pModel->mSkinInfo;

        std::vector<S32> gltfindex_to_joitindex_map;
        size_t jointCnt = gltf_skin.mJoints.size();
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

            // In scope of same skin multiple meshes reuse same bind matrices
            skin_info.mInvBindMatrix.push_back(mInverseBindMatrices[skinIdx][i]);

            // For alternate bind matrix, use the ORIGINAL joint transform (before rotation)
            // Get the original joint node and use its matrix directly
            // Todo: this seems blatantly wrong, it should have been rotated
            glm::mat4 joint_mat = jointNode.mMatrix;
            S32 root_joint = findValidRootJointNode(joint, gltf_skin); // skeleton can have multiple real roots
            if (root_joint == joint)
            {
                // This is very likely incomplete in some way.
                // Root shouldn't be the only one to need full coordinate fix
                joint_mat = coord_system_rotation * joint_mat;
                if (mApplyXYRotation)
                {
                    joint_mat = coord_system_rotationxy * joint_mat;
                }
            }
            LLMatrix4 original_joint_transform(glm::value_ptr(joint_mat));

            LL_INFOS("GLTF_DEBUG") << "mAlternateBindMatrix name: " << legal_name << " val: " << original_joint_transform << LL_ENDL;
            skin_info.mAlternateBindMatrix.push_back(LLMatrix4a(original_joint_transform));
        }

        // Remap indices for pModel->mSkinWeights
        // Todo: this is now partially redundant due to
        // remapSkinWeightsAndJoints being called later.
        // Consider storing all joints now as is and let it
        // ramap later due to missing weights.
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

void LLGLTFLoader::populateJointFromSkin(S32 skin_idx)
{
    const LL::GLTF::Skin& skin = mGLTFAsset.mSkins[skin_idx];

    LL_INFOS("GLTF_DEBUG") << "populateJointFromSkin: Processing skin " << skin_idx << " with " << skin.mJoints.size() << " joints" << LL_ENDL;

    if (skin.mInverseBindMatrices > 0 && skin.mJoints.size() != skin.mInverseBindMatricesData.size())
    {
        LL_INFOS("GLTF_IMPORT") << "Bind matrices count mismatch joints count" << LL_ENDL;
        LLSD args;
        args["Message"] = "InvBindCountMismatch";
        mWarningsArray.append(args);
    }

    S32 joint_count = (S32)skin.mJoints.size();
    S32 inverse_count = (S32)skin.mInverseBindMatricesData.size();
    if (mInverseBindMatrices.size() <= skin_idx)
    {
        mInverseBindMatrices.resize(skin_idx + 1);
    }

    for (S32 i = 0; i < joint_count; i++)
    {
        S32 joint = skin.mJoints[i];
        LL::GLTF::Node jointNode = mGLTFAsset.mNodes[joint];
        std::string legal_name(jointNode.mName);
        bool legal_joint = false;
        if (mJointMap.find(legal_name) != mJointMap.end())
        {
            legal_name = mJointMap[legal_name];
            legal_joint = true;
        }

        if (!legal_joint)
        {
            // add placeholder to not break index
            LLMatrix4 gltf_transform;
            gltf_transform.setIdentity();
            mInverseBindMatrices[skin_idx].push_back(LLMatrix4a(gltf_transform));
        }
        else if (inverse_count > i)
        {
            glm::mat4 original_bind_matrix = glm::inverse(skin.mInverseBindMatricesData[i]);
            glm::mat4 rotated_original = coord_system_rotation * original_bind_matrix;
            glm::mat4 skeleton_transform = computeGltfToViewerSkeletonTransform(skin, i, legal_name);
            glm::mat4 tranlated_original = skeleton_transform * rotated_original;
            glm::mat4 final_inverse_bind_matrix = glm::inverse(tranlated_original);

            LLMatrix4 gltf_transform = LLMatrix4(glm::value_ptr(final_inverse_bind_matrix));

            LL_INFOS("GLTF_DEBUG") << "mInvBindMatrix name: " << legal_name << " val: " << gltf_transform << LL_ENDL;
            mInverseBindMatrices[skin_idx].push_back(LLMatrix4a(gltf_transform));
        }
        else
        {
            glm::mat4 inv_bind(1.0f);
            glm::mat4 skeleton_transform = computeGltfToViewerSkeletonTransform(skin, i, legal_name);
            inv_bind = glm::inverse(skeleton_transform * inv_bind);

            LLMatrix4 gltf_transform = LLMatrix4(glm::value_ptr(inv_bind));
            gltf_transform.setIdentity();
            LL_INFOS("GLTF_DEBUG") << "mInvBindMatrix name: " << legal_name << " val: " << gltf_transform << LL_ENDL;
            mInverseBindMatrices[skin_idx].push_back(LLMatrix4a(gltf_transform));
        }
        // todo: prepare mAlternateBindMatrix here

        if (!legal_joint)
        {
            LL_DEBUGS("GLTF") << "Ignoring unrecognized joint: " << legal_name << LL_ENDL;
            continue;
        }

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


S32 LLGLTFLoader::findClosestValidJoint(S32 source_joint, const LL::GLTF::Skin& gltf_skin) const
{
    S32 source_joint_node = gltf_skin.mJoints[source_joint];
    S32 root_node = source_joint_node;
    S32 found_node = source_joint_node;
    S32 size = (S32)gltf_skin.mJoints.size();
    do
    {
        root_node = found_node;
        for (S32 i = 0; i < size; i++)
        {
            S32 joint = gltf_skin.mJoints[i];
            const LL::GLTF::Node& jointNode = mGLTFAsset.mNodes[joint];
            std::vector<S32>::const_iterator it = std::find(jointNode.mChildren.begin(), jointNode.mChildren.end(), root_node);
            if (it != jointNode.mChildren.end())
            {
                // Found node's parent
                found_node = joint;
                if (mJointMap.find(jointNode.mName) != mJointMap.end())
                {
                    return i;
                }
                break;
            }
        }
    } while (root_node != found_node);

    return -1;
}

S32 LLGLTFLoader::findValidRootJointNode(S32 source_joint_node, const LL::GLTF::Skin& gltf_skin) const
{
    S32 root_node = 0;
    S32 found_node = source_joint_node;
    S32 size = (S32)gltf_skin.mJoints.size();
    do
    {
        root_node = found_node;
        for (S32 i = 0; i < size; i++)
        {
            S32 joint = gltf_skin.mJoints[i];
            const LL::GLTF::Node& jointNode = mGLTFAsset.mNodes[joint];

            if (mJointMap.find(jointNode.mName) != mJointMap.end())
            {
                std::vector<S32>::const_iterator it = std::find(jointNode.mChildren.begin(), jointNode.mChildren.end(), root_node);
                if (it != jointNode.mChildren.end())
                {
                    // Found node's parent
                    found_node = joint;
                    break;
                }
            }
        }
    } while (root_node != found_node);

    return root_node;
}

S32 LLGLTFLoader::findGLTFRootJointNode(const LL::GLTF::Skin& gltf_skin) const
{
    S32 root_node = 0;
    S32 found_node = 0;
    S32 size = (S32)gltf_skin.mJoints.size();
    do
    {
        root_node = found_node;
        for (S32 i = 0; i < size; i++)
        {
            S32 joint = gltf_skin.mJoints[i];
            const LL::GLTF::Node& jointNode = mGLTFAsset.mNodes[joint];
            std::vector<S32>::const_iterator it = std::find(jointNode.mChildren.begin(), jointNode.mChildren.end(), root_node);
            if (it != jointNode.mChildren.end())
            {
                // Found node's parent
                found_node = joint;
                break;
            }
        }
    } while (root_node != found_node);

    LL_INFOS("GLTF_DEBUG") << "mJointList name: ";
    const LL::GLTF::Node& jointNode = mGLTFAsset.mNodes[root_node];
    LL_CONT << jointNode.mName << " index: " << root_node << LL_ENDL;
    return root_node;
}

S32 LLGLTFLoader::findParentNode(S32 node) const
{
    S32 size = (S32)mGLTFAsset.mNodes.size();
    for (S32 i = 0; i < size; i++)
    {
        const LL::GLTF::Node& jointNode = mGLTFAsset.mNodes[i];
        std::vector<S32>::const_iterator it = std::find(jointNode.mChildren.begin(), jointNode.mChildren.end(), node);
        if (it != jointNode.mChildren.end())
        {
            return i;
        }
    }
    return -1;
}

glm::mat4 LLGLTFLoader::buildGltfRestMatrix(S32 joint_node_index, const LL::GLTF::Skin& gltf_skin) const
{
    // This is inefficient since we are recalculating some joints multiple times over
    // Todo: cache it?

    if (joint_node_index < 0 || joint_node_index >= static_cast<S32>(mGLTFAsset.mNodes.size()))
    {
        return glm::mat4(1.0f);
    }

    const auto& node = mGLTFAsset.mNodes[joint_node_index];

    // Find and apply parent transform if it exists
    for (size_t i = 0; i < mGLTFAsset.mNodes.size(); ++i)
    {
        const auto& potential_parent = mGLTFAsset.mNodes[i];
        auto it = std::find(potential_parent.mChildren.begin(), potential_parent.mChildren.end(), joint_node_index);

        if (it != potential_parent.mChildren.end())
        {
            // Found parent
            if (std::find(gltf_skin.mJoints.begin(), gltf_skin.mJoints.end(), joint_node_index) != gltf_skin.mJoints.end())
            {
                // parent is a joint - recursively combine transform
                // assumes that matrix is already valid
                return buildGltfRestMatrix(static_cast<S32>(i), gltf_skin) * node.mMatrix;
            }
        }
    }
    // Should we return armature or stop earlier?
    return node.mMatrix;
}

// This function computes the transformation matrix needed to convert from GLTF skeleton space
// to viewer skeleton space for a specific joint
glm::mat4 LLGLTFLoader::computeGltfToViewerSkeletonTransform(const LL::GLTF::Skin& gltf_skin, S32 gltf_joint_index, const std::string& joint_name) const
{
    joint_rest_map_t::const_iterator found = mJointRestMatrices.find(joint_name);
    if (found == mJointRestMatrices.end())
    {
        // For now assume they are identical and return an identity (for ease of debuging)
        // But there should be no joints viewer isn't aware about
        // Warn or assert about missing joints
        return glm::mat4(1.0f);
    }
    glm::mat4 viewer_joint_rest_pose = found->second;

    // Get the GLTF joint's rest pose (in GLTF coordinate system)
    S32 joint_node_index = gltf_skin.mJoints[gltf_joint_index];
    glm::mat4 gltf_joint_rest_pose = buildGltfRestMatrix(joint_node_index, gltf_skin);
    gltf_joint_rest_pose = coord_system_rotation * gltf_joint_rest_pose;

    LL_INFOS("GLTF_DEBUG") << "rest matrix for joint " << joint_name << ": ";

    LLMatrix4 transform(glm::value_ptr(gltf_joint_rest_pose));

    LL_CONT << transform << LL_ENDL;

    // Compute transformation from GLTF space to viewer space
    // This assumes both skeletons are in rest pose initially
    if (mApplyXYRotation)
    {
        return viewer_joint_rest_pose * glm::inverse(gltf_joint_rest_pose);
    }
    return viewer_joint_rest_pose * glm::inverse(gltf_joint_rest_pose);
}

bool LLGLTFLoader::checkForXYrotation(const LL::GLTF::Skin& gltf_skin, S32 joint_idx, S32 bind_indx)
{
    glm::mat4 gltf_joint_rest = buildGltfRestMatrix(joint_idx, gltf_skin);
    glm::mat4 test_mat = glm::inverse(gltf_joint_rest) * gltf_skin.mInverseBindMatricesData[bind_indx];
    // Normally for shoulders it should be something close to
    // {1,0,0,0;0,-1,0,0;0,0,-1,0;0,0,0,1}
    // rotated one will look like
    // {0,0,0,-1;1,0,0,0;0,-1,0,0;0,0,0,1}
    // Todo: This is a cheap hack,
    // figure out how rotation is supposed to work
    return abs(test_mat[0][0]) < 0.5 && abs(test_mat[1][1]) < 0.5 && abs(test_mat[2][2]) < 0.5;
}

void LLGLTFLoader::checkForXYrotation(const LL::GLTF::Skin& gltf_skin)
{
    // HACK: figure out model's rotation from shoulders' matrix.
    // This is wrong on many levels:
    // Too limited (only models that have shoulders),
    // Will not work well with things that emulate 3 hands in some manner
    // Only supports xy 90 degree rotation
    // Todo: figure out how to find skeleton's orientation Correctly
    // when model is rotated at a triangle level
    constexpr char right_shoulder_str[] = "mShoulderRight";
    constexpr char left_shoulder_str[] = "mShoulderLeft";

    S32 size = (S32)gltf_skin.mJoints.size();
    S32 joints_found = 0;
    for (S32 i= 0; i < size; i++)
    {
        S32 joint = gltf_skin.mJoints[i];
        auto joint_node = mGLTFAsset.mNodes[joint];

        // todo: we are doing this search thing everywhere,
        // just pre-translate every joint
        JointMap::iterator found = mJointMap.find(joint_node.mName);
        if (found == mJointMap.end())
        {
            // unsupported joint
            continue;
        }
        if (found->second == right_shoulder_str || found->second == left_shoulder_str)
        {
            if (checkForXYrotation(gltf_skin, joint, i))
            {
                joints_found++;
            }
            else
            {
                return;
            }
        }
    }

    if (joints_found == 2)
    {
        // Both joints in a weird position/rotation, assume rotated model
        mApplyXYRotation = true;
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

void LLGLTFLoader::notifyUnsupportedExtension(bool unsupported)
{
    std::vector<std::string> extensions = unsupported ? mGLTFAsset.mUnsupportedExtensions : mGLTFAsset.mIgnoredExtensions;
    if (extensions.size() > 0)
    {
        LLSD args;
        args["Message"] = unsupported ? "UnsupportedExtension" : "IgnoredExtension";
        std::string del;
        std::string ext;
        for (auto& extension : extensions)
        {
            ext += del;
            ext += extension;
            del = ",";
        }
        args["EXT"] = ext;
        mWarningsArray.append(args);
    }
}

