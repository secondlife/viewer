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

#include "llappviewer.h"
#include "llbase64.h"
#include "llcommandhandler.h"
#include "llfloaterreg.h"
#include "llfloatersearch.h"
#include "llmediactrl.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llplugincookiestore.h"
#include "lllogininstance.h"
#include "lluri.h"
#include "llagent.h"
#include "llsdserialize.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llversioninfo.h"
#include "llviewermedia.h"
#include "llviewernetwork.h"
#include "llviewerparcelmgr.h"
#include "llweb.h"

// support secondlife:///app/search/{CATEGORY}/{QUERY} SLapps
class LLSearchHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLSearchHandler() : LLCommandHandler("search", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (!LLUI::sSettingGroups["config"]->getBOOL("EnableSearch"))
		{
			LLNotificationsUtil::add("NoSearch", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
			return true;
		}

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
		LLFloaterSearch::Params p;
		p.search.category = category;
		p.search.query = LLURI::unescape(search_text);

		// open the search floater and perform the requested search
		LLFloaterReg::showInstance("search", p);
		return true;
	}
};
LLSearchHandler gSearchHandler;

LLFloaterSearch::SearchQuery::SearchQuery()
:	category("category", ""),
	query("query")
{}

LLFloaterSearch::LLFloaterSearch(const Params& key) :
	LLFloaterWebContent(key),
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
	if (mCategoryPaths.has(p.category))
	{
		subs["CATEGORY"] = mCategoryPaths[p.category].asString();
	}
	else
	{
		subs["CATEGORY"] = mCategoryPaths["all"].asString();
	}

	// add the search query string
	subs["QUERY"] = LLURI::escape(p.query);

	// add the permissions token that login.cgi gave us
	// We use "search_token", and fallback to "auth_token" if not present.
	LLSD search_cookie;

	LLSD search_token = LLLoginInstance::getInstance()->getResponse("search_token");
	if (search_token.asString().empty())
	{
		search_token = LLLoginInstance::getInstance()->getResponse("auth_token");
	}
	search_cookie["AUTH_TOKEN"] = search_token.asString();

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
	search_cookie["MATURITY"] = maturity;

	// add the user's god status
	search_cookie["GODLIKE"] = gAgent.isGodlike() ? "1" : "0";
	search_cookie["VERSION"] = LLVersionInfo::getVersion();
	search_cookie["VERSION_MAJOR"] = LLVersionInfo::getMajor();
	search_cookie["VERSION_MINOR"] = LLVersionInfo::getMinor();
	search_cookie["VERSION_PATCH"] = LLVersionInfo::getPatch();
	search_cookie["VERSION_BUILD"] = LLVersionInfo::getBuild();
	search_cookie["CHANNEL"] = LLVersionInfo::getChannel();
	search_cookie["GRID"] = LLGridManager::getInstance()->getGridLabel();
	search_cookie["OS"] = LLAppViewer::instance()->getOSInfo().getOSStringSimple();
	search_cookie["SESSION_ID"] = gAgent.getSessionID();
	search_cookie["FIRST_LOGIN"] = gAgent.isFirstLogin();

	std::string lang = LLUI::getLanguage();
	if (lang == "en-us")
	{
		lang = "en";
	}
	search_cookie["LANGUAGE"] = lang;

	// find the region ID
	LLUUID region_id;
	LLViewerRegion *region = gAgent.getRegion();
	if (region)
	{
		region_id = region->getRegionID();
	}
	search_cookie["REGION_ID"] = region_id;

	// find the parcel local ID
	S32 parcel_id = 0;
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (parcel)
	{
		parcel_id = parcel->getLocalID();
	}
	search_cookie["PARCEL_ID"] = llformat("%d", parcel_id);

	std::stringstream cookie_string_stream;
	LLSDSerialize::toXML(search_cookie, cookie_string_stream);
	std::string cookie_string = cookie_string_stream.str();

	U8* cookie_string_buffer = (U8*)cookie_string.c_str();
	std::string cookie_value = LLBase64::encode(cookie_string_buffer, cookie_string.size());

	// for staging services
	LLViewerMedia::getCookieStore()->setCookiesFromHost(std::string("viewer_session_info=") + cookie_value, ".lindenlab.com");
	// for live services
	LLViewerMedia::getCookieStore()->setCookiesFromHost(std::string("viewer_session_info=") + cookie_value, ".secondlife.com");

	// get the search URL and expand all of the substitutions
	// (also adds things like [LANGUAGE], [VERSION], [OS], etc.)
	std::string url = gSavedSettings.getString("SearchURL");
	url = LLWeb::expandURLSubstitutions(url, subs);

	// and load the URL in the web view
	mWebBrowser->navigateTo(url, "text/html");
}
