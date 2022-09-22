/** 
 * @file lldrawpoolpbropaque.cpp
 * @brief LLDrawPoolPBROpaque class implementation
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

#include "lldrawpool.h"
#include "lldrawpoolpbropaque.h"
#include "llviewershadermgr.h"
#include "pipeline.h"

LLDrawPoolPBROpaque::LLDrawPoolPBROpaque() :
    LLRenderPass(POOL_PBR_OPAQUE)
{
}

void LLDrawPoolPBROpaque::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT); 
}

// Forward
void LLDrawPoolPBROpaque::beginRenderPass(S32 pass)
{
}

void LLDrawPoolPBROpaque::endRenderPass( S32 pass )
{
}

void LLDrawPoolPBROpaque::render(S32 pass)
{
}

void LLDrawPoolPBROpaque::renderDeferred(S32 pass)
{
    if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_MATERIALS))
    {
         return;
    }

    const U32 type = LLPipeline::RENDER_TYPE_PASS_PBR_OPAQUE;
    if (!gPipeline.hasRenderType(type))
    {
        return;
    }

    gGL.flush();

    LLGLDisable blend(GL_BLEND);
    LLGLDisable alpha_test(GL_ALPHA_TEST);

    LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;

    for (int i = 0; i < 2; ++i)
    {
        bool rigged = (i == 1);
        LLGLSLShader* shader = &gDeferredPBROpaqueProgram;
        U32 vertex_data_mask = getVertexDataMask();

        if (rigged)
        {
            shader = shader->mRiggedVariant;
            vertex_data_mask |= LLVertexBuffer::MAP_WEIGHT4;
        }

        shader->bind();

        LLCullResult::drawinfo_iterator begin = gPipeline.beginRenderMap(type+i);
        LLCullResult::drawinfo_iterator end = gPipeline.endRenderMap(type+i);

        for (LLCullResult::drawinfo_iterator i = begin; i != end; ++i)
        {
            LLDrawInfo* pparams = *i;
            LLGLTFMaterial *mat = pparams->mGLTFMaterial;

            // glTF 2.0 Specification 3.9.4. Alpha Coverage
            // mAlphaCutoff is only valid for LLGLTFMaterial::ALPHA_MODE_MASK
            F32 min_alpha = -1.0;
            if (mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK)
            {
                min_alpha = mat->mAlphaCutoff;
            }
            shader->uniform1f(LLShaderMgr::MINIMUM_ALPHA, min_alpha);

            if (pparams->mTexture.notNull())
            {
                gGL.getTexUnit(0)->bindFast(pparams->mTexture); // diffuse
            }
            else
            {
                gGL.getTexUnit(0)->bindFast(LLViewerFetchedTexture::sWhiteImagep);
            }

            if (pparams->mNormalMap)
            {
                shader->bindTexture(LLShaderMgr::BUMP_MAP, pparams->mNormalMap);
            }
            else
            {
                shader->bindTexture(LLShaderMgr::BUMP_MAP, LLViewerFetchedTexture::sFlatNormalImagep);
            }

            if (pparams->mSpecularMap)
            {
                shader->bindTexture(LLShaderMgr::SPECULAR_MAP, pparams->mSpecularMap); // PBR linear packed Occlusion, Roughness, Metal.
            }
            else
            {
                shader->bindTexture(LLShaderMgr::SPECULAR_MAP, LLViewerFetchedTexture::sWhiteImagep);
            }

            if (pparams->mEmissiveMap)
            {
                shader->bindTexture(LLShaderMgr::EMISSIVE_MAP, pparams->mEmissiveMap);  // PBR sRGB Emissive
            }
            else
            {
                shader->bindTexture(LLShaderMgr::EMISSIVE_MAP, LLViewerFetchedTexture::sWhiteImagep);
            }

            shader->uniform1f(LLShaderMgr::ROUGHNESS_FACTOR, pparams->mGLTFMaterial->mRoughnessFactor);
            shader->uniform1f(LLShaderMgr::METALLIC_FACTOR, pparams->mGLTFMaterial->mMetallicFactor);
            shader->uniform3fv(LLShaderMgr::EMISSIVE_COLOR, 1, pparams->mGLTFMaterial->mEmissiveColor.mV);

            LLGLDisable cull_face(mat->mDoubleSided ? GL_CULL_FACE : 0);

            bool tex_setup = false;
            if (pparams->mTextureMatrix)
            { //special case implementation of texture animation here because of special handling of textures for PBR batches
                tex_setup = true;
                gGL.getTexUnit(0)->activate();
                gGL.matrixMode(LLRender::MM_TEXTURE);
                gGL.loadMatrix((GLfloat*)pparams->mTextureMatrix->mMatrix);
                gPipeline.mTextureMatrixOps++;
            }

            if (rigged)
            {
                if (pparams->mAvatar.notNull() && (lastAvatar != pparams->mAvatar || lastMeshId != pparams->mSkinInfo->mHash))
                {
                    uploadMatrixPalette(*pparams);
                    lastAvatar = pparams->mAvatar;
                    lastMeshId = pparams->mSkinInfo->mHash;
                }

                pushBatch(*pparams, vertex_data_mask, FALSE, FALSE);
            }
            else
            {
                pushBatch(*pparams, vertex_data_mask, FALSE, FALSE);
            }

            if (tex_setup)
            {
                gGL.matrixMode(LLRender::MM_TEXTURE0);
                gGL.loadIdentity();
                gGL.matrixMode(LLRender::MM_MODELVIEW);
            }
        }
    }

    LLGLSLShader::sCurBoundShaderPtr->unbind();
}
