/** 
 * @file lltextureatlas.cpp
 * @brief LLTextureAtlas class implementation.
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
#include "linden_common.h"
#include "llerror.h"
#include "llimage.h"
#include "llmath.h"
#include "llgl.h"
#include "llrender.h"
#include "lltextureatlas.h"

//-------------------
S16 LLTextureAtlas::sMaxSubTextureSize = 64 ;
S16 LLTextureAtlas::sSlotSize = 32 ;

#ifndef DEBUG_ATLAS
#define DEBUG_ATLAS 0
#endif

#ifndef DEBUG_USAGE_BITS
#define DEBUG_USAGE_BITS 0
#endif
//**************************************************************************************************************
LLTextureAtlas::LLTextureAtlas(U8 ncomponents, S16 atlas_dim) : 
    LLViewerTexture(atlas_dim * sSlotSize, atlas_dim * sSlotSize, ncomponents, TRUE),
	mAtlasDim(atlas_dim),
	mNumSlotsReserved(0),
	mMaxSlotsInAtlas(atlas_dim * atlas_dim)
{
	generateEmptyUsageBits() ;

	//generate an empty texture
	generateGLTexture() ;
	LLPointer<LLImageRaw> image_raw = new LLImageRaw(mFullWidth, mFullHeight, mComponents);
	createGLTexture(0, image_raw, 0);
	image_raw = NULL;
}

LLTextureAtlas::~LLTextureAtlas() 
{
	if(mSpatialGroupList.size() > 0)
	{
		llerrs << "Not clean up the spatial groups!" << llendl ;
	}
	releaseUsageBits() ;
}

//virtual 
S8 LLTextureAtlas::getType() const
{
	return LLViewerTexture::ATLAS_TEXTURE ;
}

void LLTextureAtlas::getTexCoordOffset(S16 col, S16 row, F32& xoffset, F32& yoffset)
{
	xoffset = (F32)col / mAtlasDim ;
	yoffset = (F32)row / mAtlasDim ;	
}

void LLTextureAtlas::getTexCoordScale(S32 w, S32 h, F32& xscale, F32& yscale)
{
	xscale = (F32)w / (mAtlasDim * sSlotSize) ;
	yscale = (F32)h / (mAtlasDim * sSlotSize) ;	
}

//insert a texture piece into the atlas
LLGLuint LLTextureAtlas::insertSubTexture(LLImageGL* source_gl_tex, S32 discard_level, const LLImageRaw* raw_image, S16 slot_col, S16 slot_row)
{
	if(!getTexName())
	{
		return 0 ;
	}

	S32 w = raw_image->getWidth() ;
	S32 h = raw_image->getHeight() ;
	if(w < 8 || w > sMaxSubTextureSize || h < 8 || h > sMaxSubTextureSize)
	{
		//size overflow
		return 0 ;
	}

	BOOL res = gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, getTexName());
	if (!res) 
	{
		llerrs << "bindTexture failed" << llendl;
	}
	
	GLint xoffset = sSlotSize * slot_col ;
	GLint yoffset = sSlotSize * slot_row ;

	if(!source_gl_tex->preAddToAtlas(discard_level, raw_image))
	{
		return 0 ;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, w, h,
						mGLTexturep->getPrimaryFormat(), mGLTexturep->getFormatType(), raw_image->getData());
	
	source_gl_tex->postAddToAtlas() ;
	return getTexName();
}
	
//release a sub-texture slot from the atlas
void LLTextureAtlas::releaseSlot(S16 slot_col, S16 slot_row, S8 slot_width)
{
	unmarkUsageBits(slot_width, slot_col, slot_row) ;
	mNumSlotsReserved -= slot_width * slot_width ;
}

BOOL LLTextureAtlas::isEmpty() const 
{
	return !mNumSlotsReserved ;
}
	
BOOL LLTextureAtlas::isFull(S8 to_be_reserved) const 
{
	return mNumSlotsReserved + to_be_reserved > mMaxSlotsInAtlas ;
}
F32  LLTextureAtlas::getFullness() const 
{
	return (F32)mNumSlotsReserved / mMaxSlotsInAtlas ;
}

void LLTextureAtlas::addSpatialGroup(LLSpatialGroup* groupp) 
{
	if(groupp && !hasSpatialGroup(groupp))
	{
		mSpatialGroupList.push_back(groupp);
	}
}

void LLTextureAtlas::removeSpatialGroup(LLSpatialGroup* groupp) 
{
	if(groupp)
	{
		mSpatialGroupList.remove(groupp);
	}
}

void LLTextureAtlas::clearSpatialGroup() 
{
	mSpatialGroupList.clear();
}
void LLTextureAtlas::removeLastSpatialGroup() 
{
	mSpatialGroupList.pop_back() ;
}

LLSpatialGroup* LLTextureAtlas::getLastSpatialGroup() 
{
	if(mSpatialGroupList.size() > 0)
	{
		return mSpatialGroupList.back() ;
	}
	return NULL ;
}

BOOL LLTextureAtlas::hasSpatialGroup(LLSpatialGroup* groupp) 
{
	for(std::list<LLSpatialGroup*>::iterator iter = mSpatialGroupList.begin(); iter != mSpatialGroupList.end() ; ++iter)
	{
		if(*iter == groupp)
		{
			return TRUE ;
		}
	}
	return FALSE ;
}

//--------------------------------------------------------------------------------------
//private
void LLTextureAtlas::generateEmptyUsageBits()
{
	S32 col_len = (mAtlasDim + 7) >> 3 ;
	mUsageBits = new U8*[mAtlasDim] ;
	*mUsageBits = new U8[mAtlasDim * col_len] ;

	mUsageBits[0] = *mUsageBits ;
	for(S32 i = 1 ; i < mAtlasDim ; i++)
	{
	   mUsageBits[i] = mUsageBits[i-1] + col_len ;

	   for(S32 j = 0 ; j < col_len ; j++)
	   {
		   //init by 0 for all bits.
		   mUsageBits[i][j] = 0 ;
	   }
	}

	//do not forget mUsageBits[0]!
	for(S32 j = 0 ; j < col_len ; j++)
	{
		//init by 0 for all bits.
		mUsageBits[0][j] = 0 ;
	}

	mTestBits = NULL ;
#if DEBUG_USAGE_BITS
	//------------
	//test
	mTestBits = new U8*[mAtlasDim] ;
	*mTestBits = new U8[mAtlasDim * mAtlasDim] ;
	mTestBits[0] = *mTestBits ;
	for(S32 i = 1 ; i < mAtlasDim ; i++)
	{
	   mTestBits[i] = mTestBits[i-1] + mAtlasDim ;

	   for(S32 j = 0 ; j < mAtlasDim ; j++)
	   {
		   //init by 0 for all bits.
		   mTestBits[i][j] = 0 ;
	   }
	}

	for(S32 j = 0 ; j < mAtlasDim ; j++)
	{
		//init by 0 for all bits.
		mTestBits[0][j] = 0 ;
	}
#endif
}

void LLTextureAtlas::releaseUsageBits()
{
   if(mUsageBits)
   {
       delete[] *mUsageBits ;
       delete[] mUsageBits ;
   }
   mUsageBits = NULL ;

   //test
   if( mTestBits)
   {
	   delete[] *mTestBits;
	   delete[]  mTestBits;
   }
    mTestBits = NULL ;
}

void LLTextureAtlas::markUsageBits(S8 bits_len, U8 mask, S16 col, S16 row)
{
	S16 x = col >> 3 ;
	
	for(S8 i = 0 ; i < bits_len ; i++)
	{
		mUsageBits[row + i][x] |= mask ;
	}

#if DEBUG_USAGE_BITS
	//test
	for(S8 i = row ; i < row + bits_len ; i++)
	{
		for(S8 j = col ; j < col + bits_len ; j++)
		{
			mTestBits[i][j] = 1 ;
		}
	}
#endif
}

void LLTextureAtlas::unmarkUsageBits(S8 bits_len, S16 col, S16 row)
{
	S16 x = col >> 3 ;
	U8  mask = 1 ;
	for(S8 i = 1 ; i < bits_len ; i++)
	{
		mask |= (1 << i) ;
	}
	mask <<= (col & 7) ;
	mask = ~mask ;
	
	for(S8 i = 0 ; i < bits_len ; i++)
	{
		mUsageBits[row + i][x] &= mask ;
	}

#if DEBUG_USAGE_BITS
	//test
	for(S8 i = row ; i < row + bits_len ; i++)
	{
		for(S8 j = col ; j < col + bits_len ; j++)
		{
			mTestBits[i][j] = 0 ;
		}
	}
#endif
}

//return true if any of bits in the range marked.
BOOL LLTextureAtlas::areUsageBitsMarked(S8 bits_len, U8 mask, S16 col, S16 row)
{
	BOOL ret = FALSE ;	
	S16 x = col >> 3 ;
	
	for(S8 i = 0 ; i < bits_len ; i++)
	{
		if(mUsageBits[row + i][x] & mask)
		{
			ret = TRUE ;
			break ;
			//return TRUE ;
		}
	}

#if DEBUG_USAGE_BITS
	//test
	BOOL ret2 = FALSE ;
	for(S8 i = row ; i < row + bits_len ; i++)
	{
		for(S8 j = col ; j < col + bits_len ; j++)
		{
			if(mTestBits[i][j])
			{
				ret2 = TRUE ;
			}
		}
	}

	if(ret != ret2)
	{
		llerrs << "bits map corrupted." << llendl ;
	}
#endif
	return ret ;//FALSE ;
}

//----------------------------------------------------------------------
//
//index order: Z order, i.e.: 
// |-----|-----|-----|-----|
// |  10 |  11 | 14  | 15  |
// |-----|-----|-----|-----|
// |   8 |   9 | 12  | 13  |
// |-----|-----|-----|-----|
// |   2 |   3 |   6 |   7 |
// |-----|-----|-----|-----|
// |   0 |   1 |   4 |   5 |
// |-----|-----|-----|-----|
void LLTextureAtlas::getPositionFromIndex(S16 index, S16& col, S16& row)
{
	col = 0 ;
	row = 0 ;

	S16 index_copy = index ;
	for(S16 i = 0 ; index_copy && i < 16 ; i += 2)
	{
		col |= ((index & (1 << i)) >> i) << (i >> 1) ;
		row |= ((index & (1 << (i + 1))) >> (i + 1)) << (i >> 1) ;
		index_copy >>= 2 ;
	}
}
void LLTextureAtlas::getIndexFromPosition(S16 col, S16 row, S16& index)
{
	index = 0 ;
	S16 col_copy = col ;
	S16 row_copy = row ;
	for(S16 i = 0 ; (col_copy || row_copy) && i < 16 ; i++)
	{
		index |= ((col & 1 << i) << i) | ((row & 1 << i) << ( i + 1)) ;
		col_copy >>= 1 ;
		row_copy >>= 1 ;
	}
}
//----------------------------------------------------------------------
//return TRUE if succeeds.
BOOL LLTextureAtlas::getNextAvailableSlot(S8 bits_len, S16& col, S16& row)
{
    S16 index_step = bits_len * bits_len ;

    U8 mask = 1 ;
	for(S8 i = 1 ; i < bits_len ; i++)
	{
		mask |= (1 << i) ;
	}	
   
	U8 cur_mask ;
	for(S16 index = 0 ; index < mMaxSlotsInAtlas ; index += index_step)
    {
		getPositionFromIndex(index, col, row) ;
		
		cur_mask = mask << (col & 7) ;
		if(!areUsageBitsMarked(bits_len, cur_mask, col, row))
		{
			markUsageBits(bits_len, cur_mask, col, row) ;
			mNumSlotsReserved += bits_len * bits_len ;
			
			return TRUE ;
		}
    }

   return FALSE ;
}
