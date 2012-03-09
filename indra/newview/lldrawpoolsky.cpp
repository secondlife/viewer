/** 
 * @file lldrawpoolsky.cpp
 * @brief LLDrawPoolSky class implementation
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
	gGL.flush();

	if (mDrawFace.empty())
	{
		return;
	}

	// Don't draw the sky box if we can and are rendering the WL sky dome.
	if (gPipeline.canUseWindLightShaders())
	{
		return;
	}
	
	// don't render sky under water (background just gets cleared to fog color)
	if(mVertexShaderLevel > 0 && LLPipeline::sUnderWaterRender)
	{
		return;
	}


	if (LLGLSLShader::sNoFixedFunction)
	{ //just use the UI shader (generic single texture no lighting)
		gOneTextureNoColorProgram.bind();
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

	LLGLSquashToFarClip far_clip(glh_get_current_projection());

	LLGLEnable fog_enable( (mVertexShaderLevel < 1 && LLViewerCamera::getInstance()->cameraUnderWater()) ? GL_FOG : 0);

	gPipeline.disableLights();
	
	LLGLDisable clip(GL_CLIP_PLANE0);

	gGL.pushMatrix();
	LLVector3 origin = LLViewerCamera::getInstance()->getOrigin();
	gGL.translatef(origin.mV[0], origin.mV[1], origin.mV[2]);

	S32 face_count = (S32)mDrawFace.size();

	LLVertexBuffer::unbind();
	gGL.diffuseColor4f(1,1,1,1);

	for (S32 i = 0; i < llmin(6, face_count); ++i)
	{
		renderSkyCubeFace(i);
	}

	gGL.popMatrix();
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
		gGL.diffuseColor4f(1, 1, 1, LLSkyTex::getInterpVal()); // lighting is disabled
		face.renderIndexed();
	}
}

void LLDrawPoolSky::endRenderPass( S32 pass )
{
}
