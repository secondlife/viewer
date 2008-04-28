/** 
 * @file lldrawpoolsimple.cpp
 * @brief LLDrawPoolSimple class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llglslshader.h"
#include "llglimmediate.h"


static LLGLSLShader* simple_shader = NULL;
static LLGLSLShader* fullbright_shader = NULL;

void LLDrawPoolGlow::render(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_GLOW);
	LLGLEnable blend(GL_BLEND);
	LLGLDisable test(GL_ALPHA_TEST);
	gGL.blendFunc(GL_ONE, GL_ONE);
	
	U32 shader_level = LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_OBJECT);

	if (shader_level > 0 && fullbright_shader)
	{
		fullbright_shader->bind();
	}
	else
	{
		gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
	}

	LLGLDepthTest depth(GL_TRUE, GL_FALSE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
	renderTexture(LLRenderPass::PASS_GLOW, getVertexDataMask());
	
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	gGL.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
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
	mVertexShaderLevel = LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_OBJECT);
}

void LLDrawPoolSimple::beginRenderPass(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SIMPLE);

	if (LLPipeline::sUnderWaterRender)
	{
		simple_shader = &gObjectSimpleWaterProgram;
		fullbright_shader = &gObjectFullbrightWaterProgram;
	}
	else
	{
		simple_shader = &gObjectSimpleProgram;
		fullbright_shader = &gObjectFullbrightProgram;
	}

	if (mVertexShaderLevel > 0)
	{
		simple_shader->bind();
		simple_shader->uniform1f(LLShaderMgr::FULLBRIGHT, 0.f);
	}
	else 
	{
		// don't use shaders!
		if (gGLManager.mHasShaderObjects)
		{
			glUseProgramObjectARB(0);
		}		
	}
}

void LLDrawPoolSimple::endRenderPass(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SIMPLE);
	LLRenderPass::endRenderPass(pass);

	if (mVertexShaderLevel > 0){

		simple_shader->unbind();
	}
}

void LLDrawPoolSimple::render(S32 pass)
{
	LLGLDisable blend(GL_BLEND);
	LLGLState alpha_test(GL_ALPHA_TEST, gPipeline.canUseWindLightShadersOnObjects());
	glAlphaFunc(GL_GREATER, 0.5f);

	{ //render simple
		LLFastTimer t(LLFastTimer::FTM_RENDER_SIMPLE);
		gPipeline.enableLightsDynamic();
		renderTexture(LLRenderPass::PASS_SIMPLE, getVertexDataMask());
	}

	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_GRASS);
		LLGLEnable test(GL_ALPHA_TEST);
		LLGLEnable blend(GL_BLEND);
		gGL.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//render grass
		LLRenderPass::renderTexture(LLRenderPass::PASS_GRASS, getVertexDataMask());
	}			

	{ //render fullbright
		if (mVertexShaderLevel > 0)
		{
			fullbright_shader->bind();
			fullbright_shader->uniform1f(LLShaderMgr::FULLBRIGHT, 1.f);
		}
		else
		{
			gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
		}
		LLFastTimer t(LLFastTimer::FTM_RENDER_FULLBRIGHT);
		U32 fullbright_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD | LLVertexBuffer::MAP_COLOR;
		renderTexture(LLRenderPass::PASS_FULLBRIGHT, fullbright_mask);
	}

	glAlphaFunc(GL_GREATER, 0.01f);
}

