/** 
 * @file lldrawpool.cpp
 * @brief LLDrawPoolMaterials class implementation
 * @author Jonathan "Geenz" Goodman
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#include "lldrawpoolmaterials.h"
#include "llviewershadermgr.h"
#include "pipeline.h"

S32 diffuse_channel = -1;

LLDrawPoolMaterials::LLDrawPoolMaterials()
:  LLRenderPass(LLDrawPool::POOL_MATERIALS)
{
	
}

void LLDrawPoolMaterials::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT); 
}

void LLDrawPoolMaterials::beginDeferredPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_MATERIALS);
}

void LLDrawPoolMaterials::endDeferredPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_MATERIALS);
	LLRenderPass::endRenderPass(pass);
}

void LLDrawPoolMaterials::renderDeferred(S32 pass)
{
	U32 type = LLRenderPass::PASS_MATERIALS;
	LLCullResult::drawinfo_iterator begin = gPipeline.beginRenderMap(type);
	LLCullResult::drawinfo_iterator end = gPipeline.endRenderMap(type);
	
	for (LLCullResult::drawinfo_iterator i = begin; i != end; ++i)
	{
		LLDrawInfo& params = **i;
		
		switch (params.mDiffuseAlphaMode)
		{
			case 0:
				mShader = &gDeferredMaterialShinyNormal;
				mShader->bind();
				break;
			case 1: // Alpha blending not supported in the opaque draw pool.
				return;
			case 2:
				mShader = &gDeferredMaterialShinyNormalAlphaTest;
				mShader->bind();
				mShader->setMinimumAlpha(params.mAlphaMaskCutoff);
				break;
			case 3:
				mShader = &gDeferredMaterialShinyNormalEmissive;
				mShader->bind();
				break;
		};
		
		
		
		mShader->uniform4f(LLShaderMgr::SPECULAR_COLOR, params.mSpecColor.mV[0], params.mSpecColor.mV[1], params.mSpecColor.mV[2], params.mSpecColor.mV[3]);
		mShader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, params.mEnvIntensity);
		
		if (params.mNormalMap)
		{
			params.mNormalMap->addTextureStats(params.mVSize);
			bindNormalMap(params.mNormalMap);
		}
		
		if (params.mSpecularMap)
		{
			params.mSpecularMap->addTextureStats(params.mVSize);
			bindSpecularMap(params.mSpecularMap);
		}
		
		diffuse_channel = mShader->enableTexture(LLShaderMgr::DIFFUSE_MAP);
		pushBatch(params, VERTEX_DATA_MASK, TRUE);
		mShader->disableTexture(LLShaderMgr::DIFFUSE_MAP);
		mShader->unbind();
	}
}

void LLDrawPoolMaterials::bindSpecularMap(LLViewerTexture* tex)
{
	mShader->bindTexture(LLShaderMgr::SPECULAR_MAP, tex);
}

void LLDrawPoolMaterials::bindNormalMap(LLViewerTexture* tex)
{
	mShader->bindTexture(LLShaderMgr::BUMP_MAP, tex);
}

void LLDrawPoolMaterials::beginRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_MATERIALS);
	
	// Materials isn't supported under forward rendering.
	// Use the simple shaders to handle it instead.
	// This is basically replicated from LLDrawPoolSimple.
	
	if (LLPipeline::sUnderWaterRender)
	{
		mShader = &gObjectSimpleWaterProgram;
	}
	else
	{
		mShader = &gObjectSimpleProgram;
	}
	
	if (mVertexShaderLevel > 0)
	{
		mShader->bind();
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

void LLDrawPoolMaterials::endRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_MATERIALS);
	stop_glerror();
	LLRenderPass::endRenderPass(pass);
	stop_glerror();
	if (mVertexShaderLevel > 0)
	{
		mShader->unbind();
	}
}

void LLDrawPoolMaterials::render(S32 pass)
{
	LLGLDisable blend(GL_BLEND);
	
	{ //render simple
		LLFastTimer t(FTM_RENDER_MATERIALS);
		gPipeline.enableLightsDynamic();
		
		if (mVertexShaderLevel > 0)
		{
			U32 mask = getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX;
			
			pushBatches(LLRenderPass::PASS_MATERIALS, mask, TRUE, TRUE);
		}
		else
		{
			LLGLDisable alpha_test(GL_ALPHA_TEST);
			renderTexture(LLRenderPass::PASS_MATERIALS, getVertexDataMask());
		}
	}
}


void LLDrawPoolMaterials::pushBatch(LLDrawInfo& params, U32 mask, BOOL texture, BOOL batch_textures)
{
	applyModelMatrix(params);
	
	bool tex_setup = false;
	
	if (batch_textures && params.mTextureList.size() > 1)
	{
		for (U32 i = 0; i < params.mTextureList.size(); ++i)
		{
			if (params.mTextureList[i].notNull())
			{
				gGL.getTexUnit(i)->bind(params.mTextureList[i], TRUE);
			}
		}
	}
	else
	{ //not batching textures or batch has only 1 texture -- might need a texture matrix
		if (params.mTextureMatrix)
		{
			//if (mShiny)
			{
				gGL.getTexUnit(0)->activate();
				gGL.matrixMode(LLRender::MM_TEXTURE);
			}
			
			gGL.loadMatrix((GLfloat*) params.mTextureMatrix->mMatrix);
			gPipeline.mTextureMatrixOps++;
			
			tex_setup = true;
		}
		
		if (mVertexShaderLevel > 1 && texture)
		{
			if (params.mTexture.notNull())
			{
				gGL.getTexUnit(diffuse_channel)->bind(params.mTexture);
				params.mTexture->addTextureStats(params.mVSize);
			}
			else
			{
				gGL.getTexUnit(diffuse_channel)->unbind(LLTexUnit::TT_TEXTURE);
			}
		}
	}
	
	if (params.mGroup)
	{
		params.mGroup->rebuildMesh();
	}
	params.mVertexBuffer->setBuffer(mask);
	params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
	gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
	if (tex_setup)
	{
		gGL.getTexUnit(0)->activate();
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
}
