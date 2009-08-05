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
#include "llviewercontrol.h"
#include "llagent.h"

LLStyleMap::LLStyleMap()
{
}

LLStyleMap::~LLStyleMap()
{
}

LLStyleMap &LLStyleMap::instance()
{
	static LLStyleMap style_map;
	return style_map;
}

// This is similar to the [] accessor except that if the entry doesn't already exist,
// then this will create the entry.
const LLStyleSP &LLStyleMap::lookupAgent(const LLUUID &source)
{
	// Find this style in the map or add it if not.  This map holds links to residents' profiles.
	if (find(source) == end())
	{
		LLStyleSP style(new LLStyle);
		style->setVisible(true);
		style->setFontName(LLStringUtil::null);
		if (source != LLUUID::null && source != gAgent.getID() )
		{
			style->setColor(LLUIColorTable::instance().getColor("HTMLLinkColor"));
			std::string link = llformat("secondlife:///app/agent/%s/about",source.asString().c_str());
			style->setLinkHREF(link);
		}
		else
		{
			// Make the resident's own name white and don't make the name clickable.
			style->setColor(LLColor4::white);
		}
		(*this)[source] = style;
	}
	return (*this)[source];
}

// This is similar to lookupAgent for any generic URL encoded style.
const LLStyleSP &LLStyleMap::lookup(const LLUUID& id, const std::string& link)
{
	// Find this style in the map or add it if not.
	iterator iter = find(id);
	if (iter == end())
	{
		LLStyleSP style(new LLStyle);
		style->setVisible(true);
		style->setFontName(LLStringUtil::null);
		if (id != LLUUID::null && !link.empty())
		{
			style->setColor(LLUIColorTable::instance().getColor("HTMLLinkColor"));
			style->setLinkHREF(link);
		}
		else
			style->setColor(LLColor4::white);
		(*this)[id] = style;
	}
	else 
	{
		LLStyleSP style = (*iter).second;
		if ( style->getLinkHREF() != link )
		{
			style->setLinkHREF(link);
		}
	}

	return (*this)[id];
}

void LLStyleMap::update()
{
	for (style_map_t::iterator iter = begin(); iter != end(); ++iter)
	{
		LLStyleSP &style = iter->second;
		// Update the link color in case it has been changed.
		style->setColor(LLUIColorTable::instance().getColor("HTMLLinkColor"));
	}
}
