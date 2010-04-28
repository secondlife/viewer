/** 
 * @file llurlaction.cpp
 * @author Martin Reddy
 * @brief A set of actions that can performed on Urls
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

#include "linden_common.h"

#include "llurlaction.h"
#include "llview.h"
#include "llwindow.h"
#include "llurlregistry.h"

// global state for the callback functions
void (*LLUrlAction::sOpenURLCallback) (const std::string& url) = NULL;
void (*LLUrlAction::sOpenURLInternalCallback) (const std::string& url) = NULL;
void (*LLUrlAction::sOpenURLExternalCallback) (const std::string& url) = NULL;
bool (*LLUrlAction::sExecuteSLURLCallback) (const std::string& url) = NULL;


void LLUrlAction::setOpenURLCallback(void (*cb) (const std::string& url))
{
	sOpenURLCallback = cb;
}

void LLUrlAction::setOpenURLInternalCallback(void (*cb) (const std::string& url))
{
	sOpenURLInternalCallback = cb;
}

void LLUrlAction::setOpenURLExternalCallback(void (*cb) (const std::string& url))
{
	sOpenURLExternalCallback = cb;
}

void LLUrlAction::setExecuteSLURLCallback(bool (*cb) (const std::string& url))
{
	sExecuteSLURLCallback = cb;
}

void LLUrlAction::openURL(std::string url)
{
	if (sOpenURLCallback)
	{
		(*sOpenURLCallback)(url);
	}
}

void LLUrlAction::openURLInternal(std::string url)
{
	if (sOpenURLInternalCallback)
	{
		(*sOpenURLInternalCallback)(url);
	}
}

void LLUrlAction::openURLExternal(std::string url)
{
	if (sOpenURLExternalCallback)
	{
		(*sOpenURLExternalCallback)(url);
	}
}

void LLUrlAction::executeSLURL(std::string url)
{
	if (sExecuteSLURLCallback)
	{
		(*sExecuteSLURLCallback)(url);
	}
}

void LLUrlAction::clickAction(std::string url)
{
	// Try to handle as SLURL first, then http Url
	if ( (sExecuteSLURLCallback) && !(*sExecuteSLURLCallback)(url) )
	{
		if (sOpenURLCallback)
		{
			(*sOpenURLCallback)(url);
		}
	}
}

void LLUrlAction::teleportToLocation(std::string url)
{
	LLUrlMatch match;
	if (LLUrlRegistry::instance().findUrl(url, match))
	{
		if (! match.getLocation().empty())
		{
			executeSLURL("secondlife:///app/teleport/" + match.getLocation());
		}
	}	
}

void LLUrlAction::showLocationOnMap(std::string url)
{
	LLUrlMatch match;
	if (LLUrlRegistry::instance().findUrl(url, match))
	{
		if (! match.getLocation().empty())
		{
			executeSLURL("secondlife:///app/worldmap/" + match.getLocation());
		}
	}	
}

void LLUrlAction::copyURLToClipboard(std::string url)
{
	LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(url));
}

void LLUrlAction::copyLabelToClipboard(std::string url)
{
	LLUrlMatch match;
	if (LLUrlRegistry::instance().findUrl(url, match))
	{
		LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(match.getLabel()));
	}	
}

void LLUrlAction::showProfile(std::string url)
{
	// Get id from 'secondlife:///app/{cmd}/{id}/{action}'
	// and show its profile
	LLURI uri(url);
	LLSD path_array = uri.pathArray();
	if (path_array.size() == 4)
	{
		std::string id_str = path_array.get(2).asString();
		if (LLUUID::validate(id_str))
		{
			std::string cmd_str = path_array.get(1).asString();
			executeSLURL("secondlife:///app/" + cmd_str + "/" + id_str + "/about");
		}
	}
}
