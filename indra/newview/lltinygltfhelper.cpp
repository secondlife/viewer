/**
 * @file   lltinygltfhelper.cpp
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

#include "llviewerprecompiledheaders.h"

#include "lltinygltfhelper.h"

#include "llimage.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"

static void strip_alpha_channel(LLPointer<LLImageRaw>& img)
{
    if (img->getComponents() == 4)
    {
        LLImageRaw* tmp = new LLImageRaw(img->getWidth(), img->getHeight(), 3);
        tmp->copyUnscaled4onto3(img);
        img = tmp;
    }
}

// copy red channel from src_img to dst_img
// PRECONDITIONS:
// dst_img must be 3 component
// src_img and dst_image must have the same dimensions
static void copy_red_channel(const LLPointer<LLImageRaw>& src_img, LLPointer<LLImageRaw>& dst_img)
{
    llassert(src_img->getWidth() == dst_img->getWidth() && src_img->getHeight() == dst_img->getHeight());
    llassert(dst_img->getComponents() == 3);

    U32 pixel_count = dst_img->getWidth() * dst_img->getHeight();
    const U8* src = src_img->getData();
    U8* dst = dst_img->getData();
    S8 src_components = src_img->getComponents();

    for (U32 i = 0; i < pixel_count; ++i)
    {
        dst[i * 3] = src[i * src_components];
    }
}

void LLTinyGLTFHelper::initFetchedTextures(tinygltf::Material& material,
    LLPointer<LLImageRaw>& base_color_img,
    LLPointer<LLImageRaw>& normal_img,
    LLPointer<LLImageRaw>& mr_img,
    LLPointer<LLImageRaw>& emissive_img,
    LLPointer<LLImageRaw>& occlusion_img,
    LLPointer<LLViewerFetchedTexture>& base_color_tex,
    LLPointer<LLViewerFetchedTexture>& normal_tex,
    LLPointer<LLViewerFetchedTexture>& mr_tex,
    LLPointer<LLViewerFetchedTexture>& emissive_tex)
{
    if (base_color_img)
    {
        base_color_tex = LLViewerTextureManager::getFetchedTexture(base_color_img, FTType::FTT_LOCAL_FILE, true);
    }

    if (normal_img)
    {
        strip_alpha_channel(normal_img);
        normal_tex = LLViewerTextureManager::getFetchedTexture(normal_img, FTType::FTT_LOCAL_FILE, true);
    }

    if (mr_img)
    {
        strip_alpha_channel(mr_img);

        if (occlusion_img)
        {
            if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != material.occlusionTexture.index)
            {
                // occlusion is a distinct texture from pbrMetallicRoughness
                // pack into mr red channel
                int occlusion_idx = material.occlusionTexture.index;
                int mr_idx = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
                if (occlusion_idx != mr_idx)
                {
                    LLImageDataLock lockIn(occlusion_img);
                    LLImageDataLock lockOut(mr_img);
                    //scale occlusion image to match resolution of mr image
                    occlusion_img->scale(mr_img->getWidth(), mr_img->getHeight());

                    copy_red_channel(occlusion_img, mr_img);
                }
            }
        }
        else if (material.occlusionTexture.index == -1)
        {
            // no occlusion, make sure red channel of ORM is all 255
            occlusion_img = new LLImageRaw(mr_img->getWidth(), mr_img->getHeight(), 3);
            occlusion_img->clear(255, 255, 255);
            copy_red_channel(occlusion_img, mr_img);
        }
    }
    else if (occlusion_img)
    {
        LLImageDataSharedLock lock(occlusion_img);
        //no mr but occlusion exists, make a white mr_img and copy occlusion red channel over
        mr_img = new LLImageRaw(occlusion_img->getWidth(), occlusion_img->getHeight(), 3);
        mr_img->clear(255, 255, 255);
        copy_red_channel(occlusion_img, mr_img);
    }

    if (mr_img)
    {
        mr_tex = LLViewerTextureManager::getFetchedTexture(mr_img, FTType::FTT_LOCAL_FILE, true);
    }

    if (emissive_img)
    {
        strip_alpha_channel(emissive_img);
        emissive_tex = LLViewerTextureManager::getFetchedTexture(emissive_img, FTType::FTT_LOCAL_FILE, true);
    }
}

LLColor4 LLTinyGLTFHelper::getColor(const std::vector<double>& in)
{
    LLColor4 out;
    for (S32 i = 0; i < llmin((S32)in.size(), 4); ++i)
    {
        out.mV[i] = in[i];
    }

    return out;
}

const tinygltf::Image * LLTinyGLTFHelper::getImageFromTextureIndex(const tinygltf::Model & model, S32 texture_index)
{
    if (texture_index >= 0)
    {
        S32 source_idx = model.textures[texture_index].source;
        if (source_idx >= 0)
        {
            return &(model.images[source_idx]);
        }
    }

    return nullptr;
}

LLImageRaw * LLTinyGLTFHelper::getTexture(const std::string & folder, const tinygltf::Model & model, S32 texture_index, std::string & name, bool flip)
{
    const tinygltf::Image* image = getImageFromTextureIndex(model, texture_index);
    LLImageRaw* rawImage = nullptr;

    if (image != nullptr &&
        image->bits == 8 &&
        !image->image.empty() &&
        image->component <= 4)
    {
        name = image->name;
        rawImage = new LLImageRaw(&image->image[0], image->width, image->height, image->component);
        if (flip)
        {
            rawImage->verticalFlip();
        }
        rawImage->optimizeAwayAlpha();
    }

    return rawImage;
}

LLImageRaw * LLTinyGLTFHelper::getTexture(const std::string & folder, const tinygltf::Model & model, S32 texture_index, bool flip)
{
    const tinygltf::Image* image = getImageFromTextureIndex(model, texture_index);
    LLImageRaw* rawImage = nullptr;

    if (image != nullptr &&
        image->bits == 8 &&
        !image->image.empty() &&
        image->component <= 4)
    {
        rawImage = new LLImageRaw(&image->image[0], image->width, image->height, image->component);
        if (flip)
        {
            rawImage->verticalFlip();
        }
        rawImage->optimizeAwayAlpha();
    }

    return rawImage;
}

bool LLTinyGLTFHelper::loadModel(const std::string& filename, tinygltf::Model& model_in)
{
    std::string exten = gDirUtilp->getExtension(filename);

    if (exten == "gltf" || exten == "glb")
    {
        tinygltf::TinyGLTF loader;
        std::string        error_msg;
        std::string        warn_msg;

        std::string filename_lc = filename;
        LLStringUtil::toLower(filename_lc);

        // Load a tinygltf model fom a file. Assumes that the input filename has already been
        // been sanitized to one of (.gltf , .glb) extensions, so does a simple find to distinguish.
        bool decode_successful = false;
        if (std::string::npos == filename_lc.rfind(".gltf"))
        {  // file is binary
            decode_successful = loader.LoadBinaryFromFile(&model_in, &error_msg, &warn_msg, filename_lc);
        }
        else
        {  // file is ascii
            decode_successful = loader.LoadASCIIFromFile(&model_in, &error_msg, &warn_msg, filename_lc);
        }

        if (!decode_successful)
        {
            LL_WARNS("GLTF") << "Cannot load, error: Failed to decode" << error_msg
                << ", warning:" << warn_msg
                << " file: " << filename
                << LL_ENDL;
            return false;
        }

        if (model_in.materials.empty())
        {
            // materials are missing
            LL_WARNS("GLTF") << "Cannot load. File has no materials " << filename << LL_ENDL;
            return false;
        }

        return true;
    }


    return false;
}

bool LLTinyGLTFHelper::saveModel(const std::string& filename, tinygltf::Model& model_in)
{
    std::string exten = gDirUtilp->getExtension(filename);

    bool success = false;

    if (exten == "gltf" || exten == "glb")
    {
        tinygltf::TinyGLTF writer;

        std::string filename_lc = filename;
        LLStringUtil::toLower(filename_lc);


        bool embed_images = false;
        bool embed_buffers = false;
        bool pretty_print = true;
        bool write_binary = false;


        if (std::string::npos == filename_lc.rfind(".gltf"))
        {  // file is binary
            embed_images = embed_buffers = write_binary = true;
        }

        success = writer.WriteGltfSceneToFile(&model_in, filename, embed_images, embed_buffers, pretty_print, write_binary);

        if (!success)
        {
            LL_WARNS("GLTF") << "Failed to save" << LL_ENDL;
            return false;
        }
    }

    return success;
}

bool LLTinyGLTFHelper::getMaterialFromModel(
    const std::string& filename,
    const tinygltf::Model& model_in,
    S32 mat_index,
    LLFetchedGLTFMaterial* material,
    std::string& material_name,
    bool flip)
{
    llassert(material);

    if (model_in.materials.size() <= mat_index)
    {
        // materials are missing
        LL_WARNS("GLTF") << "Cannot load Material, Material " << mat_index << " is missing, " << filename << LL_ENDL;
        return false;
    }

    material->setFromModel(model_in, mat_index);

    std::string folder = gDirUtilp->getDirName(filename);
    tinygltf::Material material_in = model_in.materials[mat_index];

    material_name = material_in.name;

    // get base color texture
    LLPointer<LLImageRaw> base_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.pbrMetallicRoughness.baseColorTexture.index, flip);
    // get normal map
    LLPointer<LLImageRaw> normal_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.normalTexture.index, flip);
    // get metallic-roughness texture
    LLPointer<LLImageRaw> mr_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.pbrMetallicRoughness.metallicRoughnessTexture.index, flip);
    // get emissive texture
    LLPointer<LLImageRaw> emissive_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.emissiveTexture.index, flip);
    // get occlusion map if needed
    LLPointer<LLImageRaw> occlusion_img;
    if (material_in.occlusionTexture.index != material_in.pbrMetallicRoughness.metallicRoughnessTexture.index)
    {
        occlusion_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.occlusionTexture.index, flip);
    }

    LLPointer<LLViewerFetchedTexture> base_color_tex;
    LLPointer<LLViewerFetchedTexture> normal_tex;
    LLPointer<LLViewerFetchedTexture> mr_tex;
    LLPointer<LLViewerFetchedTexture> emissive_tex;

    // todo: pass it into local bitmaps?
    LLTinyGLTFHelper::initFetchedTextures(material_in,
        base_img, normal_img, mr_img, emissive_img, occlusion_img,
        base_color_tex, normal_tex, mr_tex, emissive_tex);

    if (base_color_tex)
    {
        base_color_tex->addTextureStats(64.f * 64.f, true);
        material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR] = base_color_tex->getID();
        material->mBaseColorTexture = base_color_tex;
    }
    else
    {
        material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR] = LLUUID::null;
        material->mBaseColorTexture = nullptr;
    }

    if (normal_tex)
    {
        normal_tex->addTextureStats(64.f * 64.f, true);
        material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL] = normal_tex->getID();
        material->mNormalTexture = normal_tex;
    }
    else
    {
        material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL] = LLUUID::null;
        material->mNormalTexture = nullptr;
    }

    if (mr_tex)
    {
        mr_tex->addTextureStats(64.f * 64.f, true);
        material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS] = mr_tex->getID();
        material->mMetallicRoughnessTexture = mr_tex;
    }
    else
    {
        material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS] = LLUUID::null;
        material->mMetallicRoughnessTexture = nullptr;
    }

    if (emissive_tex)
    {
        emissive_tex->addTextureStats(64.f * 64.f, true);
        material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE] = emissive_tex->getID();
        material->mEmissiveTexture = emissive_tex;
    }
    else
    {
        material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE] = LLUUID::null;
        material->mEmissiveTexture = nullptr;
    }

    return true;
}
