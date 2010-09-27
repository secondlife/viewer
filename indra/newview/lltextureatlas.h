/** 
 * @file lltextureatlas.h
 * @brief LLTextureAtlas base class.
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

