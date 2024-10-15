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

#define USE_VAO 0

static U32 gl_indices_type[] = { GL_UNSIGNED_SHORT, GL_UNSIGNED_INT };
static U32 gl_indices_size[] = { sizeof(U16), sizeof(U32) };

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
S32  LLDrawPool::getNumPasses()
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
bool LLFacePool::addFace(LLFace *facep)
{
    addFaceReference(facep);
    return true;
}

// virtual
bool LLFacePool::removeFace(LLFace *facep)
{
    removeFaceReference(facep);

    vector_replace_with_last(mDrawFace, facep);

    return true;
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
        facep->setReferenceIndex(static_cast<S32>(mReferences.size()));
        mReferences.push_back(facep);
    }
}

void LLFacePool::pushFaceGeometry()
{
    for (LLFace* const& face : mDrawFace)
    {
        face->renderIndexed();
    }
}

bool LLFacePool::verify() const
{
    bool ok = true;

    for (std::vector<LLFace*>::const_iterator iter = mDrawFace.begin();
         iter != mDrawFace.end(); iter++)
    {
        const LLFace* facep = *iter;
        if (facep->getPool() != this)
        {
            LL_INFOS() << "Face in wrong pool!" << LL_ENDL;
            facep->printDebugInfo();
            ok = false;
        }
        else if (!facep->verify())
        {
            ok = false;
        }
    }

    return ok;
}

void LLFacePool::printDebugInfo() const
{
    LL_INFOS() << "Pool " << this << " Type: " << getType() << LL_ENDL;
}

bool LLFacePool::LLOverrideFaceColor::sOverrideFaceColor = false;

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
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
    {
        LLDrawInfo* pparams = *k;
        if (pparams)
        {
            if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
            {
                pushBatch(*pparams, texture);
            }
        }
    }
}

void LLRenderPass::pushBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    if (texture)
    {
        auto* begin = gPipeline.beginRenderMap(type);
        auto* end = gPipeline.endRenderMap(type);
        for (LLCullResult::drawinfo_iterator i = begin; i != end; )
        {
            LLDrawInfo* pparams = *i;
            LLCullResult::increment_iterator(i, end);

            pushBatch(*pparams, texture, batch_textures);
        }
    }
    else
    {
        pushUntexturedBatches(type);
    }
}

void LLRenderPass::pushUntexturedBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;
        LLCullResult::increment_iterator(i, end);

        pushUntexturedBatch(*pparams);
    }
}

void LLRenderPass::pushRiggedBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    if (texture)
    {
        const LLVOAvatar* lastAvatar = nullptr;
        U64 lastMeshId = 0;
        bool skipLastSkin = false;
        auto* begin = gPipeline.beginRenderMap(type);
        auto* end = gPipeline.endRenderMap(type);
        for (LLCullResult::drawinfo_iterator i = begin; i != end; )
        {
            LLDrawInfo* pparams = *i;
            LLCullResult::increment_iterator(i, end);

            if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
            {
                pushBatch(*pparams, texture, batch_textures);
            }
        }
    }
    else
    {
        pushUntexturedRiggedBatches(type);
    }
}

void LLRenderPass::pushUntexturedRiggedBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;
        LLCullResult::increment_iterator(i, end);

        if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            pushUntexturedBatch(*pparams);
        }
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
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;

        LLCullResult::increment_iterator(i, end);

        llassert(pparams);

        LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(pparams->mAlphaMaskCutoff);

        if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            pushBatch(*pparams, texture, batch_textures);
        }
    }
}

void LLRenderPass::applyModelMatrix(const LLDrawInfo& params)
{
    applyModelMatrix(params.mModelMatrix);
}

void LLRenderPass::applyModelMatrix(const LLMatrix4* model_matrix)
{
    if (model_matrix != gGLLastMatrix)
    {
        gGLLastMatrix = model_matrix;
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.loadMatrix(gGLModelView);
        if (model_matrix)
        {
            gGL.multMatrix((GLfloat*) model_matrix->mMatrix);
        }
        gPipeline.mMatrixOpCount++;
    }
}

void LLRenderPass::pushBatch(LLDrawInfo& params, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    llassert(texture);

    if (!params.mCount)
    {
        return;
    }

    applyModelMatrix(params);

    bool tex_setup = false;

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

void LLRenderPass::pushUntexturedBatch(LLDrawInfo& params)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    if (!params.mCount)
    {
        return;
    }

    applyModelMatrix(params);

    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (!avatar)
    {
        return false;
    }
    const LLVOAvatar::MatrixPaletteCache& mpc = avatar->updateSkinInfoMatrixPalette(skinInfo);
    U32 count = static_cast<U32>(mpc.mMatrixPalette.size());

    if (count == 0)
    {
        //skin info not loaded yet, don't render
        return false;
    }

    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX,
        count,
        false,
        (GLfloat*)&(mpc.mGLMp[0]));

    return true;
}

// Returns true if rendering should proceed
//static
bool LLRenderPass::uploadMatrixPalette(LLVOAvatar* avatar, LLMeshSkinInfo* skinInfo, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    llassert(skinInfo);
    llassert(LLGLSLShader::sCurBoundShaderPtr);

    if (!avatar)
    {
        return false;
    }

    if (avatar == lastAvatar && skinInfo->mHash == lastMeshId)
    {
        return !skipLastSkin;
    }

    const LLVOAvatar::MatrixPaletteCache& mpc = avatar->updateSkinInfoMatrixPalette(skinInfo);
    U32 count = static_cast<U32>(mpc.mMatrixPalette.size());
    // skipLastSkin -> skin info not loaded yet, don't render
    skipLastSkin = !bool(count);
    lastAvatar = avatar;
    lastMeshId = skinInfo->mHash;

    if (!skipLastSkin)
    {
        LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX,
            count,
            false,
            (GLfloat*)&(mpc.mGLMp[0]));
    }

    return !skipLastSkin;
}

// Returns true if rendering should proceed
//static
bool LLRenderPass::uploadMatrixPalette(LLVOAvatar* avatar, LLMeshSkinInfo* skinInfo, const LLVOAvatar*& lastAvatar, U64& lastMeshId, const LLGLSLShader*& lastAvatarShader, bool& skipLastSkin)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    llassert(skinInfo);
    llassert(LLGLSLShader::sCurBoundShaderPtr);

    if (!avatar)
    {
        return false;
    }

    if (avatar == lastAvatar && skinInfo->mHash == lastMeshId && lastAvatarShader == LLGLSLShader::sCurBoundShaderPtr)
    {
        return !skipLastSkin;
    }

    const LLVOAvatar::MatrixPaletteCache& mpc = avatar->updateSkinInfoMatrixPalette(skinInfo);
    U32 count = static_cast<U32>(mpc.mMatrixPalette.size());
    // skipLastSkin -> skin info not loaded yet, don't render
    skipLastSkin = !bool(count);
    lastAvatar = avatar;
    lastMeshId = skinInfo->mHash;
    lastAvatarShader = LLGLSLShader::sCurBoundShaderPtr;

    if (!skipLastSkin)
    {
        LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX,
            count,
            false,
            (GLfloat*)&(mpc.mGLMp[0]));
    }

    return !skipLastSkin;
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

static glm::mat4 last_model_matrix;
static U32 transform_ubo = 0;
static size_t last_mat = 0;

static S32 base_tu = -1;
static S32 norm_tu = -1;
static S32 orm_tu = -1;
static S32 emis_tu = -1;
static S32 diffuse_tu = -1;
static S32 specular_tu = -1;
static S32 cur_base_tex = 0;
static S32 cur_norm_tex = 0;
static S32 cur_orm_tex = 0;
static S32 cur_emis_tex = 0;
static S32 cur_diffuse_tex = 0;
static S32 cur_specular_tex = 0;
static S32 base_instance_index = 0;

extern LLCullResult* sCull;

static void pre_push_gltf_batches()
{
    STOP_GLERROR;
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.loadMatrix(gGLModelView);
    gGL.syncMatrices();
    transform_ubo = 0;
    last_mat = 0;

    base_tu = LLGLSLShader::sCurBoundShaderPtr->getTextureChannel(LLShaderMgr::DIFFUSE_MAP);
    norm_tu = LLGLSLShader::sCurBoundShaderPtr->getTextureChannel(LLShaderMgr::BUMP_MAP);
    orm_tu = LLGLSLShader::sCurBoundShaderPtr->getTextureChannel(LLShaderMgr::SPECULAR_MAP);
    emis_tu = LLGLSLShader::sCurBoundShaderPtr->getTextureChannel(LLShaderMgr::EMISSIVE_MAP);

    base_instance_index = LLGLSLShader::sCurBoundShaderPtr->getUniformLocation(LLShaderMgr::GLTF_BASE_INSTANCE);

    cur_emis_tex = cur_orm_tex = cur_norm_tex = cur_base_tex = 0;

    S32 tex[] = { base_tu, norm_tu, orm_tu, emis_tu };

    for (S32 tu : tex)
    {
        if (tu != -1)
        {
            gGL.getTexUnit(tu)->bindManual(LLTexUnit::TT_TEXTURE, 0, true);
        }
    }
    STOP_GLERROR;
}

void LLRenderPass::pushGLTFBatches(const std::vector<LLGLTFDrawInfo>& draw_info, bool planar, bool tex_anim)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    pre_push_gltf_batches();

    for (auto& params : draw_info)
    {
        pushGLTFBatch(params, planar, tex_anim);
    }

    LLVertexBuffer::unbind();
}

void LLRenderPass::pushShadowGLTFBatches(const std::vector<LLGLTFDrawInfo>& draw_info)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    pre_push_gltf_batches();

    for (auto& params : draw_info)
    {
        pushShadowGLTFBatch(params);
    }

    LLVertexBuffer::unbind();
}

// static
void LLRenderPass::pushGLTFBatch(const LLGLTFDrawInfo& params, bool planar, bool tex_anim)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LL_PROFILE_ZONE_NUM(params.mInstanceCount);
    llassert(params.mTransformUBO != 0);
    STOP_GLERROR;
    if (params.mTransformUBO != transform_ubo)
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_NODES, params.mTransformUBO);
        glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_NODE_INSTANCE_MAP, params.mInstanceMapUBO);
        glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_MATERIALS, params.mMaterialUBO);
        if (tex_anim)
        {
            glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_TEXTURE_TRANSFORM, params.mTextureTransformUBO);
        }
        transform_ubo = params.mTransformUBO;
    }

    if (!last_mat || params.mMaterialID != last_mat)
    {
        last_mat = params.mMaterialID;
        if (base_tu != -1 && cur_base_tex != params.mBaseColorMap)
        {
            glActiveTexture(GL_TEXTURE0 + base_tu);
            glBindTexture(GL_TEXTURE_2D, params.mBaseColorMap);
            cur_base_tex = params.mBaseColorMap;
        }

        if (!LLPipeline::sShadowRender)
        {
            if (norm_tu != -1 && cur_norm_tex != params.mNormalMap)
            {
                glActiveTexture(GL_TEXTURE0 + norm_tu);
                glBindTexture(GL_TEXTURE_2D, params.mNormalMap);
                cur_norm_tex = params.mNormalMap;
            }

            if (orm_tu != -1 && cur_orm_tex != params.mMetallicRoughnessMap)
            {
                glActiveTexture(GL_TEXTURE0 + orm_tu);
                glBindTexture(GL_TEXTURE_2D, params.mMetallicRoughnessMap);
                cur_orm_tex = params.mMetallicRoughnessMap;
            }

            if (emis_tu != -1 && cur_emis_tex != params.mEmissiveMap)
            {
                glActiveTexture(GL_TEXTURE0 + emis_tu);
                glBindTexture(GL_TEXTURE_2D, params.mEmissiveMap);
                cur_emis_tex = params.mEmissiveMap;
            }
        }
    }

    glUniform1i(base_instance_index, params.mBaseInstance);

    STOP_GLERROR;
#if USE_VAO
    LLVertexBuffer::bindVAO(params.mVAO);
#else
    LLVertexBuffer::bindVBO(params.mVBO, params.mIBO, params.mVBOVertexCount);
#endif
    glDrawElementsInstanced(GL_TRIANGLES, params.mElementCount,
        gl_indices_type[params.mIndicesSize], (GLvoid*)(size_t)(params.mElementOffset * gl_indices_size[params.mIndicesSize]),
        params.mInstanceCount);
    STOP_GLERROR;
}

// static
void LLRenderPass::pushShadowGLTFBatch(const LLGLTFDrawInfo& params)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LL_PROFILE_ZONE_NUM(params.mInstanceCount);
    llassert(params.mTransformUBO != 0);

    if (params.mTransformUBO != transform_ubo)
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_NODES, params.mTransformUBO);
        glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_NODE_INSTANCE_MAP, params.mInstanceMapUBO);
        // NOTE: don't bind the material UBO here, it's not used in shadow pass
        transform_ubo = params.mTransformUBO;
    }

    glUniform1i(base_instance_index, params.mBaseInstance);

#if USE_VAO
    LLVertexBuffer::bindVAO(params.mVAO);
#else
    LLVertexBuffer::bindVBO(params.mVBO, params.mIBO, params.mVBOVertexCount);
#endif
    glDrawElementsInstanced(GL_TRIANGLES, params.mElementCount,
        gl_indices_type[params.mIndicesSize], (GLvoid*)(size_t)(params.mElementOffset * gl_indices_size[params.mIndicesSize]),
        params.mInstanceCount);
}

void LLRenderPass::pushRiggedGLTFBatches(const std::vector<LLSkinnedGLTFDrawInfo>& draw_info, bool planar, bool tex_anim)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    pre_push_gltf_batches();

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (auto& params : draw_info)
    {
        pushRiggedGLTFBatch(params, lastAvatar, lastMeshId, skipLastSkin, planar, tex_anim);
    }

    LLVertexBuffer::unbind();
}

void LLRenderPass::pushRiggedShadowGLTFBatches(const std::vector<LLSkinnedGLTFDrawInfo>& draw_info)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    pre_push_gltf_batches();

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (auto& params : draw_info)
    {
        pushRiggedShadowGLTFBatch(params, lastAvatar, lastMeshId, skipLastSkin);
    }

    LLVertexBuffer::unbind();
}

// static
void LLRenderPass::pushRiggedGLTFBatch(const LLSkinnedGLTFDrawInfo& params, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin, bool planar, bool tex_anim)
{
    if (uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
    {
        pushGLTFBatch(params, planar, tex_anim);
    }
}

// static
void LLRenderPass::pushRiggedShadowGLTFBatch(const LLSkinnedGLTFDrawInfo& params, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin)
{
    if (uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
    {
        pushShadowGLTFBatch(params);
    }
}


static void pre_push_bp_batches()
{
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.loadMatrix(gGLModelView);
    gGL.syncMatrices();
    transform_ubo = 0;
    last_mat = 0;

    diffuse_tu = LLGLSLShader::sCurBoundShaderPtr->getTextureChannel(LLShaderMgr::DIFFUSE_MAP);
    norm_tu = LLGLSLShader::sCurBoundShaderPtr->getTextureChannel(LLShaderMgr::BUMP_MAP);
    specular_tu = LLGLSLShader::sCurBoundShaderPtr->getTextureChannel(LLShaderMgr::SPECULAR_MAP);

    base_instance_index = LLGLSLShader::sCurBoundShaderPtr->getUniformLocation(LLShaderMgr::GLTF_BASE_INSTANCE);

     cur_specular_tex = cur_norm_tex = cur_diffuse_tex = 0;

    S32 tex[] = { diffuse_tu, norm_tu, specular_tu };

    for (S32 tu : tex)
    {
        if (tu != -1)
        {
            gGL.getTexUnit(tu)->bindManual(LLTexUnit::TT_TEXTURE, 0, true);
        }
    }
}

void LLRenderPass::pushBPBatches(const std::vector<LLGLTFDrawInfo>& draw_info, bool planar, bool tex_anim)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    pre_push_bp_batches();

    for (auto& params : draw_info)
    {
        pushBPBatch(params, planar, tex_anim);
    }

    LLVertexBuffer::unbind();
}

void LLRenderPass::pushShadowBPBatches(const std::vector<LLGLTFDrawInfo>& draw_info)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    pre_push_bp_batches();

    for (auto& params : draw_info)
    {
        pushShadowBPBatch(params);
    }

    LLVertexBuffer::unbind();
}

// static
void LLRenderPass::pushBPBatch(const LLGLTFDrawInfo& params, bool planar, bool tex_anim)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LL_PROFILE_ZONE_NUM(params.mInstanceCount);
    llassert(params.mTransformUBO != 0);

    if (params.mTransformUBO != transform_ubo)
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_NODES, params.mTransformUBO);
        glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_NODE_INSTANCE_MAP, params.mInstanceMapUBO);
        glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_MATERIALS, params.mMaterialUBO);
        if (tex_anim)
        {
            glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_TEXTURE_TRANSFORM, params.mTextureTransformUBO);
        }
        transform_ubo = params.mTransformUBO;
    }

    if (!last_mat || params.mMaterialID != last_mat)
    {
        last_mat = params.mMaterialID;
        if (diffuse_tu != -1 && cur_diffuse_tex != params.mDiffuseMap)
        {
            glActiveTexture(GL_TEXTURE0 + diffuse_tu);
            glBindTexture(GL_TEXTURE_2D, params.mDiffuseMap);
            cur_diffuse_tex = params.mDiffuseMap;
        }

        if (!LLPipeline::sShadowRender)
        {
            if (norm_tu != -1 && cur_norm_tex != params.mNormalMap)
            {
                glActiveTexture(GL_TEXTURE0 + norm_tu);
                glBindTexture(GL_TEXTURE_2D, params.mNormalMap);
                cur_norm_tex = params.mNormalMap;
            }

            if (specular_tu != -1 && cur_specular_tex != params.mSpecularMap)
            {
                glActiveTexture(GL_TEXTURE0 + specular_tu);
                glBindTexture(GL_TEXTURE_2D, params.mSpecularMap);
                cur_specular_tex = params.mSpecularMap;
            }
        }
    }

    glUniform1i(base_instance_index, params.mBaseInstance);

#if USE_VAO
    LLVertexBuffer::bindVAO(params.mVAO);
#else
    LLVertexBuffer::bindVBO(params.mVBO, params.mIBO, params.mVBOVertexCount);
#endif
    glDrawElementsInstanced(GL_TRIANGLES, params.mElementCount,
        gl_indices_type[params.mIndicesSize], (GLvoid*)(size_t)(params.mElementOffset * gl_indices_size[params.mIndicesSize]),
        params.mInstanceCount);
}

// static
void LLRenderPass::pushShadowBPBatch(const LLGLTFDrawInfo& params)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LL_PROFILE_ZONE_NUM(params.mInstanceCount);
    llassert(params.mTransformUBO != 0);

    if (params.mTransformUBO != transform_ubo)
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_NODES, params.mTransformUBO);
        glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_NODE_INSTANCE_MAP, params.mInstanceMapUBO);
        // NOTE: don't bind the material UBO here, it's not used in shadow pass
        transform_ubo = params.mTransformUBO;
    }

    glUniform1i(base_instance_index, params.mBaseInstance);

#if USE_VAO
    LLVertexBuffer::bindVAO(params.mVAO);
#else
    LLVertexBuffer::bindVBO(params.mVBO, params.mIBO, params.mVBOVertexCount);
#endif
    glDrawElementsInstanced(GL_TRIANGLES, params.mElementCount,
        gl_indices_type[params.mIndicesSize], (GLvoid*)(size_t)(params.mElementOffset * gl_indices_size[params.mIndicesSize]),
        params.mInstanceCount);
}

void LLRenderPass::pushRiggedBPBatches(const std::vector<LLSkinnedGLTFDrawInfo>& draw_info, bool planar, bool tex_anim)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    pre_push_bp_batches();

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (auto& params : draw_info)
    {
        pushRiggedBPBatch(params, lastAvatar, lastMeshId, skipLastSkin, planar, tex_anim);
    }

    LLVertexBuffer::unbind();
}

void LLRenderPass::pushRiggedShadowBPBatches(const std::vector<LLSkinnedGLTFDrawInfo>& draw_info)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    pre_push_bp_batches();

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (auto& params : draw_info)
    {
        pushRiggedShadowBPBatch(params, lastAvatar, lastMeshId, skipLastSkin);
    }

    LLVertexBuffer::unbind();
}

// static
void LLRenderPass::pushRiggedBPBatch(const LLSkinnedGLTFDrawInfo& params, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin, bool planar, bool tex_anim)
{
    if (uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
    {
        pushBPBatch(params, planar, tex_anim);
    }
}

// static
void LLRenderPass::pushRiggedShadowBPBatch(const LLSkinnedGLTFDrawInfo& params, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin)
{
    if (uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
    {
        pushShadowBPBatch(params);
    }
}

