/** 
 * @file llfloaterhud.cpp
 * @brief Implementation of HUD floater
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llfloaterhud.h"

// Viewer libs
#include "llviewercontrol.h"
#include "llmediactrl.h"

// Linden libs
#include "llnotificationsutil.h"
#include "lluictrlfactory.h"


///----------------------------------------------------------------------------
/// Class LLFloaterHUD
///----------------------------------------------------------------------------
#define super LLFloater	/* superclass */

// Default constructor
LLFloaterHUD::LLFloaterHUD(const LLSD& key)
:	LLFloater(key),
	mWebBrowser(0)
{
	// do not build the floater if there the url is empty
	if (gSavedSettings.getString("TutorialURL") == "")
	{
		LLNotificationsUtil::add("TutorialNotFound");
		return;
	}
	
	// Don't grab the focus as it will impede performing in-world actions
	// while using the HUD
	setIsChrome(TRUE);

	// Chrome doesn't show the window title by default, but here we
	// want to show it.
	setTitleVisible(true);
	
	// Opaque background since we never get the focus
	setBackgroundOpaque(TRUE);
}

BOOL LLFloaterHUD::postBuild()
{
	mWebBrowser = getChild<LLMediaCtrl>("floater_hud_browser" );
	if (mWebBrowser)
	{
		// This is a "chrome" floater, so we don't want anything to
		// take focus (as the user needs to be able to walk with 
		// arrow keys during tutorial).
		mWebBrowser->setTakeFocusOnClick(false);
		
		std::string language = LLUI::getLanguage();
		std::string base_url = gSavedSettings.getString("TutorialURL");
		
		std::string url = base_url + language + "/";
		mWebBrowser->navigateTo(url);
	}
	
	return TRUE;
}

// Destructor
LLFloaterHUD::~LLFloaterHUD()
{
}
