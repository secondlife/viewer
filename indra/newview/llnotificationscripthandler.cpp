/** 
 * @file llnotificationscripthandler.cpp
 * @brief Notification Handler Class for Simple Notifications and Notification Tips
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

#include "llnotificationhandler.h"
#include "lltoastnotifypanel.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llnotificationmanager.h"
#include "llnotifications.h"
#include "llscriptfloater.h"

using namespace LLNotificationsUI;

static const std::string SCRIPT_DIALOG				("ScriptDialog");
static const std::string SCRIPT_DIALOG_GROUP		("ScriptDialogGroup");
static const std::string SCRIPT_LOAD_URL			("LoadWebPage");

//--------------------------------------------------------------------------
LLScriptHandler::LLScriptHandler(e_notification_type type, const LLSD& id)
{
	mType = type;

	// Getting a Channel for our notifications
	LLScreenChannel* channel = LLChannelManager::getInstance()->createNotificationChannel();
	if(channel)
	{
		channel->setControlHovering(true);
		channel->setOnRejectToastCallback(boost::bind(&LLScriptHandler::onRejectToast, this, _1));
		mChannel = channel->getHandle();
	}
}

//--------------------------------------------------------------------------
LLScriptHandler::~LLScriptHandler()
{
}

//--------------------------------------------------------------------------
void LLScriptHandler::initChannel()
{
	S32 channel_right_bound = gViewerWindow->getWorldViewRectScaled().mRight - gSavedSettings.getS32("NotificationChannelRightMargin"); 
	S32 channel_width = gSavedSettings.getS32("NotifyBoxWidth");
	mChannel.get()->init(channel_right_bound - channel_width, channel_right_bound);
}

//--------------------------------------------------------------------------
bool LLScriptHandler::processNotification(const LLSD& notify)
{
	if(mChannel.isDead())
	{
		return false;
	}

	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());

	if(!notification)
		return false;

	// arrange a channel on a screen
	if(!mChannel.get()->getVisible())
	{
		initChannel();
	}
	
	if(notify["sigtype"].asString() == "add")
	{
		if (LLHandlerUtil::canLogToIM(notification))
		{
			LLHandlerUtil::logToIMP2P(notification);
		}

		if(SCRIPT_DIALOG == notification->getName() || SCRIPT_DIALOG_GROUP == notification->getName() || SCRIPT_LOAD_URL == notification->getName())
		{
			LLScriptFloaterManager::getInstance()->onAddNotification(notification->getID());
		}
		else
		{
			LLToastPanel* notify_box = LLToastPanel::buidPanelFromNotification(notification);

			LLToast::Params p;
			p.notif_id = notification->getID();
			p.notification = notification;
			p.panel = notify_box;	
			p.on_delete_toast = boost::bind(&LLScriptHandler::onDeleteToast, this, _1);

			LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel.get());
			if(channel)
			{
				channel->addToast(p);
			}

			// send a signal to the counter manager
			mNewNotificationSignal();
		}
	}
	else if (notify["sigtype"].asString() == "delete")
	{
		if(SCRIPT_DIALOG == notification->getName() || SCRIPT_DIALOG_GROUP == notification->getName() || SCRIPT_LOAD_URL == notification->getName())
		{
			LLScriptFloaterManager::getInstance()->onRemoveNotification(notification->getID());
		}
		else
		{
			mChannel.get()->killToastByNotificationID(notification->getID());
		}
	}
	return false;
}

//--------------------------------------------------------------------------

void LLScriptHandler::onDeleteToast(LLToast* toast)
{
	// send a signal to the counter manager
	mDelNotificationSignal();

	// send a signal to a listener to let him perform some action
	// in this case listener is a SysWellWindow and it will remove a corresponding item from its list
	mNotificationIDSignal(toast->getNotificationID());

	LLNotificationPtr notification = LLNotifications::getInstance()->find(toast->getNotificationID());
	
	if( notification && 
		(SCRIPT_DIALOG == notification->getName() || SCRIPT_DIALOG_GROUP == notification->getName()) )
	{
		LLScriptFloaterManager::getInstance()->onRemoveNotification(notification->getID());
	}
}

//--------------------------------------------------------------------------
void LLScriptHandler::onRejectToast(LLUUID& id)
{
	LLNotificationPtr notification = LLNotifications::instance().find(id);

	if (notification
			&& LLNotificationManager::getInstance()->getHandlerForNotification(
					notification->getType()) == this)
	{
		LLNotifications::instance().cancel(notification);
	}
}

//--------------------------------------------------------------------------




