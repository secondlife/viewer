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
#include "llwaterparammanager.h"

const LLUUID TRANSPARENT_WATER_TEXTURE("2bfd3884-7e27-69b9-ba3a-3e673f680004");
const LLUUID OPAQUE_WATER_TEXTURE("43c32285-d658-1793-c123-bf86315de055");

static float sTime;

BOOL deferred_render = FALSE;

BOOL LLDrawPoolWater::sSkipScreenCopy = FALSE;
BOOL LLDrawPoolWater::sNeedsReflectionUpdate = TRUE;
BOOL LLDrawPoolWater::sNeedsDistortionUpdate = TRUE;
LLColor4 LLDrawPoolWater::sWaterFogColor = LLColor4(0.2f, 0.5f, 0.5f, 0.f);
F32 LLDrawPoolWater::sWaterFogEnd = 0.f;

LLVector3 LLDrawPoolWater::sLightDir;

LLDrawPoolWater::LLDrawPoolWater() :
	LLFacePool(POOL_WATER)
{
	mHBTex[0] = LLViewerTextureManager::getFetchedTexture(gSunTextureID, TRUE, LLGLTexture::BOOST_UI);
	gGL.getTexUnit(0)->bind(mHBTex[0]) ;
	mHBTex[0]->setAddressMode(LLTexUnit::TAM_CLAMP);

	mHBTex[1] = LLViewerTextureManager::getFetchedTexture(gMoonTextureID, TRUE, LLGLTexture::BOOST_UI);
	gGL.getTexUnit(0)->bind(mHBTex[1]);
	mHBTex[1]->setAddressMode(LLTexUnit::TAM_CLAMP);


	mWaterImagep = LLViewerTextureManager::getFetchedTexture(TRANSPARENT_WATER_TEXTURE);
	llassert(mWaterImagep);
	mWaterImagep->setNoDelete();
	mOpaqueWaterImagep = LLViewerTextureManager::getFetchedTexture(OPAQUE_WATER_TEXTURE);
	llassert(mOpaqueWaterImagep);
	mWaterNormp = LLViewerTextureManager::getFetchedTexture(DEFAULT_WATER_NORMAL);
	mWaterNormp->setNoDelete();

	restoreGL();
}

LLDrawPoolWater::~LLDrawPoolWater()
{
}

//static
void LLDrawPoolWater::restoreGL()
{
	
}

LLDrawPool *LLDrawPoolWater::instancePool()
{
	llerrs << "Should never be calling instancePool on a water pool!" << llendl;
	return NULL;
}


void LLDrawPoolWater::prerender()
{
	mVertexShaderLevel = (gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps) ?
		LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_WATER) : 0;

	// got rid of modulation by light color since it got a little too
	// green at sunset and sl-57047 (underwater turns black at 8:00)
	sWaterFogColor = LLWaterParamManager::instance().getFogColor();
	sWaterFogColor.mV[3] = 0;

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
	LLFastTimer t(FTM_RENDER_WATER);
	deferred_render = TRUE;
	shade();
	deferred_render = FALSE;
}

//=========================================

void LLDrawPoolWater::render(S32 pass)
{
	LLFastTimer ftm(FTM_RENDER_WATER);
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
	if (!gSavedSettings.getBOOL("RenderTransparentWater"))
	{
		// render water for low end hardware
		renderOpaqueLegacyWater();
		return;
	}

	LLGLEnable blend(GL_BLEND);

	if ((mVertexShaderLevel > 0) && !sSkipScreenCopy)
	{
		shade();
		return;
	}

	LLVOSky *voskyp = gSky.mVOSkyp;

	stop_glerror();

	if (!gGLManager.mHasMultitexture)
	{
		// Ack!  No multitexture!  Bail!
		return;
	}

	LLFace* refl_face = voskyp->getReflFace();

	gPipeline.disableLights();
	
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	LLGLDisable cullFace(GL_CULL_FACE);
	
	// Set up second pass first
	mWaterImagep->addTextureStats(1024.f*1024.f);
	gGL.getTexUnit(1)->activate();
	gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(1)->bind(mWaterImagep) ;

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

	gGL.getTexUnit(1)->setTextureColorBlend(LLTexUnit::TBO_MULT, LLTexUnit::TBS_TEX_COLOR, LLTexUnit::TBS_PREV_COLOR);
	gGL.getTexUnit(1)->setTextureAlphaBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_PREV_ALPHA);

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

		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);

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

		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);

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

	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
}

// for low end hardware
void LLDrawPoolWater::renderOpaqueLegacyWater()
{
	LLVOSky *voskyp = gSky.mVOSkyp;

	LLGLSLShader* shader = NULL;
	if (LLGLSLShader::sNoFixedFunction)
	{
		if (LLPipeline::sUnderWaterRender)
		{
			shader = &gObjectSimpleNonIndexedTexGenWaterProgram;
		}
		else
		{
			shader = &gObjectSimpleNonIndexedTexGenProgram;
		}

		shader->bind();
	}

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

	mOpaqueWaterImagep->addTextureStats(1024.f*1024.f);

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
		shader->uniform4fv("object_plane_s", 1, tp0);
		shader->uniform4fv("object_plane_t", 1, tp1);
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
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
}


void LLDrawPoolWater::renderReflection(LLFace* face)
{
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

	gGL.getTexUnit(0)->bind(mHBTex[dr]);

	LLOverrideFaceColor override(this, face->getFaceColor().mV);
	face->renderIndexed();
}

void LLDrawPoolWater::shade()
{
	if (!deferred_render)
	{
		gGL.setColorMask(true, true);
	}

	LLVOSky *voskyp = gSky.mVOSkyp;

	if(voskyp == NULL) 
	{
		return;
	}

	LLGLDisable blend(GL_BLEND);

	LLColor3 light_diffuse(0,0,0);
	F32 light_exp = 0.0f;
	LLVector3 light_dir;
	LLColor3 light_color;

	if (gSky.getSunDirection().mV[2] > LLSky::NIGHTTIME_ELEVATION_COS) 	 
    { 	 
        light_dir  = gSky.getSunDirection(); 	 
        light_dir.normVec(); 	
		light_color = gSky.getSunDiffuseColor();
		if(gSky.mVOSkyp) {
	        light_diffuse = gSky.mVOSkyp->getSun().getColorCached(); 	 
			light_diffuse.normVec(); 	 
		}
        light_exp = light_dir * LLVector3(light_dir.mV[0], light_dir.mV[1], 0); 	 
        light_diffuse *= light_exp + 0.25f; 	 
    } 	 
    else  	 
    { 	 
        light_dir       = gSky.getMoonDirection(); 	 
        light_dir.normVec(); 	 
		light_color = gSky.getMoonDiffuseColor();
        light_diffuse   = gSky.mVOSkyp->getMoon().getColorCached(); 	 
        light_diffuse.normVec(); 	 
        light_diffuse *= 0.5f; 	 
        light_exp = light_dir * LLVector3(light_dir.mV[0], light_dir.mV[1], 0); 	 
    }

	light_exp *= light_exp;
	light_exp *= light_exp;
	light_exp *= light_exp;
	light_exp *= light_exp;
	light_exp *= 256.f;
	light_exp = light_exp > 32.f ? light_exp : 32.f;

	LLGLSLShader* shader;

	F32 eyedepth = LLViewerCamera::getInstance()->getOrigin().mV[2] - gAgent.getRegion()->getWaterHeight();
	
	if (deferred_render)
	{
		shader = &gDeferredWaterProgram;
	}
	else if (eyedepth < 0.f && LLPipeline::sWaterReflections)
	{
		shader = &gUnderWaterProgram;
	}
	else
	{
		shader = &gWaterProgram;
	}

	if (deferred_render)
	{
		gPipeline.bindDeferredShader(*shader);
	}
	else
	{
		shader->bind();
	}

	sTime = (F32)LLFrameTimer::getElapsedSeconds()*0.5f;
	
	S32 reftex = shader->enableTexture(LLViewerShaderMgr::WATER_REFTEX);
		
	if (reftex > -1)
	{
		gGL.getTexUnit(reftex)->activate();
		gGL.getTexUnit(reftex)->bind(&gPipeline.mWaterRef);
		gGL.getTexUnit(0)->activate();
	}	

	//bind normal map
	S32 bumpTex = shader->enableTexture(LLViewerShaderMgr::BUMP_MAP);

	LLWaterParamManager * param_mgr = &LLWaterParamManager::instance();

	// change mWaterNormp if needed
	if (mWaterNormp->getID() != param_mgr->getNormalMapID())
	{
		mWaterNormp = LLViewerTextureManager::getFetchedTexture(param_mgr->getNormalMapID());
	}

	mWaterNormp->addTextureStats(1024.f*1024.f);
	gGL.getTexUnit(bumpTex)->bind(mWaterNormp) ;
	if (gSavedSettings.getBOOL("RenderWaterMipNormal"))
	{
		mWaterNormp->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
	}
	else 
	{
		mWaterNormp->setFilteringOption(LLTexUnit::TFO_POINT);
	}
	
	S32 screentex = shader->enableTexture(LLViewerShaderMgr::WATER_SCREENTEX);	
		
	if (screentex > -1)
	{
		shader->uniform4fv(LLViewerShaderMgr::WATER_FOGCOLOR, 1, sWaterFogColor.mV);
		shader->uniform1f(LLViewerShaderMgr::WATER_FOGDENSITY, 
			param_mgr->getFogDensity());
		gPipeline.mWaterDis.bindTexture(0, screentex);
	}
	
	stop_glerror();
	
	gGL.getTexUnit(screentex)->bind(&gPipeline.mWaterDis);	

	if (mVertexShaderLevel == 1)
	{
		sWaterFogColor.mV[3] = param_mgr->mDensitySliderValue;
		shader->uniform4fv(LLViewerShaderMgr::WATER_FOGCOLOR, 1, sWaterFogColor.mV);
	}

	F32 screenRes[] = 
	{
		1.f/gGLViewport[2],
		1.f/gGLViewport[3]
	};
	shader->uniform2fv("screenRes", 1, screenRes);
	stop_glerror();
	
	S32 diffTex = shader->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	stop_glerror();
	
	light_dir.normVec();
	sLightDir = light_dir;
	
	light_diffuse *= 6.f;

	//shader->uniformMatrix4fv("inverse_ref", 1, GL_FALSE, (GLfloat*) gGLObliqueProjectionInverse.mMatrix);
	shader->uniform1f(LLViewerShaderMgr::WATER_WATERHEIGHT, eyedepth);
	shader->uniform1f(LLViewerShaderMgr::WATER_TIME, sTime);
	shader->uniform3fv(LLViewerShaderMgr::WATER_EYEVEC, 1, LLViewerCamera::getInstance()->getOrigin().mV);
	shader->uniform3fv(LLViewerShaderMgr::WATER_SPECULAR, 1, light_diffuse.mV);
	shader->uniform1f(LLViewerShaderMgr::WATER_SPECULAR_EXP, light_exp);
	shader->uniform2fv(LLViewerShaderMgr::WATER_WAVE_DIR1, 1, param_mgr->getWave1Dir().mV);
	shader->uniform2fv(LLViewerShaderMgr::WATER_WAVE_DIR2, 1, param_mgr->getWave2Dir().mV);
	shader->uniform3fv(LLViewerShaderMgr::WATER_LIGHT_DIR, 1, light_dir.mV);

	shader->uniform3fv("normScale", 1, param_mgr->getNormalScale().mV);
	shader->uniform1f("fresnelScale", param_mgr->getFresnelScale());
	shader->uniform1f("fresnelOffset", param_mgr->getFresnelOffset());
	shader->uniform1f("blurMultiplier", param_mgr->getBlurMultiplier());

	F32 sunAngle = llmax(0.f, light_dir.mV[2]);
	F32 scaledAngle = 1.f - sunAngle;

	shader->uniform1f("sunAngle", sunAngle);
	shader->uniform1f("scaledAngle", scaledAngle);
	shader->uniform1f("sunAngle2", 0.1f + 0.2f*sunAngle);

	LLColor4 water_color;
	LLVector3 camera_up = LLViewerCamera::getInstance()->getUpAxis();
	F32 up_dot = camera_up * LLVector3::z_axis;
	if (LLViewerCamera::getInstance()->cameraUnderWater())
	{
		water_color.setVec(1.f, 1.f, 1.f, 0.4f);
		shader->uniform1f(LLViewerShaderMgr::WATER_REFSCALE, param_mgr->getScaleBelow());
	}
	else
	{
		water_color.setVec(1.f, 1.f, 1.f, 0.5f*(1.f + up_dot));
		shader->uniform1f(LLViewerShaderMgr::WATER_REFSCALE, param_mgr->getScaleAbove());
	}

	if (water_color.mV[3] > 0.9f)
	{
		water_color.mV[3] = 0.9f;
	}

	{
		LLGLEnable depth_clamp(gGLManager.mHasDepthClamp ? GL_DEPTH_CLAMP : 0);
		LLGLDisable cullface(GL_CULL_FACE);
		for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
			iter != mDrawFace.end(); iter++)
		{
			LLFace *face = *iter;

			if (voskyp->isReflFace(face))
			{
				continue;
			}

			LLVOWater* water = (LLVOWater*) face->getViewerObject();
			gGL.getTexUnit(diffTex)->bind(face->getTexture());

			sNeedsReflectionUpdate = TRUE;
			
			if (water->getUseTexture() || !water->getIsEdgePatch())
			{
				sNeedsDistortionUpdate = TRUE;
				face->renderIndexed();
			}
			else if (gGLManager.mHasDepthClamp || deferred_render)
			{
				face->renderIndexed();
			}
			else
			{
				LLGLSquashToFarClip far_clip(glh_get_current_projection());
				face->renderIndexed();
			}
		}
	}
	
	shader->disableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
	shader->disableTexture(LLViewerShaderMgr::WATER_SCREENTEX);	
	shader->disableTexture(LLViewerShaderMgr::BUMP_MAP);
	shader->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	shader->disableTexture(LLViewerShaderMgr::WATER_REFTEX);
	shader->disableTexture(LLViewerShaderMgr::WATER_SCREENDEPTH);

	if (deferred_render)
	{
		gPipeline.unbindDeferredShader(*shader);
	}
	else
	{
		shader->unbind();
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
