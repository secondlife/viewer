/** 
 * @file llnotificationsconsole.cpp
 * @brief Debugging console for unified notifications.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

class LLNotificationChannelPanel : public LLLayoutPanel
{
public:
	LLNotificationChannelPanel(const Params& p);
	~LLNotificationChannelPanel();
	bool postBuild();

private:
	bool update(const LLSD& payload);
	static void toggleClick(void* user_data);
	static void onClickNotification(void* user_data);
	LLNotificationChannelPtr mChannelPtr;
};

LLNotificationChannelPanel::LLNotificationChannelPanel(const LLNotificationChannelPanel::Params& p) 
:	LLLayoutPanel(p)
{
	mChannelPtr = LLNotifications::instance().getChannel(p.name);
	buildFromFile( "panel_notifications_channel.xml");
}

LLNotificationChannelPanel::~LLNotificationChannelPanel()
{
	// Userdata for all records is a LLNotification* we need to clean up
	std::vector<LLScrollListItem*> data_list = getChild<LLScrollListCtrl>("notifications_list")->getAllData();
	std::vector<LLScrollListItem*>::iterator data_itor;
	for (data_itor = data_list.begin(); data_itor != data_list.end(); ++data_itor)
	{
		LLScrollListItem* item = *data_itor;
		LLNotification* notification = (LLNotification*)item->getUserdata();
		delete notification;
		notification = NULL;
	}
}

bool LLNotificationChannelPanel::postBuild()
{
	LLButton* header_button = getChild<LLButton>("header");
	header_button->setLabel(mChannelPtr->getName());
	header_button->setClickedCallback(toggleClick, this);

	mChannelPtr->connectChanged(boost::bind(&LLNotificationChannelPanel::update, this, _1));

	LLScrollListCtrl* scroll = getChild<LLScrollListCtrl>("notifications_list");
	scroll->setDoubleClickCallback(onClickNotification, this);
	scroll->setRect(LLRect( getRect().mLeft, getRect().mTop, getRect().mRight, 0));
	return true;
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
			gFloaterView->getParentFloater(self)->addDependentFloater(new LLFloaterNotification((LLNotification*)data), true);
		}
	}
}

bool LLNotificationChannelPanel::update(const LLSD& payload)
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

		LLScrollListItem* sli = getChild<LLScrollListCtrl>("notifications_list")->addElement(row);
		sli->setUserdata(new LLNotification(notification->asLLSD()));
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
}

bool LLFloaterNotificationConsole::postBuild()
{
	// these are in the order of processing
	addChannel("Unexpired");
	addChannel("Ignore");
	addChannel("VisibilityRules");
	addChannel("Visible", true);
	// all the ones below attach to the Visible channel
	addChannel("Persistent");
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

	return true;
}

void LLFloaterNotificationConsole::addChannel(const std::string& name, bool open)
{
	LLLayoutStack& stack = getChildRef<LLLayoutStack>("notification_channels");
	LLNotificationChannelPanel::Params p;
	p.min_dim = NOTIFICATION_PANEL_HEADER_HEIGHT;
	p.auto_resize = true;
	p.user_resize = true;
	p.name = name;
	LLNotificationChannelPanel* panelp = new LLNotificationChannelPanel(p);
	stack.addPanel(panelp, LLLayoutStack::ANIMATE);

	LLButton& header_button = panelp->getChildRef<LLButton>("header");
	header_button.setToggleState(!open);
	stack.collapsePanel(panelp, !open);

	updateResizeLimits();
}

void LLFloaterNotificationConsole::removeChannel(const std::string& name)
{
	LLPanel* panelp = getChild<LLPanel>(name);
	getChildRef<LLView>("notification_channels").removeChild(panelp);
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
	buildFromFile("floater_notification.xml");
}

bool LLFloaterNotification::postBuild()
{
	setTitle(mNote->getName());
	getChild<LLUICtrl>("payload")->setValue(mNote->getMessage());

	LLComboBox* responses_combo = getChild<LLComboBox>("response");
	LLCtrlListInterface* response_list = responses_combo->getListInterface();
	LLNotificationFormPtr form(mNote->getForm());
	if(!form)
	{
		return true;
	}

	responses_combo->setCommitCallback(onCommitResponse, this);

	LLSD form_sd = form->asLLSD();

	for (LLSD::array_const_iterator form_item = form_sd.beginArray(); form_item != form_sd.endArray(); ++form_item)
	{
		if ( (*form_item)["type"].asString() != "button") continue;
		std::string text = (*form_item)["text"].asString();
		response_list->addSimpleElement(text);
	}

	return true;
}

void LLFloaterNotification::respond()
{
	LLComboBox* responses_combo = getChild<LLComboBox>("response");
	LLCtrlListInterface* response_list = responses_combo->getListInterface();
	const std::string& trigger = response_list->getSelectedValue().asString();
	//LL_INFOS() << trigger << LL_ENDL;

	LLSD response = mNote->getResponseTemplate();
	response[trigger] = true;
	mNote->respond(response);
}
