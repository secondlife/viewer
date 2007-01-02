/** 
 * @file lldrawpoolsimple.cpp
 * @brief LLDrawPoolSimple class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolsimple.h"

#include "llagent.h"
#include "llagparray.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "pipeline.h"

S32 LLDrawPoolSimple::sDiffTex = 0;

LLDrawPoolSimple::LLDrawPoolSimple(LLViewerImage *texturep) :
	LLDrawPool(POOL_SIMPLE,
			   DATA_SIMPLE_IL_MASK | DATA_COLORS_MASK,
			   DATA_SIMPLE_NIL_MASK),  // ady temp
	mTexturep(texturep)
{
}

LLDrawPool *LLDrawPoolSimple::instancePool()
{
	return new LLDrawPoolSimple(mTexturep);
}

BOOL LLDrawPoolSimple::match(LLFace* last_face, LLFace* facep)  
{
	if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_CHAIN_FACES) &&
		!last_face->isState(LLFace::LIGHT | LLFace::FULLBRIGHT) &&
		!facep->isState(LLFace::LIGHT | LLFace::FULLBRIGHT) &&
		facep->getIndicesStart() == last_face->getIndicesStart()+last_face->getIndicesCount() &&
		facep->getRenderColor() == last_face->getRenderColor())
	{
		if (facep->isState(LLFace::GLOBAL))
		{
			if (last_face->isState(LLFace::GLOBAL))
			{
				return TRUE;
			}
		}
		else
		{
			if (!last_face->isState(LLFace::GLOBAL))
			{
				if (last_face->getRenderMatrix() == facep->getRenderMatrix())
				{
					return TRUE;
				}
			}
		}
	}
	
	return FALSE;
}

void LLDrawPoolSimple::prerender()
{
	mVertexShaderLevel = gPipeline.getVertexShaderLevel(LLPipeline::SHADER_OBJECT);
}

void LLDrawPoolSimple::beginRenderPass(S32 pass)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	if (gPipeline.getLightingDetail() >= 2)
	{
		glEnableClientState(GL_COLOR_ARRAY);
	}

	if (mVertexShaderLevel > 0)
	{
		S32 scatterTex = gPipeline.mObjectSimpleProgram.enableTexture(LLPipeline::GLSL_SCATTER_MAP);
		LLViewerImage::bindTexture(gSky.mVOSkyp->getScatterMap(), scatterTex);
		sDiffTex = gPipeline.mObjectSimpleProgram.enableTexture(LLPipeline::GLSL_DIFFUSE_MAP);
	}
}


void LLDrawPoolSimple::render(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SIMPLE);
	if (mDrawFace.empty())
	{
		return;
	}

	bindGLVertexPointer();
	bindGLTexCoordPointer();
	bindGLNormalPointer();
	if (gPipeline.getLightingDetail() >= 2)
	{
		bindGLColorPointer();
	}

	LLViewerImage* tex = getTexture();
	LLGLState alpha_test(GL_ALPHA_TEST, FALSE);
	LLGLState blend(GL_BLEND, FALSE);

	if (tex)
	{
		LLViewerImage::bindTexture(tex,sDiffTex);
		if (tex->getPrimaryFormat() == GL_ALPHA)
		{
			// Enable Invisibility Hack
			alpha_test.enable();
			blend.enable();
		}
	}
	else
	{
		LLImageGL::unbindTexture(sDiffTex, GL_TEXTURE_2D);
	}

	drawLoop();
}

void LLDrawPoolSimple::endRenderPass(S32 pass)
{
	if (mVertexShaderLevel > 0)
	{
		gPipeline.mObjectSimpleProgram.disableTexture(LLPipeline::GLSL_SCATTER_MAP);
		gPipeline.mObjectSimpleProgram.disableTexture(LLPipeline::GLSL_DIFFUSE_MAP);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE_2D);
	}
	
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	if (gPipeline.getLightingDetail() >= 2)
	{
		glDisableClientState(GL_COLOR_ARRAY);
	}
}

void LLDrawPoolSimple::renderForSelect()
{
	if (mDrawFace.empty() || !mMemory.count())
	{
		return;
	}

	glEnableClientState ( GL_VERTEX_ARRAY );

	bindGLVertexPointer();

	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		LLFace *facep = *iter;
		LLDrawable *drawable = facep->getDrawable();
		if (drawable && !drawable->isDead() && (facep->getViewerObject()->mGLName))
		{
			facep->renderForSelect();
		}
	}
}


void LLDrawPoolSimple::renderFaceSelected(LLFace *facep, 
									LLImageGL *image, 
									const LLColor4 &color,
									const S32 index_offset, const S32 index_count)
{
	facep->renderSelected(image, color, index_offset, index_count);
}


void LLDrawPoolSimple::dirtyTexture(const LLViewerImage *texturep)
{
	if (mTexturep == texturep)
	{
		for (std::vector<LLFace*>::iterator iter = mReferences.begin();
			 iter != mReferences.end(); iter++)
		{
			LLFace *facep = *iter;
			gPipeline.markTextured(facep->getDrawable());
		}
	}
}

LLViewerImage *LLDrawPoolSimple::getTexture()
{
	return mTexturep;
}

LLViewerImage *LLDrawPoolSimple::getDebugTexture()
{
	return mTexturep;
}

LLColor3 LLDrawPoolSimple::getDebugColor() const
{
	return LLColor3(1.f, 1.f, 1.f);
}

S32 LLDrawPoolSimple::getMaterialAttribIndex()
{
	return gPipeline.mObjectSimpleProgram.mAttribute[LLPipeline::GLSL_MATERIAL_COLOR];
}

// virtual
void LLDrawPoolSimple::enableShade()
{
	glDisableClientState(GL_COLOR_ARRAY);
}

// virtual
void LLDrawPoolSimple::disableShade()
{
	glEnableClientState(GL_COLOR_ARRAY);
}

// virtual
void LLDrawPoolSimple::setShade(F32 shade)
{
	glColor4f(0,0,0,shade);
}
