/** 
 * @file llfloatermediabrowser.cpp
 * @brief media browser floater - uses embedded media browser control
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

#include "llfloatermediabrowser.h"

#include "llfloaterreg.h"
#include "llparcel.h"
#include "llpluginclassmedia.h"
#include "lluictrlfactory.h"
#include "llmediactrl.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llweb.h"
#include "llui.h"
#include "roles_constants.h"

#include "llurlhistory.h"
#include "llmediactrl.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
#include "llcombobox.h"


// TEMP
#include "llsdutil.h"

LLFloaterMediaBrowser::LLFloaterMediaBrowser(const LLSD& key)
	: LLFloater(key)
{
//	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_media_browser.xml");

}

void LLFloaterMediaBrowser::draw()
{
	childSetEnabled("go", !mAddressCombo->getValue().asString().empty());
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if(parcel)
	{
		childSetVisible("parcel_owner_controls", LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_CHANGE_MEDIA));
		childSetEnabled("assign", !mAddressCombo->getValue().asString().empty());
	}
	bool show_time_controls = false;
	bool media_playing = false;
	if(mBrowser)
	{
		LLPluginClassMedia* media_plugin = mBrowser->getMediaPlugin();
		if(media_plugin)
		{
			show_time_controls = media_plugin->pluginSupportsMediaTime();
			media_playing = media_plugin->getStatus() == LLPluginClassMediaOwner::MEDIA_PLAYING;
		}
	}
	childSetVisible("rewind", show_time_controls);
	childSetVisible("play", show_time_controls && ! media_playing);
	childSetVisible("pause", show_time_controls && media_playing);
	childSetVisible("stop", show_time_controls);
	childSetVisible("seek", show_time_controls);

	childSetEnabled("play", ! media_playing);
	childSetEnabled("stop", media_playing);

	childSetEnabled("back", mBrowser->canNavigateBack());
	childSetEnabled("forward", mBrowser->canNavigateForward());

	LLFloater::draw();
}

BOOL LLFloaterMediaBrowser::postBuild()
{
	mBrowser = getChild<LLMediaCtrl>("browser");
	mBrowser->addObserver(this);

	mAddressCombo = getChild<LLComboBox>("address");
	mAddressCombo->setCommitCallback(onEnterAddress, this);

	childSetAction("back", onClickBack, this);
	childSetAction("forward", onClickForward, this);
	childSetAction("reload", onClickRefresh, this);
	childSetAction("rewind", onClickRewind, this);
	childSetAction("play", onClickPlay, this);
	childSetAction("stop", onClickStop, this);
	childSetAction("pause", onClickPlay, this);
	childSetAction("seek", onClickSeek, this);
	childSetAction("go", onClickGo, this);
	childSetAction("close", onClickClose, this);
	childSetAction("open_browser", onClickOpenWebBrowser, this);
	childSetAction("assign", onClickAssign, this);

	buildURLHistory();
	return TRUE;
}

void LLFloaterMediaBrowser::buildURLHistory()
{
	LLCtrlListInterface* url_list = childGetListInterface("address");
	if (url_list)
	{
		url_list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	}

	// Get all of the entries in the "browser" collection
	LLSD browser_history = LLURLHistory::getURLHistory("browser");

	LLSD::array_iterator iter_history =
		browser_history.beginArray();
	LLSD::array_iterator end_history =
		browser_history.endArray();
	for(; iter_history != end_history; ++iter_history)
	{
		std::string url = (*iter_history).asString();
		if(! url.empty())
			url_list->addSimpleElement(url);
	}

	// initialize URL history in the plugin
	mBrowser->getMediaPlugin()->initializeUrlHistory(browser_history);
}

std::string LLFloaterMediaBrowser::getSupportURL()
{
	return getString("support_page_url");
}
void LLFloaterMediaBrowser::onClose(bool app_quitting)
{
	//setVisible(FALSE);
	destroy();
}

void LLFloaterMediaBrowser::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	if(event == MEDIA_EVENT_LOCATION_CHANGED)
	{
		setCurrentURL(self->getLocation());
	}
	else if(event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{
		// This is the event these flags are sent with.
		childSetEnabled("back", self->getHistoryBackAvailable());
		childSetEnabled("forward", self->getHistoryForwardAvailable());
	}
}
void LLFloaterMediaBrowser::setCurrentURL(const std::string& url)
{
	mCurrentURL = url;

	// redirects will navigate momentarily to about:blank, don't add to history
	if (mCurrentURL != "about:blank")
	{
		mAddressCombo->remove(mCurrentURL);
		mAddressCombo->add(mCurrentURL, ADD_SORTED);
		mAddressCombo->selectByValue(mCurrentURL);

		// Serialize url history
		LLURLHistory::removeURL("browser", mCurrentURL);
		LLURLHistory::addURL("browser", mCurrentURL);
	}
	childSetEnabled("back", mBrowser->canNavigateBack());
	childSetEnabled("forward", mBrowser->canNavigateForward());
	childSetEnabled("reload", TRUE);
}

void LLFloaterMediaBrowser::onOpen(const LLSD& media_url)
{
	LLFloater::onOpen(media_url);
	openMedia(media_url.asString());
}

//static 
void LLFloaterMediaBrowser::onEnterAddress(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;
	self->mBrowser->navigateTo(self->mAddressCombo->getValue().asString());
}

//static 
void LLFloaterMediaBrowser::onClickRefresh(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	self->mAddressCombo->remove(0);
	self->mBrowser->navigateTo(self->mCurrentURL);
}

//static 
void LLFloaterMediaBrowser::onClickForward(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	self->mBrowser->navigateForward();
}

//static 
void LLFloaterMediaBrowser::onClickBack(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	self->mBrowser->navigateBack();
}

//static 
void LLFloaterMediaBrowser::onClickGo(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	self->mBrowser->navigateTo(self->mAddressCombo->getValue().asString());
}

//static 
void LLFloaterMediaBrowser::onClickClose(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	self->closeFloater();
}

//static 
void LLFloaterMediaBrowser::onClickOpenWebBrowser(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	std::string url = self->mCurrentURL.empty() ? 
		self->mBrowser->getHomePageUrl() :
		self->mCurrentURL;
	LLWeb::loadURLExternal(url);
}

void LLFloaterMediaBrowser::onClickAssign(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (!parcel)
	{
		return;
	}
	std::string media_url = self->mAddressCombo->getValue().asString();
	LLStringUtil::trim(media_url);

	if(parcel->getMediaType() != "text/html")
	{
		parcel->setMediaURL(media_url);
		parcel->setMediaCurrentURL(media_url);
		parcel->setMediaType(std::string("text/html"));
		LLViewerParcelMgr::getInstance()->sendParcelPropertiesUpdate( parcel, true );
		LLViewerParcelMedia::sendMediaNavigateMessage(media_url);
		LLViewerParcelMedia::stop();
		// LLViewerParcelMedia::update( parcel );
	}
	LLViewerParcelMedia::sendMediaNavigateMessage(media_url);
}
//static 
void LLFloaterMediaBrowser::onClickRewind(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	if(self->mBrowser->getMediaPlugin())
		self->mBrowser->getMediaPlugin()->start(-2.0f);
}
//static 
void LLFloaterMediaBrowser::onClickPlay(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	LLPluginClassMedia* plugin = self->mBrowser->getMediaPlugin();
	if(plugin)
	{
		if(plugin->getStatus() == LLPluginClassMediaOwner::MEDIA_PLAYING)
		{
			plugin->pause();
		}
		else
		{
			plugin->start();
		}
	}
}
//static 
void LLFloaterMediaBrowser::onClickStop(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	if(self->mBrowser->getMediaPlugin())
		self->mBrowser->getMediaPlugin()->stop();
}
//static 
void LLFloaterMediaBrowser::onClickSeek(void* user_data)
{
	LLFloaterMediaBrowser* self = (LLFloaterMediaBrowser*)user_data;

	if(self->mBrowser->getMediaPlugin())
		self->mBrowser->getMediaPlugin()->start(2.0f);
}
void LLFloaterMediaBrowser::openMedia(const std::string& media_url)
{
	mBrowser->setHomePageUrl(media_url);
	mBrowser->navigateTo(media_url);
	setCurrentURL(media_url);
}
