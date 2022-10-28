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

void LLFetchedGLTFMaterial::bind(LLGLSLShader* shader)
{
    // glTF 2.0 Specification 3.9.4. Alpha Coverage
    // mAlphaCutoff is only valid for LLGLTFMaterial::ALPHA_MODE_MASK
    F32 min_alpha = -1.0;

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
    }

}
