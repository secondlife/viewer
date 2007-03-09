/** 
 * @file llface.cpp
 * @brief LLFace class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawable.h" // lldrawable needs to be included before llface
#include "llface.h"
#include "llviewertextureanim.h"

#include "llviewercontrol.h"
#include "llvolume.h"
#include "m3math.h"
#include "v3color.h"

#include "lldrawpoolsimple.h"
#include "lldrawpoolbump.h"
#include "llgl.h"
#include "lllightconstants.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llvosky.h"
#include "llvovolume.h"
#include "pipeline.h"

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
	mVSize = 0.f;
	mPixelArea = 1024.f;
	mState      = GLOBAL;
	mDrawPoolp  = NULL;
	mPoolType = 0;
	mGeomIndex  = -1;
	// mCenterLocal
	// mCenterAgent
	mDistance	= 0.f;

	mGeomCount		= 0;
	mIndicesCount	= 0;
	mIndicesIndex	= -1;
	mTexture		= NULL;
	mTEOffset		= -1;

	setDrawable(drawablep);
	mVObjp = objp;

	mReferenceIndex = -1;
	mAlphaFade = 0.f;

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
		mGeomIndex = -1;

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
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	mGeomCount    = num_vertices;
	mIndicesCount = num_indices;
}

//============================================================================

S32 LLFace::getGeometryAvatar(
						LLStrider<LLVector3> &vertices,
						LLStrider<LLVector3> &normals,
						LLStrider<LLVector3> &binormals,
						LLStrider<LLVector2> &tex_coords,
						LLStrider<F32>		 &vertex_weights,
						LLStrider<LLVector4> &clothing_weights)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);

	if (mVertexBuffer.notNull())
	{
		mVertexBuffer->getVertexStrider      (vertices, mGeomIndex);
		mVertexBuffer->getNormalStrider      (normals, mGeomIndex);
		mVertexBuffer->getBinormalStrider    (binormals, mGeomIndex);
		mVertexBuffer->getTexCoordStrider    (tex_coords, mGeomIndex);
		mVertexBuffer->getWeightStrider(vertex_weights, mGeomIndex);
		mVertexBuffer->getClothWeightStrider(clothing_weights, mGeomIndex);
	}
	else
	{
		mGeomIndex = -1;
	}

	return mGeomIndex;
}

S32 LLFace::getGeometryTerrain(
						LLStrider<LLVector3> &vertices,
						LLStrider<LLVector3> &normals,
						LLStrider<LLColor4U> &colors,
						LLStrider<LLVector2> &texcoords0,
						LLStrider<LLVector2> &texcoords1,
						LLStrider<U32> &indicesp)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (mVertexBuffer.notNull())
	{
		mVertexBuffer->getVertexStrider(vertices, mGeomIndex);
		mVertexBuffer->getNormalStrider(normals, mGeomIndex);
		mVertexBuffer->getColorStrider(colors, mGeomIndex);
		mVertexBuffer->getTexCoordStrider(texcoords0, mGeomIndex);
		mVertexBuffer->getTexCoord2Strider(texcoords1, mGeomIndex);
		mVertexBuffer->getIndexStrider(indicesp, mIndicesIndex);
	}
	else
	{
		mGeomIndex = -1;
	}
	
	return mGeomIndex;
}

S32 LLFace::getGeometry(LLStrider<LLVector3> &vertices, LLStrider<LLVector3> &normals,
					    LLStrider<LLVector2> &tex_coords, LLStrider<U32> &indicesp)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (mGeomCount <= 0)
	{
		return -1;
	}
	
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
	else
	{
		mGeomIndex = -1;
	}
	
	return mGeomIndex;
}

S32 LLFace::getGeometryColors(LLStrider<LLVector3> &vertices, LLStrider<LLVector3> &normals,
							  LLStrider<LLVector2> &tex_coords, LLStrider<LLColor4U> &colors,
							  LLStrider<U32> &indicesp)
{
	S32 res = getGeometry(vertices, normals, tex_coords, indicesp);
	if (res >= 0)
	{
		getColors(colors);
	}
	return res;
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
	if(mGeomIndex < 0 || mDrawablep.isNull() || mVertexBuffer.isNull())
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
		U32* indicesp = (U32*) mVertexBuffer->getIndicesPointer() + mIndicesIndex;

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
				glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_INT, indicesp); 
			}
			else
			{
				glPushMatrix();
				glMultMatrixf((float*)getRenderMatrix().mMatrix);
				glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_INT, indicesp); 
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
	if(mGeomIndex < 0 || mDrawablep.isNull() || mVertexBuffer.isNull())
	{
		return;
	}

	if (mGeomCount > 0 && mIndicesCount > 0)
	{
		LLGLSPipelineAlpha gls_pipeline_alpha;
		glColor4fv(color.mV);

		LLViewerImage::bindTexture(imagep);
		if (!isState(GLOBAL))
		{
			// Apply the proper transform for non-global objects.
			glPushMatrix();
			glMultMatrixf((float*)getRenderMatrix().mMatrix);
		}

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);

		mVertexBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD);
#if !LL_RELEASE_FOR_DOWNLOAD
		LLGLState::checkClientArrays(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD);
#endif
		U32* indicesp = ((U32*) mVertexBuffer->getIndicesPointer()) + mIndicesIndex;

		if (count)
		{
			glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indicesp + offset); 
		}
		else
		{
			glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_INT, indicesp); 
		}
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		
		if (!isState(GLOBAL))
		{
			// Restore the tranform for non-global objects
			glPopMatrix();
		}
	}
}

void LLFace::renderSelectedUV(const S32 offset, const S32 count)
{
#if 0
	LLUUID uv_img_red_blue_id(gViewerArt.getString("uv_test1.tga"));
	LLUUID uv_img_green_id(gViewerArt.getString("uv_test2.tga"));
	LLViewerImage* red_blue_imagep = gImageList.getImage(uv_img_red_blue_id, TRUE, TRUE);
	LLViewerImage* green_imagep = gImageList.getImage(uv_img_green_id, TRUE, TRUE);

	LLGLSObjectSelect object_select;
	LLGLEnable blend(GL_BLEND);
	LLGLEnable texture(GL_TEXTURE_2D);

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
			glColor4fv(LLColor4::white.mV);

			if (pass == 0)
			{
				LLViewerImage::bindTexture(red_blue_imagep);
			}
			else // pass == 1
			{
				glBlendFunc(GL_ONE, GL_ONE);
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
				glBegin(GL_TRIANGLES);
				if (count)
				{
					for (S32 i = offset; i < offset + count; i++)
					{
						LLVector2 tc = mDrawPoolp->getTexCoord(mDrawPoolp->getIndex(getIndicesStart() + i), 0);
						glTexCoord2fv(tc.mV);
						LLVector3 vertex = mDrawPoolp->getVertex(mDrawPoolp->getIndex(getIndicesStart() + i));
						glVertex3fv(vertex.mV);
					}
				}
				else
				{
					for (U32 i = 0; i < getIndicesCount(); i++)
					{
						LLVector2 tc = mDrawPoolp->getTexCoord(mDrawPoolp->getIndex(getIndicesStart() + i), 0);
						glTexCoord2fv(tc.mV);
						LLVector3 vertex = mDrawPoolp->getVertex(mDrawPoolp->getIndex(getIndicesStart() + i));
						glVertex3fv(vertex.mV);
					}
				}
				glEnd();
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
						glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, getRawIndices() + offset); 
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
						glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_INT, getRawIndices()); 
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
				glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
			}
		}
	}
	
	//restore blend func
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

	const LLVolumeFace &face = volume.getVolumeFace(f);
	
	//get bounding box
	if (mDrawablep->isState(LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION))
	{
		if (mDrawablep->isState(LLDrawable::REBUILD_VOLUME))
		{ //vertex buffer no longer valid
			mVertexBuffer = NULL;
			mLastVertexBuffer = NULL;
		}

		LLVector3 min,max;
			
		min = face.mExtents[0];
		max = face.mExtents[1];

		//min, max are in volume space, convert to drawable render space
		LLVector3 center = ((min + max) * 0.5f)*mat_vert;
		LLVector3 size = ((max-min) * 0.5f);
		if (!global_volume)
		{
			size.scaleVec(mDrawablep->getVObj()->getScale());
		}
		LLQuaternion rotation = LLQuaternion(mat_normal);
		
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

		mCenterLocal = (newMin+newMax)*0.5f;
		updateCenterAgent();
	}

	return TRUE;
}


BOOL LLFace::getGeometryVolume(const LLVolume& volume,
							   S32 f,
								LLStrider<LLVector3>& vertices, 
								LLStrider<LLVector3>& normals,
								LLStrider<LLVector2>& tex_coords,
								LLStrider<LLVector2>& tex_coords2,
								LLStrider<LLColor4U>& colors,
								LLStrider<U32>& indicesp,
								const LLMatrix4& mat_vert, const LLMatrix3& mat_normal,
								U32& index_offset)
{
	const LLVolumeFace &vf = volume.getVolumeFace(f);
	S32 num_vertices = (S32)vf.mVertices.size();
	S32 num_indices = (S32)vf.mIndices.size();
	
	LLStrider<LLVector3> old_verts;
	LLStrider<LLVector2> old_texcoords;
	LLStrider<LLVector2> old_texcoords2;
	LLStrider<LLVector3> old_normals;
	LLStrider<LLColor4U> old_colors;

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
				vertices += mGeomCount;
				normals += mGeomCount;
				tex_coords += mGeomCount;
				colors += mGeomCount;
				tex_coords2 += mGeomCount;
				index_offset += mGeomCount;
				indicesp += mIndicesCount;
				return FALSE;
			}

			if (mLastGeomCount == mGeomCount)
			{
				if (mLastGeomIndex >= mGeomIndex && 
					mLastGeomIndex + mGeomCount+1 < mVertexBuffer->getNumVerts())
				{
					//copy from further down the buffer
					mVertexBuffer->getVertexStrider(old_verts, mLastGeomIndex);
					mVertexBuffer->getTexCoordStrider(old_texcoords, mLastGeomIndex);
					mVertexBuffer->getTexCoord2Strider(old_texcoords2, mLastGeomIndex);
					mVertexBuffer->getNormalStrider(old_normals, mLastGeomIndex);
					mVertexBuffer->getColorStrider(old_colors, mLastGeomIndex);

					if (!mDrawablep->isState(LLDrawable::REBUILD_ALL))
					{
						//quick copy
						for (S32 i = 0; i < mGeomCount; i++)
						{
							*vertices++ = *old_verts++;
							*tex_coords++ = *old_texcoords++;
							*tex_coords2++ = *old_texcoords2++;
							*colors++ = *old_colors++;
							*normals++ = *old_normals++;
						}

						for (U32 i = 0; i < mIndicesCount; i++)
						{
							*indicesp++ = vf.mIndices[i] + index_offset;
						}

						index_offset += mGeomCount;
						mLastGeomIndex = mGeomIndex;
						mLastIndicesCount = mIndicesCount;
						mLastIndicesIndex = mIndicesIndex;

						return TRUE;
					}
				}
				else
				{
					full_rebuild = TRUE;
				}
			}
		}
		else
		{
			full_rebuild = TRUE;
		}
	}
	else
	{
		mLastUpdateTime = gFrameTimeSeconds;
	}


	BOOL rebuild_pos = full_rebuild || mDrawablep->isState(LLDrawable::REBUILD_POSITION);
	BOOL rebuild_color = full_rebuild || mDrawablep->isState(LLDrawable::REBUILD_COLOR);
	BOOL rebuild_tcoord = full_rebuild || mDrawablep->isState(LLDrawable::REBUILD_TCOORD);

	F32 r = 0, os = 0, ot = 0, ms = 0, mt = 0, cos_ang = 0, sin_ang = 0;
	
	BOOL is_static = mDrawablep->isStatic();
	BOOL is_global = is_static;

	if (-1 == index_offset)
	{
		return TRUE;
	}
	
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
	
	const LLTextureEntry *tep = mVObjp->getTE(f);
	U8  bump_code = tep ? bump_code = tep->getBumpmap() : 0;

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

	if (isState(TEXTURE_ANIM))
	{
		LLVOVolume* vobj = (LLVOVolume*) (LLViewerObject*) mVObjp;
		U8 mode = vobj->mTexAnimMode;
		if (mode & LLViewerTextureAnim::TRANSLATE)
		{
			os = ot = 0.f;
		}
		if (mode & LLViewerTextureAnim::ROTATE)
		{
			r = 0.f;
			cos_ang = 1.f;
			sin_ang = 0.f;
		}
		if (mode & LLViewerTextureAnim::SCALE)
		{
			ms = mt = 1.f;
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

		if (gPipeline.getPoolTypeFromTE(tep, getTexture()) == LLDrawPool::POOL_BUMP)
		{
			color.mV[3] = U8 (alpha[tep->getShiny()] * 255);
		}
	}

    // INDICES
	if (full_rebuild || moved)
	{
		for (S32 i = 0; i < num_indices; i++)
		{
			*indicesp++ = vf.mIndices[i] + index_offset;
		}
	}
	else
	{
		indicesp += num_indices;
	}
	
	//bump setup
	LLVector3 binormal_dir( -sin_ang, cos_ang, 0 );
	LLVector3 bump_s_primary_light_ray;
	LLVector3 bump_t_primary_light_ray;
	
	if (bump_code)
	{
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
		LLVector3   sun_ray  = gSky.getSunDirection();
		LLVector3   moon_ray = gSky.getMoonDirection();
		LLVector3& primary_light_ray = (sun_ray.mV[VZ] > 0) ? sun_ray : moon_ray;
		bump_s_primary_light_ray = offset_multiple * s_scale * primary_light_ray;
		bump_t_primary_light_ray = offset_multiple * t_scale * primary_light_ray;
	}
		
	U8 texgen = getTextureEntry()->getTexGen();

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

			xform(tc, cos_ang, sin_ang, os, ot, ms, mt);
			*tex_coords++ = tc;
		
			if (bump_code)
			{
				LLVector3 tangent = vf.mVertices[i].mBinormal % vf.mVertices[i].mNormal;
				LLMatrix3 tangent_to_object;
				tangent_to_object.setRows(tangent, vf.mVertices[i].mBinormal, vf.mVertices[i].mNormal);
				LLVector3 binormal = binormal_dir * tangent_to_object;

				binormal = binormal * mat_normal;
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

	if (!rebuild_pos && !moved)
	{
		vertices += num_vertices;
	}

	if (!rebuild_tcoord && !moved)
	{
		tex_coords2 += num_vertices;
		tex_coords += num_vertices;
	}
	else if (!bump_code)
	{
		tex_coords2 += num_vertices;
	}

	if (!rebuild_color && !moved)
	{
		colors += num_vertices;
	}

	if (rebuild_tcoord)
	{
		mTexExtents[0].setVec(0,0);
		mTexExtents[1].setVec(1,1);
		xform(mTexExtents[0], cos_ang, sin_ang, os, ot, ms, mt);
		xform(mTexExtents[1], cos_ang, sin_ang, os, ot, ms, mt);		
	}

	index_offset += num_vertices;

	mLastVertexBuffer = mVertexBuffer;
	mLastGeomCount = mGeomCount;
	mLastGeomIndex = mGeomIndex;
	mLastIndicesCount = mIndicesCount;
	mLastIndicesIndex = mIndicesIndex;

	return TRUE;
}

#if 0
BOOL LLFace::genLighting(const LLVolume* volume, const LLDrawable* drawablep, S32 fstart, S32 fend, 
						 const LLMatrix4& mat_vert, const LLMatrix3& mat_normal, BOOL do_lighting)
{
	if (drawablep->isLight())
	{
		do_lighting = FALSE;
	}
	
	if (!((mDrawPoolp->mDataMaskIL) & LLDrawPool::DATA_COLORS_MASK))
	{
		return FALSE;
	}
	if (mGeomIndex < 0)
	{
		return FALSE; // no geometry
	}
	LLStrider<LLColor4U> colorsp;
	S32 idx = getColors(colorsp);
	if (idx < 0)
	{
		return FALSE;
	}
	
	for (S32 vol_face = fstart; vol_face <= fend; vol_face++)
	{
		const LLVolumeFace &vf = volume->getVolumeFace(vol_face);
		S32 num_vertices = (S32)vf.mVertices.size();

		if (isState(FULLBRIGHT) || !do_lighting)
		{
			for (S32 i = 0; i < num_vertices; i++)
			{
				(*colorsp++).setToBlack();
			}
		}
		else
		{
			for (S32 i = 0; i < num_vertices; i++)
			{
				LLVector3 vertex = vf.mVertices[i].mPosition * mat_vert;
				LLVector3 normal = vf.mVertices[i].mNormal * mat_normal;
				normal.normVec();
		
				LLColor4 color;
				for (LLDrawable::drawable_set_t::const_iterator iter = drawablep->mLightSet.begin();
					 iter != drawablep->mLightSet.end(); ++iter)
				{
					LLDrawable* light_drawable = *iter;
					LLVOVolume* light = light_drawable->getVOVolume();
					if (!light)
					{
						continue;
					}
					LLColor4 light_color;
					light->calcLightAtPoint(vertex, normal, light_color);
					color += light_color;
				}

				color.mV[3] = 1.0f;

				(*colorsp++).setVecScaleClamp(color);
			}
		}
	}
	return TRUE;
}

BOOL LLFace::genShadows(const LLVolume* volume, const LLDrawable* drawablep, S32 fstart, S32 fend, 
						 const LLMatrix4& mat_vert, const LLMatrix3& mat_normal, BOOL use_shadow_factor)
{
	if (drawablep->isLight())
	{
		return FALSE;
	}
	
	if (!((mDrawPoolp->mDataMaskIL) & LLDrawPool::DATA_COLORS_MASK))
	{
		return FALSE;
	}
	if (mGeomIndex < 0)
	{
		return FALSE; // no geometry
	}
	LLStrider<LLColor4U> colorsp;
	S32 idx = getColors(colorsp);
	if (idx < 0)
	{
		return FALSE;
	}
	
	for (S32 vol_face = fstart; vol_face <= fend; vol_face++)
	{
		const LLVolumeFace &vf = volume->getVolumeFace(vol_face);
		S32 num_vertices = (S32)vf.mVertices.size();

		if (isState(FULLBRIGHT))
		{
			continue;
		}

		for (S32 i = 0; i < num_vertices; i++)
		{
			LLVector3 vertex = vf.mVertices[i].mPosition * mat_vert;
			LLVector3 normal = vf.mVertices[i].mNormal * mat_normal;
			normal.normVec();
				
			U8 shadow;

			if (use_shadow_factor)
			{
				shadow = (U8) (drawablep->getSunShadowFactor() * 255);
			}
			else
			{
				shadow = 255;
			}
		
			(*colorsp++).mV[3] = shadow;	
		}
	}
	return TRUE;
}
#endif

BOOL LLFace::verify(const U32* indices_array) const
{
	BOOL ok = TRUE;
	// First, check whether the face data fits within the pool's range.
	if ((mGeomIndex < 0) || (mGeomIndex + mGeomCount) > mVertexBuffer->getNumVerts())
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

void LLFace::enableLights() const
{
 	if (isState(FULLBRIGHT|HUD_RENDER))
	{
		gPipeline.enableLightsFullbright(LLColor4::white);
	}
	else if (mDrawablep->isState(LLDrawable::LIGHTING_BUILT))
	{
		gPipeline.enableLightsStatic(1.f);
	}
	else
	{
		gPipeline.enableLightsDynamic(1.f);
	}
	if (isState(LIGHT))
	{
		const LLVOVolume* vovolume = (const LLVOVolume*)(mDrawablep->getVObj());
		gPipeline.setAmbient(vovolume->getLightColor());
	}
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
		
		if ((mDrawPoolp->mVertexShaderLevel > 0) && (mDrawPoolp->getMaterialAttribIndex() != 0))
		{
			glVertexAttrib4fvARB(mDrawPoolp->getMaterialAttribIndex(), color->mV);
		}
		else
		{
			glColor4fv(color->mV);
		}
	}
}

S32 LLFace::pushVertices(const U32* index_array) const
{
	if (mIndicesCount)
	{
		if (mGeomCount <= gGLManager.mGLMaxVertexRange && 
			mIndicesCount <= (U32) gGLManager.mGLMaxIndexRange)
		{
			glDrawRangeElements(GL_TRIANGLES, mGeomIndex, mGeomIndex + mGeomCount-1, mIndicesCount, 
									GL_UNSIGNED_INT, index_array + mIndicesIndex); 
		}
		else
		{
			glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_INT, index_array+mIndicesIndex);
		}
	}
	
	return mIndicesCount;
}

const LLMatrix4& LLFace::getRenderMatrix() const
{
	return mDrawablep->getRenderMatrix();
}

S32 LLFace::renderElements(const U32 *index_array) const
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
	if(mGeomIndex < 0 || mDrawablep.isNull() || mDrawPoolp == NULL)
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
	U32* index_array = (U32*) mVertexBuffer->getIndicesPointer();
	return renderElements(index_array);
}

//============================================================================
// From llface.inl

S32 LLFace::getVertices(LLStrider<LLVector3> &vertices)
{
	if (!mGeomCount)
	{
		return -1;
	}
	
	if (mGeomIndex >= 0) // flexible objects may not have geometry
	{
		mVertexBuffer->getVertexStrider(vertices, mGeomIndex);
		
	}
	return mGeomIndex;
}

S32 LLFace::getColors(LLStrider<LLColor4U> &colors)
{
	if (!mGeomCount)
	{
		return -1;
	}
	
	llassert(mGeomIndex >= 0);
	mVertexBuffer->getColorStrider(colors, mGeomIndex);
	return mGeomIndex;
}

S32	LLFace::getIndices(LLStrider<U32> &indicesp)
{
	mVertexBuffer->getIndexStrider(indicesp, mIndicesIndex);
	llassert(mGeomIndex >= 0 && indicesp[0] != indicesp[1]);
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
