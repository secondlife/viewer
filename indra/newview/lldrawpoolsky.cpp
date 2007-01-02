/** 
 * @file lldrawpoolsky.cpp
 * @brief LLDrawPoolSky class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolsky.h"

#include "imageids.h"

#include "llagparray.h"
#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvosky.h"
#include "llworld.h" // To get water height
#include "pipeline.h"

LLDrawPoolSky::LLDrawPoolSky() :
	LLDrawPool(POOL_SKY, DATA_SIMPLE_IL_MASK, DATA_SIMPLE_NIL_MASK)
{
}

LLDrawPool *LLDrawPoolSky::instancePool()
{
	return new LLDrawPoolSky();
}

void LLDrawPoolSky::prerender()
{
	mVertexShaderLevel = gPipeline.getVertexShaderLevel(LLPipeline::SHADER_ENVIRONMENT);
}

void LLDrawPoolSky::render(S32 pass)
{
	if (mDrawFace.empty())
	{
		return;
	}

	LLVOSky *voskyp = gSky.mVOSkyp;
	LLGLSPipelineSkyBox gls_skybox;
	LLGLDepthTest gls_depth(GL_FALSE, GL_FALSE);

	if (gCamera->getOrigin().mV[VZ] < gAgent.getRegion()->getWaterHeight())
		//gWorldPointer->getWaterHeight())
	{
		//gGLSFog.set();
	}

	gPipeline.disableLights();
	
	glMatrixMode( GL_PROJECTION );

	glPushMatrix();
	gViewerWindow->setup3DRender();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	bindGLVertexPointer();
	bindGLTexCoordPointer();

	S32 face_count = (S32)mDrawFace.size();

	for (S32 i = 0; i < llmin(6, face_count); ++i)
	{
		renderSkyCubeFace(i);
	}
	
	const LLFace *hbfaces[3];
	hbfaces[0] = NULL;
	hbfaces[1] = NULL;
	hbfaces[2] = NULL;
	for (S32 curr_face = 0; curr_face < face_count; curr_face++)
	{
		const LLFace* facep = mDrawFace[curr_face];
		if (voskyp->isSameFace(LLVOSky::FACE_SUN, facep))
		{
			hbfaces[0] = facep;
		}
		if (voskyp->isSameFace(LLVOSky::FACE_MOON, facep))
		{
			hbfaces[1] = facep;
		}
		if (voskyp->isSameFace(LLVOSky::FACE_BLOOM, facep))
		{
			hbfaces[2] = facep;
		}
	}

	LLGLEnable blend(GL_BLEND);

	if (hbfaces[2])
	{
		renderSunHalo(hbfaces[2]);
	}
	if (hbfaces[0])
	{
		renderHeavenlyBody(0, hbfaces[0]);
	}
	if (hbfaces[1])
	{
		renderHeavenlyBody(1, hbfaces[1]);
	}


	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
}

void LLDrawPoolSky::renderSkyCubeFace(U8 side)
{
	const LLFace &face = *mDrawFace[LLVOSky::FACE_SIDE0 + side];
	if (!face.getGeomCount())
	{
		return;
	}

	mSkyTex[side].bindTexture(TRUE);
	
	face.renderIndexed(getRawIndices());

	if (LLSkyTex::doInterpolate())
	{
		LLGLEnable blend(GL_BLEND);
		mSkyTex[side].bindTexture(FALSE);
		glColor4f(1, 1, 1, LLSkyTex::getInterpVal()); // lighting is disabled
		face.renderIndexed(getRawIndices());
	}

	mIndicesDrawn += face.getIndicesCount();
}

void LLDrawPoolSky::renderHeavenlyBody(U8 hb, const LLFace* face)
{
	if ( !mHB[hb]->getDraw() ) return;
	if (! face->getGeomCount()) return;

	LLImageGL* tex = face->getTexture();
	tex->bind();
	LLColor4 color(mHB[hb]->getInterpColor());
	LLOverrideFaceColor override(this, color);
	face->renderIndexed(getRawIndices());
	mIndicesDrawn += face->getIndicesCount();
}



void LLDrawPoolSky::renderSunHalo(const LLFace* face)
{
	if (! mHB[0]->getDraw()) return;
	if (! face->getGeomCount()) return;

	LLImageGL* tex = face->getTexture();
	tex->bind();
	LLColor4 color(mHB[0]->getInterpColor());
	color.mV[3] = llclamp(mHB[0]->getHaloBrighness(), 0.f, 1.f);

	LLOverrideFaceColor override(this, color);
	face->renderIndexed(getRawIndices());
	mIndicesDrawn += face->getIndicesCount();
}


void LLDrawPoolSky::renderForSelect()
{
}

