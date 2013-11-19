/** 
 * @file llfloaterhelpbrowser.cpp
 * @brief HTML Help floater - uses embedded web browser control
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
#include "llviewerhelp.h"


LLFloaterHelpBrowser::LLFloaterHelpBrowser(const LLSD& key)
	: LLFloater(key)
{
}

BOOL LLFloaterHelpBrowser::postBuild()
{
	mBrowser = getChild<LLMediaCtrl>("browser");
	mBrowser->addObserver(this);
	mBrowser->setErrorPageURL(gSavedSettings.getString("GenericErrorPageURL"));

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

void LLFloaterHelpBrowser::onOpen(const LLSD& key)
{
	gSavedSettings.setBOOL("HelpFloaterOpen", TRUE);

	std::string topic = key.asString();
	mBrowser->navigateTo(LLViewerHelp::instance().getURL(topic));
}

//virtual
void LLFloaterHelpBrowser::onClose(bool app_quitting)
{
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("HelpFloaterOpen", FALSE);
	}
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
		getChild<LLUICtrl>("status_text")->setValue(getString("loading_text"));
		break;
		
	case MEDIA_EVENT_NAVIGATE_COMPLETE:
		getChild<LLUICtrl>("status_text")->setValue(getString("done_text"));
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
	// explicitly make the media mime type for this floater since it will
	// only ever display one type of content (Web).
	mBrowser->setHomePageUrl(media_url, "text/html");
	mBrowser->navigateTo(media_url, "text/html");
	setCurrentURL(media_url);
}
