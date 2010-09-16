/** 
 * @file llfloatermediabrowser.cpp
 * @brief media browser floater - uses embedded media browser control
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
#include "lllayoutstack.h"
#include "llcheckboxctrl.h"

#include "llnotifications.h"

// TEMP
#include "llsdutil.h"

LLFloaterMediaBrowser::LLFloaterMediaBrowser(const LLSD& key)
	: LLFloater(key)
{
}

//static 
void LLFloaterMediaBrowser::create(const std::string &url, const std::string& target)
{
	std::string tag = target;
	
	if(target.empty() || target == "_blank")
	{
		// create a unique tag for this instance
		LLUUID id;
		id.generate();
		tag = id.asString();
	}
	
	S32 browser_window_limit = gSavedSettings.getS32("MediaBrowserWindowLimit");
	
	if(LLFloaterReg::findInstance("media_browser", tag) != NULL)
	{
		// There's already a media browser for this tag, so we won't be opening a new window.
	}
	else if(browser_window_limit != 0)
	{
		// showInstance will open a new window.  Figure out how many media browsers are already open, 
		// and close the least recently opened one if this will put us over the limit.
		
		LLFloaterReg::const_instance_list_t &instances = LLFloaterReg::getFloaterList("media_browser");
		lldebugs << "total instance count is " << instances.size() << llendl;
		
		for(LLFloaterReg::const_instance_list_t::const_iterator iter = instances.begin(); iter != instances.end(); iter++)
		{
			lldebugs << "    " << (*iter)->getKey() << llendl;
		}
		
		if(instances.size() >= (size_t)browser_window_limit)
		{
			// Destroy the least recently opened instance
			(*instances.begin())->closeFloater();
		}
	}

	LLFloaterMediaBrowser *browser = dynamic_cast<LLFloaterMediaBrowser*> (LLFloaterReg::showInstance("media_browser", tag));
	llassert(browser);
	if(browser)
	{
		// tell the browser instance to load the specified URL
		browser->openMedia(url);
	}
}

void LLFloaterMediaBrowser::draw()
{
	getChildView("go")->setEnabled(!mAddressCombo->getValue().asString().empty());
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if(parcel)
	{
		getChildView("parcel_owner_controls")->setVisible( LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_CHANGE_MEDIA));
		getChildView("assign")->setEnabled(!mAddressCombo->getValue().asString().empty());
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
	getChildView("rewind")->setVisible( show_time_controls);
	getChildView("play")->setVisible( show_time_controls && ! media_playing);
	getChildView("pause")->setVisible( show_time_controls && media_playing);
	getChildView("stop")->setVisible( show_time_controls);
	getChildView("seek")->setVisible( show_time_controls);

	getChildView("play")->setEnabled(! media_playing);
	getChildView("stop")->setEnabled(media_playing);

	getChildView("back")->setEnabled(mBrowser->canNavigateBack());
	getChildView("forward")->setEnabled(mBrowser->canNavigateForward());

	LLFloater::draw();
}

BOOL LLFloaterMediaBrowser::postBuild()
{
	mBrowser = getChild<LLMediaCtrl>("browser");
	mBrowser->setMediaID(mKey);
	mBrowser->addObserver(this);

	mAddressCombo = getChild<LLComboBox>("address");
	mAddressCombo->setCommitCallback(onEnterAddress, this);
	mAddressCombo->sortByName();

	LLButton& notification_close = getChildRef<LLButton>("close_notification");
	notification_close.setClickedCallback(boost::bind(&LLFloaterMediaBrowser::onCloseNotification, this), NULL);

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
	if(mBrowser && mBrowser->getMediaPlugin())
	{
		mBrowser->getMediaPlugin()->initializeUrlHistory(browser_history);
	}
}

std::string LLFloaterMediaBrowser::getSupportURL()
{
	return getString("support_page_url");
}

//virtual
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
		getChildView("back")->setEnabled(self->getHistoryBackAvailable());
		getChildView("forward")->setEnabled(self->getHistoryForwardAvailable());
	}
	else if(event == MEDIA_EVENT_CLOSE_REQUEST)
	{
		// The browser instance wants its window closed.
		closeFloater();
	}
}
void LLFloaterMediaBrowser::setCurrentURL(const std::string& url)
{
	mCurrentURL = url;

	// redirects will navigate momentarily to about:blank, don't add to history
	if (mCurrentURL != "about:blank")
	{
		mAddressCombo->remove(mCurrentURL);
		mAddressCombo->add(mCurrentURL);
		mAddressCombo->selectByValue(mCurrentURL);

		// Serialize url history
		LLURLHistory::removeURL("browser", mCurrentURL);
		LLURLHistory::addURL("browser", mCurrentURL);
	}
	getChildView("back")->setEnabled(mBrowser->canNavigateBack());
	getChildView("forward")->setEnabled(mBrowser->canNavigateForward());
	getChildView("reload")->setEnabled(TRUE);
}

void LLFloaterMediaBrowser::showNotification(LLNotificationPtr notify)
{
	mCurNotification = notify;

	// add popup here
	LLSD payload = notify->getPayload();

	LLNotificationFormPtr formp = notify->getForm();
	LLLayoutPanel& panel = getChildRef<LLLayoutPanel>("notification_area");
	panel.setVisible(true);
	panel.getChild<LLUICtrl>("notification_icon")->setValue(notify->getIcon());
	panel.getChild<LLUICtrl>("notification_text")->setValue(notify->getMessage());
	panel.getChild<LLUICtrl>("notification_text")->setToolTip(notify->getMessage());
	LLNotificationForm::EIgnoreType ignore_type = formp->getIgnoreType(); 
	LLLayoutPanel& form_elements = panel.getChildRef<LLLayoutPanel>("form_elements");

	const S32 FORM_PADDING_HORIZONTAL = 10;
	const S32 FORM_PADDING_VERTICAL = 5;
	S32 cur_x = FORM_PADDING_HORIZONTAL;

	if (ignore_type != LLNotificationForm::IGNORE_NO)
	{
		LLCheckBoxCtrl::Params checkbox_p;
		checkbox_p.name = "ignore_check";
		checkbox_p.rect = LLRect(cur_x, form_elements.getRect().getHeight() - FORM_PADDING_VERTICAL, cur_x, FORM_PADDING_VERTICAL);
		checkbox_p.label = formp->getIgnoreMessage();
		checkbox_p.label_text.text_color = LLColor4::black;
		checkbox_p.commit_callback.function = boost::bind(&LLFloaterMediaBrowser::onClickIgnore, this, _1);

		LLCheckBoxCtrl* check = LLUICtrlFactory::create<LLCheckBoxCtrl>(checkbox_p);
		check->setRect(check->getBoundingRect());
		form_elements.addChild(check);
		cur_x = check->getRect().mRight + FORM_PADDING_HORIZONTAL;
	}

	for (S32 i = 0; i < formp->getNumElements(); i++)
	{
		LLSD form_element = formp->getElement(i);
		if (form_element["type"].asString() == "button")
		{
			LLButton::Params button_p;
			button_p.name = form_element["name"];
			button_p.label = form_element["text"];
			button_p.rect = LLRect(cur_x, form_elements.getRect().getHeight() - FORM_PADDING_VERTICAL, cur_x, FORM_PADDING_VERTICAL);
			button_p.commit_callback.function = boost::bind(&LLFloaterMediaBrowser::onClickNotificationButton, this, form_element["name"].asString());
			button_p.auto_resize = true;

			LLButton* button = LLUICtrlFactory::create<LLButton>(button_p);
			button->autoResize();
			form_elements.addChild(button);

			cur_x = button->getRect().mRight + FORM_PADDING_HORIZONTAL;
		}
	}


	form_elements.reshape(cur_x, form_elements.getRect().getHeight());

	//LLWeb::loadURL(payload["url"], payload["target"]);
}

void LLFloaterMediaBrowser::hideNotification()
{
	LLLayoutPanel& panel = getChildRef<LLLayoutPanel>("notification_area");
	panel.setVisible(FALSE);
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

void LLFloaterMediaBrowser::onCloseNotification()
{
	LLNotifications::instance().cancel(mCurNotification);
}

void LLFloaterMediaBrowser::onClickIgnore(LLUICtrl* ctrl)
{
	bool check = ctrl->getValue().asBoolean();
	if (mCurNotification && mCurNotification->getForm()->getIgnoreType() == LLNotificationForm::IGNORE_SHOW_AGAIN)
	{
		// question was "show again" so invert value to get "ignore"
		check = !check;
	}
	mCurNotification->setIgnored(check);
}

void LLFloaterMediaBrowser::onClickNotificationButton(const std::string& name)
{
	if (!mCurNotification) return;

	LLSD response = mCurNotification->getResponseTemplate();
	response[name] = true;

	mCurNotification->respond(response); 
}
