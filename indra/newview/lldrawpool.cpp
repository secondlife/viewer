/** 
 * @file lldrawpool.cpp
 * @brief LLDrawPool class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpool.h"

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
#include "lldrawpoolstars.h"
#include "lldrawpooltree.h"
#include "lldrawpoolterrain.h"
#include "lldrawpoolwater.h"
#include "llface.h"
#include "llviewerobjectlist.h" // For debug listing.
#include "pipeline.h"

S32 LLDrawPool::sNumDrawPools = 0;


//=============================
// Draw Pool Implementation
//=============================
LLDrawPool *LLDrawPool::createPool(const U32 type, LLViewerImage *tex0)
{
	LLDrawPool *poolp = NULL;
	switch (type)
	{
	case POOL_SIMPLE:
		poolp = new LLDrawPoolSimple();
		break;
	case POOL_GLOW:
		poolp = new LLDrawPoolGlow();
		break;
	case POOL_ALPHA:
		poolp = new LLDrawPoolAlpha();
		break;
	case POOL_ALPHA_POST_WATER:
		poolp = new LLDrawPoolAlphaPostWater();
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
	case POOL_STARS:
		poolp = new LLDrawPoolStars();
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
	mIndicesDrawn = 0;
}

LLDrawPool::~LLDrawPool()
{

}

LLViewerImage *LLDrawPool::getDebugTexture()
{
	return NULL;
}

//virtual
void LLDrawPool::beginRenderPass( S32 pass )
{
}

//virtual
void LLDrawPool::endRenderPass( S32 pass )
{
	glDisableClientState ( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState ( GL_COLOR_ARRAY );
	glDisableClientState ( GL_NORMAL_ARRAY );
}

U32 LLDrawPool::getTrianglesDrawn() const
{
	return mIndicesDrawn / 3;
}

void LLDrawPool::resetTrianglesDrawn()
{
	mIndicesDrawn = 0;
}

void LLDrawPool::addIndicesDrawn(const U32 indices)
{
	mIndicesDrawn += indices;
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

void LLFacePool::dirtyTextures(const std::set<LLViewerImage*>& textures)
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
			//facep->enableLights();
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
			facep->bindTexture(stage);
			facep->enableLights();
			res += facep->renderIndexed();
		}
	}
	return res;
}

void LLFacePool::drawLoop()
{
	if (!mDrawFace.empty())
	{
		mIndicesDrawn += drawLoop(mDrawFace);
	}
}

void LLFacePool::renderFaceSelected(LLFace *facep, 
									LLImageGL *image, 
									const LLColor4 &color,
									const S32 index_offset, const S32 index_count)
{
}

void LLFacePool::renderVisibility()
{
	if (mDrawFace.empty())
	{
		return;
	}

	// SJB: Note: This may be broken now. If you need it, fix it :)
	
	glLineWidth(1.0);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glTranslatef(-0.4f,-0.3f,0);

	float table[7][3] = { 
		{ 1,0,0 },
		{ 0,1,0 },
		{ 1,1,0 },
		{ 0,0,1 },
		{ 1,0,1 },
		{ 0,1,1 },
		{ 1,1,1 }
	};

	glColor4f(0,0,0,0.5);
	glBegin(GL_POLYGON);
	glVertex3f(-0.5f,-0.5f,1.0f);
	glVertex3f(+0.5f,-0.5f,1.0f);
	glVertex3f(+0.5f,+0.5f,1.0f);
	glVertex3f(-0.5f,+0.5f,1.0f);
	glVertex3f(-0.5f,-0.5f,1.0f);
	glEnd();
	
	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		LLFace *face = *iter;

		S32 geom_count = face->getGeomCount();
		for (S32 j=0;j<geom_count;j++)
		{
			LLVector3 p1;
			LLVector3 p2;

			intptr_t p = ((intptr_t)face*13) % 7;
			F32 r = table[p][0];
			F32 g = table[p][1];
			F32 b = table[p][2];

			//p1.mV[1] = y;
			//p2.mV[1] = y;

			p1.mV[2] = 1.0;
			p2.mV[2] = 1.0;

			glColor4f(r,g,b,0.5f);

			glBegin(GL_LINE_STRIP);
			glVertex3fv(p1.mV);
			glVertex3fv(p2.mV);
			glEnd();

		}		
	}

	glColor4f(1,1,1,1);
	glBegin(GL_LINE_STRIP);
	glVertex3f(-0.5f,-0.5f,1.0f);
	glVertex3f(+0.5f,-0.5f,1.0f);
	glVertex3f(+0.5f,+0.5f,1.0f);
	glVertex3f(-0.5f,+0.5f,1.0f);
	glVertex3f(-0.5f,-0.5f,1.0f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

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

LLViewerImage *LLFacePool::getTexture()
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
	if (mPool->getVertexShaderLevel() > 0 && mPool->getMaterialAttribIndex() > 0)
	{
		glVertexAttrib4fvARB(mPool->getMaterialAttribIndex(), color.mV);
	}
	else
	{
		glColor4fv(color.mV);
	}
}

void LLFacePool::LLOverrideFaceColor::setColor(const LLColor4U& color)
{
	if (mPool->getVertexShaderLevel() > 0 && mPool->getMaterialAttribIndex() > 0)
	{
		glVertexAttrib4ubvARB(mPool->getMaterialAttribIndex(), color.mV);
	}
	else
	{
		glColor4ubv(color.mV);
	}
}

void LLFacePool::LLOverrideFaceColor::setColor(F32 r, F32 g, F32 b, F32 a)
{
	if (mPool->getVertexShaderLevel() > 0 && mPool->getMaterialAttribIndex() > 0)
	{
		glVertexAttrib4fARB(mPool->getMaterialAttribIndex(), r,g,b,a);
	}
	else
	{
		glColor4f(r,g,b,a);
	}
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
	std::vector<LLDrawInfo*>& draw_info = group->mDrawMap[type];
	
	for (std::vector<LLDrawInfo*>::const_iterator k = draw_info.begin(); k != draw_info.end(); ++k)
	{
		LLDrawInfo& params = **k;
		pushBatch(params, mask, texture);
	}
}

void LLRenderPass::renderInvisible(U32 mask)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkClientArrays(mask);
#endif
	
	std::vector<LLDrawInfo*>& draw_info = gPipeline.mRenderMap[LLRenderPass::PASS_INVISIBLE];

	U32* indices_pointer = NULL;
	for (std::vector<LLDrawInfo*>::iterator i = draw_info.begin(); i != draw_info.end(); ++i)
	{
		LLDrawInfo& params = **i;
		params.mVertexBuffer->setBuffer(mask);
		indices_pointer = (U32*) params.mVertexBuffer->getIndicesPointer();
		glDrawRangeElements(GL_TRIANGLES, params.mStart, params.mEnd, params.mCount,
							GL_UNSIGNED_INT, indices_pointer+params.mOffset);
		gPipeline.mTrianglesDrawn += params.mCount/3;
	}
}

void LLRenderPass::renderTexture(U32 type, U32 mask)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkClientArrays(mask);
#endif

	std::vector<LLDrawInfo*>& draw_info = gPipeline.mRenderMap[type];

	for (std::vector<LLDrawInfo*>::iterator i = draw_info.begin(); i != draw_info.end(); ++i)
	{
		LLDrawInfo& params = **i;
		pushBatch(params, mask, TRUE);
	}
}

void LLRenderPass::pushBatch(LLDrawInfo& params, U32 mask, BOOL texture)
{
	if (params.mVertexBuffer.isNull())
	{
		return;
	}

	if (texture)
	{
		if (params.mTexture.notNull())
		{
			params.mTexture->bind();
			if (params.mTextureMatrix)
			{
				glMatrixMode(GL_TEXTURE);
				glLoadMatrixf((GLfloat*) params.mTextureMatrix->mMatrix);
			}
			params.mTexture->addTextureStats(params.mVSize);
		}
		else
		{
			LLImageGL::unbindTexture(0);
		}
	}
	
	params.mVertexBuffer->setBuffer(mask);
	U32* indices_pointer = (U32*) params.mVertexBuffer->getIndicesPointer();
	glDrawRangeElements(GL_TRIANGLES, params.mStart, params.mEnd, params.mCount,
						GL_UNSIGNED_INT, indices_pointer+params.mOffset);
	gPipeline.mTrianglesDrawn += params.mCount/3;

	if (params.mTextureMatrix && texture && params.mTexture.notNull())
	{
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
}

void LLRenderPass::renderActive(U32 type, U32 mask, BOOL texture)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkClientArrays(mask);
#endif

	LLSpatialBridge* last_bridge = NULL;
	glPushMatrix();
	
	for (LLSpatialGroup::sg_vector_t::iterator i = gPipeline.mActiveGroups.begin(); i != gPipeline.mActiveGroups.end(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (!group->isDead() &&
			gPipeline.hasRenderType(group->mSpatialPartition->mDrawableType) &&
			group->mDrawMap.find(type) != group->mDrawMap.end())
		{
			LLSpatialBridge* bridge = (LLSpatialBridge*) group->mSpatialPartition;
			if (bridge != last_bridge)
			{
				glPopMatrix();
				glPushMatrix();
				glMultMatrixf((F32*) bridge->mDrawable->getRenderMatrix().mMatrix);
				last_bridge = bridge;
			}

			renderGroup(group,type,mask,texture);
		}
	}
	
	glPopMatrix();
}

void LLRenderPass::renderStatic(U32 type, U32 mask, BOOL texture)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkClientArrays(mask);
#endif

	for (LLSpatialGroup::sg_vector_t::iterator i = gPipeline.mVisibleGroups.begin(); i != gPipeline.mVisibleGroups.end(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (!group->isDead() &&
			gPipeline.hasRenderType(group->mSpatialPartition->mDrawableType) &&
			group->mDrawMap.find(type) != group->mDrawMap.end())
		{
			renderGroup(group,type,mask,texture);
		}
	}
}
