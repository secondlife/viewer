/** 
 * @file llstylemap.cpp
 * @brief LLStyleMap class implementation
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
		if (source != LLUUID::null && source != gAgent.getID() )
		{
			style_params.color.control = "HTMLLinkColor";
			style_params.readonly_color.control = "HTMLLinkColor";
			style_params.link_href = 
					LLSLURL("agent", source, "inspect").getSLURLString();
		}
		else
		{
			// Make the resident's own name white and don't make the name clickable.
			style_params.color = LLColor4::white;
			style_params.readonly_color = LLColor4::white;
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

