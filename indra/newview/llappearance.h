/** 
 * @file llappearance.h
 * @brief LLAppearance class definition
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

#ifndef LL_LLAPPEARANCE_H
#define LL_LLAPPEARANCE_H

#include "lluuid.h"

class LLAppearance
{
public:
	LLAppearance()										{}
	~LLAppearance()										{ mParamMap.clear(); } 

	void	addParam( S32 id, F32 value )				{ mParamMap[id] = value; }
	F32		getParam( S32 id, F32 defval )				{ return get_if_there(mParamMap, id, defval ); }

	void	addTexture( S32 te, const LLUUID& uuid )	{ if( te < LLAvatarAppearanceDefines::TEX_NUM_INDICES ) mTextures[te] = uuid; }
	const LLUUID& getTexture( S32 te )					{ return ( te < LLAvatarAppearanceDefines::TEX_NUM_INDICES ) ? mTextures[te] : LLUUID::null; }
	
	void	clear()										{ mParamMap.clear(); for( S32 i=0; i<LLAvatarAppearanceDefines::TEX_NUM_INDICES; i++ ) mTextures[i].setNull(); }

	typedef std::map<S32, F32> param_map_t;
	param_map_t mParamMap;
	LLUUID	mTextures[LLAvatarAppearanceDefines::TEX_NUM_INDICES];
};

#endif  // LL_LLAPPEARANCE_H
