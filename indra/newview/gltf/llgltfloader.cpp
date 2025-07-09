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
#include "llbase64.h"
#include "lldir.h"

#include "llmatrix4a.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>

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

static const S32 VERTICIES_LIMIT = USHRT_MAX - 2;

LLGLTFLoader::LLGLTFLoader(std::string                filename,
    S32                                               lod,
    LLModelLoader::load_callback_t                    load_cb,
    LLModelLoader::joint_lookup_func_t                joint_lookup_func,
    LLModelLoader::texture_load_func_t                texture_load_func,
    LLModelLoader::state_callback_t                   state_cb,
    void *                                            opaque_userdata,
    JointTransformMap &                               jointTransformMap,
    JointNameSet &                                    jointsFromNodes,
    std::map<std::string, std::string, std::less<>> & jointAliasMap,
    U32                                               maxJointsPerMesh,
    U32                                               modelLimit,
    U32                                               debugMode,
    std::vector<LLJointData>                          viewer_skeleton) //,
    //bool                                            preprocess)
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
                     maxJointsPerMesh,
                     modelLimit,
                     debugMode)
    , mViewerJointData(viewer_skeleton)
    , mGltfLoaded(false)
    , mApplyXYRotation(false)
{
}

LLGLTFLoader::~LLGLTFLoader() {}

bool LLGLTFLoader::OpenFile(const std::string &filename)
{
    tinygltf::TinyGLTF loader;
    std::string filename_lc(filename);
    LLStringUtil::toLower(filename_lc);

    try
    {
        mGltfLoaded = mGLTFAsset.load(filename, false);
    }
    catch (const std::exception& e)
    {
        LL_WARNS() << "Exception in LLModelLoader::run: " << e.what() << LL_ENDL;
        LLSD args;
        args["Message"] = "ParsingErrorException";
        args["FILENAME"] = filename;
        args["EXCEPTION"] = e.what();
        mWarningsArray.append(args);
        setLoadState(ERROR_PARSING);
        return false;
    }
    catch (...)
    {
        LOG_UNHANDLED_EXCEPTION("LLGLTFLoader");
        LLSD args;
        args["Message"] = "ParsingErrorException";
        args["FILENAME"] = filename;
        args["EXCEPTION"] = "Unknown exception";
        mWarningsArray.append(args);
        setLoadState(ERROR_PARSING);
        return false;
    }

    if (!mGltfLoaded)
    {
        notifyUnsupportedExtension(true);

        for (const auto& buffer : mGLTFAsset.mBuffers)
        {
            if (buffer.mByteLength > 0 && buffer.mData.empty())
            {
                bool bin_file = buffer.mUri.ends_with(".bin");
                LLSD args;
                args["Message"] = bin_file ? "ParsingErrorMissingBufferBin" : "ParsingErrorMissingBuffer";
                args["BUFFER_NAME"] = buffer.mName;
                args["BUFFER_URI"] = buffer.mUri;
                mWarningsArray.append(args);
            }
        }
        setLoadState(ERROR_PARSING);
        return false;
    }

    notifyUnsupportedExtension(false);

    bool meshesLoaded = parseMeshes();

    setLoadState(DONE);

    return meshesLoaded;
}

void LLGLTFLoader::addModelToScene(
    LLModel* pModel,
    const std::string& model_name,
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
        LL_WARNS("GLTF_IMPORT") << "Model contains " << volume_faces
            << " faces, exceeding the limit of " << face_limit << LL_ENDL;

        LLSD args;
        args["Message"] = "ModelTooManySubmodels";
        args["MODEL_NAME"] = pModel->mLabel;
        args["SUBMODEL_COUNT"] = static_cast<S32>(llfloor((F32)volume_faces / LL_SCULPT_MESH_MAX_FACES));
        args["SUBMODEL_LIMIT"] = static_cast<S32>(submodel_limit);
        mWarningsArray.append(args);

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

            std::string instance_name = model_name;
            if (next->mSubmodelID > 0)
            {
                instance_name += (char)((int)'a' + next->mSubmodelID);
            }
            // Check for duplicates and add copy suffix if needed
            int duplicate_count = 0;
            for (const auto& inst : mScene[transformation])
            {
                if (inst.mLabel == instance_name)
                {
                    ++duplicate_count;
                }
            }
            if (duplicate_count > 0) {
                instance_name += "_copy_" + std::to_string(duplicate_count);
            }
            next->mLabel = instance_name;

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
        // Keep base name for scene instance.
        std::string instance_name = model->mLabel;
        // Add suffix. Suffix is nessesary for model matching logic
        // because sometimes higher lod can be used as a lower one, so models
        // need unique names not just in scope of one lod, but across lods.
        model->mLabel += lod_suffix[mLod];
        mScene[transformation].push_back(LLModelInstance(model, instance_name, transformation, materials));
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
        populateJointGroups();
    }

    // Populate the joints from skins first.
    // Multiple meshes can share the same skin, so preparing skins beforehand.
    for (S32 i = 0; i < mGLTFAsset.mSkins.size(); i++)
    {
        populateJointsFromSkin(i);
    }

    // Track how many times each mesh name has been used
    std::map<std::string, S32> mesh_name_counts;

    // For now use mesh count, but might be better to do 'mNodes.size() - joints count'.
    U32 submodel_limit = mGLTFAsset.mMeshes.size() > 0 ? mGeneratedModelLimit / (U32)mGLTFAsset.mMeshes.size() : 0;

    // Check if we have scenes defined
    if (!mGLTFAsset.mScenes.empty())
    {
        // Process the default scene (or first scene if no default)
        S32 scene_idx = mGLTFAsset.mScene >= 0 ? mGLTFAsset.mScene : 0;

        if (scene_idx < mGLTFAsset.mScenes.size())
        {
            const LL::GLTF::Scene& scene = mGLTFAsset.mScenes[scene_idx];

            LL_INFOS("GLTF_IMPORT") << "Processing scene " << scene_idx << " with " << scene.mNodes.size() << " root nodes" << LL_ENDL;

            // Process all root nodes defined in the scene
            for (S32 root_idx : scene.mNodes)
            {
                if (root_idx >= 0 && root_idx < static_cast<S32>(mGLTFAsset.mNodes.size()))
                {
                    processNodeHierarchy(root_idx, mesh_name_counts, submodel_limit, volume_params);
                }
            }
        }
    }
    else
    {
        LL_WARNS("GLTF_IMPORT") << "No scenes defined in GLTF file" << LL_ENDL;

        LLSD args;
        args["Message"] = "NoScenesFound";
        mWarningsArray.append(args);
        return false;
    }

    checkGlobalJointUsage();

    return true;
}

void LLGLTFLoader::processNodeHierarchy(S32 node_idx, std::map<std::string, S32>& mesh_name_counts, U32 submodel_limit, const LLVolumeParams& volume_params)
{
    if (node_idx < 0 || node_idx >= static_cast<S32>(mGLTFAsset.mNodes.size()))
        return;

    const LL::GLTF::Node& node = mGLTFAsset.mNodes[node_idx];

    LL_DEBUGS("GLTF_IMPORT") << "Processing node " << node_idx << " (" << node.mName << ")"
                            << " - has mesh: " << (node.mMesh >= 0 ? "yes" : "no")
                            << " - children: " << node.mChildren.size() << LL_ENDL;

    // Process this node's mesh if it has one
    if (node.mMesh >= 0 && node.mMesh < mGLTFAsset.mMeshes.size())
    {
        LLMatrix4    transformation;
        material_map mats;

        LLModel* pModel = new LLModel(volume_params, 0.f);
        const LL::GLTF::Mesh& mesh = mGLTFAsset.mMeshes[node.mMesh];

        // Get base mesh name and track usage
        std::string base_name = getLodlessLabel(mesh);
        if (base_name.empty())
        {
            base_name = "mesh_" + std::to_string(node.mMesh);
        }

        S32 instance_count = mesh_name_counts[base_name]++;

        // make name unique
        if (instance_count > 0)
        {
            base_name = base_name + "_copy_" + std::to_string(instance_count);
        }

        if (populateModelFromMesh(pModel, base_name, mesh, node, mats) &&
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

            addModelToScene(pModel, base_name, submodel_limit, transformation, volume_params, mats);
            mats.clear();
        }
        else
        {
            setLoadState(ERROR_MODEL + pModel->getStatus());
            delete pModel;
            return;
        }
    }
    else if (node.mMesh >= 0)
    {
        // Log invalid mesh reference
        LL_WARNS("GLTF_IMPORT") << "Node " << node_idx << " (" << node.mName
                                << ") references invalid mesh " << node.mMesh
                                << " (total meshes: " << mGLTFAsset.mMeshes.size() << ")" << LL_ENDL;

        LLSD args;
        args["Message"] = "InvalidMeshReference";
        args["NODE_NAME"] = node.mName;
        args["MESH_INDEX"] = node.mMesh;
        args["TOTAL_MESHES"] = static_cast<S32>(mGLTFAsset.mMeshes.size());
        mWarningsArray.append(args);
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

bool LLGLTFLoader::addJointToModelSkin(LLMeshSkinInfo& skin_info, S32 gltf_skin_idx, size_t gltf_joint_idx)
{
    const std::string& legal_name = mJointNames[gltf_skin_idx][gltf_joint_idx];
    if (legal_name.empty())
    {
        llassert(false); // should have been stopped by gltf_joint_index_use[i] == -1
        return false;
    }
    skin_info.mJointNames.push_back(legal_name);
    skin_info.mJointNums.push_back(-1);

    // In scope of same skin multiple meshes reuse same bind matrices
    skin_info.mInvBindMatrix.push_back(mInverseBindMatrices[gltf_skin_idx][gltf_joint_idx]);
    skin_info.mAlternateBindMatrix.push_back(mAlternateBindMatrices[gltf_skin_idx][gltf_joint_idx]);

    // Track joint usage for this skin, for the sake of unused joints detection
    mJointUsage[gltf_skin_idx][gltf_joint_idx]++;

    return true;
}

bool LLGLTFLoader::populateModelFromMesh(LLModel* pModel, const std::string& base_name, const LL::GLTF::Mesh& mesh, const LL::GLTF::Node& nodeno, material_map& mats)
{
    // Set the requested label for the floater display and uploading
    pModel->mRequestedLabel = gDirUtilp->getBaseFileName(mFilename, true);
    // Set only name, suffix will be added later
    pModel->mLabel = base_name;

    LL_DEBUGS("GLTF_DEBUG") << "Processing model " << pModel->mLabel << LL_ENDL;

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
    std::vector<S32> gltf_joint_index_use;
    if (skinIdx >= 0 && mGLTFAsset.mSkins.size() > skinIdx)
    {
        LL::GLTF::Skin& gltf_skin = mGLTFAsset.mSkins[skinIdx];

        size_t jointCnt = gltf_skin.mJoints.size();
        gltf_joint_index_use.resize(jointCnt, 0);

        for (size_t i = 0; i < jointCnt; ++i)
        {
            if (mJointNames[skinIdx][i].empty())
            {
                // This might need to hold a substitute index
                gltf_joint_index_use[i] = -1; // mark as unsupported
            }
        }
    }

    for (size_t prim_idx = 0; prim_idx < mesh.mPrimitives.size(); ++prim_idx)
    {
        const LL::GLTF::Primitive& prim = mesh.mPrimitives[prim_idx];

        // So primitives already have all of the data we need for a given face in SL land.
        // Primitives may only ever have a single material assigned to them - as the relation is 1:1 in terms of intended draw call
        // count. Just go ahead and populate faces direct from the GLTF primitives here. -Geenz 2025-04-07
        LLVolumeFace face;
        std::vector<GLTFVertex> vertices;

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

                            LL_INFOS("GLTF_IMPORT") << "Found texture: " << impMat.mDiffuseMapFilename
                                << " for material: " << material->mName << LL_ENDL;

                            LLSD args;
                            args["Message"] = "TextureFound";
                            args["TEXTURE_NAME"] = impMat.mDiffuseMapFilename;
                            args["MATERIAL_NAME"] = material->mName;
                            mWarningsArray.append(args);

                            // If the image has a texture loaded already, use it
                            if (image.mTexture.notNull())
                            {
                                impMat.setDiffuseMap(image.mTexture->getID());
                                LL_INFOS("GLTF_IMPORT") << "Using existing texture ID: " << image.mTexture->getID().asString() << LL_ENDL;
                            }
                            else
                            {
                                // Texture will be loaded later through the callback system
                                LL_INFOS("GLTF_IMPORT") << "Texture needs loading: " << impMat.mDiffuseMapFilename << LL_ENDL;
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
                            std::string temp_filename = extractTextureToTempFile(texIndex, "base_color");
                            if (!temp_filename.empty())
                            {
                                impMat.mDiffuseMapFilename = temp_filename;
                                impMat.mDiffuseMapLabel = material->mName.empty() ? temp_filename : material->mName;
                            }
                        }
                    }
                }
            }
        }

        if (prim.getIndexCount() % 3 != 0)
        {
            LL_WARNS("GLTF_IMPORT") << "Mesh '" << mesh.mName << "' primitive " << prim_idx
                << ": Invalid index count " << prim.getIndexCount()
                << " (not divisible by 3). GLTF files must contain triangulated geometry." << LL_ENDL;

            LLSD args;
            args["Message"] = "InvalidGeometryNonTriangulated";
            args["MESH_NAME"] = mesh.mName;
            args["PRIMITIVE_INDEX"] = static_cast<S32>(prim_idx);
            args["INDEX_COUNT"] = static_cast<S32>(prim.getIndexCount());
            mWarningsArray.append(args);
            return false; // Skip this primitive
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

        // Check for empty vertex array before processing
        if (vertices.empty())
        {
            LL_WARNS("GLTF_IMPORT") << "Empty vertex array for primitive " << prim_idx << " in model " << mesh.mName << LL_ENDL;
            LLSD args;
            args["Message"] = "EmptyVertexArray";
            args["MESH_NAME"] = mesh.mName;
            args["PRIMITIVE_INDEX"] = static_cast<S32>(prim_idx);
            args["INDEX_COUNT"] = static_cast<S32>(prim.getIndexCount());
            mWarningsArray.append(args);
            return false; // Skip this primitive
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
                if (gltf_joint_index_use[vertices[i].joints.x] >= 0
                    && vertices[i].weights.x > 0.f)
                {
                    weight_list.push_back(LLModel::JointWeight(vertices[i].joints.x, vertices[i].weights.x));
                    gltf_joint_index_use[vertices[i].joints.x]++;
                }
                if (gltf_joint_index_use[vertices[i].joints.y] >= 0
                    && vertices[i].weights.y > 0.f)
                {
                    weight_list.push_back(LLModel::JointWeight(vertices[i].joints.y, vertices[i].weights.y));
                    gltf_joint_index_use[vertices[i].joints.y]++;
                }
                if (gltf_joint_index_use[vertices[i].joints.z] >= 0
                    && vertices[i].weights.z > 0.f)
                {
                    weight_list.push_back(LLModel::JointWeight(vertices[i].joints.z, vertices[i].weights.z));
                    gltf_joint_index_use[vertices[i].joints.z]++;
                }
                if (gltf_joint_index_use[vertices[i].joints.w] >= 0
                    && vertices[i].weights.w > 0.f)
                {
                    weight_list.push_back(LLModel::JointWeight(vertices[i].joints.w, vertices[i].weights.w));
                    gltf_joint_index_use[vertices[i].joints.w]++;
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
        mats[materialName] = impMat;

        // Indices handling
        if (faceVertices.size() >= VERTICIES_LIMIT)
        {
            // Will have to remap 32 bit indices into 16 bit indices
            // For the sake of simplicity build vector of 32 bit indices first
            std::vector<U32> indices_32;
            for (U32 i = 0; i < prim.getIndexCount(); i += 3)
            {
                // When processing indices, flip winding order if needed
                if (hasNegativeScale)
                {
                    // Flip winding order for negative scale
                    indices_32.push_back(prim.mIndexArray[i]);
                    indices_32.push_back(prim.mIndexArray[i + 2]); // Swap these two
                    indices_32.push_back(prim.mIndexArray[i + 1]);
                }
                else
                {
                    indices_32.push_back(prim.mIndexArray[i]);
                    indices_32.push_back(prim.mIndexArray[i + 1]);
                    indices_32.push_back(prim.mIndexArray[i + 2]);
                }
            }

            // Generates a vertex remap table with no gaps in the resulting sequence
            std::vector<U32> remap(faceVertices.size());
            size_t vertex_count = meshopt_generateVertexRemap(&remap[0], &indices_32[0], indices_32.size(), &faceVertices[0], faceVertices.size(), sizeof(LLVolumeFace::VertexData));

            // Manually remap vertices
            std::vector<LLVolumeFace::VertexData> optimized_vertices(vertex_count);
            for (size_t i = 0; i < vertex_count; ++i)
            {
                optimized_vertices[i] = faceVertices[remap[i]];
            }

            std::vector<U32> optimized_indices(indices_32.size());
            meshopt_remapIndexBuffer(&optimized_indices[0], &indices_32[0], indices_32.size(), &remap[0]);

            // Sort indices to improve mesh splits (reducing amount of duplicated indices)
            meshopt_optimizeVertexCache(&optimized_indices[0], &optimized_indices[0], indices_32.size(), vertex_count);

            std::vector<U16> indices_16;
            std::vector<S64> vertices_remap;
            vertices_remap.resize(vertex_count, -1);
            S32 created_faces = 0;
            std::vector<LLVolumeFace::VertexData> face_verts;
            min = glm::vec3(FLT_MAX);
            max = glm::vec3(-FLT_MAX);

            for (size_t idx = 0; idx < optimized_indices.size(); idx++)
            {
                size_t vert_index = optimized_indices[idx];
                if (vertices_remap[vert_index] == -1)
                {
                    // First encounter, add it
                    size_t new_vert_idx = face_verts.size();
                    vertices_remap[vert_index] = (S64)new_vert_idx;
                    face_verts.push_back(optimized_vertices[vert_index]);
                    vert_index = new_vert_idx;

                    // Update min/max bounds
                    const LLVector4a& vec = face_verts[new_vert_idx].getPosition();
                    if (new_vert_idx == 0)
                    {
                        min.x = vec[0];
                        min.y = vec[1];
                        min.z = vec[2];
                        max = min;
                    }
                    else
                    {
                        min.x = std::min(min.x, vec[0]);
                        min.y = std::min(min.y, vec[1]);
                        min.z = std::min(min.z, vec[2]);
                        max.x = std::max(max.x, vec[0]);
                        max.y = std::max(max.y, vec[1]);
                        max.z = std::max(max.z, vec[2]);
                    }
                }
                else
                {
                    // already in vector, get position
                    vert_index = (size_t)vertices_remap[vert_index];
                }
                indices_16.push_back((U16)vert_index);

                if (indices_16.size() % 3 == 0 && face_verts.size() >= VERTICIES_LIMIT - 1)
                {
                    LLVolumeFace face;
                    face.fillFromLegacyData(face_verts, indices_16);
                    face.mExtents[0] = LLVector4a(min.x, min.y, min.z, 0);
                    face.mExtents[1] = LLVector4a(max.x, max.y, max.z, 0);
                    pModel->getVolumeFaces().push_back(face);
                    pModel->getMaterialList().push_back(materialName);
                    created_faces++;

                    std::fill(vertices_remap.begin(), vertices_remap.end(), -1);
                    indices_16.clear();
                    face_verts.clear();

                    min = glm::vec3(FLT_MAX);
                    max = glm::vec3(-FLT_MAX);
                }
            }
            if (indices_16.size() > 0 && face_verts.size() > 0)
            {
                LLVolumeFace face;
                face.fillFromLegacyData(face_verts, indices_16);
                face.mExtents[0] = LLVector4a(min.x, min.y, min.z, 0);
                face.mExtents[1] = LLVector4a(max.x, max.y, max.z, 0);
                pModel->getVolumeFaces().push_back(face);
                pModel->getMaterialList().push_back(materialName);
                created_faces++;
            }

            LL_INFOS("GLTF_IMPORT") << "Primitive " << (S32)prim_idx << " from model " << pModel->mLabel
                << " is over vertices limit, it was split into " << created_faces
                << " faces" << LL_ENDL;
            LLSD args;
            args["Message"] = "ModelSplitPrimitive";
            args["MODEL_NAME"] = pModel->mLabel;
            args["FACE_COUNT"] = created_faces;
            mWarningsArray.append(args);
        }
        else
        {
            // can use indices directly
            std::vector<U16> indices;
            for (U32 i = 0; i < prim.getIndexCount(); i += 3)
            {
                // When processing indices, flip winding order if needed
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

            face.fillFromLegacyData(faceVertices, indices);
            face.mExtents[0] = LLVector4a(min.x, min.y, min.z, 0);
            face.mExtents[1] = LLVector4a(max.x, max.y, max.z, 0);

            pModel->getVolumeFaces().push_back(face);
            pModel->getMaterialList().push_back(materialName);
        }
    }

    // Call normalizeVolumeFacesAndWeights to compute proper extents
    pModel->normalizeVolumeFacesAndWeights();

    // Fill joint names, bind matrices and remap weight indices
    if (skinIdx >= 0)
    {
        LL::GLTF::Skin& gltf_skin = mGLTFAsset.mSkins[skinIdx];
        LLMeshSkinInfo& skin_info = pModel->mSkinInfo;
        S32 valid_joints_count = mValidJointsCount[skinIdx];

        S32 replacement_index = 0;
        std::vector<S32> gltfindex_to_joitindex_map;
        size_t jointCnt = gltf_skin.mJoints.size();
        gltfindex_to_joitindex_map.resize(jointCnt, -1);

        if (valid_joints_count > (S32)mMaxJointsPerMesh)
        {
            std::map<std::string, S32> goup_use_count;

            for (const auto& elem : mJointGroups)
            {
                goup_use_count[elem.second.mGroup] = 0;
                goup_use_count[elem.second.mParentGroup] = 0;
            }

            // Assume that 'Torso' group is always in use since that's what everything else is attached to
            goup_use_count["Torso"] = 1;
            // Note that Collisions and Extra groups are all over the place, might want to include them from the start
            // or add individual when parents are added

            // Check which groups are in use
            for (size_t i = 0; i < jointCnt; ++i)
            {
                std::string& joint_name = mJointNames[skinIdx][i];
                if (!joint_name.empty())
                {
                    if (gltf_joint_index_use[i] > 0)
                    {
                        const JointGroups &group = mJointGroups[joint_name];
                        // Joint in use, increment it's groups
                        goup_use_count[group.mGroup]++;
                        goup_use_count[group.mParentGroup]++;
                    }
                }
            }

            // 1. add joints that are in use directly
            for (size_t i = 0; i < jointCnt; ++i)
            {
                // Process joint name and idnex
                S32 joint = gltf_skin.mJoints[i];
                if (gltf_joint_index_use[i] <= 0)
                {
                    // unsupported (-1) joint, drop it
                    // unused (0) joint, drop it
                    continue;
                }

                if (addJointToModelSkin(skin_info, skinIdx, i))
                {
                    gltfindex_to_joitindex_map[i] = replacement_index++;
                }
            }

            // 2. add joints from groups that this model's joints belong to
            // It's perfectly valid to have more joints than is in use
            // Ex: sandals that make your legs digitigrade despite not skining to
            // knees or the like.
            // Todo: sort and add by usecount
            for (size_t i = 0; i < jointCnt; ++i)
            {
                S32 joint = gltf_skin.mJoints[i];
                if (gltf_joint_index_use[i] != 0)
                {
                    // this step needs only joints that have zero uses
                    continue;
                }
                if (skin_info.mInvBindMatrix.size() > mMaxJointsPerMesh)
                {
                    break;
                }
                const std::string& legal_name = mJointNames[skinIdx][i];
                std::string group_name = mJointGroups[legal_name].mGroup;
                if (goup_use_count[group_name] > 0)
                {
                    if (addJointToModelSkin(skin_info, skinIdx, i))
                    {
                        gltfindex_to_joitindex_map[i] = replacement_index++;
                    }
                }
            }
        }
        else
        {
            // Less than 110, just add every valid joint
            for (size_t i = 0; i < jointCnt; ++i)
            {
                // Process joint name and idnex
                S32 joint = gltf_skin.mJoints[i];
                if (gltf_joint_index_use[i] < 0)
                {
                    // unsupported (-1) joint, drop it
                    continue;
                }

                if (addJointToModelSkin(skin_info, skinIdx, i))
                {
                    gltfindex_to_joitindex_map[i] = replacement_index++;
                }
            }
        }

        if (skin_info.mInvBindMatrix.size() > mMaxJointsPerMesh)
        {
            // mMaxJointsPerMesh ususlly is equal to LL_MAX_JOINTS_PER_MESH_OBJECT
            // and is 110.
            LL_WARNS("GLTF_IMPORT") << "Too many jonts in " << pModel->mLabel
                << " Count: " << (S32)skin_info.mInvBindMatrix.size()
                << " Limit:" << (S32)mMaxJointsPerMesh << LL_ENDL;
            LLSD args;
            args["Message"] = "ModelTooManyJoints";
            args["MODEL_NAME"] = pModel->mLabel;
            args["JOINT_COUNT"] = (S32)skin_info.mInvBindMatrix.size();
            args["MAX"] = (S32)mMaxJointsPerMesh;
            mWarningsArray.append(args);
        }

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

void LLGLTFLoader::populateJointsFromSkin(S32 skin_idx)
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
        mAlternateBindMatrices.resize(skin_idx + 1);
        mJointNames.resize(skin_idx + 1);
        mJointUsage.resize(skin_idx + 1);
        mValidJointsCount.resize(skin_idx + 1, 0);
    }

    // fill up joints related data
    joints_data_map_t joints_data;
    joints_name_to_node_map_t names_to_nodes;
    for (S32 i = 0; i < joint_count; i++)
    {
        S32 joint = skin.mJoints[i];
        const LL::GLTF::Node &jointNode = mGLTFAsset.mNodes[joint];
        JointNodeData& data = joints_data[joint];
        data.mNodeIdx = joint;
        data.mJointListIdx = i;
        data.mGltfRestMatrix = buildGltfRestMatrix(joint, skin);
        data.mGltfMatrix = jointNode.mMatrix;
        data.mOverrideMatrix = glm::mat4(1.f);

        if (mJointMap.find(jointNode.mName) != mJointMap.end())
        {
            data.mName = mJointMap[jointNode.mName];
            data.mIsValidViewerJoint = true;
            mValidJointsCount[skin_idx]++;
        }
        else
        {
            data.mName = jointNode.mName;
            data.mIsValidViewerJoint = false;
        }
        names_to_nodes[data.mName] = joint;

        for (S32 child : jointNode.mChildren)
        {
            JointNodeData& child_data = joints_data[child];
            child_data.mParentNodeIdx = joint;
            child_data.mIsParentValidViewerJoint = data.mIsValidViewerJoint;
        }
    }

    // Go over viewer joints and build overrides
    // This is needed because gltf skeleton doesn't necessarily match viewer's skeleton.
    glm::mat4 ident(1.0);
    for (auto &viewer_data : mViewerJointData)
    {
        buildOverrideMatrix(viewer_data, joints_data, names_to_nodes, ident, ident);
    }

    for (S32 i = 0; i < joint_count; i++)
    {
        S32 joint = skin.mJoints[i];
        const LL::GLTF::Node &jointNode = mGLTFAsset.mNodes[joint];
        std::string legal_name(jointNode.mName);

        // Viewer supports a limited set of joints, mark them as legal
        bool legal_joint = false;
        if (mJointMap.find(legal_name) != mJointMap.end())
        {
            legal_name = mJointMap[legal_name];
            legal_joint = true;
            mJointNames[skin_idx].push_back(legal_name);
        }
        else
        {
            mJointNames[skin_idx].emplace_back();
        }
        mJointUsage[skin_idx].push_back(0);

        // Compute bind matrices

        if (!legal_joint)
        {
            // Add placeholder to not break index.
            // Not going to be used by viewer, will be stripped from skin_info.
            LLMatrix4 gltf_transform;
            gltf_transform.setIdentity();
            mInverseBindMatrices[skin_idx].push_back(LLMatrix4a(gltf_transform));
        }
        else if (inverse_count > i)
        {
            // Transalte existing bind matrix to viewer's overriden skeleton
            glm::mat4 original_bind_matrix = glm::inverse(skin.mInverseBindMatricesData[i]);
            glm::mat4 rotated_original = coord_system_rotation * original_bind_matrix;
            glm::mat4 skeleton_transform = computeGltfToViewerSkeletonTransform(joints_data, joint, legal_name);
            glm::mat4 tranlated_original = skeleton_transform * rotated_original;
            glm::mat4 final_inverse_bind_matrix = glm::inverse(tranlated_original);

            LLMatrix4 gltf_transform = LLMatrix4(glm::value_ptr(final_inverse_bind_matrix));
            LL_DEBUGS("GLTF_DEBUG") << "mInvBindMatrix name: " << legal_name << " Translated val: " << gltf_transform << LL_ENDL;
            mInverseBindMatrices[skin_idx].push_back(LLMatrix4a(gltf_transform));
        }
        else
        {
            // If bind matrices aren't present (they are optional in gltf),
            // assume an identy matrix
            // todo: find a model with this, might need to use YZ rotated matrix
            glm::mat4 inv_bind(1.0f);
            glm::mat4 skeleton_transform = computeGltfToViewerSkeletonTransform(joints_data, joint, legal_name);
            inv_bind = glm::inverse(skeleton_transform * inv_bind);

            LLMatrix4 gltf_transform = LLMatrix4(glm::value_ptr(inv_bind));
            LL_DEBUGS("GLTF_DEBUG") << "mInvBindMatrix name: " << legal_name << " Generated val: " << gltf_transform << LL_ENDL;
            mInverseBindMatrices[skin_idx].push_back(LLMatrix4a(gltf_transform));
        }

        // Compute Alternative matrices also known as overrides
        LLMatrix4 original_joint_transform(glm::value_ptr(joints_data[joint].mOverrideMatrix));

        // Viewer seems to care only about translation part,
        // but for parity with collada taking original value
        LLMatrix4 newInverse = LLMatrix4(mInverseBindMatrices[skin_idx].back().getF32ptr());
        newInverse.setTranslation(original_joint_transform.getTranslation());

        LL_DEBUGS("GLTF_DEBUG") << "mAlternateBindMatrix name: " << legal_name << " val: " << newInverse << LL_ENDL;
        mAlternateBindMatrices[skin_idx].push_back(LLMatrix4a(newInverse));

        if (legal_joint)
        {
            // Might be needed for uploader UI to correctly identify overriden joints
            // but going to be incorrect if multiple skins are present
            mJointList[legal_name] = newInverse;
            mJointsFromNode.push_front(legal_name);
        }
    }

    S32 valid_joints = mValidJointsCount[skin_idx];
    if (valid_joints < joint_count)
    {
        LL_INFOS("GLTF_IMPORT") << "Skin " << skin_idx
            << " defines " << joint_count
            << " joints, but only " << valid_joints
            << " were recognized and are compatible." << LL_ENDL;
        LLSD args;
        args["Message"] = "SkinUsupportedJoints";
        args["SKIN_INDEX"] = skin_idx;
        args["JOINT_COUNT"] = joint_count;
        args["LEGAL_COUNT"] = valid_joints;
        mWarningsArray.append(args);
    }
}

void LLGLTFLoader::populateJointGroups()
{
    std::string parent;
    for (auto& viewer_data : mViewerJointData)
    {
        buildJointGroup(viewer_data, parent);
    }
}

void LLGLTFLoader::buildJointGroup(LLJointData& viewer_data, const std::string &parent_group)
{
    JointGroups& jount_group_data = mJointGroups[viewer_data.mName];
    jount_group_data.mGroup = viewer_data.mGroup;
    jount_group_data.mParentGroup = parent_group;

    for (LLJointData& child_data : viewer_data.mChildren)
    {
        buildJointGroup(child_data, viewer_data.mGroup);
    }
}

void LLGLTFLoader::buildOverrideMatrix(LLJointData& viewer_data, joints_data_map_t &gltf_nodes, joints_name_to_node_map_t &names_to_nodes, glm::mat4& parent_rest, glm::mat4& parent_support_rest) const
{
    glm::mat4 rest(1.f);
    joints_name_to_node_map_t::iterator found_node = names_to_nodes.find(viewer_data.mName);
    if (found_node != names_to_nodes.end())
    {
        S32 gltf_node_idx = found_node->second;
        JointNodeData& node = gltf_nodes[gltf_node_idx];
        node.mIsOverrideValid = true;
        node.mViewerRestMatrix = viewer_data.mRestMatrix;

        glm::mat4 gltf_joint_rest_pose = coord_system_rotation * node.mGltfRestMatrix;
        if (mApplyXYRotation)
        {
            gltf_joint_rest_pose = coord_system_rotationxy * gltf_joint_rest_pose;
        }

        glm::mat4 translated_joint;
        // Example:
        // Viewer has pelvis->spine1->spine2->torso.
        // gltf example model has pelvis->torso
        // By doing glm::inverse(transalted_rest_spine2) * gltf_rest_torso
        // We get what torso would have looked like if gltf had a spine2
        if (viewer_data.mIsJoint)
        {
            translated_joint = glm::inverse(parent_rest) * gltf_joint_rest_pose;
        }
        else
        {
            translated_joint = glm::inverse(parent_support_rest) * gltf_joint_rest_pose;
        }

        glm::vec3 translation_override;
        glm::vec3 skew;
        glm::vec3 scale;
        glm::vec4 perspective;
        glm::quat rotation;
        glm::decompose(translated_joint, scale, rotation, translation_override, skew, perspective);

        // Viewer allows overrides, which are base joint with applied translation override.
        // fortunately normal bones use only translation, without rotation or scale
        node.mOverrideMatrix = glm::recompose(glm::vec3(1, 1, 1), glm::identity<glm::quat>(), translation_override, glm::vec3(0, 0, 0), glm::vec4(0, 0, 0, 1));

        glm::mat4 overriden_joint = node.mOverrideMatrix;

        // todo: if gltf bone had rotation or scale, they probably should be saved here
        // then applied to bind matrix
        rest = parent_rest * overriden_joint;
        if (viewer_data.mIsJoint)
        {
            node.mOverrideRestMatrix = rest;
        }
        else
        {
            // This is likely incomplete or even wrong.
            // Viewer Collision bones specify rotation and scale.
            // Importer should apply rotation and scale to this matrix and save as needed
            // then subsctruct them from bind matrix
            // Todo: get models that use collision bones, made by different programs

            overriden_joint = glm::scale(overriden_joint, viewer_data.mScale);
            node.mOverrideRestMatrix = parent_support_rest * overriden_joint;
        }
    }
    else
    {
        // No override for this joint
        rest = parent_rest * viewer_data.mJointMatrix;
    }

    glm::mat4 support_rest(1.f);
    if (viewer_data.mSupport == LLJointData::SUPPORT_BASE)
    {
        support_rest = rest;
    }
    else
    {
        support_rest = parent_support_rest;
    }

    for (LLJointData& child_data : viewer_data.mChildren)
    {
        buildOverrideMatrix(child_data, gltf_nodes, names_to_nodes, rest, support_rest);
    }
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

glm::mat4 LLGLTFLoader::buildGltfRestMatrix(S32 joint_node_index, const joints_data_map_t& joint_data) const
{
    // This is inefficient since we are recalculating some joints multiple times over
    // Todo: cache it?

    if (joint_node_index < 0 || joint_node_index >= static_cast<S32>(mGLTFAsset.mNodes.size()))
    {
        return glm::mat4(1.0f);
    }

    auto& data = joint_data.at(joint_node_index);

    if (data.mParentNodeIdx >=0)
    {
        return buildGltfRestMatrix(data.mParentNodeIdx, joint_data) * data.mGltfMatrix;
    }
    // Should we return armature or stop earlier?
    return data.mGltfMatrix;
}

// This function computes the transformation matrix needed to convert from GLTF skeleton space
// to viewer skeleton space for a specific joint

glm::mat4 LLGLTFLoader::computeGltfToViewerSkeletonTransform(const joints_data_map_t& joints_data_map, S32 gltf_node_index, const std::string& joint_name) const
{
    const JointNodeData& node_data = joints_data_map.at(gltf_node_index);
    if (!node_data.mIsOverrideValid)
    {
        // For now assume they are identical and return an identity (for ease of debuging)
        return glm::mat4(1.0f);
    }

    // Get the GLTF joint's rest pose (in GLTF coordinate system)
    const glm::mat4 &gltf_joint_rest_pose = node_data.mGltfRestMatrix;
    glm::mat4 rest_pose = coord_system_rotation * gltf_joint_rest_pose;

    LL_INFOS("GLTF_DEBUG") << "rest matrix for joint " << joint_name << ": ";

    LLMatrix4 transform(glm::value_ptr(rest_pose));

    LL_CONT << transform << LL_ENDL;

    // Compute transformation from GLTF space to viewer space
    // This assumes both skeletons are in rest pose initially
    return node_data.mOverrideRestMatrix * glm::inverse(rest_pose);
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
        const LL::GLTF::Node &joint_node = mGLTFAsset.mNodes[joint];

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

void LLGLTFLoader::checkGlobalJointUsage()
{
    // Check if some joints remained unused
    for (S32 skin_idx = 0; skin_idx < (S32)mGLTFAsset.mSkins.size(); ++skin_idx)
    {
        const LL::GLTF::Skin& gltf_skin = mGLTFAsset.mSkins[skin_idx];
        S32 joint_count = (S32)gltf_skin.mJoints.size();
        S32 used_joints = 0;
        for (S32 i = 0; i < joint_count; ++i)
        {
            S32 joint = gltf_skin.mJoints[i];
            if (mJointUsage[skin_idx][i] == 0)
            {
                // Joint is unused, log it
                LL_INFOS("GLTF_DEBUG") << "Joint " << mJointNames[skin_idx][i]
                    << " in skin " << skin_idx << " is unused." << LL_ENDL;
            }
            else
            {
                used_joints++;
            }
        }

        S32 valid_joints = mValidJointsCount[skin_idx];
        if (valid_joints > used_joints)
        {
            S32 unsed_joints = valid_joints - used_joints;
            LL_INFOS("GLTF_IMPORT") << "Skin " << skin_idx
                << " declares " << valid_joints
                << " valid joints, of them " << unsed_joints
                << " remained unused" << LL_ENDL;
            LLSD args;
            args["Message"] = "SkinUnusedJoints";
            args["SKIN_INDEX"] = (S32)skin_idx;
            args["JOINT_COUNT"] = valid_joints;
            args["USED_COUNT"] = used_joints;
            mWarningsArray.append(args);
        }
    }
}

std::string LLGLTFLoader::extractTextureToTempFile(S32 textureIndex, const std::string& texture_type)
{
    if (textureIndex < 0 || textureIndex >= mGLTFAsset.mTextures.size())
        return "";

    S32 sourceIndex = mGLTFAsset.mTextures[textureIndex].mSource;
    if (sourceIndex < 0 || sourceIndex >= mGLTFAsset.mImages.size())
        return "";

    LL::GLTF::Image& image = mGLTFAsset.mImages[sourceIndex];

    // Handle URI-based textures
    if (!image.mUri.empty())
    {
        return image.mUri; // Return URI directly
    }

    // Handle embedded textures
    if (image.mBufferView >= 0)
    {
        if (image.mBufferView < mGLTFAsset.mBufferViews.size())
        {
            const LL::GLTF::BufferView& buffer_view = mGLTFAsset.mBufferViews[image.mBufferView];
            if (buffer_view.mBuffer < mGLTFAsset.mBuffers.size())
            {
                const LL::GLTF::Buffer& buffer = mGLTFAsset.mBuffers[buffer_view.mBuffer];

                if (buffer_view.mByteOffset + buffer_view.mByteLength <= buffer.mData.size())
                {
                    // Extract image data
                    const U8* data_ptr = &buffer.mData[buffer_view.mByteOffset];
                    U32 data_size = buffer_view.mByteLength;

                    // Determine the file extension
                    std::string extension = ".png"; // Default
                    if (!image.mMimeType.empty())
                    {
                        if (image.mMimeType == "image/jpeg")
                            extension = ".jpg";
                        else if (image.mMimeType == "image/png")
                            extension = ".png";
                    }
                    else if (data_size >= 4)
                    {
                        if (data_ptr[0] == 0xFF && data_ptr[1] == 0xD8)
                            extension = ".jpg"; // JPEG magic bytes
                        else if (data_ptr[0] == 0x89 && data_ptr[1] == 0x50 && data_ptr[2] == 0x4E && data_ptr[3] == 0x47)
                            extension = ".png"; // PNG magic bytes
                    }

                    // Create a temporary file
                    std::string temp_dir = gDirUtilp->getTempDir();
                    std::string temp_filename = temp_dir + gDirUtilp->getDirDelimiter() +
                                               "gltf_embedded_" + texture_type + "_" + std::to_string(sourceIndex) + extension;

                    // Write the image data to the temporary file
                    std::ofstream temp_file(temp_filename, std::ios::binary);
                    if (temp_file.is_open())
                    {
                        temp_file.write(reinterpret_cast<const char*>(data_ptr), data_size);
                        temp_file.close();

                        LL_INFOS("GLTF_IMPORT") << "Extracted embedded " << texture_type << " texture to: " << temp_filename << LL_ENDL;
                        return temp_filename;
                    }
                    else
                    {
                        LL_WARNS("GLTF_IMPORT") << "Failed to create temporary file for " << texture_type << " texture: " << temp_filename << LL_ENDL;

                        LLSD args;
                        args["Message"] = "FailedToCreateTempFile";
                        args["TEXTURE_INDEX"] = sourceIndex;
                        args["TEXTURE_TYPE"]  = texture_type;
                        args["TEMP_FILE"] = temp_filename;
                        mWarningsArray.append(args);
                    }
                }
            }
        }
    }

    return "";
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

        LL_WARNS("GLTF_IMPORT") << "Model uses unsupported extension: " << ext << LL_ENDL;
    }
}

size_t LLGLTFLoader::getSuffixPosition(const std::string &label)
{
    if ((label.find("_LOD") != -1) || (label.find("_PHYS") != -1))
    {
        return label.rfind('_');
    }
    return -1;
}

std::string LLGLTFLoader::getLodlessLabel(const LL::GLTF::Mesh& mesh)
{
    size_t ext_pos = getSuffixPosition(mesh.mName);
    if (ext_pos != -1)
    {
        return mesh.mName.substr(0, ext_pos);
    }
    return mesh.mName;
}

