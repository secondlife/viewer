/** 
 * @file lldrawpoolground.cpp
 * @brief LLDrawPoolGround class implementation
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

void LLDrawPoolGround::prerender()
{
	mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_ENVIRONMENT);
}

void LLDrawPoolGround::render(S32 pass)
{
	if (mDrawFace.empty() || !LLGLSLShader::sCurBoundShaderPtr || !gSavedSettings.getBOOL("RenderGround"))
	{
		return;
	}

	LLGLSPipelineDepthTestSkyBox gls_skybox(true, false);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	F32 water_height = gAgent.getRegion()->getWaterHeight();
	gGL.pushMatrix();
	LLVector3 origin = LLViewerCamera::getInstance()->getOrigin();
	gGL.translatef(origin.mV[0], origin.mV[1], llmax(origin.mV[2], water_height));

	LLFace *facep = mDrawFace[0];

	LLOverrideFaceColor col(this, gSky.mVOSkyp->getGLFogColor());
	facep->renderIndexed();
	
	gGL.popMatrix();
}

