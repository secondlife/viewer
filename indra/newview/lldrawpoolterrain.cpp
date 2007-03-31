/** 
 * @file lldrawpoolterrain.cpp
 * @brief LLDrawPoolTerrain class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolterrain.h"

#include "llfasttimer.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llsurface.h"
#include "llsurfacepatch.h"
#include "llviewerregion.h"
#include "llvlcomposition.h"
#include "llviewerparcelmgr.h"		// for gRenderParcelOwnership
#include "llviewerparceloverlay.h"
#include "llvosurfacepatch.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h" // To get alpha gradients
#include "llworld.h"
#include "pipeline.h"
#include "llglslshader.h"

const F32 DETAIL_SCALE = 1.f/16.f;
int DebugDetailMap = 0;

S32 LLDrawPoolTerrain::sDetailMode = 1;
F32 LLDrawPoolTerrain::sDetailScale = DETAIL_SCALE;

LLDrawPoolTerrain::LLDrawPoolTerrain(LLViewerImage *texturep) :
	LLFacePool(POOL_TERRAIN),
	mTexturep(texturep)
{
	// Hack!
	sDetailScale = 1.f/gSavedSettings.getF32("RenderTerrainScale");
	sDetailMode = gSavedSettings.getS32("RenderTerrainDetail");
	mAlphaRampImagep = gImageList.getImageFromUUID(LLUUID(gViewerArt.getString("alpha_gradient.tga")),
												   TRUE, TRUE, GL_ALPHA8, GL_ALPHA);
	mAlphaRampImagep->bind(0);
	mAlphaRampImagep->setClamp(TRUE, TRUE);

	m2DAlphaRampImagep = gImageList.getImageFromUUID(LLUUID(gViewerArt.getString("alpha_gradient_2d.tga")),
													 TRUE, TRUE, GL_ALPHA8, GL_ALPHA);
	m2DAlphaRampImagep->bind(0);
	m2DAlphaRampImagep->setClamp(TRUE, TRUE);
	
	mTexturep->setBoostLevel(LLViewerImage::BOOST_TERRAIN);
	
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
}

LLDrawPoolTerrain::~LLDrawPoolTerrain()
{
	llassert( gPipeline.findPool( getType(), getTexture() ) == NULL );
}


LLDrawPool *LLDrawPoolTerrain::instancePool()
{
	return new LLDrawPoolTerrain(mTexturep);
}


void LLDrawPoolTerrain::prerender()
{
#if 0 // 1.9.2
	mVertexShaderLevel = gPipeline.getVertexShaderLevel(LLPipeline::SHADER_ENVIRONMENT);
#endif
	sDetailMode = gSavedSettings.getS32("RenderTerrainDetail");
}

//static
S32 LLDrawPoolTerrain::getDetailMode()
{
	return sDetailMode;
}

void LLDrawPoolTerrain::render(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_TERRAIN);
	
	if (mDrawFace.empty())
	{
		return;
	}

	// Hack! Get the region that this draw pool is rendering from!
	LLViewerRegion *regionp = mDrawFace[0]->getDrawable()->getVObj()->getRegion();
	LLVLComposition *compp = regionp->getComposition();
	for (S32 i = 0; i < 4; i++)
	{
		compp->mDetailTextures[i]->setBoostLevel(LLViewerImage::BOOST_TERRAIN);
		compp->mDetailTextures[i]->addTextureStats(1024.f*1024.f); // assume large pixel area
	}

	if (!gGLManager.mHasMultitexture)
	{
		// No mulititexture, render simple land.
		renderSimple(); // Render without multitexture
		return;
	}
	// Render simplified land if video card can't do sufficient multitexturing
	if (!gGLManager.mHasARBEnvCombine || (gGLManager.mNumTextureUnits < 2))
	{
		renderSimple(); // Render without multitexture
		return;
	}

	LLGLSPipeline gls;
	LLOverrideFaceColor override(this, 1.f, 1.f, 1.f, 1.f);

	if (mVertexShaderLevel > 0)
	{
		gPipeline.enableLightsDynamic(1.f);
		renderFull4TUShader();
	}
	else
	{
		gPipeline.enableLightsStatic(1.f);
		switch (sDetailMode)
		{
		  case 0:
			renderSimple();
			break;
		  default:
			if (gGLManager.mNumTextureUnits < 4)
			{
				renderFull2TU();
			}
			else
			{
				renderFull4TU();
			}
			break;
		}
	}

	// Special-case for land ownership feedback
	if (gSavedSettings.getBOOL("ShowParcelOwners"))
	{
		gPipeline.disableLights();
		if ((mVertexShaderLevel > 0))
		{
			gHighlightProgram.bind();
			gHighlightProgram.vertexAttrib4f(LLShaderMgr::MATERIAL_COLOR,1,1,1,1);
			renderOwnership();
			gTerrainProgram.bind();
		}
		else
		{
			renderOwnership();
		}
	}
}


void LLDrawPoolTerrain::renderFull4TUShader()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	if (gPipeline.getLightingDetail() >= 2)
	{
		glEnableClientState(GL_COLOR_ARRAY);
	}
	
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
	
	// Hack! Get the region that this draw pool is rendering from!
	LLViewerRegion *regionp = mDrawFace[0]->getDrawable()->getVObj()->getRegion();
	LLVLComposition *compp = regionp->getComposition();
	LLViewerImage *detail_texture0p = compp->mDetailTextures[0];
	LLViewerImage *detail_texture1p = compp->mDetailTextures[1];
	LLViewerImage *detail_texture2p = compp->mDetailTextures[2];
	LLViewerImage *detail_texture3p = compp->mDetailTextures[3];

	static F32 dp = 0.f;
	static LLFrameTimer timer;
	dp += timer.getElapsedTimeAndResetF32();

	LLVector3d region_origin_global = gAgent.getRegion()->getOriginGlobal();

	F32 offset_x = (F32)fmod(region_origin_global.mdV[VX], 1.0/(F64)sDetailScale)*sDetailScale;
	F32 offset_y = (F32)fmod(region_origin_global.mdV[VY], 1.0/(F64)sDetailScale)*sDetailScale;

	LLVector4 tp0, tp1;
	
	tp0.setVec(sDetailScale, 0.0f, 0.0f, offset_x);
	tp1.setVec(0.0f, sDetailScale, 0.0f, offset_y);

	//----------------------------------------------------------------------------
	// Pass 1/1

	//
	// Stage 0: detail texture 0
	//
	
	S32 detailTex0 = gTerrainProgram.enableTexture(LLShaderMgr::TERRAIN_DETAIL0);
	S32 detailTex1 = gTerrainProgram.enableTexture(LLShaderMgr::TERRAIN_DETAIL1);
	S32 rampTex = gTerrainProgram.enableTexture(LLShaderMgr::TERRAIN_ALPHARAMP);
	
	LLViewerImage::bindTexture(detail_texture0p,detailTex0);

	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	//
	// Stage 1: Generate alpha ramp for detail0/detail1 transition
	//
	LLViewerImage::bindTexture(m2DAlphaRampImagep,rampTex);

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//
	// Stage 2: Interpolate detail1 with existing based on ramp
	//
	LLViewerImage::bindTexture(detail_texture1p,detailTex1);
	
	glClientActiveTextureARB(GL_TEXTURE2_ARB);
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	//
	// Stage 3: Modulate with primary color for lighting
	//
	//LLViewerImage::bindTexture(detail_texture1p,3); // bind any texture
	//glEnable(GL_TEXTURE_2D); // Texture unit 3
	glClientActiveTextureARB(GL_TEXTURE3_ARB);
	glActiveTextureARB(GL_TEXTURE3_ARB);
	// GL_BLEND disabled by default
	drawLoop();

	//----------------------------------------------------------------------------
	// Second pass

	//
	// Stage 0: Write detail3 into base
	//
	LLViewerImage::bindTexture(detail_texture2p,detailTex0);
	
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	//
	// Stage 1: Generate alpha ramp for detail2/detail3 transition
	//
	LLViewerImage::bindTexture(m2DAlphaRampImagep,rampTex);
	
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glActiveTextureARB(GL_TEXTURE1_ARB);

	// Set the texture matrix
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(-2.f, 0.f, 0.f);

	//
	// Stage 2: Interpolate detail2 with existing based on ramp
	//
	LLViewerImage::bindTexture(detail_texture3p,detailTex1);
	
	glClientActiveTextureARB(GL_TEXTURE2_ARB);
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	//
	// Stage 3: Generate alpha ramp for detail1/detail2 transition
	//
	//LLViewerImage::bindTexture(m2DAlphaRampImagep,3);
	
	//glEnable(GL_TEXTURE_2D); // Texture unit 3
	
	glClientActiveTextureARB(GL_TEXTURE3_ARB);
	glActiveTextureARB(GL_TEXTURE3_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// Set the texture matrix
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(-1.f, 0.f, 0.f);

	{
		LLGLEnable blend(GL_BLEND);
		drawLoop();
	}

	// Disable multitexture
	gTerrainProgram.disableTexture(LLShaderMgr::TERRAIN_ALPHARAMP);
	gTerrainProgram.disableTexture(LLShaderMgr::TERRAIN_DETAIL0);
	gTerrainProgram.disableTexture(LLShaderMgr::TERRAIN_DETAIL1);
	
	glClientActiveTextureARB(GL_TEXTURE3_ARB);
	glActiveTextureARB(GL_TEXTURE3_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glClientActiveTextureARB(GL_TEXTURE2_ARB);
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	// Restore blend state
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	//----------------------------------------------------------------------------
	// Restore Texture Unit 0 defaults
	
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	// Restore non Texture Unit specific defaults
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void LLDrawPoolTerrain::renderFull4TU()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// Hack! Get the region that this draw pool is rendering from!
	LLViewerRegion *regionp = mDrawFace[0]->getDrawable()->getVObj()->getRegion();
	LLVLComposition *compp = regionp->getComposition();
	LLViewerImage *detail_texture0p = compp->mDetailTextures[0];
	LLViewerImage *detail_texture1p = compp->mDetailTextures[1];
	LLViewerImage *detail_texture2p = compp->mDetailTextures[2];
	LLViewerImage *detail_texture3p = compp->mDetailTextures[3];

	LLVector3d region_origin_global = gAgent.getRegion()->getOriginGlobal();
	F32 offset_x = (F32)fmod(region_origin_global.mdV[VX], 1.0/(F64)sDetailScale)*sDetailScale;
	F32 offset_y = (F32)fmod(region_origin_global.mdV[VY], 1.0/(F64)sDetailScale)*sDetailScale;

	LLVector4 tp0, tp1;
	
	tp0.setVec(sDetailScale, 0.0f, 0.0f, offset_x);
	tp1.setVec(0.0f, sDetailScale, 0.0f, offset_y);

	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
	
	//----------------------------------------------------------------------------
	// Pass 1/1

	//
	// Stage 0: detail texture 0
	//
	glActiveTextureARB(GL_TEXTURE0_ARB);
	LLViewerImage::bindTexture(detail_texture0p,0);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

	//
	// Stage 1: Generate alpha ramp for detail0/detail1 transition
	//
	glActiveTextureARB(GL_TEXTURE1_ARB);
	LLViewerImage::bindTexture(m2DAlphaRampImagep,1);

	glEnable(GL_TEXTURE_2D); // Texture unit 1
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// Care about alpha only
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	//
	// Stage 2: Interpolate detail1 with existing based on ramp
	//
	glActiveTextureARB(GL_TEXTURE2_ARB);
	LLViewerImage::bindTexture(detail_texture1p,2);
	glEnable(GL_TEXTURE_2D); // Texture unit 2
	glClientActiveTextureARB(GL_TEXTURE2_ARB);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_INTERPOLATE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB,		GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB,		GL_SRC_ALPHA);

	//
	// Stage 3: Modulate with primary (vertex) color for lighting
	//
	glActiveTextureARB(GL_TEXTURE3_ARB);
	LLViewerImage::bindTexture(detail_texture1p,3); // bind any texture
	glEnable(GL_TEXTURE_2D); // Texture unit 3
	glClientActiveTextureARB(GL_TEXTURE3_ARB);

	// Set alpha texture and do lighting modulation
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);

	// GL_BLEND disabled by default
	drawLoop();

	//----------------------------------------------------------------------------
	// Second pass

	// Stage 0: Write detail3 into base
	//
	glActiveTextureARB(GL_TEXTURE0_ARB);
	LLViewerImage::bindTexture(detail_texture3p,0);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);


	//
	// Stage 1: Generate alpha ramp for detail2/detail3 transition
	//
	glActiveTextureARB(GL_TEXTURE1_ARB);
	LLViewerImage::bindTexture(m2DAlphaRampImagep,1);

	glEnable(GL_TEXTURE_2D); // Texture unit 1
	glClientActiveTextureARB(GL_TEXTURE1_ARB);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// Set the texture matrix
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(-2.f, 0.f, 0.f);

	// Care about alpha only
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);


	//
	// Stage 2: Interpolate detail2 with existing based on ramp
	//
	glActiveTextureARB(GL_TEXTURE2_ARB);
	LLViewerImage::bindTexture(detail_texture2p,2);
	glEnable(GL_TEXTURE_2D); // Texture unit 2
	glClientActiveTextureARB(GL_TEXTURE2_ARB);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_INTERPOLATE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB,		GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB,		GL_ONE_MINUS_SRC_ALPHA);
	

	//
	// Stage 3: Generate alpha ramp for detail1/detail2 transition
	//
	glActiveTextureARB(GL_TEXTURE3_ARB);
	LLViewerImage::bindTexture(m2DAlphaRampImagep,3);

	glEnable(GL_TEXTURE_2D); // Texture unit 3
	glClientActiveTextureARB(GL_TEXTURE3_ARB);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// Set the texture matrix
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(-1.f, 0.f, 0.f);
  
	// Set alpha texture and do lighting modulation
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);


	{
		LLGLEnable blend(GL_BLEND);
		drawLoop();
	}

	// Disable multitexture
	LLImageGL::unbindTexture(3, GL_TEXTURE_2D);
	glClientActiveTextureARB(GL_TEXTURE3_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D); // Texture unit 3
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	LLImageGL::unbindTexture(2, GL_TEXTURE_2D);
	glClientActiveTextureARB(GL_TEXTURE2_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D); // Texture unit 2
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	LLImageGL::unbindTexture(1, GL_TEXTURE_2D);
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D); // Texture unit 1
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	// Restore blend state
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	//----------------------------------------------------------------------------
	// Restore Texture Unit 0 defaults
	
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	// Restore non Texture Unit specific defaults
	glDisableClientState(GL_NORMAL_ARRAY);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void LLDrawPoolTerrain::renderFull2TU()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// Hack! Get the region that this draw pool is rendering from!
	LLViewerRegion *regionp = mDrawFace[0]->getDrawable()->getVObj()->getRegion();
	LLVLComposition *compp = regionp->getComposition();
	LLViewerImage *detail_texture0p = compp->mDetailTextures[0];
	LLViewerImage *detail_texture1p = compp->mDetailTextures[1];
	LLViewerImage *detail_texture2p = compp->mDetailTextures[2];
	LLViewerImage *detail_texture3p = compp->mDetailTextures[3];

	LLVector3d region_origin_global = gAgent.getRegion()->getOriginGlobal();
	F32 offset_x = (F32)fmod(region_origin_global.mdV[VX], 1.0/(F64)sDetailScale)*sDetailScale;
	F32 offset_y = (F32)fmod(region_origin_global.mdV[VY], 1.0/(F64)sDetailScale)*sDetailScale;

	LLVector4 tp0, tp1;
	
	tp0.setVec(sDetailScale, 0.0f, 0.0f, offset_x);
	tp1.setVec(0.0f, sDetailScale, 0.0f, offset_y);

	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
	
	//----------------------------------------------------------------------------
	// Pass 1/4

	//
	// Stage 0: Render detail 0 into base
	//
	LLViewerImage::bindTexture(detail_texture0p,0);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);

	drawLoop();

	//----------------------------------------------------------------------------
	// Pass 2/4
	
	//
	// Stage 0: Generate alpha ramp for detail0/detail1 transition
	//
	LLViewerImage::bindTexture(m2DAlphaRampImagep,0);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// Care about alpha only
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);


	//
	// Stage 1: Write detail1
	//
	LLViewerImage::bindTexture(detail_texture1p,1); // Texture unit 1
	glEnable(GL_TEXTURE_2D);		// Texture unit 1
	glClientActiveTextureARB(GL_TEXTURE1_ARB);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	{
		LLGLEnable blend(GL_BLEND);
		drawLoop();
	}
	//----------------------------------------------------------------------------
	// Pass 3/4
	
	//
	// Stage 0: Generate alpha ramp for detail1/detail2 transition
	//
	LLViewerImage::bindTexture(m2DAlphaRampImagep,0);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	// Set the texture matrix
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(-1.f, 0.f, 0.f);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// Care about alpha only
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	//
	// Stage 1: Write detail2
	//
	
	LLViewerImage::bindTexture(detail_texture2p,1);
	glEnable(GL_TEXTURE_2D); // Texture unit 1
	glClientActiveTextureARB(GL_TEXTURE1_ARB);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	{
		LLGLEnable blend(GL_BLEND);
		drawLoop();
	}
	
	//----------------------------------------------------------------------------
	// Pass 4/4
	
	//
	// Stage 0: Generate alpha ramp for detail2/detail3 transition
	//
	LLViewerImage::bindTexture(m2DAlphaRampImagep,0);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	// Set the texture matrix
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(-2.f, 0.f, 0.f);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// Care about alpha only
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	// Stage 1: Write detail3

	LLViewerImage::bindTexture(detail_texture3p,1);
	glEnable(GL_TEXTURE_2D); // Texture unit 1
	glClientActiveTextureARB(GL_TEXTURE1_ARB);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	{
		LLGLEnable blend(GL_BLEND);
		drawLoop();
	}
	
	// Restore blend state
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// Disable multitexture
	LLImageGL::unbindTexture(1, GL_TEXTURE_2D);
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D); // Texture unit 1
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	//----------------------------------------------------------------------------
	// Restore Texture Unit 0 defaults
	
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);

	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	// Restore non Texture Unit specific defaults
	glDisableClientState(GL_NORMAL_ARRAY);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}


void LLDrawPoolTerrain::renderSimple()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	LLVector4 tp0, tp1;

	//----------------------------------------------------------------------------
	// Pass 1/1

	// Stage 0: Base terrain texture pass
	mTexturep->addTextureStats(1024.f*1024.f);
	mTexturep->bind(0);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D); // Texture unit 2
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	LLVector3 origin_agent = mDrawFace[0]->getDrawable()->getVObj()->getRegion()->getOriginAgent();
	F32 tscale = 1.f/256.f;
	tp0.setVec(tscale, 0.f, 0.0f, -1.f*(origin_agent.mV[0]/256.f));
	tp1.setVec(0.f, tscale, 0.0f, -1.f*(origin_agent.mV[1]/256.f));
	
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);
	
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);

	drawLoop();

	//----------------------------------------------------------------------------
	// Restore Texture Unit 0 defaults
	
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	// Restore non Texture Unit specific defaults
	glDisableClientState(GL_NORMAL_ARRAY);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

//============================================================================

void LLDrawPoolTerrain::renderOwnership()
{
	LLGLSPipelineAlpha gls_pipeline_alpha;

	llassert(!mDrawFace.empty());

	// Each terrain pool is associated with a single region.
	// We need to peek back into the viewer's data to find out
	// which ownership overlay texture to use.
	LLFace					*facep				= mDrawFace[0];
	LLDrawable				*drawablep			= facep->getDrawable();
	const LLViewerObject	*objectp				= drawablep->getVObj();
	const LLVOSurfacePatch	*vo_surface_patchp	= (LLVOSurfacePatch *)objectp;
	LLSurfacePatch			*surface_patchp		= vo_surface_patchp->getPatch();
	LLSurface				*surfacep			= surface_patchp->getSurface();
	LLViewerRegion			*regionp			= surfacep->getRegion();
	LLViewerParcelOverlay	*overlayp			= regionp->getParcelOverlay();
	LLImageGL				*texturep			= overlayp->getTexture();

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	LLViewerImage::bindTexture(texturep);

	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	// *NOTE: Because the region is 256 meters wide, but has 257 pixels, the 
	// texture coordinates for pixel 256x256 is not 1,1. This makes the
	// ownership map not line up with the selection. We address this with
	// a texture matrix multiply.
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();

	const F32 TEXTURE_FUDGE = 257.f / 256.f;
	glScalef( TEXTURE_FUDGE, TEXTURE_FUDGE, 1.f );

	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		LLFace *facep = *iter;
		facep->renderIndexed(LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD);
	}

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	// Restore non Texture Unit specific defaults
	glDisableClientState(GL_NORMAL_ARRAY);
}


void LLDrawPoolTerrain::renderForSelect()
{
	if (mDrawFace.empty())
	{
		return;
	}

	
	LLImageGL::unbindTexture(0);

	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		LLFace *facep = *iter;
		if (!facep->getDrawable()->isDead() && (facep->getDrawable()->getVObj()->mGLName))
		{
			facep->renderForSelect(LLVertexBuffer::MAP_VERTEX);
		}
	}
}

void LLDrawPoolTerrain::dirtyTextures(const std::set<LLViewerImage*>& textures)
{
	if (textures.find(mTexturep) != textures.end())
	{
		for (std::vector<LLFace*>::iterator iter = mReferences.begin();
			 iter != mReferences.end(); iter++)
		{
			LLFace *facep = *iter;
			gPipeline.markTextured(facep->getDrawable());
		}
	}
}

LLViewerImage *LLDrawPoolTerrain::getTexture()
{
	return mTexturep;
}

LLViewerImage *LLDrawPoolTerrain::getDebugTexture()
{
	return mTexturep;
}


LLColor3 LLDrawPoolTerrain::getDebugColor() const
{
	return LLColor3(0.f, 0.f, 1.f);
}

S32 LLDrawPoolTerrain::getMaterialAttribIndex()
{
	return gTerrainProgram.mAttribute[LLShaderMgr::MATERIAL_COLOR];
}
