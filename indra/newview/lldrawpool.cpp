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

#include "llagparray.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolclouds.h"
#include "lldrawpoolground.h"
#include "lldrawpoolmedia.h"
#include "lldrawpoolsimple.h"
#include "lldrawpoolsky.h"
#include "lldrawpoolstars.h"
#include "lldrawpooltree.h"
#include "lldrawpooltreenew.h"
#include "lldrawpoolterrain.h"
#include "lldrawpoolwater.h"
#include "lldrawpoolhud.h"
#include "llface.h"
#include "llviewerobjectlist.h" // For debug listing.
#include "llvotreenew.h"
#include "pipeline.h"

#include "llagparray.inl"

U32 LLDrawPool::sDataSizes[LLDrawPool::DATA_MAX_TYPES] =
{
	12, // DATA_VERTICES
	8,	// DATA_TEX_COORDS0
	8,	// DATA_TEX_COORDS1
	8,	// DATA_TEX_COORDS2
	8,	// DATA_TEX_COORDS3
	12, // DATA_NORMALS
	4,	// DATA_VERTEX_WEIGHTS,
	16, // DATA_CLOTHING_WEIGHTS
	12,	// DATA_BINORMALS
	4,	// DATA_COLORS
};

S32 LLDrawPool::sNumDrawPools = 0;

LLDrawPool *LLDrawPool::createPool(const U32 type, LLViewerImage *tex0)
{
	LLDrawPool *poolp = NULL;
	switch (type)
	{
	case POOL_SIMPLE:
		poolp = new LLDrawPoolSimple(tex0);
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
	case POOL_TREE_NEW:
		poolp = new LLDrawPoolTreeNew(tex0);			
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
	case POOL_CLOUDS:
		poolp = new LLDrawPoolClouds();
		break;
	case POOL_WATER:
		poolp = new LLDrawPoolWater();
		break;
	case POOL_GROUND:
		poolp = new LLDrawPoolGround();
		break;
	case POOL_BUMP:
		poolp = new LLDrawPoolBump(tex0);
		break;
	case POOL_MEDIA:
		poolp = new LLDrawPoolMedia(tex0);
		break;
	case POOL_HUD:
		poolp = new LLDrawPoolHUD();
		break;
	default:
		llerrs << "Unknown draw pool type!" << llendl;
		return NULL;
	}

	llassert(poolp->mType == type);
	return poolp;
}

LLDrawPool::LLDrawPool(const U32 type, const U32 data_mask_il, const U32 data_mask_nil)
{
	llassert(data_mask_il & DATA_VERTICES_MASK);
	S32 i;
	mType = type;
	sNumDrawPools++;
	mId = sNumDrawPools;

	mDataMaskIL = data_mask_il;
	mDataMaskNIL = data_mask_nil;

	U32 cur_mask = 0x01;
	U32 cur_offset = 0;
	for (i = 0; i < DATA_MAX_TYPES; i++)
	{
		mDataOffsets[i] = cur_offset;
		if (cur_mask & mDataMaskIL)
		{
			cur_offset += sDataSizes[i];
		}
		cur_mask <<= 1;
	}

	mStride = cur_offset;

	mCleanupUnused = FALSE;
	mIndicesDrawn = 0;
	mRebuildFreq = 128 + rand() % 5;
	mRebuildTime = 0;
	mGeneration  = 1;
	mSkippedVertices = 0;

	resetDrawOrders();
	resetVertexData(0);

	if (gGLManager.mHasATIVAO && !gGLManager.mIsRadeon9700)
	{
		// ATI 8500 doesn't like indices > 15 bit.
		mMaxVertices = DEFAULT_MAX_VERTICES/2;
	}
	else
	{
		mMaxVertices = DEFAULT_MAX_VERTICES;
	}

	// JC: This must happen last, as setUseAGP reads many of the
	// above variables.
	mUseAGP = FALSE;
	setUseAGP(gPipeline.usingAGP());

	for (i=0; i<NUM_BUCKETS; i++)
	{
		mFreeListGeomHead[i] = -1;
		mFreeListIndHead[i] = -1;
	}
	mVertexShaderLevel = 0;
}

void LLDrawPool::destroy()
{
	if (!mReferences.empty())
	{
		llinfos << mReferences.size() << " references left on deletion of draw pool!" << llendl;
	}
}


LLDrawPool::~LLDrawPool()
{
	destroy();

	llassert( gPipeline.findPool( getType(), getTexture() ) == NULL );
}

BOOL LLDrawPool::setUseAGP(BOOL use_agp)
{
	BOOL ok = TRUE;
	S32 vertex_count = mMemory.count() / mStride;
	if (vertex_count > mMaxVertices && use_agp)
	{
#ifdef DEBUG_AGP
		llwarns << "Allocating " << vertex_count << " vertices in pool type " << getType() << ", disabling AGP!" << llendl
#endif
		use_agp = FALSE;
		ok = FALSE;
	}

	if (mUseAGP != use_agp)
	{
		mUseAGP = use_agp;

		BOOL ok = TRUE;
		ok &= mMemory.setUseAGP(use_agp);

		if (mDataMaskNIL & DATA_VERTEX_WEIGHTS_MASK)
		{
			ok &= mWeights.setUseAGP(use_agp);
		}
		if (mDataMaskNIL & DATA_CLOTHING_WEIGHTS_MASK)
		{
			ok &= mClothingWeights.setUseAGP(use_agp);
		}

		if (!ok)
		{
			// Disable AGP if any one of these doesn't have AGP, we don't want to try
			// mixing AGP and non-agp arrays in a single pool.
#ifdef DEBUG_AGP
			llinfos << "Aborting using AGP because set failed on a mem block!" << llendl;
#endif
			setUseAGP(FALSE);
			ok = FALSE;
		}
	}
	return ok;
}

void LLDrawPool::flushAGP()
{
	mMemory.flushAGP();

	if (mDataMaskNIL & DATA_VERTEX_WEIGHTS_MASK)
	{
		mWeights.flushAGP();
	}
	if (mDataMaskNIL & DATA_CLOTHING_WEIGHTS_MASK)
	{
		mClothingWeights.flushAGP();
	}
}

void LLDrawPool::syncAGP()
{
	if (!getVertexCount())
	{
		return;
	}
	setUseAGP(gPipeline.usingAGP());

	BOOL all_agp_on = TRUE;
	mMemory.sync();
	all_agp_on &= mMemory.isAGP();

	if (mDataMaskNIL & DATA_VERTEX_WEIGHTS_MASK)
	{
		mWeights.sync();
		all_agp_on &= mWeights.isAGP();
	}

	if (mDataMaskNIL & DATA_CLOTHING_WEIGHTS_MASK)
	{
		mClothingWeights.sync();
		all_agp_on &= mClothingWeights.isAGP();
	}

	// Since sometimes AGP allocation is done during syncs, we need
	// to make sure that if AGP allocation fails, we fallback to non-agp.
	if (mUseAGP && !all_agp_on)
	{
#ifdef DEBUG_AGP
		llinfos << "setUseAGP false because of AGP sync failure!" << llendl;
#endif
		setUseAGP(FALSE);
	}
}

void LLDrawPool::dirtyTexture(const LLViewerImage *imagep)
{
}

BOOL LLDrawPool::moveFace(LLFace *face, LLDrawPool *poolp, BOOL copy_data)
{
	return TRUE;
}

// static
S32 LLDrawPool::drawLoop(face_array_t& face_list, const U32* index_array)
{
	S32 res = 0;
	if (!face_list.empty())
	{
		for (std::vector<LLFace*>::iterator iter = face_list.begin();
			 iter != face_list.end(); iter++)
		{
			LLFace *facep = *iter;
			if (facep->mSkipRender)
			{
				continue;
			}
			facep->enableLights();
			res += facep->renderIndexed(index_array);
		}
	}
	return res;
}

// static
S32 LLDrawPool::drawLoopSetTex(face_array_t& face_list, const U32* index_array, S32 stage)
{
	S32 res = 0;
	if (!face_list.empty())
	{
		for (std::vector<LLFace*>::iterator iter = face_list.begin();
			 iter != face_list.end(); iter++)
		{
			LLFace *facep = *iter;
			if (facep->mSkipRender)
			{
				continue;
			}
			facep->bindTexture(stage);
			facep->enableLights();
			res += facep->renderIndexed(index_array);
		}
	}
	return res;
}

void LLDrawPool::drawLoop()
{
	const U32* index_array = getRawIndices();	
	if (!mDrawFace.empty())
	{
		mIndicesDrawn += drawLoop(mDrawFace, index_array);
	}
}

BOOL LLDrawPool::getVertexStrider(LLStrider<LLVector3> &vertices, const U32 index)
{
	llassert(mDataMaskIL & LLDrawPool::DATA_VERTICES_MASK);
	vertices = (LLVector3*)(mMemory.getMem() + mDataOffsets[DATA_VERTICES] + index * mStride); 
	vertices.setStride(mStride);
	return TRUE;
}

BOOL LLDrawPool::getTexCoordStrider(LLStrider<LLVector2> &tex_coords, const U32 index, const U32 pass)
{
	llassert(mDataMaskIL & (LLDrawPool::DATA_TEX_COORDS0_MASK << pass));
	tex_coords = (LLVector2*)(mMemory.getMem() + mDataOffsets[DATA_TEX_COORDS0 + pass] + index * mStride); 
	tex_coords.setStride(mStride);
	return TRUE;
}


BOOL LLDrawPool::getVertexWeightStrider(LLStrider<F32> &vertex_weights, const U32 index)
{
	llassert(mDataMaskNIL & LLDrawPool::DATA_VERTEX_WEIGHTS_MASK);

	vertex_weights = &mWeights[index];
	vertex_weights.setStride( 0 );
	return TRUE;
}

BOOL LLDrawPool::getClothingWeightStrider(LLStrider<LLVector4> &clothing_weights, const U32 index)
{
	llassert(mDataMaskNIL & LLDrawPool::DATA_CLOTHING_WEIGHTS_MASK);

	clothing_weights= &mClothingWeights[index];
	clothing_weights.setStride( 0 );

	return TRUE;
}

BOOL LLDrawPool::getNormalStrider(LLStrider<LLVector3> &normals, const U32 index)
{
	llassert((mDataMaskIL) & LLDrawPool::DATA_NORMALS_MASK);

	normals = (LLVector3*)(mMemory.getMem() + mDataOffsets[DATA_NORMALS] + index * mStride); 

	normals.setStride( mStride );

	return TRUE;
}


BOOL LLDrawPool::getBinormalStrider(LLStrider<LLVector3> &binormals, const U32 index)
{
	llassert((mDataMaskIL) & LLDrawPool::DATA_BINORMALS_MASK);

	binormals = (LLVector3*)(mMemory.getMem() + mDataOffsets[DATA_BINORMALS] + index * mStride); 

	binormals.setStride( mStride );

	return TRUE;
}

BOOL LLDrawPool::getColorStrider(LLStrider<LLColor4U> &colors, const U32 index)
{
	llassert((mDataMaskIL) & LLDrawPool::DATA_COLORS_MASK);

	colors = (LLColor4U*)(mMemory.getMem() + mDataOffsets[DATA_COLORS] + index * mStride); 

	colors.setStride( mStride );

	return TRUE;
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
void LLDrawPool::renderFaceSelected(LLFace *facep, 
									LLImageGL *image, 
									const LLColor4 &color,
									const S32 index_offset, const S32 index_count)
{
}

void LLDrawPool::renderVisibility()
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

void LLDrawPool::enqueue(LLFace* facep)
{
	if (facep->isState(LLFace::BACKLIST))
	{
		mMoveFace.put(facep);
	}
	else
	{
#if ENABLE_FACE_LINKING
		facep->mSkipRender = FALSE;
		facep->mNextFace = NULL;
		
		if (mDrawFace.size() > 0)
		{
			LLFace* last_face = mDrawFace[mDrawFace.size()-1];
			if (match(last_face, facep))
			{
				last_face->link(facep);
			}
		}
#endif
		mDrawFace.put(facep);
	}
}

void LLDrawPool::bindGLVertexPointer()
{
	mMemory.bindGLVertexPointer(getStride(DATA_VERTICES), mDataOffsets[DATA_VERTICES]);
}

void LLDrawPool::bindGLTexCoordPointer(const U32 pass)
{
	mMemory.bindGLTexCoordPointer(getStride(DATA_TEX_COORDS0+pass), mDataOffsets[DATA_TEX_COORDS0+pass]);
}

void LLDrawPool::bindGLNormalPointer()
{
	mMemory.bindGLNormalPointer(getStride(DATA_NORMALS), mDataOffsets[DATA_NORMALS]);
}

void LLDrawPool::bindGLBinormalPointer(S32 index)
{
	mMemory.bindGLBinormalPointer(index, getStride(DATA_BINORMALS), mDataOffsets[DATA_BINORMALS]);
}

void LLDrawPool::bindGLColorPointer()
{
	mMemory.bindGLColorPointer(getStride(DATA_COLORS), mDataOffsets[DATA_COLORS]);
}

void LLDrawPool::bindGLVertexWeightPointer(S32 index)
{
	mWeights.bindGLVertexWeightPointer(index, 0, 0);
}

void LLDrawPool::bindGLVertexClothingWeightPointer(S32 index)
{
	mClothingWeights.bindGLVertexClothingWeightPointer(index, 0, 0);
}


U32* LLDrawPool::getIndices(S32 index)
{
	return &mIndices[index];
}

const LLVector3& LLDrawPool::getVertex(const S32 index)
{
	llassert(mDataMaskIL & DATA_VERTICES_MASK);
	llassert(index < mMemory.count());
	llassert(mMemory.getMem());
	return *(LLVector3*)(mMemory.getMem() + mDataOffsets[DATA_VERTICES] + index * mStride); 
}

const LLVector2& LLDrawPool::getTexCoord(const S32 index, const U32 pass)
{
	llassert(mDataMaskIL & (LLDrawPool::DATA_TEX_COORDS0_MASK << pass));
	llassert(index < mMemory.count());
	return *(LLVector2*)(mMemory.getMem() + mDataOffsets[DATA_TEX_COORDS0 + pass] + index * mStride); 
}

const LLVector3& LLDrawPool::getNormal(const S32 index)
{
	llassert(mDataMaskIL & DATA_NORMALS_MASK);
	llassert(index < mMemory.count());
	return *(LLVector3*)(mMemory.getMem() + mDataOffsets[DATA_NORMALS] + index * mStride); 
}

const LLVector3& LLDrawPool::getBinormal(const S32 index)
{
	llassert(mDataMaskIL & DATA_BINORMALS_MASK);
	llassert(index < mMemory.count());
	return *(LLVector3*)(mMemory.getMem() + mDataOffsets[DATA_BINORMALS] + index * mStride); 
}

const LLColor4U& LLDrawPool::getColor(const S32 index)
{
	llassert(mDataMaskIL & DATA_COLORS_MASK);
	llassert(index < mMemory.count());
	return *(LLColor4U*)(mMemory.getMem() + mDataOffsets[DATA_COLORS] + index * mStride); 
}

const F32& LLDrawPool::getVertexWeight(const S32 index)
{
	llassert(mDataMaskNIL & DATA_VERTEX_WEIGHTS_MASK);
	llassert(index < mWeights.count());
	llassert(mWeights.getMem());
	return mWeights[index];
}

const LLVector4& LLDrawPool::getClothingWeight(const S32 index)
{
	llassert(mDataMaskNIL & DATA_CLOTHING_WEIGHTS_MASK);
	llassert(index < mClothingWeights.count());
	llassert(mClothingWeights.getMem());
	return mClothingWeights[index];
}

//////////////////////////////////////////////////////////////////////////////

#define USE_FREE_LIST 0
#define DEBUG_FREELIST 0

struct tFreeListNode
{
	U32 count;
	S32 next;
};

#if DEBUG_FREELIST
static void check_list(U8 *pool, S32 stride, S32 head, S32 max)
{
	int count = 0;
	
	while (head >= 0)
	{
		tFreeListNode *node = (tFreeListNode *)(pool + head*stride);
		count++;
		if ((count > max) || ((node->count>>20) != 0xabc) || ((node->count&0xfffff) < 2))
			llerrs << "Bad Ind List" << llendl;
		head = node->next;
	}
}
#define CHECK_LIST(x) check_list##x
#else
#define CHECK_LIST(x)
#endif

// DEBUG!
void LLDrawPool::CheckIntegrity()
{
#if DEBUG_FREELIST
	int bucket;
	for (bucket=0; bucket<NUM_BUCKETS; bucket++)
	{
		CHECK_LIST(((U8 *)mMemory.getMem(), mStride, mFreeListGeomHead[bucket], mMemory.count() / mStride));
		CHECK_LIST(((U8 *)mIndices.getMem(), 4, mFreeListIndHead[bucket], mIndices.count()));
	}
#endif
}

int LLDrawPool::freeListBucket(U32 count)
{
	int bucket;

	// llassert(NUM_BUCKETS == 8)
	
	if (count & ~511) // >= 512
		bucket = 7;
	else if (count & 256) // 256-511
		bucket = 6;
	else if (count & 128)
		bucket = 5;
	else if (count & 64)
		bucket = 4;
	else if (count & 32)
		bucket = 3;
	else if (count & 16)
		bucket = 2;
	else if (count & 8)	 // 8-15
		bucket = 1;
	else 				 // 0-7
		bucket = 0;
	return bucket;
}

void remove_node(int nodeidx, int pidx, U8 *membase, int stride, int *head)
{
	LLDrawPool::FreeListNode *node = (LLDrawPool::FreeListNode *)(membase + nodeidx*stride);
	if (pidx >= 0)
	{
		LLDrawPool::FreeListNode *pnode = (LLDrawPool::FreeListNode *)(membase + pidx*stride);
		pnode->next = node->next;
	}
	else
	{
		*head = node->next;
	}
}

void LLDrawPool::freeListAddGeom(S32 index, U32 count)
{
#if USE_FREE_LIST
	int i;
	U8 *membase = (U8*)mMemory.getMem();
	// See if next block or previous block is free, if so combine them
	for (i=0; i<NUM_BUCKETS; i++)
	{
		int pidx = -1;
		int nodeidx = mFreeListGeomHead[i];
		while(nodeidx >= 0)
		{
			int change = 0;
			FreeListNode *node = (FreeListNode *)(membase + nodeidx*mStride);
			int nodecount = node->count & 0xffff;
			// Check for prev block
			if (nodeidx + nodecount == index)
			{
				remove_node(nodeidx, pidx, membase, mStride, &mFreeListGeomHead[i]);
				// Combine nodes
				index = nodeidx;
				count += nodecount;
				i = 0; // start over ; i = NUM_BUCKETS // done
				change = 1;
				//break;
			}
			// Check for next block
			if (nodeidx == index + count)
			{
				remove_node(nodeidx, pidx, membase, mStride, &mFreeListGeomHead[i]);
				// Combine nodes
				count += nodecount;
				i = 0; // start over ; i = NUM_BUCKETS // done
				change = 1;
				//break;
			}				
			if (change)
				break;
			pidx = nodeidx;
			nodeidx = node->next;
		}
	}
	// Add (extended) block to free list
	if (count >= 2) // need 2 words to store free list (theoreticly mStride could = 4)
	{
		CheckIntegrity();
		if ((index + count)*mStride >= mMemory.count())
		{
			mMemory.shrinkTo(index*mStride);
		}
		else
		{
			int bucket = freeListBucket(count);
			FreeListNode *node = (FreeListNode *)(membase + index*mStride);
			node->count = count | (0xabc<<20);
			node->next = mFreeListGeomHead[bucket];	
			mFreeListGeomHead[bucket] = index;
		}
		CheckIntegrity();
	}
#endif
}

void LLDrawPool::freeListAddInd(S32 index, U32 count)
{
#if USE_FREE_LIST
	int i;
	const U32 *membase = mIndices.getMem();
	// See if next block or previous block is free, if so combine them
	for (i=0; i<NUM_BUCKETS; i++)
	{
		int pidx = -1;
		int nodeidx = mFreeListIndHead[i];
		while(nodeidx >= 0)
		{
			int change = 0;
			FreeListNode *node = (FreeListNode *)(membase + nodeidx);
			int nodecount = node->count & 0xffff;
			// Check for prev block
			if (nodeidx + nodecount == index)
			{
				remove_node(nodeidx, pidx, (U8*)membase, 4, &mFreeListIndHead[i]);
				// Combine nodes
				index = nodeidx;
				count += nodecount;
				i = 0; // start over ; i = NUM_BUCKETS // done
				change = 1;
				//break;
			}
			// Check for next block
			if (nodeidx == index + count)
			{
				remove_node(nodeidx, pidx, (U8*)membase, 4, &mFreeListIndHead[i]);
				// Combine nodes
				count += nodecount;
				i = 0; // start over ; i = NUM_BUCKETS // done
				change = 1;
				//break;
			}
			if (change)
				break;
			pidx = nodeidx;
			nodeidx = node->next;
		}
	}
	// Add (extended) block to free list
	if (count >= 2) // need 2 words to store free list
	{
		CheckIntegrity();
		if (index + count >= mIndices.count())
		{
			mIndices.shrinkTo(index);
		}
		else
		{
			int bucket = freeListBucket(count);
			FreeListNode *node = (FreeListNode *)(membase + index);
			node->count = count | (0xabc<<20);
			node->next = mFreeListIndHead[bucket];
			mFreeListIndHead[bucket] = index;
		}
		CheckIntegrity();
	}
#endif
}

S32 LLDrawPool::freeListFindGeom(U32 count)
{
#if USE_FREE_LIST
	int i, nodeidx, pidx;
	int firstbucket = freeListBucket(count);
	U8 *membase = (U8*)mMemory.getMem();
	for (i=firstbucket; i<NUM_BUCKETS; i++)
	{
		pidx = -1;
		nodeidx = mFreeListGeomHead[i];
		CHECK_LIST(((U8 *)mMemory.getMem(), mStride, mFreeListGeomHead[i], mMemory.count() / mStride));
		while(nodeidx >= 0)
		{
			FreeListNode *node = (FreeListNode *)(membase + nodeidx*mStride);
			int nodecount = node->count & 0xffff;
			llassert((node->count>>20) == 0xabc);
			if (nodecount >= count)
			{
				remove_node(nodeidx, pidx, membase, mStride, &mFreeListGeomHead[i]);
#if 1
				if (nodecount > count)
				{
					int leftover = nodecount - count;
					freeListAddGeom(nodeidx + count, leftover);
				}
#endif
				return nodeidx;
			}
			pidx = nodeidx;
			nodeidx = node->next;
		}
	}
#endif // USE_FREE_LIST
	return -1;
}

S32 LLDrawPool::freeListFindInd(U32 count)
{
#if USE_FREE_LIST
	int i, nodeidx, pidx;
	int firstbucket = freeListBucket(count);
	U32 *membase = (U32 *)mIndices.getMem();
	for (i=firstbucket; i<NUM_BUCKETS; i++)
	{
		pidx = -1;
		nodeidx = mFreeListIndHead[i];
		CHECK_LIST(((U8 *)mIndices.getMem(), 4, mFreeListIndHead[i], mIndices.count()));
		while(nodeidx >= 0)
		{
			FreeListNode *node = (FreeListNode *)(membase + nodeidx);
			int nodecount = node->count & 0xffff;
			llassert((node->count>>20) == 0xabc);
			if (nodecount >= count)
			{
				remove_node(nodeidx, pidx, (U8*)membase, 4, &mFreeListIndHead[i]);
#if 1
				if (nodecount > count)
				{
					int leftover = nodecount - count;
					freeListAddInd(nodeidx + count, leftover);
				}
#endif
				return nodeidx;
			}
			pidx = nodeidx;
			nodeidx = node->next;
		}
	}
#endif // USE_FREE_LIST
	return -1;
}

//////////////////////////////////////////////////////////////////////////////

S32 LLDrawPool::reserveGeom(const U32 geom_count)
{
	LLFastTimer t(LLFastTimer::FTM_GEO_RESERVE);
	
	S32 index;
	index = freeListFindGeom(geom_count);
	if (index < 0)
	{
		index = mMemory.count() / mStride;
		if (!geom_count)
		{
			llwarns << "Attempting to reserve zero bytes!" << llendl;
			return index;
		}

		S32 bytes = geom_count * mStride;

		if ((index + (S32)geom_count) > (S32)mMaxVertices)
		{
			//
			// Various drivers have issues with the number of indices being greater than a certain number.
			// if you're using AGP.  Disable AGP if we've got more vertices than in the pool.
			//
#ifdef DEBUG_AGP
			llinfos << "setUseAGP false because of large vertex count in reserveGeom" << llendl;
#endif
			setUseAGP(FALSE);
		}

		mMemory.reserve_block(bytes);
		if (mDataMaskNIL & DATA_VERTEX_WEIGHTS_MASK)
		{
			mWeights.reserve_block(geom_count);
		}
		if (mDataMaskNIL & DATA_CLOTHING_WEIGHTS_MASK)
		{
			mClothingWeights.reserve_block(geom_count);
		}
	}
	CHECK_LIST(((U8 *)mMemory.getMem(), mStride, mFreeListGeomHead[0], mMemory.count() / mStride));
	return index;
}

S32 LLDrawPool::reserveInd(U32 indCount)
{
	S32 index;
	index = freeListFindInd(indCount);
	if (index < 0)
	{
		index = mIndices.count();

		if (indCount)
		{
			mIndices.reserve_block(indCount);
		}
	}
	for (U32 i=0;i<indCount;i++)
	{
		mIndices[index+i]=0;
	}
	CHECK_LIST(((U8 *)mIndices.getMem(), 4, mFreeListIndHead[0], mIndices.count()));
	return index;
}

S32 LLDrawPool::unReserveGeom(const S32 index, const U32 count)
{
	if (index < 0 || count == 0)
		return -1;

	freeListAddGeom(index, count);
	
#if 0
	int i;
	S32 bytes,words;
	U32 *memp;
	// Fill mem with bad data (for testing only)
	bytes = count * mStride;
	bytes -= sizeof(FreeListNode);
	memp = (U32*)(mMemory.getMem() + index * mStride);	
	memp += sizeof(FreeListNode)>>2;
	words = bytes >> 2;
	for (i=0; i<words; i++)
		*memp++ = 0xffffffff;

	words = count; //  (sizeof each array is a word)
	if (mDataMaskNIL & DATA_VERTEX_WEIGHTS_MASK)
	{
		memp = (U32*)(&mWeights[index]);
		for (i=0; i<words; i++)
			*memp++ = 0xffffffff;
	}
	if (mDataMaskNIL & DATA_CLOTHING_WEIGHTS_MASK)
	{
		memp = (U32*)(&mClothingWeights[index]);
		for (i=0; i<count; i++)
			*memp++ = 0xffffffff;
	}
#endif
	return -1;
}

S32 LLDrawPool::unReserveInd(const S32 index, const U32 count)
{
	if (index < 0 || count == 0)
		return -1;

	freeListAddInd(index, count);
	
#if 0
	int i;
	U32 *memp = &mIndices[index];
	for (i=0; i<count; i++)
		*memp++ = 0xffffffff;
#endif
	return -1;
}

//////////////////////////////////////////////////////////////////////////////

const U32 LLDrawPool::getIndexCount() const
{
	return mIndices.count();
}

const U32 LLDrawPool::getVertexCount() const
{
	return mMemory.count() / mStride;
}

const U32 LLDrawPool::getTexCoordCount(U32 pass) const
{
	return mMemory.count() / mStride;
}


const U32 LLDrawPool::getNormalCount() const
{
	return mMemory.count() / mStride;
}


const U32 LLDrawPool::getBinormalCount() const
{
	return mMemory.count() / mStride;
}

const U32 LLDrawPool::getColorCount() const
{
	return mMemory.count() / mStride;
}

const U32 LLDrawPool::getVertexWeightCount() const
{
	return mWeights.count();
}

// virtual
BOOL LLDrawPool::addFace(LLFace *facep)
{
	addFaceReference(facep);
	return TRUE;
}

// virtual
BOOL LLDrawPool::removeFace(LLFace *facep)
{
	removeFaceReference(facep);

	vector_replace_with_last(mDrawFace, facep);

	facep->unReserve();
	
	return TRUE;
}

// Not absolutely sure if we should be resetting all of the chained pools as well - djs
void LLDrawPool::resetDrawOrders()
{
	mDrawFace.resize(0);
}

void LLDrawPool::resetIndices(S32 indices_count)
{
	mIndices.reset(indices_count);
	for (S32 i=0; i<NUM_BUCKETS; i++)
		mFreeListIndHead[i] = -1;
}

void LLDrawPool::resetVertexData(S32 reserve_count)
{
	mMemory.reset(reserve_count*mStride);
		
	for (S32 i=0; i<NUM_BUCKETS; i++)
	{
		mFreeListGeomHead[i] = -1;
	}
	if (mDataMaskNIL & DATA_VERTEX_WEIGHTS_MASK)
	{
		mWeights.reset(reserve_count);
	}

	if (mDataMaskNIL & DATA_CLOTHING_WEIGHTS_MASK)
	{
		mClothingWeights.reset(reserve_count);
	}
}

void LLDrawPool::resetAll()
{
	resetDrawOrders();
	resetVertexData(0);
	mGeneration++;

}

S32 LLDrawPool::rebuild()
{
	mRebuildTime++;

	BOOL needs_rebuild = FALSE;
	S32 rebuild_cost = 0;
	
	if (mUseAGP)
	{
		if (getVertexCount() > 0.75f*DEFAULT_MAX_VERTICES)
		{
			if (mRebuildTime > 8)
			{
				needs_rebuild = TRUE;
			}
#ifdef DEBUG_AGP
			llwarns << "More than " << DEFAULT_MAX_VERTICES << " in pool type " << (S32)mType << " at rebuild!" << llendl;
#endif
		}
	}
	
	// rebuild de-allocates 'stale' objects, so we still need to do a rebuild periodically
	if (mRebuildFreq > 0 && mRebuildTime >= mRebuildFreq)
	{
		needs_rebuild = TRUE;
	}

	if (needs_rebuild) 
	{
		mGeneration++;

		if (mReferences.empty())
		{
			resetIndices(0);
			resetVertexData(0);
		}
		else
		{
			for (std::vector<LLFace*>::iterator iter = mReferences.begin();
				 iter != mReferences.end(); iter++)
			{
				LLFace *facep = *iter;
				if (facep->hasGeometry() && !facep->isState(LLFace::BACKLIST | LLFace::SHARED_GEOM))
				{
					facep->backup();
				}
			}
			S32 tot_verts = 0;
			S32 tot_indices = 0;
			for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
				 iter != mDrawFace.end(); iter++)
			{
				LLFace *facep = *iter;
				if (facep->isState(LLFace::BACKLIST))
				{
					tot_verts += facep->getGeomCount();
					tot_indices += facep->getIndicesCount();
				}
			}
			for (std::vector<LLFace*>::iterator iter = mMoveFace.begin();
				 iter != mMoveFace.end(); iter++)
			{
				LLFace *facep = *iter;
				if (facep->isState(LLFace::BACKLIST))
				{
					tot_verts += facep->getGeomCount();
					tot_indices += facep->getIndicesCount();
				}
			}
			
			resetIndices(tot_indices);
			flushAGP();
			resetVertexData(tot_verts);

			for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
				 iter != mDrawFace.end(); iter++)
			{
				LLFace *facep = *iter;
				llassert(facep->getPool() == this);
				facep->restore();
			}
		}
		mRebuildTime = 0;
		setDirty();
	}

	if (!mMoveFace.empty())
	{
		for (std::vector<LLFace*>::iterator iter = mMoveFace.begin();
			 iter != mMoveFace.end(); iter++)
		{
			LLFace *facep = *iter;
			facep->restore();
			enqueue(facep);
		}
		setDirty();
		mMoveFace.reset();
		rebuild_cost++;
	}
	return rebuild_cost;
}

LLViewerImage *LLDrawPool::getTexture()
{
	return NULL;
}

LLViewerImage *LLDrawPool::getDebugTexture()
{
	return NULL;
}

void LLDrawPool::removeFaceReference(LLFace *facep)
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

void LLDrawPool::addFaceReference(LLFace *facep)
{
	if (-1 == facep->getReferenceIndex())
	{
		facep->setReferenceIndex(mReferences.size());
		mReferences.push_back(facep);
	}
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

BOOL LLDrawPool::verify() const
{
	BOOL ok = TRUE;
	// Verify all indices in the pool are in the right range
	const U32 *indicesp = getRawIndices();
	for (U32 i = 0; i < getIndexCount(); i++)
	{
		if (indicesp[i] > getVertexCount())
		{
			ok = FALSE;
			llinfos << "Bad index in tree pool!" << llendl;
		}
	}

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

void LLDrawPool::printDebugInfo() const
{
	llinfos << "Pool " << this << " Type: " << getType() << llendl;
	llinfos << "--------------------" << llendl;
	llinfos << "Vertex count: "  << getVertexCount() << llendl;
	llinfos << "Normal count: "  << getNormalCount() << llendl; 
	llinfos << "Indices count: " << getIndexCount() << llendl;
	llinfos << llendl;
}


S32 LLDrawPool::getMemUsage(const BOOL print)
{
	S32 mem_usage = 0;

	mem_usage += sizeof(this);

	// Usage beyond the pipeline allocated data (color and mMemory)
	mem_usage += mIndices.getMax() * sizeof(U32);
	mem_usage += mDrawFace.capacity() * sizeof(LLFace *);
	mem_usage += mMoveFace.capacity() * sizeof(LLFace *);
	mem_usage += mReferences.capacity() * sizeof(LLFace *);

	mem_usage += mMemory.getSysMemUsage();
	mem_usage += mWeights.getSysMemUsage();
	mem_usage += mClothingWeights.getSysMemUsage();

	return mem_usage;
}

LLColor3 LLDrawPool::getDebugColor() const
{
	return LLColor3(0.f, 0.f, 0.f);
}

void LLDrawPool::setDirty()
{ 
	mMemory.setDirty(); 
	mWeights.setDirty();
	mClothingWeights.setDirty();
}

BOOL LLDrawPool::LLOverrideFaceColor::sOverrideFaceColor = FALSE;

void LLDrawPool::LLOverrideFaceColor::setColor(const LLColor4& color)
{
	if (mPool->mVertexShaderLevel > 0 && mPool->getMaterialAttribIndex() > 0)
	{
		glVertexAttrib4fvARB(mPool->getMaterialAttribIndex(), color.mV);
	}
	else
	{
		glColor4fv(color.mV);
	}
}

void LLDrawPool::LLOverrideFaceColor::setColor(const LLColor4U& color)
{
	if (mPool->mVertexShaderLevel > 0 && mPool->getMaterialAttribIndex() > 0)
	{
		glVertexAttrib4ubvARB(mPool->getMaterialAttribIndex(), color.mV);
	}
	else
	{
		glColor4ubv(color.mV);
	}
}

void LLDrawPool::LLOverrideFaceColor::setColor(F32 r, F32 g, F32 b, F32 a)
{
	if (mPool->mVertexShaderLevel > 0 && mPool->getMaterialAttribIndex() > 0)
	{
		glVertexAttrib4fARB(mPool->getMaterialAttribIndex(), r,g,b,a);
	}
	else
	{
		glColor4f(r,g,b,a);
	}
}

// virtual
void LLDrawPool::enableShade()
{ }

// virtual
void LLDrawPool::disableShade()
{ }

// virtual
void LLDrawPool::setShade(F32 shade)
{ }
