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
#include "llvieweruictrlfactory.h"
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
:	LLFloater("floater_hud"),
	mWebBrowser(0)
{
	// Don't grab the focus as it will impede performing in-world actions
	// while using the HUD
	setAutoFocus(FALSE);
	
	// Opaque background since we never get the focus
	setBackgroundOpaque(TRUE);

	// Create floater from its XML definition
	gUICtrlFactory->buildFloater(this, "floater_hud.xml");
	
	// Position floater based on saved location
	LLRect saved_position_rect = gSavedSettings.getRect("FloaterHUDRect");
	reshape(saved_position_rect.getWidth(), saved_position_rect.getHeight(), FALSE);
	setRect(saved_position_rect);
	
	mWebBrowser = LLViewerUICtrlFactory::getWebBrowserByName(this,  "floater_hud_browser" );
	if (mWebBrowser)
	{
		// Always refresh the browser
		mWebBrowser->setAlwaysRefresh(true);

		// Open links in internal browser
		mWebBrowser->setOpenInExternalBrowser(false);

		LLString language(gSavedSettings.getString("Language"));
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
void LLFloaterHUD::show()
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

void LLFloaterHUD::close()
{
	if (sInstance) sInstance->close();
}

void LLFloaterHUD::onFocusReceived()
{
	// Never get the focus
	setFocus(FALSE);
}
