/** 
 * @file lldrawpoolground.cpp
 * @brief LLDrawPoolGround class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llviewershadermgr.h"

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
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_ENVIRONMENT);
}

void LLDrawPoolGround::render(S32 pass)
{
	if (mDrawFace.empty())
	{
		return;
	}	
	
	LLGLSPipelineSkyBox gls_skybox;
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	LLGLClampToFarClip far_clip(glh_get_current_projection());

	F32 water_height = gAgent.getRegion()->getWaterHeight();
	glPushMatrix();
	LLVector3 origin = LLViewerCamera::getInstance()->getOrigin();
	glTranslatef(origin.mV[0], origin.mV[1], llmax(origin.mV[2], water_height));

	LLFace *facep = mDrawFace[0];

	gPipeline.disableLights();

	LLOverrideFaceColor col(this, gSky.mVOSkyp->getGLFogColor());
	facep->renderIndexed();
	
	glPopMatrix();
}

void LLDrawPoolGround::renderForSelect()
{
}

