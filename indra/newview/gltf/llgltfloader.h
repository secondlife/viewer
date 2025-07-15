/**
 * @file LLGLTFLoader.h
 * @brief LLGLTFLoader class definition
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

#ifndef LL_LLGLTFLoader_H
#define LL_LLGLTFLoader_H

#include "tinygltf/tiny_gltf.h"

#include "asset.h"

#include "llglheaders.h"
#include "lljointdata.h"
#include "llmodelloader.h"

class LLGLTFLoader : public LLModelLoader
{
  public:
    typedef std::map<std::string, LLImportMaterial> material_map;
    typedef std::map<std::string, std::string> joint_viewer_parent_map_t;
    typedef std::map<std::string, glm::mat4> joint_viewer_rest_map_t;
    typedef std::map<S32, glm::mat4> joint_node_mat4_map_t;

    struct JointNodeData
    {
        JointNodeData()
            : mJointListIdx(-1)
            , mNodeIdx(-1)
            , mParentNodeIdx(-1)
            , mIsValidViewerJoint(false)
            , mIsParentValidViewerJoint(false)
            , mIsOverrideValid(false)
        {

        }
        S32 mJointListIdx;
        S32 mNodeIdx;
        S32 mParentNodeIdx;
        glm::mat4 mGltfRestMatrix;
        glm::mat4 mViewerRestMatrix;
        glm::mat4 mOverrideRestMatrix;
        glm::mat4 mGltfMatrix;
        glm::mat4 mOverrideMatrix;
        std::string mName;
        bool mIsValidViewerJoint;
        bool mIsParentValidViewerJoint;
        bool mIsOverrideValid;
    };
    typedef std::map <S32, JointNodeData> joints_data_map_t;
    typedef std::map <std::string, S32> joints_name_to_node_map_t;

    class LLGLTFImportMaterial : public LLImportMaterial
    {
    public:
        std::string name;
        LLGLTFImportMaterial() = default;
        LLGLTFImportMaterial(const LLImportMaterial& mat, const std::string& n) : LLImportMaterial(mat), name(n) {}
    };

    LLGLTFLoader(std::string filename,
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
                    std::vector<LLJointData>                          viewer_skeleton); //,
                    //bool                                            preprocess );
    virtual ~LLGLTFLoader();

    virtual bool OpenFile(const std::string &filename);

    struct GLTFVertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv0;
        glm::u16vec4 joints;
        glm::vec4 weights;
    };

protected:
    LL::GLTF::Asset mGLTFAsset;
    tinygltf::Model mGltfModel;
    bool            mGltfLoaded = false;
    bool            mApplyXYRotation = false;

    // GLTF isn't aware of viewer's skeleton and uses it's own,
    // so need to take viewer's joints and use them to
    // recalculate iverse bind matrices
    std::vector<LLJointData>             mViewerJointData;

    // vector of vectors because of a posibility of having more than one skin
    typedef std::vector<LLMeshSkinInfo::matrix_list_t> bind_matrices_t;
    typedef std::vector<std::vector<std::string> > joint_names_t;
    bind_matrices_t                     mInverseBindMatrices;
    bind_matrices_t                     mAlternateBindMatrices;
    joint_names_t                       mJointNames; // empty string when no legal name for a given idx
    std::vector<std::vector<S32>>       mJointUsage; // detect and warn about unsed joints

    // what group a joint belongs to.
    // For purpose of stripping unused groups when joints are over limit.
    struct JointGroups
    {
        std::string mGroup;
        std::string mParentGroup;
    };
    typedef std::map<std::string, JointGroups, std::less<> > joint_to_group_map_t;
    joint_to_group_map_t mJointGroups;

    // per skin joint count, needs to be tracked for the sake of limits check.
    std::vector<S32>                    mValidJointsCount;

    // Cached material information
    typedef std::map<S32, LLGLTFImportMaterial> MaterialCache;
    MaterialCache mMaterialCache;

private:
    bool parseMeshes();
    void computeCombinedNodeTransform(const LL::GLTF::Asset& asset, S32 node_index, glm::mat4& combined_transform) const;
    void processNodeHierarchy(S32 node_idx, std::map<std::string, S32>& mesh_name_counts, U32 submodel_limit, const LLVolumeParams& volume_params);
    bool addJointToModelSkin(LLMeshSkinInfo& skin_info, S32 gltf_skin_idx, size_t gltf_joint_idx);
    LLGLTFImportMaterial processMaterial(S32 material_index, S32 fallback_index);
    std::string processTexture(S32 texture_index, const std::string& texture_type, const std::string& material_name);
    bool validateTextureIndex(S32 texture_index, S32& source_index);
    std::string generateMaterialName(S32 material_index, S32 fallback_index = -1);
    bool populateModelFromMesh(LLModel* pModel, const std::string& base_name, const LL::GLTF::Mesh &mesh, const LL::GLTF::Node &node, material_map& mats);
    void populateJointsFromSkin(S32 skin_idx);
    void populateJointGroups();
    void addModelToScene(LLModel* pModel, const std::string& model_name, U32 submodel_limit, const LLMatrix4& transformation, const LLVolumeParams& volume_params, const material_map& mats);
    void buildJointGroup(LLJointData& viewer_data, const std::string& parent_group);
    void buildOverrideMatrix(LLJointData& data, joints_data_map_t &gltf_nodes, joints_name_to_node_map_t &names_to_nodes, glm::mat4& parent_rest, glm::mat4& support_rest) const;
    glm::mat4 buildGltfRestMatrix(S32 joint_node_index, const LL::GLTF::Skin& gltf_skin) const;
    glm::mat4 buildGltfRestMatrix(S32 joint_node_index, const joints_data_map_t& joint_data) const;
    glm::mat4 computeGltfToViewerSkeletonTransform(const joints_data_map_t& joints_data_map, S32 gltf_node_index, const std::string& joint_name) const;
    bool checkForXYrotation(const LL::GLTF::Skin& gltf_skin, S32 joint_idx, S32 bind_indx);
    void checkForXYrotation(const LL::GLTF::Skin& gltf_skin);
    void checkGlobalJointUsage();

    std::string extractTextureToTempFile(S32 textureIndex, const std::string& texture_type);

    void notifyUnsupportedExtension(bool unsupported);

    static size_t getSuffixPosition(const std::string& label);
    static std::string getLodlessLabel(const LL::GLTF::Mesh& mesh);

    //    bool mPreprocessGLTF;

    /*  Below inherited from dae loader - unknown if/how useful here

    void processElement(gltfElement *element, bool &badElement, GLTF *gltf);
    void processGltfModel(LLModel *model, GLTF *gltf, gltfElement *pRoot, gltfMesh *mesh, gltfSkin *skin);

    material_map     getMaterials(LLModel *model, gltfInstance_geometry *instance_geo, GLTF *gltf);
    LLImportMaterial profileToMaterial(gltfProfile_COMMON *material, GLTF *gltf);
    LLColor4         getGltfColor(gltfElement *element);

    gltfElement *getChildFromElement(gltfElement *pElement, std::string const &name);

    bool isNodeAJoint(gltfNode *pNode);
    void processJointNode(gltfNode *pNode, std::map<std::string, LLMatrix4> &jointTransforms);
    void extractTranslation(gltfTranslate *pTranslate, LLMatrix4 &transform);
    void extractTranslationViaElement(gltfElement *pTranslateElement, LLMatrix4 &transform);
    void extractTranslationViaSID(gltfElement *pElement, LLMatrix4 &transform);
    void buildJointToNodeMappingFromScene(gltfElement *pRoot);
    void processJointToNodeMapping(gltfNode *pNode);
    void processChildJoints(gltfNode *pParentNode);

    bool verifyCount(int expected, int result);

    // Verify that a controller matches vertex counts
    bool verifyController(gltfController *pController);

    static bool addVolumeFacesFromGltfMesh(LLModel *model, gltfMesh *mesh, LLSD &log_msg);
    static bool createVolumeFacesFromGltfMesh(LLModel *model, gltfMesh *mesh);

    static LLModel *loadModelFromGltfMesh(gltfMesh *mesh);

    // Loads a mesh breaking it into one or more models as necessary
    // to get around volume face limitations while retaining >8 materials
    //
    bool loadModelsFromGltfMesh(gltfMesh *mesh, std::vector<LLModel *> &models_out, U32 submodel_limit);

    static std::string preprocessGLTF(std::string filename);
    */

};
#endif  // LL_LLGLTFLLOADER_H
