/** 
 * @file lldrawpoolground.cpp
 * @brief LLDrawPoolGround class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolground.h"

#include "llviewercontrol.h"

#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "pipeline.h"
#include "llagent.h"
#include "llviewerregion.h"

LLDrawPoolGround::LLDrawPoolGround() :
	LLFacePool(POOL_GROUND)
{
}

LLDrawPool *LLDrawPoolGround::instancePool()
{
	return new LLDrawPoolGround();
}

void LLDrawPoolGround::prerender()
{
	mVertexShaderLevel = gPipeline.getVertexShaderLevel(LLPipeline::SHADER_ENVIRONMENT);
}

void LLDrawPoolGround::render(S32 pass)
{
	if (mDrawFace.empty())
	{
		return;
	}	
	
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	LLGLSPipelineSkyBox gls_skybox;
	LLGLDisable tex(GL_TEXTURE_2D);
	LLGLDepthTest gls_depth(GL_FALSE, GL_FALSE);

	glMatrixMode( GL_PROJECTION );
		
	glPushMatrix();
	//gViewerWindow->setup3DRender();

	glMatrixMode(GL_MODELVIEW);

	F32 water_height = gAgent.getRegion()->getWaterHeight();
	glPushMatrix();
	LLVector3 origin = gCamera->getOrigin();
	glTranslatef(origin.mV[0], origin.mV[1], llmax(origin.mV[2], water_height));

	LLFace *facep = mDrawFace[0];

	gPipeline.disableLights();

	LLOverrideFaceColor col(this, gSky.mVOSkyp->getGLFogColor());
	facep->renderIndexed();
	
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}

void LLDrawPoolGround::renderForSelect()
{
}

