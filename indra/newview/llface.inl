/** 
 * @file llface.inl
 * @brief Inline functions for LLFace
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
