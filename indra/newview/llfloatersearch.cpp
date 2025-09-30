/**
 * @file llfloatersearch.cpp
 * @brief Floater for Search (update 2025, preload)
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloatersearch.h"

#include "llagent.h"
#include "llcommandhandler.h"
#include "llfloaterreg.h"
#include "llmediactrl.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llweb.h"

// support secondlife:///app/search/{CATEGORY}/{QUERY} SLapps
class LLSearchHandler : public LLCommandHandler {
    public:
        // requires trusted browser to trigger
        LLSearchHandler() : LLCommandHandler("search", UNTRUSTED_CLICK_ONLY) { }
        bool handle(const LLSD& tokens, const LLSD& query_map, const std::string& grid, LLMediaCtrl* web) {
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

            // open the search floater and perform the requested search
            LLFloaterReg::showInstance("search", llsd::map("collection", collection,"query", search_text));
            return true;
        }
};
LLSearchHandler gSearchHandler;

LLFloaterSearch::LLFloaterSearch(const LLSD& key)
    : LLFloaterWebContent(key)
{
    mSearchType.insert("standard");
    mSearchType.insert("land");
    mSearchType.insert("classified");

    mCollectionType.insert("events");
    mCollectionType.insert("destinations");
    mCollectionType.insert("places");
    mCollectionType.insert("groups");
    mCollectionType.insert("people");
}

LLFloaterSearch::~LLFloaterSearch()
{
}

void LLFloaterSearch::onOpen(const LLSD& tokens)
{
    initiateSearch(tokens);
    mWebBrowser->setFocus(true);
}

// just to override LLFloaterWebContent
void LLFloaterSearch::onClose(bool app_quitting)
{
}

void LLFloaterSearch::initiateSearch(const LLSD& tokens)
{
    std::string url = gSavedSettings.getString("SearchURL");

    LLSD subs;

    // Setting this substitution here results in a full set of collections being
    // substituted into the final URL using the logic from the original search.
    subs["TYPE"] = "standard";

    std::string collection = tokens.has("collection") ? tokens["collection"].asString() : "";

    std::string search_text = tokens.has("query") ? tokens["query"].asString() : "";

    std::string category = tokens.has("category") ? tokens["category"].asString() : "";
    if (mSearchType.find(category) != mSearchType.end())
    {
        subs["TYPE"] = category;
    }
    else
    {
        subs["TYPE"] = "standard";
    }

    subs["QUERY"] = LLURI::escape(search_text);

    subs["COLLECTION"] = "";
    if (subs["TYPE"] == "standard")
    {
        if (mCollectionType.find(collection) != mCollectionType.end())
        {
            subs["COLLECTION"] = "&collection_chosen=" + std::string(collection);
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

    // Default to PG
    std::string maturity = "g";
    if (gAgent.prefersAdult())
    {
        // PG,Mature,Adult
        maturity = "gma";
    }
    else if (gAgent.prefersMature())
    {
        // PG,Mature
        maturity = "gm";
    }
    subs["MATURITY"] = maturity;

    // God status
    subs["GODLIKE"] = gAgent.isGodlike() ? "1" : "0";

    // This call expands a set of generic substitutions like language, viewer version
    // etc. and then also does the same with the list of subs passed in.
    url = LLWeb::expandURLSubstitutions(url, subs);

    // Naviation to the calculated URL - we know it's HTML so we can
    // tell the media system not to bother with the MIME type check.
    LLMediaCtrl* search_browser = findChild<LLMediaCtrl>("search_contents");
    search_browser->navigateTo(url, HTTP_CONTENT_TEXT_HTML);
}

bool LLFloaterSearch::postBuild()
{
    LLFloaterWebContent::postBuild();
    mWebBrowser = getChild<LLMediaCtrl>("search_contents");
    mWebBrowser->addObserver(this);
    getChildView("address")->setEnabled(false);
    getChildView("popexternal")->setEnabled(false);

    // This call is actioned by the preload code in llViewerWindow
    // that creates the search floater during the login process
    // using a generic search with no query
    initiateSearch(LLSD());

    return true;
}
