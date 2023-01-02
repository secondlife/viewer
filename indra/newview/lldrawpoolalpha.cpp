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
#include "llvoavatar.h"

BOOL LLDrawPoolAlpha::sShowDebugAlpha = FALSE;

#define current_shader (LLGLSLShader::sCurBoundShaderPtr)

static BOOL deferred_render = FALSE;

// minimum alpha before discarding a fragment
static const F32 MINIMUM_ALPHA = 0.004f; // ~ 1/255

// minimum alpha before discarding a fragment when rendering impostors
static const F32 MINIMUM_IMPOSTOR_ALPHA = 0.1f;

LLDrawPoolAlpha::LLDrawPoolAlpha(U32 type) :
		LLRenderPass(type), target_shader(NULL),
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
    return 1;
}

// set some common parameters on the given shader to prepare for alpha rendering
static void prepare_alpha_shader(LLGLSLShader* shader, bool textureGamma, bool deferredEnvironment)
{
    static LLCachedControl<F32> displayGamma(gSavedSettings, "RenderDeferredDisplayGamma");
    F32 gamma = displayGamma;

    // Does this deferred shader need environment uniforms set such as sun_dir, etc. ?
    // NOTE: We don't actually need a gbuffer since we are doing forward rendering (for transparency) post deferred rendering
    // TODO: bindDeferredShader() probably should have the updating of the environment uniforms factored out into updateShaderEnvironmentUniforms()
    // i.e. shaders\class1\deferred\alphaF.glsl
    if (deferredEnvironment)
    {
        gPipeline.bindDeferredShader( *shader );
    }
    else
    {
        shader->bind();
    }
    shader->uniform1i(LLShaderMgr::NO_ATMO, (LLPipeline::sRenderingHUDs) ? 1 : 0);
    shader->uniform1f(LLShaderMgr::DISPLAY_GAMMA, (gamma > 0.1f) ? 1.0f / gamma : (1.0f / 2.2f));

    if (LLPipeline::sImpostorRender)
    {
        shader->setMinimumAlpha(MINIMUM_IMPOSTOR_ALPHA);
    }
    else
    {
        shader->setMinimumAlpha(MINIMUM_ALPHA);
    }
    if (textureGamma)
    {
        shader->uniform1f(LLShaderMgr::TEXTURE_GAMMA, 2.2f);
    }

    //also prepare rigged variant
    if (shader->mRiggedVariant && shader->mRiggedVariant != shader)
    { 
        prepare_alpha_shader(shader->mRiggedVariant, textureGamma, deferredEnvironment);
    }
}

void LLDrawPoolAlpha::renderPostDeferred(S32 pass) 
{ 
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    deferred_render = TRUE;

    // prepare shaders
    emissive_shader = (LLPipeline::sRenderDeferred)   ? &gDeferredEmissiveProgram    :
                      (LLPipeline::sUnderWaterRender) ? &gObjectEmissiveWaterProgram : &gObjectEmissiveProgram;
    prepare_alpha_shader(emissive_shader, true, false);

    fullbright_shader   = (LLPipeline::sImpostorRender) ? &gDeferredFullbrightAlphaMaskProgram :
        (LLPipeline::sUnderWaterRender) ? &gDeferredFullbrightWaterProgram : &gDeferredFullbrightAlphaMaskProgram;
    prepare_alpha_shader(fullbright_shader, true, false);

    simple_shader   = (LLPipeline::sImpostorRender) ? &gDeferredAlphaImpostorProgram :
        (LLPipeline::sUnderWaterRender) ? &gDeferredAlphaWaterProgram : &gDeferredAlphaProgram;
    prepare_alpha_shader(simple_shader, false, true); //prime simple shader (loads shadow relevant uniforms)

    for (int i = 0; i < LLMaterial::SHADER_COUNT; ++i)
    {
        prepare_alpha_shader(LLPipeline::sUnderWaterRender ? &gDeferredMaterialWaterProgram[i] : &gDeferredMaterialProgram[i], false, false); // note: bindDeferredShader will get called during render loop for materials
    }

    // first pass, render rigged objects only and render to depth buffer
    forwardRender(true);

    // second pass, regular forward alpha rendering
    forwardRender();

    // final pass, render to depth for depth of field effects
    if (!LLPipeline::sImpostorRender && gSavedSettings.getBOOL("RenderDepthOfField"))
    { 
        //update depth buffer sampler
        gPipeline.mScreen.flush();
        gPipeline.mDeferredDepth.copyContents(gPipeline.mDeferredScreen, 0, 0, gPipeline.mDeferredScreen.getWidth(), gPipeline.mDeferredScreen.getHeight(),
            0, 0, gPipeline.mDeferredDepth.getWidth(), gPipeline.mDeferredDepth.getHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        gPipeline.mDeferredDepth.bindTarget();
        simple_shader = fullbright_shader = &gObjectFullbrightAlphaMaskProgram;

        simple_shader->bind();
        simple_shader->setMinimumAlpha(0.33f);

        // mask off color buffer writes as we're only writing to depth buffer
        gGL.setColorMask(false, false);

        // If the face is more than 90% transparent, then don't update the Depth buffer for Dof
        // We don't want the nearly invisible objects to cause of DoF effects
        renderAlpha(getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2, 
            true); // <--- discard mostly transparent faces

        gPipeline.mDeferredDepth.flush();
        gPipeline.mScreen.bindTarget();
        gGL.setColorMask(true, false);
    }

    deferred_render = FALSE;
}

//set some generic parameters for forward (non-deferred) rendering
static void prepare_forward_shader(LLGLSLShader* shader, F32 minimum_alpha)
{
    shader->bind();
    shader->setMinimumAlpha(minimum_alpha);
    shader->uniform1i(LLShaderMgr::NO_ATMO, LLPipeline::sRenderingHUDs ? 1 : 0);

    //also prepare rigged variant
    if (shader->mRiggedVariant && shader->mRiggedVariant != shader)
    {
        prepare_forward_shader(shader->mRiggedVariant, minimum_alpha);
    }
}

void LLDrawPoolAlpha::render(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    simple_shader = (LLPipeline::sImpostorRender) ? &gObjectSimpleImpostorProgram :
        (LLPipeline::sUnderWaterRender) ? &gObjectSimpleWaterProgram : &gObjectSimpleAlphaMaskProgram;

    fullbright_shader = (LLPipeline::sImpostorRender) ? &gObjectFullbrightAlphaMaskProgram :
        (LLPipeline::sUnderWaterRender) ? &gObjectFullbrightWaterProgram : &gObjectFullbrightAlphaMaskProgram;

    emissive_shader = (LLPipeline::sImpostorRender) ? &gObjectEmissiveProgram :
        (LLPipeline::sUnderWaterRender) ? &gObjectEmissiveWaterProgram : &gObjectEmissiveProgram;

    F32 minimum_alpha = MINIMUM_ALPHA;
    if (LLPipeline::sImpostorRender)
    {
        minimum_alpha = MINIMUM_IMPOSTOR_ALPHA;
    }

    prepare_forward_shader(fullbright_shader, minimum_alpha);
    prepare_forward_shader(simple_shader, minimum_alpha);

    for (int i = 0; i < LLMaterial::SHADER_COUNT; ++i)
    {
        prepare_forward_shader(LLPipeline::sUnderWaterRender ? &gDeferredMaterialWaterProgram[i] : &gDeferredMaterialProgram[i], minimum_alpha);
    }

    //first pass -- rigged only and drawn to depth buffer
    forwardRender(true);

    //second pass -- non-rigged, no depth buffer writes
    forwardRender();
}

void LLDrawPoolAlpha::forwardRender(bool rigged)
{
    gPipeline.enableLightsDynamic();

    LLGLSPipelineAlpha gls_pipeline_alpha;

    //enable writing to alpha for emissive effects
    gGL.setColorMask(true, true);

    bool write_depth = rigged || 
        LLDrawPoolWater::sSkipScreenCopy
        // we want depth written so that rendered alpha will
        // contribute to the alpha mask used for impostors
        || LLPipeline::sImpostorRenderAlphaDepthPass;

    LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);

    mColorSFactor = LLRender::BF_SOURCE_ALPHA;           // } regular alpha blend
    mColorDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA; // }
    mAlphaSFactor = LLRender::BF_ZERO;                         // } glow suppression
    mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;       // }
    gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

    // If the face is more than 90% transparent, then don't update the Depth buffer for Dof
    // We don't want the nearly invisible objects to cause of DoF effects
    renderAlpha(getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2, false, rigged);

    gGL.setColorMask(true, false);

    if (!rigged)
    { //render "highlight alpha" on final non-rigged pass
        // NOTE -- hacky call here protected by !rigged instead of alongside "forwardRender"
        // so renderDebugAlpha is executed while gls_pipeline_alpha and depth GL state
        // variables above are still in scope
        renderDebugAlpha();
    }
}

void LLDrawPoolAlpha::renderDebugAlpha()
{
	if (sShowDebugAlpha)
	{
        gHighlightProgram.bind();
        gGL.diffuseColor4f(1, 0, 0, 1);
        LLViewerFetchedTexture::sSmokeImagep->addTextureStats(1024.f * 1024.f);
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

        gHighlightProgram.mRiggedVariant->bind();
        gGL.diffuseColor4f(1, 0, 0, 1);

        pushRiggedBatches(LLRenderPass::PASS_ALPHA_MASK_RIGGED, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
        pushRiggedBatches(LLRenderPass::PASS_ALPHA_INVISIBLE_RIGGED, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);

        // Material alpha mask
        gGL.diffuseColor4f(0, 0, 1, 1);
        pushRiggedBatches(LLRenderPass::PASS_MATERIAL_ALPHA_MASK_RIGGED, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
        pushRiggedBatches(LLRenderPass::PASS_NORMMAP_MASK_RIGGED, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
        pushRiggedBatches(LLRenderPass::PASS_SPECMAP_MASK_RIGGED, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
        pushRiggedBatches(LLRenderPass::PASS_NORMSPEC_MASK_RIGGED, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
        pushRiggedBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);

        gGL.diffuseColor4f(0, 1, 0, 1);
        pushRiggedBatches(LLRenderPass::PASS_INVISIBLE_RIGGED, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
        LLGLSLShader::sCurBoundShaderPtr->unbind();
	}
}

void LLDrawPoolAlpha::renderAlphaHighlight(U32 mask)
{
    for (int pass = 0; pass < 2; ++pass)
    { //two passes, one rigged and one not
        LLVOAvatar* lastAvatar = nullptr;
        U64 lastMeshId = 0;

        LLCullResult::sg_iterator begin = pass == 0 ? gPipeline.beginAlphaGroups() : gPipeline.beginRiggedAlphaGroups();
        LLCullResult::sg_iterator end = pass == 0 ? gPipeline.endAlphaGroups() : gPipeline.endRiggedAlphaGroups();

        for (LLCullResult::sg_iterator i = begin; i != end; ++i)
        {
            LLSpatialGroup* group = *i;
            if (group->getSpatialPartition()->mRenderByGroup &&
                !group->isDead())
            {
                LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA+pass]; // <-- hacky + pass to use PASS_ALPHA_RIGGED on second pass 

                for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
                {
                    LLDrawInfo& params = **k;

                    if (params.mParticle)
                    {
                        continue;
                    }

                    bool rigged = (params.mAvatar != nullptr);
                    gHighlightProgram.bind(rigged);
                    gGL.diffuseColor4f(1, 0, 0, 1);

                    if (rigged)
                    {
                        if (lastAvatar != params.mAvatar ||
                            lastMeshId != params.mSkinInfo->mHash)
                        {
                            if (!uploadMatrixPalette(params))
                            {
                                continue;
                            }
                            lastAvatar = params.mAvatar;
                            lastMeshId = params.mSkinInfo->mHash;
                        }
                    }

                    LLRenderPass::applyModelMatrix(params);
                    if (params.mGroup)
                    {
                        params.mGroup->rebuildMesh();
                    }
                    params.mVertexBuffer->setBufferFast(rigged ? mask | LLVertexBuffer::MAP_WEIGHT4 : mask);
                    params.mVertexBuffer->drawRangeFast(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
                }
            }
        }
    }

    // make sure static version of highlight shader is bound before returning
    gHighlightProgram.bind();
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

bool LLDrawPoolAlpha::TexSetup(LLDrawInfo* draw, bool use_material)
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
    else if (current_shader == simple_shader || current_shader == simple_shader->mRiggedVariant)
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

void LLDrawPoolAlpha::drawEmissive(U32 mask, LLDrawInfo* draw)
{
    LLGLSLShader::sCurBoundShaderPtr->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);
    draw->mVertexBuffer->setBufferFast((mask & ~LLVertexBuffer::MAP_COLOR) | LLVertexBuffer::MAP_EMISSIVE);
	draw->mVertexBuffer->drawRangeFast(draw->mDrawMode, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
}


void LLDrawPoolAlpha::renderEmissives(U32 mask, std::vector<LLDrawInfo*>& emissives)
{
    emissive_shader->bind();
    emissive_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);

    for (LLDrawInfo* draw : emissives)
    {
        bool tex_setup = TexSetup(draw, false);
        drawEmissive(mask, draw);
        RestoreTexSetup(tex_setup);
    }
}

void LLDrawPoolAlpha::renderRiggedEmissives(U32 mask, std::vector<LLDrawInfo*>& emissives)
{
    LLGLDepthTest depth(GL_TRUE, GL_FALSE); //disable depth writes since "emissive" is additive so sorting doesn't matter
    LLGLSLShader* shader = emissive_shader->mRiggedVariant;
    shader->bind();
    shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);

    LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;

    mask |= LLVertexBuffer::MAP_WEIGHT4;

    for (LLDrawInfo* draw : emissives)
    {
        bool tex_setup = TexSetup(draw, false);
        if (lastAvatar != draw->mAvatar || lastMeshId != draw->mSkinInfo->mHash)
        {
            if (!uploadMatrixPalette(*draw))
            { // failed to upload matrix palette, skip rendering
                continue;
            }
            lastAvatar = draw->mAvatar;
            lastMeshId = draw->mSkinInfo->mHash;
        }
        drawEmissive(mask, draw);
        RestoreTexSetup(tex_setup);
    }
}

void LLDrawPoolAlpha::renderAlpha(U32 mask, bool depth_only, bool rigged)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    BOOL initialized_lighting = FALSE;
	BOOL light_enabled = TRUE;

    LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    LLGLSLShader* lastAvatarShader = nullptr;

    LLCullResult::sg_iterator begin;
    LLCullResult::sg_iterator end;

    if (rigged)
    {
        begin = gPipeline.beginRiggedAlphaGroups();
        end = gPipeline.endRiggedAlphaGroups();
    }
    else
    {
        begin = gPipeline.beginAlphaGroups();
        end = gPipeline.endAlphaGroups();
    }

    for (LLCullResult::sg_iterator i = begin; i != end; ++i)
	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("renderAlpha - group");
		LLSpatialGroup* group = *i;
		llassert(group);
		llassert(group->getSpatialPartition());

		if (group->getSpatialPartition()->mRenderByGroup &&
		    !group->isDead())
		{
            static std::vector<LLDrawInfo*> emissives;
            static std::vector<LLDrawInfo*> rigged_emissives;
            emissives.resize(0);
            rigged_emissives.resize(0);

			bool is_particle_or_hud_particle = group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_PARTICLE
													  || group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_HUD_PARTICLE;

			bool draw_glow_for_this_partition = mShaderLevel > 0; // no shaders = no glow.

			bool disable_cull = is_particle_or_hud_particle;
			LLGLDisable cull(disable_cull ? GL_CULL_FACE : 0);

			LLSpatialGroup::drawmap_elem_t& draw_info = rigged ? group->mDrawMap[LLRenderPass::PASS_ALPHA_RIGGED] : group->mDrawMap[LLRenderPass::PASS_ALPHA];

			for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)	
			{
				LLDrawInfo& params = **k;
                if ((bool)params.mAvatar != rigged)
                {
                    continue;
                }

                LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("ra - push batch")

                U32 have_mask = params.mVertexBuffer->getTypeMask() & mask;
				if (have_mask != mask)
				{ //FIXME!
					LL_WARNS_ONCE() << "Missing required components, expected mask: " << mask
									<< " present: " << have_mask
									<< ". Skipping render batch." << LL_ENDL;
					continue;
				}

				if(depth_only)
				{
                    // when updating depth buffer, discard faces that are more than 90% transparent
					LLFace*	face = params.mFace;
					if(face)
					{
						const LLTextureEntry* tep = face->getTextureEntry();
						if(tep)
						{ // don't render faces that are more than 90% transparent
							if(tep->getColor().mV[3] < MINIMUM_IMPOSTOR_ALPHA)
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

                    if (params.mAvatar != nullptr)
                    {
                        llassert(target_shader->mRiggedVariant != nullptr);
                        target_shader = target_shader->mRiggedVariant;
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
				
                if (params.mAvatar != nullptr)
                {
                    target_shader = target_shader->mRiggedVariant;
                }

                if (current_shader != target_shader)
                {// If we need shaders, and we're not ALREADY using the proper shader, then bind it
                // (this way we won't rebind shaders unnecessarily).
                    target_shader->bind();
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

                if (params.mAvatar != nullptr)
                {
                    if (lastAvatar != params.mAvatar ||
                        lastMeshId != params.mSkinInfo->mHash ||
                        lastAvatarShader != LLGLSLShader::sCurBoundShaderPtr)
                    {
                        if (!uploadMatrixPalette(params))
                        {
                            continue;
                        }
                        lastAvatar = params.mAvatar;
                        lastMeshId = params.mSkinInfo->mHash;
                        lastAvatarShader = LLGLSLShader::sCurBoundShaderPtr;
                    }
                }

                bool tex_setup = TexSetup(&params, (mat != nullptr));

				{
					LLGLEnableFunc stencil_test(GL_STENCIL_TEST, params.mSelected, &LLGLCommonFunc::selected_stencil_test);

					gGL.blendFunc((LLRender::eBlendFactor) params.mBlendFuncSrc, (LLRender::eBlendFactor) params.mBlendFuncDst, mAlphaSFactor, mAlphaDFactor);

                    bool reset_minimum_alpha = false;
                    if (!LLPipeline::sImpostorRender &&
                        params.mBlendFuncDst != LLRender::BF_SOURCE_ALPHA &&
                        params.mBlendFuncSrc != LLRender::BF_SOURCE_ALPHA)
                    { // this draw call has a custom blend function that may require rendering of "invisible" fragments
                        current_shader->setMinimumAlpha(0.f);
                        reset_minimum_alpha = true;
                    }
                    
                    U32 drawMask = mask;
                    if (params.mFullbright)
                    {
                        drawMask &= ~(LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2);
                    }
                    if (params.mAvatar != nullptr)
                    {
                        drawMask |= LLVertexBuffer::MAP_WEIGHT4;
                    }

                    params.mVertexBuffer->setBufferFast(drawMask);
                    params.mVertexBuffer->drawRangeFast(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);

                    if (reset_minimum_alpha)
                    {
                        current_shader->setMinimumAlpha(MINIMUM_ALPHA);
                    }
				}

				// If this alpha mesh has glow, then draw it a second time to add the destination-alpha (=glow).  Interleaving these state-changing calls is expensive, but glow must be drawn Z-sorted with alpha.
				if (draw_glow_for_this_partition &&
					params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE))
				{
                    if (params.mAvatar != nullptr)
                    {
                        rigged_emissives.push_back(&params);
                    }
                    else
                    {
                        emissives.push_back(&params);
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

            // render emissive faces into alpha channel for bloom effects
            if (!depth_only)
            {
                gPipeline.enableLightsDynamic();

                // install glow-accumulating blend mode
                // don't touch color, add to alpha (glow)
                gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE);

                bool rebind = false;
                LLGLSLShader* lastShader = current_shader;
                if (!emissives.empty())
                {
                    light_enabled = true;
                    renderEmissives(mask, emissives);
                    rebind = true;
                }

                if (!rigged_emissives.empty())
                {
                    light_enabled = true;
                    renderRiggedEmissives(mask, rigged_emissives);
                    rebind = true;
                }

                // restore our alpha blend mode
                gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

                if (lastShader && rebind)
                {
                    lastShader->bind();
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

bool LLDrawPoolAlpha::uploadMatrixPalette(const LLDrawInfo& params)
{
    if (params.mAvatar.isNull())
    {
        return false;
    }
    const LLVOAvatar::MatrixPaletteCache& mpc = params.mAvatar.get()->updateSkinInfoMatrixPalette(params.mSkinInfo);
    U32 count = mpc.mMatrixPalette.size();

    if (count == 0)
    {
        //skin info not loaded yet, don't render
        return false;
    }

    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX,
        count,
        FALSE,
        (GLfloat*)&(mpc.mGLMp[0]));

    return true;
}
