/** 
 * @file llavatarnamecache.cpp
 * @brief Provides lookup of avatar SLIDs ("bobsmith123") and display names
 * ("James Cook") from avatar UUIDs.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
#include "linden_common.h"

#include "llavatarnamecache.h"

#include "llcachename.h"	// until we get our own web service

#include <cctype>	// tolower()

static std::string slid_from_full_name(const std::string& full_name)
{
	std::string id = full_name;
	std::string::size_type end = id.length();
	for (std::string::size_type i = 0; i < end; ++i)
	{
		if (id[i] == ' ')
			id[i] = '.';
		else
			id[i] = tolower(id[i]);
	}
	return id;
}

void LLAvatarNameCache::initClass()
{
}

void LLAvatarNameCache::cleanupClass()
{
}

void LLAvatarNameCache::importFile(std::istream& istr)
{
}

void LLAvatarNameCache::exportFile(std::ostream& ostr)
{
}

void LLAvatarNameCache::idle()
{
}

bool LLAvatarNameCache::get(const LLUUID& agent_id, LLAvatarName *av_name)
{
	std::string full_name;
	bool found = gCacheName->getFullName(agent_id, full_name);
	if (found)
	{
		av_name->mSLID = slid_from_full_name(full_name);

		std::string display_name;
		if (gCacheName->getDisplayName(agent_id, display_name))
		{
			av_name->mDisplayName = display_name;
		}
		else
		{
			// ...no explicit display name, use legacy name
			av_name->mDisplayName = full_name;
		}
	}
	return found;
}

void LLAvatarNameCache::get(const LLUUID& agent_id, name_cache_callback_t callback)
{
}
