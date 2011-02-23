/** 
 * @file llshareavatarhandler.cpp
 * @brief slapp to handle sharing with an avatar
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

#include "llviewerprecompiledheaders.h"
#include "llcommandhandler.h"
#include "llavataractions.h"

class LLShareWithAvatarHandler : public LLCommandHandler
{
public: 
	// requires trusted browser to trigger
	LLShareWithAvatarHandler() : LLCommandHandler("sharewithavatar", UNTRUSTED_THROTTLE) 
	{ 
	}
	
	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		//Make sure we have some parameters
		if (params.size() == 0)
		{
			return false;
		}
		
		//Get the ID
		LLUUID id;
		if (!id.set( params[0], FALSE ))
		{
			return false;
		}
		
		//instigate share with this avatar
		LLAvatarActions::share( id );		
		return true;
	}
};
LLShareWithAvatarHandler gShareWithAvatar;
