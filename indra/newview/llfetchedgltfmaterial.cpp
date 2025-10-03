/**
 * @file   llfetchedgltfmaterial.cpp
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

#include "llfetchedgltfmaterial.h"

#include "llviewertexturelist.h"
#include "llavatarappearancedefines.h"
#include "llviewerobject.h"
#include "llselectmgr.h"
#include "llshadermgr.h"
#include "pipeline.h"

//static
LLFetchedGLTFMaterial LLFetchedGLTFMaterial::sDefault;

LLFetchedGLTFMaterial::LLFetchedGLTFMaterial()
    : LLGLTFMaterial()
    , mExpectedFlusTime(0.f)
{

}

LLFetchedGLTFMaterial::~LLFetchedGLTFMaterial()
{

}

LLFetchedGLTFMaterial& LLFetchedGLTFMaterial::operator=(const LLFetchedGLTFMaterial& rhs)
{
    LLGLTFMaterial::operator =(rhs);

    mBaseColorTexture = rhs.mBaseColorTexture;
    mNormalTexture = rhs.mNormalTexture;
    mMetallicRoughnessTexture = rhs.mMetallicRoughnessTexture;
    mEmissiveTexture = rhs.mEmissiveTexture;

    return *this;
}

void LLFetchedGLTFMaterial::bind(LLViewerTexture* media_tex)
{
    // glTF 2.0 Specification 3.9.4. Alpha Coverage
    // mAlphaCutoff is only valid for LLGLTFMaterial::ALPHA_MODE_MASK
    F32 min_alpha = -1.0;

    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

    // override emissive and base color textures with media tex if present
    LLViewerTexture* baseColorTex = media_tex ? media_tex : mBaseColorTexture;
    LLViewerTexture* emissiveTex = media_tex ? media_tex : mEmissiveTexture;

    if (!LLPipeline::sShadowRender || (mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK))
    {
        if (mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK)
        {
            min_alpha = mAlphaCutoff;
        }
        shader->uniform1f(LLShaderMgr::MINIMUM_ALPHA, min_alpha);
    }

    if (baseColorTex != nullptr)
    {
        shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, baseColorTex);
    }
    else
    {
        shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, LLViewerFetchedTexture::sWhiteImagep);
    }

    F32 base_color_packed[8];
    mTextureTransform[GLTF_TEXTURE_INFO_BASE_COLOR].getPacked(base_color_packed);
    shader->uniform4fv(LLShaderMgr::TEXTURE_BASE_COLOR_TRANSFORM, 2, (F32*)base_color_packed);

    if (!LLPipeline::sShadowRender)
    {
        if (mNormalTexture.notNull() && mNormalTexture->getDiscardLevel() <= 4)
        {
            shader->bindTexture(LLShaderMgr::BUMP_MAP, mNormalTexture);
        }
        else
        {
            shader->bindTexture(LLShaderMgr::BUMP_MAP, LLViewerFetchedTexture::sFlatNormalImagep);
        }

        if (mMetallicRoughnessTexture.notNull())
        {
            shader->bindTexture(LLShaderMgr::SPECULAR_MAP, mMetallicRoughnessTexture); // PBR linear packed Occlusion, Roughness, Metal.
        }
        else
        {
            shader->bindTexture(LLShaderMgr::SPECULAR_MAP, LLViewerFetchedTexture::sWhiteImagep);
        }

        if (emissiveTex != nullptr)
        {
            shader->bindTexture(LLShaderMgr::EMISSIVE_MAP, emissiveTex);  // PBR sRGB Emissive
        }
        else
        {
            shader->bindTexture(LLShaderMgr::EMISSIVE_MAP, LLViewerFetchedTexture::sWhiteImagep);
        }

        // NOTE: base color factor is baked into vertex stream

        shader->uniform1f(LLShaderMgr::ROUGHNESS_FACTOR, mRoughnessFactor);
        shader->uniform1f(LLShaderMgr::METALLIC_FACTOR, mMetallicFactor);
        shader->uniform3fv(LLShaderMgr::EMISSIVE_COLOR, 1, mEmissiveColor.mV);

        F32 normal_packed[8];
        mTextureTransform[GLTF_TEXTURE_INFO_NORMAL].getPacked(normal_packed);
        shader->uniform4fv(LLShaderMgr::TEXTURE_NORMAL_TRANSFORM, 2, (F32*)normal_packed);

        F32 metallic_roughness_packed[8];
        mTextureTransform[GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS].getPacked(metallic_roughness_packed);
        shader->uniform4fv(LLShaderMgr::TEXTURE_METALLIC_ROUGHNESS_TRANSFORM, 2, (F32*)metallic_roughness_packed);

        F32 emissive_packed[8];
        mTextureTransform[GLTF_TEXTURE_INFO_EMISSIVE].getPacked(emissive_packed);
        shader->uniform4fv(LLShaderMgr::TEXTURE_EMISSIVE_TRANSFORM, 2, (F32*)emissive_packed);
    }
}

LLViewerFetchedTexture* fetch_texture(const LLUUID& id)
{
    LLViewerFetchedTexture* img = nullptr;
    if (id.notNull())
    {
        img = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
        img->addTextureStats(64.f * 64.f, true);
    }
    return img;
};

bool LLFetchedGLTFMaterial::replaceLocalTexture(const LLUUID& tracking_id, const LLUUID& old_id, const LLUUID& new_id)
{
    bool res = false;
    if (mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR] == old_id)
    {
        mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR] = new_id;
        mBaseColorTexture = fetch_texture(new_id);
        res = true;
    }
    if (mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL] == old_id)
    {
        mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL] = new_id;
        mNormalTexture = fetch_texture(new_id);
        res = true;
    }
    if (mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS] == old_id)
    {
        mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS] = new_id;
        mMetallicRoughnessTexture = fetch_texture(new_id);
        res = true;
    }
    if (mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE] == old_id)
    {
        mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE] = new_id;
        mEmissiveTexture = fetch_texture(new_id);
        res = true;
    }

    for (int i = 0; i < GLTF_TEXTURE_INFO_COUNT; ++i)
    {
        if (mTextureId[i] == new_id)
        {
            res = true;
        }
    }

    if (res)
    {
        mTrackingIdToLocalTexture[tracking_id] = new_id;
    }
    else
    {
        mTrackingIdToLocalTexture.erase(tracking_id);
    }
    updateLocalTexDataDigest();

    return res;
}

void LLFetchedGLTFMaterial::addTextureEntry(LLTextureEntry* te)
{
    mTextureEntires.insert(te);
}

void LLFetchedGLTFMaterial::removeTextureEntry(LLTextureEntry* te)
{
    mTextureEntires.erase(te);
}

void LLFetchedGLTFMaterial::updateTextureTracking()
{
    for (local_tex_map_t::value_type &val : mTrackingIdToLocalTexture)
    {
        LLLocalBitmapMgr::getInstance()->associateGLTFMaterial(val.first, this);
    }
}

void LLFetchedGLTFMaterial::materialBegin()
{
    llassert(!mFetching);
    mFetching = true;
}

void LLFetchedGLTFMaterial::onMaterialComplete(std::function<void()> material_complete)
{
    if (!material_complete) { return; }

    if (!mFetching)
    {
        material_complete();
        return;
    }

    materialCompleteCallbacks.push_back(material_complete);
}

void LLFetchedGLTFMaterial::materialComplete(bool success)
{
    llassert(mFetching);
    mFetching = false;
    mFetchSuccess = success;

    for (std::function<void()> material_complete : materialCompleteCallbacks)
    {
        material_complete();
    }
    materialCompleteCallbacks.clear();
    materialCompleteCallbacks.shrink_to_fit();
}
