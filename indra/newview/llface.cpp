/** 
 * @file llface.cpp
 * @brief LLFace class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llglimmediate.h"
#include "lllightconstants.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llvosky.h"
#include "llvovolume.h"
#include "pipeline.h"
#include "llviewerregion.h"

#define LL_MAX_INDICES_COUNT 1000000

extern BOOL gPickFaces;

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
void planarProjection(LLVector2 &tc, const LLVolumeFace::VertexData &vd, const LLVector3 &mCenter, const LLVector3& vec)
{	//DONE!
	LLVector3 binormal;
	float d = vd.mNormal * LLVector3(1,0,0);
	if (d >= 0.5f || d <= -0.5f)
	{
		binormal = LLVector3(0,1,0);
		if (vd.mNormal.mV[0] < 0)
		{
			binormal = -binormal;
		}
	}
	else
	{
        binormal = LLVector3(1,0,0);
		if (vd.mNormal.mV[1] > 0)
		{
			binormal = -binormal;
		}
	}
	LLVector3 tangent = binormal % vd.mNormal;

	tc.mV[1] = -((tangent*vec)*2 - 0.5f);
	tc.mV[0] = 1.0f+((binormal*vec)*2 - 0.5f);
}

void sphericalProjection(LLVector2 &tc, const LLVolumeFace::VertexData &vd, const LLVector3 &mCenter, const LLVector3& vec)
{	//BROKEN
	/*tc.mV[0] = acosf(vd.mNormal * LLVector3(1,0,0))/3.14159f;
	
	tc.mV[1] = acosf(vd.mNormal * LLVector3(0,0,1))/6.284f;
	if (vd.mNormal.mV[1] > 0)
	{
		tc.mV[1] = 1.0f-tc.mV[1];
	}*/
}

void cylindricalProjection(LLVector2 &tc, const LLVolumeFace::VertexData &vd, const LLVector3 &mCenter, const LLVector3& vec)
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
	mTexture		= NULL;
	mTEOffset		= -1;

	setDrawable(drawablep);
	mVObjp = objp;

	mReferenceIndex = -1;

	mTextureMatrix = NULL;

	mFaceColor = LLColor4(1,0,0,1);

	mLastVertexBuffer = mVertexBuffer;
	mLastGeomCount = mGeomCount;
	mLastGeomIndex = mGeomIndex;
	mLastIndicesCount = mIndicesCount;
	mLastIndicesIndex = mIndicesIndex;
}


void LLFace::destroy()
{
	mDrawablep = NULL;
	mVObjp = NULL;

	if (mDrawPoolp)
	{
		mDrawPoolp->removeFace(this);
		mDrawPoolp = NULL;
	}

	if (mTextureMatrix)
	{
		delete mTextureMatrix;
		mTextureMatrix = NULL;
	}
}


// static
void LLFace::initClass()
{
}

void LLFace::setWorldMatrix(const LLMatrix4 &mat)
{
	llerrs << "Faces on this drawable are not independently modifiable\n" << llendl;
}

void LLFace::setPool(LLFacePool* new_pool, LLViewerImage *texturep)
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
	mTexture = texturep;
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
	mGeomCount    = num_vertices;
	mIndicesCount = num_indices;
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
		mVertexBuffer->getTexCoordStrider    (tex_coords, mGeomIndex);
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
		if (mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_TEXCOORD))
		{
			mVertexBuffer->getTexCoordStrider(tex_coords, mGeomIndex);
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
				getTexture()->bind();
				break;
			default:
				LLImageGL::unbindTexture(0);
				break;
			}
		}

		mVertexBuffer->setBuffer(data_mask);
#if !LL_RELEASE_FOR_DOWNLOAD
		LLGLState::checkClientArrays(data_mask);
#endif
		U16* indicesp = (U16*) mVertexBuffer->getIndicesPointer() + mIndicesIndex;

		if (gPickFaces && mTEOffset != -1)
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
					glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_SHORT, indicesp); 
					glPopMatrix();
				}
				else
				{
					glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_SHORT, indicesp); 
				}
			}
			else
			{
				glPushMatrix();
				glMultMatrixf((float*)getRenderMatrix().mMatrix);
				glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_SHORT, indicesp); 
				glPopMatrix();
			}
		}
		else if (mGeomCount > 0)
		{
			if (isState(GLOBAL))
			{
				glDrawArrays(GL_TRIANGLES, mGeomIndex, mGeomCount); 
			}
			else
			{
				glPushMatrix();
				glMultMatrixf((float*)getRenderMatrix().mMatrix);
				glDrawArrays(GL_TRIANGLES, mGeomIndex, mGeomCount); 
				glPopMatrix();
			}
		}
	}
}

void LLFace::renderSelected(LLImageGL *imagep, const LLColor4& color, const S32 offset, const S32 count)
{
	if(mDrawablep.isNull() || mVertexBuffer.isNull())
	{
		return;
	}

	if (mGeomCount > 0 && mIndicesCount > 0)
	{
		LLGLSPipelineAlpha gls_pipeline_alpha;
		glColor4fv(color.mV);

		LLViewerImage::bindTexture(imagep);
	
		glPushMatrix();
		if (mDrawablep->isActive())
		{
			glMultMatrixf((GLfloat*)mDrawablep->getRenderMatrix().mMatrix);
		}
		else
		{
			glMultMatrixf((GLfloat*)mDrawablep->getRegion()->mRenderMatrix.mMatrix);
		}

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);

		mVertexBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD);
#if !LL_RELEASE_FOR_DOWNLOAD
		LLGLState::checkClientArrays(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD);
#endif
		U16* indicesp = ((U16*) mVertexBuffer->getIndicesPointer()) + mIndicesIndex;

		if (count)
		{
			glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indicesp + offset); 
		}
		else
		{
			glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_SHORT, indicesp); 
		}
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		
		glPopMatrix();
	}
}

void LLFace::renderSelectedUV(const S32 offset, const S32 count)
{
#if 0
	LLViewerImage* red_blue_imagep = gImageList.getImageFromFile("uv_test1.j2c", TRUE, TRUE);
	LLViewerImage* green_imagep = gImageList.getImageFromFile("uv_test2.tga", TRUE, TRUE);

	LLGLSObjectSelect object_select;
	LLGLEnable blend(GL_BLEND);
	
	if (!mDrawPoolp || !getIndicesCount() || getIndicesStart() < 0)
	{
		return;
	}
	for (S32 pass = 0; pass < 2; pass++)
	{
		static F32 bias = 0.f;
		static F32 factor = -10.f;
		if (mGeomCount > 0)
		{
			gGL.color4fv(LLColor4::white.mV);

			if (pass == 0)
			{
				LLViewerImage::bindTexture(red_blue_imagep);
			}
			else // pass == 1
			{
				gGL.blendFunc(GL_ONE, GL_ONE);
				LLViewerImage::bindTexture(green_imagep);
				glMatrixMode(GL_TEXTURE);
				glPushMatrix();
				glScalef(256.f, 256.f, 1.f);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


			if (!isState(GLOBAL))
			{
				// Apply the proper transform for non-global objects.
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glMultMatrixf((float*)getRenderMatrix().mMatrix);
			}

			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(factor, bias);
			if (sSafeRenderSelect)
			{
				gGL.begin(GL_TRIANGLES);
				if (count)
				{
					for (S32 i = offset; i < offset + count; i++)
					{
						LLVector2 tc = mDrawPoolp->getTexCoord(mDrawPoolp->getIndex(getIndicesStart() + i), 0);
						gGL.texCoord2fv(tc.mV);
						LLVector3 vertex = mDrawPoolp->getVertex(mDrawPoolp->getIndex(getIndicesStart() + i));
						gGL.vertex3fv(vertex.mV);
					}
				}
				else
				{
					for (U32 i = 0; i < getIndicesCount(); i++)
					{
						LLVector2 tc = mDrawPoolp->getTexCoord(mDrawPoolp->getIndex(getIndicesStart() + i), 0);
						gGL.texCoord2fv(tc.mV);
						LLVector3 vertex = mDrawPoolp->getVertex(mDrawPoolp->getIndex(getIndicesStart() + i));
						gGL.vertex3fv(vertex.mV);
					}
				}
				gGL.end();
			}
			else
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glEnableClientState(GL_VERTEX_ARRAY);
				llassert(mGeomIndex >= 0);
				if (count)
				{
					if (mIndicesCount > 0)
					{
						glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, getRawIndices() + offset); 
					}
					else
					{
						llerrs << "Rendering non-indexed volume face!" << llendl;
						glDrawArrays(mPrimType, mGeomIndex, mGeomCount); 
					}
				}
				else
				{
					if (mIndicesCount > 0)
					{
						glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_SHORT, getRawIndices()); 
					}
					else
					{
						glDrawArrays(GL_TRIANGLES, mGeomIndex, mGeomCount); 
					}
				}
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}

			glDisable(GL_POLYGON_OFFSET_FILL);
			if (!isState(GLOBAL))
			{
				// Restore the tranform for non-global objects
				glPopMatrix();
			}
			if (pass == 1)
			{
				glMatrixMode(GL_TEXTURE);
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
				gGL.blendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
			}
		}
	}
	
	//restore blend func
	gGL.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
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
		if (mDrawablep->isState(LLDrawable::REBUILD_VOLUME))
		{ //vertex buffer no longer valid
			mVertexBuffer = NULL;
			mLastVertexBuffer = NULL;
		}

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
		
		updateCenterAgent();
	}

	return TRUE;
}

BOOL LLFace::getGeometryVolume(const LLVolume& volume,
							   const S32 &f,
								const LLMatrix4& mat_vert, const LLMatrix3& mat_normal,
								const U16 &index_offset)
{
	const LLVolumeFace &vf = volume.getVolumeFace(f);
	S32 num_vertices = (S32)vf.mVertices.size();
	S32 num_indices = (S32)vf.mIndices.size();
	
	if (mVertexBuffer.notNull())
	{
		if (num_indices + (S32) mIndicesIndex > mVertexBuffer->getNumIndices())
		{
			llwarns << "Index buffer overflow!" << llendl;
			return FALSE;
		}

		if (num_vertices + mGeomIndex > mVertexBuffer->getNumVerts())
		{
			llwarns << "Vertex buffer overflow!" << llendl;
			return FALSE;
		}
	}

	LLStrider<LLVector3> old_verts,vertices;
	LLStrider<LLVector2> old_texcoords,tex_coords;
	LLStrider<LLVector2> old_texcoords2,tex_coords2;
	LLStrider<LLVector3> old_normals,normals;
	LLStrider<LLColor4U> old_colors,colors;
	LLStrider<U16> indicesp;

	BOOL full_rebuild = mDrawablep->isState(LLDrawable::REBUILD_VOLUME);
	BOOL moved = TRUE;

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

	if (!full_rebuild)
	{   
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
			
			if (!moved && !mDrawablep->isState(LLDrawable::REBUILD_ALL))
			{ //nothing needs to be done
				return FALSE;
			}
		}
		mLastMoveTime = gFrameTimeSeconds;
	}
	else
	{
		mLastUpdateTime = gFrameTimeSeconds;
	}
	
	BOOL rebuild_pos = full_rebuild || moved || mDrawablep->isState(LLDrawable::REBUILD_POSITION);
	BOOL rebuild_color = full_rebuild || moved || mDrawablep->isState(LLDrawable::REBUILD_COLOR);
	BOOL rebuild_tcoord = full_rebuild || moved || mDrawablep->isState(LLDrawable::REBUILD_TCOORD);

	const LLTextureEntry *tep = mVObjp->getTE(f);
	U8  bump_code = tep ? tep->getBumpmap() : 0;

	if (rebuild_pos)
	{
		mVertexBuffer->getVertexStrider(vertices, mGeomIndex);
		mVertexBuffer->getNormalStrider(normals, mGeomIndex);
	}
	if (rebuild_tcoord)
	{
		mVertexBuffer->getTexCoordStrider(tex_coords, mGeomIndex);
		if (bump_code)
		{
			mVertexBuffer->getTexCoord2Strider(tex_coords2, mGeomIndex);
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
		//else if (getVirtualSize() <= 512.f)
		//{			
		//	//vobj->mTextureAnimp->animateTextures(os, ot, ms, mt, r);
		//	//cos_ang = cos(r);
		//	//sin_ang = sin(r);
		//}
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
		GLfloat alpha[4] =
		{
			0.00f,
			0.25f,
			0.5f,
			0.75f
		};

		if (getPoolType() != LLDrawPool::POOL_ALPHA && LLPipeline::sRenderBump && tep->getShiny())
		{
			color.mV[3] = U8 (alpha[tep->getShiny()] * 255);
		}
	}

    // INDICES
	if (full_rebuild || moved)
	{
		mVertexBuffer->getIndexStrider(indicesp, mIndicesIndex);
		for (U16 i = 0; i < num_indices; i++)
		{
			*indicesp++ = vf.mIndices[i] + index_offset;
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
			if( mTexture.notNull() && mTexture->getHasGLTexture())
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
						planarProjection(tc, vf.mVertices[i], vf.mCenter, vec);
						break;
					case LLTextureEntry::TEX_GEN_SPHERICAL:
						sphericalProjection(tc, vf.mVertices[i], vf.mCenter, vec);
						break;
					case LLTextureEntry::TEX_GEN_CYLINDRICAL:
						cylindricalProjection(tc, vf.mVertices[i], vf.mCenter, vec);
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

			*tex_coords++ = tc;
		
			if (bump_code)
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
		else if (moved)
		{
			*tex_coords++ = *old_texcoords++;
			if (bump_code)
			{
				*tex_coords2++ = *old_texcoords2++;
			}
		}
			
		if (rebuild_pos)
		{
			*vertices++ = vf.mVertices[i].mPosition * mat_vert;

			LLVector3 normal = vf.mVertices[i].mNormal * mat_normal;
			normal.normVec();
			
			*normals++ = normal;
		}
		else if (moved)
		{
			*normals++ = *old_normals++;
			*vertices++ = *old_verts++;
		}

		if (rebuild_color)
		{
			*colors++ = color;		
		}
		else if (moved)
		{
			*colors++ = *old_colors++;
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

BOOL LLFace::verify(const U32* indices_array) const
{
	BOOL ok = TRUE;
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
		if (mGeomCount <= gGLManager.mGLMaxVertexRange && 
			mIndicesCount <= (U32) gGLManager.mGLMaxIndexRange)
		{
			glDrawRangeElements(GL_TRIANGLES, mGeomIndex, mGeomIndex + mGeomCount-1, mIndicesCount, 
									GL_UNSIGNED_SHORT, index_array + mIndicesIndex); 
		}
		else
		{
			glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_SHORT, index_array+mIndicesIndex);
		}
		gPipeline.addTrianglesDrawn(mIndicesCount/3);
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
