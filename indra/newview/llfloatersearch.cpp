/** 
 * @file llfloatersearch.cpp
 * @author Martin Reddy
 * @brief Search floater - uses an embedded web browser control
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

#include "llviewerprecompiledheaders.h"

#include "llcommandhandler.h"
#include "llfloaterreg.h"
#include "llfloatersearch.h"
#include "llmediactrl.h"
#include "lllogininstance.h"
#include "lluri.h"
#include "llagent.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llweb.h"

// support secondlife:///app/search/{CATEGORY}/{QUERY} SLapps
class LLSearchHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLSearchHandler() : LLCommandHandler("search", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		const size_t parts = tokens.size();

		// get the (optional) category for the search
		std::string category;
		if (parts > 0)
		{
			category = tokens[0].asString();
		}

		// get the (optional) search string
		std::string search_text;
		if (parts > 1)
		{
			search_text = tokens[1].asString();
		}

		// create the LLSD arguments for the search floater
		LLSD args;
		args["category"] = category;
		args["id"] = LLURI::unescape(search_text);

		// open the search floater and perform the requested search
		LLFloaterReg::showInstance("search", args);
		return true;
	}
};
LLSearchHandler gSearchHandler;

LLFloaterSearch::LLFloaterSearch(const LLSD& key) :
	LLFloater(key),
	LLViewerMediaObserver(),
	mBrowser(NULL),
	mSearchGodLevel(0)
{
	// declare a map that transforms a category name into
	// the URL suffix that is used to search that category
	mCategoryPaths = LLSD::emptyMap();
	mCategoryPaths["all"]          = "search";
	mCategoryPaths["people"]       = "search/people";
	mCategoryPaths["places"]       = "search/places";
	mCategoryPaths["events"]       = "search/events";
	mCategoryPaths["groups"]       = "search/groups";
	mCategoryPaths["wiki"]         = "search/wiki";
	mCategoryPaths["destinations"] = "destinations";
	mCategoryPaths["classifieds"]  = "classifieds";
}

BOOL LLFloaterSearch::postBuild()
{
	mBrowser = getChild<LLMediaCtrl>("browser");
	mBrowser->addObserver(this);

	return TRUE;
}

void LLFloaterSearch::onOpen(const LLSD& key)
{
	search(key);
}

void LLFloaterSearch::onClose(bool app_quitting)
{
	// tear down the web view so we don't show the previous search
	// result when the floater is opened next time
	destroy();
}

void LLFloaterSearch::handleMediaEvent(LLPluginClassMedia *self, EMediaEvent event)
{
	switch (event) 
	{
	case MEDIA_EVENT_NAVIGATE_BEGIN:
		getChild<LLUICtrl>("status_text")->setValue(getString("loading_text"));
		break;
		
	case MEDIA_EVENT_NAVIGATE_COMPLETE:
		getChild<LLUICtrl>("status_text")->setValue(getString("done_text"));
		break;

	default:
		break;
	}
}

void LLFloaterSearch::godLevelChanged(U8 godlevel)
{
	// search results can change based upon god level - if the user
	// changes god level, then give them a warning (we don't refresh
	// the search as this might undo any page navigation or
	// AJAX-driven changes since the last search).
	getChildView("refresh_search")->setVisible( (godlevel != mSearchGodLevel));
}

void LLFloaterSearch::search(const LLSD &key)
{
	if (! mBrowser)
	{
		return;
	}

	// reset the god level warning as we're sending the latest state
	getChildView("refresh_search")->setVisible(FALSE);
	mSearchGodLevel = gAgent.getGodLevel();

	// work out the subdir to use based on the requested category
	LLSD subs;
	std::string category = key.has("category") ? key["category"].asString() : "";
	if (mCategoryPaths.has(category))
	{
		subs["CATEGORY"] = mCategoryPaths[category].asString();
	}
	else
	{
		subs["CATEGORY"] = mCategoryPaths["all"].asString();
	}

	// add the search query string
	std::string search_text = key.has("id") ? key["id"].asString() : "";
	subs["QUERY"] = LLURI::escape(search_text);

	// add the permissions token that login.cgi gave us
	// We use "search_token", and fallback to "auth_token" if not present.
	LLSD search_token = LLLoginInstance::getInstance()->getResponse("search_token");
	if (search_token.asString().empty())
	{
		search_token = LLLoginInstance::getInstance()->getResponse("auth_token");
	}
	subs["AUTH_TOKEN"] = search_token.asString();

	// add the user's preferred maturity (can be changed via prefs)
	std::string maturity;
	if (gAgent.prefersAdult())
	{
		maturity = "42";  // PG,Mature,Adult
	}
	else if (gAgent.prefersMature())
	{
		maturity = "21";  // PG,Mature
	}
	else
	{
		maturity = "13";  // PG
	}
	subs["MATURITY"] = maturity;

	// add the user's god status
	subs["GODLIKE"] = gAgent.isGodlike() ? "1" : "0";

	// get the search URL and expand all of the substitutions
	// (also adds things like [LANGUAGE], [VERSION], [OS], etc.)
	std::string url = gSavedSettings.getString("SearchURL");
	url = LLWeb::expandURLSubstitutions(url, subs);

	// and load the URL in the web view
	mBrowser->navigateTo(url);
}
