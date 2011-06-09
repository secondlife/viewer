/** 
 * @file lldrawpoolalpha.cpp
 * @brief LLDrawPoolAlpha class implementation
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

#include "lldrawpoolalpha.h"

#include "llglheaders.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "llfasttimer.h"
#include "llrender.h"

#include "llcubemap.h"
#include "llsky.h"
#include "lldrawable.h"
#include "llface.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"	// For debugging
#include "llviewerobjectlist.h" // For debugging
#include "llviewerwindow.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llviewerregion.h"
#include "lldrawpoolwater.h"
#include "llspatialpartition.h"

BOOL LLDrawPoolAlpha::sShowDebugAlpha = FALSE;

static BOOL deferred_render = FALSE;

LLDrawPoolAlpha::LLDrawPoolAlpha(U32 type) :
		LLRenderPass(type), current_shader(NULL), target_shader(NULL),
		simple_shader(NULL), fullbright_shader(NULL),
		mColorSFactor(LLRender::BF_UNDEF), mColorDFactor(LLRender::BF_UNDEF),
		mAlphaSFactor(LLRender::BF_UNDEF), mAlphaDFactor(LLRender::BF_UNDEF)
{

}

LLDrawPoolAlpha::~LLDrawPoolAlpha()
{
}


void LLDrawPoolAlpha::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

S32 LLDrawPoolAlpha::getNumDeferredPasses()
{
	return 1;
}

void LLDrawPoolAlpha::beginDeferredPass(S32 pass)
{
	
}

void LLDrawPoolAlpha::endDeferredPass(S32 pass)
{
	
}

void LLDrawPoolAlpha::renderDeferred(S32 pass)
{
	gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.f);
	{
		LLFastTimer t(FTM_RENDER_GRASS);
		gDeferredTreeProgram.bind();
		LLGLEnable test(GL_ALPHA_TEST);
		//render alpha masked objects
		LLRenderPass::renderTexture(LLRenderPass::PASS_ALPHA_MASK, getVertexDataMask());
		gDeferredTreeProgram.unbind();
	}			
	gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
}


S32 LLDrawPoolAlpha::getNumPostDeferredPasses() 
{ 
	if (LLPipeline::sImpostorRender)
	{ //skip depth buffer filling pass when rendering impostors
		return 1;
	}
	else if (gSavedSettings.getBOOL("RenderDepthOfField"))
	{
		return 2; 
	}
	else
	{
		return 1;
	}
}

void LLDrawPoolAlpha::beginPostDeferredPass(S32 pass) 
{ 
	LLFastTimer t(FTM_RENDER_ALPHA);

	if (pass == 0)
	{
		simple_shader = &gDeferredAlphaProgram;
		fullbright_shader = &gObjectFullbrightProgram;

		//prime simple shader (loads shadow relevant uniforms)
		gPipeline.bindDeferredShader(*simple_shader);
	}
	else
	{
		//update depth buffer sampler
		gPipeline.mScreen.flush();
		gPipeline.mDeferredDepth.copyContents(gPipeline.mDeferredScreen, 0, 0, gPipeline.mDeferredScreen.getWidth(), gPipeline.mDeferredScreen.getHeight(),
							0, 0, gPipeline.mDeferredDepth.getWidth(), gPipeline.mDeferredDepth.getHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);	
		gPipeline.mDeferredDepth.bindTarget();
		simple_shader = NULL;
		fullbright_shader = NULL;
	}

	deferred_render = TRUE;
	if (mVertexShaderLevel > 0)
	{
		// Start out with no shaders.
		current_shader = target_shader = NULL;
	}
	gPipeline.enableLightsDynamic();
}

void LLDrawPoolAlpha::endPostDeferredPass(S32 pass) 
{ 

	if (pass == 1)
	{
		gPipeline.mDeferredDepth.flush();
		gPipeline.mScreen.bindTarget();
	}

	deferred_render = FALSE;
	endRenderPass(pass);
}

void LLDrawPoolAlpha::renderPostDeferred(S32 pass) 
{ 
	render(pass); 
}

void LLDrawPoolAlpha::beginRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_ALPHA);
	
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
		// Start out with no shaders.
		current_shader = target_shader = NULL;
		LLGLSLShader::bindNoShader();
	}
	gPipeline.enableLightsDynamic();
}

void LLDrawPoolAlpha::endRenderPass( S32 pass )
{
	LLFastTimer t(FTM_RENDER_ALPHA);
	LLRenderPass::endRenderPass(pass);

	if(gPipeline.canUseWindLightShaders()) 
	{
		LLGLSLShader::bindNoShader();
	}
}

void LLDrawPoolAlpha::render(S32 pass)
{
	LLFastTimer t(FTM_RENDER_ALPHA);

	LLGLSPipelineAlpha gls_pipeline_alpha;

	if (deferred_render && pass == 1)
	{ //depth only
		gGL.setColorMask(false, false);
	}
	else
	{
		gGL.setColorMask(true, true);
	}

	if (LLPipeline::sAutoMaskAlphaNonDeferred)
	{
		mColorSFactor = LLRender::BF_ONE;  // }
		mColorDFactor = LLRender::BF_ZERO; // } these are like disabling blend on the color channels, but we're still blending on the alpha channel so that we can suppress glow
		mAlphaSFactor = LLRender::BF_ZERO;
		mAlphaDFactor = LLRender::BF_ZERO; // block (zero-out) glow where the alpha test succeeds
		gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

		gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.33f);
		if (mVertexShaderLevel > 0)
		{
			if (!LLPipeline::sRenderDeferred)
			{
				simple_shader->bind();
				pushBatches(LLRenderPass::PASS_ALPHA_MASK, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
			}
			if (fullbright_shader)
			{
				fullbright_shader->bind();
			}
			pushBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
			LLGLSLShader::bindNoShader();
		}
		else
		{
			gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
			pushBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, getVertexDataMask());
			gPipeline.enableLightsDynamic();
			pushBatches(LLRenderPass::PASS_ALPHA_MASK, getVertexDataMask());
		}
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	}

	LLGLDepthTest depth(GL_TRUE, LLDrawPoolWater::sSkipScreenCopy || 
				(deferred_render && pass == 1) ? GL_TRUE : GL_FALSE);

	if (deferred_render && pass == 1)
	{
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.33f);
		gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);
	}
	else
	{
		mColorSFactor = LLRender::BF_SOURCE_ALPHA;           // } regular alpha blend
		mColorDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA; // }
		mAlphaSFactor = LLRender::BF_ZERO;                         // } glow suppression
		mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;       // }
		gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

		if (LLPipeline::sImpostorRender)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
		}
		else
		{
			gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
		}
	}

	if (mVertexShaderLevel > 0)
	{
		renderAlpha(getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX);
	}
	else
	{
		renderAlpha(getVertexDataMask());
	}

	gGL.setColorMask(true, false);

	if (deferred_render && pass == 1)
	{
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
	}

	if (sShowDebugAlpha)
	{
		if(gPipeline.canUseWindLightShaders()) 
		{
			LLGLSLShader::bindNoShader();
		}
		gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
		glColor4f(1,0,0,1);
		LLViewerFetchedTexture::sSmokeImagep->addTextureStats(1024.f*1024.f);
		gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sSmokeImagep, TRUE) ;
		renderAlphaHighlight(LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0);
	}
}

void LLDrawPoolAlpha::renderAlphaHighlight(U32 mask)
{
	for (LLCullResult::sg_list_t::iterator i = gPipeline.beginAlphaGroups(); i != gPipeline.endAlphaGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (group->mSpatialPartition->mRenderByGroup &&
			!group->isDead())
		{
			LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA];	

			for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)	
			{
				LLDrawInfo& params = **k;
				
				if (params.mParticle)
				{
					continue;
				}

				LLRenderPass::applyModelMatrix(params);
				if (params.mGroup)
				{
					params.mGroup->rebuildMesh();
				}
				params.mVertexBuffer->setBuffer(mask);
				params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
				gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
			}
		}
	}
}

void LLDrawPoolAlpha::renderAlpha(U32 mask)
{
	BOOL initialized_lighting = FALSE;
	BOOL light_enabled = TRUE;
	
	BOOL use_shaders = gPipeline.canUseVertexShaders();
		
	for (LLCullResult::sg_list_t::iterator i = gPipeline.beginAlphaGroups(); i != gPipeline.endAlphaGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		llassert(group);
		llassert(group->mSpatialPartition);

		if (group->mSpatialPartition->mRenderByGroup &&
		    !group->isDead())
		{
			bool draw_glow_for_this_partition = mVertexShaderLevel > 0 && // no shaders = no glow.
				// All particle systems seem to come off the wire with texture entries which claim that they glow.  This is probably a bug in the data.  Suppress.
				group->mSpatialPartition->mPartitionType != LLViewerRegion::PARTITION_PARTICLE &&
				group->mSpatialPartition->mPartitionType != LLViewerRegion::PARTITION_CLOUD &&
				group->mSpatialPartition->mPartitionType != LLViewerRegion::PARTITION_HUD_PARTICLE;

			LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA];

			for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)	
			{
				LLDrawInfo& params = **k;

				LLRenderPass::applyModelMatrix(params);

				
				if (params.mFullbright)
				{
					// Turn off lighting if it hasn't already been so.
					if (light_enabled || !initialized_lighting)
					{
						initialized_lighting = TRUE;
						if (use_shaders) 
						{
							target_shader = fullbright_shader;
						}
						else
						{
							gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
						}
						light_enabled = FALSE;
					}
				}
				// Turn on lighting if it isn't already.
				else if (!light_enabled || !initialized_lighting)
				{
					initialized_lighting = TRUE;
					if (use_shaders) 
					{
						target_shader = simple_shader;
					}
					else
					{
						gPipeline.enableLightsDynamic();
					}
					light_enabled = TRUE;
				}

				// If we need shaders, and we're not ALREADY using the proper shader, then bind it
				// (this way we won't rebind shaders unnecessarily).
				if(use_shaders && (current_shader != target_shader))
				{
					llassert(target_shader != NULL);
					current_shader = target_shader;
					current_shader->bind();
				}
				else if (!use_shaders && current_shader != NULL)
				{
					LLGLSLShader::bindNoShader();
					current_shader = NULL;
				}

				if (params.mGroup)
				{
					params.mGroup->rebuildMesh();
				}

				bool tex_setup = false;

				if (use_shaders && params.mTextureList.size() > 1)
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
					if (params.mTexture.notNull())
					{
						params.mTexture->addTextureStats(params.mVSize);
						gGL.getTexUnit(0)->bind(params.mTexture, TRUE) ;
						if (params.mTextureMatrix)
						{
							tex_setup = true;
							gGL.getTexUnit(0)->activate();
							glMatrixMode(GL_TEXTURE);
							glLoadMatrixf((GLfloat*) params.mTextureMatrix->mMatrix);
							gPipeline.mTextureMatrixOps++;
						}
					}
					else
					{
						gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
					}
				}

				params.mVertexBuffer->setBuffer(mask);
				params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
				gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
				
				// If this alpha mesh has glow, then draw it a second time to add the destination-alpha (=glow).  Interleaving these state-changing calls could be expensive, but glow must be drawn Z-sorted with alpha.
				if (draw_glow_for_this_partition &&
				    params.mGlowColor.mV[3] > 0)
				{
					// install glow-accumulating blend mode
					gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE, // don't touch color
						      LLRender::BF_ONE, LLRender::BF_ONE); // add to alpha (glow)

					// glow doesn't use vertex colors from the mesh data
					params.mVertexBuffer->setBuffer(mask & ~LLVertexBuffer::MAP_COLOR);
					glColor4ubv(params.mGlowColor.mV);

					// do the actual drawing, again
					params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
					gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);

					// restore our alpha blend mode
					gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);
				}
			
				if (tex_setup)
				{
					gGL.getTexUnit(0)->activate();
					glLoadIdentity();
					glMatrixMode(GL_MODELVIEW);
				}
			}
		}
	}

	LLVertexBuffer::unbind();	
		
	if (!light_enabled)
	{
		gPipeline.enableLightsDynamic();
	}
}
