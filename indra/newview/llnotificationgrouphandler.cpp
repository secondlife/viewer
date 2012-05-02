/** 
 * @file llnotificationgrouphandler.cpp
 * @brief Notification Handler Class for Group Notifications
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
#include "lltoastgroupnotifypanel.h"
#include "llgroupactions.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llnotificationmanager.h"
#include "llnotifications.h"

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLGroupHandler::LLGroupHandler(e_notification_type type, const LLSD& id)
{
	mType = type;

	// Getting a Channel for our notifications
	LLScreenChannel* channel = LLChannelManager::getInstance()->createNotificationChannel();
	if(channel)
	{
		channel->setOnRejectToastCallback(boost::bind(&LLGroupHandler::onRejectToast, this, _1));
		mChannel = channel->getHandle();
	}
}

//--------------------------------------------------------------------------
LLGroupHandler::~LLGroupHandler()
{
}

//--------------------------------------------------------------------------
void LLGroupHandler::initChannel()
{
	S32 channel_right_bound = gViewerWindow->getWorldViewRectScaled().mRight - gSavedSettings.getS32("NotificationChannelRightMargin"); 
	S32 channel_width = gSavedSettings.getS32("NotifyBoxWidth");
	mChannel.get()->init(channel_right_bound - channel_width, channel_right_bound);
}

//--------------------------------------------------------------------------
bool LLGroupHandler::processNotification(const LLSD& notify)
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
	
	if(notify["sigtype"].asString() == "add" || notify["sigtype"].asString() == "change")
	{
		LLHandlerUtil::logGroupNoticeToIMGroup(notification);

		LLPanel* notify_box = new LLToastGroupNotifyPanel(notification);
		LLToast::Params p;
		p.notif_id = notification->getID();
		p.notification = notification;
		p.panel = notify_box;
		p.on_delete_toast = boost::bind(&LLGroupHandler::onDeleteToast, this, _1);

		LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel.get());
		if(channel)
			channel->addToast(p);

		// send a signal to the counter manager
		mNewNotificationSignal();

		LLGroupActions::refresh_notices();
	}
	else if (notify["sigtype"].asString() == "delete")
	{
		mChannel.get()->killToastByNotificationID(notification->getID());
	}
	return false;
}

//--------------------------------------------------------------------------
void LLGroupHandler::onDeleteToast(LLToast* toast)
{
	// send a signal to the counter manager
	mDelNotificationSignal();

	// send a signal to a listener to let him perform some action
	// in this case listener is a SysWellWindow and it will remove a corresponding item from its list
	mNotificationIDSignal(toast->getNotificationID());
}

//--------------------------------------------------------------------------
void LLGroupHandler::onRejectToast(LLUUID& id)
{
	LLNotificationPtr notification = LLNotifications::instance().find(id);

	if (notification && LLNotificationManager::getInstance()->getHandlerForNotification(notification->getType()) == this)
	{
		LLNotifications::instance().cancel(notification);
	}
}

//--------------------------------------------------------------------------

