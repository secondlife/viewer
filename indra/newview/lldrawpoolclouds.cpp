/** 
 * @file lldrawpoolclouds.cpp
 * @brief LLDrawPoolClouds class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "lldrawpoolclouds.h"

#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llvoclouds.h"
#include "pipeline.h"

LLDrawPoolClouds::LLDrawPoolClouds() :
	LLDrawPool(POOL_CLOUDS)
{
}

LLDrawPool *LLDrawPoolClouds::instancePool()
{
	return new LLDrawPoolClouds();
}

BOOL LLDrawPoolClouds::addFace(LLFace* face)
{
	llerrs << "WTF?" << llendl;
	return FALSE;
}

void LLDrawPoolClouds::enqueue(LLFace *facep)
{
	mDrawFace.push_back(facep);
	facep->mDistance = (facep->mCenterAgent - gCamera->getOrigin()) * gCamera->getAtAxis();
}

void LLDrawPoolClouds::beginRenderPass(S32 pass)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
}

void LLDrawPoolClouds::prerender()
{
	mVertexShaderLevel = gPipeline.getVertexShaderLevel(LLPipeline::SHADER_ENVIRONMENT);
}

void LLDrawPoolClouds::render(S32 pass)
{
	LLFastTimer ftm(LLFastTimer::FTM_RENDER_CLOUDS);
 	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS)))
	{
		return;
	}
	
	if (mDrawFace.empty())
	{
		return;
	}

	LLGLSPipelineAlpha gls_pipeline_alpha;
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
	glAlphaFunc(GL_GREATER,0.01f);

	gPipeline.enableLightsFullbright(LLColor4(1.f,1.f,1.f));

	mDrawFace[0]->bindTexture();

	std::sort(mDrawFace.begin(), mDrawFace.end(), LLFace::CompareDistanceGreater());

	drawLoop();
}


