/**
 * @file   llgltfhelper.cpp
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

#include "llgltfhelper.h"

#include "llimage.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "lldir.h"

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

// Decode image data from an LL::GLTF::Image's buffer view
static LLImageRaw* decodeImage(const LL::GLTF::Asset& asset, const LL::GLTF::Image& image, bool flip)
{
    const U8* data = nullptr;
    U32 data_size = 0;
    std::string mime_type = image.mMimeType;

    std::string buffer_data; // holds file data if needed

    if (image.mBufferView != LL::GLTF::INVALID_INDEX &&
        (size_t)image.mBufferView < asset.mBufferViews.size())
    {
        const auto& bv = asset.mBufferViews[image.mBufferView];
        if (bv.mBuffer != LL::GLTF::INVALID_INDEX &&
            (size_t)bv.mBuffer < asset.mBuffers.size())
        {
            const auto& buf = asset.mBuffers[bv.mBuffer];
            if ((size_t)(bv.mByteOffset + bv.mByteLength) <= buf.mData.size())
            {
                data = (const U8*)buf.mData.data() + bv.mByteOffset;
                data_size = (U32)bv.mByteLength;
            }
        }
    }
    else if (!image.mUri.empty())
    {
        // External file URI - load from disk relative to asset
        std::string folder = gDirUtilp->getDirName(asset.mFilename);
        std::string filepath = folder + gDirUtilp->getDirDelimiter() + image.mUri;

        if (!gDirUtilp->fileExists(filepath))
        {
            // URI might be escaped, unescape
            filepath = folder + gDirUtilp->getDirDelimiter() + LLURI::unescape(image.mUri);
        }

        llifstream ifs(filepath, std::ios::binary);
        if (ifs.good())
        {
            buffer_data.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            data = (const U8*)buffer_data.data();
            data_size = (U32)buffer_data.size();

            // Infer mime type from extension if not set
            if (mime_type.empty())
            {
                std::string ext = gDirUtilp->getExtension(image.mUri);
                LLStringUtil::toLower(ext);
                if (ext == "png") mime_type = "image/png";
                else if (ext == "jpg" || ext == "jpeg") mime_type = "image/jpeg";
                else if (ext == "bmp") mime_type = "image/bmp";
            }
        }
    }

    if (!data || data_size == 0)
    {
        return nullptr;
    }

    // Use LLImageFormatted to decode from memory with the given mimetype
    LLPointer<LLImageFormatted> formatted = LLImageFormatted::loadFromMemory(data, data_size, mime_type);
    if (!formatted)
    {
        return nullptr;
    }

    LLImageRaw* raw = new LLImageRaw();
    if (!formatted->decode(raw, 0.0f))
    {
        // Use LLPointer to clean up via refcounted destructor
        LLPointer<LLImageRaw> guard(raw);
        return nullptr;
    }

    if (flip)
    {
        raw->verticalFlip();
    }
    raw->optimizeAwayAlpha();

    return raw;
}

void LLGLTFHelper::initFetchedTextures(const LL::GLTF::Material& material,
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
            if (material.mPbrMetallicRoughness.mMetallicRoughnessTexture.mIndex != material.mOcclusionTexture.mIndex)
            {
                // occlusion is a distinct texture from pbrMetallicRoughness
                // pack into mr red channel
                S32 occlusion_idx = material.mOcclusionTexture.mIndex;
                S32 mr_idx = material.mPbrMetallicRoughness.mMetallicRoughnessTexture.mIndex;
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
        else if (material.mOcclusionTexture.mIndex == LL::GLTF::INVALID_INDEX)
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

LLColor4 LLGLTFHelper::getColor(const std::vector<double>& in)
{
    LLColor4 out;
    for (S32 i = 0; i < llmin((S32)in.size(), 4); ++i)
    {
        out.mV[i] = (F32)in[i];
    }

    return out;
}

LLImageRaw* LLGLTFHelper::getTexture(const std::string& folder, const LL::GLTF::Asset& asset, S32 texture_index, std::string& name, bool flip)
{
    if (texture_index < 0 || (size_t)texture_index >= asset.mTextures.size())
    {
        return nullptr;
    }

    S32 source_idx = asset.mTextures[texture_index].mSource;
    if (source_idx < 0 || (size_t)source_idx >= asset.mImages.size())
    {
        return nullptr;
    }

    const auto& image = asset.mImages[source_idx];
    name = image.mName;

    return decodeImage(asset, image, flip);
}

LLImageRaw* LLGLTFHelper::getTexture(const std::string& folder, const LL::GLTF::Asset& asset, S32 texture_index, bool flip)
{
    std::string name;
    return getTexture(folder, asset, texture_index, name, flip);
}

bool LLGLTFHelper::loadModel(const std::string& filename, LL::GLTF::Asset& asset_out)
{
    std::string exten = gDirUtilp->getExtension(filename);

    if (exten == "gltf" || exten == "glb")
    {
        try
        {
            if (!asset_out.load(filename, false))
            {
                LL_WARNS("GLTF") << "Cannot load, error: Failed to load " << filename << LL_ENDL;
                return false;
            }
        }
        catch (const std::exception& e)
        {
            LL_WARNS("GLTF") << "Cannot load, exception: " << e.what() << " file: " << filename << LL_ENDL;
            return false;
        }

        if (asset_out.mMaterials.empty())
        {
            // materials are missing
            LL_WARNS("GLTF") << "Cannot load. File has no materials " << filename << LL_ENDL;
            return false;
        }

        return true;
    }

    return false;
}

bool LLGLTFHelper::getMaterialFromModel(
    const std::string& filename,
    const LL::GLTF::Asset& asset,
    S32 mat_index,
    LLFetchedGLTFMaterial* material,
    std::string& material_name,
    bool flip)
{
    llassert(material);

    if ((size_t)mat_index >= asset.mMaterials.size())
    {
        // materials are missing
        LL_WARNS("GLTF") << "Cannot load Material, Material " << mat_index << " is missing, " << filename << LL_ENDL;
        return false;
    }

    const auto& mat_in = asset.mMaterials[mat_index];
    material_name = mat_in.mName;

    // Serialize material to JSON, then load via fromJSON to populate LLGLTFMaterial fields
    {
        boost::json::object doc;
        boost::json::object asset_obj;
        asset_obj["version"] = "2.0";
        doc["asset"] = asset_obj;

        // Serialize the material
        boost::json::object mat_json;
        mat_in.serialize(mat_json);

        // Serialize referenced textures and images
        boost::json::array textures_arr;
        boost::json::array images_arr;

        auto serializeTexIndex = [&](S32 tex_index) {
            if (tex_index >= 0 && (size_t)tex_index < asset.mTextures.size())
            {
                const auto& tex = asset.mTextures[tex_index];
                // Ensure we have entries up to the needed index
                while ((S32)textures_arr.size() <= tex_index)
                {
                    textures_arr.push_back(boost::json::object());
                }
                boost::json::object tex_obj;
                tex_obj["source"] = tex.mSource;
                textures_arr[(size_t)tex_index] = tex_obj;

                if (tex.mSource >= 0 && (size_t)tex.mSource < asset.mImages.size())
                {
                    while ((S32)images_arr.size() <= tex.mSource)
                    {
                        images_arr.push_back(boost::json::object());
                    }
                    // For material system, URIs are UUIDs - but from file they won't be.
                    // We'll use a placeholder and rely on texture loading below.
                    boost::json::object img_obj;
                    img_obj["uri"] = LLUUID::null.asString();
                    images_arr[(size_t)tex.mSource] = img_obj;
                }
            }
        };

        serializeTexIndex(mat_in.mPbrMetallicRoughness.mBaseColorTexture.mIndex);
        serializeTexIndex(mat_in.mNormalTexture.mIndex);
        serializeTexIndex(mat_in.mPbrMetallicRoughness.mMetallicRoughnessTexture.mIndex);
        serializeTexIndex(mat_in.mEmissiveTexture.mIndex);
        serializeTexIndex(mat_in.mOcclusionTexture.mIndex);

        doc["materials"] = boost::json::array({ mat_json });
        if (!textures_arr.empty()) doc["textures"] = textures_arr;
        if (!images_arr.empty()) doc["images"] = images_arr;

        std::string json_str = boost::json::serialize(doc);
        std::string warn_msg, error_msg;
        material->fromJSON(json_str, warn_msg, error_msg);
    }

    std::string folder = gDirUtilp->getDirName(filename);

    // get base color texture
    LLPointer<LLImageRaw> base_img = LLGLTFHelper::getTexture(folder, asset, mat_in.mPbrMetallicRoughness.mBaseColorTexture.mIndex, flip);
    // get normal map
    LLPointer<LLImageRaw> normal_img = LLGLTFHelper::getTexture(folder, asset, mat_in.mNormalTexture.mIndex, flip);
    // get metallic-roughness texture
    LLPointer<LLImageRaw> mr_img = LLGLTFHelper::getTexture(folder, asset, mat_in.mPbrMetallicRoughness.mMetallicRoughnessTexture.mIndex, flip);
    // get emissive texture
    LLPointer<LLImageRaw> emissive_img = LLGLTFHelper::getTexture(folder, asset, mat_in.mEmissiveTexture.mIndex, flip);
    // get occlusion map if needed
    LLPointer<LLImageRaw> occlusion_img;
    if (mat_in.mOcclusionTexture.mIndex != mat_in.mPbrMetallicRoughness.mMetallicRoughnessTexture.mIndex)
    {
        occlusion_img = LLGLTFHelper::getTexture(folder, asset, mat_in.mOcclusionTexture.mIndex, flip);
    }

    LLPointer<LLViewerFetchedTexture> base_color_tex;
    LLPointer<LLViewerFetchedTexture> normal_tex;
    LLPointer<LLViewerFetchedTexture> mr_tex;
    LLPointer<LLViewerFetchedTexture> emissive_tex;

    LLGLTFHelper::initFetchedTextures(mat_in,
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
