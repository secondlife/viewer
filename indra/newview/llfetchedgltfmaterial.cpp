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
#include "llshadermgr.h"
#include "pipeline.h"

LLFetchedGLTFMaterial::LLFetchedGLTFMaterial()
    : LLGLTFMaterial()
    , mExpectedFlusTime(0.f)
    , mActive(true)
    , mFetching(false)
{

}

LLFetchedGLTFMaterial::~LLFetchedGLTFMaterial()
{
    
}

void LLFetchedGLTFMaterial::bind()
{
    // glTF 2.0 Specification 3.9.4. Alpha Coverage
    // mAlphaCutoff is only valid for LLGLTFMaterial::ALPHA_MODE_MASK
    F32 min_alpha = -1.0;

    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

    if (mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK)
    {
        min_alpha = mAlphaCutoff;
    }
    shader->uniform1f(LLShaderMgr::MINIMUM_ALPHA, min_alpha);

    if (mBaseColorTexture.notNull())
    {
        gGL.getTexUnit(0)->bindFast(mBaseColorTexture);
    }
    else
    {
        gGL.getTexUnit(0)->bindFast(LLViewerFetchedTexture::sWhiteImagep);
    }

    if (!LLPipeline::sShadowRender)
    {
        if (mNormalTexture.notNull())
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

        if (mEmissiveTexture.notNull())
        {
            shader->bindTexture(LLShaderMgr::EMISSIVE_MAP, mEmissiveTexture);  // PBR sRGB Emissive
        }
        else
        {
            shader->bindTexture(LLShaderMgr::EMISSIVE_MAP, LLViewerFetchedTexture::sWhiteImagep);
        }

        // NOTE: base color factor is baked into vertex stream

        shader->uniform1f(LLShaderMgr::ROUGHNESS_FACTOR, mRoughnessFactor);
        shader->uniform1f(LLShaderMgr::METALLIC_FACTOR, mMetallicFactor);
        shader->uniform3fv(LLShaderMgr::EMISSIVE_COLOR, 1, mEmissiveColor.mV);

        const LLMatrix3 base_color_matrix = mTextureTransform[GLTF_TEXTURE_INFO_BASE_COLOR].asMatrix();
        shader->uniformMatrix3fv(LLShaderMgr::TEXTURE_BASECOLOR_MATRIX, 1, false, (F32*)base_color_matrix.mMatrix);
        const LLMatrix3 normal_matrix = mTextureTransform[GLTF_TEXTURE_INFO_NORMAL].asMatrix();
        shader->uniformMatrix3fv(LLShaderMgr::TEXTURE_NORMAL_MATRIX, 1, false, (F32*)normal_matrix.mMatrix);
        const LLMatrix3 metallic_roughness_matrix = mTextureTransform[GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS].asMatrix();
        shader->uniformMatrix3fv(LLShaderMgr::TEXTURE_METALLIC_ROUGHNESS_MATRIX, 1, false, (F32*)metallic_roughness_matrix.mMatrix);
        const LLMatrix3 emissive_matrix = mTextureTransform[GLTF_TEXTURE_INFO_EMISSIVE].asMatrix();
        shader->uniformMatrix3fv(LLShaderMgr::TEXTURE_EMISSIVE_MATRIX, 1, false, (F32*)emissive_matrix.mMatrix);
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

void LLFetchedGLTFMaterial::materialComplete()
{
    llassert(mFetching);
    mFetching = false;

    for (std::function<void()> material_complete : materialCompleteCallbacks)
    {
        material_complete();
    }
    materialCompleteCallbacks.clear();
    materialCompleteCallbacks.shrink_to_fit();
}
