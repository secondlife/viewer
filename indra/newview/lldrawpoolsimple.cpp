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
#include "lldrawpoolbump.h"

#include "llviewercamera.h"
#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "pipeline.h"
#include "llglslshader.h"

class LLRenderShinyGlow : public LLDrawPoolBump
{
public:
	LLRenderShinyGlow() { }
	
	void render(S32 pass = 0)
	{
		LLCubeMap* cube_map = gSky.mVOSkyp->getCubeMap();
		if( cube_map )
		{
			cube_map->enable(0);
			cube_map->setMatrix(0);
			cube_map->bind();
			glEnableClientState(GL_NORMAL_ARRAY);
			
			glColor4f(1,1,1,1);

            U32 mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL;
			renderStatic(LLRenderPass::PASS_SHINY, mask);
			renderActive(LLRenderPass::PASS_SHINY, mask);

			glDisableClientState(GL_NORMAL_ARRAY);
			cube_map->disable();
			cube_map->restoreMatrix();
		}
	}
};

void LLDrawPoolGlow::render(S32 pass)
{
	LLGLEnable blend(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	renderTexture(LLRenderPass::PASS_GLOW, getVertexDataMask());
	renderActive(LLRenderPass::PASS_GLOW, getVertexDataMask());

	if (gSky.mVOSkyp)
	{
		glPushMatrix();
		LLVector3 origin = gCamera->getOrigin();
		glTranslatef(origin.mV[0], origin.mV[1], origin.mV[2]);

		LLFace* facep = gSky.mVOSkyp->mFace[LLVOSky::FACE_BLOOM];

		if (facep)
		{
			LLGLDisable cull(GL_CULL_FACE);
			facep->getTexture()->bind();
			glColor4f(1,1,1,1);
			facep->renderIndexed(getVertexDataMask());
		}

		glPopMatrix();
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (LLPipeline::sDynamicReflections)
	{
		LLRenderShinyGlow glow;
		glow.render();
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
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
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
}

void LLDrawPoolSimple::render(S32 pass)
{
	LLGLDisable blend(GL_BLEND);
	LLGLDisable alpha_test(GL_ALPHA_TEST);
	
	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_SIMPLE);
		gPipeline.enableLightsDynamic(1.f);
		renderTexture(LLRenderPass::PASS_SIMPLE, getVertexDataMask());
		renderActive(LLRenderPass::PASS_SIMPLE, getVertexDataMask());
	}

	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_GRASS);
		LLGLEnable blend(GL_BLEND);
		LLGLEnable alpha_test(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//render grass
		LLRenderPass::renderTexture(LLRenderPass::PASS_GRASS, getVertexDataMask());
		glAlphaFunc(GL_GREATER, 0.01f);
	}
		
	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_FULLBRIGHT);
		U32 fullbright_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD | LLVertexBuffer::MAP_COLOR;
		gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
		glDisableClientState(GL_NORMAL_ARRAY);
		renderTexture(LLRenderPass::PASS_FULLBRIGHT, fullbright_mask);
		renderActive(LLRenderPass::PASS_FULLBRIGHT, fullbright_mask);
	}

	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_INVISIBLE);
		U32 invisi_mask = LLVertexBuffer::MAP_VERTEX;
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		renderInvisible(invisi_mask);
		renderActive(LLRenderPass::PASS_INVISIBLE, invisi_mask);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	}
}

