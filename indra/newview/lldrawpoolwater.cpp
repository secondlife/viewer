/** 
 * @file lldrawpoolwater.cpp
 * @brief LLDrawPoolWater class implementation
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
#include "llfeaturemanager.h"
#include "lldrawpoolwater.h"

#include "llviewercontrol.h"
#include "lldir.h"
#include "llerror.h"
#include "m3math.h"
#include "llrender.h"

#include "llagent.h"		// for gAgent for getRegion for getWaterHeight
#include "llcubemap.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "llvosky.h"
#include "llvowater.h"
#include "llworld.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llenvironment.h"
#include "llsettingssky.h"
#include "llsettingswater.h"

BOOL deferred_render = FALSE;

BOOL LLDrawPoolWater::sSkipScreenCopy = FALSE;
BOOL LLDrawPoolWater::sNeedsReflectionUpdate = TRUE;
BOOL LLDrawPoolWater::sNeedsDistortionUpdate = TRUE;
F32 LLDrawPoolWater::sWaterFogEnd = 0.f;

LLDrawPoolWater::LLDrawPoolWater() : LLFacePool(POOL_WATER)
{
}

LLDrawPoolWater::~LLDrawPoolWater()
{
}

void LLDrawPoolWater::setTransparentTextures(const LLUUID& transparentTextureId, const LLUUID& nextTransparentTextureId)
{
    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();
    mWaterImagep[0] = LLViewerTextureManager::getFetchedTexture(!transparentTextureId.isNull() ? transparentTextureId : pwater->GetDefaultTransparentTextureAssetId());
    mWaterImagep[1] = LLViewerTextureManager::getFetchedTexture(!nextTransparentTextureId.isNull() ? nextTransparentTextureId : (!transparentTextureId.isNull() ? transparentTextureId : pwater->GetDefaultTransparentTextureAssetId()));
    mWaterImagep[0]->addTextureStats(1024.f*1024.f);
    mWaterImagep[1]->addTextureStats(1024.f*1024.f);
}

void LLDrawPoolWater::setOpaqueTexture(const LLUUID& opaqueTextureId)
{
    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();
    mOpaqueWaterImagep = LLViewerTextureManager::getFetchedTexture(opaqueTextureId);
    mOpaqueWaterImagep->addTextureStats(1024.f*1024.f);
}

void LLDrawPoolWater::setNormalMaps(const LLUUID& normalMapId, const LLUUID& nextNormalMapId)
{
    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();
    mWaterNormp[0] = LLViewerTextureManager::getFetchedTexture(!normalMapId.isNull() ? normalMapId : pwater->GetDefaultWaterNormalAssetId());
    mWaterNormp[1] = LLViewerTextureManager::getFetchedTexture(!nextNormalMapId.isNull() ? nextNormalMapId : (!normalMapId.isNull() ? normalMapId : pwater->GetDefaultWaterNormalAssetId()));
    mWaterNormp[0]->addTextureStats(1024.f*1024.f);
    mWaterNormp[1]->addTextureStats(1024.f*1024.f);
}

//static
void LLDrawPoolWater::restoreGL()
{
	/*LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();
    if (pwater)
    {
        setTransparentTextures(pwater->getTransparentTextureID(), pwater->getNextTransparentTextureID());
        setOpaqueTexture(pwater->GetDefaultOpaqueTextureAssetId());
        setNormalMaps(pwater->getNormalMapID(), pwater->getNextNormalMapID());
    }*/
}

void LLDrawPoolWater::prerender()
{
	mShaderLevel = LLCubeMap::sUseCubeMaps ? LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_WATER) : 0;
}

S32 LLDrawPoolWater::getNumPasses()
{
	if (LLViewerCamera::getInstance()->getOrigin().mV[2] < 1024.f)
	{
		return 1;
	}

	return 0;
}

void LLDrawPoolWater::beginPostDeferredPass(S32 pass)
{
	beginRenderPass(pass);
	deferred_render = TRUE;
}

void LLDrawPoolWater::endPostDeferredPass(S32 pass)
{
	endRenderPass(pass);
	deferred_render = FALSE;
}

//===============================
//DEFERRED IMPLEMENTATION
//===============================
void LLDrawPoolWater::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_WATER);

    if (!LLPipeline::sRenderTransparentWater)
    {
        // Will render opaque water without use of ALM
        render(pass);
        return;
    }

	deferred_render = TRUE;
	renderWater();
	deferred_render = FALSE;
}

//=========================================

void LLDrawPoolWater::render(S32 pass)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_WATER);
	if (mDrawFace.empty() || LLDrawable::getCurrentFrame() <= 1)
	{
		return;
	}

	//do a quick 'n dirty depth sort
	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
			 iter != mDrawFace.end(); iter++)
	{
		LLFace* facep = *iter;
		facep->mDistance = -facep->mCenterLocal.mV[2];
	}

	std::sort(mDrawFace.begin(), mDrawFace.end(), LLFace::CompareDistanceGreater());

	// See if we are rendering water as opaque or not
	if (!LLPipeline::sRenderTransparentWater)
	{
		// render water for low end hardware
		renderOpaqueLegacyWater();
		return;
	}

	LLGLEnable blend(GL_BLEND);

	if ((mShaderLevel > 0) && !sSkipScreenCopy)
	{
		renderWater();
		return;
	}

	LLVOSky *voskyp = gSky.mVOSkyp;

	stop_glerror();

	LLFace* refl_face = voskyp->getReflFace();

	gPipeline.disableLights();
	
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	LLGLDisable cullFace(GL_CULL_FACE);
	
	// Set up second pass first
	gGL.getTexUnit(1)->activate();
	gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(1)->bind(mWaterImagep[0]) ;

    gGL.getTexUnit(2)->activate();
	gGL.getTexUnit(2)->enable(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(2)->bind(mWaterImagep[1]) ;

	LLVector3 camera_up = LLViewerCamera::getInstance()->getUpAxis();
	F32 up_dot = camera_up * LLVector3::z_axis;

	LLColor4 water_color;
	if (LLViewerCamera::getInstance()->cameraUnderWater())
	{
		water_color.setVec(1.f, 1.f, 1.f, 0.4f);
	}
	else
	{
		water_color.setVec(1.f, 1.f, 1.f, 0.5f*(1.f + up_dot));
	}

	gGL.diffuseColor4fv(water_color.mV);

	// Automatically generate texture coords for detail map
	glEnable(GL_TEXTURE_GEN_S); //texture unit 1
	glEnable(GL_TEXTURE_GEN_T); //texture unit 1
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	// Slowly move over time.
	F32 offset = fmod(gFrameTimeSeconds*2.f, 100.f);
	F32 tp0[4] = {16.f/256.f, 0.0f, 0.0f, offset*0.01f};
	F32 tp1[4] = {0.0f, 16.f/256.f, 0.0f, offset*0.01f};
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1);

	gGL.getTexUnit(0)->activate();
	
	glClearStencil(1);
	glClear(GL_STENCIL_BUFFER_BIT);
	LLGLEnable gls_stencil(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_REPLACE, GL_KEEP);
	glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);

	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		LLFace *face = *iter;
		if (voskyp->isReflFace(face))
		{
			continue;
		}
		gGL.getTexUnit(0)->bind(face->getTexture());
		face->renderIndexed();
	}

	// Now, disable texture coord generation on texture state 1
	gGL.getTexUnit(1)->activate();
	gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(1)->disable();

    glDisable(GL_TEXTURE_GEN_S); //texture unit 1
	glDisable(GL_TEXTURE_GEN_T); //texture unit 1

    gGL.getTexUnit(2)->activate();
	gGL.getTexUnit(2)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(2)->disable();

	glDisable(GL_TEXTURE_GEN_S); //texture unit 1
	glDisable(GL_TEXTURE_GEN_T); //texture unit 1

	// Disable texture coordinate and color arrays
	gGL.getTexUnit(0)->activate();
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	stop_glerror();
	
	if (gSky.mVOSkyp->getCubeMap())
	{
		gSky.mVOSkyp->getCubeMap()->enable(0);
		gSky.mVOSkyp->getCubeMap()->bind();

		gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.loadIdentity();
		LLMatrix4 camera_mat = LLViewerCamera::getInstance()->getModelview();
		LLMatrix4 camera_rot(camera_mat.getMat3());
		camera_rot.invert();

		gGL.loadMatrix((F32 *)camera_rot.mMatrix);

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		LLOverrideFaceColor overrid(this, 1.f, 1.f, 1.f,  0.5f*up_dot);

		for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
			 iter != mDrawFace.end(); iter++)
		{
			LLFace *face = *iter;
			if (voskyp->isReflFace(face))
			{
				//refl_face = face;
				continue;
			}

			if (face->getGeomCount() > 0)
			{					
				face->renderIndexed();
			}
		}

		gSky.mVOSkyp->getCubeMap()->disable();
		
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
		gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		
	}

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    if (refl_face)
	{
		glStencilFunc(GL_NOTEQUAL, 0, 0xFFFFFFFF);
		renderReflection(refl_face);
	}
}

// for low end hardware
void LLDrawPoolWater::renderOpaqueLegacyWater()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLVOSky *voskyp = gSky.mVOSkyp;

    if (voskyp == NULL)
    {
        return;
    }

	LLGLSLShader* shader = NULL;
	if (LLPipeline::sUnderWaterRender)
	{
		shader = &gObjectSimpleNonIndexedTexGenWaterProgram;
	}
	else
	{
		shader = &gObjectSimpleNonIndexedTexGenProgram;
	}

	shader->bind();

	stop_glerror();

	// Depth sorting and write to depth buffer
	// since this is opaque, we should see nothing
	// behind the water.  No blending because
	// of no transparency.  And no face culling so
	// that the underside of the water is also opaque.
	LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE);
	LLGLDisable no_cull(GL_CULL_FACE);
	LLGLDisable no_blend(GL_BLEND);

	gPipeline.disableLights();

	// Activate the texture binding and bind one
	// texture since all images will have the same texture
	gGL.getTexUnit(0)->activate();
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(0)->bind(mOpaqueWaterImagep);

	// Automatically generate texture coords for water texture
	if (!shader)
	{
		glEnable(GL_TEXTURE_GEN_S); //texture unit 0
		glEnable(GL_TEXTURE_GEN_T); //texture unit 0
		glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	}

	// Use the fact that we know all water faces are the same size
	// to save some computation

	// Slowly move texture coordinates over time so the watter appears
	// to be moving.
	F32 movement_period_secs = 50.f;

	F32 offset = fmod(gFrameTimeSeconds, movement_period_secs);

	if (movement_period_secs != 0)
	{
	 	offset /= movement_period_secs;
	}
	else
	{
		offset = 0;
	}

	F32 tp0[4] = { 16.f / 256.f, 0.0f, 0.0f, offset };
	F32 tp1[4] = { 0.0f, 16.f / 256.f, 0.0f, offset };

	if (!shader)
	{
		glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0);
		glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1);
	}
	else
	{
		shader->uniform4fv(LLShaderMgr::OBJECT_PLANE_S, 1, tp0);
		shader->uniform4fv(LLShaderMgr::OBJECT_PLANE_T, 1, tp1);
	}

	gGL.diffuseColor3f(1.f, 1.f, 1.f);

	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		LLFace *face = *iter;
		if (voskyp->isReflFace(face))
		{
			continue;
		}

		face->renderIndexed();
	}

	stop_glerror();

	if (!shader)
	{
		// Reset the settings back to expected values
		glDisable(GL_TEXTURE_GEN_S); //texture unit 0
		glDisable(GL_TEXTURE_GEN_T); //texture unit 0
	}

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}


void LLDrawPoolWater::renderReflection(LLFace* face)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
	LLVOSky *voskyp = gSky.mVOSkyp;

	if (!voskyp)
	{
		return;
	}

	if (!face->getGeomCount())
	{
		return;
	}
	
	S8 dr = voskyp->getDrawRefl();
	if (dr < 0)
	{
		return;
	}

	LLGLSNoFog noFog;

	gGL.getTexUnit(0)->bind((dr == 0) ? voskyp->getSunTex() : voskyp->getMoonTex());

	LLOverrideFaceColor override(this, LLColor4(face->getFaceColor().mV));
	face->renderIndexed();
}

void LLDrawPoolWater::renderWater()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    if (!deferred_render)
    {
        gGL.setColorMask(true, true);
    }

    LLGLDisable blend(GL_BLEND);

    LLColor3 light_diffuse(0, 0, 0);
    F32      light_exp = 0.0f;

    LLEnvironment &        environment     = LLEnvironment::instance();
    LLSettingsWater::ptr_t pwater          = environment.getCurrentWater();
    LLSettingsSky::ptr_t   psky            = environment.getCurrentSky();
    LLVector3              light_dir       = environment.getLightDirection();
    bool                   sun_up          = environment.getIsSunUp();
    bool                   moon_up         = environment.getIsMoonUp();
    bool                   has_normal_mips = gSavedSettings.getBOOL("RenderWaterMipNormal");
    bool                   underwater      = LLViewerCamera::getInstance()->cameraUnderWater();

    if (sun_up)
    {
        light_diffuse += psky->getSunlightColor();
    }
    // moonlight is several orders of magnitude less bright than sunlight,
    // so only use this color when the moon alone is showing
    else if (moon_up)
    {
        light_diffuse += psky->getMoonlightColor();
    }

    // Apply magic numbers translating light direction into intensities
    light_dir.normalize();
    F32 ground_proj_sq = light_dir.mV[0] * light_dir.mV[0] + light_dir.mV[1] * light_dir.mV[1];
    light_exp          = llmax(32.f, 256.f * powf(ground_proj_sq, 16.0f));
    if (0.f < light_diffuse.normalize())  // Normalizing a color? Puzzling...
    {
        light_diffuse *= (1.5f + (6.f * ground_proj_sq));
    }

    // set up normal maps filtering
    for (auto norm_map : mWaterNormp)
    {
        if (norm_map) norm_map->setFilteringOption(has_normal_mips ? LLTexUnit::TFO_ANISOTROPIC : LLTexUnit::TFO_POINT);
    }

    LLColor4      specular(sun_up ? psky->getSunlightColor() : psky->getMoonlightColor());
    F32           phase_time = (F32) LLFrameTimer::getElapsedSeconds() * 0.5f;
    LLGLSLShader *shader     = nullptr;

    // two passes, first with standard water shader bound, second with edge water shader bound
    for( int edge = 0 ; edge < 2; edge++ )
    {
        // select shader
        if (underwater && LLPipeline::sWaterReflections)
        {
            shader = deferred_render ? &gDeferredUnderWaterProgram : &gUnderWaterProgram;
        }
        else
        {
            if (edge && !deferred_render)
            {
                shader = &gWaterEdgeProgram;
            }
            else
            {
                shader = deferred_render ? &gDeferredWaterProgram : &gWaterProgram;
            }
        }
        shader->bind();

        // bind textures for water rendering
        S32 reftex = shader->enableTexture(LLShaderMgr::WATER_REFTEX);
        if (reftex > -1)
        {
            gGL.getTexUnit(reftex)->activate();
            gGL.getTexUnit(reftex)->bind(&gPipeline.mWaterRef);
            gGL.getTexUnit(0)->activate();
        }

        // bind normal map
        S32 bumpTex  = shader->enableTexture(LLViewerShaderMgr::BUMP_MAP);
        S32 bumpTex2 = shader->enableTexture(LLViewerShaderMgr::BUMP_MAP2);

        LLViewerTexture *tex_a = mWaterNormp[0];
        LLViewerTexture *tex_b = mWaterNormp[1];

        F32 blend_factor = pwater->getBlendFactor();

        gGL.getTexUnit(bumpTex)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(bumpTex2)->unbind(LLTexUnit::TT_TEXTURE);

        if (tex_a && (!tex_b || (tex_a == tex_b)))
        {
            gGL.getTexUnit(bumpTex)->bind(tex_a);
            blend_factor = 0;  // only one tex provided, no blending
        }
        else if (tex_b && !tex_a)
        {
            gGL.getTexUnit(bumpTex)->bind(tex_b);
            blend_factor = 0;  // only one tex provided, no blending
        }
        else if (tex_b != tex_a)
        {
            gGL.getTexUnit(bumpTex)->bind(tex_a);
            gGL.getTexUnit(bumpTex2)->bind(tex_b);
        }

        // bind reflection texture from RenderTarget
        S32 screentex   = shader->enableTexture(LLShaderMgr::WATER_SCREENTEX);
        F32 screenRes[] = {1.f / gGLViewport[2], 1.f / gGLViewport[3]};

        S32 diffTex = shader->enableTexture(LLShaderMgr::DIFFUSE_MAP);

        // set uniforms for shader
        if (deferred_render)
        {
            if (shader->getUniformLocation(LLShaderMgr::DEFERRED_NORM_MATRIX) >= 0)
            {
                glh::matrix4f norm_mat = get_current_modelview().inverse().transpose();
                shader->uniformMatrix4fv(LLShaderMgr::DEFERRED_NORM_MATRIX, 1, FALSE, norm_mat.m);
            }
        }

        shader->uniform2fv(LLShaderMgr::DEFERRED_SCREEN_RES, 1, screenRes);
        shader->uniform1f(LLShaderMgr::BLEND_FACTOR, blend_factor);

        LLColor4 fog_color(pwater->getWaterFogColor(), 0.0f);
        F32      fog_density = pwater->getModifiedWaterFogDensity(underwater);

        if (screentex > -1)
        {
            shader->uniform1f(LLShaderMgr::WATER_FOGDENSITY, fog_density);
            gGL.getTexUnit(screentex)->bind(&gPipeline.mWaterDis);
        }

        if (mShaderLevel == 1)
        {
            fog_color.mV[VW] = log(fog_density) / log(2);
        }

        F32 water_height  = environment.getWaterHeight();
        F32 camera_height = LLViewerCamera::getInstance()->getOrigin().mV[2];
        shader->uniform1f(LLShaderMgr::WATER_WATERHEIGHT, camera_height - water_height);
        shader->uniform1f(LLShaderMgr::WATER_TIME, phase_time);
        shader->uniform3fv(LLShaderMgr::WATER_EYEVEC, 1, LLViewerCamera::getInstance()->getOrigin().mV);

        shader->uniform4fv(LLShaderMgr::SPECULAR_COLOR, 1, specular.mV);
        shader->uniform4fv(LLShaderMgr::WATER_FOGCOLOR, 1, fog_color.mV);

        shader->uniform3fv(LLShaderMgr::WATER_SPECULAR, 1, light_diffuse.mV);
        shader->uniform1f(LLShaderMgr::WATER_SPECULAR_EXP, light_exp);

        shader->uniform2fv(LLShaderMgr::WATER_WAVE_DIR1, 1, pwater->getWave1Dir().mV);
        shader->uniform2fv(LLShaderMgr::WATER_WAVE_DIR2, 1, pwater->getWave2Dir().mV);

        shader->uniform3fv(LLShaderMgr::WATER_LIGHT_DIR, 1, light_dir.mV);

        shader->uniform3fv(LLShaderMgr::WATER_NORM_SCALE, 1, pwater->getNormalScale().mV);
        shader->uniform1f(LLShaderMgr::WATER_FRESNEL_SCALE, pwater->getFresnelScale());
        shader->uniform1f(LLShaderMgr::WATER_FRESNEL_OFFSET, pwater->getFresnelOffset());
        shader->uniform1f(LLShaderMgr::WATER_BLUR_MULTIPLIER, pwater->getBlurMultiplier());

        F32 sunAngle    = llmax(0.f, light_dir.mV[1]);
        F32 scaledAngle = 1.f - sunAngle;

        shader->uniform1i(LLShaderMgr::SUN_UP_FACTOR, sun_up ? 1 : 0);
        shader->uniform1f(LLShaderMgr::WATER_SUN_ANGLE, sunAngle);
        shader->uniform1f(LLShaderMgr::WATER_SCALED_ANGLE, scaledAngle);
        shader->uniform1f(LLShaderMgr::WATER_SUN_ANGLE2, 0.1f + 0.2f * sunAngle);
        shader->uniform1i(LLShaderMgr::WATER_EDGE_FACTOR, edge ? 1 : 0);

        LLVector4 rotated_light_direction = LLEnvironment::instance().getRotatedLightNorm();
        shader->uniform4fv(LLViewerShaderMgr::LIGHTNORM, 1, rotated_light_direction.mV);
        shader->uniform3fv(LLShaderMgr::WL_CAMPOSLOCAL, 1, LLViewerCamera::getInstance()->getOrigin().mV);

        if (LLViewerCamera::getInstance()->cameraUnderWater())
        {
            shader->uniform1f(LLShaderMgr::WATER_REFSCALE, pwater->getScaleBelow());
        }
        else
        {
            shader->uniform1f(LLShaderMgr::WATER_REFSCALE, pwater->getScaleAbove());
        }

        LLGLDisable cullface(GL_CULL_FACE);

        LLVOWater *water = nullptr;
        for (LLFace *const &face : mDrawFace)
        {
            if (!face) continue;
            water = static_cast<LLVOWater *>(face->getViewerObject());
            if (!water) continue;

            gGL.getTexUnit(diffTex)->bind(face->getTexture());

            if ((bool)edge == (bool) water->getIsEdgePatch())
            {
                face->renderIndexed();

                // Note non-void water being drawn, updates required
                if (!edge)  // SL-16461 remove !LLPipeline::sUseOcclusion check
                {
                    sNeedsReflectionUpdate = TRUE;
                    sNeedsDistortionUpdate = TRUE;
                }
            }
        }

        shader->disableTexture(LLShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
        shader->disableTexture(LLShaderMgr::WATER_SCREENTEX);
        shader->disableTexture(LLShaderMgr::BUMP_MAP);
        shader->disableTexture(LLShaderMgr::DIFFUSE_MAP);
        shader->disableTexture(LLShaderMgr::WATER_REFTEX);
        shader->disableTexture(LLShaderMgr::WATER_SCREENDEPTH);

        // clean up
        shader->unbind();
        gGL.getTexUnit(bumpTex)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(bumpTex2)->unbind(LLTexUnit::TT_TEXTURE);
    }

    gGL.getTexUnit(0)->activate();
    gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
    if (!deferred_render)
    {
        gGL.setColorMask(true, false);
    }
}

LLViewerTexture *LLDrawPoolWater::getDebugTexture()
{
	return LLViewerFetchedTexture::sSmokeImagep;
}

LLColor3 LLDrawPoolWater::getDebugColor() const
{
	return LLColor3(0.f, 1.f, 1.f);
}
