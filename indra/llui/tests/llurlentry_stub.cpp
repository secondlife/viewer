/**
 * @file llurlentry_stub.cpp
 * @author Martin Reddy
 * @brief Stub implementations for LLUrlEntry unit test dependencies
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
 * this source code is governed by the Linden Lab Source Code Disclosure
 * Agreement ("Agreement") previously entered between you and Linden
 * Lab. By accessing, using, copying, modifying or distributing this
 * software, you acknowledge that you have been informed of your
 * obligations under the Agreement and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llstring.h"
#include "llfile.h"
#include "llavatarnamecache.h"
#include "llcachename.h"
#include "lluuid.h"

#include <string>

// Stub for LLAvatarNameCache
bool LLAvatarNameCache::get(const LLUUID& agent_id, LLAvatarName *av_name)
{
	return false;
}

void LLAvatarNameCache::get(const LLUUID& agent_id, callback_slot_t slot)
{
	return;
}

bool LLAvatarNameCache::useDisplayNames()
{
	return false;
}

//
// Stub implementation for LLCacheName
//
BOOL LLCacheName::getFullName(const LLUUID& id, std::string& fullname)
{
	fullname = "Lynx Linden";
	return TRUE;
}

BOOL LLCacheName::getGroupName(const LLUUID& id, std::string& group)
{
	group = "My Group";
	return TRUE;
}

boost::signals2::connection LLCacheName::get(const LLUUID& id, bool is_group, const LLCacheNameCallback& callback)
{
	return boost::signals2::connection();
}

LLCacheName* gCacheName = NULL;

//
// Stub implementation for LLTrans
//
class LLTrans
{
public:
	static std::string getString(const std::string &xml_desc, const LLStringUtil::format_map_t& args);
};

std::string LLTrans::getString(const std::string &xml_desc, const LLStringUtil::format_map_t& args)
{
	return std::string();
}
