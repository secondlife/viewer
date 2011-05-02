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
#include "llwlparammanager.h"
#include "llsky.h"
#include "llvowlsky.h"
#include "llviewerregion.h"
#include "llface.h"
#include "llrender.h"

LLPointer<LLViewerTexture> LLDrawPoolWLSky::sCloudNoiseTexture = NULL;

LLPointer<LLImageRaw> LLDrawPoolWLSky::sCloudNoiseRawImage = NULL;



LLDrawPoolWLSky::LLDrawPoolWLSky(void) :
	LLDrawPool(POOL_WL_SKY)
{
	const std::string cloudNoiseFilename(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight", "clouds2.tga"));
	llinfos << "loading WindLight cloud noise from " << cloudNoiseFilename << llendl;

	LLPointer<LLImageFormatted> cloudNoiseFile(LLImageFormatted::createFromExtension(cloudNoiseFilename));

	if(cloudNoiseFile.isNull()) {
		llerrs << "Error: Failed to load cloud noise image " << cloudNoiseFilename << llendl;
	}

	cloudNoiseFile->load(cloudNoiseFilename);

	sCloudNoiseRawImage = new LLImageRaw();

	cloudNoiseFile->decode(sCloudNoiseRawImage, 0.0f);

	sCloudNoiseTexture = LLViewerTextureManager::getLocalTexture(sCloudNoiseRawImage.get(), TRUE);

	LLWLParamManager::instance()->propagateParameters();
}

LLDrawPoolWLSky::~LLDrawPoolWLSky()
{
	//llinfos << "destructing wlsky draw pool." << llendl;
	sCloudNoiseTexture = NULL;
	sCloudNoiseRawImage = NULL;
}

LLViewerTexture *LLDrawPoolWLSky::getDebugTexture()
{
	return NULL;
}

void LLDrawPoolWLSky::beginRenderPass( S32 pass )
{
}

void LLDrawPoolWLSky::endRenderPass( S32 pass )
{
}

void LLDrawPoolWLSky::renderDome(F32 camHeightLocal, LLGLSLShader * shader) const
{
	LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();

	llassert_always(NULL != shader);

	glPushMatrix();

	//chop off translation
	if (LLPipeline::sReflectionRender && origin.mV[2] > 256.f)
	{
		glTranslatef(origin.mV[0], origin.mV[1], 256.f-origin.mV[2]*0.5f);
	}
	else
	{
		glTranslatef(origin.mV[0], origin.mV[1], origin.mV[2]);
	}
		

	// the windlight sky dome works most conveniently in a coordinate system
	// where Y is up, so permute our basis vectors accordingly.
	glRotatef(120.f, 1.f / F_SQRT3, 1.f / F_SQRT3, 1.f / F_SQRT3);

	glScalef(0.333f, 0.333f, 0.333f);

	glTranslatef(0.f,-camHeightLocal, 0.f);
	
	// Draw WL Sky	
	shader->uniform3f("camPosLocal", 0.f, camHeightLocal, 0.f);

	gSky.mVOWLSkyp->drawDome();

	glPopMatrix();
}

void LLDrawPoolWLSky::renderSkyHaze(F32 camHeightLocal) const
{
	if (gPipeline.canUseWindLightShaders() && gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
	{
		LLGLSLShader* shader =
			LLPipeline::sUnderWaterRender ?
				&gObjectSimpleWaterProgram :
				&gWLSkyProgram;

		LLGLDisable blend(GL_BLEND);

		shader->bind();

		/// Render the skydome
		renderDome(camHeightLocal, shader);	

		shader->unbind();
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
	bool error;
	LLColor4 star_alpha(LLColor4::black);
	star_alpha.mV[3] = LLWLParamManager::instance()->mCurParams.getFloat("star_brightness", error) / 2.f;
	llassert_always(!error);

	gGL.getTexUnit(0)->bind(gSky.mVOSkyp->getBloomTex());

	gGL.pushMatrix();
	glRotatef(gFrameTimeSeconds*0.01f, 0.f, 0.f, 1.f);
	// gl_FragColor.rgb = gl_Color.rgb;
	// gl_FragColor.a = gl_Color.a * star_alpha.a;
	gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_MULT, LLTexUnit::TBS_TEX_COLOR, LLTexUnit::TBS_VERT_COLOR);
	gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_MULT_X2, LLTexUnit::TBS_CONST_ALPHA, LLTexUnit::TBS_TEX_ALPHA);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, star_alpha.mV);

	gSky.mVOWLSkyp->drawStars();

	gGL.popMatrix();
	
	// and disable the combiner states
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
}

void LLDrawPoolWLSky::renderSkyClouds(F32 camHeightLocal) const
{
	if (gPipeline.canUseWindLightShaders() && gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS))
	{
		LLGLSLShader* shader =
			LLPipeline::sUnderWaterRender ?
				&gObjectSimpleWaterProgram :
				&gWLCloudProgram;

		LLGLEnable blend(GL_BLEND);
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);

		gGL.getTexUnit(0)->bind(sCloudNoiseTexture);

		shader->bind();

		/// Render the skydome
		renderDome(camHeightLocal, shader);

		shader->unbind();
	}
}

void LLDrawPoolWLSky::renderHeavenlyBodies()
{
	LLGLSPipelineSkyBox gls_skybox;
	LLGLEnable blend_on(GL_BLEND);
	gPipeline.disableLights();

#if 0 // when we want to re-add a texture sun disc, here's where to do it.
	LLFace * face = gSky.mVOSkyp->mFace[LLVOSky::FACE_SUN];
	if (gSky.mVOSkyp->getSun().getDraw() && face->getGeomCount())
	{
		LLViewerTexture * tex  = face->getTexture();
		gGL.getTexUnit(0)->bind(tex);
		LLColor4 color(gSky.mVOSkyp->getSun().getInterpColor());
		LLFacePool::LLOverrideFaceColor color_override(this, color);
		face->renderIndexed();
	}
#endif

	LLFace * face = gSky.mVOSkyp->mFace[LLVOSky::FACE_MOON];

	if (gSky.mVOSkyp->getMoon().getDraw() && face->getGeomCount())
	{
		// *NOTE: even though we already bound this texture above for the
		// stars register combiners, we bind again here for defensive reasons,
		// since LLImageGL::bind detects that it's a noop, and optimizes it out.
		gGL.getTexUnit(0)->bind(face->getTexture());
		LLColor4 color(gSky.mVOSkyp->getMoon().getInterpColor());
		F32 a = gSky.mVOSkyp->getMoon().getDirection().mV[2];
		if (a > 0.f)
		{
			a = a*a*4.f;
		}
			
		color.mV[3] = llclamp(a, 0.f, 1.f);
		
		LLFacePool::LLOverrideFaceColor color_override(this, color);
		face->renderIndexed();
	}
}

void LLDrawPoolWLSky::render(S32 pass)
{
	if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
	{
		return;
	}
	LLFastTimer ftm(FTM_RENDER_WL_SKY);

	const F32 camHeightLocal = LLWLParamManager::instance()->getDomeOffset() * LLWLParamManager::instance()->getDomeRadius();

	LLGLSNoFog disableFog;
	LLGLDepthTest depth(GL_TRUE, GL_FALSE);
	LLGLDisable clip(GL_CLIP_PLANE0);

	LLGLSquashToFarClip far_clip(glh_get_current_projection());

	renderSkyHaze(camHeightLocal);

	LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();
	glPushMatrix();

		glTranslatef(origin.mV[0], origin.mV[1], origin.mV[2]);

		// *NOTE: have to bind a texture here since register combiners blending in
		// renderStars() requires something to be bound and we might as well only
		// bind the moon's texture once.		
		gGL.getTexUnit(0)->bind(gSky.mVOSkyp->mFace[LLVOSky::FACE_MOON]->getTexture());

		renderHeavenlyBodies();

		renderStars();
		

	glPopMatrix();

	renderSkyClouds(camHeightLocal);

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

void LLDrawPoolWLSky::prerender()
{
	//llinfos << "wlsky prerendering pass." << llendl;
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
	sCloudNoiseTexture = NULL;
}

//static
void LLDrawPoolWLSky::restoreGL()
{
	sCloudNoiseTexture = LLViewerTextureManager::getLocalTexture(sCloudNoiseRawImage.get(), TRUE);
}
