 /**
 * @file llface.cpp
 * @brief LLFace class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include <boost/functional/hash.hpp>

#include "lldrawable.h" // lldrawable needs to be included before llface
#include "llface.h"
#include "llviewertextureanim.h"

#include "llviewercontrol.h"
#include "llvolume.h"
#include "m3math.h"
#include "llmatrix4a.h"
#include "v3color.h"

#include "lldefs.h"

#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "llgl.h"
#include "llrender.h"
#include "lllightconstants.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llvopartgroup.h"
#include "llvovolume.h"
#include "pipeline.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llviewershadermgr.h"
#include "llviewertexture.h"
#include "llvoavatar.h"
#include "llsculptidsize.h"
#include "llmeshrepository.h"
#include "llskinningutil.h"

#define LL_MAX_INDICES_COUNT 1000000

static LLStaticHashedString sTextureIndexIn("texture_index_in");
static LLStaticHashedString sColorIn("color_in");

bool LLFace::sSafeRenderSelect = true; // false


#define DOTVEC(a,b) (a.mV[0]*b.mV[0] + a.mV[1]*b.mV[1] + a.mV[2]*b.mV[2])

/*
For each vertex, given:
    B - binormal
    T - tangent
    N - normal
    P - position

The resulting texture coordinate <u,v> is:

    u = 2(B dot P)
    v = 2(T dot P)
*/
void planarProjection(LLVector2 &tc, const LLVector4a& normal,
                      const LLVector4a &center, const LLVector4a& vec)
{
    LLVector4a binormal;
    F32 d = normal[0];

    if (d >= 0.5f || d <= -0.5f)
    {
        if (d < 0)
        {
            binormal.set(0,-1,0);
        }
        else
        {
            binormal.set(0, 1, 0);
        }
    }
    else
    {
        if (normal[1] > 0)
        {
            binormal.set(-1,0,0);
        }
        else
        {
            binormal.set(1,0,0);
        }
    }
    LLVector4a tangent;
    tangent.setCross3(binormal,normal);

    tc.mV[1] = -((tangent.dot3(vec).getF32())*2 - 0.5f);
    tc.mV[0] = 1.0f+((binormal.dot3(vec).getF32())*2 - 0.5f);
}

////////////////////
//
// LLFace implementation
//

void LLFace::init(LLDrawable* drawablep, LLViewerObject* objp)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_FACE;
    mLastUpdateTime = gFrameTimeSeconds;
    mLastMoveTime = 0.f;
    mLastSkinTime = gFrameTimeSeconds;
    mVSize = 0.f;
    mPixelArea = 16.f;
    mState      = GLOBAL;
    mDrawPoolp  = NULL;
    mPoolType = 0;
    mCenterLocal = objp->getPosition();
    mCenterAgent = drawablep->getPositionAgent();
    mDistance   = 0.f;

    mGeomCount      = 0;
    mGeomIndex      = 0;
    mIndicesCount   = 0;

    //special value to indicate uninitialized position
    mIndicesIndex   = 0xFFFFFFFF;

    for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
    {
        mIndexInTex[i] = 0;
        mTexture[i] = NULL;
    }

    mTEOffset       = -1;
    mTextureIndex = FACE_DO_NOT_BATCH_TEXTURES;

    setDrawable(drawablep);
    mVObjp = objp;

    mReferenceIndex = -1;

    mTextureMatrix = NULL;
    mDrawInfo = NULL;

    mFaceColor = LLColor4(1,0,0,1);

    mImportanceToCamera = 0.f ;
    mBoundingSphereRadius = 0.0f ;

    mHasMedia = false ;
    mIsMediaAllowed = true;
}

void LLFace::destroy()
{
    if (gDebugGL)
    {
        gPipeline.checkReferences(this);
    }

    for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
    {
        if(mTexture[i].notNull())
        {
            mTexture[i]->removeFace(i, this) ;
            mTexture[i] = NULL;
        }
    }

    if (isState(LLFace::PARTICLE))
    {
        clearState(LLFace::PARTICLE);
    }

    if (mDrawPoolp)
    {
        mDrawPoolp->removeFace(this);
        mDrawPoolp = NULL;
    }

    if (mTextureMatrix)
    {
        delete mTextureMatrix;
        mTextureMatrix = NULL;

        if (mDrawablep)
        {
            LLSpatialGroup* group = mDrawablep->getSpatialGroup();
            if (group)
            {
                group->dirtyGeom();
                gPipeline.markRebuild(group);
                gPipeline.markTransformDirty(group);
            }
        }
    }

    setDrawInfo(NULL);

    mDrawablep = NULL;
    mVObjp = NULL;
}

void LLFace::setWorldMatrix(const LLMatrix4 &mat)
{
    LL_ERRS() << "Faces on this drawable are not independently modifiable\n" << LL_ENDL;
}

void LLFace::setPool(LLFacePool* pool)
{
    mDrawPoolp = pool;
}

void LLFace::setPool(LLFacePool* new_pool, LLViewerTexture *texturep)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_FACE;

    if (!new_pool)
    {
        LL_ERRS() << "Setting pool to null!" << LL_ENDL;
    }

    if (new_pool != mDrawPoolp)
    {
        // Remove from old pool
        if (mDrawPoolp)
        {
            mDrawPoolp->removeFace(this);

            if (mDrawablep)
            {
                gPipeline.markRebuild(mDrawablep, LLDrawable::REBUILD_ALL);
            }
        }
        mGeomIndex = 0;

        // Add to new pool
        if (new_pool)
        {
            new_pool->addFace(this);
        }
        mDrawPoolp = new_pool;
    }

    setTexture(texturep) ;
}

void LLFace::setTexture(U32 ch, LLViewerTexture* tex)
{
    llassert(ch < LLRender::NUM_TEXTURE_CHANNELS);

    if(mTexture[ch] == tex)
    {
        return ;
    }

    if(mTexture[ch].notNull())
    {
        mTexture[ch]->removeFace(ch, this) ;
    }

    if(tex)
    {
        tex->addFace(ch, this) ;
    }

    mTexture[ch] = tex ;

    if (mGLTFDrawInfo)
    {
        gPipeline.markTransformDirty(mDrawablep->getSpatialGroup());
    }
}

void LLFace::setTexture(LLViewerTexture* tex)
{
    setDiffuseMap(tex);
}

void LLFace::setDiffuseMap(LLViewerTexture* tex)
{
    setTexture(LLRender::DIFFUSE_MAP, tex);
}

void LLFace::setAlternateDiffuseMap(LLViewerTexture* tex)
{
    setTexture(LLRender::ALTERNATE_DIFFUSE_MAP, tex);
}

void LLFace::setNormalMap(LLViewerTexture* tex)
{
    setTexture(LLRender::NORMAL_MAP, tex);
}

void LLFace::setSpecularMap(LLViewerTexture* tex)
{
    setTexture(LLRender::SPECULAR_MAP, tex);
}

void LLFace::dirtyTexture()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_FACE;

    LLDrawable* drawablep = getDrawable();

    if (mVObjp.notNull() && mVObjp->getVolume())
    {
        for (U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
        {
            if (mTexture[ch].notNull() && mTexture[ch]->getComponents() == 4)
            { //dirty texture on an alpha object should be treated as an LoD update
                LLVOVolume* vobj = drawablep->getVOVolume();
                if (vobj)
                {
                    vobj->mLODChanged = true;

                    vobj->updateVisualComplexity();
                }
                gPipeline.markRebuild(drawablep, LLDrawable::REBUILD_VOLUME);
            }
        }
    }

    gPipeline.markTextured(drawablep);
}

void LLFace::switchTexture(U32 ch, LLViewerTexture* new_texture)
{
    llassert(ch < LLRender::NUM_TEXTURE_CHANNELS);

    if(mTexture[ch] == new_texture)
    {
        return ;
    }

    if(!new_texture)
    {
        LL_ERRS() << "Can not switch to a null texture." << LL_ENDL;
        return;
    }

    if (ch == LLRender::DIFFUSE_MAP)
    {
        getViewerObject()->changeTEImage(mTEOffset, new_texture) ;
    }

    setTexture(ch, new_texture) ;
    dirtyTexture();
}

void LLFace::setTEOffset(const S32 te_offset)
{
    mTEOffset = te_offset;
}


void LLFace::setFaceColor(const LLColor4& color)
{
    mFaceColor = color;
    setState(USE_FACE_COLOR);
}

void LLFace::unsetFaceColor()
{
    clearState(USE_FACE_COLOR);
}

void LLFace::setDrawable(LLDrawable *drawable)
{
    mDrawablep  = drawable;
    mXform      = &drawable->mXform;
}

void LLFace::setSize(S32 num_vertices, S32 num_indices, bool align)
{
    if (align)
    {
        //allocate vertices in blocks of 4 for alignment
        num_vertices = (num_vertices + 0x3) & ~0x3;
    }

    if (mGeomCount != num_vertices ||
        mIndicesCount != num_indices)
    {
        mGeomCount    = num_vertices;
        mIndicesCount = num_indices;
        mVertexBuffer = NULL;
    }

    llassert(verify());
}

void LLFace::setGeomIndex(U16 idx)
{
    if (mGeomIndex != idx)
    {
        mGeomIndex = idx;
        mVertexBuffer = NULL;
    }
}

void LLFace::setTextureIndex(U8 index)
{
    if (index != mTextureIndex)
    {
        mTextureIndex = index;

        if (mTextureIndex != FACE_DO_NOT_BATCH_TEXTURES)
        {
            mDrawablep->setState(LLDrawable::REBUILD_POSITION);
        }
        else
        {
            if (mDrawInfo && !mDrawInfo->mTextureList.empty())
            {
                LL_ERRS() << "Face with no texture index references indexed texture draw info." << LL_ENDL;
            }
        }
    }
}

void LLFace::setIndicesIndex(S32 idx)
{
    if (mIndicesIndex != idx)
    {
        mIndicesIndex = idx;
        mVertexBuffer = NULL;
    }
}

//============================================================================

U16 LLFace::getGeometryAvatar(
                        LLStrider<LLVector3> &vertices,
                        LLStrider<LLVector3> &normals,
                        LLStrider<LLVector2> &tex_coords,
                        LLStrider<F32>       &vertex_weights,
                        LLStrider<LLVector4> &clothing_weights)
{
    if (mVertexBuffer.notNull())
    {
        mVertexBuffer->getVertexStrider      (vertices, mGeomIndex, mGeomCount);
        mVertexBuffer->getNormalStrider      (normals, mGeomIndex, mGeomCount);
        mVertexBuffer->getTexCoord0Strider    (tex_coords, mGeomIndex, mGeomCount);
        mVertexBuffer->getWeightStrider(vertex_weights, mGeomIndex, mGeomCount);
        mVertexBuffer->getClothWeightStrider(clothing_weights, mGeomIndex, mGeomCount);
    }

    return mGeomIndex;
}

U16 LLFace::getGeometry(LLStrider<LLVector3> &vertices, LLStrider<LLVector3> &normals,
                        LLStrider<LLVector2> &tex_coords, LLStrider<U16> &indicesp)
{
    if (mVertexBuffer.notNull())
    {
        mVertexBuffer->getVertexStrider(vertices,   mGeomIndex, mGeomCount);
        if (mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_NORMAL))
        {
            mVertexBuffer->getNormalStrider(normals,    mGeomIndex, mGeomCount);
        }
        if (mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD0))
        {
            mVertexBuffer->getTexCoord0Strider(tex_coords, mGeomIndex, mGeomCount);
        }

        mVertexBuffer->getIndexStrider(indicesp, mIndicesIndex, mIndicesCount);
    }

    return mGeomIndex;
}

void LLFace::updateCenterAgent()
{
    if (mDrawablep->isActive())
    {
        mCenterAgent = mCenterLocal * getRenderMatrix();
    }
    else
    {
        mCenterAgent = mCenterLocal;
    }
}

void LLFace::renderSelected(LLViewerTexture *imagep, const LLColor4& color)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_FACE;

    if (mDrawablep == NULL || mDrawablep->getSpatialGroup() == NULL)
    {
        return;
    }

    mDrawablep->getSpatialGroup()->rebuildGeom();
    mDrawablep->getSpatialGroup()->rebuildMesh();

    if (mVertexBuffer.isNull())
    {
        return;
    }

    if (mGeomCount > 0 && mIndicesCount > 0)
    {
        gGL.getTexUnit(0)->bind(imagep);

        gGL.pushMatrix();
        if (mDrawablep->isActive())
        {
            gGL.multMatrix((GLfloat*)mDrawablep->getRenderMatrix().mMatrix);
        }
        else
        {
            gGL.multMatrix((GLfloat*)mDrawablep->getRegion()->mRenderMatrix.mMatrix);
        }

        gGL.diffuseColor4fv(color.mV);

        if (mDrawablep->isState(LLDrawable::RIGGED))
        {
#if 0 // TODO --  there is no way this won't destroy our GL machine as implemented, rewrite it to not rely on software skinning
            LLVOVolume* volume = mDrawablep->getVOVolume();
            if (volume)
            {
                LLRiggedVolume* rigged = volume->getRiggedVolume();
                if (rigged)
                {
                    // called when selecting a face during edit of a mesh object
                    LLGLEnable offset(GL_POLYGON_OFFSET_FILL);
                    glPolygonOffset(-1.f, -1.f);
                    gGL.multMatrix((F32*) volume->getRelativeXform().mMatrix);
                    const LLVolumeFace& vol_face = rigged->getVolumeFace(getTEOffset());
                    LLVertexBuffer::unbind();
                    glVertexPointer(3, GL_FLOAT, 16, vol_face.mPositions);
                    if (vol_face.mTexCoords)
                    {
                        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                        glTexCoordPointer(2, GL_FLOAT, 8, vol_face.mTexCoords);
                    }
                    gGL.syncMatrices();
                    glDrawElements(GL_TRIANGLES, vol_face.mNumIndices, GL_UNSIGNED_SHORT, vol_face.mIndices);
                    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                }
            }
#endif
        }
        else
        {
            // cheaters sometimes prosper...
            //
            LLVertexBuffer* vertex_buffer = mVertexBuffer.get();
            // To display selection markers (white squares with the rounded cross at the center)
            // on faces with GLTF textures we use a spectal vertex buffer with other transforms
            if (const LLTextureEntry* te = getTextureEntry())
            {
                if (LLGLTFMaterial* gltf_mat = te->getGLTFRenderMaterial())
                {
                    vertex_buffer = mVertexBufferGLTF.get();
                }
            }
            // Draw the selection marker using the correctly chosen vertex buffer
            if (vertex_buffer)
            {
                vertex_buffer->setBuffer();
                vertex_buffer->draw(LLRender::TRIANGLES, mIndicesCount, mIndicesIndex);
            }
        }

        gGL.popMatrix();
    }
}


void renderFace(LLDrawable* drawable, LLFace *face)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_FACE;

    LLVOVolume* vobj = drawable->getVOVolume();
    if (vobj)
    {
        LLVolume* volume = NULL;

        if (drawable->isState(LLDrawable::RIGGED))
        {
            volume = vobj->getRiggedVolume();
        }
        else
        {
            volume = vobj->getVolume();
        }

        if (volume)
        {
            const LLVolumeFace& vol_face = volume->getVolumeFace(face->getTEOffset());
            LLVertexBuffer::drawElements(LLRender::TRIANGLES, vol_face.mPositions, NULL, vol_face.mNumIndices, vol_face.mIndices);
        }
    }
}

void LLFace::renderOneWireframe(const LLColor4 &color, F32 fogCfx, bool wireframe_selection, bool bRenderHiddenSelections, bool shader)
{
    if (bRenderHiddenSelections)
    {
        gGL.blendFunc(LLRender::BF_SOURCE_COLOR, LLRender::BF_ONE);
        LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_GEQUAL);
        if (shader)
        {
            gGL.diffuseColor4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.4f);
            renderFace(mDrawablep, this);
        }
        else
        {
            gGL.flush();
            {
                gGL.diffuseColor4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.4f);
                renderFace(mDrawablep, this);
            }
        }
    }

    gGL.flush();
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    gGL.diffuseColor4f(color.mV[VRED] * 2, color.mV[VGREEN] * 2, color.mV[VBLUE] * 2, color.mV[VALPHA]);

    {
        LLGLDisable depth(wireframe_selection ? 0 : GL_BLEND);

        LLGLEnable offset(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(3.f, 3.f);
        glLineWidth(5.f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        renderFace(mDrawablep, this);
    }
}

void LLFace::setDrawInfo(LLDrawInfo* draw_info)
{
    mDrawInfo = draw_info;
}

void LLFace::printDebugInfo() const
{
    LLFacePool *poolp = getPool();
    LL_INFOS() << "Object: " << getViewerObject()->mID << LL_ENDL;
    if (getDrawable())
    {
        LL_INFOS() << "Type: " << LLPrimitive::pCodeToString(getDrawable()->getVObj()->getPCode()) << LL_ENDL;
    }
    if (getTexture())
    {
        LL_INFOS() << "Texture: " << getTexture() << " Comps: " << (U32)getTexture()->getComponents() << LL_ENDL;
    }
    else
    {
        LL_INFOS() << "No texture: " << LL_ENDL;
    }

    LL_INFOS() << "Face: " << this << LL_ENDL;
    LL_INFOS() << "State: " << getState() << LL_ENDL;
    LL_INFOS() << "Geom Index Data:" << LL_ENDL;
    LL_INFOS() << "--------------------" << LL_ENDL;
    LL_INFOS() << "GI: " << mGeomIndex << " Count:" << mGeomCount << LL_ENDL;
    LL_INFOS() << "Face Index Data:" << LL_ENDL;
    LL_INFOS() << "--------------------" << LL_ENDL;
    LL_INFOS() << "II: " << mIndicesIndex << " Count:" << mIndicesCount << LL_ENDL;
    LL_INFOS() << LL_ENDL;

    if (poolp)
    {
        poolp->printDebugInfo();

        S32 pool_references = 0;
        for (std::vector<LLFace*>::iterator iter = poolp->mReferences.begin();
             iter != poolp->mReferences.end(); iter++)
        {
            LLFace *facep = *iter;
            if (facep == this)
            {
                LL_INFOS() << "Pool reference: " << pool_references << LL_ENDL;
                pool_references++;
            }
        }

        if (pool_references != 1)
        {
            LL_INFOS() << "Incorrect number of pool references!" << LL_ENDL;
        }
    }

#if 0
    LL_INFOS() << "Indices:" << LL_ENDL;
    LL_INFOS() << "--------------------" << LL_ENDL;

    const U32 *indicesp = getRawIndices();
    S32 indices_count = getIndicesCount();
    S32 geom_start = getGeomStart();

    for (S32 i = 0; i < indices_count; i++)
    {
        LL_INFOS() << i << ":" << indicesp[i] << ":" << (S32)(indicesp[i] - geom_start) << LL_ENDL;
    }
    LL_INFOS() << LL_ENDL;

    LL_INFOS() << "Vertices:" << LL_ENDL;
    LL_INFOS() << "--------------------" << LL_ENDL;
    for (S32 i = 0; i < mGeomCount; i++)
    {
        LL_INFOS() << mGeomIndex + i << ":" << poolp->getVertex(mGeomIndex + i) << LL_ENDL;
    }
    LL_INFOS() << LL_ENDL;
#endif
}

// Transform the texture coordinates for this face.
static void xform(LLVector2 &tex_coord, F32 cosAng, F32 sinAng, F32 offS, F32 offT, F32 magS, F32 magT)
{
    // New, good way
    F32 s = tex_coord.mV[0];
    F32 t = tex_coord.mV[1];

    // Texture transforms are done about the center of the face.
    s -= 0.5;
    t -= 0.5;

    // Handle rotation
    F32 temp = s;
    s  = s     * cosAng + t * sinAng;
    t  = -temp * sinAng + t * cosAng;

    // Then scale
    s *= magS;
    t *= magT;

    // Then offset
    s += offS + 0.5f;
    t += offT + 0.5f;

    tex_coord.mV[0] = s;
    tex_coord.mV[1] = t;
}

#if 0
// Transform the texture coordinates for this face.
static void xform4a(LLVector4a &tex_coord, const LLVector4a& trans, const LLVector4Logical& mask, const LLVector4a& rot0, const LLVector4a& rot1, const LLVector4a& offset, const LLVector4a& scale)
{
    //tex coord is two coords, <s0, t0, s1, t1>
    LLVector4a st;

    // Texture transforms are done about the center of the face.
    st.setAdd(tex_coord, trans);

    // Handle rotation
    LLVector4a rot_st;

    // <s0 * cosAng, s0*-sinAng, s1*cosAng, s1*-sinAng>
    LLVector4a s0;
    s0.splat(st, 0);
    LLVector4a s1;
    s1.splat(st, 2);
    LLVector4a ss;
    ss.setSelectWithMask(mask, s1, s0);

    LLVector4a a;
    a.setMul(rot0, ss);

    // <t0*sinAng, t0*cosAng, t1*sinAng, t1*cosAng>
    LLVector4a t0;
    t0.splat(st, 1);
    LLVector4a t1;
    t1.splat(st, 3);
    LLVector4a tt;
    tt.setSelectWithMask(mask, t1, t0);

    LLVector4a b;
    b.setMul(rot1, tt);

    st.setAdd(a,b);

    // Then scale
    st.mul(scale);

    // Then offset
    tex_coord.setAdd(st, offset);
}
#endif

bool less_than_max_mag(const LLVector4a& vec)
{
    LLVector4a MAX_MAG;
    MAX_MAG.splat(1024.f*1024.f);

    LLVector4a val;
    val.setAbs(vec);

    S32 lt = val.lessThan(MAX_MAG).getGatheredBits() & 0x7;

    return lt == 0x7;
}

bool LLFace::genVolumeBBoxes(const LLVolume &volume, S32 f,
                             const LLMatrix4& mat_vert_in, bool global_volume)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_FACE;

    //get bounding box
    if (mDrawablep->isState(LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION | LLDrawable::REBUILD_RIGGED))
    {
        if (f >= volume.getNumVolumeFaces())
        {
            LL_WARNS() << "Generating bounding box for invalid face index!" << LL_ENDL;
            f = 0;
        }

        const LLVolumeFace &face = volume.getVolumeFace(f);

        // MAINT-8264 - stray vertices, especially in low LODs, cause bounding box errors.
        if (face.mNumVertices < 3)
        {
            LL_DEBUGS("RiggedBox") << "skipping face " << f << ", bad num vertices "
                                   << face.mNumVertices << " " << face.mNumIndices << " " << face.mWeights << LL_ENDL;
            return false;
        }

        //VECTORIZE THIS
        LLMatrix4a mat_vert;
        mat_vert.loadu(mat_vert_in);

        llassert(less_than_max_mag(face.mExtents[0]));
        llassert(less_than_max_mag(face.mExtents[1]));

        matMulBoundBox(mat_vert, face.mExtents, mExtents);

        if (!mDrawablep->isActive())
        {   // Shift position for region
            LLVector4a offset;
            offset.load3(mDrawablep->getRegion()->getOriginAgent().mV);
            mExtents[0].add(offset);
            mExtents[1].add(offset);
        }

        LLVector4a t;
        t.setAdd(mExtents[0],mExtents[1]);
        t.mul(0.5f);

        mCenterLocal.set(t.getF32ptr());

        t.setSub(mExtents[1],mExtents[0]);
        mBoundingSphereRadius = t.getLength3().getF32()*0.5f;

        updateCenterAgent();
    }

    return true;
}



// convert surface coordinates to texture coordinates, based on
// the values in the texture entry.
LLVector2 LLFace::surfaceToTexture(LLVector2 surface_coord, const LLVector4a& position, const LLVector4a& normal)
{
    LLVector2 tc = surface_coord;

    const LLTextureEntry *tep = getTextureEntry();

    if (tep == NULL)
    {
        // can't do much without the texture entry
        return surface_coord;
    }

    //VECTORIZE THIS
    // see if we have a non-default mapping
    U8 texgen = getTextureEntry()->getTexGen();
    if (texgen != LLTextureEntry::TEX_GEN_DEFAULT)
    {
        LLVector4a& center = *(mDrawablep->getVOVolume()->getVolume()->getVolumeFace(mTEOffset).mCenter);

        LLVector4a volume_position;
        LLVector3 v_position(position.getF32ptr());

        volume_position.load3(mDrawablep->getVOVolume()->agentPositionToVolume(v_position).mV);

        if (!mDrawablep->getVOVolume()->isVolumeGlobal())
        {
            LLVector4a scale;
            scale.load3(mVObjp->getScale().mV);
            volume_position.mul(scale);
        }

        LLVector4a volume_normal;
        LLVector3 v_normal(normal.getF32ptr());
        volume_normal.load3(mDrawablep->getVOVolume()->agentDirectionToVolume(v_normal).mV);
        volume_normal.normalize3fast();

        if (texgen == LLTextureEntry::TEX_GEN_PLANAR)
        {
            planarProjection(tc, volume_normal, center, volume_position);
        }
    }

    if (mTextureMatrix) // if we have a texture matrix, use it
    {
        LLVector3 tc3(tc);
        tc3 = tc3 * *mTextureMatrix;
        tc = LLVector2(tc3);
    }

    else // otherwise use the texture entry parameters
    {
        xform(tc, cos(tep->getRotation()), sin(tep->getRotation()),
              tep->mOffsetS, tep->mOffsetT, tep->mScaleS, tep->mScaleT);
    }


    return tc;
}

// Returns scale compared to default texgen, and face orientation as calculated
// by planarProjection(). This is needed to match planar texgen parameters.
void LLFace::getPlanarProjectedParams(LLQuaternion* face_rot, LLVector3* face_pos, F32* scale) const
{
    const LLMatrix4& vol_mat = getWorldMatrix();
    const LLVolumeFace& vf = getViewerObject()->getVolume()->getVolumeFace(mTEOffset);
    if (! (vf.mNormals && vf.mTangents))
    {
        return;
    }
    const LLVector4a& normal4a = *vf.mNormals;
    const LLVector4a& tangent  = *vf.mTangents;

    LLVector4a binormal4a;
    binormal4a.setCross3(normal4a, tangent);
    binormal4a.mul(tangent.getF32ptr()[3]);

    LLVector2 projected_binormal;
    planarProjection(projected_binormal, normal4a, *vf.mCenter, binormal4a);
    projected_binormal -= LLVector2(0.5f, 0.5f); // this normally happens in xform()
    *scale = projected_binormal.length();
    // rotate binormal to match what planarProjection() thinks it is,
    // then find rotation from that:
    projected_binormal.normalize();
    F32 ang = acos(projected_binormal.mV[VY]);
    ang = (projected_binormal.mV[VX] < 0.f) ? -ang : ang;

    //VECTORIZE THIS
    LLVector3 binormal(binormal4a.getF32ptr());
    LLVector3 normal(normal4a.getF32ptr());
    binormal.rotVec(ang, normal);
    LLQuaternion local_rot( binormal % normal, binormal, normal );
    *face_rot = local_rot * vol_mat.quaternion();
    *face_pos = vol_mat.getTranslation();
}

// Returns the necessary texture transform to align this face's TE to align_to's TE
bool LLFace::calcAlignedPlanarTE(const LLFace* align_to,  LLVector2* res_st_offset,
                                 LLVector2* res_st_scale, F32* res_st_rot, LLRender::eTexIndex map) const
{
    if (!align_to)
    {
        return false;
    }
    const LLTextureEntry *orig_tep = align_to->getTextureEntry();
    if ((orig_tep->getTexGen() != LLTextureEntry::TEX_GEN_PLANAR) ||
        (getTextureEntry()->getTexGen() != LLTextureEntry::TEX_GEN_PLANAR))
    {
        return false;
    }

    F32 map_rot = 0.f, map_scaleS = 0.f, map_scaleT = 0.f, map_offsS = 0.f, map_offsT = 0.f;

    LLMaterial* mat = orig_tep->getMaterialParams();
    if (!mat && map != LLRender::DIFFUSE_MAP)
    {
        LL_WARNS_ONCE("llface") << "Face is set to use specular or normal map but has no material, defaulting to diffuse" << LL_ENDL;
        map = LLRender::DIFFUSE_MAP;
    }

    switch (map)
    {
    case LLRender::DIFFUSE_MAP:
        map_rot = orig_tep->getRotation();
        map_scaleS = orig_tep->mScaleS;
        map_scaleT = orig_tep->mScaleT;
        map_offsS = orig_tep->mOffsetS;
        map_offsT = orig_tep->mOffsetT;
        break;
    case LLRender::NORMAL_MAP:
        if (mat->getNormalID().isNull())
        {
            return false;
        }
        map_rot = mat->getNormalRotation();
        map_scaleS = mat->getNormalRepeatX();
        map_scaleT = mat->getNormalRepeatY();
        map_offsS = mat->getNormalOffsetX();
        map_offsT = mat->getNormalOffsetY();
        break;
    case LLRender::SPECULAR_MAP:
        if (mat->getSpecularID().isNull())
        {
            return false;
        }
        map_rot = mat->getSpecularRotation();
        map_scaleS = mat->getSpecularRepeatX();
        map_scaleT = mat->getSpecularRepeatY();
        map_offsS = mat->getSpecularOffsetX();
        map_offsT = mat->getSpecularOffsetY();
        break;
    default: /*make compiler happy*/
        break;
    }

    LLVector3 orig_pos, this_pos;
    LLQuaternion orig_face_rot, this_face_rot;
    F32 orig_proj_scale, this_proj_scale;
    align_to->getPlanarProjectedParams(&orig_face_rot, &orig_pos, &orig_proj_scale);
    getPlanarProjectedParams(&this_face_rot, &this_pos, &this_proj_scale);

    // The rotation of "this face's" texture:
    LLQuaternion orig_st_rot = LLQuaternion(map_rot, LLVector3::z_axis) * orig_face_rot;
    LLQuaternion this_st_rot = orig_st_rot * ~this_face_rot;
    F32 x_ang, y_ang, z_ang;
    this_st_rot.getEulerAngles(&x_ang, &y_ang, &z_ang);
    *res_st_rot = z_ang;

    // Offset and scale of "this face's" texture:
    LLVector3 centers_dist = (this_pos - orig_pos) * ~orig_st_rot;
    LLVector3 st_scale(map_scaleS, map_scaleT, 1.f);
    st_scale *= orig_proj_scale;
    centers_dist.scaleVec(st_scale);
    LLVector2 orig_st_offset(map_offsS, map_offsT);

    *res_st_offset = orig_st_offset + (LLVector2)centers_dist;
    res_st_offset->mV[VX] -= (S32)res_st_offset->mV[VX];
    res_st_offset->mV[VY] -= (S32)res_st_offset->mV[VY];

    st_scale /= this_proj_scale;
    *res_st_scale = (LLVector2)st_scale;
    return true;
}

void LLFace::updateRebuildFlags()
{
    if (mDrawablep->isState(LLDrawable::REBUILD_VOLUME))
    { //this rebuild is zero overhead (direct consequence of some change that affects this face)
        mLastUpdateTime = gFrameTimeSeconds;
    }
    else
    { //this rebuild is overhead (side effect of some change that does not affect this face)
        mLastMoveTime = gFrameTimeSeconds;
    }
}


bool LLFace::canRenderAsMask()
{
    const LLTextureEntry* te = getTextureEntry();
    if( !te || !getViewerObject() || !getTexture() )
    {
        return false;
    }

    if (te->getGLTFRenderMaterial())
    {
        return false;
    }

    if (LLPipeline::sNoAlpha)
    {
        return true;
    }

    if (isState(LLFace::RIGGED))
    { // never auto alpha-mask rigged faces
        return false;
    }


    LLMaterial* mat = te->getMaterialParams();
    if (mat && mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_BLEND)
    {
        return false;
    }

    if ((te->getColor().mV[3] == 1.0f) && // can't treat as mask if we have face alpha
        (te->getGlow() == 0.f) && // glowing masks are hard to implement - don't mask
        getTexture()->getIsAlphaMask()) // texture actually qualifies for masking (lazily recalculated but expensive)
    {
        if (getViewerObject()->isHUDAttachment() || te->getFullbright())
        { //hud attachments and fullbright objects are NOT subject to the deferred rendering pipe
            return LLPipeline::sAutoMaskAlphaNonDeferred;
        }
        else
        {
            return LLPipeline::sAutoMaskAlphaDeferred;
        }
    }

    return false;
}

//helper function for pushing primitives for transform shaders and cleaning up
//uninitialized data on the tail, plus tracking number of expected primitives
void push_for_transform(LLVertexBuffer* buff, U32 source_count, U32 dest_count)
{
    if (source_count > 0 && dest_count >= source_count) //protect against possible U32 wrapping
    {
        //push source primitives
        buff->drawArrays(LLRender::POINTS, 0, source_count);
        U32 tail = dest_count-source_count;
        for (U32 i = 0; i < tail; ++i)
        { //copy last source primitive into each element in tail
            buff->drawArrays(LLRender::POINTS, source_count-1, 1);
        }
        gPipeline.mTransformFeedbackPrimitives += dest_count;
    }
}

void LLFace::renderIndexed()
{
    if (mVertexBuffer.notNull())
    {
        mVertexBuffer->setBuffer();
        mVertexBuffer->drawRange(LLRender::TRIANGLES, getGeomIndex(), getGeomIndex() + getGeomCount()-1, getIndicesCount(), getIndicesStart());
    }
}

//check if the face has a media
bool LLFace::hasMedia() const
{
    if(mHasMedia)
    {
        return true ;
    }
    if(mTexture[LLRender::DIFFUSE_MAP].notNull())
    {
        return mTexture[LLRender::DIFFUSE_MAP]->hasParcelMedia() ;  //if has a parcel media
    }

    return false ; //no media.
}

const F32 LEAST_IMPORTANCE = 0.05f ;
const F32 LEAST_IMPORTANCE_FOR_LARGE_IMAGE = 0.3f ;

void LLFace::resetVirtualSize()
{
    setVirtualSize(0.f);
    mImportanceToCamera = 0.f;
}

F32 LLFace::getTextureVirtualSize()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    F32 radius;
    F32 cos_angle_to_view_dir;
    bool in_frustum = calcPixelArea(cos_angle_to_view_dir, radius);


    if (mPixelArea < F_ALMOST_ZERO || !in_frustum)
    {
        setVirtualSize(0.f) ;
        return 0.f;
    }

    F32 face_area;
    if (mVObjp->isSculpted())
    {
        //sculpts can break assumptions about texel area
        face_area = mPixelArea;
    }
    else
    {
        //apply texel area to face area to get accurate ratio
        //face_area /= llclamp(texel_area, 1.f/64.f, 16.f);
        face_area = mPixelArea;
    }

    face_area = LLFace::adjustPixelArea(mImportanceToCamera, face_area);
    if(face_area > LLViewerTexture::sMinLargeImageSize) //if is large image, shrink face_area by considering the partial overlapping.
    {
        if(mImportanceToCamera > LEAST_IMPORTANCE_FOR_LARGE_IMAGE && mTexture[LLRender::DIFFUSE_MAP].notNull() && mTexture[LLRender::DIFFUSE_MAP]->isLargeImage())
        {
            face_area *= adjustPartialOverlapPixelArea(cos_angle_to_view_dir, radius );
        }
    }

    setVirtualSize(face_area) ;

    return face_area;
}

bool LLFace::calcPixelArea(F32& cos_angle_to_view_dir, F32& radius)
{
    constexpr F32 PIXEL_AREA_UPDATE_PERIOD = 0.1f;
    // this is an expensive operation and the result is valid (enough) for several frames
    // don't update every frame
    if (gFrameTimeSeconds - mLastPixelAreaUpdate < PIXEL_AREA_UPDATE_PERIOD)
    {
        return true;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_FACE;

    //get area of circle around face
    LLVector4a center;
    LLVector4a size;

    if (isState(LLFace::RIGGED))
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_FACE("calcPixelArea - rigged");
        //override with joint volume face joint bounding boxes
        LLVOAvatar* avatar = mVObjp->getAvatar();
        bool hasRiggedExtents = false;

        if (avatar && avatar->mDrawable)
        {
            LLVolume* volume = mVObjp->getVolume();
            if (volume)
            {
                LLVolumeFace& face = volume->getVolumeFace(mTEOffset);

                auto& rigInfo = face.mJointRiggingInfoTab;

                if (rigInfo.needsUpdate())
                {
                    LLVOVolume* vo_volume = (LLVOVolume*)mVObjp.get();
                    LLVOAvatar* avatar = mVObjp->getAvatar();
                    const LLMeshSkinInfo* skin = vo_volume->getSkinInfo();
                    LLSkinningUtil::updateRiggingInfo(skin, avatar, face);
                }

                // calculate the world space bounding box of the face by combining the bounding boxes of all the joints
                LLVector4a& minp = mRiggedExtents[0];
                LLVector4a& maxp = mRiggedExtents[1];
                minp = LLVector4a(FLT_MAX, FLT_MAX, FLT_MAX);
                maxp = LLVector4a(-FLT_MAX, -FLT_MAX, -FLT_MAX);

                for (S32 i = 0; i < rigInfo.size(); i++)
                {
                    auto& jointInfo = rigInfo[i];
                    if (jointInfo.isRiggedTo())
                    {
                        LLJoint* joint = avatar->getJoint(i);

                        if (joint)
                        {
                            LLVector4a jointPos;

                            LLMatrix4a worldMat;
                            worldMat.loadu((F32*)&joint->getWorldMatrix().mMatrix[0][0]);

                            LLVector4a extents[2];

                            matMulBoundBox(worldMat, jointInfo.getRiggedExtents(), extents);

                            minp.setMin(minp, extents[0]);
                            maxp.setMax(maxp, extents[1]);
                            hasRiggedExtents = true;
                        }
                    }
                }
            }
        }

        if (!hasRiggedExtents)
        {
            // no rigged extents, zero out bounding box and skip update
            mRiggedExtents[0] = mRiggedExtents[1] = LLVector4a(0.f, 0.f, 0.f);

            return false;
        }

        center.setAdd(mRiggedExtents[1], mRiggedExtents[0]);
        center.mul(0.5f);
        size.setSub(mRiggedExtents[1], mRiggedExtents[0]);
    }
    else if (mDrawablep && mVObjp.notNull() && mVObjp->getPartitionType() == LLViewerRegion::PARTITION_PARTICLE && mDrawablep->getSpatialGroup())
    { // use box of spatial group for particles (over approximates size, but we don't actually have a good size per particle)
        LLSpatialGroup* group = mDrawablep->getSpatialGroup();
        const LLVector4a* extents = group->getExtents();
        size.setSub(extents[1], extents[0]);
        center.setAdd(extents[1], extents[0]);
        center.mul(0.5f);
    }
    else
    {
        center.load3(getPositionAgent().mV);
        size.setSub(mExtents[1], mExtents[0]);
    }
    size.mul(0.5f);

    LLViewerCamera* camera = LLViewerCamera::getInstance();

    F32 size_squared = size.dot3(size).getF32();
    LLVector4a lookAt;
    LLVector4a t;
    t.load3(camera->getOrigin().mV);
    lookAt.setSub(center, t);

    F32 dist = lookAt.getLength3().getF32();
    dist = llmax(dist-size.getLength3().getF32(), 0.001f);

    lookAt.normalize3fast() ;

    //get area of circle around node
    F32 app_angle = atanf((F32) sqrt(size_squared) / dist);
    radius = app_angle*LLDrawable::sCurPixelAngle;
    mPixelArea = radius*radius * 3.14159f;

    // remember last update time, add 10% noise to avoid all faces updating at the same time
    mLastPixelAreaUpdate = gFrameTimeSeconds + ll_frand() * PIXEL_AREA_UPDATE_PERIOD * 0.1f;

    LLVector4a x_axis;
    x_axis.load3(camera->getXAxis().mV);
    cos_angle_to_view_dir = lookAt.dot3(x_axis).getF32();

    //if has media, check if the face is out of the view frustum.
    if(hasMedia())
    {
        if(!camera->AABBInFrustum(center, size))
        {
            mImportanceToCamera = 0.f ;
            return false ;
        }
        if(cos_angle_to_view_dir > camera->getCosHalfFov()) //the center is within the view frustum
        {
            cos_angle_to_view_dir = 1.0f ;
        }
        else
        {
            LLVector4a d;
            d.setSub(lookAt, x_axis);

            if(dist * dist * d.dot3(d) < size_squared)
            {
                cos_angle_to_view_dir = 1.0f ;
            }
        }
    }

    if(dist < mBoundingSphereRadius) //camera is very close
    {
        cos_angle_to_view_dir = 1.0f ;
        mImportanceToCamera = 1.0f ;
    }
    else
    {
        mImportanceToCamera = LLFace::calcImportanceToCamera(cos_angle_to_view_dir, dist) ;
    }

    return true ;
}

//the projection of the face partially overlaps with the screen
F32 LLFace::adjustPartialOverlapPixelArea(F32 cos_angle_to_view_dir, F32 radius )
{
    F32 screen_radius = (F32)llmax(gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw()) ;
    F32 center_angle = acosf(cos_angle_to_view_dir) ;
    F32 d = center_angle * LLDrawable::sCurPixelAngle ;

    if(d + radius > screen_radius + 5.f)
    {
        //----------------------------------------------
        //calculate the intersection area of two circles
        //F32 radius_square = radius * radius ;
        //F32 d_square = d * d ;
        //F32 screen_radius_square = screen_radius * screen_radius ;
        //face_area =
        //  radius_square * acosf((d_square + radius_square - screen_radius_square)/(2 * d * radius)) +
        //  screen_radius_square * acosf((d_square + screen_radius_square - radius_square)/(2 * d * screen_radius)) -
        //  0.5f * sqrtf((-d + radius + screen_radius) * (d + radius - screen_radius) * (d - radius + screen_radius) * (d + radius + screen_radius)) ;
        //----------------------------------------------

        //the above calculation is too expensive
        //the below is a good estimation: bounding box of the bounding sphere:
        F32 alpha = 0.5f * (radius + screen_radius - d) / radius ;
        alpha = llclamp(alpha, 0.f, 1.f) ;
        return alpha * alpha ;
    }
    return 1.0f ;
}

const S8 FACE_IMPORTANCE_LEVEL = 4 ;
const F32 FACE_IMPORTANCE_TO_CAMERA_OVER_DISTANCE[FACE_IMPORTANCE_LEVEL][2] = //{distance, importance_weight}
    {{16.1f, 1.0f}, {32.1f, 0.5f}, {48.1f, 0.2f}, {96.1f, 0.05f} } ;
const F32 FACE_IMPORTANCE_TO_CAMERA_OVER_ANGLE[FACE_IMPORTANCE_LEVEL][2] =    //{cos(angle), importance_weight}
    {{0.985f /*cos(10 degrees)*/, 1.0f}, {0.94f /*cos(20 degrees)*/, 0.8f}, {0.866f /*cos(30 degrees)*/, 0.64f}, {0.0f, 0.36f}} ;

//static
F32 LLFace::calcImportanceToCamera(F32 cos_angle_to_view_dir, F32 dist)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_FACE;
    F32 importance = 0.f ;

    if(cos_angle_to_view_dir > LLViewerCamera::getInstance()->getCosHalfFov() &&
        dist < FACE_IMPORTANCE_TO_CAMERA_OVER_DISTANCE[FACE_IMPORTANCE_LEVEL - 1][0])
    {
        LLViewerCamera* camera = LLViewerCamera::getInstance();
        F32 camera_moving_speed = camera->getAverageSpeed() ;
        F32 camera_angular_speed = camera->getAverageAngularSpeed();

        if(camera_moving_speed > 10.0f || camera_angular_speed > 1.0f)
        {
            //if camera moves or rotates too fast, ignore the importance factor
            return 0.f ;
        }

        S32 i = 0 ;
        for(i = 0; i < FACE_IMPORTANCE_LEVEL && dist > FACE_IMPORTANCE_TO_CAMERA_OVER_DISTANCE[i][0]; ++i);
        i = llmin(i, FACE_IMPORTANCE_LEVEL - 1) ;
        F32 dist_factor = FACE_IMPORTANCE_TO_CAMERA_OVER_DISTANCE[i][1] ;

        for(i = 0; i < FACE_IMPORTANCE_LEVEL && cos_angle_to_view_dir < FACE_IMPORTANCE_TO_CAMERA_OVER_ANGLE[i][0] ; ++i) ;
        i = llmin(i, FACE_IMPORTANCE_LEVEL - 1) ;
        importance = dist_factor * FACE_IMPORTANCE_TO_CAMERA_OVER_ANGLE[i][1] ;
    }

    return importance ;
}

//static
F32 LLFace::adjustPixelArea(F32 importance, F32 pixel_area)
{
    if(pixel_area > LLViewerTexture::sMaxSmallImageSize)
    {
        if(importance < LEAST_IMPORTANCE) //if the face is not important, do not load hi-res.
        {
            static const F32 MAX_LEAST_IMPORTANCE_IMAGE_SIZE = 128.0f * 128.0f ;
            pixel_area = llmin(pixel_area * 0.5f, MAX_LEAST_IMPORTANCE_IMAGE_SIZE) ;
        }
        else if(pixel_area > LLViewerTexture::sMinLargeImageSize) //if is large image, shrink face_area by considering the partial overlapping.
        {
            if(importance < LEAST_IMPORTANCE_FOR_LARGE_IMAGE)//if the face is not important, do not load hi-res.
            {
                pixel_area = (F32)LLViewerTexture::sMinLargeImageSize ;
            }
        }
    }

    return pixel_area ;
}

bool LLFace::verify(const U32* indices_array) const
{
    bool ok = true;

    if( mVertexBuffer.isNull() )
    { //no vertex buffer, face is implicitly valid
        return true;
    }

    // First, check whether the face data fits within the pool's range.
    if ((U32)(mGeomIndex + mGeomCount) > mVertexBuffer->getNumVerts())
    {
        ok = false;
        LL_INFOS() << "Face references invalid vertices!" << LL_ENDL;
    }

    S32 indices_count = (S32)getIndicesCount();

    if (!indices_count)
    {
        return true;
    }

    if (indices_count > LL_MAX_INDICES_COUNT)
    {
        ok = false;
        LL_INFOS() << "Face has bogus indices count" << LL_ENDL;
    }

    if (mIndicesIndex + mIndicesCount > mVertexBuffer->getNumIndices())
    {
        ok = false;
        LL_INFOS() << "Face references invalid indices!" << LL_ENDL;
    }

#if 0
    S32 geom_start = getGeomStart();
    S32 geom_count = mGeomCount;

    const U32 *indicesp = indices_array ? indices_array + mIndicesIndex : getRawIndices();

    for (S32 i = 0; i < indices_count; i++)
    {
        S32 delta = indicesp[i] - geom_start;
        if (0 > delta)
        {
            LL_WARNS() << "Face index too low!" << LL_ENDL;
            LL_INFOS() << "i:" << i << " Index:" << indicesp[i] << " GStart: " << geom_start << LL_ENDL;
            ok = false;
        }
        else if (delta >= geom_count)
        {
            LL_WARNS() << "Face index too high!" << LL_ENDL;
            LL_INFOS() << "i:" << i << " Index:" << indicesp[i] << " GEnd: " << geom_start + geom_count << LL_ENDL;
            ok = false;
        }
    }
#endif

    if (!ok)
    {
        printDebugInfo();
    }
    return ok;
}


void LLFace::setViewerObject(LLViewerObject* objp)
{
    mVObjp = objp;
}


const LLMatrix4& LLFace::getRenderMatrix() const
{
    return mDrawablep->getRenderMatrix();
}

//============================================================================
// From llface.inl

S32 LLFace::getColors(LLStrider<LLColor4U> &colors)
{
    if (!mGeomCount)
    {
        return -1;
    }

    // llassert(mGeomIndex >= 0);
    mVertexBuffer->getColorStrider(colors, mGeomIndex, mGeomCount);
    return mGeomIndex;
}

S32 LLFace::getIndices(LLStrider<U16> &indicesp)
{
    mVertexBuffer->getIndexStrider(indicesp, mIndicesIndex, mIndicesCount);
    llassert(indicesp[0] != indicesp[1]);
    return mIndicesIndex;
}

LLVector3 LLFace::getPositionAgent() const
{
    if (mDrawablep->isStatic())
    {
        return mCenterAgent;
    }
    else
    {
        return mCenterLocal * getRenderMatrix();
    }
}

LLViewerTexture* LLFace::getTexture(U32 ch) const
{
    llassert(ch < LLRender::NUM_TEXTURE_CHANNELS);

    return mTexture[ch] ;
}

void LLFace::setVertexBuffer(LLVertexBuffer* buffer)
{
    if (buffer)
    {
        LLSculptIDSize::instance().inc(mDrawablep, buffer->getSize() + buffer->getIndicesSize());
    }

    if (mVertexBuffer)
    {
        LLSculptIDSize::instance().dec(mDrawablep);
    }

    mVertexBuffer = buffer;
    llassert(verify());
}

void LLFace::clearVertexBuffer()
{
    if (mVertexBuffer)
    {
        LLSculptIDSize::instance().dec(mDrawablep);
    }

    mVertexBuffer = NULL;
}

S32 LLFace::getRiggedIndex(U32 type) const
{
    if (mRiggedIndex.empty())
    {
        return -1;
    }

    llassert(type < mRiggedIndex.size());

    return mRiggedIndex[type];
}

U64 LLFace::getSkinHash()
{
    return mSkinInfo ? mSkinInfo->mHash : 0;
}

void LLFace::updateBatchHash()
{
    auto* gltf_mat = getTextureEntry()->getGLTFRenderMaterial();
    if (gltf_mat)
    {
        gltf_mat->updateBatchHash();
        mBatchHash = gltf_mat->getBatchHash();
        mAlphaMode = gltf_mat->mAlphaMode;
    }
    else
    {
        // WIP - calculate blinn-phong batch hash and alpha mode
        const LLTextureEntry* te = getTextureEntry();

        mBatchHash = 0;

        boost::hash_combine(mBatchHash, te->getColor());
        boost::hash_combine(mBatchHash, te->getAlpha());
        boost::hash_combine(mBatchHash, te->getScaleS());
        boost::hash_combine(mBatchHash, te->getScaleT());
        boost::hash_combine(mBatchHash, te->getRotation());
        boost::hash_combine(mBatchHash, te->getOffsetS());
        boost::hash_combine(mBatchHash, te->getOffsetT());
        boost::hash_combine(mBatchHash, te->getFullbright());

        boost::hash_combine(mBatchHash, te->getID());
        const auto& mat = te->getMaterialParams();
        if (mat.notNull())
        {
            boost::hash_combine(mBatchHash, mat->getBatchHash());

            if (mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
            {
                boost::hash_combine(mBatchHash, mat->getAlphaMaskCutoff());
            }

            switch (mat->getDiffuseAlphaMode())
            {
            case LLMaterial::DIFFUSE_ALPHA_MODE_BLEND:
                mAlphaMode = LLGLTFMaterial::ALPHA_MODE_BLEND;
                break;
            case LLMaterial::DIFFUSE_ALPHA_MODE_MASK:
                mAlphaMode = LLGLTFMaterial::ALPHA_MODE_MASK;
                break;
            default:
                mAlphaMode = LLGLTFMaterial::ALPHA_MODE_OPAQUE;
                break;
            };
        }
        else
        {
            if (te->getAlpha() < 1.f || getTexture()->getComponents() == 4)
            {
                mAlphaMode = LLGLTFMaterial::ALPHA_MODE_BLEND;
            }
            else
            {
                mAlphaMode = LLGLTFMaterial::ALPHA_MODE_OPAQUE;
            }
        }
    }
}

void LLFace::packMaterialOnto(std::vector<LLVector4a>& dst)
{
    auto* gltf_mat = getTextureEntry()->getGLTFRenderMaterial();
    if (gltf_mat)
    {
        gltf_mat->packOnto(dst);
    }
    {
        // WIP -- pack blinn-phong material

        const LLTextureEntry* te = getTextureEntry();

        F32 env_intensity = 0.f;
        LLColor3 spec = LLColor3::black;

        const LLMaterial* mat = te->getMaterialParams().get();

        dst.resize(dst.size()+7);
        LLVector4a* data = &dst[dst.size()-7];

        F32 min_alpha = 1.f;
        F32 glossiness = 0.f;
        F32 emissive = 0.f;
        F32 emissive_mask = 0.f;

        LLColor4 col = te->getColor();

        data[0].set(te->getScaleS(), te->getScaleT(), te->getRotation(), te->getOffsetS());
        data[1].set(te->getOffsetT(), col.mV[0], col.mV[1], col.mV[2]);

        if (mat)
        {
            env_intensity = mat->getEnvironmentIntensity()/255.f;
            spec = mat->getSpecularLightColor();

            if (mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
            {
                min_alpha = mat->getAlphaMaskCutoff()/255.f;
            }
            
            data[2].set(mat->getNormalRepeatX(), mat->getNormalRepeatY(), mat->getNormalRotation(), mat->getNormalOffsetX());
            data[3].set(mat->getNormalOffsetY(), col.mV[3], min_alpha, env_intensity);

            data[4].set(mat->getSpecularRepeatX(), mat->getSpecularRepeatY(), mat->getSpecularRotation(), mat->getSpecularOffsetX());
            data[5].set(mat->getSpecularOffsetY(), spec.mV[0], spec.mV[1], spec.mV[2]);

            glossiness = mat->getSpecularLightExponent() / 255.f;

            if (mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_EMISSIVE)
            {
                emissive_mask = 1.f;
            }
        }
        else
        {
            F32 v[] = { 0.f, 0.25f, 0.5f, 0.75f };
            env_intensity = v[llclamp(te->getShiny(), 0, 4)];
            spec.set(env_intensity, env_intensity, env_intensity);

            data[2].set(1, 1, 0, 0);
            data[3].set(0, col.mV[3], min_alpha, env_intensity);

            data[4].set(1, 1, 0, 0);
            data[5].set(1, spec.mV[0], spec.mV[1], spec.mV[2]);
        }

        if (te->getFullbright())
        {
            emissive = 1.f;
        }

        data[6].set(emissive, emissive_mask, glossiness, 0.f);
    }
}

bool LLFace::isInAlphaPool() const
{
    return  getPoolType() == LLDrawPool::POOL_ALPHA ||
        getPoolType() == LLDrawPool::POOL_ALPHA_PRE_WATER ||
        getPoolType() == LLDrawPool::POOL_ALPHA_POST_WATER;
}
