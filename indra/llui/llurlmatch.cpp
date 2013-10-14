/** 
 * @file llurlmatch.cpp
 * @author Martin Reddy
 * @brief Specifies a matched Url in a string, as returned by LLUrlRegistry
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "linden_common.h"
#include "llurlmatch.h"

LLUrlMatch::LLUrlMatch() :
	mStart(0),
	mEnd(0),
	mUrl(""),
	mLabel(""),
	mTooltip(""),
	mIcon(""),
	mMenuName(""),
	mLocation(""),
	mUnderlineOnHoverOnly(false)
{
}

void LLUrlMatch::setValues(U32 start, U32 end, const std::string &url,
						   const std::string &label, const std::string &tooltip,
						   const std::string &icon, const LLStyle::Params& style,
						   const std::string &menu, const std::string &location,
						   const LLUUID& id, bool underline_on_hover_only)
{
	mStart = start;
	mEnd = end;
	mUrl = url;
	mLabel = label;
	mTooltip = tooltip;
	mIcon = icon;
	mStyle = style;
	mStyle.link_href = url;
	mMenuName = menu;
	mLocation = location;
	mID = id;
	mUnderlineOnHoverOnly = underline_on_hover_only;
}
