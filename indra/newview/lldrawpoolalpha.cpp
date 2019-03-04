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
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_PUSH("Alpha Push Verts");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_DEFERRED("Alpha Deferred");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_SETBUFFER("Alpha SetBuffer");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_DRAW("Alpha Draw");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_TEX_BINDS("Alpha Tex Binds");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_MATS("Alpha Mat Tex Binds");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_GLOW("Alpha Glow Binds");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_SHADER_BINDS("Alpha Shader Binds");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_DEFERRED_SHADER_BINDS("Alpha Def Binds");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_DEFERRED_TEX_BINDS("Alpha Def Tex Binds");
static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_MESH_REBUILD("Alpha Mesh Rebuild");

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
			gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
		}

		gGL.diffuseColor4f(1,0,0,1);
				
		LLViewerFetchedTexture::sSmokeImagep->addTextureStats(1024.f*1024.f);
		gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sSmokeImagep, TRUE) ;
		renderAlphaHighlight(LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0);

		pushBatches(LLRenderPass::PASS_ALPHA_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
		pushBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
		pushBatches(LLRenderPass::PASS_ALPHA_INVISIBLE, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);

		gGL.diffuseColor4f(0, 0, 1, 1);
		pushBatches(LLRenderPass::PASS_MATERIAL_ALPHA_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);

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
				params.mVertexBuffer->setBuffer(mask);
				params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
				gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
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
    draw->mVertexBuffer->setBuffer(mask);
    LLRenderPass::applyModelMatrix(*draw);
	draw->mVertexBuffer->drawRange(draw->mDrawMode, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);                    
    gPipeline.addTrianglesDrawn(draw->mCount, draw->mDrawMode);
}

inline bool TexSetup(LLDrawInfo* draw)
{
    bool tex_setup = false;

    if (draw->mTextureMatrix)
	{
		tex_setup = true;
		gGL.getTexUnit(0)->activate();
		gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.loadMatrix((GLfloat*) draw->mTextureMatrix->mMatrix);
		gPipeline.mTextureMatrixOps++;
	}

    LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_TEX_BINDS);    
	for (U32 i = 0; i < draw->mTextureList.size(); ++i)
	{
        if (draw->mTextureList[i].notNull())
        {
            gGL.getTexUnit(i)->bind(draw->mTextureList[i], TRUE);
        }
	}

    return tex_setup;
}

inline void RestoreTexSetup(bool tex_setup)
{
    if (tex_setup)
	{
		gGL.getTexUnit(0)->activate();
        gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
}

void LLDrawPoolAlpha::renderSimples(U32 mask, std::vector<LLDrawInfo*>& simples)
{
    gPipeline.enableLightsDynamic();
    simple_shader->bind();
	simple_shader->bindTexture(LLShaderMgr::BUMP_MAP, LLViewerFetchedTexture::sFlatNormalImagep);
	simple_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, LLViewerFetchedTexture::sWhiteImagep);
    simple_shader->uniform4f(LLShaderMgr::SPECULAR_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);
	simple_shader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, 0.0f);
    simple_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 0.0f);

    for (LLDrawInfo* draw : simples)
    {
        bool tex_setup = TexSetup(draw);
	    Draw(draw, mask);
        RestoreTexSetup(tex_setup);
    }
    simple_shader->unbind();
}

void LLDrawPoolAlpha::renderFullbrights(U32 mask, std::vector<LLDrawInfo*>& fullbrights)
{
    gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
    fullbright_shader->bind();
    fullbright_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 0.0f);
    for (LLDrawInfo* draw : fullbrights)
    {
        bool tex_setup = TexSetup(draw);
        Draw(draw, mask & ~(LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2));
        RestoreTexSetup(tex_setup);
    }
    fullbright_shader->unbind();
}

void LLDrawPoolAlpha::renderMaterials(U32 mask, std::vector<LLDrawInfo*>& materials)
{
    LLGLSLShader::bindNoShader();
    current_shader = NULL;

    gPipeline.enableLightsDynamic();    
    for (LLDrawInfo* draw : materials)
    {
        U32 mask = draw->mShaderMask;

		llassert(mask < LLMaterial::SHADER_COUNT);
		target_shader = (LLPipeline::sUnderWaterRender) ? &(gDeferredMaterialWaterProgram[mask]) : &(gDeferredMaterialProgram[mask]);

		if (current_shader != target_shader)
		{
            LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_DEFERRED_SHADER_BINDS);
            if (current_shader)
            {
                gPipeline.unbindDeferredShader(*current_shader);
            }
			gPipeline.bindDeferredShader(*target_shader);
            current_shader = target_shader;
		}
        
        bool tex_setup = TexSetup(draw);    
        if (!draw->mTexture.isNull())
        {
            current_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, draw->mTexture);
        }

        current_shader->uniform4f(LLShaderMgr::SPECULAR_COLOR, draw->mSpecColor.mV[0], draw->mSpecColor.mV[1], draw->mSpecColor.mV[2], draw->mSpecColor.mV[3]);						
		current_shader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, draw->mEnvIntensity);
		current_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, draw->mFullbright ? 1.f : 0.f);

        {
            LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_DEFERRED_TEX_BINDS);
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

        Draw(draw, mask);
        RestoreTexSetup(tex_setup);
    }
}

void LLDrawPoolAlpha::renderEmissives(U32 mask, std::vector<LLDrawInfo*>& emissives)
{
    emissive_shader->bind();
    emissive_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);

    // install glow-accumulating blend mode
    // don't touch color, add to alpha (glow)
	gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE); 

    for (LLDrawInfo* draw : emissives)
    {
        bool tex_setup = TexSetup(draw);
        Draw(draw, (mask & ~LLVertexBuffer::MAP_COLOR) | LLVertexBuffer::MAP_EMISSIVE);
        RestoreTexSetup(tex_setup);
    }

    // restore our alpha blend mode
	gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

    emissive_shader->unbind();
}

void LLDrawPoolAlpha::renderAlpha(U32 mask, S32 pass)
{
	BOOL initialized_lighting = FALSE;
	BOOL light_enabled = TRUE;
	
	BOOL use_shaders = gPipeline.canUseVertexShaders();
		
	for (LLCullResult::sg_iterator i = gPipeline.beginAlphaGroups(); i != gPipeline.endAlphaGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		llassert(group);
		llassert(group->getSpatialPartition());

		if (group->getSpatialPartition()->mRenderByGroup &&
		    !group->isDead())
		{
			bool is_particle_or_hud_particle = group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_PARTICLE
													  || group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_HUD_PARTICLE;

			bool draw_glow_for_this_partition = mShaderLevel > 0; // no shaders = no glow.

			
			LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_GROUP_LOOP);

			bool disable_cull = is_particle_or_hud_particle;
			LLGLDisable cull(disable_cull ? GL_CULL_FACE : 0);

			LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA];

			for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)	
			{
				LLDrawInfo& params = **k;

				if ((params.mVertexBuffer->getTypeMask() & mask) != mask)
				{ //FIXME!
					LL_WARNS_ONCE() << "Missing required components, expected mask: " << mask
									<< " present: " << (params.mVertexBuffer->getTypeMask() & mask)
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
				
				if(use_shaders && (current_shader != target_shader))
				{// If we need shaders, and we're not ALREADY using the proper shader, then bind it
				// (this way we won't rebind shaders unnecessarily).
					current_shader = target_shader;
					current_shader->bind();
				}
				else if (!use_shaders && current_shader != NULL)
				{
					LLGLSLShader::bindNoShader();
					current_shader = NULL;
				}

				if (use_shaders && mat)
				{
					// We have a material.  Supply the appropriate data here.
					if (LLPipeline::sRenderDeferred)
					{
						current_shader->uniform4f(LLShaderMgr::SPECULAR_COLOR, params.mSpecColor.mV[0], params.mSpecColor.mV[1], params.mSpecColor.mV[2], params.mSpecColor.mV[3]);						
						current_shader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, params.mEnvIntensity);
						current_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, params.mFullbright ? 1.f : 0.f);

						if (params.mNormalMap)
						{
							params.mNormalMap->addTextureStats(params.mVSize);
							current_shader->bindTexture(LLShaderMgr::BUMP_MAP, params.mNormalMap);
						} 
						
						if (params.mSpecularMap)
						{
							params.mSpecularMap->addTextureStats(params.mVSize);
							current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, params.mSpecularMap);
						} 
					}

				} else if (LLPipeline::sRenderDeferred && current_shader && (current_shader == simple_shader))
				{
					current_shader->uniform4f(LLShaderMgr::SPECULAR_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);						
					current_shader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, 0.0f);			
					LLViewerFetchedTexture::sFlatNormalImagep->addTextureStats(params.mVSize);
					current_shader->bindTexture(LLShaderMgr::BUMP_MAP, LLViewerFetchedTexture::sFlatNormalImagep);						
					LLViewerFetchedTexture::sWhiteImagep->addTextureStats(params.mVSize);
					current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, LLViewerFetchedTexture::sWhiteImagep);
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
						if (use_shaders && mat)
						{
							current_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, params.mTexture);
						}
						else
						{
						gGL.getTexUnit(0)->bind(params.mTexture, TRUE) ;
						}
						
						if (params.mTextureMatrix)
						{
							tex_setup = true;
							gGL.getTexUnit(0)->activate();
							gGL.matrixMode(LLRender::MM_TEXTURE);
							gGL.loadMatrix((GLfloat*) params.mTextureMatrix->mMatrix);
							gPipeline.mTextureMatrixOps++;
						}
					}
					else
					{
						gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
					}
				}

				{
					LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_PUSH);

					LLGLEnableFunc stencil_test(GL_STENCIL_TEST, params.mSelected, &LLGLCommonFunc::selected_stencil_test);

					gGL.blendFunc((LLRender::eBlendFactor) params.mBlendFuncSrc, (LLRender::eBlendFactor) params.mBlendFuncDst, mAlphaSFactor, mAlphaDFactor);
					params.mVertexBuffer->setBuffer(mask & ~(params.mFullbright ? (LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2) : 0));

					params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
					gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
				}
				
				// If this alpha mesh has glow, then draw it a second time to add the destination-alpha (=glow).  Interleaving these state-changing calls could be expensive, but glow must be drawn Z-sorted with alpha.
				if (current_shader && 
					draw_glow_for_this_partition &&
					params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE))
				{
					// install glow-accumulating blend mode
					gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE, // don't touch color
					LLRender::BF_ONE, LLRender::BF_ONE); // add to alpha (glow)

					emissive_shader->bind();
					if (LLPipeline::sRenderingHUDs)
	                {
		                emissive_shader->uniform1i(LLShaderMgr::NO_ATMO, 1);
	                }
	                else
	                {
		                emissive_shader->uniform1i(LLShaderMgr::NO_ATMO, 0);
	                }
					params.mVertexBuffer->setBuffer((mask & ~LLVertexBuffer::MAP_COLOR) | LLVertexBuffer::MAP_EMISSIVE);
					
					// do the actual drawing, again
					params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
					gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);

					// restore our alpha blend mode
					gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

					current_shader->bind();
				}
			
				if (tex_setup)
				{
					gGL.getTexUnit(0)->activate();
					gGL.loadIdentity();
					gGL.matrixMode(LLRender::MM_MODELVIEW);
				}
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
