/** 
 * @file lltextureatlasmanager.h
 * @brief LLTextureAtlasManager base class.
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
	LLSINGLETON(LLTextureAtlasManager);
	~LLTextureAtlasManager();
	typedef std::list<LLPointer<LLTextureAtlas> > ll_texture_atlas_list_t ;

public:

	LLPointer<LLTextureAtlasSlot> reserveAtlasSlot(S32 sub_texture_size, S8 ncomponents, 
		LLSpatialGroup* groupp, LLViewerTexture* imagep) ;
	void releaseAtlas(LLTextureAtlas* atlasp);

	BOOL canAddToAtlas(S32 w, S32 h, S8 ncomponents, LLGLenum target) ;

private:	
	std::vector<ll_texture_atlas_list_t> mAtlasMap ;
	std::vector<ll_texture_atlas_list_t> mEmptyAtlasMap ; //delay some empty atlases deletion to avoid possible creation of new atlas immediately.
};

#endif
