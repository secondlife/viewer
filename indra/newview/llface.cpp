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

#include "llviewercontrol.h"
#include "llvolume.h"
#include "m3math.h"
#include "v3color.h"

#include "llagparray.h"
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

#include "llagparray.inl"

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
	mGeneration = DIRTY;
	mState      = GLOBAL;
	mDrawPoolp  = NULL;
	mGeomIndex  = -1;
	mSkipRender = FALSE;
	mNextFace = NULL;
	// mCenterLocal
	// mCenterAgent
	mDistance	= 0.f;

	mPrimType		= LLTriangles;
	mGeomCount		= 0;
	mIndicesCount	= 0;
	mIndicesIndex	= -1;
	mTexture		= NULL;
	mTEOffset		= -1;

	mBackupMem = NULL;

	setDrawable(drawablep);
	mVObjp = objp;

	mReferenceIndex = -1;
	mAlphaFade = 0.f;

	mFaceColor = LLColor4(1,0,0,1);
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

	// Remove light and blocker list references

	delete[] mBackupMem;
	mBackupMem = NULL;
}


// static
void LLFace::initClass()
{
}

void LLFace::setWorldMatrix(const LLMatrix4 &mat)
{
	llerrs << "Faces on this drawable are not independently modifiable\n" << llendl;
}


void LLFace::setDirty()
{
	mGeneration = DIRTY;
}

void LLFace::setPool(LLDrawPool* new_pool, LLViewerImage *texturep)
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
			mSkipRender = FALSE;
			mNextFace = NULL;

			// Invalidate geometry (will get rebuilt next frame)
			setDirty();
			if (mDrawablep)
			{
				gPipeline.markRebuild(mDrawablep, LLDrawable::REBUILD_ALL, TRUE);
			}
		}
		if (isState(BACKLIST))
		{
			delete[] mBackupMem;
			mBackupMem = NULL;
			clearState(BACKLIST);
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

S32 LLFace::allocBackupMem()
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	S32 size = 0;
	size += mIndicesCount * 4;
	size += mGeomCount * mDrawPoolp->getStride();

	if (mDrawPoolp->mDataMaskNIL & LLDrawPool::DATA_VERTEX_WEIGHTS_MASK)
	{
		size += mGeomCount * mDrawPoolp->sDataSizes[LLDrawPool::DATA_VERTEX_WEIGHTS];
	}

	if (mDrawPoolp->mDataMaskNIL & LLDrawPool::DATA_CLOTHING_WEIGHTS_MASK)
	{
		size += mGeomCount * mDrawPoolp->sDataSizes[LLDrawPool::DATA_CLOTHING_WEIGHTS];
	}

	delete[] mBackupMem;
	mBackupMem = new U8[size];
	return size;
}


void LLFace::setSize(const S32 num_vertices, const S32 num_indices)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (getState() & SHARED_GEOM)
	{
		mGeomCount    = num_vertices;
		mIndicesCount = num_indices;
		return; // Shared, don't allocate or do anything with memory
	}
	if (num_vertices != (S32)mGeomCount || num_indices != (S32)mIndicesCount)
	{
		setDirty();

		delete[] mBackupMem;
		mBackupMem = NULL;
		clearState(BACKLIST);

		mGeomCount    = num_vertices;
		mIndicesCount = num_indices;
	}

}

BOOL LLFace::reserveIfNeeded()
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (getDirty())
	{
		if (isState(BACKLIST))
		{
			llwarns << "Reserve on backlisted object!" << llendl;
		}

		if (0 == mGeomCount)
		{
			//llwarns << "Reserving zero bytes for face!" << llendl;
			mGeomCount = 0;
			mIndicesCount = 0;
			return FALSE;
		}

		mGeomIndex	  = mDrawPoolp->reserveGeom(mGeomCount);
		// (reserveGeom() always returns a valid index)
		mIndicesIndex = mDrawPoolp->reserveInd (mIndicesCount);
		mGeneration   = mDrawPoolp->mGeneration;
	}

	return TRUE;
}

void LLFace::unReserve()
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (!(isState(SHARED_GEOM)))
	{
		mGeomIndex    = mDrawPoolp->unReserveGeom(mGeomIndex, mGeomCount);
		mIndicesIndex = mDrawPoolp->unReserveInd(mIndicesIndex, mIndicesCount);
	}
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
	
	if (mGeomCount <= 0)
	{
		return -1;
	}
	
	if (isState(BACKLIST))
	{
		if (!mBackupMem)
		{
			llerrs << "No backup memory for backlist" << llendl;
		}

		vertices        = (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_VERTICES]);
		normals         = (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_NORMALS]);
		binormals       = (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_BINORMALS]);
		tex_coords      = (LLVector2*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_TEX_COORDS0]);
		clothing_weights = (LLVector4*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_CLOTHING_WEIGHTS]);
		vertex_weights  = (F32*)(mBackupMem + (4 * mIndicesCount) + (mGeomCount * mDrawPoolp->getStride()));
		tex_coords.setStride( mDrawPoolp->getStride());
		vertices.setStride( mDrawPoolp->getStride());
		normals.setStride( mDrawPoolp->getStride());
		binormals.setStride( mDrawPoolp->getStride());
		clothing_weights.setStride( mDrawPoolp->getStride());

		return 0;
	}
	else
	{
		if (!reserveIfNeeded())
		{
			return -1;
		}

		llassert(mGeomIndex >= 0);
		llassert(mIndicesIndex >= 0);
	
		mDrawPoolp->getVertexStrider      (vertices, mGeomIndex);
		mDrawPoolp->getNormalStrider      (normals, mGeomIndex);
		mDrawPoolp->getBinormalStrider    (binormals, mGeomIndex);
		mDrawPoolp->getTexCoordStrider    (tex_coords, mGeomIndex);
		mDrawPoolp->getVertexWeightStrider(vertex_weights, mGeomIndex);
		mDrawPoolp->getClothingWeightStrider(clothing_weights, mGeomIndex);

		mDrawPoolp->setDirty();

		llassert(mGeomIndex >= 0);
		return mGeomIndex;
	}
}

S32 LLFace::getGeometryTerrain(
						LLStrider<LLVector3> &vertices,
						LLStrider<LLVector3> &normals,
						LLStrider<LLColor4U> &colors,
						LLStrider<LLVector2> &texcoords0,
						LLStrider<LLVector2> &texcoords1,
						U32 *&indicesp)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (mGeomCount <= 0)
	{
		return -1;
	}
	
	if (isState(BACKLIST))
	{
		if (!mBackupMem)
		{
			printDebugInfo();
			llerrs << "No backup memory for face" << llendl;
		}
		vertices  = (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_VERTICES]);
		normals   = (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_NORMALS]);
		colors   =  (LLColor4U*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_COLORS]);
		texcoords0= (LLVector2*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_TEX_COORDS0]);
		texcoords1= (LLVector2*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_TEX_COORDS1]);
		texcoords0.setStride(mDrawPoolp->getStride());
		texcoords1.setStride(mDrawPoolp->getStride());
		vertices.setStride( mDrawPoolp->getStride());
		normals.setStride( mDrawPoolp->getStride());
		colors.setStride( mDrawPoolp->getStride());
		indicesp    = (U32*)mBackupMem;

		return 0;
	}
	else
	{
		if (!reserveIfNeeded())
		{
			llinfos << "Get geometry failed!" << llendl;
			return -1;
		}

		llassert(mGeomIndex >= 0);
		llassert(mIndicesIndex >= 0);
	
		mDrawPoolp->getVertexStrider(vertices, mGeomIndex);
		mDrawPoolp->getNormalStrider(normals, mGeomIndex);
		mDrawPoolp->getColorStrider(colors, mGeomIndex);
		mDrawPoolp->getTexCoordStrider(texcoords0, mGeomIndex, 0);
		mDrawPoolp->getTexCoordStrider(texcoords1, mGeomIndex, 1);

		indicesp = mDrawPoolp->getIndices(mIndicesIndex);

		mDrawPoolp->setDirty();

		llassert(mGeomIndex >= 0);
		return mGeomIndex;
	}
}

S32 LLFace::getGeometry(LLStrider<LLVector3> &vertices, LLStrider<LLVector3> &normals,
					    LLStrider<LLVector2> &tex_coords, U32 *&indicesp)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (mGeomCount <= 0)
	{
		return -1;
	}
	
	if (isState(BACKLIST))
	{
		if (!mBackupMem)
		{
			printDebugInfo();
			llerrs << "No backup memory for face" << llendl;
		}
		vertices  = (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_VERTICES]);
		normals   = (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_NORMALS]);
		tex_coords= (LLVector2*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_TEX_COORDS0]);
		tex_coords.setStride(mDrawPoolp->getStride());
		vertices.setStride( mDrawPoolp->getStride());
		normals.setStride( mDrawPoolp->getStride());
		indicesp    = (U32*)mBackupMem;

		return 0;
	}
	else
	{
		if (!reserveIfNeeded())
		{
			return -1;
		}

		llassert(mGeomIndex >= 0);
		llassert(mIndicesIndex >= 0);
	
		mDrawPoolp->getVertexStrider(vertices,   mGeomIndex);
		if (mDrawPoolp->mDataMaskIL & LLDrawPool::DATA_NORMALS_MASK)
		{
			mDrawPoolp->getNormalStrider(normals,    mGeomIndex);
		}
		if (mDrawPoolp->mDataMaskIL & LLDrawPool::DATA_TEX_COORDS0_MASK)
		{
			mDrawPoolp->getTexCoordStrider(tex_coords, mGeomIndex);
		}

		indicesp      =mDrawPoolp->getIndices   (mIndicesIndex);

		mDrawPoolp->setDirty();

		llassert(mGeomIndex >= 0);
		return mGeomIndex;
	}
}

S32 LLFace::getGeometryColors(LLStrider<LLVector3> &vertices, LLStrider<LLVector3> &normals,
							  LLStrider<LLVector2> &tex_coords, LLStrider<LLColor4U> &colors,
							  U32 *&indicesp)
{
	S32 res = getGeometry(vertices, normals, tex_coords, indicesp);
	if (res >= 0)
	{
		getColors(colors);
	}
	return res;
}

S32 LLFace::getGeometryMultiTexture(
	LLStrider<LLVector3> &vertices, 
	LLStrider<LLVector3> &normals,
	LLStrider<LLVector3> &binormals,
	LLStrider<LLVector2> &tex_coords0,
	LLStrider<LLVector2> &tex_coords1,
	U32 *&indicesp)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (mGeomCount <= 0)
	{
		return -1;
	}
	
	if (isState(BACKLIST))
	{
		if (!mBackupMem)
		{
			printDebugInfo();
			llerrs << "No backup memory for face" << llendl;
		}
		vertices	= (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_VERTICES]);
		normals		= (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_NORMALS]);
		tex_coords0	= (LLVector2*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_TEX_COORDS0]);
		tex_coords0.setStride(	mDrawPoolp->getStride() );
		vertices.setStride(		mDrawPoolp->getStride() );
		normals.setStride(		mDrawPoolp->getStride() );
		if (mDrawPoolp->mDataMaskIL & LLDrawPool::DATA_BINORMALS_MASK)
		{
			binormals	= (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_BINORMALS]);
			binormals.setStride(	mDrawPoolp->getStride() );
		}
		if (mDrawPoolp->mDataMaskIL & LLDrawPool::DATA_TEX_COORDS1_MASK)
		{
			tex_coords1	= (LLVector2*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_TEX_COORDS1]);
			tex_coords1.setStride(	mDrawPoolp->getStride() );
		}
		indicesp	= (U32*)mBackupMem;

		return 0;
	}
	else
	{
		if (!reserveIfNeeded())
		{
			return -1;
		}

		llassert(mGeomIndex >= 0);
		llassert(mIndicesIndex >= 0);
	
		mDrawPoolp->getVertexStrider(vertices, mGeomIndex);
		if (mDrawPoolp->mDataMaskIL & LLDrawPool::DATA_NORMALS_MASK)
		{
			mDrawPoolp->getNormalStrider(normals, mGeomIndex);
		}
		if (mDrawPoolp->mDataMaskIL & LLDrawPool::DATA_TEX_COORDS0_MASK)
		{
			mDrawPoolp->getTexCoordStrider(tex_coords0, mGeomIndex);
		}
		if (mDrawPoolp->mDataMaskIL & LLDrawPool::DATA_BINORMALS_MASK)
		{
			mDrawPoolp->getBinormalStrider(binormals,  mGeomIndex);
		}
		if (mDrawPoolp->mDataMaskIL & LLDrawPool::DATA_TEX_COORDS1_MASK)
		{
			mDrawPoolp->getTexCoordStrider(tex_coords1, mGeomIndex, 1);
		}
		indicesp = mDrawPoolp->getIndices(mIndicesIndex);

		mDrawPoolp->setDirty();

		llassert(mGeomIndex >= 0);
		return mGeomIndex;
	}
}

void LLFace::updateCenterAgent()
{
	mCenterAgent = mCenterLocal * getRenderMatrix();
}

void LLFace::renderForSelect() const
{
	if(mGeomIndex < 0 || mDrawablep.isNull())
	{
		return;
	}
	if (mVObjp->mGLName)
	{
		S32 name = mVObjp->mGLName;

		LLColor4U color((U8)(name >> 16), (U8)(name >> 8), (U8)name);
#if 0 // FIXME: Postponing this fix until we have texcoord pick info...
		if (mTEOffset != -1)
		{
			color.mV[VALPHA] = (U8)(getTextureEntry()->getColor().mV[VALPHA] * 255.f);
		}
#endif
		glColor4ubv(color.mV);

		if (mVObjp->getPCode() == LL_PCODE_VOLUME)
		{
			LLVOVolume *volp;
			volp = (LLVOVolume *)(LLViewerObject*)mVObjp;
			if (volp->getNumFaces() == 1 && !volp->getVolumeChanged())
			{
				// We need to special case the coalesced face model.
				S32 num_vfs = volp->getVolume()->getNumFaces();
				S32 offset = 0;
				S32 i;
				
				for (i = 0; i < num_vfs; i++)
				{
					if (gPickFaces)
					{
						// mask off high 4 bits (16 total possible faces)
						color.mV[0] &= 0x0f;
						color.mV[0] |= (i & 0x0f) << 4;
						glColor4ubv(color.mV);
					}
					S32 count = volp->getVolume()->getVolumeFace(i).mIndices.size();
					if (isState(GLOBAL))
					{
						glDrawElements(mPrimType, count, GL_UNSIGNED_INT, getRawIndices() + offset); 
					}
					else
					{
						glPushMatrix();
						glMultMatrixf((float*)getRenderMatrix().mMatrix);
						glDrawElements(mPrimType, count, GL_UNSIGNED_INT, getRawIndices() + offset); 
						glPopMatrix();
					}
					offset += count;
				}
				// We're done, return.
				return;
			}

			// We don't have coalesced faces, do this the normal way.
		}

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
				glDrawElements(mPrimType, mIndicesCount, GL_UNSIGNED_INT, getRawIndices()); 
			}
			else
			{
				glPushMatrix();
				glMultMatrixf((float*)getRenderMatrix().mMatrix);
				glDrawElements(mPrimType, mIndicesCount, GL_UNSIGNED_INT, getRawIndices()); 
				glPopMatrix();
			}
		}
		else if (mGeomCount > 0)
		{
			if (isState(GLOBAL))
			{
				glDrawArrays(mPrimType, mGeomIndex, mGeomCount); 
			}
			else
			{
				glPushMatrix();
				glMultMatrixf((float*)getRenderMatrix().mMatrix);
				glDrawArrays(mPrimType, mGeomIndex, mGeomCount); 
				glPopMatrix();
			}
		}
	}
}

void LLFace::renderSelected(LLImageGL *imagep, const LLColor4& color, const S32 offset, const S32 count)
{
	if(mGeomIndex < 0 || mDrawablep.isNull())
	{
		return;
	}
	if (mGeomCount > 0)
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

		if (sSafeRenderSelect)
		{
			glBegin(mPrimType);
			if (count)
			{
				for (S32 i = offset; i < offset + count; i++)
				{
					LLVector2 tc = mDrawPoolp->getTexCoord(mDrawPoolp->getIndex(getIndicesStart() + i), 0);
					glTexCoord2fv(tc.mV);
					LLVector3 normal = mDrawPoolp->getNormal(mDrawPoolp->getIndex(getIndicesStart() + i));
					glNormal3fv(normal.mV);
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
					LLVector3 normal = mDrawPoolp->getNormal(mDrawPoolp->getIndex(getIndicesStart() + i));
					glNormal3fv(normal.mV);
					LLVector3 vertex = mDrawPoolp->getVertex(mDrawPoolp->getIndex(getIndicesStart() + i));
					glVertex3fv(vertex.mV);
				}
			}
			glEnd();

			if( gSavedSettings.getBOOL("ShowTangentBasis") )
			{
				S32 start;
				S32 end;
				if (count)
				{
					start = offset;
					end = offset + count;
				}
				else
				{
					start = 0;
					end = getIndicesCount();
				}

				LLGLSNoTexture gls_no_texture;
				glColor4f(1, 1, 1, 1);
				glBegin(GL_LINES);
				for (S32 i = start; i < end; i++)
				{
					LLVector3 vertex = mDrawPoolp->getVertex(mDrawPoolp->getIndex(getIndicesStart() + i));
					glVertex3fv(vertex.mV);
					LLVector3 normal = mDrawPoolp->getNormal(mDrawPoolp->getIndex(getIndicesStart() + i));
					glVertex3fv( (vertex + normal * 0.1f).mV );
				}
				glEnd();

				if (mDrawPoolp->mDataMaskIL & LLDrawPool::DATA_BINORMALS_MASK)
				{
					glColor4f(0, 1, 0, 1);
					glBegin(GL_LINES);
					for (S32 i = start; i < end; i++)
					{
						LLVector3 vertex = mDrawPoolp->getVertex(mDrawPoolp->getIndex(getIndicesStart() + i));
						glVertex3fv(vertex.mV);
						LLVector3 binormal = mDrawPoolp->getBinormal(mDrawPoolp->getIndex(getIndicesStart() + i));
						glVertex3fv( (vertex + binormal * 0.1f).mV );
					}
					glEnd();
				}
			}
		}
		else
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);
			if (count)
			{
				if (mIndicesCount > 0)
				{
					glDrawElements(mPrimType, count, GL_UNSIGNED_INT, getRawIndices() + offset); 
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
					glDrawElements(mPrimType, mIndicesCount, GL_UNSIGNED_INT, getRawIndices()); 
				}
				else
				{
					glDrawArrays(mPrimType, mGeomIndex, mGeomCount); 
				}
			}
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
		}

		if (!isState(GLOBAL))
		{
			// Restore the tranform for non-global objects
			glPopMatrix();
		}
	}
}

void LLFace::renderSelectedUV(const S32 offset, const S32 count)
{
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
				glBegin(mPrimType);
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
						glDrawElements(mPrimType, count, GL_UNSIGNED_INT, getRawIndices() + offset); 
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
						glDrawElements(mPrimType, mIndicesCount, GL_UNSIGNED_INT, getRawIndices()); 
					}
					else
					{
						glDrawArrays(mPrimType, mGeomIndex, mGeomCount); 
					}
				}
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
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
}


void LLFace::printDebugInfo() const
{
	LLDrawPool *poolp = getPool();
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
	if (isState(BACKLIST))
	{
		llinfos << "Backlisted!" << llendl;
	}
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
}

S32 LLFace::backup()
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (isState(BACKLIST))
	{
		llwarns << "Face is already backed up in LLFace::backup!" << llendl;
		return mGeomCount;
	}
	if (mGeomIndex < 0)
	{
		// flexible objects can cause this
		//llwarns << "No geometry to back-up" << llendl;
		return 0;
	}
	
	S32 size = 0;
	if (!mBackupMem)
	{
		size = allocBackupMem();
	}
	else
	{
		llerrs << "Memory already backed up!" << llendl;
	}

	// Need to flag this, because we can allocate a non-zero backup mem if we have indices and no geometry.

	if (mGeomCount || mIndicesCount)
	{
		setState(BACKLIST);
#if !RELEASE_FOR_DOWNLOAD
		if (mGeomIndex < 0 || mIndicesIndex < 0)
		{
			llerrs << "LLFace::backup" << llendl;
		}
#endif
		
		U32 *backup  = (U32*)mBackupMem;
		S32 stride   = mDrawPoolp->getStride();
		
		U32 *index   = mDrawPoolp->getIndices(mIndicesIndex);
		for (U32 i=0;i<mIndicesCount;i++)
		{
			*backup++ = index[i] - mGeomIndex;
			index[i]  = 0;
		}

		if (!mGeomCount)
		{
			return mGeomCount;
		}
		//
		// Don't change the order of these unles you change the corresponding getGeometry calls that read out of
		// backup memory, and also the other of the backup/restore pair!
		//
		memcpy(backup, (mDrawPoolp->mMemory.getMem() + mGeomIndex * stride), mGeomCount * stride);
		backup += mGeomCount * stride / 4;

		if (mDrawPoolp->mDataMaskNIL & LLDrawPool::DATA_CLOTHING_WEIGHTS_MASK)
		{
			memcpy(backup, &mDrawPoolp->getClothingWeight(mGeomIndex), mGeomCount * sizeof(LLVector4));
			backup += mGeomCount*4;
		}

		if (mDrawPoolp->mDataMaskNIL & LLDrawPool::DATA_VERTEX_WEIGHTS_MASK)
		{
			memcpy(backup, &mDrawPoolp->getVertexWeight(mGeomIndex), mGeomCount * sizeof(F32));
			backup += mGeomCount;
		}

		llassert((U8*)backup - mBackupMem == size);

		unReserve();
	}
	return mGeomCount;
}

void LLFace::restore()
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);
	
	if (!isState(BACKLIST))
	{
		// flexible objects can cause this
// 		printDebugInfo();
// 		llwarns << "not backlisted for restore" << llendl;
		return;
	}
	if (!mGeomCount || !mBackupMem) 
	{
		if (!mBackupMem)
		{
			printDebugInfo();
			llwarns  << "no backmem for restore" << llendl;
		}

		clearState(BACKLIST);
		return;
	}

	S32 stride     = mDrawPoolp->getStride();
	mGeomIndex     = mDrawPoolp->reserveGeom(mGeomCount);
	mIndicesIndex  = mDrawPoolp->reserveInd (mIndicesCount);
	mGeneration    = mDrawPoolp->mGeneration;

	llassert(mGeomIndex >= 0);
	llassert(mIndicesIndex >= 0);
	
	U32 *backup  = (U32*)mBackupMem;
	U32 *index   = mDrawPoolp->getIndices(mIndicesIndex);

	for (U32 i=0;i<mIndicesCount;i++)
	{
		S32  ind = mGeomIndex + *backup;
		index[i] = ind;
		backup++;
	}

	mDrawPoolp->mMemory.copyToMem(mGeomIndex * stride, (U8 *)backup, mGeomCount * stride);
	backup += mGeomCount * stride / 4;

	//
	// Don't change the order of these unles you change the corresponding getGeometry calls that read out of
	// backup memory, and also the other of the backup/restore pair!
	//
	if (mDrawPoolp->mDataMaskNIL & LLDrawPool::DATA_CLOTHING_WEIGHTS_MASK)
	{
		mDrawPoolp->mClothingWeights.copyToMem(mGeomIndex, (U8 *)backup, mGeomCount);
		backup += mGeomCount*4;
	}

	if (mDrawPoolp->mDataMaskNIL & LLDrawPool::DATA_VERTEX_WEIGHTS_MASK)
	{
		mDrawPoolp->mWeights.copyToMem(mGeomIndex, (U8 *)backup, mGeomCount);
		backup += mGeomCount;
	}

	delete[] mBackupMem;
	mBackupMem = NULL;
	clearState(BACKLIST);
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


BOOL LLFace::genVolumeTriangles(const LLVolume &volume, S32 f,
								const LLMatrix4& mat, const LLMatrix3& inv_trans_mat, BOOL global_volume)
{
	const LLVolumeFace &vf = volume.getVolumeFace(f);
	S32 num_vertices = (S32)vf.mVertices.size();
	S32 num_indices = (S32)vf.mIndices.size();
	setSize(num_vertices, num_indices);
	
	return genVolumeTriangles(volume, f, f, mat, inv_trans_mat, global_volume);
}

BOOL LLFace::genVolumeTriangles(const LLVolume &volume, S32 fstart, S32 fend, 
								const LLMatrix4& mat_vert, const LLMatrix3& mat_normal, const BOOL global_volume)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);

	if (!mDrawablep)
	{
		return TRUE;
	}

	S32 index_offset;
	F32 r, os, ot, ms, mt, cos_ang, sin_ang;
	LLStrider<LLVector3> vertices;
	LLStrider<LLVector3> normals;
	LLStrider<LLVector3> binormals;
	LLStrider<LLVector2> tex_coords;
	LLStrider<LLVector2> tex_coords2;
	U32                 *indicesp = NULL;

	BOOL bump = mDrawPoolp && (mDrawPoolp->mDataMaskIL & LLDrawPool::DATA_BINORMALS_MASK);
	BOOL is_static = mDrawablep->isStatic();
	BOOL is_global = is_static;
	
	if (bump)
	{
		index_offset = getGeometryMultiTexture(vertices, normals, binormals, tex_coords, tex_coords2, indicesp);
	}
	else
	{
		index_offset = getGeometry(vertices, normals, tex_coords, indicesp);
	}
	if (-1 == index_offset)
	{
		return TRUE;
	}
	
	LLVector3 center_sum(0.f, 0.f, 0.f);
	
	LLVector3 render_pos;
	
	if (mDrawablep->isState(LLDrawable::REBUILD_TCOORD) &&
		global_volume)
	{
		render_pos = mVObjp->getRenderPosition();
	}
	
	setPrimType(LLTriangles);
	
	if (is_global)
	{
		setState(GLOBAL);
	}
	else
	{
		clearState(GLOBAL);
	}
	
	LLVector3 min, max;
	LLVector2 tmin, tmax;
	
	BOOL grab_first_vert = TRUE;
	BOOL grab_first_tcoord = TRUE;
	
	for (S32 vol_face = fstart; vol_face <= fend; vol_face++)
	{
		const LLVolumeFace &vf = volume.getVolumeFace(vol_face);
		S32 num_vertices = (S32)vf.mVertices.size();
		S32 num_indices = (S32)vf.mIndices.size();
		llassert(num_indices > 0);
		
		U8  bump_code;		
		const LLTextureEntry *tep = mVObjp->getTE(vol_face);
		
		if (tep)
		{
			bump_code = tep->getBumpmap();
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
			bump_code = 0;
			cos_ang = 1.0f;
			sin_ang = 0.0f;
			os = 0.0f;
			ot = 0.0f;
			ms = 1.0f;
			mt = 1.0f;
		}

		if (mDrawablep->isState(LLDrawable::REBUILD_VOLUME))
		{
			// VERTICES & NORMALS			
			for (S32 i = 0; i < num_vertices; i++)
			{
				LLVector3 v;
				v = vf.mVertices[i].mPosition * mat_vert;

				LLVector3 normal = vf.mVertices[i].mNormal * mat_normal;
				normal.normVec();
				*normals++ = normal;
				
				*vertices++ = v;
				
				if (grab_first_vert)
				{
					grab_first_vert = FALSE;
					min = max = v;
				}
				else
				{
					for (U32 j = 0; j < 3; j++)
					{
						if (v.mV[j] < min.mV[j])
						{
							min.mV[j] = v.mV[j];
						}
						if (v.mV[j] > max.mV[j])
						{
							max.mV[j] = v.mV[j];
						}
					}
				}
			}
			for (S32 i = 0; i < num_indices; i++)
			{
				S32 index = vf.mIndices[i] + index_offset;
				llassert(index >= 0 && (i != 1 || *(indicesp-1)!=(U32)index));
				*indicesp++ = index;
			}
		}

		if ((mDrawablep->isState(LLDrawable::REBUILD_TCOORD)) ||
			((bump || getTextureEntry()->getTexGen() != 0) && mDrawablep->isState(LLDrawable::REBUILD_VOLUME)))
		{
			// TEX COORDS AND BINORMALS
			LLVector3 binormal_dir( -sin_ang, cos_ang, 0 );
			LLVector3 bump_s_primary_light_ray;
			LLVector3 bump_t_primary_light_ray;
			if (bump)
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
			
			for (S32 i = 0; i < num_vertices; i++)
			{
				LLVector2 tc = vf.mVertices[i].mTexCoord;

				U8 texgen = getTextureEntry()->getTexGen();
				if (texgen != LLTextureEntry::TEX_GEN_DEFAULT)
				{

					LLVector3 vec = vf.mVertices[i].mPosition; //-vf.mCenter;
					
					if (global_volume)
					{	
						vec -= render_pos;
					}
					else
					{
						vec.scaleVec(mVObjp->getScale());
					}

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
				if (grab_first_tcoord)
				{
					grab_first_tcoord = FALSE;
					tmin = tmax = tc;
				}
				else
				{
					for (U32 j = 0; j < 2; j++)
					{
						if (tmin.mV[j] > tc.mV[j])
						{
							tmin.mV[j] = tc.mV[j];
						}
						else if (tmax.mV[j] < tc.mV[j])
						{
							tmax.mV[j] = tc.mV[j];
						}
					}
				}
				if (bump)
				{
					LLVector3 tangent = vf.mVertices[i].mBinormal % vf.mVertices[i].mNormal;
					LLMatrix3 tangent_to_object;
					tangent_to_object.setRows(tangent, vf.mVertices[i].mBinormal, vf.mVertices[i].mNormal);
					LLVector3 binormal = binormal_dir * tangent_to_object;

					if (!global_volume)
					{
						binormal = binormal * mat_normal;
					}
					binormal.normVec();
					tangent.normVec();
					
 					tc += LLVector2( bump_s_primary_light_ray * tangent, bump_t_primary_light_ray * binormal );
					*tex_coords2++ = tc;

					*binormals++ = binormal;
				}
			}
		}

		index_offset += num_vertices;

		center_sum += vf.mCenter * mat_vert;
	}
	
	center_sum /= (F32)(fend-fstart+1);
	
	if (is_static)
	{
		mCenterAgent = center_sum;
		mCenterLocal = mCenterAgent - mDrawablep->getPositionAgent();
	}
	else
	{
		mCenterLocal = center_sum;
		updateCenterAgent();
	}
	
	if (!grab_first_vert && mDrawablep->isState(LLDrawable::REBUILD_VOLUME))
	{
		mExtents[0] = min;
		mExtents[1] = max;
	}
	
	if (!grab_first_tcoord && mDrawablep->isState(LLDrawable::REBUILD_TCOORD))
	{
		mTexExtents[0] = tmin;
		mTexExtents[1] = tmax;
	}
	
	return TRUE;
}

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

BOOL LLFace::verify(const U32* indices_array) const
{
	BOOL ok = TRUE;
	// First, check whether the face data fits within the pool's range.
	if ((mGeomIndex < 0) || (mGeomIndex + mGeomCount) > (S32)getPool()->getVertexCount())
	{
		ok = FALSE;
		llinfos << "Face not within pool range!" << llendl;
	}

	S32 indices_count = (S32)getIndicesCount();
	S32 geom_start = getGeomStart();
	S32 geom_count = mGeomCount;

	if (!indices_count)
	{
		return TRUE;
	}
	
	if (indices_count > LL_MAX_INDICES_COUNT)
	{
		ok = FALSE;
		llinfos << "Face has bogus indices count" << llendl;
	}
	
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
	if (!LLDrawPool::LLOverrideFaceColor::sOverrideFaceColor)
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
	U32 indices_count = mIndicesCount;
	S32 ret = 0;
#if ENABLE_FACE_LINKING
	LLFace* next = mNextFace;
#endif
	
	if (mGeomCount < gGLManager.mGLMaxVertexRange && (S32) indices_count < gGLManager.mGLMaxIndexRange)
	{
		LLFace* current = (LLFace*) this;
		S32 geom_count = mGeomCount;
#if ENABLE_FACE_LINKING
		while (current)
		{
			//chop up batch into implementation recommended sizes
			while (next &&
				   (current == next ||
					((S32) (indices_count + next->mIndicesCount) < gGLManager.mGLMaxIndexRange &&
					 geom_count + next->mGeomCount < gGLManager.mGLMaxVertexRange)))
			{
				indices_count += next->mIndicesCount;
				geom_count += next->mGeomCount;
				next = next->mNextFace;
			}
#endif
			if (indices_count)
			{
				glDrawRangeElements(mPrimType, current->mGeomIndex, current->mGeomIndex + geom_count, indices_count, 
										GL_UNSIGNED_INT, index_array + current->mIndicesIndex); 
			}
			ret += (S32) indices_count;
			indices_count = 0;
			geom_count = 0;
#if ENABLE_FACE_LINKING
			current = next;
		}
#endif
	}
	else
	{
#if ENABLE_FACE_LINKING
		while (next)
		{
			indices_count += next->mIndicesCount;
			next = next->mNextFace;
		}
#endif
		if (indices_count)
		{
			glDrawElements(mPrimType, indices_count, GL_UNSIGNED_INT, index_array + mIndicesIndex); 
		}
		ret += (S32) indices_count;
	}
	
	return ret;

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

S32 LLFace::renderIndexed(const U32 *index_array) const
{
	if (mSkipRender)
	{
		return 0;
	}

	if(mGeomIndex < 0 || mDrawablep.isNull())
	{
		return 0;
	}
	
	renderSetColor();
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
	if (isState(BACKLIST))
	{
		if (!mBackupMem)
		{
			printDebugInfo();
			llerrs << "No backup memory for face" << llendl;
		}
		vertices = (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_VERTICES]);
		vertices.setStride( mDrawPoolp->getStride());
		return 0;
	}
	else
	{
		if (mGeomIndex >= 0) // flexible objects may not have geometry
		{
			mDrawPoolp->getVertexStrider(vertices, mGeomIndex);
			mDrawPoolp->setDirty();
		}
		return mGeomIndex;
	}
}

S32 LLFace::getColors(LLStrider<LLColor4U> &colors)
{
	if (!mGeomCount)
	{
		return -1;
	}
	if (isState(BACKLIST))
	{
		llassert(mBackupMem);
		colors   = (LLColor4U*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_COLORS]);
		colors.setStride( mDrawPoolp->getStride());
		return 0;
	}
	else
	{
		llassert(mGeomIndex >= 0);
		mDrawPoolp->getColorStrider(colors, mGeomIndex);
		return mGeomIndex;
	}
}

S32	LLFace::getIndices(U32*  &indicesp)
{
	if (isState(BACKLIST))
	{
		indicesp = (U32*)mBackupMem;
		return 0;
	}
	else
	{
		indicesp = mDrawPoolp->getIndices(mIndicesIndex);
		llassert(mGeomIndex >= 0 && indicesp[0] != indicesp[1]);
		return mGeomIndex;
	}
}

void LLFace::link(LLFace* facep)
{
#if ENABLE_FACE_LINKING
	mNextFace = facep;
	facep->mSkipRender = TRUE;
#endif
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
