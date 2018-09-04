/** 
 * @file lldrawpoolwlsky.cpp
 * @brief LLDrawPoolWLSky class implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "lldrawpoolwlsky.h"

#include "llerror.h"
#include "llgl.h"
#include "pipeline.h"
#include "llviewercamera.h"
#include "llimage.h"
#include "llviewershadermgr.h"
#include "llglslshader.h"
#include "llsky.h"
#include "llvowlsky.h"
#include "llviewerregion.h"
#include "llface.h"
#include "llrender.h"

#include "llenvironment.h" 
#include "llatmosphere.h"

static LLStaticHashedString sCamPosLocal("camPosLocal");
static LLStaticHashedString sCustomAlpha("custom_alpha");

static LLGLSLShader* cloud_shader = NULL;
static LLGLSLShader* sky_shader   = NULL;
static LLGLSLShader* sun_shader   = NULL;
static LLGLSLShader* moon_shader  = NULL;

static float sStarTime;

LLDrawPoolWLSky::LLDrawPoolWLSky(void) :
	LLDrawPool(POOL_WL_SKY)
{
}

LLDrawPoolWLSky::~LLDrawPoolWLSky()
{
}

LLViewerTexture *LLDrawPoolWLSky::getDebugTexture()
{
	return NULL;
}

void LLDrawPoolWLSky::beginRenderPass( S32 pass )
{
	sky_shader =
		LLPipeline::sUnderWaterRender ?
			&gObjectFullbrightNoColorWaterProgram :
			&gWLSkyProgram;

	cloud_shader =
			LLPipeline::sUnderWaterRender ?
				&gObjectFullbrightNoColorWaterProgram :
				&gWLCloudProgram;

    sun_shader =
			LLPipeline::sUnderWaterRender ?
				&gObjectFullbrightNoColorWaterProgram :
				&gWLSunProgram;

    moon_shader =
			LLPipeline::sUnderWaterRender ?
				&gObjectFullbrightNoColorWaterProgram :
				&gWLMoonProgram;
}

void LLDrawPoolWLSky::endRenderPass( S32 pass )
{
}

void LLDrawPoolWLSky::beginDeferredPass(S32 pass)
{
	sky_shader = &gDeferredWLSkyProgram;
	cloud_shader = &gDeferredWLCloudProgram;

    sun_shader =
			LLPipeline::sUnderWaterRender ?
				&gObjectFullbrightNoColorWaterProgram :
				&gDeferredWLSunProgram;

    moon_shader =
			LLPipeline::sUnderWaterRender ?
				&gObjectFullbrightNoColorWaterProgram :
				&gDeferredWLMoonProgram;
}

void LLDrawPoolWLSky::endDeferredPass(S32 pass)
{

}

void LLDrawPoolWLSky::renderFsSky(const LLVector3& camPosLocal, F32 camHeightLocal, LLGLSLShader * shader) const
{
    gSky.mVOWLSkyp->drawFsSky();
}

void LLDrawPoolWLSky::renderDome(const LLVector3& camPosLocal, F32 camHeightLocal, LLGLSLShader * shader) const
{
    llassert_always(NULL != shader);

	gGL.pushMatrix();

	//chop off translation
	if (LLPipeline::sReflectionRender && camPosLocal.mV[2] > 256.f)
	{
		gGL.translatef(camPosLocal.mV[0], camPosLocal.mV[1], 256.f-camPosLocal.mV[2]*0.5f);
	}
	else
	{
		gGL.translatef(camPosLocal.mV[0], camPosLocal.mV[1], camPosLocal.mV[2]);
	}
		

	// the windlight sky dome works most conveniently in a coordinate system
	// where Y is up, so permute our basis vectors accordingly.
	gGL.rotatef(120.f, 1.f / F_SQRT3, 1.f / F_SQRT3, 1.f / F_SQRT3);

	gGL.scalef(0.333f, 0.333f, 0.333f);

	gGL.translatef(0.f,-camHeightLocal, 0.f);
	
	// Draw WL Sky
	shader->uniform3f(sCamPosLocal, 0.f, camHeightLocal, 0.f);

    gSky.mVOWLSkyp->drawDome();

	gGL.popMatrix();
}

void LLDrawPoolWLSky::renderSkyHazeDeferred(const LLVector3& camPosLocal, F32 camHeightLocal) const
{
    if (gPipeline.useAdvancedAtmospherics() && gPipeline.canUseWindLightShaders() && gAtmosphere)
    {
		sky_shader->bind();

        // bind precomputed textures necessary for calculating sun and sky luminance
        sky_shader->bindTexture(LLShaderMgr::TRANSMITTANCE_TEX, gAtmosphere->getTransmittance());
        sky_shader->bindTexture(LLShaderMgr::SCATTER_TEX, gAtmosphere->getScattering());
        sky_shader->bindTexture(LLShaderMgr::SINGLE_MIE_SCATTER_TEX, gAtmosphere->getMieScattering());
        sky_shader->bindTexture(LLShaderMgr::ILLUMINANCE_TEX, gAtmosphere->getIlluminance());

        LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();
        LLVector4 sun_dir = LLEnvironment::instance().getClampedSunNorm();
        LLVector4 moon_dir = LLEnvironment::instance().getClampedMoonNorm();

        F32 sunSize = (float)cosf(psky->getSunArcRadians());
        sky_shader->uniform1f(LLShaderMgr::SUN_SIZE, sunSize);
        sky_shader->uniform3fv(LLShaderMgr::DEFERRED_SUN_DIR, 1, sun_dir.mV);
        sky_shader->uniform3fv(LLShaderMgr::DEFERRED_MOON_DIR, 1, moon_dir.mV);

        llassert(sky_shader->getUniformLocation(LLShaderMgr::INVERSE_PROJECTION_MATRIX));

        glh::matrix4f proj_mat = get_current_projection();
		glh::matrix4f inv_proj = proj_mat.inverse();

	    sky_shader->uniformMatrix4fv(LLShaderMgr::INVERSE_PROJECTION_MATRIX, 1, FALSE, inv_proj.m);

        /* clouds are rendered along with sky in adv atmo
        if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS) && gSky.mVOSkyp->getCloudNoiseTex())
        {
            sky_shader->bindTexture(LLShaderMgr::CLOUD_NOISE_MAP, gSky.mVOSkyp->getCloudNoiseTex());
            sky_shader->bindTexture(LLShaderMgr::CLOUD_NOISE_MAP_NEXT, gSky.mVOSkyp->getCloudNoiseTexNext());
        }*/

        sky_shader->uniform3f(sCamPosLocal, camPosLocal.mV[0], camPosLocal.mV[1], camPosLocal.mV[2]);

        renderFsSky(camPosLocal, camHeightLocal, sky_shader);

		sky_shader->unbind();
	}
}

void LLDrawPoolWLSky::renderSkyHaze(const LLVector3& camPosLocal, F32 camHeightLocal) const
{
    LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();

	if (gPipeline.canUseWindLightShaders() && gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
	{
        LLGLDisable blend(GL_BLEND);
        sky_shader->bind();

        /// Render the skydome
        renderDome(origin, camHeightLocal, sky_shader);	

		sky_shader->unbind();
    }
}

void LLDrawPoolWLSky::renderStars(void) const
{
	LLGLSPipelineSkyBox gls_sky;
	LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	
	// *NOTE: have to have bound the cloud noise texture already since register
	// combiners blending below requires something to be bound
	// and we might as well only bind once.
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
	
	gPipeline.disableLights();
	
	// *NOTE: we divide by two here and GL_ALPHA_SCALE by two below to avoid
	// clamping and allow the star_alpha param to brighten the stars.
	LLColor4 star_alpha(LLColor4::black);

    // *LAPRAS
    star_alpha.mV[3] = LLEnvironment::instance().getCurrentSky()->getStarBrightness() / (2.f + ((rand() >> 16)/65535.0f)); // twinkle twinkle

	// If start_brightness is not set, exit
	if( star_alpha.mV[3] < 0.001 )
	{
		LL_DEBUGS("SKY") << "star_brightness below threshold." << LL_ENDL;
		return;
	}

    LLViewerTexture* tex_a = gSky.mVOSkyp->getBloomTex();
    LLViewerTexture* tex_b = gSky.mVOSkyp->getBloomTexNext();
	
    if (tex_a && (!tex_b || (tex_a == tex_b)))
    {
        // Bind current and next sun textures
		gGL.getTexUnit(0)->bind(tex_a);
    }
    else if (tex_b && !tex_a)
    {
        gGL.getTexUnit(0)->bind(tex_b);
    }
    else if (tex_b != tex_a)
    {
        gGL.getTexUnit(0)->bind(tex_a);
    }

	gGL.pushMatrix();
	gGL.rotatef(gFrameTimeSeconds*0.01f, 0.f, 0.f, 1.f);
	if (LLGLSLShader::sNoFixedFunction)
	{
		gCustomAlphaProgram.bind();
		gCustomAlphaProgram.uniform1f(sCustomAlpha, star_alpha.mV[3]);
	}
	else
	{
		gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_MULT, LLTexUnit::TBS_TEX_COLOR, LLTexUnit::TBS_VERT_COLOR);
		gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_MULT_X2, LLTexUnit::TBS_CONST_ALPHA, LLTexUnit::TBS_TEX_ALPHA);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, star_alpha.mV);
	}

	gSky.mVOWLSkyp->drawStars();

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	gGL.popMatrix();

	if (LLGLSLShader::sNoFixedFunction)
	{
		gCustomAlphaProgram.unbind();
	}
	else
	{
		// and disable the combiner states
		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	}
}

void LLDrawPoolWLSky::renderStarsDeferred(void) const
{
	LLGLSPipelineSkyBox gls_sky;
	LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);
		
	// *LAPRAS
    F32 star_alpha = LLEnvironment::instance().getCurrentSky()->getStarBrightness() / (2.f + ((rand() >> 16)/65535.0f)); // twinkle twinkle

	// If start_brightness is not set, exit
	if(star_alpha < 0.001f)
	{
		LL_DEBUGS("SKY") << "star_brightness below threshold." << LL_ENDL;
		return;
	}

	gDeferredStarProgram.bind();	

    LLViewerTexture* tex_a = gSky.mVOSkyp->getBloomTex();
    LLViewerTexture* tex_b = gSky.mVOSkyp->getBloomTexNext();

    F32 blend_factor = LLEnvironment::instance().getCurrentSky()->getBlendFactor();
	
    if (tex_a && (!tex_b || (tex_a == tex_b)))
    {
        // Bind current and next sun textures
		gGL.getTexUnit(0)->bind(tex_a);
        gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
        blend_factor = 0;
    }
    else if (tex_b && !tex_a)
    {
        gGL.getTexUnit(0)->bind(tex_b);
        gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
        blend_factor = 0;
    }
    else if (tex_b != tex_a)
    {
        gGL.getTexUnit(0)->bind(tex_a);
        gGL.getTexUnit(1)->bind(tex_b);
    }

    gDeferredStarProgram.uniform1f(LLShaderMgr::BLEND_FACTOR, blend_factor);
	gDeferredStarProgram.uniform1f(sCustomAlpha, star_alpha);
	gSky.mVOWLSkyp->drawStars();

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);

    gDeferredStarProgram.unbind();
}

void LLDrawPoolWLSky::renderSkyClouds(const LLVector3& camPosLocal, F32 camHeightLocal) const
{
	if (gPipeline.canUseWindLightShaders() && gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS) && gSky.mVOSkyp->getCloudNoiseTex())
	{
		LLGLEnable blend(GL_BLEND);
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
		
		gGL.getTexUnit(0)->bind(gSky.mVOSkyp->getCloudNoiseTex());
        gGL.getTexUnit(1)->bind(gSky.mVOSkyp->getCloudNoiseTexNext());

		cloud_shader->bind();
        F32 blend_factor = LLEnvironment::instance().getCurrentSky()->getBlendFactor();
        cloud_shader->uniform1f(LLShaderMgr::BLEND_FACTOR, blend_factor);

		/// Render the skydome
        renderDome(camPosLocal, camHeightLocal, cloud_shader);

		cloud_shader->unbind();
	}
}

void LLDrawPoolWLSky::renderHeavenlyBodies()
{
	LLGLSPipelineSkyBox gls_skybox;
	LLGLEnable blend_on(GL_BLEND);
	gPipeline.disableLights();

    LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();
	gGL.pushMatrix();
	gGL.translatef(origin.mV[0], origin.mV[1], origin.mV[2]);	        

	LLFace * face = gSky.mVOSkyp->mFace[LLVOSky::FACE_SUN];

    F32 blend_factor = LLEnvironment::instance().getCurrentSky()->getBlendFactor();
    bool can_use_vertex_shaders = gPipeline.canUseVertexShaders();

	if (gSky.mVOSkyp->getSun().getDraw() && face && face->getGeomCount())
	{
		LLViewerTexture* tex_a = face->getTexture(LLRender::DIFFUSE_MAP);
        LLViewerTexture* tex_b = face->getTexture(LLRender::ALTERNATE_DIFFUSE_MAP);

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);

        // if we even have sun disc textures to work with...
        if (tex_a || tex_b)
        {
            // if and only if we have a texture defined, render the sun disc
            if (can_use_vertex_shaders)
		    {
			    sun_shader->bind();
            }

            if (tex_a && (!tex_b || (tex_a == tex_b)))
            {
                // Bind current and next sun textures
                sun_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_a, LLTexUnit::TT_TEXTURE);
                blend_factor = 0;
            }
            else if (tex_b && !tex_a)
            {
                sun_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_b, LLTexUnit::TT_TEXTURE);
                blend_factor = 0;
            }
            else if (tex_b != tex_a)
            {
                sun_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_a, LLTexUnit::TT_TEXTURE);
                sun_shader->bindTexture(LLShaderMgr::ALTERNATE_DIFFUSE_MAP, tex_b, LLTexUnit::TT_TEXTURE);
            }

		    LLColor4 color(gSky.mVOSkyp->getSun().getInterpColor());

            if (can_use_vertex_shaders)
		    {
                sun_shader->uniform4fv(LLShaderMgr::DIFFUSE_COLOR, 1, color.mV);
                sun_shader->uniform1f(LLShaderMgr::BLEND_FACTOR, blend_factor);
		    }

		    LLFacePool::LLOverrideFaceColor color_override(this, color);
		    face->renderIndexed();

            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);

            if (can_use_vertex_shaders)
		    {
			    sun_shader->unbind();
            }
        }
	}

	face = gSky.mVOSkyp->mFace[LLVOSky::FACE_MOON];

	if (gSky.mVOSkyp->getMoon().getDraw() && face && face->getTexture(LLRender::DIFFUSE_MAP) && face->getGeomCount() && moon_shader)
	{        
        LLViewerTexture* tex_a = face->getTexture(LLRender::DIFFUSE_MAP);
        LLViewerTexture* tex_b = face->getTexture(LLRender::ALTERNATE_DIFFUSE_MAP);

		LLColor4 color(gSky.mVOSkyp->getMoon().getInterpColor());
		
        if (can_use_vertex_shaders)
		{
			moon_shader->bind();
        }

        if (tex_a && (!tex_b || (tex_a == tex_b)))
        {
            // Bind current and next sun textures
            moon_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_a, LLTexUnit::TT_TEXTURE);
            blend_factor = 0;
        }
        else if (tex_b && !tex_a)
        {
            moon_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_b, LLTexUnit::TT_TEXTURE);
            blend_factor = 0;
        }
        else if (tex_b != tex_a)
        {
            moon_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_a, LLTexUnit::TT_TEXTURE);
            moon_shader->bindTexture(LLShaderMgr::ALTERNATE_DIFFUSE_MAP, tex_b, LLTexUnit::TT_TEXTURE);
        }

        if (can_use_vertex_shaders)
		{
            moon_shader->uniform4fv(LLShaderMgr::DIFFUSE_COLOR, 1, color.mV);                
            moon_shader->uniform1f(LLShaderMgr::BLEND_FACTOR, blend_factor);
        }

		LLFacePool::LLOverrideFaceColor color_override(this, color);
		
		face->renderIndexed();

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);

		if (can_use_vertex_shaders)
		{
			moon_shader->unbind();
		}
	}

    gGL.popMatrix();
}

void LLDrawPoolWLSky::renderDeferred(S32 pass)
{
	if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
	{
		return;
	}
	LL_RECORD_BLOCK_TIME(FTM_RENDER_WL_SKY);

    const F32 camHeightLocal = LLEnvironment::instance().getCamHeight();

	LLGLSNoFog disableFog;	
	LLGLDisable clip(GL_CLIP_PLANE0);

	gGL.setColorMask(true, false);

	LLGLSquashToFarClip far_clip(get_current_projection());

    LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();

    if (gPipeline.canUseWindLightShaders())
    {
        {
            // Disable depth-test for sky, but re-enable depth writes for the cloud
            // rendering below so the cloud shader can write out depth for the stars to test against
            LLGLDepthTest depth(GL_TRUE, GL_FALSE);
            if (gPipeline.useAdvancedAtmospherics())
            {
	            renderSkyHazeDeferred(origin, camHeightLocal);
            }
            else
            {
                renderSkyHaze(origin, camHeightLocal);   
		        
            }
            renderHeavenlyBodies();
        }

        renderSkyClouds(origin, camHeightLocal);
    }    
    gGL.setColorMask(true, true);
}

void LLDrawPoolWLSky::renderPostDeferred(S32 pass)
{
    LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();

    LLGLSNoFog disableFog;	
	LLGLDisable clip(GL_CLIP_PLANE0);
    LLGLSquashToFarClip far_clip(get_current_projection());

	gGL.pushMatrix();
	gGL.translatef(origin.mV[0], origin.mV[1], origin.mV[2]);
    gGL.setColorMask(true, false);

    // would be nice to do this here, but would need said bodies
    // to render at a realistic distance for depth-testing against the clouds...
    //renderHeavenlyBodies();
    renderStarsDeferred();

    gGL.popMatrix();
    gGL.setColorMask(true, true);
}

void LLDrawPoolWLSky::render(S32 pass)
{
	if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
	{
		return;
	}
	LL_RECORD_BLOCK_TIME(FTM_RENDER_WL_SKY);

    const F32 camHeightLocal = LLEnvironment::instance().getCamHeight();

	LLGLSNoFog disableFog;
	LLGLDepthTest depth(GL_TRUE, GL_FALSE);
	LLGLDisable clip(GL_CLIP_PLANE0);

	LLGLSquashToFarClip far_clip(get_current_projection());

    LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();

	renderSkyHaze(origin, camHeightLocal);

    bool use_advanced = gPipeline.useAdvancedAtmospherics();
    
    if (!use_advanced)
    {
	    gGL.pushMatrix();

        // MAINT-9006 keep sun position consistent between ALM and non-ALM rendering
		//gGL.translatef(origin.mV[0], origin.mV[1], origin.mV[2]);

		// *NOTE: have to bind a texture here since register combiners blending in
		// renderStars() requires something to be bound and we might as well only
		// bind the moon's texture once.		
		gGL.getTexUnit(0)->bind(gSky.mVOSkyp->mFace[LLVOSky::FACE_MOON]->getTexture());
        gGL.getTexUnit(1)->bind(gSky.mVOSkyp->mFace[LLVOSky::FACE_MOON]->getTexture(LLRender::ALTERNATE_DIFFUSE_MAP));

		renderHeavenlyBodies();

		renderStars();

	    gGL.popMatrix();
    }

	renderSkyClouds(origin, camHeightLocal);

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

void LLDrawPoolWLSky::prerender()
{
	//LL_INFOS() << "wlsky prerendering pass." << LL_ENDL;
}

LLDrawPoolWLSky *LLDrawPoolWLSky::instancePool()
{
	return new LLDrawPoolWLSky();
}

LLViewerTexture* LLDrawPoolWLSky::getTexture()
{
	return NULL;
}

void LLDrawPoolWLSky::resetDrawOrders()
{
}

//static
void LLDrawPoolWLSky::cleanupGL()
{
}

//static
void LLDrawPoolWLSky::restoreGL()
{
}
