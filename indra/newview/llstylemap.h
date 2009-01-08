/** 
 * @file LLStyleMap.h
 * @brief LLStyleMap class definition
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


#ifndef LL_LLSTYLE_MAP_H
#define LL_LLSTYLE_MAP_H

#include "llstyle.h"
#include "lluuid.h"

// Lightweight class for holding and managing mappings between UUIDs and links.
// Used (for example) to create clickable name links off of IM chat.

typedef std::map<LLUUID, LLStyleSP> style_map_t;

class LLStyleMap : public style_map_t
{
public:
	LLStyleMap();
	~LLStyleMap();
	// Just like the [] accessor but it will add the entry in if it doesn't exist.
	const LLStyleSP &lookupAgent(const LLUUID &source); 
	const LLStyleSP &lookup(const LLUUID &source, const std::string& link); 
	static LLStyleMap &instance();

	// Forces refresh of the entries, call when something changes (e.g. link color).
	void update();
};

#endif  // LL_LLSTYLE_MAP_H
