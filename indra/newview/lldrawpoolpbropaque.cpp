/** 
 * @file lldrawpoolpbropaque.cpp
 * @brief LLDrawPoolGLTFPBR class implementation
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

LLDrawPoolGLTFPBR::LLDrawPoolGLTFPBR() :
    LLRenderPass(POOL_GLTF_PBR)
{
}

void LLDrawPoolGLTFPBR::renderDeferred(S32 pass)
{
    const U32 type = LLPipeline::RENDER_TYPE_PASS_GLTF_PBR;

    gGL.flush();

    LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;

    for (int i = 0; i < 2; ++i)
    {
        bool rigged = (i == 1);
        LLGLSLShader* shader = LLPipeline::sShadowRender ? &gDeferredShadowAlphaMaskProgram : &gDeferredPBROpaqueProgram;
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
            auto& mat = pparams->mGLTFMaterial;
            
            mat->bind(shader);
            
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

void LLDrawPoolGLTFPBR::renderShadow(S32 pass)
{
    renderDeferred(pass);
}

