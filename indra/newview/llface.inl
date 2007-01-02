/** 
 * @file llface.inl
 * @brief Inline functions for LLFace
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFACE_INL
#define LL_LLFACE_INL

#include "llglheaders.h"

inline BOOL LLFace::getDirty() const
{ 
	return (mGeneration != mDrawPoolp->mGeneration); 
}

inline void LLFace::clearDirty()
{
	mGeneration = mDrawPoolp->mGeneration;
}

inline const LLTextureEntry* LLFace::getTextureEntry()   const
{
	return mVObjp->getTE(mTEOffset);
}

inline LLDrawPool* LLFace::getPool()     const
{
	return mDrawPoolp;
}

inline S32 LLFace::getStride()   const
{
	return mDrawPoolp->getStride();
}

inline LLDrawable* LLFace::getDrawable() const
{
	return mDrawablep;
}

inline LLViewerObject* LLFace::getViewerObject() const
{
	return mVObjp;
}



inline S32 LLFace::getVertices(LLStrider<LLVector3> &vertices)
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
		llassert(mGeomIndex >= 0);
		mDrawPoolp->getVertexStrider(vertices, mGeomIndex);
		mDrawPoolp->setDirty();
		return mGeomIndex;
	}
}

inline S32 LLFace::getNormals(LLStrider<LLVector3> &normals)
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
		normals   = (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_NORMALS]);
		normals.setStride( mDrawPoolp->getStride());
		return 0;
	}
	else
	{
		llassert(mGeomIndex >= 0);
		mDrawPoolp->getNormalStrider(normals, mGeomIndex);
		mDrawPoolp->setDirty();
		return mGeomIndex;
	}
}

inline S32 LLFace::getBinormals(LLStrider<LLVector3> &binormals)
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
		binormals   = (LLVector3*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_BINORMALS]);
		binormals.setStride( mDrawPoolp->getStride());
		return 0;
	}
	else
	{
		llassert(mGeomIndex >= 0);
		mDrawPoolp->getBinormalStrider(binormals, mGeomIndex);
		mDrawPoolp->setDirty();
		return mGeomIndex;
	}
}


inline S32	LLFace::getColors     (LLStrider<LLColor4U> &colors)
{
	if (!mGeomCount)
	{
		return -1;
	}
	LLColor4U *colorp = NULL;
	if (isState(BACKLIST))
	{
		if (!mBackupMem)
		{
			printDebugInfo();
			llerrs << "No backup memory for face" << llendl;
		}
		colorp = (LLColor4U*)(mBackupMem + (4 * mIndicesCount) + (mGeomCount * mDrawPoolp->getStride()));
		colors = colorp;
		return 0;
	}
	else
	{
		llassert(mGeomIndex >= 0);
		if (!mDrawPoolp->getColorStrider(colors, mGeomIndex))
		{
			printDebugInfo();
			llerrs << "No color pointer for a color strider!" << llendl;
		}
		mDrawPoolp->setDirtyColors();
		return mGeomIndex;
	}
}

inline S32	LLFace::getTexCoords  (LLStrider<LLVector2> &texCoords, S32 pass )
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
		texCoords = (LLVector2*)(mBackupMem + (4 * mIndicesCount) + mDrawPoolp->mDataOffsets[LLDrawPool::DATA_TEX_COORDS0 + pass]);
		texCoords.setStride( mDrawPoolp->getStride());
		return 0;
	}
	else
	{
		llassert(mGeomIndex >= 0);
		mDrawPoolp->getTexCoordStrider(texCoords, mGeomIndex, pass );
		mDrawPoolp->setDirty();
		return mGeomIndex;
	}
}

inline S32	LLFace::getIndices    (U32*  &indicesp)
{
	if (isState(BACKLIST))
	{
		indicesp = (U32*)mBackupMem;
		return 0;
	}
	else
	{
		indicesp = mDrawPoolp->getIndices(mIndicesIndex);
		llassert(mGeomIndex >= 0);
		return mGeomIndex;
	}
}

inline const U32* LLFace::getRawIndices() const
{
	llassert(!isState(BACKLIST));

	return &mDrawPoolp->mIndices[mIndicesIndex];
}


inline void LLFace::bindTexture(S32 stage) const
{
	if (mTexture)
	{
		mTexture->bindTexture(stage);
	}
	else
	{
		LLImageGL::unbindTexture(stage, GL_TEXTURE_2D);
	}
}

#endif
