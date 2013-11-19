/** 
 * @file lltextureatlas.h
 * @brief LLTextureAtlas base class.
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


#ifndef LL_TEXTUREATLAS_H
#define LL_TEXTUREATLAS_H

#include "llviewertexture.h"
class LLSpatialGroup ;

class LLTextureAtlas : public LLViewerTexture
{
protected:
	/*virtual*/ ~LLTextureAtlas() ;

public:
	LLTextureAtlas(U8 ncomponents, S16 atlas_dim = 16) ;	

	/*virtual*/ S8 getType() const;

	LLGLuint insertSubTexture(LLImageGL* source_gl_tex, S32 discard_level, const LLImageRaw* raw_image, S16 slot_col, S16 slot_row) ;
	void releaseSlot(S16 slot_col, S16 slot_row, S8 slot_width);

	BOOL getNextAvailableSlot(S8 bits_len, S16& col, S16& row) ;
	void getTexCoordOffset(S16 col, S16 row, F32& xoffset, F32& yOffset) ;
	void getTexCoordScale(S32 w, S32 h, F32& xscale, F32& yscale) ;

	BOOL isEmpty() const ;
	BOOL isFull(S8 to_be_reserved = 1) const ;
	F32  getFullness() const ;

	void addSpatialGroup(LLSpatialGroup* groupp) ;
	void removeSpatialGroup(LLSpatialGroup* groupp) ;
	LLSpatialGroup* getLastSpatialGroup() ;
	void removeLastSpatialGroup() ;
	BOOL hasSpatialGroup(LLSpatialGroup* groupp) ;
	void clearSpatialGroup() ;
	std::list<LLSpatialGroup*>* getSpatialGroupList() {return &mSpatialGroupList;}
private:
	void generateEmptyUsageBits() ;
	void releaseUsageBits() ;

	void markUsageBits(S8 bits_len, U8 mask, S16 col, S16 row) ;
	void unmarkUsageBits(S8 bits_len, S16 col, S16 row) ;

	void getPositionFromIndex(S16 index, S16& col, S16& row) ;
	void getIndexFromPosition(S16 col, S16 row, S16& index) ;
	BOOL areUsageBitsMarked(S8 bits_len, U8 mask, S16 col, S16 row) ;

private:	
	S16 mAtlasDim ; //number of slots per edge, i.e, there are "mAtlasDim * mAtlasDim" total slots in the atlas. 
	S16 mNumSlotsReserved ;
	S16 mMaxSlotsInAtlas ;
	U8  **mUsageBits ;	
	std::list<LLSpatialGroup*> mSpatialGroupList ;

public:
	//debug use only
	U8  **mTestBits ;

public:
	static S16 sMaxSubTextureSize ;
	static S16 sSlotSize ;
};

#endif

