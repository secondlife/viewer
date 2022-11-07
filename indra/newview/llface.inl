/** 
 * @file llface.inl
 * @brief Inline functions for LLFace
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

#ifndef LL_LLFACE_INL
#define LL_LLFACE_INL

#include "llglheaders.h"
#include "llrender.h"

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

inline S32  LLFace::getColors     (LLStrider<LLColor4U> &colors)
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
            LL_ERRS() << "No backup memory for face" << LL_ENDL;
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
            LL_ERRS() << "No color pointer for a color strider!" << LL_ENDL;
        }
        mDrawPoolp->setDirtyColors();
        return mGeomIndex;
    }
}

inline S32  LLFace::getTexCoords  (LLStrider<LLVector2> &texCoords, S32 pass )
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
            LL_ERRS() << "No backup memory for face" << LL_ENDL;
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

inline S32  LLFace::getIndices    (U32*  &indicesp)
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
