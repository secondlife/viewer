/** 
 * @file llnotificationtiphandler.cpp
 * @brief Notification Handler Class for Notification Tips
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

#include "llfloaterreg.h"
#include "llnearbychat.h"
#include "llnotificationhandler.h"
#include "lltoastnotifypanel.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

using namespace LLNotificationsUI;


//--------------------------------------------------------------------------
LLTipHandler::LLTipHandler(e_notification_type type, const LLSD& id)
{
	mType = type;	

	// Getting a Channel for our notifications
	mChannel = LLChannelManager::getInstance()->createNotificationChannel();
}

//--------------------------------------------------------------------------
LLTipHandler::~LLTipHandler()
{
}

//--------------------------------------------------------------------------
void LLTipHandler::initChannel()
{
	S32 channel_right_bound = gViewerWindow->getWorldViewRect().mRight - gSavedSettings.getS32("NotificationChannelRightMargin"); 
	S32 channel_width = gSavedSettings.getS32("NotifyBoxWidth");
	mChannel->init(channel_right_bound - channel_width, channel_right_bound);
}

//--------------------------------------------------------------------------
bool LLTipHandler::processNotification(const LLSD& notify)
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
		LLToastNotifyPanel* notify_box = new LLToastNotifyPanel(notification);

		LLToast::Params p;
		p.notif_id = notification->getID();
		p.notification = notification;
		p.panel = notify_box;
		p.is_tip = true;
		p.can_be_stored = false;
		
		LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel);
		if(channel)
			channel->addToast(p);

		// archive message in nearby chat
		LLNearbyChat* nearby_chat = LLFloaterReg::getTypedInstance<LLNearbyChat>("nearby_chat", LLSD());
		if(nearby_chat)
		{
			LLChat chat_msg(notification->getMessage());
			nearby_chat->addMessage(chat_msg);
		}
	}
	else if (notify["sigtype"].asString() == "delete")
	{
		mChannel->killToastByNotificationID(notification->getID());
	}
	return true;
}

//--------------------------------------------------------------------------
void LLTipHandler::onDeleteToast(LLToast* toast)
{
}

//--------------------------------------------------------------------------


