/** 
 * @file lldrawpoolclouds.cpp
 * @brief LLDrawPoolClouds class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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


void LLDrawPoolClouds::renderForSelect()
{
}
