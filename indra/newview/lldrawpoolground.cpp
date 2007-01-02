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

#include "llagparray.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "pipeline.h"

LLDrawPoolGround::LLDrawPoolGround() :
	LLDrawPool(POOL_GROUND, DATA_SIMPLE_IL_MASK, DATA_SIMPLE_NIL_MASK)
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
	glDisableClientState(GL_NORMAL_ARRAY);

	bindGLVertexPointer();
	bindGLTexCoordPointer();

	LLGLSPipelineSkyBox gls_skybox;
	LLGLDepthTest gls_depth(GL_FALSE, GL_FALSE);

	glMatrixMode( GL_PROJECTION );

	glPushMatrix();
	gViewerWindow->setup3DRender();

	glMatrixMode(GL_MODELVIEW);

	LLGLState tex2d(GL_TEXTURE_2D, (mVertexShaderLevel > 0) ? TRUE : FALSE);
	LLViewerImage::bindTexture(gSky.mVOSkyp->getScatterMap(), 0);

	LLFace *facep = mDrawFace[0];

	if (!(mVertexShaderLevel > 0))
	{
		gPipeline.disableLights();
	}

	glColor4fv(facep->getFaceColor().mV);	

	facep->renderIndexed(getRawIndices());
	
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
}

void LLDrawPoolGround::renderForSelect()
{
}

