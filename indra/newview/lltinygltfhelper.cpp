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

void strip_alpha_channel(LLPointer<LLImageRaw>& img)
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
void copy_red_channel(LLPointer<LLImageRaw>& src_img, LLPointer<LLImageRaw>& dst_img)
{
    llassert(src_img->getWidth() == dst_img->getWidth() && src_img->getHeight() == dst_img->getHeight());
    llassert(dst_img->getComponents() == 3);

    U32 pixel_count = dst_img->getWidth() * dst_img->getHeight();
    U8* src = src_img->getData();
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

        if (occlusion_img && material.pbrMetallicRoughness.metallicRoughnessTexture.index != material.occlusionTexture.index)
        {
            // occlusion is a distinct texture from pbrMetallicRoughness
            // pack into mr red channel
            int occlusion_idx = material.occlusionTexture.index;
            int mr_idx = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
            if (occlusion_idx != mr_idx)
            {
                //scale occlusion image to match resolution of mr image
                occlusion_img->scale(mr_img->getWidth(), mr_img->getHeight());

                copy_red_channel(occlusion_img, mr_img);
            }
        }
    }
    else if (occlusion_img)
    {
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

LLImageRaw * LLTinyGLTFHelper::getTexture(const std::string & folder, const tinygltf::Model & model, S32 texture_index, std::string & name)
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
        rawImage->verticalFlip();
    }

    return rawImage;
}

LLImageRaw * LLTinyGLTFHelper::getTexture(const std::string & folder, const tinygltf::Model & model, S32 texture_index)
{
    const tinygltf::Image* image = getImageFromTextureIndex(model, texture_index);
    LLImageRaw* rawImage = nullptr;

    if (image != nullptr &&
        image->bits == 8 &&
        !image->image.empty() &&
        image->component <= 4)
    {
        rawImage = new LLImageRaw(&image->image[0], image->width, image->height, image->component);
        rawImage->verticalFlip();
    }

    return rawImage;
}
