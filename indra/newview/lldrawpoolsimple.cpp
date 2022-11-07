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

static LLGLSLShader* simple_shader = NULL;

static LLTrace::BlockTimerStatHandle FTM_RENDER_SIMPLE_DEFERRED("Deferred Simple");
static LLTrace::BlockTimerStatHandle FTM_RENDER_GRASS_DEFERRED("Deferred Grass");


static void setup_simple_shader(LLGLSLShader* shader)
{
    shader->bind();

    if (LLPipeline::sRenderingHUDs)
    {
        shader->uniform1i(LLShaderMgr::NO_ATMO, 1);
    }
    else
    {
        shader->uniform1i(LLShaderMgr::NO_ATMO, 0);
    }
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
    shader->uniform1f(LLViewerShaderMgr::FULLBRIGHT, 1.f);
}


void LLDrawPoolGlow::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_GLOW);
    render(&gDeferredEmissiveProgram);
}

void LLDrawPoolGlow::render(LLGLSLShader* shader)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_GLOW);
    LLGLEnable blend(GL_BLEND);
    LLGLDisable test(GL_ALPHA_TEST);
    gGL.flush();
    /// Get rid of z-fighting with non-glow pass.
    LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    gGL.setSceneBlendType(LLRender::BT_ADD);
    
    LLGLDepthTest depth(GL_TRUE, GL_FALSE);
    gGL.setColorMask(false, true);

    //first pass -- static objects
    setup_glow_shader(shader);
    pushBatches(LLRenderPass::PASS_GLOW, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
        
    // second pass -- rigged objects
    shader = shader->mRiggedVariant;
    setup_glow_shader(shader);
    pushRiggedBatches(LLRenderPass::PASS_GLOW_RIGGED, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
 
    gGL.setColorMask(true, false);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);  
}

S32 LLDrawPoolGlow::getNumPasses()
{
    return 1;
}

void LLDrawPoolGlow::render(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLGLSLShader* shader = LLPipeline::sUnderWaterRender ? &gObjectEmissiveWaterProgram : &gObjectEmissiveProgram;
    render(shader);
}

LLDrawPoolSimple::LLDrawPoolSimple() :
    LLRenderPass(POOL_SIMPLE)
{
}

void LLDrawPoolSimple::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

S32 LLDrawPoolSimple::getNumPasses()
{
    return 1;
}

void LLDrawPoolSimple::render(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_SIMPLE);

    LLGLDisable blend(GL_BLEND);
    
    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sImpostorRender)
    {
        shader = &gObjectSimpleImpostorProgram;
    }
    else if (LLPipeline::sUnderWaterRender)
    {
        shader = &gObjectSimpleWaterProgram;
    }
    else
    {
        shader = &gObjectSimpleProgram;
    }

    { //render simple
    
        gPipeline.enableLightsDynamic();

        U32 mask = getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX;

        // first pass -- static objects
        {
            setup_simple_shader(shader);
            pushBatches(LLRenderPass::PASS_SIMPLE, mask, TRUE, TRUE);

            if (LLPipeline::sRenderDeferred)
            { //if deferred rendering is enabled, bump faces aren't registered as simple
                //render bump faces here as simple so bump faces will appear under water
                pushBatches(LLRenderPass::PASS_BUMP, mask, TRUE, TRUE);
                pushBatches(LLRenderPass::PASS_MATERIAL, mask, TRUE, TRUE);
                pushBatches(LLRenderPass::PASS_SPECMAP, mask, TRUE, TRUE);
                pushBatches(LLRenderPass::PASS_NORMMAP, mask, TRUE, TRUE);
                pushBatches(LLRenderPass::PASS_NORMSPEC, mask, TRUE, TRUE);
            }
        }
        
        //second pass, rigged
        {
            shader = shader->mRiggedVariant;
            setup_simple_shader(shader);
            pushRiggedBatches(LLRenderPass::PASS_SIMPLE_RIGGED, mask, TRUE, TRUE);

            if (LLPipeline::sRenderDeferred)
            { //if deferred rendering is enabled, bump faces aren't registered as simple
                //render bump faces here as simple so bump faces will appear under water
                pushRiggedBatches(LLRenderPass::PASS_BUMP_RIGGED, mask, TRUE, TRUE);
                pushRiggedBatches(LLRenderPass::PASS_MATERIAL_RIGGED, mask, TRUE, TRUE);
                pushRiggedBatches(LLRenderPass::PASS_SPECMAP_RIGGED, mask, TRUE, TRUE);
                pushRiggedBatches(LLRenderPass::PASS_NORMMAP_RIGGED, mask, TRUE, TRUE);
                pushRiggedBatches(LLRenderPass::PASS_NORMSPEC_RIGGED, mask, TRUE, TRUE);
            }
        }
    }
}


static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_MASK("Alpha Mask");

LLDrawPoolAlphaMask::LLDrawPoolAlphaMask() :
    LLRenderPass(POOL_ALPHA_MASK)
{
}

void LLDrawPoolAlphaMask::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

void LLDrawPoolAlphaMask::render(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLGLDisable blend(GL_BLEND);
    
    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sUnderWaterRender)
    {
        shader = &gObjectSimpleWaterAlphaMaskProgram;
    }
    else
    {
        shader = &gObjectSimpleAlphaMaskProgram;
    }

    // render static
    setup_simple_shader(shader);
    pushMaskBatches(LLRenderPass::PASS_ALPHA_MASK, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
    pushMaskBatches(LLRenderPass::PASS_MATERIAL_ALPHA_MASK, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
    pushMaskBatches(LLRenderPass::PASS_SPECMAP_MASK, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
    pushMaskBatches(LLRenderPass::PASS_NORMMAP_MASK, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
    pushMaskBatches(LLRenderPass::PASS_NORMSPEC_MASK, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);

    // render rigged
    setup_simple_shader(shader->mRiggedVariant);
    pushRiggedMaskBatches(LLRenderPass::PASS_ALPHA_MASK_RIGGED, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
    pushRiggedMaskBatches(LLRenderPass::PASS_MATERIAL_ALPHA_MASK_RIGGED, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
    pushRiggedMaskBatches(LLRenderPass::PASS_SPECMAP_MASK_RIGGED, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
    pushRiggedMaskBatches(LLRenderPass::PASS_NORMMAP_MASK_RIGGED, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
    pushRiggedMaskBatches(LLRenderPass::PASS_NORMSPEC_MASK_RIGGED, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
}

LLDrawPoolFullbrightAlphaMask::LLDrawPoolFullbrightAlphaMask() :
    LLRenderPass(POOL_FULLBRIGHT_ALPHA_MASK)
{
}

void LLDrawPoolFullbrightAlphaMask::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

void LLDrawPoolFullbrightAlphaMask::render(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_MASK);

    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sUnderWaterRender)
    {
        shader = &gObjectFullbrightWaterAlphaMaskProgram;
    }
    else
    {
        shader = &gObjectFullbrightAlphaMaskProgram;
    }

    // render static
    setup_fullbright_shader(shader);
    pushMaskBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);

    // render rigged
    setup_fullbright_shader(shader->mRiggedVariant);
    pushRiggedMaskBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
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
    LLGLDisable alpha_test(GL_ALPHA_TEST);

    //render static
    setup_simple_shader(&gDeferredDiffuseProgram);
    pushBatches(LLRenderPass::PASS_SIMPLE, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
    
    //render rigged
    setup_simple_shader(gDeferredDiffuseProgram.mRiggedVariant);
    pushRiggedBatches(LLRenderPass::PASS_SIMPLE_RIGGED, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
}

static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_MASK_DEFERRED("Deferred Alpha Mask");


void LLDrawPoolAlphaMask::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_MASK_DEFERRED);
    LLGLSLShader* shader = &gDeferredDiffuseAlphaMaskProgram;

    //render static
    setup_simple_shader(shader);
    pushMaskBatches(LLRenderPass::PASS_ALPHA_MASK, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);

    //render rigged
    setup_simple_shader(shader->mRiggedVariant);
    pushRiggedMaskBatches(LLRenderPass::PASS_ALPHA_MASK_RIGGED, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
}

// grass drawpool
LLDrawPoolGrass::LLDrawPoolGrass() :
 LLRenderPass(POOL_GRASS)
{

}

void LLDrawPoolGrass::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}


void LLDrawPoolGrass::beginRenderPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_GRASS);
    stop_glerror();

    if (LLPipeline::sUnderWaterRender)
    {
        simple_shader = &gObjectAlphaMaskNonIndexedWaterProgram;
    }
    else
    {
        simple_shader = &gObjectAlphaMaskNonIndexedProgram;
    }

    if (mShaderLevel > 0)
    {
        simple_shader->bind();
        simple_shader->setMinimumAlpha(0.5f);
        if (LLPipeline::sRenderingHUDs)
        {
            simple_shader->uniform1i(LLShaderMgr::NO_ATMO, 1);
        }
        else
        {
            simple_shader->uniform1i(LLShaderMgr::NO_ATMO, 0);
        }
    }
    else 
    {
        gGL.flush();
        LLGLSLShader::bindNoShader();
    }
}

void LLDrawPoolGrass::endRenderPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_GRASS);
    LLRenderPass::endRenderPass(pass);

    if (mShaderLevel > 0)
    {
        simple_shader->unbind();
    }
    else
    {
        gGL.flush();
    }
}

void LLDrawPoolGrass::render(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLGLDisable blend(GL_BLEND);
    
    {
        //LL_RECORD_BLOCK_TIME(FTM_RENDER_GRASS);
        LLGLEnable test(GL_ALPHA_TEST);
        gGL.setSceneBlendType(LLRender::BT_ALPHA);
        //render grass
        LLRenderPass::pushBatches(LLRenderPass::PASS_GRASS, getVertexDataMask());
    }
}

void LLDrawPoolGrass::beginDeferredPass(S32 pass)
{

}

void LLDrawPoolGrass::endDeferredPass(S32 pass)
{

}

void LLDrawPoolGrass::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    {
        //LL_RECORD_BLOCK_TIME(FTM_RENDER_GRASS_DEFERRED);
        gDeferredNonIndexedDiffuseAlphaMaskProgram.bind();
        gDeferredNonIndexedDiffuseAlphaMaskProgram.setMinimumAlpha(0.5f);

        if (LLPipeline::sRenderingHUDs)
        {
            gDeferredNonIndexedDiffuseAlphaMaskProgram.uniform1i(LLShaderMgr::NO_ATMO, 1);
        }
        else
        {
            gDeferredNonIndexedDiffuseAlphaMaskProgram.uniform1i(LLShaderMgr::NO_ATMO, 0);
        }

        //render grass
        LLRenderPass::pushBatches(LLRenderPass::PASS_GRASS, getVertexDataMask());
    }           
}


// Fullbright drawpool
LLDrawPoolFullbright::LLDrawPoolFullbright() :
    LLRenderPass(POOL_FULLBRIGHT)
{
}

void LLDrawPoolFullbright::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}


void LLDrawPoolFullbright::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_FULLBRIGHT);

    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sUnderWaterRender)
    {
        shader = &gDeferredFullbrightWaterProgram;
    }
    else
    {
        shader = &gDeferredFullbrightProgram;
    }

    gGL.setSceneBlendType(LLRender::BT_ALPHA);
    U32 fullbright_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_TEXTURE_INDEX;
    
    // render static
    setup_fullbright_shader(shader);
    pushBatches(LLRenderPass::PASS_FULLBRIGHT, fullbright_mask, TRUE, TRUE);
    
    // render rigged
    setup_fullbright_shader(shader->mRiggedVariant);
    pushRiggedBatches(LLRenderPass::PASS_FULLBRIGHT_RIGGED, fullbright_mask, TRUE, TRUE);
}

void LLDrawPoolFullbright::render(S32 pass)
{ //render fullbright
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_FULLBRIGHT);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    stop_glerror();
    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sUnderWaterRender)
    {
        shader = &gObjectFullbrightWaterProgram;
    }
    else
    {
        shader = &gObjectFullbrightProgram;
    }


    U32 mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_TEXTURE_INDEX;

    // render static
    setup_fullbright_shader(shader);
    pushBatches(LLRenderPass::PASS_FULLBRIGHT, mask, TRUE, TRUE);
    pushBatches(LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE, mask, TRUE, TRUE);
    pushBatches(LLRenderPass::PASS_SPECMAP_EMISSIVE, mask, TRUE, TRUE);
    pushBatches(LLRenderPass::PASS_NORMMAP_EMISSIVE, mask, TRUE, TRUE);
    pushBatches(LLRenderPass::PASS_NORMSPEC_EMISSIVE, mask, TRUE, TRUE);
 
    // render rigged
    setup_fullbright_shader(shader->mRiggedVariant);
    pushRiggedBatches(LLRenderPass::PASS_FULLBRIGHT_RIGGED, mask, TRUE, TRUE);
    pushRiggedBatches(LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE_RIGGED, mask, TRUE, TRUE);
    pushRiggedBatches(LLRenderPass::PASS_SPECMAP_EMISSIVE_RIGGED, mask, TRUE, TRUE);
    pushRiggedBatches(LLRenderPass::PASS_NORMMAP_EMISSIVE_RIGGED, mask, TRUE, TRUE);
    pushRiggedBatches(LLRenderPass::PASS_NORMSPEC_EMISSIVE_RIGGED, mask, TRUE, TRUE);
}

S32 LLDrawPoolFullbright::getNumPasses()
{ 
    return 1;
}

void LLDrawPoolFullbrightAlphaMask::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_FULLBRIGHT);
    
    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sRenderingHUDs)
    {
        shader = &gObjectFullbrightAlphaMaskProgram;
    }
    else if (LLPipeline::sRenderDeferred)
    {
        if (LLPipeline::sUnderWaterRender)
        {
            shader = &gDeferredFullbrightAlphaMaskWaterProgram;
        }
        else
        {
            shader = &gDeferredFullbrightAlphaMaskProgram;
        }
    }
    else
    {
        shader = &gObjectFullbrightAlphaMaskProgram;
    }

    LLGLDisable blend(GL_BLEND);
    U32 fullbright_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_TEXTURE_INDEX;
    
    // render static
    setup_fullbright_shader(shader);
    pushMaskBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, fullbright_mask, TRUE, TRUE);
    
    // render rigged
    setup_fullbright_shader(shader->mRiggedVariant);
    pushRiggedMaskBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED, fullbright_mask, TRUE, TRUE);
}

