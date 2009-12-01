/** 
 * @file llnotificationgrouphandler.cpp
 * @brief Notification Handler Class for Group Notifications
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
	mChannel = LLChannelManager::getInstance()->createNotificationChannel();
	LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel);
	if(channel)
		channel->setOnRejectToastCallback(boost::bind(&LLGroupHandler::onRejectToast, this, _1));
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
	mChannel->init(channel_right_bound - channel_width, channel_right_bound);
}

//--------------------------------------------------------------------------
bool LLGroupHandler::processNotification(const LLSD& notify)
{
	if(!mChannel)
	{
		return false;
	}

	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());

	if(!notification)
		return false;

	// arrange a channel on a screen
	if(!mChannel->getVisible())
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

		LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel);
		if(channel)
			channel->addToast(p);

		// send a signal to the counter manager
		mNewNotificationSignal();

		LLGroupActions::refresh_notices();
	}
	else if (notify["sigtype"].asString() == "delete")
	{
		mChannel->killToastByNotificationID(notification->getID());
	}
	return true;
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

