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
#include "llglcommonfunc.h"

BOOL LLDrawPoolAlpha::sShowDebugAlpha = FALSE;

static BOOL deferred_render = FALSE;

static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_SETUP("Alpha Setup");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_GROUP_LOOP("Alpha Group");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_DEFERRED("Alpha Deferred");

LLDrawPoolAlpha::LLDrawPoolAlpha(U32 type) :
		LLRenderPass(type), current_shader(NULL), target_shader(NULL),
		simple_shader(NULL), fullbright_shader(NULL), emissive_shader(NULL),
		mColorSFactor(LLRender::BF_UNDEF), mColorDFactor(LLRender::BF_UNDEF),
		mAlphaSFactor(LLRender::BF_UNDEF), mAlphaDFactor(LLRender::BF_UNDEF)
{

}

LLDrawPoolAlpha::~LLDrawPoolAlpha()
{
}


void LLDrawPoolAlpha::prerender()
{
	mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);

    // TODO: is this even necessay?  These are probably set to never discard
    LLViewerFetchedTexture::sFlatNormalImagep->addTextureStats(1024.f*1024.f);
    LLViewerFetchedTexture::sWhiteImagep->addTextureStats(1024.f * 1024.f);
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
    LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_DEFERRED);

    F32 gamma = gSavedSettings.getF32("RenderDeferredDisplayGamma");

    emissive_shader = (LLPipeline::sRenderDeferred)   ? &gDeferredEmissiveProgram    :
                      (LLPipeline::sUnderWaterRender) ? &gObjectEmissiveWaterProgram : &gObjectEmissiveProgram;

    emissive_shader->bind();
    emissive_shader->uniform1i(LLShaderMgr::NO_ATMO, (LLPipeline::sRenderingHUDs) ? 1 : 0);
    emissive_shader->uniform1f(LLShaderMgr::TEXTURE_GAMMA, 2.2f); 
	emissive_shader->uniform1f(LLShaderMgr::DISPLAY_GAMMA, (gamma > 0.1f) ? 1.0f / gamma : (1.0f/2.2f));

	if (pass == 0)
	{
        fullbright_shader = (LLPipeline::sImpostorRender)   ? &gDeferredFullbrightProgram      :
                            (LLPipeline::sUnderWaterRender) ? &gDeferredFullbrightWaterProgram : &gDeferredFullbrightProgram;

		fullbright_shader->bind();
		fullbright_shader->uniform1f(LLShaderMgr::TEXTURE_GAMMA, 2.2f); 
		fullbright_shader->uniform1f(LLShaderMgr::DISPLAY_GAMMA, (gamma > 0.1f) ? 1.0f / gamma : (1.0f/2.2f));
        fullbright_shader->uniform1i(LLShaderMgr::NO_ATMO, LLPipeline::sRenderingHUDs ? 1 : 0);
		fullbright_shader->unbind();

        simple_shader = (LLPipeline::sImpostorRender)   ? &gDeferredAlphaImpostorProgram :
                        (LLPipeline::sUnderWaterRender) ? &gDeferredAlphaWaterProgram    : &gDeferredAlphaProgram;

		//prime simple shader (loads shadow relevant uniforms)
		gPipeline.bindDeferredShader(*simple_shader);

		simple_shader->uniform1f(LLShaderMgr::DISPLAY_GAMMA, (gamma > 0.1f) ? 1.0f / gamma : (1.0f/2.2f));
        simple_shader->uniform1i(LLShaderMgr::NO_ATMO, LLPipeline::sRenderingHUDs ? 1 : 0);
	}
	else if (!LLPipeline::sImpostorRender)
	{
		//update depth buffer sampler
		gPipeline.mScreen.flush();
		gPipeline.mDeferredDepth.copyContents(gPipeline.mDeferredScreen, 0, 0, gPipeline.mDeferredScreen.getWidth(), gPipeline.mDeferredScreen.getHeight(),
							0, 0, gPipeline.mDeferredDepth.getWidth(), gPipeline.mDeferredDepth.getHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);	
		gPipeline.mDeferredDepth.bindTarget();
		simple_shader = fullbright_shader = &gObjectFullbrightAlphaMaskProgram;
		gObjectFullbrightAlphaMaskProgram.bind();
		gObjectFullbrightAlphaMaskProgram.setMinimumAlpha(0.33f);
	}

	deferred_render = TRUE;
	if (mShaderLevel > 0)
	{
		// Start out with no shaders.
		current_shader = target_shader = NULL;
	}
	gPipeline.enableLightsDynamic();
}

void LLDrawPoolAlpha::endPostDeferredPass(S32 pass) 
{ 
    LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_DEFERRED);

	if (pass == 1 && !LLPipeline::sImpostorRender)
	{
		gPipeline.mDeferredDepth.flush();
		gPipeline.mScreen.bindTarget();
		gObjectFullbrightAlphaMaskProgram.unbind();
	}

	deferred_render = FALSE;
	endRenderPass(pass);
}

void LLDrawPoolAlpha::renderPostDeferred(S32 pass) 
{ 
    LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_DEFERRED);
	render(pass); 
}

void LLDrawPoolAlpha::beginRenderPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_SETUP);
	
    simple_shader     = (LLPipeline::sImpostorRender)   ? &gObjectSimpleImpostorProgram  :
                        (LLPipeline::sUnderWaterRender) ? &gObjectSimpleWaterProgram     : &gObjectSimpleProgram;

    fullbright_shader = (LLPipeline::sImpostorRender)   ? &gObjectFullbrightProgram      :
                        (LLPipeline::sUnderWaterRender) ? &gObjectFullbrightWaterProgram : &gObjectFullbrightProgram;

    emissive_shader   = (LLPipeline::sImpostorRender)   ? &gObjectEmissiveProgram        :
                        (LLPipeline::sUnderWaterRender) ? &gObjectEmissiveWaterProgram   : &gObjectEmissiveProgram;

    if (LLPipeline::sImpostorRender)
	{
        if (mShaderLevel > 0)
		{
            fullbright_shader->bind();
			fullbright_shader->setMinimumAlpha(0.5f);
            fullbright_shader->uniform1i(LLShaderMgr::NO_ATMO, LLPipeline::sRenderingHUDs ? 1 : 0);
			simple_shader->bind();
			simple_shader->setMinimumAlpha(0.5f);
            simple_shader->uniform1i(LLShaderMgr::NO_ATMO, LLPipeline::sRenderingHUDs ? 1 : 0);
        }
        else
        {
            gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f); //OK
        }
	}
    else
	{
        if (mShaderLevel > 0)
	    {
			fullbright_shader->bind();
			fullbright_shader->setMinimumAlpha(0.f);
            fullbright_shader->uniform1i(LLShaderMgr::NO_ATMO, LLPipeline::sRenderingHUDs ? 1 : 0);
			simple_shader->bind();
			simple_shader->setMinimumAlpha(0.f);
            simple_shader->uniform1i(LLShaderMgr::NO_ATMO, LLPipeline::sRenderingHUDs ? 1 : 0);
		}
        else
        {
            gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT); //OK
        }
    }
	gPipeline.enableLightsDynamic();

    LLGLSLShader::bindNoShader();
	current_shader = NULL;
}

void LLDrawPoolAlpha::endRenderPass( S32 pass )
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_SETUP);
	LLRenderPass::endRenderPass(pass);

	if(gPipeline.canUseWindLightShaders()) 
	{
		LLGLSLShader::bindNoShader();
	}
}

void LLDrawPoolAlpha::render(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA);

	LLGLSPipelineAlpha gls_pipeline_alpha;

	if (deferred_render && pass == 1)
	{ //depth only
		gGL.setColorMask(false, false);
	}
	else
	{
		gGL.setColorMask(true, true);
	}
	
	bool write_depth = LLDrawPoolWater::sSkipScreenCopy
						 || (deferred_render && pass == 1)
						 // we want depth written so that rendered alpha will
						 // contribute to the alpha mask used for impostors
						 || LLPipeline::sImpostorRenderAlphaDepthPass;

	LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);

	if (deferred_render && pass == 1)
	{
		gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);
	}
	else
	{
		mColorSFactor = LLRender::BF_SOURCE_ALPHA;           // } regular alpha blend
		mColorDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA; // }
		mAlphaSFactor = LLRender::BF_ZERO;                         // } glow suppression
		mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;       // }
		gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);
	}

	if (mShaderLevel > 0)
	{
		renderAlpha(getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2, pass);
	}
	else
	{
		renderAlpha(getVertexDataMask(), pass);
	}

	gGL.setColorMask(true, false);

	if (deferred_render && pass == 1)
	{
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
	}

	if (sShowDebugAlpha)
	{
		BOOL shaders = gPipeline.canUseVertexShaders();
		if(shaders) 
		{
			gHighlightProgram.bind();
		}
		else
		{
			gPipeline.enableLightsFullbright();
		}

		gGL.diffuseColor4f(1,0,0,1);
				
		LLViewerFetchedTexture::sSmokeImagep->addTextureStats(1024.f*1024.f);
		gGL.getTexUnit(0)->bindFast(LLViewerFetchedTexture::sSmokeImagep);
		renderAlphaHighlight(LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0);

		pushBatches(LLRenderPass::PASS_ALPHA_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
		pushBatches(LLRenderPass::PASS_ALPHA_INVISIBLE, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);

		// Material alpha mask
		gGL.diffuseColor4f(0, 0, 1, 1);
		pushBatches(LLRenderPass::PASS_MATERIAL_ALPHA_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
		pushBatches(LLRenderPass::PASS_NORMMAP_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
		pushBatches(LLRenderPass::PASS_SPECMAP_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
		pushBatches(LLRenderPass::PASS_NORMSPEC_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
		pushBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);

		gGL.diffuseColor4f(0, 1, 0, 1);
		pushBatches(LLRenderPass::PASS_INVISIBLE, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);

		if(shaders) 
		{
			gHighlightProgram.unbind();
		}
	}
}

void LLDrawPoolAlpha::renderAlphaHighlight(U32 mask)
{
	for (LLCullResult::sg_iterator i = gPipeline.beginAlphaGroups(); i != gPipeline.endAlphaGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (group->getSpatialPartition()->mRenderByGroup &&
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
				params.mVertexBuffer->setBufferFast(mask);
				params.mVertexBuffer->drawRangeFast(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
			}
		}
	}
}

inline bool IsFullbright(LLDrawInfo& params)
{
    return params.mFullbright;
}

inline bool IsMaterial(LLDrawInfo& params)
{
    return params.mMaterial != nullptr;
}

inline bool IsEmissive(LLDrawInfo& params)
{
    return params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE);
}

inline void Draw(LLDrawInfo* draw, U32 mask)
{
    draw->mVertexBuffer->setBufferFast(mask);
    LLRenderPass::applyModelMatrix(*draw);
	draw->mVertexBuffer->drawRangeFast(draw->mDrawMode, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);                    
}

bool LLDrawPoolAlpha::TexSetup(LLDrawInfo* draw, bool use_material, LLGLSLShader* current_shader)
{
    bool tex_setup = false;

    if (deferred_render && use_material && current_shader)
    {
        if (draw->mNormalMap)
		{
			draw->mNormalMap->addTextureStats(draw->mVSize);
			current_shader->bindTexture(LLShaderMgr::BUMP_MAP, draw->mNormalMap);
		} 

		if (draw->mSpecularMap)
		{
			draw->mSpecularMap->addTextureStats(draw->mVSize);
			current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, draw->mSpecularMap);
		} 
    }
    else if (current_shader == simple_shader)
    {
        current_shader->bindTexture(LLShaderMgr::BUMP_MAP, LLViewerFetchedTexture::sFlatNormalImagep);
	    current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, LLViewerFetchedTexture::sWhiteImagep);
    }
	if (draw->mTextureList.size() > 1)
	{
		for (U32 i = 0; i < draw->mTextureList.size(); ++i)
		{
			if (draw->mTextureList[i].notNull())
			{
				gGL.getTexUnit(i)->bindFast(draw->mTextureList[i]);
			}
		}
	}
	else
	{ //not batching textures or batch has only 1 texture -- might need a texture matrix
		if (draw->mTexture.notNull())
		{
			if (use_material)
			{
				current_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, draw->mTexture);
			}
			else
			{
			    gGL.getTexUnit(0)->bindFast(draw->mTexture);
			}

			if (draw->mTextureMatrix)
			{
				tex_setup = true;
				gGL.getTexUnit(0)->activate();
				gGL.matrixMode(LLRender::MM_TEXTURE);
				gGL.loadMatrix((GLfloat*) draw->mTextureMatrix->mMatrix);
				gPipeline.mTextureMatrixOps++;
			}
		}
		else
		{
			gGL.getTexUnit(0)->unbindFast(LLTexUnit::TT_TEXTURE);
		}
	}
    
    return tex_setup;
}

void LLDrawPoolAlpha::RestoreTexSetup(bool tex_setup)
{
    if (tex_setup)
	{
		gGL.getTexUnit(0)->activate();
        gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
}

void LLDrawPoolAlpha::renderFullbrights(U32 mask, std::vector<LLDrawInfo*>& fullbrights)
{
    gPipeline.enableLightsFullbright();
    fullbright_shader->bind();
    fullbright_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.0f);
    
    for (LLDrawInfo* draw : fullbrights)
    {
        bool tex_setup = TexSetup(draw, false, fullbright_shader);

        LLGLEnableFunc stencil_test(GL_STENCIL_TEST, draw->mSelected, &LLGLCommonFunc::selected_stencil_test);
		gGL.blendFunc((LLRender::eBlendFactor) draw->mBlendFuncSrc, (LLRender::eBlendFactor) draw->mBlendFuncDst, mAlphaSFactor, mAlphaDFactor);

        Draw(draw, mask & ~(LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2));
        RestoreTexSetup(tex_setup);
    }
    fullbright_shader->unbind();
}

void LLDrawPoolAlpha::drawEmissive(U32 mask, LLDrawInfo* draw)
{
    draw->mVertexBuffer->setBufferFast((mask & ~LLVertexBuffer::MAP_COLOR) | LLVertexBuffer::MAP_EMISSIVE);
	draw->mVertexBuffer->drawRangeFast(draw->mDrawMode, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
}

void LLDrawPoolAlpha::drawEmissiveInline(U32 mask, LLDrawInfo* draw)
{
    // install glow-accumulating blend mode
    gGL.blendFunc(
            LLRender::BF_ZERO, LLRender::BF_ONE, // don't touch color
			LLRender::BF_ONE, LLRender::BF_ONE); // add to alpha (glow)

	emissive_shader->bind();
					
	drawEmissive(mask, draw);

	// restore our alpha blend mode
	gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

    current_shader->bind();
}

void LLDrawPoolAlpha::renderEmissives(U32 mask, std::vector<LLDrawInfo*>& emissives)
{
    emissive_shader->bind();
    emissive_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);

    gPipeline.enableLightsDynamic();

    // install glow-accumulating blend mode
    // don't touch color, add to alpha (glow)
	gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE); 
 
    for (LLDrawInfo* draw : emissives)
    {
        bool tex_setup = TexSetup(draw, false, emissive_shader);
        drawEmissive(mask, draw);
        RestoreTexSetup(tex_setup);
    }

    // restore our alpha blend mode
	gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

    emissive_shader->unbind();
}

void LLDrawPoolAlpha::renderAlpha(U32 mask, S32 pass)
{
    BOOL batch_fullbrights = gSavedSettings.getBOOL("RenderAlphaBatchFullbrights");
    BOOL batch_emissives   = gSavedSettings.getBOOL("RenderAlphaBatchEmissives");
	BOOL initialized_lighting = FALSE;
	BOOL light_enabled = TRUE;
	
	for (LLCullResult::sg_iterator i = gPipeline.beginAlphaGroups(); i != gPipeline.endAlphaGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		llassert(group);
		llassert(group->getSpatialPartition());

		if (group->getSpatialPartition()->mRenderByGroup &&
		    !group->isDead())
		{
            static std::vector<LLDrawInfo*> emissives;
            static std::vector<LLDrawInfo*> fullbrights;
            emissives.resize(0);
            fullbrights.resize(0);

			bool is_particle_or_hud_particle = group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_PARTICLE
													  || group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_HUD_PARTICLE;

			bool draw_glow_for_this_partition = mShaderLevel > 0; // no shaders = no glow.

			
			LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_GROUP_LOOP);

			bool disable_cull = is_particle_or_hud_particle;
			LLGLDisable cull(disable_cull ? GL_CULL_FACE : 0);

			LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA];

			for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)	
			{
                LL_PROFILE_ZONE_NAMED("ra - push batch")
				LLDrawInfo& params = **k;
                U32 have_mask = params.mVertexBuffer->getTypeMask() & mask;
				if (have_mask != mask)
				{ //FIXME!
					LL_WARNS_ONCE() << "Missing required components, expected mask: " << mask
									<< " present: " << have_mask
									<< ". Skipping render batch." << LL_ENDL;
					continue;
				}

				// Fix for bug - NORSPEC-271
				// If the face is more than 90% transparent, then don't update the Depth buffer for Dof
				// We don't want the nearly invisible objects to cause of DoF effects
				if(pass == 1 && !LLPipeline::sImpostorRender)
				{
					LLFace*	face = params.mFace;
					if(face)
					{
						const LLTextureEntry* tep = face->getTextureEntry();
						if(tep)
						{
							if(tep->getColor().mV[3] < 0.1f)
								continue;
						}
					}
				}

                if (params.mFullbright && batch_fullbrights)
				{
                    fullbrights.push_back(&params);
                    continue;
				}

				LLRenderPass::applyModelMatrix(params);

				LLMaterial* mat = NULL;

				if (deferred_render)
				{
					mat = params.mMaterial;
				}
				
				if (params.mFullbright)
				{
					// Turn off lighting if it hasn't already been so.
					if (light_enabled || !initialized_lighting)
					{
						initialized_lighting = TRUE;
						target_shader = fullbright_shader;

						light_enabled = FALSE;
					}
				}
				// Turn on lighting if it isn't already.
				else if (!light_enabled || !initialized_lighting)
				{
					initialized_lighting = TRUE;
					target_shader = simple_shader;
					light_enabled = TRUE;
				}

				if (deferred_render && mat)
				{
					U32 mask = params.mShaderMask;

					llassert(mask < LLMaterial::SHADER_COUNT);
					target_shader = &(gDeferredMaterialProgram[mask]);

					if (LLPipeline::sUnderWaterRender)
					{
						target_shader = &(gDeferredMaterialWaterProgram[mask]);
					}

					if (current_shader != target_shader)
					{
						gPipeline.bindDeferredShader(*target_shader);
                        current_shader = target_shader;
					}
				}
				else if (!params.mFullbright)
				{
					target_shader = simple_shader;
				}
				else
				{
					target_shader = fullbright_shader;
				}
				
				if(current_shader != target_shader)
				{// If we need shaders, and we're not ALREADY using the proper shader, then bind it
				// (this way we won't rebind shaders unnecessarily).
					current_shader = target_shader;
					current_shader->bind();
				}

                LLVector4 spec_color(1, 1, 1, 1);
                F32 env_intensity = 0.0f;
                F32 brightness = 1.0f;

                // We have a material.  Supply the appropriate data here.
				if (mat && deferred_render)
				{
					spec_color    = params.mSpecColor;
                    env_intensity = params.mEnvIntensity;
                    brightness    = params.mFullbright ? 1.f : 0.f;
                }

                if (current_shader)
                {
                    current_shader->uniform4f(LLShaderMgr::SPECULAR_COLOR, spec_color.mV[0], spec_color.mV[1], spec_color.mV[2], spec_color.mV[3]);						
				    current_shader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, env_intensity);
					current_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, brightness);
                }

				if (params.mGroup)
				{
					params.mGroup->rebuildMesh();
				}

                bool tex_setup = TexSetup(&params, (mat != nullptr), current_shader);

				{
					LLGLEnableFunc stencil_test(GL_STENCIL_TEST, params.mSelected, &LLGLCommonFunc::selected_stencil_test);

					gGL.blendFunc((LLRender::eBlendFactor) params.mBlendFuncSrc, (LLRender::eBlendFactor) params.mBlendFuncDst, mAlphaSFactor, mAlphaDFactor);
					params.mVertexBuffer->setBufferFast(mask & ~(params.mFullbright ? (LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2) : 0));

                    {
					    params.mVertexBuffer->drawRangeFast(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
                    }
				}

				// If this alpha mesh has glow, then draw it a second time to add the destination-alpha (=glow).  Interleaving these state-changing calls is expensive, but glow must be drawn Z-sorted with alpha.
				if (current_shader && 
					draw_glow_for_this_partition &&
					params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE))
				{
                    if (batch_emissives)
                    {
                        emissives.push_back(&params);
                    }
                    else
                    {
                        drawEmissiveInline(mask, &params);
                    } 
				}
			
				if (tex_setup)
				{
					gGL.getTexUnit(0)->activate();
                    gGL.matrixMode(LLRender::MM_TEXTURE);
					gGL.loadIdentity();
					gGL.matrixMode(LLRender::MM_MODELVIEW);
				}
			}


            bool rebind = false;
            if (batch_fullbrights)
            {
                if (!fullbrights.empty())
                {
                    light_enabled = false;
                    renderFullbrights(mask, fullbrights);
                    rebind = true;
                }
            }

            if (batch_emissives)
            {
                if (!emissives.empty())
                {
                    light_enabled = true;
                    renderEmissives(mask, emissives);
                    rebind = true;
                }
            }

            if (current_shader && rebind)
            {
                current_shader->bind();
            }
		}        
	}

	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	LLVertexBuffer::unbind();	
		
	if (!light_enabled)
	{
		gPipeline.enableLightsDynamic();
	}
}
