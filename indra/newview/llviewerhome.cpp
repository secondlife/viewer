/** 
 * @file llviewerhome.cpp
 * @brief Model (non-View) component for the web-based Home side panel
 * @author Martin Reddy
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

