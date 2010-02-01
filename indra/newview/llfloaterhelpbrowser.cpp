/** 
 * @file llfloaterhelpbrowser.cpp
 * @brief HTML Help floater - uses embedded web browser control
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llfloaterhelpbrowser.h"

#include "llfloaterreg.h"
#include "llpluginclassmedia.h"
#include "llmediactrl.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llweb.h"
#include "llui.h"

#include "llurlhistory.h"
#include "llmediactrl.h"
#include "llviewermedia.h"


LLFloaterHelpBrowser::LLFloaterHelpBrowser(const LLSD& key)
	: LLFloater(key)
{
}

BOOL LLFloaterHelpBrowser::postBuild()
{
	mBrowser = getChild<LLMediaCtrl>("browser");
	mBrowser->addObserver(this);

	childSetAction("open_browser", onClickOpenWebBrowser, this);

	buildURLHistory();
	return TRUE;
}

void LLFloaterHelpBrowser::buildURLHistory()
{
	// Get all of the entries in the "browser" collection
	LLSD browser_history = LLURLHistory::getURLHistory("browser");

	// initialize URL history in the plugin
	LLPluginClassMedia *plugin = mBrowser->getMediaPlugin();
	if (plugin)
	{
		plugin->initializeUrlHistory(browser_history);
	}
}

//virtual
void LLFloaterHelpBrowser::onClose(bool app_quitting)
{
	// really really destroy the help browser when it's closed, it'll be recreated.
	destroy(); // really destroy this dialog on closure, it's relatively heavyweight.
}

void LLFloaterHelpBrowser::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	switch (event) 
	{
	case MEDIA_EVENT_LOCATION_CHANGED:
		setCurrentURL(self->getLocation());
		break;

	case MEDIA_EVENT_NAVIGATE_BEGIN:
		childSetText("status_text", getString("loading_text"));
		break;
		
	case MEDIA_EVENT_NAVIGATE_COMPLETE:
		childSetText("status_text", getString("done_text"));
		break;

	default:
		break;
	}
}

void LLFloaterHelpBrowser::setCurrentURL(const std::string& url)
{
	mCurrentURL = url;

	// redirects will navigate momentarily to about:blank, don't add to history
	if (mCurrentURL != "about:blank")
	{
		// Serialize url history
		LLURLHistory::removeURL("browser", mCurrentURL);
		LLURLHistory::addURL("browser", mCurrentURL);
	}
}

//static 
void LLFloaterHelpBrowser::onClickClose(void* user_data)
{
	LLFloaterHelpBrowser* self = (LLFloaterHelpBrowser*)user_data;

	self->closeFloater();
}

//static 
void LLFloaterHelpBrowser::onClickOpenWebBrowser(void* user_data)
{
	LLFloaterHelpBrowser* self = (LLFloaterHelpBrowser*)user_data;

	std::string url = self->mCurrentURL.empty() ? 
		self->mBrowser->getHomePageUrl() :
		self->mCurrentURL;
	LLWeb::loadURLExternal(url);
}

void LLFloaterHelpBrowser::openMedia(const std::string& media_url)
{
	mBrowser->setHomePageUrl(media_url);
	//mBrowser->navigateTo("data:text/html;charset=utf-8,I'd really love to be going to:<br><b>" + media_url + "</b>"); // tofu HACK for debugging =:)
	mBrowser->navigateTo(media_url);
	setCurrentURL(media_url);
}

void LLFloaterHelpBrowser::navigateToLocalPage( const std::string& subdir, const std::string& filename_in )
{
	mBrowser->navigateToLocalPage(subdir, filename_in);
}
