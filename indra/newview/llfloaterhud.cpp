/** 
 * @file llfloaterhud.cpp
 * @brief Implementation of HUD floater
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
	
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_hud.xml");
	
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
		// Open links in internal browser
		mWebBrowser->setOpenInExternalBrowser(false);
		
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
