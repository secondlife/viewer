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

// Deferred
void LLDrawPoolPBROpaque::beginDeferredPass(S32 pass)
{
    gDeferredPBROpaqueProgram.bind();
}

void LLDrawPoolPBROpaque::endDeferredPass(S32 pass)
{
    gDeferredPBROpaqueProgram.unbind();
    LLRenderPass::endRenderPass(pass);
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

    // TODO: handle HUDs?
    //if (LLPipeline::sRenderingHUDs)
    //    mShader->uniform1i(LLShaderMgr::NO_ATMO, 1);
    //else
    //    mShader->uniform1i(LLShaderMgr::NO_ATMO, 0);

    // TODO: handle under water?
    // if (LLPipeline::sUnderWaterRender)
    LLCullResult::drawinfo_iterator begin = gPipeline.beginRenderMap(type);
    LLCullResult::drawinfo_iterator end = gPipeline.endRenderMap(type);

    for (LLCullResult::drawinfo_iterator i = begin; i != end; ++i)
    {
        LLDrawInfo& params = **i;

        //gGL.getTexUnit(0)->activate();

        if (params.mTexture.notNull())
        {
            gGL.getTexUnit(0)->bindFast(params.mTexture); // diffuse
        }
        else
        {
            gGL.getTexUnit(0)->bindFast(LLViewerFetchedTexture::sWhiteImagep);
        }

        if (params.mNormalMap)
        {
            gDeferredPBROpaqueProgram.bindTexture(LLShaderMgr::BUMP_MAP, params.mNormalMap);
        }
        else
        {
            // TODO: bind default normal map (???? WTF is it ??? )
        }

        if (params.mSpecularMap)
        {
            gDeferredPBROpaqueProgram.bindTexture(LLShaderMgr::SPECULAR_MAP, params.mSpecularMap); // Packed Occlusion Roughness Metal
        }
        else
        {
            gDeferredPBROpaqueProgram.bindTexture(LLShaderMgr::SPECULAR_MAP, LLViewerFetchedTexture::sWhiteImagep);
        }

        if (params.mEmissiveMap)
        {
            gDeferredPBROpaqueProgram.bindTexture(LLShaderMgr::EMISSIVE_MAP, params.mEmissiveMap);
        }
        else
        {
            gDeferredPBROpaqueProgram.bindTexture(LLShaderMgr::EMISSIVE_MAP, LLViewerFetchedTexture::sWhiteImagep);
        }
        
        gDeferredPBROpaqueProgram.uniform1f(LLShaderMgr::ROUGHNESS_FACTOR, params.mGLTFMaterial->mRoughnessFactor);
        gDeferredPBROpaqueProgram.uniform1f(LLShaderMgr::METALLIC_FACTOR, params.mGLTFMaterial->mMetallicFactor);
        gDeferredPBROpaqueProgram.uniform3fv(LLShaderMgr::EMISSIVE_COLOR, 1, params.mGLTFMaterial->mEmissiveColor.mV);

        LLRenderPass::pushBatch(params, getVertexDataMask(), FALSE, FALSE);
    }
}
