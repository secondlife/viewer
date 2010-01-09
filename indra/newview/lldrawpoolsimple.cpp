/** 
 * @file lldrawpoolsimple.cpp
 * @brief LLDrawPoolSimple class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
static LLGLSLShader* fullbright_shader = NULL;

static LLFastTimerUtil::DeclareTimer FTM_RENDER_SIMPLE_DEFERRED("Deferred Simple");
static LLFastTimerUtil::DeclareTimer FTM_RENDER_GRASS_DEFERRED("Deferred Grass");

void LLDrawPoolGlow::render(S32 pass)
{
	LLFastTimer t(FTM_RENDER_GLOW);
	LLGLEnable blend(GL_BLEND);
	LLGLDisable test(GL_ALPHA_TEST);
	gGL.setSceneBlendType(LLRender::BT_ADD);
	
	U32 shader_level = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);

	if (shader_level > 0 && fullbright_shader)
	{
		fullbright_shader->bind();
	}
	else
	{
		gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
	}

	LLGLDepthTest depth(GL_TRUE, GL_FALSE);
	gGL.setColorMask(false, true);
	renderTexture(LLRenderPass::PASS_GLOW, getVertexDataMask());
	
	gGL.setColorMask(true, false);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	
	if (shader_level > 0 && fullbright_shader)
	{
		fullbright_shader->unbind();
	}
}

void LLDrawPoolGlow::pushBatch(LLDrawInfo& params, U32 mask, BOOL texture)
{
	glColor4ubv(params.mGlowColor.mV);
	LLRenderPass::pushBatch(params, mask, texture);
}


LLDrawPoolSimple::LLDrawPoolSimple() :
	LLRenderPass(POOL_SIMPLE)
{
}

void LLDrawPoolSimple::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

void LLDrawPoolSimple::beginRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_SIMPLE);

	if (LLPipeline::sUnderWaterRender)
	{
		simple_shader = &gObjectSimpleWaterProgram;
	}
	else
	{
		simple_shader = &gObjectSimpleProgram;
	}

	if (mVertexShaderLevel > 0)
	{
		simple_shader->bind();
	}
	else 
	{
		// don't use shaders!
		if (gGLManager.mHasShaderObjects)
		{
			LLGLSLShader::bindNoShader();
		}		
	}
}

void LLDrawPoolSimple::endRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_SIMPLE);
	LLRenderPass::endRenderPass(pass);

	if (mVertexShaderLevel > 0){

		simple_shader->unbind();
	}
}

void LLDrawPoolSimple::render(S32 pass)
{
	LLGLDisable blend(GL_BLEND);
	LLGLDisable alpha_test(GL_ALPHA_TEST);
	
	{ //render simple
		LLFastTimer t(FTM_RENDER_SIMPLE);
		gPipeline.enableLightsDynamic();
		renderTexture(LLRenderPass::PASS_SIMPLE, getVertexDataMask());

		if (LLPipeline::sRenderDeferred)
		{
			renderTexture(LLRenderPass::PASS_BUMP, getVertexDataMask());
		}
	}
}

//===============================
//DEFERRED IMPLEMENTATION
//===============================

void LLDrawPoolSimple::beginDeferredPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_SIMPLE_DEFERRED);
	gDeferredDiffuseProgram.bind();
}

void LLDrawPoolSimple::endDeferredPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_SIMPLE_DEFERRED);
	LLRenderPass::endRenderPass(pass);

	gDeferredDiffuseProgram.unbind();
}

void LLDrawPoolSimple::renderDeferred(S32 pass)
{
	LLGLDisable blend(GL_BLEND);
	LLGLDisable alpha_test(GL_ALPHA_TEST);

	{ //render simple
		LLFastTimer t(FTM_RENDER_SIMPLE_DEFERRED);
		renderTexture(LLRenderPass::PASS_SIMPLE, getVertexDataMask());
	}
}

// grass drawpool
LLDrawPoolGrass::LLDrawPoolGrass() :
 LLRenderPass(POOL_GRASS)
{

}

void LLDrawPoolGrass::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}


void LLDrawPoolGrass::beginRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_GRASS);

	if (LLPipeline::sUnderWaterRender)
	{
		simple_shader = &gObjectSimpleWaterProgram;
	}
	else
	{
		simple_shader = &gObjectSimpleProgram;
	}

	if (mVertexShaderLevel > 0)
	{
		simple_shader->bind();
	}
	else 
	{
		// don't use shaders!
		if (gGLManager.mHasShaderObjects)
		{
			LLGLSLShader::bindNoShader();
		}		
	}
}

void LLDrawPoolGrass::endRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_GRASS);
	LLRenderPass::endRenderPass(pass);

	if (mVertexShaderLevel > 0)
	{
		simple_shader->unbind();
	}
}

void LLDrawPoolGrass::render(S32 pass)
{
	LLGLDisable blend(GL_BLEND);
	gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);

	{
		LLFastTimer t(FTM_RENDER_GRASS);
		LLGLEnable test(GL_ALPHA_TEST);
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
		//render grass
		LLRenderPass::renderTexture(LLRenderPass::PASS_GRASS, getVertexDataMask());
	}			

	gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
}

void LLDrawPoolGrass::beginDeferredPass(S32 pass)
{

}

void LLDrawPoolGrass::endDeferredPass(S32 pass)
{

}

void LLDrawPoolGrass::renderDeferred(S32 pass)
{
	gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.f);

	{
		LLFastTimer t(FTM_RENDER_GRASS_DEFERRED);
		gDeferredTreeProgram.bind();
		LLGLEnable test(GL_ALPHA_TEST);
		//render grass
		LLRenderPass::renderTexture(LLRenderPass::PASS_GRASS, getVertexDataMask());
	}			

	gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
}


// Fullbright drawpool
LLDrawPoolFullbright::LLDrawPoolFullbright() :
	LLRenderPass(POOL_FULLBRIGHT)
{
}

void LLDrawPoolFullbright::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

void LLDrawPoolFullbright::beginRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_FULLBRIGHT);
	
	if (LLPipeline::sUnderWaterRender)
	{
		fullbright_shader = &gObjectFullbrightWaterProgram;
	}
	else
	{
		fullbright_shader = &gObjectFullbrightProgram;
	}
}

void LLDrawPoolFullbright::endRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_FULLBRIGHT);
	LLRenderPass::endRenderPass(pass);

	if (mVertexShaderLevel > 0)
	{
		fullbright_shader->unbind();
	}
}

void LLDrawPoolFullbright::render(S32 pass)
{ //render fullbright
	LLFastTimer t(FTM_RENDER_FULLBRIGHT);
	if (mVertexShaderLevel > 0)
	{
		fullbright_shader->bind();
		fullbright_shader->uniform1f(LLViewerShaderMgr::FULLBRIGHT, 1.f);
	}
	else
	{
		gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
	}
	
	//gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.25f);
	
	//LLGLEnable test(GL_ALPHA_TEST);
	//LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	U32 fullbright_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_COLOR;
	renderTexture(LLRenderPass::PASS_FULLBRIGHT, fullbright_mask);

	//gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
}

S32 LLDrawPoolFullbright::getNumPasses()
{ 
	return 1;
}

