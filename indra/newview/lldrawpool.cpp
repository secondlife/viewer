/** 
 * @file lldrawpool.cpp
 * @brief LLDrawPool class implementation
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

#include "lldrawpool.h"
#include "llrender.h"
#include "llfasttimer.h"
#include "llviewercontrol.h"

#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolclouds.h"
#include "lldrawpoolground.h"
#include "lldrawpoolsimple.h"
#include "lldrawpoolsky.h"
#include "lldrawpooltree.h"
#include "lldrawpoolterrain.h"
#include "lldrawpoolwater.h"
#include "llface.h"
#include "llviewerobjectlist.h" // For debug listing.
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llviewercamera.h"
#include "lldrawpoolwlsky.h"

S32 LLDrawPool::sNumDrawPools = 0;

//=============================
// Draw Pool Implementation
//=============================
LLDrawPool *LLDrawPool::createPool(const U32 type, LLViewerTexture *tex0)
{
	LLDrawPool *poolp = NULL;
	switch (type)
	{
	case POOL_SIMPLE:
		poolp = new LLDrawPoolSimple();
		break;
	case POOL_GRASS:
		poolp = new LLDrawPoolGrass();
		break;
	case POOL_FULLBRIGHT:
		poolp = new LLDrawPoolFullbright();
		break;
	case POOL_INVISIBLE:
		poolp = new LLDrawPoolInvisible();
		break;
	case POOL_GLOW:
		poolp = new LLDrawPoolGlow();
		break;
	case POOL_ALPHA:
		poolp = new LLDrawPoolAlpha();
		break;
	case POOL_AVATAR:
		poolp = new LLDrawPoolAvatar();
		break;
	case POOL_TREE:
		poolp = new LLDrawPoolTree(tex0);
		break;
	case POOL_TERRAIN:
		poolp = new LLDrawPoolTerrain(tex0);
		break;
	case POOL_SKY:
		poolp = new LLDrawPoolSky();
		break;
	case POOL_WATER:
		poolp = new LLDrawPoolWater();
		break;
	case POOL_GROUND:
		poolp = new LLDrawPoolGround();
		break;
	case POOL_BUMP:
		poolp = new LLDrawPoolBump();
		break;
	case POOL_WL_SKY:
		poolp = new LLDrawPoolWLSky();
		break;
	default:
		llerrs << "Unknown draw pool type!" << llendl;
		return NULL;
	}

	llassert(poolp->mType == type);
	return poolp;
}

LLDrawPool::LLDrawPool(const U32 type)
{
	mType = type;
	sNumDrawPools++;
	mId = sNumDrawPools;
	mVertexShaderLevel = 0;
}

LLDrawPool::~LLDrawPool()
{

}

LLViewerTexture *LLDrawPool::getDebugTexture()
{
	return NULL;
}

//virtual
void LLDrawPool::beginRenderPass( S32 pass )
{
}

//virtual 
S32	 LLDrawPool::getNumPasses()
{
	return 1;
}
	
//virtual 
void LLDrawPool::beginDeferredPass(S32 pass)
{

}

//virtual 
void LLDrawPool::endDeferredPass(S32 pass)
{

}

//virtual 
S32 LLDrawPool::getNumDeferredPasses()
{
	return 0;
}

//virtual 
void LLDrawPool::renderDeferred(S32 pass)
{

}

//virtual 
void LLDrawPool::beginPostDeferredPass(S32 pass)
{

}

//virtual 
void LLDrawPool::endPostDeferredPass(S32 pass)
{

}

//virtual 
S32 LLDrawPool::getNumPostDeferredPasses()
{
	return 0;
}

//virtual 
void LLDrawPool::renderPostDeferred(S32 pass)
{

}

//virtual
void LLDrawPool::endRenderPass( S32 pass )
{
}

//virtual 
void LLDrawPool::beginShadowPass(S32 pass)
{

}

//virtual 
void LLDrawPool::endShadowPass(S32 pass)
{

}

//virtual 
S32 LLDrawPool::getNumShadowPasses()
{
	return 0;
}

//virtual 
void LLDrawPool::renderShadow(S32 pass)
{

}

//=============================
// Face Pool Implementation
//=============================
LLFacePool::LLFacePool(const U32 type)
: LLDrawPool(type)
{
	resetDrawOrders();
}

LLFacePool::~LLFacePool()
{
	destroy();
}

void LLFacePool::destroy()
{
	if (!mReferences.empty())
	{
		llinfos << mReferences.size() << " references left on deletion of draw pool!" << llendl;
	}
}

void LLFacePool::dirtyTextures(const std::set<LLViewerFetchedTexture*>& textures)
{
}

BOOL LLFacePool::moveFace(LLFace *face, LLDrawPool *poolp, BOOL copy_data)
{
	return TRUE;
}

// static
S32 LLFacePool::drawLoop(face_array_t& face_list)
{
	S32 res = 0;
	if (!face_list.empty())
	{
		for (std::vector<LLFace*>::iterator iter = face_list.begin();
			 iter != face_list.end(); iter++)
		{
			LLFace *facep = *iter;
			res += facep->renderIndexed();
		}
	}
	return res;
}

// static
S32 LLFacePool::drawLoopSetTex(face_array_t& face_list, S32 stage)
{
	S32 res = 0;
	if (!face_list.empty())
	{
		for (std::vector<LLFace*>::iterator iter = face_list.begin();
			 iter != face_list.end(); iter++)
		{
			LLFace *facep = *iter;
			gGL.getTexUnit(stage)->bind(facep->getTexture(), TRUE) ;
			gGL.getTexUnit(0)->activate();
			res += facep->renderIndexed();
		}
	}
	return res;
}

void LLFacePool::drawLoop()
{
	if (!mDrawFace.empty())
	{
		drawLoop(mDrawFace);
	}
}

void LLFacePool::enqueue(LLFace* facep)
{
	mDrawFace.push_back(facep);
}

// virtual
BOOL LLFacePool::addFace(LLFace *facep)
{
	addFaceReference(facep);
	return TRUE;
}

// virtual
BOOL LLFacePool::removeFace(LLFace *facep)
{
	removeFaceReference(facep);

	vector_replace_with_last(mDrawFace, facep);

	return TRUE;
}

// Not absolutely sure if we should be resetting all of the chained pools as well - djs
void LLFacePool::resetDrawOrders()
{
	mDrawFace.resize(0);
}

LLViewerTexture *LLFacePool::getTexture()
{
	return NULL;
}

void LLFacePool::removeFaceReference(LLFace *facep)
{
	if (facep->getReferenceIndex() != -1)
	{
		if (facep->getReferenceIndex() != (S32)mReferences.size())
		{
			LLFace *back = mReferences.back();
			mReferences[facep->getReferenceIndex()] = back;
			back->setReferenceIndex(facep->getReferenceIndex());
		}
		mReferences.pop_back();
	}
	facep->setReferenceIndex(-1);
}

void LLFacePool::addFaceReference(LLFace *facep)
{
	if (-1 == facep->getReferenceIndex())
	{
		facep->setReferenceIndex(mReferences.size());
		mReferences.push_back(facep);
	}
}

BOOL LLFacePool::verify() const
{
	BOOL ok = TRUE;
	
	for (std::vector<LLFace*>::const_iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		const LLFace* facep = *iter;
		if (facep->getPool() != this)
		{
			llinfos << "Face in wrong pool!" << llendl;
			facep->printDebugInfo();
			ok = FALSE;
		}
		else if (!facep->verify())
		{
			ok = FALSE;
		}
	}

	return ok;
}

void LLFacePool::printDebugInfo() const
{
	llinfos << "Pool " << this << " Type: " << getType() << llendl;
}

BOOL LLFacePool::LLOverrideFaceColor::sOverrideFaceColor = FALSE;

void LLFacePool::LLOverrideFaceColor::setColor(const LLColor4& color)
{
	glColor4fv(color.mV);
}

void LLFacePool::LLOverrideFaceColor::setColor(const LLColor4U& color)
{
	glColor4ubv(color.mV);
}

void LLFacePool::LLOverrideFaceColor::setColor(F32 r, F32 g, F32 b, F32 a)
{
	glColor4f(r,g,b,a);
}


//=============================
// Render Pass Implementation
//=============================
LLRenderPass::LLRenderPass(const U32 type)
: LLDrawPool(type)
{

}

LLRenderPass::~LLRenderPass()
{

}

LLDrawPool* LLRenderPass::instancePool()
{
#if LL_RELEASE_FOR_DOWNLOAD
	llwarns << "Attempting to instance a render pass.  Invalid operation." << llendl;
#else
	llerrs << "Attempting to instance a render pass.  Invalid operation." << llendl;
#endif
	return NULL;
}

void LLRenderPass::renderGroup(LLSpatialGroup* group, U32 type, U32 mask, BOOL texture)
{					
	LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[type];
	
	for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)	
	{
		LLDrawInfo *pparams = *k;
		if (pparams) {
			pushBatch(*pparams, mask, texture);
		}
	}
}

void LLRenderPass::renderTexture(U32 type, U32 mask)
{
	pushBatches(type, mask, TRUE);
}

void LLRenderPass::pushBatches(U32 type, U32 mask, BOOL texture)
{
	for (LLCullResult::drawinfo_list_t::iterator i = gPipeline.beginRenderMap(type); i != gPipeline.endRenderMap(type); ++i)	
	{
		LLDrawInfo* pparams = *i;
		if (pparams) 
		{
			pushBatch(*pparams, mask, texture);
		}
	}
}

void LLRenderPass::applyModelMatrix(LLDrawInfo& params)
{
	if (params.mModelMatrix != gGLLastMatrix)
	{
		gGLLastMatrix = params.mModelMatrix;
		glLoadMatrixd(gGLModelView);
		if (params.mModelMatrix)
		{
			glMultMatrixf((GLfloat*) params.mModelMatrix->mMatrix);
		}
		gPipeline.mMatrixOpCount++;
	}
}

void LLRenderPass::pushBatch(LLDrawInfo& params, U32 mask, BOOL texture)
{
	applyModelMatrix(params);

	if (texture)
	{
		if (params.mTexture.notNull())
		{
			params.mTexture->addTextureStats(params.mVSize);
			gGL.getTexUnit(0)->bind(params.mTexture, TRUE) ;
			if (params.mTextureMatrix)
			{
				glMatrixMode(GL_TEXTURE);
				glLoadMatrixf((GLfloat*) params.mTextureMatrix->mMatrix);
				gPipeline.mTextureMatrixOps++;
			}
		}
		else
		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		}
	}
	
	if (params.mVertexBuffer.notNull())
	{
		if (params.mGroup)
		{
			params.mGroup->rebuildMesh();
		}
		params.mVertexBuffer->setBuffer(mask);
		params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
		gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
	}

	if (params.mTextureMatrix && texture && params.mTexture.notNull())
	{
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
}

void LLRenderPass::renderGroups(U32 type, U32 mask, BOOL texture)
{
	gPipeline.renderGroups(this, type, mask, texture);
}
