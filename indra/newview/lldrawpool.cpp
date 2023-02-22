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
#include "lldrawpoolmaterials.h"
#include "lldrawpoolpbropaque.h"
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
#include "llglslshader.h"
#include "llglcommonfunc.h"
#include "llvoavatar.h"
#include "llviewershadermgr.h"


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
	case POOL_ALPHA_MASK:
		poolp = new LLDrawPoolAlphaMask();
		break;
	case POOL_FULLBRIGHT_ALPHA_MASK:
		poolp = new LLDrawPoolFullbrightAlphaMask();
		break;
	case POOL_FULLBRIGHT:
		poolp = new LLDrawPoolFullbright();
		break;
	case POOL_GLOW:
		poolp = new LLDrawPoolGlow();
		break;
	case POOL_ALPHA_PRE_WATER:
		poolp = new LLDrawPoolAlpha(LLDrawPool::POOL_ALPHA_PRE_WATER);
		break;
    case POOL_ALPHA_POST_WATER:
        poolp = new LLDrawPoolAlpha(LLDrawPool::POOL_ALPHA_POST_WATER);
        break;
	case POOL_AVATAR:
	case POOL_CONTROL_AV:
		poolp = new LLDrawPoolAvatar(type);
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
	case POOL_BUMP:
		poolp = new LLDrawPoolBump();
		break;
	case POOL_MATERIALS:
		poolp = new LLDrawPoolMaterials();
		break;
	case POOL_WL_SKY:
		poolp = new LLDrawPoolWLSky();
		break;
	case POOL_GLTF_PBR:
		poolp = new LLDrawPoolGLTFPBR();
        break;
    case POOL_GLTF_PBR_ALPHA_MASK:
        poolp = new LLDrawPoolGLTFPBR(LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK);
		break;
	default:
		LL_ERRS() << "Unknown draw pool type!" << LL_ENDL;
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
	mShaderLevel = 0;
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
		LL_INFOS() << mReferences.size() << " references left on deletion of draw pool!" << LL_ENDL;
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
			LL_INFOS() << "Face in wrong pool!" << LL_ENDL;
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
	LL_INFOS() << "Pool " << this << " Type: " << getType() << LL_ENDL;
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

void LLRenderPass::renderGroup(LLSpatialGroup* group, U32 type, bool texture)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
	LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[type];
	
	for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)	
	{
		LLDrawInfo *pparams = *k;
		if (pparams) 
        {
			pushBatch(*pparams, texture);
		}
	}
}

void LLRenderPass::renderRiggedGroup(LLSpatialGroup* group, U32 type, bool texture)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[type];
    LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    
    for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
    {
        LLDrawInfo* pparams = *k;
        if (pparams) 
        {
            if (lastAvatar != pparams->mAvatar || lastMeshId != pparams->mSkinInfo->mHash)
            {
                uploadMatrixPalette(*pparams);
                lastAvatar = pparams->mAvatar;
                lastMeshId = pparams->mSkinInfo->mHash;
            }

            pushBatch(*pparams, texture);
        }
    }
}

void setup_texture_matrix(LLDrawInfo& params)
{
    if (params.mTextureMatrix)
    { //special case implementation of texture animation here because of special handling of textures for PBR batches
        gGL.getTexUnit(0)->activate();
        gGL.matrixMode(LLRender::MM_TEXTURE);
        gGL.loadMatrix((GLfloat*)params.mTextureMatrix->mMatrix);
        gPipeline.mTextureMatrixOps++;
    }
}

void teardown_texture_matrix(LLDrawInfo& params)
{
    if (params.mTextureMatrix)
    {
        gGL.matrixMode(LLRender::MM_TEXTURE0);
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
    }
}

void LLRenderPass::pushGLTFBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("pushGLTFBatch");
        LLDrawInfo& params = **i;
        LLCullResult::increment_iterator(i, end);

        auto& mat = params.mGLTFMaterial;

        mat->bind(params.mTexture);

        LLGLDisable cull_face(mat->mDoubleSided ? GL_CULL_FACE : 0);

        setup_texture_matrix(params);
        
        applyModelMatrix(params);

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);

        teardown_texture_matrix(params);
    }
}

void LLRenderPass::pushRiggedGLTFBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;

    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("pushRiggedGLTFBatch");
        LLDrawInfo& params = **i;
        LLCullResult::increment_iterator(i, end);

        auto& mat = params.mGLTFMaterial;

        mat->bind(params.mTexture);

        LLGLDisable cull_face(mat->mDoubleSided ? GL_CULL_FACE : 0);

        setup_texture_matrix(params);

        applyModelMatrix(params);

        if (params.mAvatar.notNull() && (lastAvatar != params.mAvatar || lastMeshId != params.mSkinInfo->mHash))
        {
            uploadMatrixPalette(params);
            lastAvatar = params.mAvatar;
            lastMeshId = params.mSkinInfo->mHash;
        }

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);

        teardown_texture_matrix(params);
    }
}

void LLRenderPass::pushBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;
        LLCullResult::increment_iterator(i, end);

		pushBatch(*pparams, texture, batch_textures);
	}
}

void LLRenderPass::pushRiggedBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;
        LLCullResult::increment_iterator(i, end);

        if (pparams->mAvatar.notNull() && (lastAvatar != pparams->mAvatar || lastMeshId != pparams->mSkinInfo->mHash))
        {
            uploadMatrixPalette(*pparams);
            lastAvatar = pparams->mAvatar;
            lastMeshId = pparams->mSkinInfo->mHash;
        }

        pushBatch(*pparams, texture, batch_textures);
    }
}

void LLRenderPass::pushMaskBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
	for (LLCullResult::drawinfo_iterator i = begin; i != end; )
	{
        LLDrawInfo* pparams = *i;
        LLCullResult::increment_iterator(i, end);
		LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(pparams->mAlphaMaskCutoff);
		pushBatch(*pparams, texture, batch_textures);
	}
}

void LLRenderPass::pushRiggedMaskBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;

        LLCullResult::increment_iterator(i, end);

        if (LLGLSLShader::sCurBoundShaderPtr)
        {
            LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(pparams->mAlphaMaskCutoff);
        }
        else
        {
            gGL.flush();
        }

        if (lastAvatar != pparams->mAvatar || lastMeshId != pparams->mSkinInfo->mHash)
        {
            uploadMatrixPalette(*pparams);
            lastAvatar = pparams->mAvatar;
            lastMeshId = pparams->mSkinInfo->mHash;
        }

        pushBatch(*pparams, texture, batch_textures);
    }
}

void LLRenderPass::applyModelMatrix(const LLDrawInfo& params)
{
	if (params.mModelMatrix != gGLLastMatrix)
	{
		gGLLastMatrix = params.mModelMatrix;
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.loadMatrix(gGLModelView);
		if (params.mModelMatrix)
		{
			gGL.multMatrix((GLfloat*) params.mModelMatrix->mMatrix);
		}
		gPipeline.mMatrixOpCount++;
	}
}

void LLRenderPass::pushBatch(LLDrawInfo& params, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    if (!params.mCount)
    {
        return;
    }

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
					gGL.getTexUnit(i)->bindFast(params.mTextureList[i]);
				}
			}
		}
		else
		{ //not batching textures or batch has only 1 texture -- might need a texture matrix
			if (params.mTexture.notNull())
			{
				gGL.getTexUnit(0)->bindFast(params.mTexture);
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
				gGL.getTexUnit(0)->unbindFast(LLTexUnit::TT_TEXTURE);
			}
		}
	}
	
    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);

	if (tex_setup)
	{
        gGL.matrixMode(LLRender::MM_TEXTURE0);
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
}

// static
bool LLRenderPass::uploadMatrixPalette(LLDrawInfo& params)
{
    // upload matrix palette to shader
    return uploadMatrixPalette(params.mAvatar, params.mSkinInfo);
}

//static
bool LLRenderPass::uploadMatrixPalette(LLVOAvatar* avatar, LLMeshSkinInfo* skinInfo)
{
    if (!avatar)
    {
        return false;
    }
    const LLVOAvatar::MatrixPaletteCache& mpc = avatar->updateSkinInfoMatrixPalette(skinInfo);
    U32 count = mpc.mMatrixPalette.size();

    if (count == 0)
    {
        //skin info not loaded yet, don't render
        return false;
    }

    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX,
        count,
        FALSE,
        (GLfloat*)&(mpc.mGLMp[0]));

    return true;
}

