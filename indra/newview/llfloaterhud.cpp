/** 
 * @file llfloaterhud.cpp
 * @brief Implementation of HUD floater
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * Copyright (c) 2008, Linden Research, Inc.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterhud.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llwebbrowserctrl.h"
#include "llalertdialog.h"

// statics 
LLFloaterHUD* LLFloaterHUD::sInstance = 0; 
std::string LLFloaterHUD::sTutorialUrl = "";

///----------------------------------------------------------------------------
/// Class LLFloaterHUD
///----------------------------------------------------------------------------
#define super LLFloater	/* superclass */

// Default constructor
LLFloaterHUD::LLFloaterHUD()
:	LLFloater(std::string("floater_hud")),
	mWebBrowser(0)
{
	// Create floater from its XML definition
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_hud.xml");
	
	// Don't grab the focus as it will impede performing in-world actions
	// while using the HUD
	setIsChrome(TRUE);

	// Chrome doesn't show the window title by default, but here we
	// want to show it.
	setTitleVisible(true);
	
	// Opaque background since we never get the focus
	setBackgroundOpaque(TRUE);

	// Position floater based on saved location
	LLRect saved_position_rect = gSavedSettings.getRect("FloaterHUDRect");
	reshape(saved_position_rect.getWidth(), saved_position_rect.getHeight(), FALSE);
	setRect(saved_position_rect);
	
	mWebBrowser = getChild<LLWebBrowserCtrl>("floater_hud_browser" );
	if (mWebBrowser)
	{
		// Open links in internal browser
		mWebBrowser->setOpenInExternalBrowser(false);

		// This is a "chrome" floater, so we don't want anything to
		// take focus (as the user needs to be able to walk with 
		// arrow keys during tutorial).
		mWebBrowser->setTakeFocusOnClick(false);

		std::string language(gSavedSettings.getString("Language"));
		if(language == "default")
		{
			language = gSavedSettings.getString("SystemLanguage");
		}
	
		std::string url = sTutorialUrl + language + "/";
		mWebBrowser->navigateTo(url);
	}

	// Remember the one instance
	sInstance = this;
}

// Get the instance
LLFloaterHUD* LLFloaterHUD::getInstance()
{
	if (!sInstance)
	{
		new LLFloaterHUD();
	}
	return sInstance;
}

// Destructor
LLFloaterHUD::~LLFloaterHUD()
{
	// Save floater position
	gSavedSettings.setRect("FloaterHUDRect", getRect() );

	// Clear out the one instance if it's ours
	if (sInstance == this)
	{
		sInstance = NULL;
	}
}

// Show the HUD
void LLFloaterHUD::showHUD()
{
	// do not build the floater if there the url is empty
	if (sTutorialUrl == "")
	{
		LLAlertDialog::showXml("TutorialNotFound");
		return;
	}

	// Create the instance if necessary
	LLFloaterHUD* hud = getInstance();
	hud->open();
	hud->setFrontmost(FALSE);
}

void LLFloaterHUD::closeHUD()
{
	if (sInstance) sInstance->close();
}
