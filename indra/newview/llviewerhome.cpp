/** 
 * @file llviewerhome.cpp
 * @brief Model (non-View) component for the web-based Home side panel
 * @author Martin Reddy
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
#include "llviewerhome.h"

#include "lllogininstance.h"
#include "llsd.h"
#include "llui.h"
#include "lluri.h"
#include "llviewercontrol.h"
#include "llweb.h"

//static
std::string LLViewerHome::getHomeURL()
{   
    // Return the URL to display in the Home side tray. We read
    // this value from settings.xml and support various substitutions

    LLSD substitution;
    substitution["AUTH_TOKEN"] = LLURI::escape(getAuthKey());

    // get the home URL from the settings.xml file
    std::string homeURL = gSavedSettings.getString("HomeSidePanelURL");

    // support a grid-level override of the URL from login.cgi
    LLSD grid_url = LLLoginInstance::getInstance()->getResponse("home_sidetray_url");
    if (! grid_url.asString().empty())
    {
        homeURL = grid_url.asString();
    }   

    // expand all substitution strings in the URL and return it
    // (also adds things like [LANGUAGE], [VERSION], [OS], etc.)
    return LLWeb::expandURLSubstitutions(homeURL, substitution);
}

//static
std::string LLViewerHome::getAuthKey()
{
    // return the value of the (optional) auth token returned by login.cgi
    // this lets the server provide an authentication token that we can
    // blindly pass to the Home web page for it to perform authentication.
    // We use "home_sidetray_token", and fallback to "auth_token" if not
    // present.
    LLSD auth_token = LLLoginInstance::getInstance()->getResponse("home_sidetray_token");
    if (auth_token.asString().empty())
    {
        auth_token = LLLoginInstance::getInstance()->getResponse("auth_token");
    }
    return auth_token.asString();
}

