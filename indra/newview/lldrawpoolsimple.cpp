/**
 * @file lldrawpoolsimple.cpp
 * @brief LLDrawPoolSimple class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "lldrawpoolsimple.h"

#include "llviewercamera.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llviewershadermgr.h"
#include "llrender.h"

static LLTrace::BlockTimerStatHandle FTM_RENDER_SIMPLE_DEFERRED("Deferred Simple");
static LLTrace::BlockTimerStatHandle FTM_RENDER_GRASS_DEFERRED("Deferred Grass");


static void setup_simple_shader(LLGLSLShader* shader)
{
    shader->bind();
}

static void setup_glow_shader(LLGLSLShader* shader)
{
    setup_simple_shader(shader);
    if (LLPipeline::sRenderDeferred && !LLPipeline::sRenderingHUDs)
    {
        shader->uniform1f(LLShaderMgr::TEXTURE_GAMMA, 2.2f);
    }
    else
    {
        shader->uniform1f(LLShaderMgr::TEXTURE_GAMMA, 1.f);
    }
}

static void setup_fullbright_shader(LLGLSLShader* shader)
{
    setup_glow_shader(shader);

    S32 channel = shader->enableTexture(LLShaderMgr::EXPOSURE_MAP);
    if (channel > -1)
    {
        gGL.getTexUnit(channel)->bind(&gPipeline.mExposureMap);
    }

    shader->uniform1f(LLViewerShaderMgr::FULLBRIGHT, 1.f);
}


void LLDrawPoolGlow::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLGLSLShader* shader = &gDeferredEmissiveProgram;

    LLGLEnable blend(GL_BLEND);
    gGL.flush();
    /// Get rid of z-fighting with non-glow pass.
    LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    gGL.setSceneBlendType(LLRender::BT_ADD);

    LLGLDepthTest depth(GL_TRUE, GL_FALSE);
    gGL.setColorMask(false, true);

    //first pass -- static objects
    setup_glow_shader(shader);
    pushBatches(LLRenderPass::PASS_GLOW, true, true);

    // second pass -- rigged objects
    shader = shader->mRiggedVariant;
    setup_glow_shader(shader);
    pushRiggedBatches(LLRenderPass::PASS_GLOW_RIGGED, true, true);

    gGL.setColorMask(true, false);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

LLDrawPoolSimple::LLDrawPoolSimple() :
    LLRenderPass(POOL_SIMPLE)
{
}

static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_MASK("Alpha Mask");

LLDrawPoolAlphaMask::LLDrawPoolAlphaMask() :
    LLRenderPass(POOL_ALPHA_MASK)
{
}

LLDrawPoolFullbrightAlphaMask::LLDrawPoolFullbrightAlphaMask() :
    LLRenderPass(POOL_FULLBRIGHT_ALPHA_MASK)
{
}

//===============================
//DEFERRED IMPLEMENTATION
//===============================

S32 LLDrawPoolSimple::getNumDeferredPasses()
{
    return 1;
}

void LLDrawPoolSimple::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_SIMPLE_DEFERRED);
    LLGLDisable blend(GL_BLEND);

    //render static
    setup_simple_shader(&gDeferredDiffuseProgram);
    pushBatches(LLRenderPass::PASS_SIMPLE, true, true);

    //render rigged
    setup_simple_shader(gDeferredDiffuseProgram.mRiggedVariant);
    pushRiggedBatches(LLRenderPass::PASS_SIMPLE_RIGGED, true, true);
}

static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_MASK_DEFERRED("Deferred Alpha Mask");


void LLDrawPoolAlphaMask::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_MASK_DEFERRED);
    LLGLSLShader* shader = &gDeferredDiffuseAlphaMaskProgram;

    //render static
    setup_simple_shader(shader);
    pushMaskBatches(LLRenderPass::PASS_ALPHA_MASK, true, true);

    //render rigged
    setup_simple_shader(shader->mRiggedVariant);
    pushRiggedMaskBatches(LLRenderPass::PASS_ALPHA_MASK_RIGGED, true, true);
}

// grass drawpool
LLDrawPoolGrass::LLDrawPoolGrass() :
 LLRenderPass(POOL_GRASS)
{

}

void LLDrawPoolGrass::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    {
        gDeferredNonIndexedDiffuseAlphaMaskProgram.bind();
        gDeferredNonIndexedDiffuseAlphaMaskProgram.setMinimumAlpha(0.5f);

        //render grass
        LLRenderPass::pushBatches(LLRenderPass::PASS_GRASS, getVertexDataMask());
    }
}


// Fullbright drawpool
LLDrawPoolFullbright::LLDrawPoolFullbright() :
    LLRenderPass(POOL_FULLBRIGHT)
{
}

void LLDrawPoolFullbright::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_FULLBRIGHT);

    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sRenderingHUDs)
    {
        shader = &gHUDFullbrightProgram;
    }
    else
    {
        shader = &gDeferredFullbrightProgram;
    }

    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    // render static
    setup_fullbright_shader(shader);
    pushBatches(LLRenderPass::PASS_FULLBRIGHT, true, true);

    if (!LLPipeline::sRenderingHUDs)
    {
        // render rigged
        setup_fullbright_shader(shader->mRiggedVariant);
        pushRiggedBatches(LLRenderPass::PASS_FULLBRIGHT_RIGGED, true, true);
    }
}

void LLDrawPoolFullbrightAlphaMask::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_FULLBRIGHT);

    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sRenderingHUDs)
    {
        shader = &gHUDFullbrightAlphaMaskProgram;
    }
    else
    {
        shader = &gDeferredFullbrightAlphaMaskProgram;
    }

    LLGLDisable blend(GL_BLEND);

    // render static
    setup_fullbright_shader(shader);
    pushMaskBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, true, true);

    if (!LLPipeline::sRenderingHUDs)
    {
        // render rigged
        setup_fullbright_shader(shader->mRiggedVariant);
        pushRiggedMaskBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED, true, true);
    }
}

