/** 
 * @file lltextureatlasmanager.cpp
 * @brief LLTextureAtlasManager class implementation.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "linden_common.h"
#include "llerror.h"
#include "llmath.h"
#include "lltextureatlas.h"
#include "lltextureatlasmanager.h"
#include "llspatialpartition.h"

const S8 MAX_NUM_EMPTY_ATLAS = 2 ;
const F32 MIN_ATLAS_FULLNESS = 0.6f ;

//*********************************************************************************************
//implementation of class LLTextureAtlasInfo
//*********************************************************************************************
LLTextureAtlasSlot::LLTextureAtlasSlot(LLTextureAtlas* atlasp, LLSpatialGroup* groupp, S16 col, S16 row, F32 xoffset, F32 yoffset, S8 slot_width) : 
	mAtlasp(atlasp),
	mGroupp(groupp),
	mCol(col),
	mRow(row),
	mReservedSlotWidth(slot_width),
	mValid(FALSE),
	mUpdatedTime(0),
	mTexCoordOffset(xoffset, yoffset),
	mTexCoordScale(1.f, 1.f)
{
	llassert_always(mAtlasp || mGroupp || mReservedSlotWidth) ;
}

LLTextureAtlasSlot::~LLTextureAtlasSlot()
{
	if(mAtlasp)
	{
		mAtlasp->releaseSlot(mCol, mRow, mReservedSlotWidth) ;
		if(mAtlasp->isEmpty())
		{
			LLTextureAtlasManager::getInstance()->releaseAtlas(mAtlasp) ;
		}
		mAtlasp = NULL ;
	}
}

//void LLTextureAtlasSlot::setAtlas(LLTextureAtlas* atlasp) 
//{
//	mAtlasp = atlasp ;
//}
//void LLTextureAtlasSlot::setSlotPos(S16 col, S16 row) 
//{
//	mCol = col ;
//	mRow = row ;
//}
//void LLTextureAtlasSlot::setSlotWidth(S8 width) 
//{
//	//slot is a square with each edge length a power-of-two number
//	mReservedSlotWidth = width ;
//}
//void LLTextureAtlasSlot::setTexCoordOffset(F32 xoffset, F32 yoffset) 
//{
//	mTexCoordOffset.mV[0] = xoffset ;
//	mTexCoordOffset.mV[1] = yoffset ;
//}

void LLTextureAtlasSlot::setSpatialGroup(LLSpatialGroup* groupp) 
{
	mGroupp = groupp ;
}
void LLTextureAtlasSlot::setTexCoordScale(F32 xscale, F32 yscale) 
{
	mTexCoordScale.mV[0] = xscale ;
	mTexCoordScale.mV[1] = yscale ;
}
//*********************************************************************************************
//END of implementation of class LLTextureAtlasInfo
//*********************************************************************************************

//*********************************************************************************************
//implementation of class LLTextureAtlasManager
//*********************************************************************************************
LLTextureAtlasManager::LLTextureAtlasManager() :
	mAtlasMap(4),
	mEmptyAtlasMap(4) 
{
}

LLTextureAtlasManager::~LLTextureAtlasManager()
{
	for(S32 i = 0 ; i < 4 ; i++)
	{
		for(ll_texture_atlas_list_t::iterator j = mAtlasMap[i].begin() ; j != mAtlasMap[i].end() ; ++j)
		{
			*j = NULL ;
		}
		for(ll_texture_atlas_list_t::iterator j = mEmptyAtlasMap[i].begin() ; j != mEmptyAtlasMap[i].end() ; ++j)
		{
			*j = NULL ;
		}

		mAtlasMap[i].clear() ;
		mEmptyAtlasMap[i].clear() ;
	}
	mAtlasMap.clear() ;
	mEmptyAtlasMap.clear() ;
}

//return TRUE if qualified
BOOL LLTextureAtlasManager::canAddToAtlas(S32 w, S32 h, S8 ncomponents, LLGLenum target) 
{
	if(ncomponents < 1 || ncomponents > 4)
	{
		return FALSE ;
	}
	//only support GL_TEXTURE_2D
	if(GL_TEXTURE_2D != target)
	{
		return FALSE ;
	}
	//real image size overflows
	if(w < 8 || w > LLTextureAtlas::sMaxSubTextureSize || h < 8 || h > LLTextureAtlas::sMaxSubTextureSize)
	{
		return FALSE ;
	}

	//if non-power-of-two number
	if((w & (w - 1)) || (h & (h - 1)))
	{
		return FALSE ;
	}

	return TRUE ;
}

void LLTextureAtlasManager::releaseAtlas(LLTextureAtlas* atlasp)
{	
	LLSpatialGroup* groupp = atlasp->getLastSpatialGroup() ;
	while(groupp)
	{
		groupp->removeAtlas(atlasp, FALSE) ;
		atlasp->removeLastSpatialGroup() ;

		groupp = atlasp->getLastSpatialGroup() ;
	}

	S8 type = atlasp->getComponents() - 1 ;	
	//insert to the empty list
	if(mEmptyAtlasMap[type].size() < MAX_NUM_EMPTY_ATLAS)
	{					
		mEmptyAtlasMap[type].push_back(atlasp) ;
	}
		
	//delete the atlasp
	mAtlasMap[type].remove(atlasp) ;
}

//
//this function reserves an appropriate slot from atlas pool for an image.
//return non-NULL if succeeds.
//Note:
//1, this function does not check if the image this slot assigned for qualifies for atlas or not, 
//       call LLTextureAtlasManager::canAddToAtlas(...) to do the check before calling this function.
//2, this function also dose not check if the image is already in atlas. It always assigns a new slot anyway.
//3, this function tries to group sub-textures from same spatial group into ONE atlas to improve render batching.
//
LLPointer<LLTextureAtlasSlot> LLTextureAtlasManager::reserveAtlasSlot(S32 sub_texture_size, S8 ncomponents, 
																		  LLSpatialGroup* groupp, LLViewerTexture* imagep)
{
	if(!groupp)
	{
		//do not insert to atlas if does not have a group.
		return NULL ;
	}

	//bits_len must <= 8 and is a power of two number, i.e.: must be one of these numbers: 1, 2, 4, 8.
	if(sub_texture_size > LLTextureAtlas::sMaxSubTextureSize)
	{
		sub_texture_size = LLTextureAtlas::sMaxSubTextureSize ;
	}
	S8 bits_len = sub_texture_size / LLTextureAtlas::sSlotSize ;
	if(bits_len < 1)
	{
	   bits_len = 1 ;
	}
		
	S16 col = -1, row = -1;
	S8 total_bits = bits_len * bits_len ;

	//insert to the atlas reserved by the same spatial group
	LLPointer<LLTextureAtlas> atlasp = groupp->getAtlas(ncomponents, total_bits) ;
	if(atlasp.notNull())
	{
		if(!atlasp->getNextAvailableSlot(bits_len, col, row))
		{
			//failed
			atlasp = NULL ;
		}		
	}

   //search an atlas to fit for 'size'
	if(!atlasp)
	{
		S8 atlas_index = ncomponents - 1 ;
		ll_texture_atlas_list_t::iterator iter = mAtlasMap[atlas_index].begin() ;
		for(; iter != mAtlasMap[atlas_index].end(); ++iter) 
		{
			LLTextureAtlas* cur = (LLTextureAtlas*)*iter ;
			if(cur->getFullness() < MIN_ATLAS_FULLNESS)//this atlas is empty enough for this group to insert more sub-textures later if necessary.
			{
				if(cur->getNextAvailableSlot(bits_len, col, row))
				{
					atlasp = cur ;
					groupp->addAtlas(atlasp) ;				
					break ;
				}
			}
		}
	}

	//create a new atlas if necessary
	if(!atlasp)
	{
		if(mEmptyAtlasMap[ncomponents - 1].size() > 0)
		{
			//there is an empty one
			atlasp = mEmptyAtlasMap[ncomponents - 1].back() ;
			mEmptyAtlasMap[ncomponents - 1].pop_back() ;
		}
		else
		{
			atlasp = new LLTextureAtlas(ncomponents, 16) ;
		}
		mAtlasMap[ncomponents - 1].push_back(atlasp) ;
		atlasp->getNextAvailableSlot(bits_len, col, row) ;		
		groupp->addAtlas(atlasp) ;
	}

	F32 xoffset, yoffset ;
	atlasp->getTexCoordOffset(col, row, xoffset, yoffset) ;
	LLPointer<LLTextureAtlasSlot> slot_infop = new LLTextureAtlasSlot(atlasp, groupp, col, row, xoffset, yoffset, bits_len) ;
	
	return slot_infop ;
}

//*********************************************************************************************
//END of implementation of class LLTextureAtlasManager
//*********************************************************************************************
