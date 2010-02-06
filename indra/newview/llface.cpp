/** 
 * @file llface.cpp
 * @brief LLFace class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "lldrawable.h" // lldrawable needs to be included before llface
#include "llface.h"
#include "llviewertextureanim.h"

#include "llviewercontrol.h"
#include "llvolume.h"
#include "m3math.h"
#include "v3color.h"

#include "lldrawpoolbump.h"
#include "llgl.h"
#include "llrender.h"
#include "lllightconstants.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llvosky.h"
#include "llvovolume.h"
#include "pipeline.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

#define LL_MAX_INDICES_COUNT 1000000

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
void planarProjection(LLVector2 &tc, const LLVector3& normal,
					  const LLVector3 &mCenter, const LLVector3& vec)
{	//DONE!
	LLVector3 binormal;
	float d = normal * LLVector3(1,0,0);
	if (d >= 0.5f || d <= -0.5f)
	{
		binormal = LLVector3(0,1,0);
		if (normal.mV[0] < 0)
		{
			binormal = -binormal;
		}
	}
	else
	{
        binormal = LLVector3(1,0,0);
		if (normal.mV[1] > 0)
		{
			binormal = -binormal;
		}
	}
	LLVector3 tangent = binormal % normal;

	tc.mV[1] = -((tangent*vec)*2 - 0.5f);
	tc.mV[0] = 1.0f+((binormal*vec)*2 - 0.5f);
}

void sphericalProjection(LLVector2 &tc, const LLVector3& normal,
						 const LLVector3 &mCenter, const LLVector3& vec)
{	//BROKEN
	/*tc.mV[0] = acosf(vd.mNormal * LLVector3(1,0,0))/3.14159f;
	
	tc.mV[1] = acosf(vd.mNormal * LLVector3(0,0,1))/6.284f;
	if (vd.mNormal.mV[1] > 0)
	{
		tc.mV[1] = 1.0f-tc.mV[1];
	}*/
}

void cylindricalProjection(LLVector2 &tc, const LLVector3& normal, const LLVector3 &mCenter, const LLVector3& vec)
{	//BROKEN
	/*LLVector3 binormal;
	float d = vd.mNormal * LLVector3(1,0,0);
	if (d >= 0.5f || d <= -0.5f)
	{
		binormal = LLVector3(0,1,0);
	}
	else{
		binormal = LLVector3(1,0,0);
	}
	LLVector3 tangent = binormal % vd.mNormal;

	tc.mV[1] = -((tangent*vec)*2 - 0.5f);

	tc.mV[0] = acosf(vd.mNormal * LLVector3(1,0,0))/6.284f;

	if (vd.mNormal.mV[1] < 0)
	{
		tc.mV[0] = 1.0f-tc.mV[0];
	}*/
}

////////////////////
//
// LLFace implementation
//

void LLFace::init(LLDrawable* drawablep, LLViewerObject* objp)
{
	mLastUpdateTime = gFrameTimeSeconds;
	mLastMoveTime = 0.f;
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
	mIndicesIndex	= 0;
	mIndexInTex = 0;
	mTexture		= NULL;
	mTEOffset		= -1;

	setDrawable(drawablep);
	mVObjp = objp;

	mReferenceIndex = -1;

	mTextureMatrix = NULL;
	mDrawInfo = NULL;

	mFaceColor = LLColor4(1,0,0,1);

	mLastVertexBuffer = mVertexBuffer;
	mLastGeomCount = mGeomCount;
	mLastGeomIndex = mGeomIndex;
	mLastIndicesCount = mIndicesCount;
	mLastIndicesIndex = mIndicesIndex;

	mImportanceToCamera = 0.f ;
	mBoundingSphereRadius = 0.0f ;

	mAtlasInfop = NULL ;
	mUsingAtlas  = FALSE ;
	mHasMedia = FALSE ;
}


void LLFace::destroy()
{
	if(mTexture.notNull())
	{
		mTexture->removeFace(this) ;
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
	
	removeAtlas();
	mDrawablep = NULL;
	mVObjp = NULL;
}


// static
void LLFace::initClass()
{
}

void LLFace::setWorldMatrix(const LLMatrix4 &mat)
{
	llerrs << "Faces on this drawable are not independently modifiable\n" << llendl;
}

void LLFace::setPool(LLFacePool* new_pool, LLViewerTexture *texturep)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (!new_pool)
	{
		llerrs << "Setting pool to null!" << llendl;
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

void LLFace::setTexture(LLViewerTexture* tex) 
{
	if(mTexture == tex)
	{
		return ;
	}

	if(mTexture.notNull())
	{
		mTexture->removeFace(this) ;
		removeAtlas() ;
	}	
	
	if(tex)
	{
		tex->addFace(this) ;
	}

	mTexture = tex ;
}

void LLFace::dirtyTexture()
{
	gPipeline.markTextured(getDrawable());
}

void LLFace::switchTexture(LLViewerTexture* new_texture)
{
	if(mTexture == new_texture)
	{
		return ;
	}

	if(!new_texture)
	{
		llerrs << "Can not switch to a null texture." << llendl;
		return;
	}
	new_texture->addTextureStats(mTexture->getMaxVirtualSize()) ;

	getViewerObject()->changeTEImage(mTEOffset, new_texture) ;
	setTexture(new_texture) ;	
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

void LLFace::setSize(const S32 num_vertices, const S32 num_indices)
{
	if (mGeomCount != num_vertices ||
		mIndicesCount != num_indices)
	{
		mGeomCount    = num_vertices;
		mIndicesCount = num_indices;
		mVertexBuffer = NULL;
		mLastVertexBuffer = NULL;
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
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);

	if (mVertexBuffer.notNull())
	{
		mVertexBuffer->getVertexStrider      (vertices, mGeomIndex);
		mVertexBuffer->getNormalStrider      (normals, mGeomIndex);
		mVertexBuffer->getTexCoord0Strider    (tex_coords, mGeomIndex);
		mVertexBuffer->getWeightStrider(vertex_weights, mGeomIndex);
		mVertexBuffer->getClothWeightStrider(clothing_weights, mGeomIndex);
	}

	return mGeomIndex;
}

U16 LLFace::getGeometry(LLStrider<LLVector3> &vertices, LLStrider<LLVector3> &normals,
					    LLStrider<LLVector2> &tex_coords, LLStrider<U16> &indicesp)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (mVertexBuffer.notNull())
	{
		mVertexBuffer->getVertexStrider(vertices,   mGeomIndex);
		if (mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_NORMAL))
		{
			mVertexBuffer->getNormalStrider(normals,    mGeomIndex);
		}
		if (mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD0))
		{
			mVertexBuffer->getTexCoord0Strider(tex_coords, mGeomIndex);
		}

		mVertexBuffer->getIndexStrider(indicesp, mIndicesIndex);
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

void LLFace::renderForSelect(U32 data_mask)
{
	if(mDrawablep.isNull() || mVertexBuffer.isNull())
	{
		return;
	}

	LLSpatialGroup* group = mDrawablep->getSpatialGroup();
	if (!group || group->isState(LLSpatialGroup::GEOM_DIRTY))
	{
		return;
	}

	if (mVObjp->mGLName)
	{
		S32 name = mVObjp->mGLName;

		LLColor4U color((U8)(name >> 16), (U8)(name >> 8), (U8)name);
#if 0 // *FIX: Postponing this fix until we have texcoord pick info...
		if (mTEOffset != -1)
		{
			color.mV[VALPHA] = (U8)(getTextureEntry()->getColor().mV[VALPHA] * 255.f);
		}
#endif
		glColor4ubv(color.mV);

		if (!getPool())
		{
			switch (getPoolType())
			{
			case LLDrawPool::POOL_ALPHA:
				gGL.getTexUnit(0)->bind(getTexture());
				break;
			default:
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				break;
			}
		}

		mVertexBuffer->setBuffer(data_mask);
#if !LL_RELEASE_FOR_DOWNLOAD
		LLGLState::checkClientArrays("", data_mask);
#endif
		if (mTEOffset != -1)
		{
			// mask off high 4 bits (16 total possible faces)
			color.mV[0] &= 0x0f;
			color.mV[0] |= (mTEOffset & 0x0f) << 4;
			glColor4ubv(color.mV);
		}

		if (mIndicesCount)
		{
			if (isState(GLOBAL))
			{
				if (mDrawablep->getVOVolume())
				{
					glPushMatrix();
					glMultMatrixf((float*) mDrawablep->getRegion()->mRenderMatrix.mMatrix);
					mVertexBuffer->draw(LLRender::TRIANGLES, mIndicesCount, mIndicesIndex);
					glPopMatrix();
				}
				else
				{
					mVertexBuffer->draw(LLRender::TRIANGLES, mIndicesCount, mIndicesIndex);
				}
			}
			else
			{
				glPushMatrix();
				glMultMatrixf((float*)getRenderMatrix().mMatrix);
				mVertexBuffer->draw(LLRender::TRIANGLES, mIndicesCount, mIndicesIndex);
				glPopMatrix();
			}
		}
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
			glMultMatrixf((GLfloat*)mDrawablep->getRenderMatrix().mMatrix);
		}
		else
		{
			glMultMatrixf((GLfloat*)mDrawablep->getRegion()->mRenderMatrix.mMatrix);
		}

		glColor4fv(color.mV);
		mVertexBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0);
		mVertexBuffer->draw(LLRender::TRIANGLES, mIndicesCount, mIndicesIndex);

		gGL.popMatrix();
	}
}


/* removed in lieu of raycast uv detection
void LLFace::renderSelectedUV()
{
	LLViewerTexture* red_blue_imagep = LLViewerTextureManager::getFetchedTextureFromFile("uv_test1.j2c", TRUE, LLViewerTexture::BOOST_UI);
	LLViewerTexture* green_imagep = LLViewerTextureManager::getFetchedTextureFromFile("uv_test2.tga", TRUE, LLViewerTexture::BOOST_UI);

	LLGLSUVSelect object_select;

	// use red/blue gradient to get coarse UV coordinates
	renderSelected(red_blue_imagep, LLColor4::white);
	
	static F32 bias = 0.f;
	static F32 factor = -10.f;
	glPolygonOffset(factor, bias);

	// add green dither pattern on top of red/blue gradient
	gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ONE);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	// make green pattern repeat once per texel in red/blue texture
	glScalef(256.f, 256.f, 1.f);
	glMatrixMode(GL_MODELVIEW);

	renderSelected(green_imagep, LLColor4::white);

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
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
	llinfos << "Object: " << getViewerObject()->mID << llendl;
	if (getDrawable())
	{
		llinfos << "Type: " << LLPrimitive::pCodeToString(getDrawable()->getVObj()->getPCode()) << llendl;
	}
	if (getTexture())
	{
		llinfos << "Texture: " << getTexture() << " Comps: " << (U32)getTexture()->getComponents() << llendl;
	}
	else
	{
		llinfos << "No texture: " << llendl;
	}

	llinfos << "Face: " << this << llendl;
	llinfos << "State: " << getState() << llendl;
	llinfos << "Geom Index Data:" << llendl;
	llinfos << "--------------------" << llendl;
	llinfos << "GI: " << mGeomIndex << " Count:" << mGeomCount << llendl;
	llinfos << "Face Index Data:" << llendl;
	llinfos << "--------------------" << llendl;
	llinfos << "II: " << mIndicesIndex << " Count:" << mIndicesCount << llendl;
	llinfos << llendl;

	poolp->printDebugInfo();

	S32 pool_references = 0;
	for (std::vector<LLFace*>::iterator iter = poolp->mReferences.begin();
		 iter != poolp->mReferences.end(); iter++)
	{
		LLFace *facep = *iter;
		if (facep == this)
		{
			llinfos << "Pool reference: " << pool_references << llendl;
			pool_references++;
		}
	}

	if (pool_references != 1)
	{
		llinfos << "Incorrect number of pool references!" << llendl;
	}

#if 0
	llinfos << "Indices:" << llendl;
	llinfos << "--------------------" << llendl;

	const U32 *indicesp = getRawIndices();
	S32 indices_count = getIndicesCount();
	S32 geom_start = getGeomStart();

	for (S32 i = 0; i < indices_count; i++)
	{
		llinfos << i << ":" << indicesp[i] << ":" << (S32)(indicesp[i] - geom_start) << llendl;
	}
	llinfos << llendl;

	llinfos << "Vertices:" << llendl;
	llinfos << "--------------------" << llendl;
	for (S32 i = 0; i < mGeomCount; i++)
	{
		llinfos << mGeomIndex + i << ":" << poolp->getVertex(mGeomIndex + i) << llendl;
	}
	llinfos << llendl;
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


BOOL LLFace::genVolumeBBoxes(const LLVolume &volume, S32 f,
								const LLMatrix4& mat_vert, const LLMatrix3& mat_normal, BOOL global_volume)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);

	//get bounding box
	if (mDrawablep->isState(LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION))
	{
		//if (mDrawablep->isState(LLDrawable::REBUILD_VOLUME))
		//{ //vertex buffer no longer valid
		//	mVertexBuffer = NULL;
		//	mLastVertexBuffer = NULL;
		//}

		LLVector3 min,max;
	
		if (f >= volume.getNumVolumeFaces())
		{
			min = LLVector3(-1,-1,-1);
			max = LLVector3(1,1,1);
		}
		else
		{
			const LLVolumeFace &face = volume.getVolumeFace(f);
			min = face.mExtents[0];
			max = face.mExtents[1];
		}

		//min, max are in volume space, convert to drawable render space
		LLVector3 center = ((min + max) * 0.5f)*mat_vert;
		LLVector3 size = ((max-min) * 0.5f);
		if (!global_volume)
		{
			size.scaleVec(mDrawablep->getVObj()->getScale());
		}

		LLMatrix3 mat = mat_normal;
		LLVector3 x = mat.getFwdRow();
		LLVector3 y = mat.getLeftRow();
		LLVector3 z = mat.getUpRow();
		x.normVec();
		y.normVec();
		z.normVec();

		mat.setRows(x,y,z);

		LLQuaternion rotation = LLQuaternion(mat);
		
		LLVector3 v[4];
		//get 4 corners of bounding box
		v[0] = (size * rotation);
		v[1] = (LLVector3(-size.mV[0], -size.mV[1], size.mV[2]) * rotation);
		v[2] = (LLVector3(size.mV[0], -size.mV[1], -size.mV[2]) * rotation);
		v[3] = (LLVector3(-size.mV[0], size.mV[1], -size.mV[2]) * rotation);

		LLVector3& newMin = mExtents[0];
		LLVector3& newMax = mExtents[1];
		
		newMin = newMax = center;
		
		for (U32 i = 0; i < 4; i++)
		{
			for (U32 j = 0; j < 3; j++)
			{
				F32 delta = fabsf(v[i].mV[j]);
				F32 min = center.mV[j] - delta;
				F32 max = center.mV[j] + delta;
				
				if (min < newMin.mV[j])
				{
					newMin.mV[j] = min;
				}
				
				if (max > newMax.mV[j])
				{
					newMax.mV[j] = max;
				}
			}
		}

		if (!mDrawablep->isActive())
		{
			LLVector3 offset = mDrawablep->getRegion()->getOriginAgent();
			newMin += offset;
			newMax += offset;
		}

		mCenterLocal = (newMin+newMax)*0.5f;
		LLVector3 tmp = (newMin - newMax) ;
		mBoundingSphereRadius = tmp.length() * 0.5f ;

		updateCenterAgent();
	}

	return TRUE;
}



// convert surface coordinates to texture coordinates, based on
// the values in the texture entry.  probably should be
// integrated with getGeometryVolume() for its texture coordinate
// generation - but i'll leave that to someone more familiar
// with the implications.
LLVector2 LLFace::surfaceToTexture(LLVector2 surface_coord, LLVector3 position, LLVector3 normal)
{
	LLVector2 tc = surface_coord;
	
	const LLTextureEntry *tep = getTextureEntry();

	if (tep == NULL)
	{
		// can't do much without the texture entry
		return surface_coord;
	}

	// see if we have a non-default mapping
    U8 texgen = getTextureEntry()->getTexGen();
	if (texgen != LLTextureEntry::TEX_GEN_DEFAULT)
	{
		LLVector3 center = mDrawablep->getVOVolume()->getVolume()->getVolumeFace(mTEOffset).mCenter;
		
		LLVector3 scale  = (mDrawablep->getVOVolume()->isVolumeGlobal()) ? LLVector3(1,1,1) : mVObjp->getScale();
		LLVector3 volume_position = mDrawablep->getVOVolume()->agentPositionToVolume(position);
		volume_position.scaleVec(scale);
		
		LLVector3 volume_normal   = mDrawablep->getVOVolume()->agentDirectionToVolume(normal);
		volume_normal.normalize();
		
		switch (texgen)
		{
		case LLTextureEntry::TEX_GEN_PLANAR:
			planarProjection(tc, volume_normal, center, volume_position);
			break;
		case LLTextureEntry::TEX_GEN_SPHERICAL:
			sphericalProjection(tc, volume_normal, center, volume_position);
			break;
		case LLTextureEntry::TEX_GEN_CYLINDRICAL:
			cylindricalProjection(tc, volume_normal, center, volume_position);
			break;
		default:
			break;
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

void LLFace::updateRebuildFlags()
{
	if (!mDrawablep->isState(LLDrawable::REBUILD_VOLUME))
	{
		BOOL moved = TRUE;
		if (mLastVertexBuffer == mVertexBuffer && 
			!mVertexBuffer->isEmpty())
		{	//this face really doesn't need to be regenerated, try real hard not to do so
			if (mLastGeomCount == mGeomCount &&
				mLastGeomIndex == mGeomIndex &&
				mLastIndicesCount == mIndicesCount &&
				mLastIndicesIndex == mIndicesIndex)
			{ //data is in same location in vertex buffer
				moved = FALSE;
			}
		}
		mLastMoveTime = gFrameTimeSeconds;
		
		if (moved)
		{
			mDrawablep->setState(LLDrawable::REBUILD_VOLUME);
		}
	}
	else
	{
		mLastUpdateTime = gFrameTimeSeconds;
	}
}

BOOL LLFace::getGeometryVolume(const LLVolume& volume,
							   const S32 &f,
								const LLMatrix4& mat_vert, const LLMatrix3& mat_normal,
								const U16 &index_offset)
{
	llpushcallstacks ;
	const LLVolumeFace &vf = volume.getVolumeFace(f);
	S32 num_vertices = (S32)vf.mVertices.size();
	S32 num_indices = LLPipeline::sUseTriStrips ? (S32)vf.mTriStrip.size() : (S32) vf.mIndices.size();
	
	if (mVertexBuffer.notNull())
	{
		if (num_indices + (S32) mIndicesIndex > mVertexBuffer->getNumIndices())
		{
			llwarns	<< "Index buffer overflow!" << llendl;
			llwarns << "Indices Count: " << mIndicesCount
					<< " VF Num Indices: " << num_indices
					<< " Indices Index: " << mIndicesIndex
					<< " VB Num Indices: " << mVertexBuffer->getNumIndices() << llendl;
			llwarns	<< "Last Indices Count: " << mLastIndicesCount
					<< " Last Indices Index: " << mLastIndicesIndex
					<< " Face Index: " << f
					<< " Pool Type: " << mPoolType << llendl;
			return FALSE;
		}

		if (num_vertices + mGeomIndex > mVertexBuffer->getNumVerts())
		{
			llwarns << "Vertex buffer overflow!" << llendl;
			return FALSE;
		}
	}

	LLStrider<LLVector3> vertices;
	LLStrider<LLVector2> tex_coords;
	LLStrider<LLVector2> tex_coords2;
	LLStrider<LLVector3> normals;
	LLStrider<LLColor4U> colors;
	LLStrider<LLVector3> binormals;
	LLStrider<U16> indicesp;

	BOOL full_rebuild = mDrawablep->isState(LLDrawable::REBUILD_VOLUME);
	
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
	
	BOOL rebuild_pos = full_rebuild || mDrawablep->isState(LLDrawable::REBUILD_POSITION);
	BOOL rebuild_color = full_rebuild || mDrawablep->isState(LLDrawable::REBUILD_COLOR);
	BOOL rebuild_tcoord = full_rebuild || mDrawablep->isState(LLDrawable::REBUILD_TCOORD);
	BOOL rebuild_normal = rebuild_pos && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_NORMAL);
	BOOL rebuild_binormal = rebuild_pos && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_BINORMAL);

	const LLTextureEntry *tep = mVObjp->getTE(f);
	U8  bump_code = tep ? tep->getBumpmap() : 0;

	if (rebuild_pos)
	{
		mVertexBuffer->getVertexStrider(vertices, mGeomIndex);
	}
	if (rebuild_normal)
	{
		mVertexBuffer->getNormalStrider(normals, mGeomIndex);
	}
	if (rebuild_binormal)
	{
		mVertexBuffer->getBinormalStrider(binormals, mGeomIndex);
	}

	F32 tcoord_xoffset = 0.f ;
	F32 tcoord_yoffset = 0.f ;
	F32 tcoord_xscale = 1.f ;
	F32 tcoord_yscale = 1.f ;
	BOOL in_atlas = FALSE ;

	if (rebuild_tcoord)
	{
		mVertexBuffer->getTexCoord0Strider(tex_coords, mGeomIndex);
		if (bump_code && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD1))
		{
			mVertexBuffer->getTexCoord1Strider(tex_coords2, mGeomIndex);
		}

		in_atlas = isAtlasInUse() ;
		if(in_atlas)
		{
			const LLVector2* tmp = getTexCoordOffset() ;
			tcoord_xoffset = tmp->mV[0] ; 
			tcoord_yoffset = tmp->mV[1] ;

			tmp = getTexCoordScale() ;
			tcoord_xscale = tmp->mV[0] ; 
			tcoord_yscale = tmp->mV[1] ;	
		}
	}
	if (rebuild_color)
	{	
		mVertexBuffer->getColorStrider(colors, mGeomIndex);
	}

	F32 r = 0, os = 0, ot = 0, ms = 0, mt = 0, cos_ang = 0, sin_ang = 0;
	
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

	LLVector2 tmin, tmax;
	
	

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
		}
		else
		{
			cos_ang = 1.0f;
			sin_ang = 0.0f;
			os = 0.0f;
			ot = 0.0f;
			ms = 1.0f;
			mt = 1.0f;
		}
	}

	U8 tex_mode = 0;
	
	if (isState(TEXTURE_ANIM))
	{
		LLVOVolume* vobj = (LLVOVolume*) (LLViewerObject*) mVObjp;	
		tex_mode = vobj->mTexAnimMode;

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
		}

		if (getVirtualSize() >= MIN_TEX_ANIM_SIZE)
		{ //don't override texture transform during tc bake
			tex_mode = 0;
		}
	}

	LLColor4U color = tep->getColor();

	if (rebuild_color)
	{
		if (tep)
		{
			GLfloat alpha[4] =
			{
				0.00f,
				0.25f,
				0.5f,
				0.75f
			};
			
			if (getPoolType() != LLDrawPool::POOL_ALPHA && (LLPipeline::sRenderDeferred || (LLPipeline::sRenderBump && tep->getShiny())))
			{
				color.mV[3] = U8 (alpha[tep->getShiny()] * 255);
			}
		}
	}

    // INDICES
	if (full_rebuild)
	{
		mVertexBuffer->getIndexStrider(indicesp, mIndicesIndex);
		if (LLPipeline::sUseTriStrips)
		{
			for (U16 i = 0; i < num_indices; i++)
			{
				*indicesp++ = vf.mTriStrip[i] + index_offset;
			}
		}
		else
		{
			for (U16 i = 0; i < num_indices; i++)
			{
				*indicesp++ = vf.mIndices[i] + index_offset;
			}
		}
	}
	
	
	//bump setup
	LLVector3 binormal_dir( -sin_ang, cos_ang, 0 );
	LLVector3 bump_s_primary_light_ray;
	LLVector3 bump_t_primary_light_ray;

	LLQuaternion bump_quat;
	if (mDrawablep->isActive())
	{
		bump_quat = LLQuaternion(mDrawablep->getRenderMatrix());
	}
	
	if (bump_code)
	{
		mVObjp->getVolume()->genBinormals(f);
		F32 offset_multiple; 
		switch( bump_code )
		{
			case BE_NO_BUMP:
			offset_multiple = 0.f;
			break;
			case BE_BRIGHTNESS:
			case BE_DARKNESS:
			if( mTexture.notNull() && mTexture->hasGLTexture())
			{
				// Offset by approximately one texel
				S32 cur_discard = mTexture->getDiscardLevel();
				S32 max_size = llmax( mTexture->getWidth(), mTexture->getHeight() );
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

		bump_s_primary_light_ray = offset_multiple * s_scale * primary_light_ray;
		bump_t_primary_light_ray = offset_multiple * t_scale * primary_light_ray;
	}
		
	U8 texgen = getTextureEntry()->getTexGen();
	if (rebuild_tcoord && texgen != LLTextureEntry::TEX_GEN_DEFAULT)
	{ //planar texgen needs binormals
		mVObjp->getVolume()->genBinormals(f);
	}

	for (S32 i = 0; i < num_vertices; i++)
	{
		if (rebuild_tcoord)
		{
			LLVector2 tc = vf.mVertices[i].mTexCoord;
		
			if (texgen != LLTextureEntry::TEX_GEN_DEFAULT)
			{
				LLVector3 vec = vf.mVertices[i].mPosition; 
			
				vec.scaleVec(scale);

				switch (texgen)
				{
					case LLTextureEntry::TEX_GEN_PLANAR:
						planarProjection(tc, vf.mVertices[i].mNormal, vf.mCenter, vec);
						break;
					case LLTextureEntry::TEX_GEN_SPHERICAL:
						sphericalProjection(tc, vf.mVertices[i].mNormal, vf.mCenter, vec);
						break;
					case LLTextureEntry::TEX_GEN_CYLINDRICAL:
						cylindricalProjection(tc, vf.mVertices[i].mNormal, vf.mCenter, vec);
						break;
					default:
						break;
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

			if(in_atlas)
			{
				//
				//manually calculate tex-coord per vertex for varying address modes.
				//should be removed if shader can handle this.
				//

				S32 int_part = 0 ;
				switch(mTexture->getAddressMode())
				{
				case LLTexUnit::TAM_CLAMP:
					if(tc.mV[0] < 0.f)
					{
						tc.mV[0] = 0.f ;
					}
					else if(tc.mV[0] > 1.f)
					{
						tc.mV[0] = 1.f;
					}

					if(tc.mV[1] < 0.f)
					{
						tc.mV[1] = 0.f ;
					}
					else if(tc.mV[1] > 1.f)
					{
						tc.mV[1] = 1.f;
					}
					break;
				case LLTexUnit::TAM_MIRROR:
					if(tc.mV[0] < 0.f)
					{
						tc.mV[0] = -tc.mV[0] ;
					}
					int_part = (S32)tc.mV[0] ;
					if(int_part & 1) //odd number
					{
						tc.mV[0] = int_part + 1 - tc.mV[0] ;
					}
					else //even number
					{
						tc.mV[0] -= int_part ;
					}

					if(tc.mV[1] < 0.f)
					{
						tc.mV[1] = -tc.mV[1] ;
					}
					int_part = (S32)tc.mV[1] ;
					if(int_part & 1) //odd number
					{
						tc.mV[1] = int_part + 1 - tc.mV[1] ;
					}
					else //even number
					{
						tc.mV[1] -= int_part ;
					}
					break;
				case LLTexUnit::TAM_WRAP:
					if(tc.mV[0] > 1.f)
						tc.mV[0] -= (S32)(tc.mV[0] - 0.00001f) ;
					else if(tc.mV[0] < -1.f)
						tc.mV[0] -= (S32)(tc.mV[0] + 0.00001f) ;

					if(tc.mV[1] > 1.f)
						tc.mV[1] -= (S32)(tc.mV[1] - 0.00001f) ;
					else if(tc.mV[1] < -1.f)
						tc.mV[1] -= (S32)(tc.mV[1] + 0.00001f) ;

					if(tc.mV[0] < 0.f)
					{
						tc.mV[0] = 1.0f + tc.mV[0] ;
					}
					if(tc.mV[1] < 0.f)
					{
						tc.mV[1] = 1.0f + tc.mV[1] ;
					}
					break;
				default:
					break;
				}
			
				tc.mV[0] = tcoord_xoffset + tcoord_xscale * tc.mV[0] ;
				tc.mV[1] = tcoord_yoffset + tcoord_yscale * tc.mV[1] ;
			}
			

			*tex_coords++ = tc;
		
			if (bump_code && mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD1))
			{
				LLVector3 tangent = vf.mVertices[i].mBinormal % vf.mVertices[i].mNormal;

				LLMatrix3 tangent_to_object;
				tangent_to_object.setRows(tangent, vf.mVertices[i].mBinormal, vf.mVertices[i].mNormal);
				LLVector3 binormal = binormal_dir * tangent_to_object;
				binormal = binormal * mat_normal;
				
				if (mDrawablep->isActive())
				{
					binormal *= bump_quat;
				}

				binormal.normVec();
				tc += LLVector2( bump_s_primary_light_ray * tangent, bump_t_primary_light_ray * binormal );
				
				*tex_coords2++ = tc;
			}	
		}
			
		if (rebuild_pos)
		{
			*vertices++ = vf.mVertices[i].mPosition * mat_vert;
		}
		
		if (rebuild_normal)
		{
			LLVector3 normal = vf.mVertices[i].mNormal * mat_normal;
			normal.normVec();
			
			*normals++ = normal;
		}
		
		if (rebuild_binormal)
		{
			LLVector3 binormal = vf.mVertices[i].mBinormal * mat_normal;
			binormal.normVec();
			*binormals++ = binormal;
		}
		
		if (rebuild_color)
		{
			*colors++ = color;		
		}
	}

	if (rebuild_tcoord)
	{
		mTexExtents[0].setVec(0,0);
		mTexExtents[1].setVec(1,1);
		xform(mTexExtents[0], cos_ang, sin_ang, os, ot, ms, mt);
		xform(mTexExtents[1], cos_ang, sin_ang, os, ot, ms, mt);		
	}

	mLastVertexBuffer = mVertexBuffer;
	mLastGeomCount = mGeomCount;
	mLastGeomIndex = mGeomIndex;
	mLastIndicesCount = mIndicesCount;
	mLastIndicesIndex = mIndicesIndex;

	return TRUE;
}

//check if the face has a media
BOOL LLFace::hasMedia() const 
{
	if(mHasMedia)
	{
		return TRUE ;
	}
	if(mTexture.notNull()) 
	{
		return mTexture->hasParcelMedia() ;  //if has a parcel media
	}

	return FALSE ; //no media.
}

const F32 LEAST_IMPORTANCE = 0.05f ;
const F32 LEAST_IMPORTANCE_FOR_LARGE_IMAGE = 0.3f ;

F32 LLFace::getTextureVirtualSize()
{
	F32 radius;
	F32 cos_angle_to_view_dir;	
	BOOL in_frustum = calcPixelArea(cos_angle_to_view_dir, radius);

	if (mPixelArea < 0.0001f || !in_frustum)
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

	//apply texel area to face area to get accurate ratio
	//face_area /= llclamp(texel_area, 1.f/64.f, 16.f);
	F32 face_area = mPixelArea / llclamp(texel_area, 0.015625f, 128.f);

	if(face_area > LLViewerTexture::sMaxSmallImageSize)
	{
		if(mImportanceToCamera < LEAST_IMPORTANCE) //if the face is not important, do not load hi-res.
		{
			static const F32 MAX_LEAST_IMPORTANCE_IMAGE_SIZE = 128.0f * 128.0f ;
			face_area = llmin(face_area * 0.5f, MAX_LEAST_IMPORTANCE_IMAGE_SIZE) ;
		}
		else if(face_area > LLViewerTexture::sMinLargeImageSize) //if is large image, shrink face_area by considering the partial overlapping.
		{
			if(mImportanceToCamera < LEAST_IMPORTANCE_FOR_LARGE_IMAGE)//if the face is not important, do not load hi-res.
			{
				face_area = LLViewerTexture::sMinLargeImageSize ;
			}	
			else if(mTexture.notNull() && mTexture->isLargeImage())
			{		
				face_area *= adjustPartialOverlapPixelArea(cos_angle_to_view_dir, radius );
			}			
		}
	}

	setVirtualSize(face_area) ;

	return face_area;
}

BOOL LLFace::calcPixelArea(F32& cos_angle_to_view_dir, F32& radius)
{
	//get area of circle around face
	LLVector3 center = getPositionAgent();
	LLVector3 size = (mExtents[1] - mExtents[0]) * 0.5f;	
	LLViewerCamera* camera = LLViewerCamera::getInstance();

	F32 size_squared = size.lengthSquared() ;
	LLVector3 lookAt = center - camera->getOrigin();
	F32 dist = lookAt.normVec() ;	

	//get area of circle around node
	F32 app_angle = atanf(fsqrtf(size_squared) / dist);
	radius = app_angle*LLDrawable::sCurPixelAngle;
	mPixelArea = radius*radius * 3.14159f;
	cos_angle_to_view_dir = lookAt * camera->getXAxis() ;

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
			if(dist * dist * (lookAt - camera->getXAxis()).lengthSquared() < size_squared)
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
		
		//F32 camera_relative_speed = camera_moving_speed * (lookAt * LLViewerCamera::getInstance()->getVelocityDir()) ;
		
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

BOOL LLFace::verify(const U32* indices_array) const
{
	BOOL ok = TRUE;

	if( mVertexBuffer.isNull() )
	{
		if( mGeomCount )
		{
			// This happens before teleports as faces are torn down.
			// Stop the crash in DEV-31893 with a null pointer check,
			// but present this info.
			// To clean up the log, the geometry could be cleared, or the
			// face could otherwise be marked for no ::verify.
			llinfos << "Face with no vertex buffer and " << mGeomCount << " mGeomCount" << llendl;
		}
		return TRUE;
	}
	
	// First, check whether the face data fits within the pool's range.
	if ((mGeomIndex + mGeomCount) > mVertexBuffer->getNumVerts())
	{
		ok = FALSE;
		llinfos << "Face not within pool range!" << llendl;
	}

	S32 indices_count = (S32)getIndicesCount();
	
	if (!indices_count)
	{
		return TRUE;
	}
	
	if (indices_count > LL_MAX_INDICES_COUNT)
	{
		ok = FALSE;
		llinfos << "Face has bogus indices count" << llendl;
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
			llwarns << "Face index too low!" << llendl;
			llinfos << "i:" << i << " Index:" << indicesp[i] << " GStart: " << geom_start << llendl;
			ok = FALSE;
		}
		else if (delta >= geom_count)
		{
			llwarns << "Face index too high!" << llendl;
			llinfos << "i:" << i << " Index:" << indicesp[i] << " GEnd: " << geom_start + geom_count << llendl;
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
		
		glColor4fv(color->mV);
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
		glPushMatrix();
		glMultMatrixf((float*)getRenderMatrix().mMatrix);
		ret = pushVertices(index_array);
		glPopMatrix();
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
	mVertexBuffer->getColorStrider(colors, mGeomIndex);
	return mGeomIndex;
}

S32	LLFace::getIndices(LLStrider<U16> &indicesp)
{
	mVertexBuffer->getIndexStrider(indicesp, mIndicesIndex);
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

//
//atlas
//
void LLFace::removeAtlas()
{
	setAtlasInUse(FALSE) ;
	mAtlasInfop = NULL ;	
}

const LLTextureAtlas* LLFace::getAtlas()const 
{
	if(mAtlasInfop)
	{
		return mAtlasInfop->getAtlas() ;
	}
	return NULL ;
}

const LLVector2* LLFace::getTexCoordOffset()const 
{
	if(isAtlasInUse())
	{
		return mAtlasInfop->getTexCoordOffset() ;
	}
	return NULL ;
}
const LLVector2* LLFace::getTexCoordScale() const 
{
	if(isAtlasInUse())
	{
		return mAtlasInfop->getTexCoordScale() ;
	}
	return NULL ;
}

BOOL LLFace::isAtlasInUse()const
{
	return mUsingAtlas ;
}

BOOL LLFace::canUseAtlas()const
{
	//no drawable or no spatial group, do not use atlas
	if(!mDrawablep || !mDrawablep->getSpatialGroup())
	{
		return FALSE ;
	}

	//if bump face, do not use atlas
	if(getTextureEntry() && getTextureEntry()->getBumpmap())
	{
		return FALSE ;
	}

	//if animated texture, do not use atlas
	if(isState(TEXTURE_ANIM))
	{
		return FALSE ;
	}

	return TRUE ;
}

void LLFace::setAtlasInUse(BOOL flag)
{
	//no valid atlas to use.
	if(flag && (!mAtlasInfop || !mAtlasInfop->isValid()))
	{
		flag = FALSE ;
	}

	if(!flag && !mUsingAtlas)
	{
		return ;
	}

	//
	//at this stage (flag || mUsingAtlas) is always true.
	//

	//rebuild the tex coords
	if(mDrawablep)
	{
		gPipeline.markRebuild(mDrawablep, LLDrawable::REBUILD_TCOORD);
		mUsingAtlas = flag ;
	}
	else
	{
		mUsingAtlas = FALSE ;
	}
}

LLTextureAtlasSlot* LLFace::getAtlasInfo()
{
	return mAtlasInfop ;
}

void LLFace::setAtlasInfo(LLTextureAtlasSlot* atlasp)
{	
	if(mAtlasInfop != atlasp)
	{
		if(mAtlasInfop)
		{
			//llerrs << "Atlas slot changed!" << llendl ;
		}
		mAtlasInfop = atlasp ;
	}
}

LLViewerTexture* LLFace::getTexture() const
{
	if(isAtlasInUse())
	{
		return (LLViewerTexture*)mAtlasInfop->getAtlas() ;
	}

	return mTexture ;
}

//switch to atlas or switch back to gl texture 
//return TRUE if using atlas.
BOOL LLFace::switchTexture()
{
	//no valid atlas or texture
	if(!mAtlasInfop || !mAtlasInfop->isValid() || !mTexture)
	{
		return FALSE ;
	}
	
	if(mTexture->getTexelsInAtlas() >= (U32)mVSize || 
		mTexture->getTexelsInAtlas() >= mTexture->getTexelsInGLTexture())
	{
		//switch to use atlas
		//atlas resolution is qualified, use it.		
		if(!mUsingAtlas)
		{
			setAtlasInUse(TRUE) ;
		}
	}
	else //if atlas not qualified.
	{
		//switch back to GL texture
		if(mUsingAtlas && mTexture->isGLTextureCreated() && 
			mTexture->getDiscardLevel() < mTexture->getDiscardLevelInAtlas())
		{
			setAtlasInUse(FALSE) ;
		}
	}

	return mUsingAtlas ;
}

