/**
 * @file llweb.cpp
 * @brief Functions dealing with web browsers
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llweb.h"

// Library includes
#include "llwindow.h"   // spawnWebBrowser()

#include "llagent.h"
#include "llappviewer.h"
#include "llfloaterwebcontent.h"
#include "llfloaterreg.h"
#include "lllogininstance.h"
#include "llparcel.h"
#include "llregex.h"
#include "llsd.h"
#include "llui.h"
#include "lluri.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llnotificationsutil.h"
#include "lluriparser.h"
#include "uriparser/Uri.h"

bool on_load_url_external_response(const LLSD& notification, const LLSD& response, bool async );


// static
void LLWeb::loadURL(const std::string& url, const std::string& target, const std::string& uuid)
{
    if(target == "_internal")
    {
        // Force load in the internal browser, as if with a blank target.
        loadURLInternal(url, "", uuid);
    }

    else if (useExternalBrowser(url) || (target == "_external"))
    {
        loadURLExternal(url);
    }
    else
    {
        loadURLInternal(url, target, uuid);
    }
}

// static
// Explicitly open a Web URL using the Web content floater
void LLWeb::loadURLInternal(const std::string &url, const std::string& target, const std::string& uuid, bool dev_mode)
{
    LLFloaterWebContent::Params p;
    p.url(url).target(target).id(uuid).dev_mode(dev_mode);
    LLFloaterReg::showInstance("web_content", p);
}

// static
void LLWeb::loadURLExternal(const std::string& url, const std::string& uuid)
{
    loadURLExternal(url, true, uuid);
}

// static
void LLWeb::loadURLExternal(const std::string& url, bool async, const std::string& uuid)
{
    // Act like the proxy window was closed, since we won't be able to track targeted windows in the external browser.
    LLViewerMedia::getInstance()->proxyWindowClosed(uuid);

    if(gSavedSettings.getBOOL("DisableExternalBrowser"))
    {
        // Don't open an external browser under any circumstances.
        LL_WARNS() << "Blocked attempt to open external browser." << LL_ENDL;
        return;
    }

    LLSD payload;
    payload["url"] = url;
    LLNotificationsUtil::add( "WebLaunchExternalTarget", LLSD(), payload, boost::bind(on_load_url_external_response, _1, _2, async));
}

// static
bool on_load_url_external_response(const LLSD& notification, const LLSD& response, bool async )
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if ( 0 == option )
    {
        LLSD payload = notification["payload"];
        std::string url = payload["url"].asString();
        std::string escaped_url = LLWeb::escapeURL(url);
        if (gViewerWindow)
        {
            gViewerWindow->getWindow()->spawnWebBrowser(escaped_url, async);
        }
    }
    return false;
}


// static
std::string LLWeb::escapeURL(const std::string& url)
{
    // The CURL curl_escape() function escapes colons, slashes,
    // and all characters but A-Z and 0-9.  Do a cheesy mini-escape.
    std::string escaped_url;
    S32 len = url.length();
    for (S32 i = 0; i < len; i++)
    {
        char c = url[i];
        if (c == ' ')
        {
            escaped_url += "%20";
        }
        else if (c == '\\')
        {
            escaped_url += "%5C";
        }
        else
        {
            escaped_url += c;
        }
    }
    return escaped_url;
}

//static
std::string LLWeb::expandURLSubstitutions(const std::string &url,
                                          const LLSD &default_subs)
{
    LLSD substitution = default_subs;
    substitution["VERSION"] = LLVersionInfo::instance().getVersion();
    substitution["VERSION_MAJOR"] = LLVersionInfo::instance().getMajor();
    substitution["VERSION_MINOR"] = LLVersionInfo::instance().getMinor();
    substitution["VERSION_PATCH"] = LLVersionInfo::instance().getPatch();
    substitution["VERSION_BUILD"] = std::to_string(LLVersionInfo::instance().getBuild());
    substitution["CHANNEL"] = LLVersionInfo::instance().getChannel();
    substitution["GRID"] = LLGridManager::getInstance()->getGridId();
    substitution["GRID_LOWERCASE"] = utf8str_tolower(LLGridManager::getInstance()->getGridId());
    substitution["OS"] = LLOSInfo::instance().getOSStringSimple();
    substitution["SESSION_ID"] = gAgent.getSessionID();
    substitution["FIRST_LOGIN"] = gAgent.isFirstLogin();

    // work out the current language
    std::string lang = LLUI::getLanguage();
    if (lang == "en-us")
    {
        // *HACK: the correct fix is to change English.lproj/language.txt,
        // but we're late in the release cycle and this is a less risky fix
        lang = "en";
    }
    substitution["LANGUAGE"] = lang;

    // find the region ID
    LLUUID region_id;
    LLViewerRegion *region = gAgent.getRegion();
    if (region)
    {
        region_id = region->getRegionID();
    }
    substitution["REGION_ID"] = region_id;

    // find the parcel local ID
    S32 parcel_id = 0;
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (parcel)
    {
        parcel_id = parcel->getLocalID();
    }
    substitution["PARCEL_ID"] = llformat("%d", parcel_id);

    // find the grid
    std::string current_grid = LLGridManager::getInstance()->getGridId();
    std::transform(current_grid.begin(), current_grid.end(), current_grid.begin(), ::tolower);
    if (current_grid == "damballah")
    {
      // Staging grid has its own naming scheme.
      substitution["GRID"] = "secondlife-staging.com";
    }
    else
    {
        substitution["GRID"] = "secondlife.com";
    }

    // expand all of the substitution strings and escape the url
    std::string expanded_url = url;
    LLStringUtil::format(expanded_url, substitution);

    return LLWeb::escapeURL(expanded_url);
}

//static
bool LLWeb::useExternalBrowser(const std::string &url)
{
#ifdef EXTERNAL_TOS
    return true;
#else
    if (gSavedSettings.getU32("PreferredBrowserBehavior") == BROWSER_EXTERNAL_ONLY)
    {
        return true;
    }
    else if (gSavedSettings.getU32("PreferredBrowserBehavior") == BROWSER_INT_LL_EXT_OTHERS)
    {
        LLUriParser up(url);
        up.normalize();
        up.extractParts();
        std::string uri_string = up.host();

        boost::regex pattern = boost::regex("\\b(lindenlab.com|secondlife.com|secondlife.io)$", boost::regex::perl|boost::regex::icase);
        boost::match_results<std::string::const_iterator> matches;
        return !(ll_regex_search(uri_string, matches, pattern));
    }
    else
    {
        boost::regex pattern = boost::regex("^mailto:", boost::regex::perl | boost::regex::icase);
        boost::match_results<std::string::const_iterator> matches;
        return ll_regex_search(url, matches, pattern);
    }
#endif
}
