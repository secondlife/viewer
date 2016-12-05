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

#include "lldrawable.h" // lldrawable needs to be included before llface
#include "llface.h"
#include "llviewertextureanim.h"

#include "llviewercontrol.h"
#include "llvolume.h"
#include "m3math.h"
#include "llmatrix4a.h"
#include "v3color.h"

#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "llgl.h"
#include "llrender.h"
#include "lllightconstants.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llvopartgroup.h"
#include "llvosky.h"
#include "llvovolume.h"
#include "pipeline.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llviewershadermgr.h"
#include "llviewertexture.h"
#include "llvoavatar.h"

#if LL_LINUX
// Work-around spurious used before init warning on Vector4a
//
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

extern BOOL gGLDebugLoggingEnabled;
#define LL_MAX_INDICES_COUNT 1000000

static LLStaticHashedString sTextureIndexIn("texture_index_in");
static LLStaticHashedString sColorIn("color_in");

BOOL LLFace::sSafeRenderSelect = TRUE; // FALSE

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
	mDistance	= 0.f;

	mGeomCount		= 0;
	mGeomIndex		= 0;
	mIndicesCount	= 0;

	//special value to indicate uninitialized position
	mIndicesIndex	= 0xFFFFFFFF;
	
	for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
	{
		mIndexInTex[i] = 0;
		mTexture[i] = NULL;
	}

	mTEOffset		= -1;
	mTextureIndex = 255;

	setDrawable(drawablep);
	mVObjp = objp;

	mReferenceIndex = -1;

	mTextureMatrix = NULL;
	mDrawInfo = NULL;

	mFaceColor = LLColor4(1,0,0,1);

	mImportanceToCamera = 0.f ;
	mBoundingSphereRadius = 0.0f ;

	mHasMedia = FALSE ;
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
		}
	}
	
	if (isState(LLFace::PARTICLE))
	{
		LLVOPartGroup::freeVBSlot(getGeomIndex()/4);
		clearState(LLFace::PARTICLE);
	}

	if (mDrawPoolp)
	{
		if (this->isState(LLFace::RIGGED) && mDrawPoolp->getType() == LLDrawPool::POOL_AVATAR)
		{
			((LLDrawPoolAvatar*) mDrawPoolp)->removeRiggedFace(this);
		}
		else
		{
			mDrawPoolp->removeFace(this);
		}
	
		mDrawPoolp = NULL;
	}

	if (mTextureMatrix)
	{
		delete mTextureMatrix;
		mTextureMatrix = NULL;

		if (mDrawablep.notNull())
		{
			LLSpatialGroup* group = mDrawablep->getSpatialGroup();
			if (group)
			{
				group->dirtyGeom();
				gPipeline.markRebuild(group, TRUE);
			}
		}
	}
	
	setDrawInfo(NULL);
		
	mDrawablep = NULL;
	mVObjp = NULL;
}


// static
void LLFace::initClass()
{
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
				gPipeline.markRebuild(mDrawablep, LLDrawable::REBUILD_ALL, TRUE);
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
}

void LLFace::setTexture(LLViewerTexture* tex) 
{
	setDiffuseMap(tex);
}

void LLFace::setDiffuseMap(LLViewerTexture* tex)
{
	setTexture(LLRender::DIFFUSE_MAP, tex);
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
					vobj->mLODChanged = TRUE;

					LLVOAvatar* avatar = vobj->getAvatar();
					if (avatar)
					{ //avatar render cost may have changed
						avatar->updateVisualComplexity();
					}
				}
				gPipeline.markRebuild(drawablep, LLDrawable::REBUILD_VOLUME, FALSE);
			}
		}
	}

	gPipeline.markTextured(drawablep);
}

void LLFace::notifyAboutCreatingTexture(LLViewerTexture *texture)
{
	LLDrawable* drawablep = getDrawable();
	if(mVObjp.notNull() && mVObjp->getVolume())
	{
		LLVOVolume *vobj = drawablep->getVOVolume();
		if(vobj && vobj->notifyAboutCreatingTexture(texture))
		{
			gPipeline.markTextured(drawablep);
			gPipeline.markRebuild(drawablep, LLDrawable::REBUILD_VOLUME);
		}
	}
}

void LLFace::notifyAboutMissingAsset(LLViewerTexture *texture)
{
	LLDrawable* drawablep = getDrawable();
	if(mVObjp.notNull() && mVObjp->getVolume())
	{
		LLVOVolume *vobj = drawablep->getVOVolume();
		if(vobj && vobj->notifyAboutMissingAsset(texture))
		{
			gPipeline.markTextured(drawablep);
			gPipeline.markRebuild(drawablep, LLDrawable::REBUILD_VOLUME);
		}
	}
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

	llassert(mTexture[ch].notNull());

	new_texture->addTextureStats(mTexture[ch]->getMaxVirtualSize()) ;

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

		if (mTextureIndex != 255)
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
						LLStrider<F32>		 &vertex_weights,
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
	if (mDrawablep->getSpatialGroup() == NULL)
	{
		return;
	}

	mDrawablep->getSpatialGroup()->rebuildGeom();
	mDrawablep->getSpatialGroup()->rebuildMesh();
		
	if(mDrawablep.isNull() || mVertexBuffer.isNull())
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
		}
		else
		{
			// cheaters sometimes prosper...
			//
			mVertexBuffer->setBuffer(mVertexBuffer->getTypeMask());
			mVertexBuffer->draw(LLRender::TRIANGLES, mIndicesCount, mIndicesIndex);
		}

		gGL.popMatrix();
	}
}


/* removed in lieu of raycast uv detection
void LLFace::renderSelectedUV()
{
	LLViewerTexture* red_blue_imagep = LLViewerTextureManager::getFetchedTextureFromFile("uv_test1.j2c", TRUE, LLGLTexture::BOOST_UI);
	LLViewerTexture* green_imagep = LLViewerTextureManager::getFetchedTextureFromFile("uv_test2.tga", TRUE, LLGLTexture::BOOST_UI);

	LLGLSUVSelect object_select;

	// use red/blue gradient to get coarse UV coordinates
	renderSelected(red_blue_imagep, LLColor4::white);
	
	static F32 bias = 0.f;
	static F32 factor = -10.f;
	glPolygonOffset(factor, bias);

	// add green dither pattern on top of red/blue gradient
	gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ONE);
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.pushMatrix();
	// make green pattern repeat once per texel in red/blue texture
	gGL.scalef(256.f, 256.f, 1.f);
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	renderSelected(green_imagep, LLColor4::white);

	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);
}
*/

void LLFace::setDrawInfo(LLDrawInfo* draw_info)
{
	if (draw_info)
	{
		if (draw_info->mFace)
		{
			draw_info->mFace->setDrawInfo(NULL);
		}
		draw_info->mFace = this;
	}
	
	if (mDrawInfo)
	{
		mDrawInfo->mFace = NULL;
	}

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


bool less_than_max_mag(const LLVector4a& vec)
{
	LLVector4a MAX_MAG;
	MAX_MAG.splat(1024.f*1024.f);

	LLVector4a val;
	val.setAbs(vec);

	S32 lt = val.lessThan(MAX_MAG).getGatheredBits() & 0x7;
	
	return lt == 0x7;
}

BOOL LLFace::genVolumeBBoxes(const LLVolume &volume, S32 f,
								const LLMatrix4& mat_vert_in, BOOL global_volume)
{
	//get bounding box
	if (mDrawablep->isState(LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION | LLDrawable::REBUILD_RIGGED))
	{
		//VECTORIZE THIS
		LLMatrix4a mat_vert;
		mat_vert.loadu(mat_vert_in);

		LLVector4a min,max;
	
		if (f >= volume.getNumVolumeFaces())
		{
			LL_WARNS() << "Generating bounding box for invalid face index!" << LL_ENDL;
			f = 0;
		}

		const LLVolumeFace &face = volume.getVolumeFace(f);
		min = face.mExtents[0];
		max = face.mExtents[1];
		
		llassert(less_than_max_mag(min));
		llassert(less_than_max_mag(max));

		//min, max are in volume space, convert to drawable render space

		//get 8 corners of bounding box
		LLVector4Logical mask[6];

		for (U32 i = 0; i < 6; ++i)
		{
			mask[i].clear();
		}

		mask[0].setElement<2>(); //001
		mask[1].setElement<1>(); //010
		mask[2].setElement<1>(); //011
		mask[2].setElement<2>();
		mask[3].setElement<0>(); //100
		mask[4].setElement<0>(); //101
		mask[4].setElement<2>();
		mask[5].setElement<0>(); //110
		mask[5].setElement<1>();

		LLVector4a v[8];

		v[6] = min;
		v[7] = max;

		for (U32 i = 0; i < 6; ++i)
		{
			v[i].setSelectWithMask(mask[i], min, max);
		}

		LLVector4a tv[8];

		//transform bounding box into drawable space
		for (U32 i = 0; i < 8; ++i)
		{
			mat_vert.affineTransform(v[i], tv[i]);
		}
	
		//find bounding box
		LLVector4a& newMin = mExtents[0];
		LLVector4a& newMax = mExtents[1];

		newMin = newMax = tv[0];

		for (U32 i = 1; i < 8; ++i)
		{
			newMin.setMin(newMin, tv[i]);
			newMax.setMax(newMax, tv[i]);
		}

		if (!mDrawablep->isActive())
		{	// Shift position for region
			LLVector4a offset;
			offset.load3(mDrawablep->getRegion()->getOriginAgent().mV);
			newMin.add(offset);
			newMax.add(offset);
		}

		LLVector4a t;
		t.setAdd(newMin,newMax);
		t.mul(0.5f);

		mCenterLocal.set(t.getF32ptr());

		t.setSub(newMax,newMin);
		mBoundingSphereRadius = t.getLength3().getF32()*0.5f;

		updateCenterAgent();
	}

	return TRUE;
}



// convert surface coordinates to texture coordinates, based on
// the values in the texture entry.  probably should be
// integrated with getGeometryVolume() for its texture coordinate
// generation - but i'll leave that to someone more familiar
// with the implications.
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

	if (mTextureMatrix)	// if we have a texture matrix, use it
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
	const LLVector4a& normal4a = vf.mNormals[0];
	const LLVector4a& tangent = vf.mTangents[0];
	if (!&tangent)
	{
		return;
	}

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
								 LLVector2* res_st_scale, F32* res_st_rot) const
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

	LLVector3 orig_pos, this_pos;
	LLQuaternion orig_face_rot, this_face_rot;
	F32 orig_proj_scale, this_proj_scale;
	align_to->getPlanarProjectedParams(&orig_face_rot, &orig_pos, &orig_proj_scale);
	getPlanarProjectedParams(&this_face_rot, &this_pos, &this_proj_scale);

	// The rotation of "this face's" texture:
	LLQuaternion orig_st_rot = LLQuaternion(orig_tep->getRotation(), LLVector3::z_axis) * orig_face_rot;
	LLQuaternion this_st_rot = orig_st_rot * ~this_face_rot;
	F32 x_ang, y_ang, z_ang;
	this_st_rot.getEulerAngles(&x_ang, &y_ang, &z_ang);
	*res_st_rot = z_ang;

	// Offset and scale of "this face's" texture:
	LLVector3 centers_dist = (this_pos - orig_pos) * ~orig_st_rot;
	LLVector3 st_scale(orig_tep->mScaleS, orig_tep->mScaleT, 1.f);
	st_scale *= orig_proj_scale;
	centers_dist.scaleVec(st_scale);
	LLVector2 orig_st_offset(orig_tep->mOffsetS, orig_tep->mOffsetT);

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
	if (LLPipeline::sNoAlpha)
	{
		return true;
	}

	const LLTextureEntry* te = getTextureEntry();
	if( !te || !getViewerObject() || !getTexture() )
	{
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
		if (LLPipeline::sRenderDeferred)
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
		else
		{
			return LLPipeline::sAutoMaskAlphaNonDeferred;
		}
	}

	return false;
}


static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_VOLUME("Volume VB Cache");

//static 
void LLFace::cacheFaceInVRAM(const LLVolumeFace& vf)
{
	LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_VOLUME);
	U32 mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 |
				LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_NORMAL;
	
	if (vf.mWeights)
	{
		mask |= LLVertexBuffer::MAP_WEIGHT4;
	}

	LLVertexBuffer* buff = new LLVertexBuffer(mask, GL_STATIC_DRAW_ARB);
	vf.mVertexBuffer = buff;

	buff->allocateBuffer(vf.mNumVertices, 0, true);

	LLStrider<LLVector4a> f_vert;
	LLStrider<LLVector4a> f_tangent;
	LLStrider<LLVector3> f_norm;
	LLStrider<LLVector2> f_tc;

	buff->getTangentStrider(f_tangent);
	buff->getVertexStrider(f_vert);
	buff->getNormalStrider(f_norm);
	buff->getTexCoord0Strider(f_tc);

	for (U32 i = 0; i < vf.mNumVertices; ++i)
	{
		*f_vert++ = vf.mPositions[i];
		*f_tangent++ = vf.mTangents[i];
		*f_tc++ = vf.mTexCoords[i];
		(*f_norm++).set(vf.mNormals[i].getF32ptr());
	}

	if (vf.mWeights)
	{
		LLStrider<LLVector4> f_wght;
		buff->getWeight4Strider(f_wght);
		for (U32 i = 0; i < vf.mNumVertices; ++i)
		{
			(*f_wght++).set(vf.mWeights[i].getF32ptr());
		}
	}

	buff->flush();
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

static LLTrace::BlockTimerStatHandle FTM_FACE_GET_GEOM("Face Geom");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_POSITION("Position");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_NORMAL("Normal");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_TEXTURE("Texture");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_COLOR("Color");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_EMISSIVE("Emissive");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_WEIGHTS("Weights");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_TANGENT("Binormal");

static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_FEEDBACK("Face Feedback");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_FEEDBACK_POSITION("Feedback Position");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_FEEDBACK_NORMAL("Feedback  Normal");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_FEEDBACK_TEXTURE("Feedback  Texture");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_FEEDBACK_COLOR("Feedback  Color");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_FEEDBACK_EMISSIVE("Feedback  Emissive");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_FEEDBACK_BINORMAL("Feedback Binormal");

static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_INDEX("Index");
static LLTrace::BlockTimerStatHandle FTM_FACE_GEOM_INDEX_TAIL("Tail");
static LLTrace::BlockTimerStatHandle FTM_FACE_POSITION_STORE("Pos");
static LLTrace::BlockTimerStatHandle FTM_FACE_TEXTURE_INDEX_STORE("TexIdx");
static LLTrace::BlockTimerStatHandle FTM_FACE_POSITION_PAD("Pad");
static LLTrace::BlockTimerStatHandle FTM_FACE_TEX_DEFAULT("Default");
static LLTrace::BlockTimerStatHandle FTM_FACE_TEX_QUICK("Quick");
static LLTrace::BlockTimerStatHandle FTM_FACE_TEX_QUICK_NO_XFORM("No Xform");
static LLTrace::BlockTimerStatHandle FTM_FACE_TEX_QUICK_XFORM("Xform");
static LLTrace::BlockTimerStatHandle FTM_FACE_TEX_QUICK_PLANAR("Quick Planar");

BOOL LLFace::getGeometryVolume(const LLVolume& volume,
							   const S32 &f,
								const LLMatrix4& mat_vert_in, const LLMatrix3& mat_norm_in,
								const U16 &index_offset,
								bool force_rebuild)
{
	LL_RECORD_BLOCK_TIME(FTM_FACE_GET_GEOM);
	llassert(verify());
	const LLVolumeFace &vf = volume.getVolumeFace(f);
	S32 num_vertices = (S32)vf.mNumVertices;
	S32 num_indices = (S32) vf.mNumIndices;
	
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCTREE))
	{
		updateRebuildFlags();
	}


	//don't use map range (generates many redundant unmap calls)
	bool map_range = false; //gGLManager.mHasMapBufferRange || gGLManager.mHasFlushBufferRange;

	if (mVertexBuffer.notNull())
	{
		if (num_indices + (S32) mIndicesIndex > mVertexBuffer->getNumIndices())
		{
			if (gDebugGL)
			{
				LL_WARNS()	<< "Index buffer overflow!" << LL_ENDL;
				LL_WARNS() << "Indices Count: " << mIndicesCount
						<< " VF Num Indices: " << num_indices
						<< " Indices Index: " << mIndicesIndex
						<< " VB Num Indices: " << mVertexBuffer->getNumIndices() << LL_ENDL;
				LL_WARNS()	<< " Face Index: " << f
						<< " Pool Type: " << mPoolType << LL_ENDL;
			}
			return FALSE;
		}

		if (num_vertices + mGeomIndex > mVertexBuffer->getNumVerts())
		{
			if (gDebugGL)
			{
				LL_WARNS() << "Vertex buffer overflow!" << LL_ENDL;
			}
			return FALSE;
		}
	}

	LLStrider<LLVector3> vert;
	LLStrider<LLVector2> tex_coords0;
	LLStrider<LLVector2> tex_coords1;
	LLStrider<LLVector2> tex_coords2;
	LLStrider<LLVector3> norm;
	LLStrider<LLColor4U> colors;
	LLStrider<LLVector3> tangent;
	LLStrider<U16> indicesp;
	LLStrider<LLVector4> wght;

	BOOL full_rebuild = force_rebuild || mDrawablep->isState(LLDrawable::REBUILD_VOLUME);
	
	BOOL global_volume = mDrawablep->getVOVolume()->isVolumeGlobal();
	LLVector3 scale;
	if (global_volume)
	{
		scale.setVec(1,1,1);
	}
	else
	{
		scale = mVObjp->getScale();
	}
	
	bool rebuild_pos = full_rebuild || mDrawablep->isState(LLDrawable::REBUILD_POSITION);
	bool rebuild_color = full_rebuild || mDrawablep->isState(LLDrawable::REBUILD_COLOR);
	bool rebuild_emissive = rebuild_color && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE);
	bool rebuild_tcoord = full_rebuild || mDrawablep->isState(LLDrawable::REBUILD_TCOORD);
	bool rebuild_normal = rebuild_pos && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_NORMAL);
	bool rebuild_tangent = rebuild_pos && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TANGENT);
	bool rebuild_weights = rebuild_pos && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_WEIGHT4);

	const LLTextureEntry *tep = mVObjp->getTE(f);
	const U8 bump_code = tep ? tep->getBumpmap() : 0;

	BOOL is_static = mDrawablep->isStatic();
	BOOL is_global = is_static;

	LLVector3 center_sum(0.f, 0.f, 0.f);
	
	if (is_global)
	{
		setState(GLOBAL);
	}
	else
	{
		clearState(GLOBAL);
	}

	LLColor4U color = tep->getColor();

	if (rebuild_color)
	{ //decide if shiny goes in alpha channel of color
		if (tep && 
			getPoolType() != LLDrawPool::POOL_ALPHA)  // <--- alpha channel MUST contain transparency, not shiny
	{
			LLMaterial* mat = tep->getMaterialParams().get();
						
			bool shiny_in_alpha = false;
			
			if (LLPipeline::sRenderDeferred)
			{ //store shiny in alpha if we don't have a specular map
				if  (!mat || mat->getSpecularID().isNull())
				{
					shiny_in_alpha = true;
				}
			}
			else
			{
				if (!mat || mat->getDiffuseAlphaMode() != LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
				{
					shiny_in_alpha = true;
				}
			}

			if (shiny_in_alpha)
			{

				static const GLfloat alpha[4] =
				{
					0.00f,
					0.25f,
					0.5f,
					0.75f
				};
			
				llassert(tep->getShiny() <= 3);
				color.mV[3] = U8 (alpha[tep->getShiny()] * 255);
			}
		}
	}

	// INDICES
	if (full_rebuild)
	{
		LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_INDEX);
		mVertexBuffer->getIndexStrider(indicesp, mIndicesIndex, mIndicesCount, map_range);

		volatile __m128i* dst = (__m128i*) indicesp.get();
		__m128i* src = (__m128i*) vf.mIndices;
		__m128i offset = _mm_set1_epi16(index_offset);

		S32 end = num_indices/8;
		
		for (S32 i = 0; i < end; i++)
		{
			__m128i res = _mm_add_epi16(src[i], offset);
			_mm_storeu_si128((__m128i*) dst++, res);
		}

		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_INDEX_TAIL);
			U16* idx = (U16*) dst;

			for (S32 i = end*8; i < num_indices; ++i)
			{
				*idx++ = vf.mIndices[i]+index_offset;
			}
		}

		if (map_range)
		{
			mVertexBuffer->flush();
		}
	}
	
	LLMatrix4a mat_normal;
	mat_normal.loadu(mat_norm_in);
	
	F32 r = 0, os = 0, ot = 0, ms = 0, mt = 0, cos_ang = 0, sin_ang = 0;
	bool do_xform = false;
	if (rebuild_tcoord)
	{
		if (tep)
		{
			r  = tep->getRotation();
			os = tep->mOffsetS;
			ot = tep->mOffsetT;
			ms = tep->mScaleS;
			mt = tep->mScaleT;
			cos_ang = cos(r);
			sin_ang = sin(r);

			if (cos_ang != 1.f || 
				sin_ang != 0.f ||
				os != 0.f ||
				ot != 0.f ||
				ms != 1.f ||
				mt != 1.f)
			{
				do_xform = true;
			}
			else
			{
				do_xform = false;
			}	
		}
		else
		{
			do_xform = false;
		}
	}
	
	static LLCachedControl<bool> use_transform_feedback(gSavedSettings, "RenderUseTransformFeedback", false);

#ifdef GL_TRANSFORM_FEEDBACK_BUFFER
	if (use_transform_feedback &&
		mVertexBuffer->getUsage() == GL_DYNAMIC_COPY_ARB &&
		gTransformPositionProgram.mProgramObject && //transform shaders are loaded
		mVertexBuffer->useVBOs() && //target buffer is in VRAM
		!rebuild_weights && //TODO: add support for weights
		!volume.isUnique()) //source volume is NOT flexi
	{ //use transform feedback to pack vertex buffer
		//gGLDebugLoggingEnabled = TRUE;
		LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_FEEDBACK);
		LLGLEnable discard(GL_RASTERIZER_DISCARD);
		LLVertexBuffer* buff = (LLVertexBuffer*) vf.mVertexBuffer.get();

		if (vf.mVertexBuffer.isNull() || buff->getNumVerts() != vf.mNumVertices)
		{
			mVObjp->getVolume()->genTangents(f);
			LLFace::cacheFaceInVRAM(vf);
			buff = (LLVertexBuffer*) vf.mVertexBuffer.get();
		}		

		LLGLSLShader* cur_shader = LLGLSLShader::sCurBoundShaderPtr;
		
		gGL.pushMatrix();
		gGL.loadMatrix((GLfloat*) mat_vert_in.mMatrix);

		if (rebuild_pos)
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_FEEDBACK_POSITION);
			gTransformPositionProgram.bind();

			mVertexBuffer->bindForFeedback(0, LLVertexBuffer::TYPE_VERTEX, mGeomIndex, mGeomCount);

			U8 index = mTextureIndex < 255 ? mTextureIndex : 0;

			S32 val = 0;
			U8* vp = (U8*) &val;
			vp[0] = index;
			vp[1] = 0;
			vp[2] = 0;
			vp[3] = 0;
			
			gTransformPositionProgram.uniform1i(sTextureIndexIn, val);
			glBeginTransformFeedback(GL_POINTS);
			buff->setBuffer(LLVertexBuffer::MAP_VERTEX);

			push_for_transform(buff, vf.mNumVertices, mGeomCount);

			glEndTransformFeedback();
		}

		if (rebuild_color)
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_FEEDBACK_COLOR);
			gTransformColorProgram.bind();
			
			mVertexBuffer->bindForFeedback(0, LLVertexBuffer::TYPE_COLOR, mGeomIndex, mGeomCount);

			S32 val = *((S32*) color.mV);

			gTransformColorProgram.uniform1i(sColorIn, val);
			glBeginTransformFeedback(GL_POINTS);
			buff->setBuffer(LLVertexBuffer::MAP_VERTEX);
			push_for_transform(buff, vf.mNumVertices, mGeomCount);
			glEndTransformFeedback();
		}

		if (rebuild_emissive)
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_FEEDBACK_EMISSIVE);
			gTransformColorProgram.bind();
			
			mVertexBuffer->bindForFeedback(0, LLVertexBuffer::TYPE_EMISSIVE, mGeomIndex, mGeomCount);

			U8 glow = (U8) llclamp((S32) (getTextureEntry()->getGlow()*255), 0, 255);

			S32 glow32 = glow |
						 (glow << 8) |
						 (glow << 16) |
						 (glow << 24);

			gTransformColorProgram.uniform1i(sColorIn, glow32);
			glBeginTransformFeedback(GL_POINTS);
			buff->setBuffer(LLVertexBuffer::MAP_VERTEX);
			push_for_transform(buff, vf.mNumVertices, mGeomCount);
			glEndTransformFeedback();
		}

		if (rebuild_normal)
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_FEEDBACK_NORMAL);
			gTransformNormalProgram.bind();
			
			mVertexBuffer->bindForFeedback(0, LLVertexBuffer::TYPE_NORMAL, mGeomIndex, mGeomCount);
						
			glBeginTransformFeedback(GL_POINTS);
			buff->setBuffer(LLVertexBuffer::MAP_NORMAL);
			push_for_transform(buff, vf.mNumVertices, mGeomCount);
			glEndTransformFeedback();
		}

		if (rebuild_tangent)
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_TANGENT);
			gTransformTangentProgram.bind();
			
			mVertexBuffer->bindForFeedback(0, LLVertexBuffer::TYPE_TANGENT, mGeomIndex, mGeomCount);
						
			glBeginTransformFeedback(GL_POINTS);
			buff->setBuffer(LLVertexBuffer::MAP_TANGENT);
			push_for_transform(buff, vf.mNumVertices, mGeomCount);
			glEndTransformFeedback();
		}

		if (rebuild_tcoord)
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_FEEDBACK_TEXTURE);
			gTransformTexCoordProgram.bind();
			
			mVertexBuffer->bindForFeedback(0, LLVertexBuffer::TYPE_TEXCOORD0, mGeomIndex, mGeomCount);
						
			glBeginTransformFeedback(GL_POINTS);
			buff->setBuffer(LLVertexBuffer::MAP_TEXCOORD0);
			push_for_transform(buff, vf.mNumVertices, mGeomCount);
			glEndTransformFeedback();

			bool do_bump = bump_code && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD1);

			if (do_bump)
			{
				mVertexBuffer->bindForFeedback(0, LLVertexBuffer::TYPE_TEXCOORD1, mGeomIndex, mGeomCount);
				glBeginTransformFeedback(GL_POINTS);
				buff->setBuffer(LLVertexBuffer::MAP_TEXCOORD0);
				push_for_transform(buff, vf.mNumVertices, mGeomCount);
				glEndTransformFeedback();
			}				
		}

		glBindBufferARB(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
		gGL.popMatrix();

		if (cur_shader)
		{
			cur_shader->bind();
		}
	}
	else
#endif
	{
		//if it's not fullbright and has no normals, bake sunlight based on face normal
		//bool bake_sunlight = !getTextureEntry()->getFullbright() &&
		//  !mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_NORMAL);

		if (rebuild_tcoord)
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_TEXTURE);
									
			//bump setup
			LLVector4a binormal_dir( -sin_ang, cos_ang, 0.f );
			LLVector4a bump_s_primary_light_ray(0.f, 0.f, 0.f);
			LLVector4a bump_t_primary_light_ray(0.f, 0.f, 0.f);

			LLQuaternion bump_quat;
			if (mDrawablep->isActive())
			{
				bump_quat = LLQuaternion(mDrawablep->getRenderMatrix());
			}
		
			if (bump_code)
			{
				mVObjp->getVolume()->genTangents(f);
				F32 offset_multiple; 
				switch( bump_code )
				{
					case BE_NO_BUMP:
					offset_multiple = 0.f;
					break;
					case BE_BRIGHTNESS:
					case BE_DARKNESS:
					if( mTexture[LLRender::DIFFUSE_MAP].notNull() && mTexture[LLRender::DIFFUSE_MAP]->hasGLTexture())
					{
						// Offset by approximately one texel
						S32 cur_discard = mTexture[LLRender::DIFFUSE_MAP]->getDiscardLevel();
						S32 max_size = llmax( mTexture[LLRender::DIFFUSE_MAP]->getWidth(), mTexture[LLRender::DIFFUSE_MAP]->getHeight() );
						max_size <<= cur_discard;
						const F32 ARTIFICIAL_OFFSET = 2.f;
						offset_multiple = ARTIFICIAL_OFFSET / (F32)max_size;
					}
					else
					{
						offset_multiple = 1.f/256;
					}
					break;

					default:  // Standard bumpmap textures.  Assumed to be 256x256
					offset_multiple = 1.f / 256;
					break;
				}

				F32 s_scale = 1.f;
				F32 t_scale = 1.f;
				if( tep )
				{
					tep->getScale( &s_scale, &t_scale );
				}
				// Use the nudged south when coming from above sun angle, such
				// that emboss mapping always shows up on the upward faces of cubes when 
				// it's noon (since a lot of builders build with the sun forced to noon).
				LLVector3   sun_ray  = gSky.mVOSkyp->mBumpSunDir;
				LLVector3   moon_ray = gSky.getMoonDirection();
				LLVector3& primary_light_ray = (sun_ray.mV[VZ] > 0) ? sun_ray : moon_ray;

				bump_s_primary_light_ray.load3((offset_multiple * s_scale * primary_light_ray).mV);
				bump_t_primary_light_ray.load3((offset_multiple * t_scale * primary_light_ray).mV);
			}

			U8 texgen = getTextureEntry()->getTexGen();
			if (rebuild_tcoord && texgen != LLTextureEntry::TEX_GEN_DEFAULT)
			{ //planar texgen needs binormals
				mVObjp->getVolume()->genTangents(f);
			}

			U8 tex_mode = 0;
	
			bool tex_anim = false;

				LLVOVolume* vobj = (LLVOVolume*) (LLViewerObject*) mVObjp;	
				tex_mode = vobj->mTexAnimMode;

			if (vobj->mTextureAnimp)
			{ //texture animation is in play, override specular and normal map tex coords with diffuse texcoords
				tex_anim = true;
			}

			if (isState(TEXTURE_ANIM))
			{
				if (!tex_mode)
				{
					clearState(TEXTURE_ANIM);
				}
				else
				{
					os = ot = 0.f;
					r = 0.f;
					cos_ang = 1.f;
					sin_ang = 0.f;
					ms = mt = 1.f;

					do_xform = false;
				}

				if (getVirtualSize() >= MIN_TEX_ANIM_SIZE || isState(LLFace::RIGGED))
				{ //don't override texture transform during tc bake
					tex_mode = 0;
				}
			}

			LLVector4a scalea;
			scalea.load3(scale.mV);

			LLMaterial* mat = tep->getMaterialParams().get();

			bool do_bump = bump_code && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD1);

			if (mat && !do_bump)
			{
				do_bump  = mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD1)
					     || mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD2);
			}
			
			bool do_tex_mat = tex_mode && mTextureMatrix;

			if (!do_bump)
			{ //not bump mapped, might be able to do a cheap update
				mVertexBuffer->getTexCoord0Strider(tex_coords0, mGeomIndex, mGeomCount);

				if (texgen != LLTextureEntry::TEX_GEN_PLANAR)
				{
					LL_RECORD_BLOCK_TIME(FTM_FACE_TEX_QUICK);
					if (!do_tex_mat)
					{
						if (!do_xform)
						{
							LL_RECORD_BLOCK_TIME(FTM_FACE_TEX_QUICK_NO_XFORM);
							S32 tc_size = (num_vertices*2*sizeof(F32)+0xF) & ~0xF;
							LLVector4a::memcpyNonAliased16((F32*) tex_coords0.get(), (F32*) vf.mTexCoords, tc_size);
						}
						else
						{
							LL_RECORD_BLOCK_TIME(FTM_FACE_TEX_QUICK_XFORM);
							F32* dst = (F32*) tex_coords0.get();
							LLVector4a* src = (LLVector4a*) vf.mTexCoords;

							LLVector4a trans;
							trans.splat(-0.5f);

							LLVector4a rot0;
							rot0.set(cos_ang, -sin_ang, cos_ang, -sin_ang);

							LLVector4a rot1;
							rot1.set(sin_ang, cos_ang, sin_ang, cos_ang);

							LLVector4a scale;
							scale.set(ms, mt, ms, mt);

							LLVector4a offset;
							offset.set(os+0.5f, ot+0.5f, os+0.5f, ot+0.5f);

							LLVector4Logical mask;
							mask.clear();
							mask.setElement<2>();
							mask.setElement<3>();

							U32 count = num_vertices/2 + num_vertices%2;

							for (S32 i = 0; i < count; i++)
							{	
								LLVector4a res = *src++;
								xform4a(res, trans, mask, rot0, rot1, offset, scale);
								res.store4a(dst);
								dst += 4;
							}
						}
					}
					else
					{ //do tex mat, no texgen, no bump
						for (S32 i = 0; i < num_vertices; i++)
						{	
							LLVector2 tc(vf.mTexCoords[i]);
							//LLVector4a& norm = vf.mNormals[i];
							//LLVector4a& center = *(vf.mCenter);

							LLVector3 tmp(tc.mV[0], tc.mV[1], 0.f);
							tmp = tmp * *mTextureMatrix;
							tc.mV[0] = tmp.mV[0];
							tc.mV[1] = tmp.mV[1];
							*tex_coords0++ = tc;	
						}
					}
				}
				else
				{ //no bump, tex gen planar
					LL_RECORD_BLOCK_TIME(FTM_FACE_TEX_QUICK_PLANAR);
					if (do_tex_mat)
					{
						for (S32 i = 0; i < num_vertices; i++)
						{	
							LLVector2 tc(vf.mTexCoords[i]);
							LLVector4a& norm = vf.mNormals[i];
							LLVector4a& center = *(vf.mCenter);
							LLVector4a vec = vf.mPositions[i];	
							vec.mul(scalea);
							planarProjection(tc, norm, center, vec);
						
							LLVector3 tmp(tc.mV[0], tc.mV[1], 0.f);
							tmp = tmp * *mTextureMatrix;
							tc.mV[0] = tmp.mV[0];
							tc.mV[1] = tmp.mV[1];
				
							*tex_coords0++ = tc;	
						}
					}
					else
					{
						for (S32 i = 0; i < num_vertices; i++)
						{	
							LLVector2 tc(vf.mTexCoords[i]);
							LLVector4a& norm = vf.mNormals[i];
							LLVector4a& center = *(vf.mCenter);
							LLVector4a vec = vf.mPositions[i];	
							vec.mul(scalea);
							planarProjection(tc, norm, center, vec);
						
							xform(tc, cos_ang, sin_ang, os, ot, ms, mt);

							*tex_coords0++ = tc;	
						}
					}
				}

				if (map_range)
				{
					mVertexBuffer->flush();
				}
			}
			else
			{ //bump mapped or has material, just do the whole expensive loop
				LL_RECORD_BLOCK_TIME(FTM_FACE_TEX_DEFAULT);

				std::vector<LLVector2> bump_tc;
		
				if (mat && !mat->getNormalID().isNull())
				{ //writing out normal and specular texture coordinates, not bump offsets
					do_bump = false;
				}

				LLStrider<LLVector2> dst;

				for (U32 ch = 0; ch < 3; ++ch)
				{
					switch (ch)
					{
						case 0: 
							mVertexBuffer->getTexCoord0Strider(dst, mGeomIndex, mGeomCount, map_range); 
							break;
						case 1:
							if (mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD1))
							{
								mVertexBuffer->getTexCoord1Strider(dst, mGeomIndex, mGeomCount, map_range);
								if (mat && !tex_anim)
								{
									r  = mat->getNormalRotation();
									mat->getNormalOffset(os, ot);
									mat->getNormalRepeat(ms, mt);

									cos_ang = cos(r);
									sin_ang = sin(r);

								}
							}
							else
							{
								continue;
							}
							break;
						case 2:
							if (mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD2))
							{
								mVertexBuffer->getTexCoord2Strider(dst, mGeomIndex, mGeomCount, map_range);
								if (mat && !tex_anim)
								{
									r  = mat->getSpecularRotation();
									mat->getSpecularOffset(os, ot);
									mat->getSpecularRepeat(ms, mt);

									cos_ang = cos(r);
									sin_ang = sin(r);
								}
							}
							else
							{
								continue;
							}
							break;
					}
					

				for (S32 i = 0; i < num_vertices; i++)
				{	
					LLVector2 tc(vf.mTexCoords[i]);
			
					LLVector4a& norm = vf.mNormals[i];
				
					LLVector4a& center = *(vf.mCenter);
		   
					if (texgen != LLTextureEntry::TEX_GEN_DEFAULT)
					{
						LLVector4a vec = vf.mPositions[i];
				
						vec.mul(scalea);

							if (texgen == LLTextureEntry::TEX_GEN_PLANAR)
						{
								planarProjection(tc, norm, center, vec);
						}		
					}

					if (tex_mode && mTextureMatrix)
					{
						LLVector3 tmp(tc.mV[0], tc.mV[1], 0.f);
						tmp = tmp * *mTextureMatrix;
						tc.mV[0] = tmp.mV[0];
						tc.mV[1] = tmp.mV[1];
					}
					else
					{
						xform(tc, cos_ang, sin_ang, os, ot, ms, mt);
					}

						*dst++ = tc;
					if (do_bump)
					{
						bump_tc.push_back(tc);
					}
				}
				}

				if (map_range)
				{
					mVertexBuffer->flush();
				}

				if (!mat && do_bump)
				{
					mVertexBuffer->getTexCoord1Strider(tex_coords1, mGeomIndex, mGeomCount, map_range);
		
					for (S32 i = 0; i < num_vertices; i++)
					{
						LLVector4a tangent = vf.mTangents[i];

						LLVector4a binorm;
						binorm.setCross3(vf.mNormals[i], tangent);
						binorm.mul(tangent.getF32ptr()[3]);

						LLMatrix4a tangent_to_object;
						tangent_to_object.setRows(tangent, binorm, vf.mNormals[i]);
						LLVector4a t;
						tangent_to_object.rotate(binormal_dir, t);
						LLVector4a binormal;
						mat_normal.rotate(t, binormal);
						
						//VECTORIZE THIS
						if (mDrawablep->isActive())
						{
							LLVector3 t;
							t.set(binormal.getF32ptr());
							t *= bump_quat;
							binormal.load3(t.mV);
						}

						binormal.normalize3fast();

						LLVector2 tc = bump_tc[i];
						tc += LLVector2( bump_s_primary_light_ray.dot3(tangent).getF32(), bump_t_primary_light_ray.dot3(binormal).getF32() );
					
						*tex_coords1++ = tc;
					}

					if (map_range)
					{
						mVertexBuffer->flush();
					}
				}
			}
		}

		if (rebuild_pos)
		{
			LLVector4a* src = vf.mPositions;
			
			//_mm_prefetch((char*)src, _MM_HINT_T0);

			LLVector4a* end = src+num_vertices;
			//LLVector4a* end_64 = end-4;

			//LL_RECORD_TIME_BLOCK(FTM_FACE_GEOM_POSITION);
			llassert(num_vertices > 0);
		
			mVertexBuffer->getVertexStrider(vert, mGeomIndex, mGeomCount, map_range);
			
			LLMatrix4a mat_vert;
			mat_vert.loadu(mat_vert_in);

			F32* dst = (F32*) vert.get();
			F32* end_f32 = dst+mGeomCount*4;

			//_mm_prefetch((char*)dst, _MM_HINT_NTA);
			//_mm_prefetch((char*)src, _MM_HINT_NTA);
				
			//_mm_prefetch((char*)dst, _MM_HINT_NTA);


			LLVector4a res0; //,res1,res2,res3;

			LLVector4a texIdx;

			S32 index = mTextureIndex < 255 ? mTextureIndex : 0;

			F32 val = 0.f;
			S32* vp = (S32*) &val;
			*vp = index;
			
			llassert(index <= LLGLSLShader::sIndexedTextureChannels-1);

			LLVector4Logical mask;
			mask.clear();
			mask.setElement<3>();
		
			texIdx.set(0,0,0,val);

			LLVector4a tmp;

			{
				//LL_RECORD_TIME_BLOCK(FTM_FACE_POSITION_STORE);

				/*if (num_vertices > 4)
				{ //more than 64 bytes
					while (src < end_64)
					{	
						_mm_prefetch((char*)src + 64, _MM_HINT_T0);
						_mm_prefetch((char*)dst + 64, _MM_HINT_T0);

						mat_vert.affineTransform(*src, res0);
						tmp.setSelectWithMask(mask, texIdx, res0);
						tmp.store4a((F32*) dst);

						mat_vert.affineTransform(*(src+1), res1);
						tmp.setSelectWithMask(mask, texIdx, res1);
						tmp.store4a((F32*) dst+4);

						mat_vert.affineTransform(*(src+2), res2);
						tmp.setSelectWithMask(mask, texIdx, res2);
						tmp.store4a((F32*) dst+8);

						mat_vert.affineTransform(*(src+3), res3);
						tmp.setSelectWithMask(mask, texIdx, res3);
						tmp.store4a((F32*) dst+12);

						dst += 16;
						src += 4;
					}
				}*/

				while (src < end)
				{	
					mat_vert.affineTransform(*src++, res0);
					tmp.setSelectWithMask(mask, texIdx, res0);
					tmp.store4a((F32*) dst);
					dst += 4;
				}
			}

			{
				//LL_RECORD_TIME_BLOCK(FTM_FACE_POSITION_PAD);
				while (dst < end_f32)
				{
					res0.store4a((F32*) dst);
					dst += 4;
				}
			}

			if (map_range)
			{
				mVertexBuffer->flush();
			}
		}

		
		if (rebuild_normal)
		{
			//LL_RECORD_TIME_BLOCK(FTM_FACE_GEOM_NORMAL);
			mVertexBuffer->getNormalStrider(norm, mGeomIndex, mGeomCount, map_range);
			F32* normals = (F32*) norm.get();
			LLVector4a* src = vf.mNormals;
			LLVector4a* end = src+num_vertices;
			
			while (src < end)
			{	
				LLVector4a normal;
				mat_normal.rotate(*src++, normal);
				normal.store4a(normals);
				normals += 4;
			}

			if (map_range)
			{
				mVertexBuffer->flush();
			}
		}
		
		if (rebuild_tangent)
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_TANGENT);
			mVertexBuffer->getTangentStrider(tangent, mGeomIndex, mGeomCount, map_range);
			F32* tangents = (F32*) tangent.get();
			
			mVObjp->getVolume()->genTangents(f);
			
			LLVector4Logical mask;
			mask.clear();
			mask.setElement<3>();

			LLVector4a* src = vf.mTangents;
			LLVector4a* end = vf.mTangents+num_vertices;

			while (src < end)
			{
				LLVector4a tangent_out;
				mat_normal.rotate(*src, tangent_out);
				tangent_out.normalize3fast();
				tangent_out.setSelectWithMask(mask, *src, tangent_out);
				tangent_out.store4a(tangents);
				
				src++;
				tangents += 4;
			}

			if (map_range)
			{
				mVertexBuffer->flush();
			}
		}
	
		if (rebuild_weights && vf.mWeights)
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_WEIGHTS);
			mVertexBuffer->getWeight4Strider(wght, mGeomIndex, mGeomCount, map_range);
			F32* weights = (F32*) wght.get();
			LLVector4a::memcpyNonAliased16(weights, (F32*) vf.mWeights, num_vertices*4*sizeof(F32));
			if (map_range)
			{
				mVertexBuffer->flush();
			}
		}

		if (rebuild_color && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_COLOR) )
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_COLOR);
			mVertexBuffer->getColorStrider(colors, mGeomIndex, mGeomCount, map_range);

			LLVector4a src;

			U32 vec[4];
			vec[0] = vec[1] = vec[2] = vec[3] = color.asRGBA();
		
			src.loadua((F32*) vec);

			F32* dst = (F32*) colors.get();
			S32 num_vecs = num_vertices/4;
			if (num_vertices%4 > 0)
			{
				++num_vecs;
			}

			for (S32 i = 0; i < num_vecs; i++)
			{	
				src.store4a(dst);
				dst += 4;
			}

			if (map_range)
			{
				mVertexBuffer->flush();
			}
		}

		if (rebuild_emissive)
		{
			LL_RECORD_BLOCK_TIME(FTM_FACE_GEOM_EMISSIVE);
			LLStrider<LLColor4U> emissive;
			mVertexBuffer->getEmissiveStrider(emissive, mGeomIndex, mGeomCount, map_range);

			U8 glow = (U8) llclamp((S32) (getTextureEntry()->getGlow()*255), 0, 255);

			LLVector4a src;

		
			LLColor4U glow4u = LLColor4U(0,0,0,glow);

			U32 glow32 = glow4u.asRGBA();

			U32 vec[4];
			vec[0] = vec[1] = vec[2] = vec[3] = glow32;
		
			src.loadua((F32*) vec);

			F32* dst = (F32*) emissive.get();
			S32 num_vecs = num_vertices/4;
			if (num_vertices%4 > 0)
			{
				++num_vecs;
			}

			for (S32 i = 0; i < num_vecs; i++)
			{	
				src.store4a(dst);
				dst += 4;
			}

			if (map_range)
			{
				mVertexBuffer->flush();
			}
		}
	}

	if (rebuild_tcoord)
	{
		mTexExtents[0].setVec(0,0);
		mTexExtents[1].setVec(1,1);
		xform(mTexExtents[0], cos_ang, sin_ang, os, ot, ms, mt);
		xform(mTexExtents[1], cos_ang, sin_ang, os, ot, ms, mt);
		
		F32 es = vf.mTexCoordExtents[1].mV[0] - vf.mTexCoordExtents[0].mV[0] ;
		F32 et = vf.mTexCoordExtents[1].mV[1] - vf.mTexCoordExtents[0].mV[1] ;
		mTexExtents[0][0] *= es ;
		mTexExtents[1][0] *= es ;
		mTexExtents[0][1] *= et ;
		mTexExtents[1][1] *= et ;
	}


	return TRUE;
}

//check if the face has a media
BOOL LLFace::hasMedia() const 
{
	if(mHasMedia)
	{
		return TRUE ;
	}
	if(mTexture[LLRender::DIFFUSE_MAP].notNull()) 
	{
		return mTexture[LLRender::DIFFUSE_MAP]->hasParcelMedia() ;  //if has a parcel media
	}

	return FALSE ; //no media.
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
	F32 radius;
	F32 cos_angle_to_view_dir;	
	BOOL in_frustum = calcPixelArea(cos_angle_to_view_dir, radius);

	if (mPixelArea < F_ALMOST_ZERO || !in_frustum)
	{
		setVirtualSize(0.f) ;
		return 0.f;
	}

	//get area of circle in texture space
	LLVector2 tdim = mTexExtents[1] - mTexExtents[0];
	F32 texel_area = (tdim * 0.5f).lengthSquared()*3.14159f;
	if (texel_area <= 0)
	{
		// Probably animated, use default
		texel_area = 1.f;
	}

	F32 face_area;
	if (mVObjp->isSculpted() && texel_area > 1.f)
	{
		//sculpts can break assumptions about texel area
		face_area = mPixelArea;
	}
	else
	{
		//apply texel area to face area to get accurate ratio
		//face_area /= llclamp(texel_area, 1.f/64.f, 16.f);
		face_area =  mPixelArea / llclamp(texel_area, 0.015625f, 128.f);
	}

	face_area = LLFace::adjustPixelArea(mImportanceToCamera, face_area) ;
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

BOOL LLFace::calcPixelArea(F32& cos_angle_to_view_dir, F32& radius)
{
	//VECTORIZE THIS
	//get area of circle around face
	LLVector4a center;
	center.load3(getPositionAgent().mV);
	LLVector4a size;
	size.setSub(mExtents[1], mExtents[0]);
	size.mul(0.5f);

	LLViewerCamera* camera = LLViewerCamera::getInstance();

	F32 size_squared = size.dot3(size).getF32();
	LLVector4a lookAt;
	LLVector4a t;
	t.load3(camera->getOrigin().mV);
	lookAt.setSub(center, t);
	
	F32 dist = lookAt.getLength3().getF32();
	dist = llmax(dist-size.getLength3().getF32(), 0.001f);
	//ramp down distance for nearby objects
	if (dist < 16.f)
	{
		dist /= 16.f;
		dist *= dist;
		dist *= 16.f;
	}

	lookAt.normalize3fast() ;	

	//get area of circle around node
	F32 app_angle = atanf((F32) sqrt(size_squared) / dist);
	radius = app_angle*LLDrawable::sCurPixelAngle;
	mPixelArea = radius*radius * 3.14159f;
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
		//	radius_square * acosf((d_square + radius_square - screen_radius_square)/(2 * d * radius)) +
		//	screen_radius_square * acosf((d_square + screen_radius_square - radius_square)/(2 * d * screen_radius)) -
		//	0.5f * sqrtf((-d + radius + screen_radius) * (d + radius - screen_radius) * (d - radius + screen_radius) * (d + radius + screen_radius)) ;			
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
				pixel_area = LLViewerTexture::sMinLargeImageSize ;
			}				
		}
	}

	return pixel_area ;
}

BOOL LLFace::verify(const U32* indices_array) const
{
	BOOL ok = TRUE;

	if( mVertexBuffer.isNull() )
	{ //no vertex buffer, face is implicitly valid
		return TRUE;
	}
	
	// First, check whether the face data fits within the pool's range.
	if ((mGeomIndex + mGeomCount) > mVertexBuffer->getNumVerts())
	{
		ok = FALSE;
		LL_INFOS() << "Face references invalid vertices!" << LL_ENDL;
	}

	S32 indices_count = (S32)getIndicesCount();
	
	if (!indices_count)
	{
		return TRUE;
	}
	
	if (indices_count > LL_MAX_INDICES_COUNT)
	{
		ok = FALSE;
		LL_INFOS() << "Face has bogus indices count" << LL_ENDL;
	}
	
	if (mIndicesIndex + mIndicesCount > mVertexBuffer->getNumIndices())
	{
		ok = FALSE;
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
			ok = FALSE;
		}
		else if (delta >= geom_count)
		{
			LL_WARNS() << "Face index too high!" << LL_ENDL;
			LL_INFOS() << "i:" << i << " Index:" << indicesp[i] << " GEnd: " << geom_start + geom_count << LL_ENDL;
			ok = FALSE;
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

const LLColor4& LLFace::getRenderColor() const
{
	if (isState(USE_FACE_COLOR))
	{
		  return mFaceColor; // Face Color
	}
	else
	{
		const LLTextureEntry* tep = getTextureEntry();
		return (tep ? tep->getColor() : LLColor4::white);
	}
}
	
void LLFace::renderSetColor() const
{
	if (!LLFacePool::LLOverrideFaceColor::sOverrideFaceColor)
	{
		const LLColor4* color = &(getRenderColor());
		
		gGL.diffuseColor4fv(color->mV);
	}
}

S32 LLFace::pushVertices(const U16* index_array) const
{
	if (mIndicesCount)
	{
		U32 render_type = LLRender::TRIANGLES;
		if (mDrawInfo)
		{
			render_type = mDrawInfo->mDrawMode;
		}
		mVertexBuffer->drawRange(render_type, mGeomIndex, mGeomIndex+mGeomCount-1, mIndicesCount, mIndicesIndex);
		gPipeline.addTrianglesDrawn(mIndicesCount, render_type);
	}

	return mIndicesCount;
}

const LLMatrix4& LLFace::getRenderMatrix() const
{
	return mDrawablep->getRenderMatrix();
}

S32 LLFace::renderElements(const U16 *index_array) const
{
	S32 ret = 0;
	
	if (isState(GLOBAL))
	{	
		ret = pushVertices(index_array);
	}
	else
	{
		gGL.pushMatrix();
		gGL.multMatrix((float*)getRenderMatrix().mMatrix);
		ret = pushVertices(index_array);
		gGL.popMatrix();
	}
	
	return ret;
}

S32 LLFace::renderIndexed()
{
	if(mDrawablep.isNull() || mDrawPoolp == NULL)
	{
		return 0;
	}
	
	return renderIndexed(mDrawPoolp->getVertexDataMask());
}

S32 LLFace::renderIndexed(U32 mask)
{
	if (mVertexBuffer.isNull())
	{
		return 0;
	}

	mVertexBuffer->setBuffer(mask);
	U16* index_array = (U16*) mVertexBuffer->getIndicesPointer();
	return renderElements(index_array);
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

S32	LLFace::getIndices(LLStrider<U16> &indicesp)
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
	mVertexBuffer = buffer;
	llassert(verify());
}

void LLFace::clearVertexBuffer()
{
	mVertexBuffer = NULL;
}

//static
U32 LLFace::getRiggedDataMask(U32 type)
{
	static const U32 rigged_data_mask[] = {
		LLDrawPoolAvatar::RIGGED_MATERIAL_MASK,
		LLDrawPoolAvatar::RIGGED_MATERIAL_ALPHA_VMASK,
		LLDrawPoolAvatar::RIGGED_MATERIAL_ALPHA_MASK_MASK,
		LLDrawPoolAvatar::RIGGED_MATERIAL_ALPHA_EMISSIVE_MASK,
		LLDrawPoolAvatar::RIGGED_SPECMAP_VMASK,
		LLDrawPoolAvatar::RIGGED_SPECMAP_BLEND_MASK,
		LLDrawPoolAvatar::RIGGED_SPECMAP_MASK_MASK,
		LLDrawPoolAvatar::RIGGED_SPECMAP_EMISSIVE_MASK,
		LLDrawPoolAvatar::RIGGED_NORMMAP_VMASK,
		LLDrawPoolAvatar::RIGGED_NORMMAP_BLEND_MASK,
		LLDrawPoolAvatar::RIGGED_NORMMAP_MASK_MASK,
		LLDrawPoolAvatar::RIGGED_NORMMAP_EMISSIVE_MASK,
		LLDrawPoolAvatar::RIGGED_NORMSPEC_VMASK,
		LLDrawPoolAvatar::RIGGED_NORMSPEC_BLEND_MASK,
		LLDrawPoolAvatar::RIGGED_NORMSPEC_MASK_MASK,
		LLDrawPoolAvatar::RIGGED_NORMSPEC_EMISSIVE_MASK,
		LLDrawPoolAvatar::RIGGED_SIMPLE_MASK,
		LLDrawPoolAvatar::RIGGED_FULLBRIGHT_MASK,
		LLDrawPoolAvatar::RIGGED_SHINY_MASK,
		LLDrawPoolAvatar::RIGGED_FULLBRIGHT_SHINY_MASK,
		LLDrawPoolAvatar::RIGGED_GLOW_MASK,
		LLDrawPoolAvatar::RIGGED_ALPHA_MASK,
		LLDrawPoolAvatar::RIGGED_FULLBRIGHT_ALPHA_MASK,
		LLDrawPoolAvatar::RIGGED_DEFERRED_BUMP_MASK,						 
		LLDrawPoolAvatar::RIGGED_DEFERRED_SIMPLE_MASK,
	};

	llassert(type < sizeof(rigged_data_mask)/sizeof(U32));

	return rigged_data_mask[type];
}

U32 LLFace::getRiggedVertexBufferDataMask() const
{
	U32 data_mask = 0;
	for (U32 i = 0; i < mRiggedIndex.size(); ++i)
	{
		if (mRiggedIndex[i] > -1)
		{
			data_mask |= LLFace::getRiggedDataMask(i);
		}
	}

	return data_mask;
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

void LLFace::setRiggedIndex(U32 type, S32 index)
{
	if (mRiggedIndex.empty())
	{
		mRiggedIndex.resize(LLDrawPoolAvatar::NUM_RIGGED_PASSES);
		for (U32 i = 0; i < mRiggedIndex.size(); ++i)
		{
			mRiggedIndex[i] = -1;
		}
	}

	llassert(type < mRiggedIndex.size());

	mRiggedIndex[type] = index;
}

