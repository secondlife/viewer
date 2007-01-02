/** 
 * @file llappearance.h
 * @brief LLAppearance class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLAPPEARANCE_H
#define LL_LLAPPEARANCE_H

#include "llskiplist.h"
#include "lluuid.h"

class LLAppearance
{
public:
	LLAppearance()										{}
	~LLAppearance()										{ mParamMap.deleteAllData(); } 

	void	addParam( S32 id, F32 value )				{ mParamMap.addData( id, new F32(value) ); }
	F32*	getParam( S32 id )							{ F32* temp = mParamMap.getIfThere( id ); return temp; } // temp works around an invalid warning.

	void	addTexture( S32 te, const LLUUID& uuid )	{ if( te < LLVOAvatar::TEX_NUM_ENTRIES ) mTextures[te] = uuid; }
	const LLUUID& getTexture( S32 te )					{ return ( te < LLVOAvatar::TEX_NUM_ENTRIES ) ? mTextures[te] : LLUUID::null; }
	
	void	clear()										{ mParamMap.deleteAllData(); for( S32 i=0; i<LLVOAvatar::TEX_NUM_ENTRIES; i++ ) mTextures[i].setNull(); }

	LLPtrSkipMap<S32, F32*> mParamMap;
	LLUUID	mTextures[LLVOAvatar::TEX_NUM_ENTRIES];
};

#endif  // LL_LLAPPEARANCE_H
