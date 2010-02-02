/** 
 * @file llurlentry.cpp
 * @author Martin Reddy
 * @brief Describes the Url types that can be registered in LLUrlRegistry
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llurlentryagent.h"

#include "llcachename.h"
#include "lltrans.h"
#include "lluicolortable.h"

//
// LLUrlEntryAgent Describes a Second Life agent Url, e.g.,
// secondlife:///app/agent/0e346d8b-4433-4d66-a6b0-fd37083abc4c/about
//
LLUrlEntryAgent::LLUrlEntryAgent()
{
	mPattern = boost::regex("secondlife:///app/agent/[\\da-f-]+/\\w+",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_agent.xml";
	mIcon = "Generic_Person";
	mColor = LLUIColorTable::instance().getColor("AgentLinkColor");
}

// IDEVO demo code
static std::string clean_name(const std::string& full_name)
{
	std::string displayname;
	if (full_name == "miyazaki23") // IDEVO demo code
	{
		// miyazaki
		displayname += (char)(0xE5);
		displayname += (char)(0xAE);
		displayname += (char)(0xAE);
		displayname += (char)(0xE5);
		displayname += (char)(0xB4);
		displayname += (char)(0x8E);
		// hayao
		displayname += (char)(0xE9);
		displayname += (char)(0xA7);
		displayname += (char)(0xBF);
		// san
		displayname += (char)(0xE3);
		displayname += (char)(0x81);
		displayname += (char)(0x95);
		displayname += (char)(0xE3);
		displayname += (char)(0x82);
		displayname += (char)(0x93);
	}
	else if (full_name == "Jim Linden")
	{
		displayname = "Jos";
		displayname += (char)(0xC3);
		displayname += (char)(0xA9);
		displayname += " Sanchez";
	}
	else if (full_name == "James Linden")
	{
		displayname = "James Cook";
	}

	std::string final;
	if (!displayname.empty())
	{
		final = displayname + " (" + full_name + ")";
	}
	else
	{
		final = full_name;
	}
	return final;
}

void LLUrlEntryAgent::onNameCache(const LLUUID& id,
								  const std::string& full_name,
								  bool is_group)
{
	std::string final = clean_name(full_name);
	// received the agent name from the server - tell our observers
	callObservers(id.asString(), final);
}

std::string LLUrlEntryAgent::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	if (!gCacheName)
	{
		// probably at the login screen, use short string for layout
		return LLTrans::getString("LoadingData");
	}

	std::string agent_id_string = getIDStringFromUrl(url);
	if (agent_id_string.empty())
	{
		// something went wrong, just give raw url
		return unescapeUrl(url);
	}

	LLUUID agent_id(agent_id_string);
	std::string full_name;
	if (agent_id.isNull())
	{
		return LLTrans::getString("AvatarNameNobody");
	}
	else if (gCacheName->getFullName(agent_id, full_name))
	{
		return clean_name(full_name);
	}
	else
	{
		gCacheName->get(agent_id, false,
			boost::bind(&LLUrlEntryAgent::onNameCache,
				this, _1, _2, _3));
		addObserver(agent_id_string, url, cb);
		return LLTrans::getString("LoadingData");
	}
}
