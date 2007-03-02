/** 
 * @file lldrawpoolclouds.cpp
 * @brief LLDrawPoolClouds class implementation
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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
