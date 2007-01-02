/** 
 * @file lldrawpoolalpha.cpp
 * @brief LLDrawPoolAlpha class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolalpha.h"

#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "llfasttimer.h"

#include "llagparray.h"
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

const F32 MAX_DIST = 512.f;
const F32 ALPHA_FALLOFF_START_DISTANCE = 0.8f;

BOOL LLDrawPoolAlpha::sShowDebugAlpha = FALSE;

LLDrawPoolAlpha::LLDrawPoolAlpha() :
	LLDrawPool(POOL_ALPHA,
			   DATA_SIMPLE_IL_MASK | DATA_COLORS_MASK,
			   DATA_SIMPLE_NIL_MASK)
{
	mRebuiltLastFrame = FALSE;
	mMinDistance = 0.f;
	mMaxDistance = MAX_DIST;
	mInvBinSize = NUM_ALPHA_BINS/(mMaxDistance - mMinDistance);
	mCleanupUnused = TRUE;
	//mRebuildFreq = -1 ; // Only rebuild if nearly full

//	for (S32 i = 0; i < NUM_ALPHA_BINS; i++)
//	{
//		mDistanceBins[i].realloc(200);
//	}
}

LLDrawPoolAlpha::~LLDrawPoolAlpha()
{
}

LLDrawPool *LLDrawPoolAlpha::instancePool()
{
	llerrs << "Should never be calling instancePool on an alpha pool!" << llendl;
	return NULL;
}

void LLDrawPoolAlpha::enqueue(LLFace *facep)
{
	if (!facep->isState(LLFace::GLOBAL))
	{
		facep->mCenterAgent = facep->mCenterLocal * facep->getRenderMatrix();
	}
	facep->mDistance = (facep->mCenterAgent - gCamera->getOrigin()) * gCamera->getAtAxis();
	
	if (facep->isState(LLFace::BACKLIST))
	{
		mMoveFace.put(facep);
	}
	else
	{
		mDrawFace.put(facep);
	}

	{
		S32 dist_bin = lltrunc( (mMaxDistance - (facep->mDistance+32))*mInvBinSize );

		if (dist_bin >= NUM_ALPHA_BINS)
		{
			mDistanceBins[NUM_ALPHA_BINS-1].put(facep);
			//mDistanceBins[NUM_ALPHA_BINS-1].push(facep, (U32)(void*)facep->getTexture());
		}
		else if (dist_bin > 0)
		{
			mDistanceBins[dist_bin].put(facep);
			//mDistanceBins[dist_bin].push(facep, (U32)(void*)facep->getTexture());
		}
		else
		{
			mDistanceBins[0].put(facep);
			//mDistanceBins[0].push(facep, (U32)(void*)facep->getTexture());
		}
	}
}

BOOL LLDrawPoolAlpha::removeFace(LLFace *facep)
{
	BOOL removed = FALSE;

	LLDrawPool::removeFace(facep);

	{
		for (S32 i = 0; i < NUM_ALPHA_BINS; i++)
		{
			if (mDistanceBins[i].removeObj(facep) != -1)
			{
				if (removed)
				{
					llerrs << "Warning! " << "Face in multiple distance bins on removal" << llendl;
				}
				removed = TRUE;
			}
		}
	}
	if (removed)
	{
		return TRUE;
	}

	return FALSE;
}

void LLDrawPoolAlpha::prerender()
{
	mVertexShaderLevel = gPipeline.getVertexShaderLevel(LLPipeline::SHADER_OBJECT);
}

void LLDrawPoolAlpha::beginRenderPass(S32 pass)
{
	if (mDrawFace.empty())
	{
		// No alpha objects, early exit.
		return;
	}

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	if (gPipeline.getLightingDetail() >= 2)
	{
		glEnableClientState(GL_COLOR_ARRAY);
	}
}


void LLDrawPoolAlpha::render(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_ALPHA);
	
	if (mDrawFace.empty())
	{
		// No alpha objects, early exit.
		return;
	}

	GLfloat shiny[4] =
	{
		0.00f,
		0.25f,
		0.5f,
		0.75f
	};

	GLint specularIndex = (mVertexShaderLevel > 0) ? 
		gPipeline.mObjectAlphaProgram.mAttribute[LLPipeline::GLSL_SPECULAR_COLOR] : 0;

	S32 diffTex = 0;
	S32 envTex = -1;

	if (mVertexShaderLevel > 0) //alpha pass uses same shader as shiny/bump
	{
		envTex = gPipeline.mObjectAlphaProgram.enableTexture(LLPipeline::GLSL_ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
		LLCubeMap* cube_map = gSky.mVOSkyp->getCubeMap();
		if (envTex >= 0 && cube_map)
		{
			cube_map->bind();
			cube_map->setMatrix(1);
		}
		
		if (specularIndex > 0)
		{
			glVertexAttrib4fARB(specularIndex, 0, 0, 0, 0);
		}
		
		S32 scatterTex = gPipeline.mObjectAlphaProgram.enableTexture(LLPipeline::GLSL_SCATTER_MAP);
		LLViewerImage::bindTexture(gSky.mVOSkyp->getScatterMap(), scatterTex);

		diffTex = gPipeline.mObjectAlphaProgram.enableTexture(LLPipeline::GLSL_DIFFUSE_MAP);
	}

	bindGLVertexPointer();
	bindGLTexCoordPointer();
	bindGLNormalPointer();
	if (gPipeline.getLightingDetail() >= 2)
	{
		bindGLColorPointer();
	}

	S32 i, j;
	glAlphaFunc(GL_GREATER,0.01f);
	// This needs to be turned off or there will be lots of artifacting with the clouds - djs
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	LLGLSPipelineAlpha gls_pipeline_alpha;

	LLDynamicArray<LLFace*>* distance_bins;
	distance_bins = mDistanceBins;

	S32 num_bins_no_alpha_test = ((gPickAlphaThreshold != 0.f) && gUsePickAlpha) ? 
		(NUM_ALPHA_BINS - llmax(2, (S32)(ALPHA_FALLOFF_START_DISTANCE * mInvBinSize))) : 
		NUM_ALPHA_BINS;

	typedef std::vector<LLFace*> face_list_t;

	for (i = 0; i < num_bins_no_alpha_test; i++)
	{
		S32 obj_count = distance_bins[i].count();

		if (!obj_count)
		{
			continue;
		}
		else if (i > (NUM_ALPHA_BINS / 2) && obj_count < 100)
		{
			face_list_t pri_queue;
			pri_queue.reserve(distance_bins[i].count());
			for (j = 0; j < distance_bins[i].count(); j++)
			{
				pri_queue.push_back(distance_bins[i][j]);
			}
			std::sort(pri_queue.begin(), pri_queue.end(), LLFace::CompareDistanceGreater());
			
			for (face_list_t::iterator iter = pri_queue.begin(); iter != pri_queue.end(); iter++)
			{
				const LLFace &face = *(*iter);
				face.enableLights();
				face.bindTexture(diffTex);
				if ((mVertexShaderLevel > 0) && face.getTextureEntry() && specularIndex > 0)
				{
					U8 s = face.getTextureEntry()->getShiny();
					glVertexAttrib4fARB(specularIndex, shiny[s], shiny[s], shiny[s], shiny[s]);
				}
				face.renderIndexed(getRawIndices());
				mIndicesDrawn += face.getIndicesCount();
			}
		}
		else
		{
			S32 count = distance_bins[i].count();
			for (j = 0; j < count; j++)
			{
				const LLFace &face = *distance_bins[i][j];
				face.enableLights();
				face.bindTexture(diffTex);
				if ((mVertexShaderLevel > 0) && face.getTextureEntry() && specularIndex > 0)
				{
					U8 s = face.getTextureEntry()->getShiny();
					glVertexAttrib4fARB(specularIndex, shiny[s], shiny[s], shiny[s], shiny[s]);
				}
				face.renderIndexed(getRawIndices());
				mIndicesDrawn += face.getIndicesCount();
			}
		}
	}

	GLfloat			ogl_matrix[16];
	gCamera->getOpenGLTransform(ogl_matrix);

	for (i = num_bins_no_alpha_test; i < NUM_ALPHA_BINS; i++)
	{
		BOOL use_pri_queue = distance_bins[i].count() < 100;

		face_list_t pri_queue;
		
		if (use_pri_queue)
		{
			pri_queue.reserve(distance_bins[i].count());
			for (j = 0; j < distance_bins[i].count(); j++)
			{
				pri_queue.push_back(distance_bins[i][j]);
			}
			std::sort(pri_queue.begin(), pri_queue.end(), LLFace::CompareDistanceGreater());
		}

		S32 count = distance_bins[i].count();
		for (j = 0; j < count; j++)
		{
			const LLFace &face = use_pri_queue ? *pri_queue[j] : *distance_bins[i][j];
			F32 fade_value = face.mAlphaFade * gPickAlphaThreshold;

			face.enableLights();
			
			if (fade_value < 1.f)
			{
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE);
					glAlphaFunc(GL_LESS, fade_value);
					glBlendFunc(GL_ZERO, GL_ONE);
					LLViewerImage::bindTexture(gPipeline.mAlphaSizzleImagep, diffTex);
					LLVector4 s_params(ogl_matrix[2], ogl_matrix[6], ogl_matrix[10], ogl_matrix[14]);
					LLVector4 t_params(ogl_matrix[1], ogl_matrix[5], ogl_matrix[9], ogl_matrix[13]);

					LLGLEnable gls_texgen_s(GL_TEXTURE_GEN_S);
					LLGLEnable gls_texgen_t(GL_TEXTURE_GEN_T);
					glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
					glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
					glTexGenfv(GL_S, GL_OBJECT_PLANE, s_params.mV);
					glTexGenfv(GL_T, GL_OBJECT_PLANE, t_params.mV);
					if ((mVertexShaderLevel > 0) && face.getTextureEntry() && specularIndex > 0)
					{
						U8 s = face.getTextureEntry()->getShiny();
						glVertexAttrib4fARB(specularIndex, shiny[s], shiny[s], shiny[s], shiny[s]);
					}
					face.renderIndexed(getRawIndices());
				}

				{
					// should get GL_GREATER to work, as it's faster
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_LESS);
					glAlphaFunc(GL_GEQUAL, fade_value);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					face.bindTexture(diffTex);
					if ((mVertexShaderLevel > 0) && face.getTextureEntry() && specularIndex > 0)
					{
						U8 s = face.getTextureEntry()->getShiny();
						glVertexAttrib4fARB(specularIndex, shiny[s], shiny[s], shiny[s], shiny[s]);
					}
					face.renderIndexed(getRawIndices());
				}
			}

			// render opaque portion of actual texture
			glAlphaFunc(GL_GREATER, 0.98f);

			face.bindTexture(diffTex);
			face.renderIndexed(getRawIndices());

			glAlphaFunc(GL_GREATER, 0.01f);

			mIndicesDrawn += face.getIndicesCount();
		}
	}

	if (mVertexShaderLevel > 0) //single pass shader driven shiny/bump
	{
		gPipeline.mObjectAlphaProgram.disableTexture(LLPipeline::GLSL_ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
		LLCubeMap* cube_map = gSky.mVOSkyp->getCubeMap();
		if (envTex >= 0 && cube_map)
		{
			cube_map->restoreMatrix();
		}
		gPipeline.mObjectAlphaProgram.disableTexture(LLPipeline::GLSL_SCATTER_MAP);
		gPipeline.mObjectAlphaProgram.disableTexture(LLPipeline::GLSL_DIFFUSE_MAP);

		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE_2D);
	}

	if (sShowDebugAlpha)
	{
		gPipeline.disableLights();
		if ((mVertexShaderLevel > 0))
		{
			gPipeline.mHighlightProgram.bind();
		}
		
		LLViewerImage::sSmokeImagep->bind();
		LLOverrideFaceColor override_color(this, 1.f, 0.f, 0.f, 1.f);
		glColor4f(1.f, 0.f, 0.f, 1.f); // in case vertex shaders are enabled
		glDisableClientState(GL_COLOR_ARRAY);
		
		for (S32 i = 0; i < NUM_ALPHA_BINS; i++)
		{
			if (distance_bins[i].count() < 100)
			{
				face_list_t pri_queue;
				pri_queue.reserve(distance_bins[i].count());
				for (j = 0; j < distance_bins[i].count(); j++)
				{
					pri_queue.push_back(distance_bins[i][j]);
				}
				std::sort(pri_queue.begin(), pri_queue.end(), LLFace::CompareDistanceGreater());

				for (face_list_t::iterator iter = pri_queue.begin(); iter != pri_queue.end(); iter++)
				{
					const LLFace &face = *(*iter);
					face.renderIndexed(getRawIndices());
					mIndicesDrawn += face.getIndicesCount();
				}
			}
			else
			{
				for (j = 0; j < distance_bins[i].count(); j++)
				{
					const LLFace &face = *distance_bins[i][j];
					face.renderIndexed(getRawIndices());
					mIndicesDrawn += face.getIndicesCount();
				}
			}
		}

		if ((mVertexShaderLevel > 0))
		{
			gPipeline.mHighlightProgram.unbind();
		}

	}

}

void LLDrawPoolAlpha::renderForSelect()
{
	if (mDrawFace.empty() || !mMemory.count())
	{
		return;
	}

	// force faces on focus object to proper alpha cutoff based on object bbox distance
	if (gAgent.getFocusObject())
	{
		LLDrawable* drawablep = gAgent.getFocusObject()->mDrawable;

		if (drawablep)
		{
			const S32 num_faces = drawablep->getNumFaces();

			for (S32 f = 0; f < num_faces; f++)
			{
				LLFace* facep = drawablep->getFace(f);
				facep->mDistance = gAgent.getFocusObjectDist();
			}
		}
	}

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);

	LLGLSObjectSelectAlpha gls_alpha;

	glBlendFunc(GL_ONE, GL_ZERO);
	glAlphaFunc(gPickTransparent ? GL_GEQUAL : GL_GREATER, 0.f);

	bindGLVertexPointer();
	bindGLTexCoordPointer();

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_MODULATE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB,		GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB,	GL_SRC_ALPHA);

	LLDynamicArray<LLFace*>* distance_bins;
	distance_bins = mDistanceBins;

	S32 j;
	S32 num_bins_no_alpha_test = (gPickAlphaThreshold != 0.f) ? 
		(NUM_ALPHA_BINS - llmax(2, (S32)(ALPHA_FALLOFF_START_DISTANCE * mInvBinSize))) : 
		NUM_ALPHA_BINS;

	S32 i;
	for (i = 0; i < num_bins_no_alpha_test; i++)
	{
		S32 distance_bin_size = distance_bins[i].count();
		for (j = 0; j < distance_bin_size; j++)
		{
			const LLFace &face = *distance_bins[i][j];
			if (face.getDrawable() && !face.getDrawable()->isDead() && (face.getViewerObject()->mGLName))
			{
				face.bindTexture();
				face.renderForSelect();
			}
		}
	}

	for (i = num_bins_no_alpha_test; i < NUM_ALPHA_BINS; i++)
	{
		S32 distance_bin_size = distance_bins[i].count();
		if (distance_bin_size)
		{
			for (j = 0; j < distance_bin_size; j++)
			{
				const LLFace &face = *distance_bins[i][j];

				glAlphaFunc(GL_GEQUAL, face.mAlphaFade * gPickAlphaTargetThreshold);

				if (face.getDrawable() && !face.getDrawable()->isDead() && (face.getViewerObject()->mGLName))
				{
					face.bindTexture();
					face.renderForSelect();
				}
			}
		}
	}

	glAlphaFunc(GL_GREATER, 0.01f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
}


void LLDrawPoolAlpha::renderFaceSelected(LLFace *facep, 
									LLImageGL *image, 
									const LLColor4 &color,
									const S32 index_offset, const S32 index_count)
{
	facep->renderSelected(image, color, index_offset, index_count);
}


void LLDrawPoolAlpha::resetDrawOrders()
{
	LLDrawPool::resetDrawOrders();

	for (S32 i = 0; i < NUM_ALPHA_BINS; i++)
	{
		mDistanceBins[i].resize(0);
	}
}

BOOL LLDrawPoolAlpha::verify() const
{
	S32 i, j;
	BOOL ok;
	ok = LLDrawPool::verify();
	for (i = 0; i < NUM_ALPHA_BINS; i++)
	{
		for (j = 0; j < mDistanceBins[i].count(); j++)
		{
			const LLFace &face = *mDistanceBins[i][j];
			if (!face.verify())
			{
				ok = FALSE;
			}
		}
	}
	return ok;
}

LLViewerImage *LLDrawPoolAlpha::getDebugTexture()
{
	return LLViewerImage::sSmokeImagep;
}


LLColor3 LLDrawPoolAlpha::getDebugColor() const
{
	return LLColor3(1.f, 0.f, 0.f);
}

S32 LLDrawPoolAlpha::getMaterialAttribIndex()
{
	return gPipeline.mObjectAlphaProgram.mAttribute[LLPipeline::GLSL_MATERIAL_COLOR];
}

// virtual
void LLDrawPoolAlpha::enableShade()
{
	glDisableClientState(GL_COLOR_ARRAY);
}

// virtual
void LLDrawPoolAlpha::disableShade()
{
	glEnableClientState(GL_COLOR_ARRAY);
}

// virtual
void LLDrawPoolAlpha::setShade(F32 shade)
{
	glColor4f(0,0,0,shade);
}
