/** 
 * @file lldrawpoolsky.cpp
 * @brief LLDrawPoolSky class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolsky.h"

#include "imageids.h"

#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "llvosky.h"
#include "llworld.h" // To get water height
#include "pipeline.h"
#include "llviewershadermgr.h"

LLDrawPoolSky::LLDrawPoolSky()
:	LLFacePool(POOL_SKY),
	
	mSkyTex(NULL),
	mShader(NULL)
{
}

LLDrawPool *LLDrawPoolSky::instancePool()
{
	return new LLDrawPoolSky();
}

void LLDrawPoolSky::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_ENVIRONMENT); 
	gSky.mVOSkyp->updateGeometry(gSky.mVOSkyp->mDrawable);
}

void LLDrawPoolSky::render(S32 pass)
{
	if (mDrawFace.empty())
	{
		return;
	}

	// Don't draw the sky box if we can and are rendering the WL sky dome.
	if (gPipeline.canUseWindLightShaders())
	{
		return;
	}
	
	// use a shader only underwater
	if(mVertexShaderLevel > 0 && LLPipeline::sUnderWaterRender)
	{
		mShader = &gObjectFullbrightWaterProgram;
		mShader->bind();
	}
	else
	{
		// don't use shaders!
		if (gGLManager.mHasShaderObjects)
		{
			// Ironically, we must support shader objects to be
			// able to use this call.
			LLGLSLShader::bindNoShader();
		}
		mShader = NULL;
	}
	

	LLGLSPipelineSkyBox gls_skybox;

	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	LLGLClampToFarClip far_clip(glh_get_current_projection());

	LLGLEnable fog_enable( (mVertexShaderLevel < 1 && LLViewerCamera::getInstance()->cameraUnderWater()) ? GL_FOG : 0);

	gPipeline.disableLights();
	
	LLGLDisable clip(GL_CLIP_PLANE0);

	glPushMatrix();
	LLVector3 origin = LLViewerCamera::getInstance()->getOrigin();
	glTranslatef(origin.mV[0], origin.mV[1], origin.mV[2]);

	S32 face_count = (S32)mDrawFace.size();

	for (S32 i = 0; i < llmin(6, face_count); ++i)
	{
		renderSkyCubeFace(i);
	}

	LLGLEnable blend(GL_BLEND);

	glPopMatrix();
}

void LLDrawPoolSky::renderSkyCubeFace(U8 side)
{
	LLFace &face = *mDrawFace[LLVOSky::FACE_SIDE0 + side];
	if (!face.getGeomCount())
	{
		return;
	}

	llassert(mSkyTex);
	mSkyTex[side].bindTexture(TRUE);
	
	face.renderIndexed();

	if (LLSkyTex::doInterpolate())
	{
		LLGLEnable blend(GL_BLEND);
		mSkyTex[side].bindTexture(FALSE);
		glColor4f(1, 1, 1, LLSkyTex::getInterpVal()); // lighting is disabled
		face.renderIndexed();
	}
}

void LLDrawPoolSky::renderForSelect()
{
}

void LLDrawPoolSky::endRenderPass( S32 pass )
{
}
