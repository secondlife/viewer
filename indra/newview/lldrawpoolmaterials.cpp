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
#include "llglcommonfunc.h"

S32 diffuse_channel = -1;

LLDrawPoolMaterials::LLDrawPoolMaterials()
:  LLRenderPass(LLDrawPool::POOL_MATERIALS)
{
	
}

void LLDrawPoolMaterials::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT); 
}

S32 LLDrawPoolMaterials::getNumDeferredPasses()
{
	return 12;
}

void LLDrawPoolMaterials::beginDeferredPass(S32 pass)
{
	U32 shader_idx[] = 
	{
		0, //LLRenderPass::PASS_MATERIAL,
		//1, //LLRenderPass::PASS_MATERIAL_ALPHA,
		2, //LLRenderPass::PASS_MATERIAL_ALPHA_MASK,
		3, //LLRenderPass::PASS_MATERIAL_ALPHA_GLOW,
		4, //LLRenderPass::PASS_SPECMAP,
		//5, //LLRenderPass::PASS_SPECMAP_BLEND,
		6, //LLRenderPass::PASS_SPECMAP_MASK,
		7, //LLRenderPass::PASS_SPECMAP_GLOW,
		8, //LLRenderPass::PASS_NORMMAP,
		//9, //LLRenderPass::PASS_NORMMAP_BLEND,
		10, //LLRenderPass::PASS_NORMMAP_MASK,
		11, //LLRenderPass::PASS_NORMMAP_GLOW,
		12, //LLRenderPass::PASS_NORMSPEC,
		//13, //LLRenderPass::PASS_NORMSPEC_BLEND,
		14, //LLRenderPass::PASS_NORMSPEC_MASK,
		15, //LLRenderPass::PASS_NORMSPEC_GLOW,
	};
	
	mShader = &(gDeferredMaterialProgram[shader_idx[pass]]);

	if (LLPipeline::sUnderWaterRender)
	{
		mShader = &(gDeferredMaterialWaterProgram[shader_idx[pass]]);
	}

	mShader->bind();

	diffuse_channel = mShader->enableTexture(LLShaderMgr::DIFFUSE_MAP);
		
	LL_RECORD_BLOCK_TIME(FTM_RENDER_MATERIALS);
}

void LLDrawPoolMaterials::endDeferredPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_MATERIALS);

	mShader->unbind();

	LLRenderPass::endRenderPass(pass);
}

void LLDrawPoolMaterials::renderDeferred(S32 pass)
{
	static const U32 type_list[] = 
	{
		LLRenderPass::PASS_MATERIAL,
		//LLRenderPass::PASS_MATERIAL_ALPHA,
		LLRenderPass::PASS_MATERIAL_ALPHA_MASK,
		LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE,
		LLRenderPass::PASS_SPECMAP,
		//LLRenderPass::PASS_SPECMAP_BLEND,
		LLRenderPass::PASS_SPECMAP_MASK,
		LLRenderPass::PASS_SPECMAP_EMISSIVE,
		LLRenderPass::PASS_NORMMAP,
		//LLRenderPass::PASS_NORMMAP_BLEND,
		LLRenderPass::PASS_NORMMAP_MASK,
		LLRenderPass::PASS_NORMMAP_EMISSIVE,
		LLRenderPass::PASS_NORMSPEC,
		//LLRenderPass::PASS_NORMSPEC_BLEND,
		LLRenderPass::PASS_NORMSPEC_MASK,
		LLRenderPass::PASS_NORMSPEC_EMISSIVE,
	};

	llassert(pass < sizeof(type_list)/sizeof(U32));

	U32 type = type_list[pass];

	U32 mask = mShader->mAttributeMask;

	LLCullResult::drawinfo_iterator begin = gPipeline.beginRenderMap(type);
	LLCullResult::drawinfo_iterator end = gPipeline.endRenderMap(type);
	
	for (LLCullResult::drawinfo_iterator i = begin; i != end; ++i)
	{
		LLDrawInfo& params = **i;
		
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
		
		mShader->setMinimumAlpha(params.mAlphaMaskCutoff);
		mShader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, params.mFullbright ? 1.f : 0.f);

		pushBatch(params, mask, TRUE);
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

	LLGLEnableFunc stencil_test(GL_STENCIL_TEST, params.mSelected, &LLGLCommonFunc::selected_stencil_test);

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

