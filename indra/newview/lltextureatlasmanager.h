/** 
 * @file lltextureatlasmanager.h
 * @brief LLTextureAtlasManager base class.
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


#ifndef LL_TEXTUREATLASMANAGER_H
#define LL_TEXTUREATLASMANAGER_H

#include "llmemory.h"

class LLSpatialGroup ;
class LLViewerTexture ;

//just use it as a structure.
class LLTextureAtlasSlot : public LLRefCount
{
public:
	LLTextureAtlasSlot(LLTextureAtlas* atlasp, LLSpatialGroup* groupp, S16 col, S16 row, F32 xoffset, F32 yoffset, S8 slot_width) ;

protected:
	virtual ~LLTextureAtlasSlot();
	
public:

	//
	//do not allow to change those values
	//
	//void setAtlas(LLTextureAtlas* atlasp) ;	
	//void setSlotPos(S16 col, S16 row) ;
	//void setSlotWidth(S8 width) ;
	//void setTexCoordOffset(F32 xoffser, F32 yoffset) ;
	//

	void setSpatialGroup(LLSpatialGroup* groupp) ;
	void setTexCoordScale(F32 xscale, F32 yscale) ;
	void setValid() {mValid = TRUE ;}

	LLTextureAtlas* getAtlas()const {return mAtlasp;}
	LLSpatialGroup* getSpatialGroup() const {return mGroupp ;} 
	S16             getSlotCol()const {return mCol;}
	S16             getSlotRow()const {return mRow;}
	S8              getSlotWidth()const{return mReservedSlotWidth;}
	BOOL            isValid()const { return mValid;}
	const LLVector2*      getTexCoordOffset()const {return &mTexCoordOffset;}
	const LLVector2*      getTexCoordScale() const {return &mTexCoordScale;}

	void setUpdatedTime(U32 t) {mUpdatedTime = t;}
	U32  getUpdatedTime()const {return mUpdatedTime;}

private:
	LLTextureAtlas* mAtlasp;
	S16             mCol ;//col of the slot
	S16             mRow ;//row of the slot
	S8              mReservedSlotWidth ; //slot is a square with each edge length a power-of-two number	
	LLSpatialGroup* mGroupp ;
	BOOL            mValid ;

	LLVector2       mTexCoordOffset ;
	LLVector2       mTexCoordScale ;

	U32             mUpdatedTime ;	
} ;

class LLTextureAtlasManager : public LLSingleton<LLTextureAtlasManager>
{
private:
	typedef std::list<LLPointer<LLTextureAtlas> > ll_texture_atlas_list_t ;

public:
	LLTextureAtlasManager();
	~LLTextureAtlasManager();

	LLPointer<LLTextureAtlasSlot> reserveAtlasSlot(S32 sub_texture_size, S8 ncomponents, 
		LLSpatialGroup* groupp, LLViewerTexture* imagep) ;
	void releaseAtlas(LLTextureAtlas* atlasp);

	BOOL canAddToAtlas(S32 w, S32 h, S8 ncomponents, LLGLenum target) ;

private:	
	std::vector<ll_texture_atlas_list_t> mAtlasMap ;
	std::vector<ll_texture_atlas_list_t> mEmptyAtlasMap ; //delay some empty atlases deletion to avoid possible creation of new atlas immediately.
};

#endif
