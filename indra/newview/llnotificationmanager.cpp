/** 
 * @file llnotificationmanager.cpp
 * @brief Class implements a brige between the old and a new notification sistems
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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



#include "llviewerprecompiledheaders.h" // must be first include


#include "llnotificationmanager.h"

#include "llnearbychathandler.h"
#include "llnotifications.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLNotificationManager::LLNotificationManager()
{
	mNotifyHandlers.clear();
	init();
}

//--------------------------------------------------------------------------
LLNotificationManager::~LLNotificationManager()
{
	BOOST_FOREACH(listener_pair_t& pair, mChannelListeners)
	{
		pair.second.disconnect();
	}
}

//--------------------------------------------------------------------------
void LLNotificationManager::init()
{
	LLNotificationChannel::buildChannel("Notifications", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "notify"));
	LLNotificationChannel::buildChannel("NotificationTips", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "notifytip"));
	LLNotificationChannel::buildChannel("Group Notifications", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "groupnotify"));
	LLNotificationChannel::buildChannel("Alerts", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "alert"));
	LLNotificationChannel::buildChannel("AlertModal", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "alertmodal"));
	LLNotificationChannel::buildChannel("IM Notifications", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "notifytoast"));
	LLNotificationChannel::buildChannel("Offer", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "offer"));
	LLNotificationChannel::buildChannel("Hints", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "hint"));
	LLNotificationChannel::buildChannel("Browser", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "browser"));
	LLNotificationChannel::buildChannel("Outbox", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "outbox"));
  
	mChannelListeners["Notifications"] = LLNotifications::instance().getChannel("Notifications")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	mChannelListeners["NotificationTips"] = LLNotifications::instance().getChannel("NotificationTips")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	mChannelListeners["Group Notifications"] = LLNotifications::instance().getChannel("Group Notifications")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	mChannelListeners["Alerts"] = LLNotifications::instance().getChannel("Alerts")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	mChannelListeners["AlertModal"] = LLNotifications::instance().getChannel("AlertModal")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	mChannelListeners["IM Notifications"] = LLNotifications::instance().getChannel("IM Notifications")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	mChannelListeners["Offer"] = LLNotifications::instance().getChannel("Offer")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	mChannelListeners["Hints"] = LLNotifications::instance().getChannel("Hints")->connectChanged(boost::bind(&LLHintHandler::processNotification, LLHintHandler::getInstance(), _1));
	mChannelListeners["Browser"] = LLNotifications::instance().getChannel("Browser")->connectChanged(boost::bind(&LLBrowserNotification::processNotification, LLBrowserNotification::getInstance(), _1));
	mChannelListeners["Outbox"] = LLNotifications::instance().getChannel("Outbox")->connectChanged(boost::bind(&LLOutboxNotification::processNotification, LLOutboxNotification::getInstance(), _1));

	mNotifyHandlers["notify"] = boost::shared_ptr<LLEventHandler>(new LLScriptHandler(NT_NOTIFY, LLSD()));
	mNotifyHandlers["notifytip"] =  boost::shared_ptr<LLEventHandler>(new LLTipHandler(NT_NOTIFY, LLSD()));
	mNotifyHandlers["groupnotify"] = boost::shared_ptr<LLEventHandler>(new LLGroupHandler(NT_GROUPNOTIFY, LLSD()));
	mNotifyHandlers["alert"] = boost::shared_ptr<LLEventHandler>(new LLAlertHandler(NT_ALERT, LLSD()));
	mNotifyHandlers["alertmodal"] = boost::shared_ptr<LLEventHandler>(new LLAlertHandler(NT_ALERT, LLSD()));
	static_cast<LLAlertHandler*>(mNotifyHandlers["alertmodal"].get())->setAlertMode(true);
	mNotifyHandlers["notifytoast"] = boost::shared_ptr<LLEventHandler>(new LLIMHandler(NT_IMCHAT, LLSD()));
	
	mNotifyHandlers["nearbychat"] = boost::shared_ptr<LLEventHandler>(new LLNearbyChatHandler(NT_NEARBYCHAT, LLSD()));
	mNotifyHandlers["offer"] = boost::shared_ptr<LLEventHandler>(new LLOfferHandler(NT_OFFER, LLSD()));
}

//--------------------------------------------------------------------------
bool LLNotificationManager::onNotification(const LLSD& notify)
{
	LLSysHandler* handle = NULL;

	// Don't bother if we're going down.
	// Otherwise we might crash when trying to use handlers that are already dead.
	if( LLApp::isExiting() )
	{
		return false;
	}

	if (LLNotifications::destroyed())
		return false;

	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());
	
	if (!notification) 
		return false;

	std::string notification_type = notification->getType();
	handle = static_cast<LLSysHandler*>(mNotifyHandlers[notification_type].get());

	if(!handle)
		return false;
	
	return handle->processNotification(notify);
}

//--------------------------------------------------------------------------
void LLNotificationManager::onChat(const LLChat& msg, const LLSD &args)
{
	// check ENotificationType argument
	switch(args["type"].asInteger())
	{
	case NT_NEARBYCHAT:
		{
			LLNearbyChatHandler* handle = dynamic_cast<LLNearbyChatHandler*>(mNotifyHandlers["nearbychat"].get());

			if(handle)
				handle->processChat(msg, args);
		}
		break;
	default: 	//no need to handle all enum types
		break;
	}
}

//--------------------------------------------------------------------------
LLEventHandler* LLNotificationManager::getHandlerForNotification(std::string notification_type) 
{ 
	std::map<std::string, boost::shared_ptr<LLEventHandler> >::iterator it = mNotifyHandlers.find(notification_type);

	if(it != mNotifyHandlers.end())
		return (*it).second.get();

	return NULL;
}

//--------------------------------------------------------------------------

