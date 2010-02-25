/** 
 * @file llnotificationsconsole.cpp
 * @brief Debugging console for unified notifications.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
#include "llfloaternotificationsconsole.h"
#include "llnotifications.h"
#include "lluictrlfactory.h"
#include "llbutton.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llpanel.h"
#include "llcombobox.h"

const S32 NOTIFICATION_PANEL_HEADER_HEIGHT = 20;
const S32 HEADER_PADDING = 38;

class LLNotificationChannelPanel : public LLPanel
{
public:
	LLNotificationChannelPanel(const std::string& channel_name);
	BOOL postBuild();

private:
	bool update(const LLSD& payload, bool passed_filter);
	static void toggleClick(void* user_data);
	static void onClickNotification(void* user_data);
	static void onClickNotificationReject(void* user_data);
	LLNotificationChannelPtr mChannelPtr;
	LLNotificationChannelPtr mChannelRejectsPtr;
};

LLNotificationChannelPanel::LLNotificationChannelPanel(const std::string& channel_name) 
	: LLPanel()
{
	mChannelPtr = LLNotifications::instance().getChannel(channel_name);
	mChannelRejectsPtr = LLNotificationChannelPtr(
		LLNotificationChannel::buildChannel(channel_name + "rejects", mChannelPtr->getParentChannelName(),
											!boost::bind(mChannelPtr->getFilter(), _1)));
	LLUICtrlFactory::instance().buildPanel(this, "panel_notifications_channel.xml");
}

BOOL LLNotificationChannelPanel::postBuild()
{
	LLButton* header_button = getChild<LLButton>("header");
	header_button->setLabel(mChannelPtr->getName());
	header_button->setClickedCallback(toggleClick, this);

	mChannelPtr->connectChanged(boost::bind(&LLNotificationChannelPanel::update, this, _1, true));
	mChannelRejectsPtr->connectChanged(boost::bind(&LLNotificationChannelPanel::update, this, _1, false));

	LLScrollListCtrl* scroll = getChild<LLScrollListCtrl>("notifications_list");
	scroll->setDoubleClickCallback(onClickNotification, this);
	scroll->setRect(LLRect( getRect().mLeft, getRect().mTop, getRect().mRight, 0));
	scroll = getChild<LLScrollListCtrl>("notification_rejects_list");
	scroll->setDoubleClickCallback(onClickNotificationReject, this);
	scroll->setRect(LLRect( getRect().mLeft, getRect().mTop, getRect().mRight, 0));
	return TRUE;
}

//static
void LLNotificationChannelPanel::toggleClick(void *user_data)
{
	LLNotificationChannelPanel* self = (LLNotificationChannelPanel*)user_data;
	if (!self) return;
	
	LLButton* header_button = self->getChild<LLButton>("header");
	
	LLLayoutStack* stack = dynamic_cast<LLLayoutStack*>(self->getParent());
	if (stack)
	{
		stack->collapsePanel(self, header_button->getToggleState());
	}

	// turn off tab stop for collapsed panel
	self->getChild<LLScrollListCtrl>("notifications_list")->setTabStop(!header_button->getToggleState());
	self->getChild<LLScrollListCtrl>("notifications_list")->setVisible(!header_button->getToggleState());
	self->getChild<LLScrollListCtrl>("notification_rejects_list")->setTabStop(!header_button->getToggleState());
	self->getChild<LLScrollListCtrl>("notification_rejects_list")->setVisible(!header_button->getToggleState());
}

/*static*/
void LLNotificationChannelPanel::onClickNotification(void* user_data)
{
	LLNotificationChannelPanel* self = (LLNotificationChannelPanel*)user_data;
	if (!self) return;
	LLScrollListItem* firstselected = self->getChild<LLScrollListCtrl>("notifications_list")->getFirstSelected();
	llassert(firstselected);
	if (firstselected)
	{
		void* data = firstselected->getUserdata();
		if (data)
		{
			gFloaterView->getParentFloater(self)->addDependentFloater(new LLFloaterNotification((LLNotification*)data), TRUE);
		}
	}
}

/*static*/
void LLNotificationChannelPanel::onClickNotificationReject(void* user_data)
{
	LLNotificationChannelPanel* self = (LLNotificationChannelPanel*)user_data;
	if (!self) return;
	LLScrollListItem* firstselected = self->getChild<LLScrollListCtrl>("notification_rejects_list")->getFirstSelected();
	llassert(firstselected);
	if (firstselected)
	{
		void* data = firstselected->getUserdata();
		if (data)
		{
			gFloaterView->getParentFloater(self)->addDependentFloater(new LLFloaterNotification((LLNotification*)data), TRUE);
		}
	}
}

bool LLNotificationChannelPanel::update(const LLSD& payload, bool passed_filter)
{
	LLNotificationPtr notification = LLNotifications::instance().find(payload["id"].asUUID());
	if (notification)
	{
		LLSD row;
		row["columns"][0]["value"] = notification->getName();
		row["columns"][0]["column"] = "name";

		row["columns"][1]["value"] = notification->getMessage();
		row["columns"][1]["column"] = "content";

		row["columns"][2]["value"] = notification->getDate();
		row["columns"][2]["column"] = "date";
		row["columns"][2]["type"] = "date";

		LLScrollListItem* sli = passed_filter ? 
			getChild<LLScrollListCtrl>("notifications_list")->addElement(row) :
			getChild<LLScrollListCtrl>("notification_rejects_list")->addElement(row);
		sli->setUserdata(&(*notification));
	}

	return false;
}

//
// LLFloaterNotificationConsole
//
LLFloaterNotificationConsole::LLFloaterNotificationConsole(const LLSD& key)
: LLFloater(key)
{
	mCommitCallbackRegistrar.add("ClickAdd",     boost::bind(&LLFloaterNotificationConsole::onClickAdd, this));	

	//LLUICtrlFactory::instance().buildFloater(this, "floater_notifications_console.xml");
}

BOOL LLFloaterNotificationConsole::postBuild()
{
	// these are in the order of processing
	addChannel("Unexpired");
	addChannel("Ignore");
	addChannel("Visible", true);
	// all the ones below attach to the Visible channel
	addChannel("History");
	addChannel("Alerts");
	addChannel("AlertModal");
	addChannel("Group Notifications");
	addChannel("Notifications");
	addChannel("NotificationTips");

//	getChild<LLButton>("add_notification")->setClickedCallback(onClickAdd, this);

	LLComboBox* notifications = getChild<LLComboBox>("notification_types");
	LLNotifications::TemplateNames names = LLNotifications::instance().getTemplateNames();
	for (LLNotifications::TemplateNames::iterator template_it = names.begin();
		template_it != names.end();
		++template_it)
	{
		notifications->add(*template_it);
	}
	notifications->sortByName();

	return TRUE;
}

void LLFloaterNotificationConsole::addChannel(const std::string& name, bool open)
{
	LLLayoutStack& stack = getChildRef<LLLayoutStack>("notification_channels");
	LLNotificationChannelPanel* panelp = new LLNotificationChannelPanel(name);
	stack.addPanel(panelp, 0, NOTIFICATION_PANEL_HEADER_HEIGHT, S32_MAX, S32_MAX, TRUE, TRUE, LLLayoutStack::ANIMATE);

	LLButton& header_button = panelp->getChildRef<LLButton>("header");
	header_button.setToggleState(!open);
	stack.collapsePanel(panelp, !open);

	updateResizeLimits();
}

void LLFloaterNotificationConsole::removeChannel(const std::string& name)
{
	LLPanel* panelp = getChild<LLPanel>(name);
	getChildRef<LLLayoutStack>("notification_channels").removePanel(panelp);
	delete panelp;

	updateResizeLimits();
}

//static 
void LLFloaterNotificationConsole::updateResizeLimits()
{
	const LLFloater::Params& floater_params = LLFloater::getDefaultParams();
	S32 floater_header_size = floater_params.header_height;

	LLLayoutStack& stack = getChildRef<LLLayoutStack>("notification_channels");
	setResizeLimits(getMinWidth(), floater_header_size + HEADER_PADDING + ((NOTIFICATION_PANEL_HEADER_HEIGHT + 3) * stack.getNumPanels()));
}

void LLFloaterNotificationConsole::onClickAdd()
{
	std::string message_name = getChild<LLComboBox>("notification_types")->getValue().asString();
	if (!message_name.empty())
	{
		LLNotifications::instance().add(message_name, LLSD(), LLSD());
	}
}


//=============== LLFloaterNotification ================

LLFloaterNotification::LLFloaterNotification(LLNotification* note) 
:	LLFloater(LLSD()),
	mNote(note)
{
	LLUICtrlFactory::instance().buildFloater(this, "floater_notification.xml", NULL);
}

BOOL LLFloaterNotification::postBuild()
{
	setTitle(mNote->getName());
	getChild<LLUICtrl>("payload")->setValue(mNote->getMessage());

	LLComboBox* responses_combo = getChild<LLComboBox>("response");
	LLCtrlListInterface* response_list = responses_combo->getListInterface();
	LLNotificationFormPtr form(mNote->getForm());
	if(!form)
	{
		return TRUE;
	}

	responses_combo->setCommitCallback(onCommitResponse, this);

	LLSD form_sd = form->asLLSD();

	for (LLSD::array_const_iterator form_item = form_sd.beginArray(); form_item != form_sd.endArray(); ++form_item)
	{
		if ( (*form_item)["type"].asString() != "button") continue;
		std::string text = (*form_item)["text"].asString();
		response_list->addSimpleElement(text);
	}

	return TRUE;
}

void LLFloaterNotification::respond()
{
	LLComboBox* responses_combo = getChild<LLComboBox>("response");
	LLCtrlListInterface* response_list = responses_combo->getListInterface();
	const std::string& trigger = response_list->getSelectedValue().asString();
	//llinfos << trigger << llendl;

	LLSD response = mNote->getResponseTemplate();
	response[trigger] = true;
	mNote->respond(response);
}
