/** 
 * @file lldrawpoolalpha.cpp
 * @brief LLDrawPoolAlpha class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolalpha.h"

#include "llglheaders.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "llfasttimer.h"

#include "llcubemap.h"
#include "llsky.h"
#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"	// For debugging
#include "llviewerobjectlist.h" // For debugging
#include "llviewerwindow.h"
#include "pipeline.h"
#include "llviewerregion.h"
#include "llglslshader.h"

BOOL LLDrawPoolAlpha::sShowDebugAlpha = FALSE;

LLDrawPoolAlpha::LLDrawPoolAlpha(U32 type) :
	LLRenderPass(type)
{

}

LLDrawPoolAlphaPostWater::LLDrawPoolAlphaPostWater()
: LLDrawPoolAlpha(POOL_ALPHA_POST_WATER)
{
}

LLDrawPoolAlpha::~LLDrawPoolAlpha()
{
}


void LLDrawPoolAlpha::prerender()
{
	mVertexShaderLevel = LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_OBJECT);
}

void LLDrawPoolAlpha::beginRenderPass(S32 pass)
{
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
}

void setup_clip_plane(BOOL pre_water)
{
	F32 height = gAgent.getRegion()->getWaterHeight();
	BOOL above = gCamera->getOrigin().mV[2] > height ? TRUE : FALSE;
	
	F64 plane[4];
	
	plane[0] = 0;
	plane[1] = 0;
	plane[2] = above == pre_water ? -1.0 : 1.0;
	plane[3] = -plane[2] * height;
	
	glClipPlane(GL_CLIP_PLANE0, plane);
}

void LLDrawPoolAlphaPostWater::render(S32 pass)
{
    LLFastTimer t(LLFastTimer::FTM_RENDER_ALPHA);

	if (gPipeline.hasRenderType(LLDrawPool::POOL_ALPHA))
	{
		LLGLEnable clip(GL_CLIP_PLANE0);
		setup_clip_plane(FALSE);
		LLDrawPoolAlpha::render(gPipeline.mAlphaGroupsPostWater);
	}
	else
	{
		LLDrawPoolAlpha::render(gPipeline.mAlphaGroupsPostWater);
	}
}

void LLDrawPoolAlpha::render(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_ALPHA);
	
	LLGLEnable clip(GL_CLIP_PLANE0);
	setup_clip_plane(TRUE);
	render(gPipeline.mAlphaGroups);
}

void LLDrawPoolAlpha::render(std::vector<LLSpatialGroup*>& groups)
{
	LLGLDepthTest gls_depth(GL_TRUE);
	LLGLSPipelineAlpha gls_pipeline_alpha;

	gPipeline.enableLightsDynamic(1.f);
	renderAlpha(getVertexDataMask(), groups);

	if (sShowDebugAlpha)
	{
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
		glColor4f(1,0,0,1);
		LLViewerImage::sSmokeImagep->addTextureStats(1024.f*1024.f);
        LLViewerImage::sSmokeImagep->bind();
		renderAlphaHighlight(LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD, groups);
	}
}

void LLDrawPoolAlpha::renderAlpha(U32 mask, std::vector<LLSpatialGroup*>& groups)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkClientArrays(mask);
#endif

	LLSpatialBridge* last_bridge = NULL;
	LLSpatialPartition* last_part = NULL;
	glPushMatrix();
	LLGLDepthTest depth(GL_TRUE, GL_FALSE);

	for (std::vector<LLSpatialGroup*>::iterator i = groups.begin(); i != groups.end(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (group->mSpatialPartition->mRenderByGroup &&
			!group->isDead())
		{
			LLSpatialPartition* part = group->mSpatialPartition;
			if (part != last_part)
			{
				LLSpatialBridge* bridge = part->asBridge();
				if (bridge != last_bridge)
				{
					glPopMatrix();
					glPushMatrix();
					if (bridge)
					{
						glMultMatrixf((F32*) bridge->mDrawable->getRenderMatrix().mMatrix);
					}
					last_bridge = bridge;
				}

//				if (!last_part || part->mDepthMask != last_part->mDepthMask)
//				{
//					glDepthMask(part->mDepthMask);
//				}
				last_part = part;
			}

			renderGroupAlpha(group,LLRenderPass::PASS_ALPHA,mask,TRUE);
		}
	}
	
	glPopMatrix();
}

void LLDrawPoolAlpha::renderAlphaHighlight(U32 mask, std::vector<LLSpatialGroup*>& groups)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkClientArrays(mask);
#endif

	LLSpatialBridge* last_bridge = NULL;
	LLSpatialPartition* last_part = NULL;
	glPushMatrix();
	
	for (std::vector<LLSpatialGroup*>::iterator i = groups.begin(); i != groups.end(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (group->mSpatialPartition->mRenderByGroup &&
			!group->isDead())
		{
			LLSpatialPartition* part = group->mSpatialPartition;
			if (part != last_part)
			{
				LLSpatialBridge* bridge = part->asBridge();
				if (bridge != last_bridge)
				{
					glPopMatrix();
					glPushMatrix();
					if (bridge)
					{
						glMultMatrixf((F32*) bridge->mDrawable->getRenderMatrix().mMatrix);
					}
					last_bridge = bridge;
				}
				
				last_part = part;
			}

			LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA];	

			for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)	
			{
				LLDrawInfo& params = **k;
				
				if (params.mParticle)
				{
					continue;
				}
				params.mVertexBuffer->setBuffer(mask);
				U32* indices_pointer = (U32*) params.mVertexBuffer->getIndicesPointer();
				glDrawRangeElements(GL_TRIANGLES, params.mStart, params.mEnd, params.mCount,
									GL_UNSIGNED_INT, indices_pointer+params.mOffset);
				
				addIndicesDrawn(params.mCount);
			}
		}
	}
	glPopMatrix();
}

void LLDrawPoolAlpha::renderGroupAlpha(LLSpatialGroup* group, U32 type, U32 mask, BOOL texture)
{					
	BOOL light_enabled = TRUE;

	LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[type];	
	
	U32 prim_type = GL_TRIANGLES;

	//F32 width = (F32) gViewerWindow->getWindowDisplayWidth();

	//F32 view = gCamera->getView();
	
	if (group->mSpatialPartition->mDrawableType == LLPipeline::RENDER_TYPE_CLOUDS)
	{
		glAlphaFunc(GL_GREATER, 0.f);
	}
	else
	{
		glAlphaFunc(GL_GREATER, 0.01f);
	}

	/*LLGLEnable point_sprite(GL_POINT_SPRITE_ARB);

	if (gGLManager.mHasPointParameters)
	{
		glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, TRUE);
		glPointParameterfARB(GL_POINT_SIZE_MIN_ARB, 0.f);
		glPointParameterfARB(GL_POINT_SIZE_MAX_ARB, width*16.f);
		glPointSize(width/(view*view));
	}*/

	for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)	
	{
		LLDrawInfo& params = **k;
		if (texture && params.mTexture.notNull())
		{
			params.mTexture->bind();
			params.mTexture->addTextureStats(params.mVSize);
			if (params.mTextureMatrix)
			{
				glMatrixMode(GL_TEXTURE);
				glLoadMatrixf((GLfloat*) params.mTextureMatrix->mMatrix);
			}
		}
		
		if (params.mFullbright)
		{
			if (light_enabled)
			{
				gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
				light_enabled = FALSE;
				if (LLPipeline::sRenderGlow)
				{
					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				}
			}
		}
		else if (!light_enabled)
		{
			gPipeline.enableLightsDynamic(1.f);
			light_enabled = TRUE;
			if (LLPipeline::sRenderGlow)
			{
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
			}
		}

		/*if (params.mParticle)
		{
			F32 size = params.mPartSize;
			size *= size;
			float param[] = { 0, 0, 0.01f/size*view*view };
			prim_type = GL_POINTS;
			glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, param);
		}
		else*/
		{
			prim_type = GL_TRIANGLES;
		}

		params.mVertexBuffer->setBuffer(mask);
		U32* indices_pointer = (U32*) params.mVertexBuffer->getIndicesPointer();
		glDrawRangeElements(prim_type, params.mStart, params.mEnd, params.mCount,
							GL_UNSIGNED_INT, indices_pointer+params.mOffset);
		
		addIndicesDrawn(params.mCount);

		if (params.mTextureMatrix && texture && params.mTexture.notNull())
		{
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
		}
	}

	if (!light_enabled)
	{
		gPipeline.enableLightsDynamic(1.f);
	
		if (LLPipeline::sRenderGlow)
		{
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		}
	}

	/*glPointSize(1.f);

	if (gGLManager.mHasPointParameters)
	{
		float param[] = {1, 0, 0 };
		glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, param);
	}*/
}	
