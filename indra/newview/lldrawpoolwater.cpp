/** 
 * @file lldrawpoolwater.cpp
 * @brief LLDrawPoolWater class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"
#include "llfeaturemanager.h"
#include "lldrawpoolwater.h"

#include "llviewercontrol.h"
#include "lldir.h"
#include "llerror.h"
#include "m3math.h"

#include "llagent.h"		// for gAgent for getRegion for getWaterHeight
#include "llcubemap.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h" // to get OGL_TO_CFR_ROTATION
#include "llviewerimagelist.h"
#include "llviewerregion.h"
#include "llvosky.h"
#include "llvowater.h"
#include "llworld.h"
#include "pipeline.h"
#include "viewer.h"			// gSunTextureID, gMoonTextureID
#include "llglslshader.h"

const LLUUID WATER_TEST("2bfd3884-7e27-69b9-ba3a-3e673f680004");

static float sTime;

BOOL LLDrawPoolWater::sSkipScreenCopy = FALSE;

LLDrawPoolWater::LLDrawPoolWater() :
	LLFacePool(POOL_WATER)
{
	mHBTex[0] = gImageList.getImage(gSunTextureID, TRUE, TRUE);
	mHBTex[0]->bind();
	mHBTex[0]->setClamp(TRUE, TRUE);

	mHBTex[1] = gImageList.getImage(gMoonTextureID, TRUE, TRUE);
	mHBTex[1]->bind();
	mHBTex[1]->setClamp(TRUE, TRUE);

	mWaterImagep = gImageList.getImage(WATER_TEST);
	mWaterNormp = gImageList.getImage(LLUUID(gViewerArt.getString("water_normal.tga")));

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
	mVertexShaderLevel = gSavedSettings.getBOOL("RenderRippleWater") ?
				LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_ENVIRONMENT) : 0;

}

extern LLColor4U MAX_WATER_COLOR;

void LLDrawPoolWater::render(S32 pass)
{
	LLFastTimer ftm(LLFastTimer::FTM_RENDER_WATER);
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

	LLGLEnable blend(GL_BLEND);

	if ((mVertexShaderLevel >= SHADER_LEVEL_RIPPLE))
	{
		shade();
		return;
	}

	if ((mVertexShaderLevel > 0))
	{
		renderShaderSimple();
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
	
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	
	// Set up second pass first
	glActiveTextureARB(GL_TEXTURE1_ARB);
	mWaterImagep->addTextureStats(1024.f*1024.f);
	mWaterImagep->bind(1);

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D); // Texture unit 1

	LLVector3 camera_up = gCamera->getUpAxis();
	F32 up_dot = camera_up * LLVector3::z_axis;

	LLColor4 water_color;
	if (gCamera->cameraUnderWater())
	{
		water_color.setVec(1.f, 1.f, 1.f, 0.4f);
	}
	else
	{
		water_color.setVec(1.f, 1.f, 1.f, 0.5f*(1.f + up_dot));
	}

	glColor4fv(water_color.mV);

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

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	
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
		face->bindTexture();
		face->renderIndexed();
		mIndicesDrawn += face->getIndicesCount();
	}

	// Now, disable texture coord generation on texture state 1
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D); // Texture unit 1
	LLImageGL::unbindTexture(1, GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S); //texture unit 1
	glDisable(GL_TEXTURE_GEN_T); //texture unit 1

	// Disable texture coordinate and color arrays
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	stop_glerror();
	
	if (gSky.mVOSkyp->getCubeMap())
	{
		gSky.mVOSkyp->getCubeMap()->enable(0);
		gSky.mVOSkyp->getCubeMap()->bind();

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		LLMatrix4 camera_mat = gCamera->getModelview();
		LLMatrix4 camera_rot(camera_mat.getMat3());
		camera_rot.invert();

		glLoadMatrixf((F32 *)camera_rot.mMatrix);

		glMatrixMode(GL_MODELVIEW);
		LLOverrideFaceColor overrid(this, 1.f, 1.f, 1.f,  0.5f*up_dot);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		/*glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_ADD);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);*/
		
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
				mIndicesDrawn += face->getIndicesCount();
			}
		}

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		if (gSky.mVOSkyp->getCubeMap())
		{
			gSky.mVOSkyp->getCubeMap()->disable();
		}
		LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_2D);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		
	}

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    if (refl_face)
	{
		glStencilFunc(GL_NOTEQUAL, 0, 0xFFFFFFFF);
		renderReflection(refl_face);
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}


void LLDrawPoolWater::renderShaderSimple()
{
	LLVOSky *voskyp = gSky.mVOSkyp;

	stop_glerror();

	if (!gGLManager.mHasMultitexture)
	{
		// Ack!  No multitexture!  Bail!
		return;
	}

	LLFace* refl_face = voskyp->getReflFace();

	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	LLGLDisable cullFace(GL_CULL_FACE);
	
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	
	// Set up second pass first
	S32 bumpTex = gWaterProgram.enableTexture(LLShaderMgr::BUMP_MAP);
	mWaterImagep->addTextureStats(1024.f*1024.f);
	mWaterImagep->bind(bumpTex);

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	LLVector3 camera_up = gCamera->getUpAxis();
	F32 up_dot = camera_up * LLVector3::z_axis;

	LLColor4 water_color;
	if (gCamera->cameraUnderWater())
	{
		water_color.setVec(1.f, 1.f, 1.f, 0.4f);
	}
	else
	{
		water_color.setVec(1.f, 1.f, 1.f, 0.5f*(1.f + up_dot));
	}

	glColor4fv(water_color.mV);

	// Automatically generate texture coords for detail map
	glActiveTextureARB(GL_TEXTURE1_ARB);
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

	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	
	glClearStencil(1);
	glClear(GL_STENCIL_BUFFER_BIT);
	LLGLEnable gls_stencil(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_REPLACE, GL_KEEP);
	glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);

	S32 envTex = -1;

	if (gSky.mVOSkyp->getCubeMap())
	{
		envTex = gWaterProgram.enableTexture(LLShaderMgr::ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
		gSky.mVOSkyp->getCubeMap()->bind();
		
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		LLMatrix4 camera_mat = gCamera->getModelview();
		LLMatrix4 camera_rot(camera_mat.getMat3());
		camera_rot.invert();

		glLoadMatrixf((F32 *)camera_rot.mMatrix);

		glMatrixMode(GL_MODELVIEW);
	}

	S32 diffTex = gWaterProgram.enableTexture(LLShaderMgr::DIFFUSE_MAP);
    
	gWaterProgram.bind();

	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		LLFace *face = *iter;
		if (voskyp->isReflFace(face))
		{
			continue;
		}
		face->bindTexture(diffTex);
		face->renderIndexed();
		mIndicesDrawn += face->getIndicesCount();
	}
		
	if (gSky.mVOSkyp->getCubeMap())
	{
		gWaterProgram.disableTexture(LLShaderMgr::ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
	
	// Now, disable texture coord generation on texture state 1
	gWaterProgram.disableTexture(LLShaderMgr::BUMP_MAP);
	LLImageGL::unbindTexture(bumpTex, GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_GEN_S); //texture unit 1
	glDisable(GL_TEXTURE_GEN_T); //texture unit 1

	gWaterProgram.disableTexture(LLShaderMgr::DIFFUSE_MAP);
	
	// Disable texture coordinate and color arrays
	LLImageGL::unbindTexture(diffTex, GL_TEXTURE_2D);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	stop_glerror();
	
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glUseProgramObjectARB(0);
	gPipeline.disableLights();

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);

    if (refl_face)
	{
		glStencilFunc(GL_NOTEQUAL, 0, 0xFFFFFFFF);
		renderReflection(refl_face);
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
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

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	LLViewerImage::bindTexture(mHBTex[dr]);

	LLOverrideFaceColor override(this, face->getFaceColor().mV);
	face->renderIndexed();
	mIndicesDrawn += face->getIndicesCount();

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void LLDrawPoolWater::shade()
{
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);

	static LLVector2 d1( 0.5f, -0.17f );
	static LLVector2 d2( 0.58f, -0.67f );
	static LLVector2 d3( 0.5f, 0.25f );

	static LLVector3 wave1(1,0.42f,1);
	static LLVector3 wave2(0.58f,0.42f,0.17f);
	static LLVector3 wave3(0.42f,0.67f,0.33f);

	/*static LLVector2 d1( 0.83f, -1 );
	static LLVector2 d2( 0.58f, 1 );
	static LLVector2 d3( 1, -0.88f );

	static LLVector4 wave1(0.75f,0.08f,0.5f,0.67f);
	static LLVector4 wave2(0.17f,0.33f,0.53f,0.62f);
	static LLVector4 wave3(0.17f,0.6f,0.67f,1);*/

	/*LLDebugVarMessageBox::show("Wave Direction 1", &d1, LLVector2(1,1), LLVector2(0.01f, 0.01f));
	LLDebugVarMessageBox::show("Wave Direction 2", &d2, LLVector2(1,1), LLVector2(0.01f, 0.01f));
	LLDebugVarMessageBox::show("Wave Direction 3", &d3, LLVector2(1,1), LLVector2(0.01f, 0.01f));

	LLDebugVarMessageBox::show("Wave 1", &wave1, LLVector3(2,1,4), LLVector3(0.01f, 0.01f, 0.01f));
	LLDebugVarMessageBox::show("Wave 2", &wave2, LLVector3(2,1,4), LLVector3(0.01f, 0.01f, 0.01f));
	LLDebugVarMessageBox::show("Wave 3", &wave3, LLVector3(2,1,4), LLVector3(0.01f, 0.01f, 0.01f));*/

	LLVOSky *voskyp = gSky.mVOSkyp;

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	LLGLDisable blend(GL_BLEND);

	LLColor3 light_diffuse(0,0,0);
	F32 light_exp = 0.0f;
	LLVector3 light_dir;

	if (gSky.getSunDirection().mV[2] > NIGHTTIME_ELEVATION_COS) 	 
    { 	 
        light_dir  = gSky.getSunDirection(); 	 
        light_dir.normVec(); 	 
        light_diffuse   = gSky.mVOSkyp->getSun().getColorCached(); 	 
        light_diffuse.normVec(); 	 
        light_exp = light_dir * LLVector3(light_dir.mV[0], light_dir.mV[1], 0); 	 
        light_diffuse *= light_exp + 0.25f; 	 
    } 	 
    else  	 
    { 	 
        light_dir       = gSky.getMoonDirection(); 	 
        light_dir.normVec(); 	 
        light_diffuse   = gSky.mVOSkyp->getMoon().getColorCached(); 	 
        light_diffuse.normVec(); 	 
        light_diffuse *= 0.5f; 	 
        light_exp = light_dir * LLVector3(light_dir.mV[0], light_dir.mV[1], 0); 	 
    }

	light_exp *= light_exp;
	light_exp *= light_exp;
	light_exp *= light_exp;
	light_exp *= light_exp;
	light_exp *= light_exp;
	light_exp *= 512.f;
	light_exp = light_exp > 32.f ? light_exp : 32.f;

	sTime = (F32)LLFrameTimer::getElapsedSeconds()*0.5f;
	
	LLCubeMap* skyMap = gSky.mVOSkyp->getCubeMap();
	
	gWaterProgram.enableTexture(LLShaderMgr::ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
    skyMap->bind();

	//bind normal map
	S32 bumpTex = gWaterProgram.enableTexture(LLShaderMgr::BUMP_MAP);
	mWaterNormp->addTextureStats(1024.f*1024.f);
	mWaterNormp->bind(bumpTex);
	
	gWaterProgram.enableTexture(LLShaderMgr::WATER_SCREENTEX);	

	gWaterProgram.bind();
	
	if (!sSkipScreenCopy)
	{
		gPipeline.bindScreenToTexture();
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	glUniform2fvARB(gWaterProgram.mUniform[LLShaderMgr::WATER_FBSCALE], 1, 
			gPipeline.mScreenScale.mV);

	S32 diffTex = gWaterProgram.enableTexture(LLShaderMgr::DIFFUSE_MAP);
	
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	glUniform1fARB(gWaterProgram.mUniform[LLShaderMgr::WATER_TIME], sTime);
	glUniform3fvARB(gWaterProgram.mUniform[LLShaderMgr::WATER_SPECULAR], 1, light_diffuse.mV);
	glUniform1fARB(gWaterProgram.mUniform[LLShaderMgr::WATER_SPECULAR_EXP], light_exp);
	glUniform3fvARB(gWaterProgram.mUniform[LLShaderMgr::WATER_EYEVEC], 1, (GLfloat *)(gCamera->getOrigin().mV));
	glUniform2fvARB(gWaterProgram.mUniform[LLShaderMgr::WATER_WAVE_DIR1], 1, d1.mV);
	glUniform2fvARB(gWaterProgram.mUniform[LLShaderMgr::WATER_WAVE_DIR2], 1, d2.mV);
	glUniform3fvARB(gWaterProgram.mUniform[LLShaderMgr::WATER_LIGHT_DIR], 1, light_dir.mV);

	LLColor4 water_color;
	LLVector3 camera_up = gCamera->getUpAxis();
	F32 up_dot = camera_up * LLVector3::z_axis;
	if (gCamera->cameraUnderWater())
	{
		water_color.setVec(1.f, 1.f, 1.f, 0.4f);
		glUniform1fARB(gWaterProgram.mUniform[LLShaderMgr::WATER_REFSCALE], 0.25f);
	}
	else
	{
		water_color.setVec(1.f, 1.f, 1.f, 0.5f*(1.f + up_dot));
		glUniform1fARB(gWaterProgram.mUniform[LLShaderMgr::WATER_REFSCALE], 0.01f);
	}
	if (water_color.mV[3] > 0.9f)
	{
		water_color.mV[3] = 0.9f;
	}

	glColor4fv(water_color.mV);

	{
		LLGLDisable cullface(GL_CULL_FACE);
		for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
			iter != mDrawFace.end(); iter++)
		{
			LLFace *face = *iter;

			if (voskyp->isReflFace(face))
			{
				continue;
			}

			face->bindTexture(diffTex);
			face->renderIndexed();
			mIndicesDrawn += face->getIndicesCount();
		}
	}
	
	gWaterProgram.disableTexture(LLShaderMgr::ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
	gWaterProgram.disableTexture(LLShaderMgr::WATER_SCREENTEX);
	gWaterProgram.disableTexture(LLShaderMgr::BUMP_MAP);
	gWaterProgram.disableTexture(LLShaderMgr::DIFFUSE_MAP);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glUseProgramObjectARB(0);
	
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_FALSE);
}

void LLDrawPoolWater::renderForSelect()
{
	// Can't select water!
	return;
}


void LLDrawPoolWater::renderFaceSelected(LLFace *facep, 
									LLImageGL *image, 
									const LLColor4 &color,
									const S32 index_offset, const S32 index_count)
{
	// Can't select water
	return;
}


LLViewerImage *LLDrawPoolWater::getDebugTexture()
{
	return LLViewerImage::sSmokeImagep;
}

LLColor3 LLDrawPoolWater::getDebugColor() const
{
	return LLColor3(0.f, 1.f, 1.f);
}
