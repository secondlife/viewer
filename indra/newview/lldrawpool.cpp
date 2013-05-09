/** 
 * @file lldrawpool.cpp
 * @brief LLDrawPool class implementation
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

#include "lldrawpool.h"
#include "llrender.h"
#include "llfasttimer.h"
#include "llviewercontrol.h"

#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
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
	case POOL_VOIDWATER:
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
	mSkipRender = false;
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
	/*for (U32 i = 0; i < gGLManager.mNumTextureImageUnits; i++)
	{ //dummy cleanup of any currently bound textures
		if (gGL.getTexUnit(i)->getCurrType() != LLTexUnit::TT_NONE)
		{
			gGL.getTexUnit(i)->unbind(gGL.getTexUnit(i)->getCurrType());
			gGL.getTexUnit(i)->disable();
		}
	}*/

	//make sure channel 0 is active channel
	gGL.getTexUnit(0)->activate();
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
	gGL.diffuseColor4fv(color.mV);
}

void LLFacePool::LLOverrideFaceColor::setColor(const LLColor4U& color)
{
	glColor4ubv(color.mV);
}

void LLFacePool::LLOverrideFaceColor::setColor(F32 r, F32 g, F32 b, F32 a)
{
	gGL.diffuseColor4f(r,g,b,a);
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

void LLRenderPass::pushBatches(U32 type, U32 mask, BOOL texture, BOOL batch_textures)
{
	for (LLCullResult::drawinfo_iterator i = gPipeline.beginRenderMap(type); i != gPipeline.endRenderMap(type); ++i)	
	{
		LLDrawInfo* pparams = *i;
		if (pparams) 
		{
			pushBatch(*pparams, mask, texture, batch_textures);
		}
	}
}

void LLRenderPass::applyModelMatrix(LLDrawInfo& params)
{
	if (params.mModelMatrix != gGLLastMatrix)
	{
		gGLLastMatrix = params.mModelMatrix;
		gGL.loadMatrix(gGLModelView);
		if (params.mModelMatrix)
		{
			llassert(gGL.getMatrixMode() == LLRender::MM_MODELVIEW);
			gGL.multMatrix((GLfloat*) params.mModelMatrix->mMatrix);
		}
		gPipeline.mMatrixOpCount++;
	}
}

void LLRenderPass::pushBatch(LLDrawInfo& params, U32 mask, BOOL texture, BOOL batch_textures)
{
	applyModelMatrix(params);

	bool tex_setup = false;

	if (texture)
	{
		if (batch_textures && params.mTextureList.size() > 1)
		{
			for (U32 i = 0; i < params.mTextureList.size(); ++i)
			{
				if (params.mTextureList[i].notNull())
				{
					gGL.getTexUnit(i)->bind(params.mTextureList[i], TRUE);
				}
			}
		}
		else
		{ //not batching textures or batch has only 1 texture -- might need a texture matrix
			if (params.mTexture.notNull())
			{
				params.mTexture->addTextureStats(params.mVSize);
				gGL.getTexUnit(0)->bind(params.mTexture, TRUE) ;
				if (params.mTextureMatrix)
				{
					tex_setup = true;
					gGL.getTexUnit(0)->activate();
					gGL.matrixMode(LLRender::MM_TEXTURE);
					gGL.loadMatrix((GLfloat*) params.mTextureMatrix->mMatrix);
					gPipeline.mTextureMatrixOps++;
				}
			}
			else
			{
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			}
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

	if (tex_setup)
	{
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
}

void LLRenderPass::renderGroups(U32 type, U32 mask, BOOL texture)
{
	gPipeline.renderGroups(this, type, mask, texture);
}
