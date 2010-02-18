/** 
 * @file llavatarnamecache.h
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
#ifndef LLAVATARNAMECACHE_H
#define LLAVATARNAMECACHE_H

#include "llavatarname.h"	// for convenience

namespace LLAvatarNameCache
{
	void initClass();
	void cleanupClass();

	void importFile(std::istream& istr);
	void exportFile(std::ostream& ostr);

	// Periodically makes a batch request for display names not already in
	// cache.  Call once per frame.
	void idle();

	// If name is in cache, returns true and fills in provided LLAvatarName
	// otherwise returns false
	bool get(const LLUUID& agent_id, LLAvatarName *av_name);

	// Fetches name information and calls callback.
	// If name information is in cache, callback will be called immediately.
	typedef void (*name_cache_callback_t)(const LLUUID& agent_id, const LLAvatarName& av_name);
	void get(const LLUUID& agent_id, name_cache_callback_t callback);
}

#endif
