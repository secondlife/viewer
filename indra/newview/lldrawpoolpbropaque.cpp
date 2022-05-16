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
    // PASS_SIMPLE or PASS_MATERIAL
    //pushBatches(LLRenderPass::PASS_SIMPLE, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
}

