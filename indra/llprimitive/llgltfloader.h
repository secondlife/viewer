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

#include "llglheaders.h"
#include "llmodelloader.h"

// gltf_* structs are temporary, used to organize the subset of data that eventually goes into the material LLSD

class gltf_sampler
{
public:
    // Uses GL enums
    S32 minFilter;      // GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR or GL_LINEAR_MIPMAP_LINEAR
    S32 magFilter;      // GL_NEAREST or GL_LINEAR
    S32 wrapS;          // GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT or GL_REPEAT
    S32 wrapT;          // GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT or GL_REPEAT
    //S32 wrapR;        // Found in some sample files, but not part of glTF 2.0 spec. Ignored.
    std::string name;   // optional, currently unused
    // extensions and extras are sampler optional fields that we don't support - at least initially
};

class gltf_image
{
public:// Note that glTF images are defined with row 0 at the top (opposite of OpenGL)
    U8* data;               // ptr to decoded image data
    U32 size;               // in bytes, regardless of channel width
    U32 width;
    U32 height;
    U32 numChannels;        // range 1..4
    U32 bytesPerChannel;    // converted from gltf "bits", expects only 8, 16 or 32 as input
    U32 pixelType;          // one of (TINYGLTF_COMPONENT_TYPE)_UNSIGNED_BYTE, _UNSIGNED_SHORT, _UNSIGNED_INT, or _FLOAT
};

class gltf_texture
{
public:
    U32 imageIdx;
    U32 samplerIdx;
    LLUUID imageUuid = LLUUID::null;
};

class gltf_render_material
{
public:
    std::string name;

    // scalar values
    LLColor4    baseColor;      // linear encoding. Multiplied with vertex color, if present.
    double      metalness;
    double      roughness;
    double      normalScale;    // scale applies only to X,Y components of normal
    double      occlusionScale; // strength multiplier for occlusion
    LLColor4    emissiveColor;  // emissive mulitiplier, assumed linear encoding (spec 2.0 is silent)
    std::string alphaMode;      // "OPAQUE", "MASK" or "BLEND"
    double      alphaMask;      // alpha cut-off

    // textures
    U32 baseColorTexIdx;    // always sRGB encoded
    U32 metalRoughTexIdx;   // always linear, roughness in G channel, metalness in B channel
    U32 normalTexIdx;       // linear, valid range R[0-1], G[0-1], B[0.5-1]. Normal = texel * 2 - vec3(1.0)
    U32 occlusionTexIdx;    // linear, occlusion in R channel, 0 meaning fully occluded, 1 meaning not occluded
    U32 emissiveTexIdx;     // always stored as sRGB, in nits (candela / meter^2)

    // texture coordinates
    U32 baseColorTexCoords;
    U32 metalRoughTexCoords;
    U32 normalTexCoords;
    U32 occlusionTexCoords;
    U32 emissiveTexCoords;

    // TODO: Add traditional (diffuse, normal, specular) UUIDs here, or add this struct to LL_TextureEntry??

    bool        hasPBR;
    bool        hasBaseTex, hasMRTex, hasNormalTex, hasOcclusionTex, hasEmissiveTex;

    // This field is populated after upload
    LLUUID      material_uuid = LLUUID::null;

};

class gltf_mesh
{
public:
    std::string name;

    // TODO add mesh import DJH 2022-04

};

class LLGLTFLoader : public LLModelLoader
{
  public:
    typedef std::map<std::string, LLImportMaterial> material_map;

    LLGLTFLoader(std::string filename,
                    S32                                             lod,
                    LLModelLoader::load_callback_t                  load_cb,
                    LLModelLoader::joint_lookup_func_t              joint_lookup_func,
                    LLModelLoader::texture_load_func_t              texture_load_func,
                    LLModelLoader::state_callback_t                 state_cb,
                    void *                                          opaque_userdata,
                    JointTransformMap &                             jointTransformMap,
                    JointNameSet &                                  jointsFromNodes,
                    std::map<std::string, std::string,std::less<>> &jointAliasMap,
                    U32                                             maxJointsPerMesh,
                    U32                                             modelLimit); //,
                    //bool                                          preprocess );
    virtual ~LLGLTFLoader();

    virtual bool OpenFile(const std::string &filename);

protected:
    tinygltf::Model mGltfModel;
    bool            mGltfLoaded;
    bool            mMeshesLoaded;
    bool            mMaterialsLoaded;

    std::vector<gltf_mesh>              mMeshes;
    std::vector<gltf_render_material>   mMaterials;

    std::vector<gltf_texture>           mTextures;
    std::vector<gltf_image>             mImages;
    std::vector<gltf_sampler>           mSamplers;

private:
    bool parseMeshes();
    void uploadMeshes();
    bool parseMaterials();
    void uploadMaterials();
    bool populateModelFromMesh(LLModel* pModel, const tinygltf::Mesh &mesh);
    LLUUID imageBufferToTextureUUID(const gltf_texture& tex);

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

    static std::string getElementLabel(gltfElement *element);
    static size_t      getSuffixPosition(std::string label);
    static std::string getLodlessLabel(gltfElement *element);

    static std::string preprocessGLTF(std::string filename);
    */

};
#endif  // LL_LLGLTFLLOADER_H
