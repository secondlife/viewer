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
LLGroupHandler::LLGroupHandler()
:	LLSysHandler("Group Notifications", "groupnotify")
{
	// Getting a Channel for our notifications
	LLScreenChannel* channel = LLChannelManager::getInstance()->createNotificationChannel();
	if(channel)
	{
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
bool LLGroupHandler::processNotification(const LLNotificationPtr& notification)
{
	if(mChannel.isDead())
	{
		return false;
	}

	// arrange a channel on a screen
	if(!mChannel.get()->getVisible())
	{
		initChannel();
	}
	
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

	LLGroupActions::refresh_notices();

	return false;
}


//--------------------------------------------------------------------------

