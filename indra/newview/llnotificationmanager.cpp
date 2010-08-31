/** 
 * @file llnotificationmanager.cpp
 * @brief Class implements a brige between the old and a new notification sistems
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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



#include "llviewerprecompiledheaders.h" // must be first include


#include "llnotificationmanager.h"

#include "llnearbychathandler.h"
#include "llnotifications.h"

#include <boost/bind.hpp>

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
  
	LLNotifications::instance().getChannel("Notifications")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	LLNotifications::instance().getChannel("NotificationTips")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	LLNotifications::instance().getChannel("Group Notifications")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	LLNotifications::instance().getChannel("Alerts")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	LLNotifications::instance().getChannel("AlertModal")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	LLNotifications::instance().getChannel("IM Notifications")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));
	LLNotifications::instance().getChannel("Offer")->connectChanged(boost::bind(&LLNotificationManager::onNotification, this, _1));

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

