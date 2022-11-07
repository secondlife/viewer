/** 
 * @file LLStyleMap.h
 * @brief LLStyleMap class definition
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


#ifndef LL_LLSTYLE_MAP_H
#define LL_LLSTYLE_MAP_H

#include "llstyle.h"
#include "lluuid.h"
#include "llsingleton.h"

// Lightweight class for holding and managing mappings between UUIDs and links.
// Used (for example) to create clickable name links off of IM chat.

typedef std::map<LLUUID, LLStyle::Params> style_map_t;

class LLStyleMap : public LLSingleton<LLStyleMap>
{
    LLSINGLETON_EMPTY_CTOR(LLStyleMap);
public:
    // Just like the [] accessor but it will add the entry in if it doesn't exist.
    const LLStyle::Params &lookupAgent(const LLUUID &source); 
    const LLStyle::Params &lookup(const LLUUID &source, const std::string& link); 

private:
    style_map_t mMap;
};

#endif  // LL_LLSTYLE_MAP_H
