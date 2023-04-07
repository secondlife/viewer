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
#include "llhttpconstants.h"
#include "llmediactrl.h"
#include "llnotificationsutil.h"
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
	LLSearchHandler() : LLCommandHandler("search", UNTRUSTED_CLICK_ONLY) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (!LLUI::getInstance()->mSettingGroups["config"]->getBOOL("EnableSearch"))
		{
			LLNotificationsUtil::add("NoSearch", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
			return true;
		}

		const size_t parts = tokens.size();

		// get the (optional) category for the search
		std::string collection;
		if (parts > 0)
		{
            collection = tokens[0].asString();
		}

		// get the (optional) search string
		std::string search_text;
		if (parts > 1)
		{
			search_text = tokens[1].asString();
		}

		// create the LLSD arguments for the search floater
		LLFloaterSearch::Params p;
		p.search.collection = collection;
		p.search.query = LLURI::unescape(search_text);

		// open the search floater and perform the requested search
		LLFloaterReg::showInstance("search", p);
		return true;
	}
};
LLSearchHandler gSearchHandler;

LLFloaterSearch::SearchQuery::SearchQuery()
:   category("category", ""),
    collection("collection", ""),
    query("query")
{}

LLFloaterSearch::LLFloaterSearch(const Params& key) :
	LLFloaterWebContent(key),
	mSearchGodLevel(0)
{
	// declare a map that transforms a category name into
	// the URL suffix that is used to search that category

    mSearchType.insert("standard");
    mSearchType.insert("land");
    mSearchType.insert("classified");

    mCollectionType.insert("events");
    mCollectionType.insert("destinations");
    mCollectionType.insert("places");
    mCollectionType.insert("groups");
    mCollectionType.insert("people");
}

BOOL LLFloaterSearch::postBuild()
{
	LLFloaterWebContent::postBuild();
	mWebBrowser->addObserver(this);

	return TRUE;
}

void LLFloaterSearch::onOpen(const LLSD& key)
{
	Params p(key);
	p.trusted_content = true;
	p.allow_address_entry = false;

	LLFloaterWebContent::onOpen(p);
	mWebBrowser->setFocus(TRUE);
	search(p.search);
}

void LLFloaterSearch::onClose(bool app_quitting)
{
	LLFloaterWebContent::onClose(app_quitting);
	// tear down the web view so we don't show the previous search
	// result when the floater is opened next time
	destroy();
}

void LLFloaterSearch::godLevelChanged(U8 godlevel)
{
	// search results can change based upon god level - if the user
	// changes god level, then give them a warning (we don't refresh
	// the search as this might undo any page navigation or
	// AJAX-driven changes since the last search).
	
	//FIXME: set status bar text

	//getChildView("refresh_search")->setVisible( (godlevel != mSearchGodLevel));
}

void LLFloaterSearch::search(const SearchQuery &p)
{
	if (! mWebBrowser || !p.validateBlock())
	{
		return;
	}

	// reset the god level warning as we're sending the latest state
	getChildView("refresh_search")->setVisible(FALSE);
	mSearchGodLevel = gAgent.getGodLevel();

	// work out the subdir to use based on the requested category
	LLSD subs;
	if (mSearchType.find(p.category) != mSearchType.end())
	{
		subs["TYPE"] = p.category;
	}
	else
	{
		subs["TYPE"] = "standard";
	}

	// add the search query string
	subs["QUERY"] = LLURI::escape(p.query);

    subs["COLLECTION"] = "";
    if (subs["TYPE"] == "standard")
    {
        if (mCollectionType.find(p.collection) != mCollectionType.end())
        {
            subs["COLLECTION"] = "&collection_chosen=" + std::string(p.collection);
        }
        else
        {
            std::string collection_args("");
            for (std::set<std::string>::iterator it = mCollectionType.begin(); it != mCollectionType.end(); ++it)
            {
                collection_args += "&collection_chosen=" + std::string(*it);
            }
            subs["COLLECTION"] = collection_args;
        }
    }

	// add the user's preferred maturity (can be changed via prefs)
	std::string maturity;
	if (gAgent.prefersAdult())
	{
		maturity = "gma";  // PG,Mature,Adult
	}
	else if (gAgent.prefersMature())
	{
		maturity = "gm";  // PG,Mature
	}
	else
	{
		maturity = "g";  // PG
	}
	subs["MATURITY"] = maturity;

	// add the user's god status
	subs["GODLIKE"] = gAgent.isGodlike() ? "1" : "0";

	// get the search URL and expand all of the substitutions
	// (also adds things like [LANGUAGE], [VERSION], [OS], etc.)
	std::string url = gSavedSettings.getString("SearchURL");
	url = LLWeb::expandURLSubstitutions(url, subs);

	// and load the URL in the web view
	mWebBrowser->navigateTo(url, HTTP_CONTENT_TEXT_HTML);
}
