/** 
 * @file llstylemap.cpp
 * @brief LLStyleMap class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llstylemap.h"

#include "llstring.h"
#include "llui.h"
#include "llslurl.h"
#include "llviewercontrol.h"
#include "llagent.h"

const LLStyle::Params &LLStyleMap::lookupAgent(const LLUUID &source)
{
    // Find this style in the map or add it if not.  This map holds links to residents' profiles.
    if (mMap.find(source) == mMap.end())
    {
        LLStyle::Params style_params;
        if (source != LLUUID::null)
        {
            style_params.color.control = "HTMLLinkColor";
            style_params.readonly_color.control = "HTMLLinkColor";
            style_params.link_href = LLSLURL("agent", source, "inspect").getSLURLString();
        }
        mMap[source] = style_params;
    }
    return mMap[source];
}

// This is similar to lookupAgent for any generic URL encoded style.
const LLStyle::Params &LLStyleMap::lookup(const LLUUID& id, const std::string& link)
{
    // Find this style in the map or add it if not.
    style_map_t::iterator iter = mMap.find(id);
    if (iter == mMap.end())
    {
        LLStyle::Params style_params;

        if (id != LLUUID::null && !link.empty())
        {
            style_params.color.control = "HTMLLinkColor";
            style_params.readonly_color.control = "HTMLLinkColor";
            style_params.link_href = link;
        }
        else
        {
            style_params.color = LLColor4::white;
            style_params.readonly_color = LLColor4::white;
        }
        mMap[id] = style_params;
    }
    else 
    {
        iter->second.link_href = link;
    }

    return mMap[id];
}

