/** 
 * @file lldrawpoolalpha.cpp
 * @brief LLDrawPoolAlpha class implementation
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
	return 1; 
}

void LLDrawPoolAlpha::beginPostDeferredPass(S32 pass) 
{ 
	LLFastTimer t(FTM_RENDER_ALPHA);

	simple_shader = &gDeferredAlphaProgram;
	fullbright_shader = &gDeferredFullbrightProgram;
	
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

	gGL.setColorMask(true, true);

	if (LLPipeline::sFastAlpha && !deferred_render)
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
				pushBatches(LLRenderPass::PASS_ALPHA_MASK, getVertexDataMask());
			}
			fullbright_shader->bind();
			pushBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, getVertexDataMask());
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

	LLGLDepthTest depth(GL_TRUE, LLDrawPoolWater::sSkipScreenCopy ? GL_TRUE : GL_FALSE);

	mColorSFactor = LLRender::BF_SOURCE_ALPHA;           // } regular alpha blend
	mColorDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA; // }
	mAlphaSFactor = LLRender::BF_ZERO;                         // } glow suppression
	mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;       // }
	gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

	renderAlpha(getVertexDataMask());

	gGL.setColorMask(true, false);

	if (deferred_render && current_shader != NULL)
	{
		gPipeline.unbindDeferredShader(*current_shader);
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
	S32 diffuse_channel = 0;

	//BOOL is_particle = FALSE;
	BOOL use_shaders = (LLPipeline::sUnderWaterRender && gPipeline.canUseVertexShaders())
		|| gPipeline.canUseWindLightShadersOnObjects();
	
	// check to see if it's a particle and if it's "close"
	{
		if (LLPipeline::sImpostorRender)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
		}
		else
		{
			gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
		}
	}

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

				{
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
						if (deferred_render && current_shader != NULL)
						{
							gPipeline.unbindDeferredShader(*current_shader);
							diffuse_channel = 0;
						}
						current_shader = target_shader;
						if (deferred_render)
						{
							gPipeline.bindDeferredShader(*current_shader);
							diffuse_channel = current_shader->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
						}
						else
						{
							current_shader->bind();
						}
					}
					else if (!use_shaders && current_shader != NULL)
					{
						if (deferred_render)
						{
							gPipeline.unbindDeferredShader(*current_shader);
							diffuse_channel = 0;
						}
						LLGLSLShader::bindNoShader();
						current_shader = NULL;
					}

					if (params.mGroup)
					{
						params.mGroup->rebuildMesh();
					}

					
					if (params.mTexture.notNull())
					{
						gGL.getTexUnit(diffuse_channel)->bind(params.mTexture.get());
						if(params.mTexture.notNull())
						{
							params.mTexture->addTextureStats(params.mVSize);
						}
						if (params.mTextureMatrix)
						{
							gGL.getTexUnit(0)->activate();
							glMatrixMode(GL_TEXTURE);
							glLoadMatrixf((GLfloat*) params.mTextureMatrix->mMatrix);
							gPipeline.mTextureMatrixOps++;
						}
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
			
				if (params.mTextureMatrix && params.mTexture.notNull())
				{
					gGL.getTexUnit(0)->activate();
					glLoadIdentity();
					glMatrixMode(GL_MODELVIEW);
				}
			}
		}
	}

	if (deferred_render && current_shader != NULL)
	{
		gPipeline.unbindDeferredShader(*current_shader);
		LLVertexBuffer::unbind();	
		LLGLState::checkStates();
		LLGLState::checkTextureChannels();
		LLGLState::checkClientArrays();
	}
	
	if (!light_enabled)
	{
		gPipeline.enableLightsDynamic();
	}
}
