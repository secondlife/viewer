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
    U32                                               modelLimit) //,
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

    // Load a tinygltf model fom a file. Assumes that the input filename has already been
    // been sanitized to one of (.gltf , .glb) extensions, so does a simple find to distinguish.
    if (std::string::npos == filename_lc.rfind(".gltf"))
    {  // file is binary
        mGltfLoaded = loader.LoadBinaryFromFile(&mGltfModel, &error_msg, &warn_msg, filename);
    }
    else
    {  // file is ascii
        mGltfLoaded = loader.LoadASCIIFromFile(&mGltfModel, &error_msg, &warn_msg, filename);
    }

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

    return (mMeshesLoaded || mMaterialsLoaded);
}

bool LLGLTFLoader::parseMeshes()
{
    if (!mGltfLoaded) return false;

    // 2022-04 DJH Volume params from dae example. TODO understand PCODE
    LLVolumeParams volume_params;
    volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);

    for (tinygltf::Mesh mesh : mGltfModel.meshes)
    {
        LLModel *pModel = new LLModel(volume_params, 0.f);

        if (populateModelFromMesh(pModel, mesh)         &&
            (LLModel::NO_ERRORS == pModel->getStatus()) &&
            validate_model(pModel))
        {
            mModelList.push_back(pModel);
        }
        else
        {
            setLoadState(ERROR_MODEL + pModel->getStatus());
            delete(pModel);
            return false;
        }
    }
    return true;
}

bool LLGLTFLoader::populateModelFromMesh(LLModel* pModel, const tinygltf::Mesh &mesh)
{
    pModel->mLabel = mesh.name;
    int pos_idx;
    tinygltf::Accessor indices_a, positions_a, normals_a, uv0_a, color0_a;

    auto prims = mesh.primitives;
    for (auto prim : prims)
    {
        if (prim.indices >= 0) indices_a = mGltfModel.accessors[prim.indices];

        pos_idx = (prim.attributes.count("POSITION") > 0) ? prim.attributes.at("POSITION") : -1;
        if (pos_idx >= 0)
        {
            positions_a = mGltfModel.accessors[pos_idx];
            if (TINYGLTF_COMPONENT_TYPE_FLOAT != positions_a.componentType)
                continue;
            auto positions_bv = mGltfModel.bufferViews[positions_a.bufferView];
            auto positions_buf = mGltfModel.buffers[positions_bv.buffer];
            //auto type = positions_vb.
            //if (positions_buf.name
        }

#if 0
        int norm_idx, tan_idx, uv0_idx, uv1_idx, color0_idx, color1_idx;
        norm_idx = (prim.attributes.count("NORMAL") > 0) ? prim.attributes.at("NORMAL") : -1;
        tan_idx = (prim.attributes.count("TANGENT") > 0) ? prim.attributes.at("TANGENT") : -1;
        uv0_idx = (prim.attributes.count("TEXCOORDS_0") > 0) ? prim.attributes.at("TEXCOORDS_0") : -1;
        uv1_idx = (prim.attributes.count("TEXCOORDS_1") > 0) ? prim.attributes.at("TEXCOORDS_1") : -1;
        color0_idx = (prim.attributes.count("COLOR_0") > 0) ? prim.attributes.at("COLOR_0") : -1;
        color1_idx = (prim.attributes.count("COLOR_1") > 0) ? prim.attributes.at("COLOR_1") : -1;
#endif

        if (prim.mode == TINYGLTF_MODE_TRIANGLES)
        {
            //auto pos = mesh.    TODO resume here DJH 2022-04
        }
    }

    //pModel->addFace()
    return false;
}

bool LLGLTFLoader::parseMaterials()
{
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
}

// TODO: convert raw vertex buffers to UUIDs
void LLGLTFLoader::uploadMeshes()
{
    llassert(0);
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
