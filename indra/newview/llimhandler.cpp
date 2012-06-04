/** 
 * @file llimhandler.cpp
 * @brief Notification Handler Class for IM notifications
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

#include "llagentdata.h"
#include "llnotifications.h"
#include "lltoastimpanel.h"
#include "llviewerwindow.h"

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLIMHandler::LLIMHandler(e_notification_type type, const LLSD& id)
{
	mType = type;

	// Getting a Channel for our notifications
	mChannel = LLChannelManager::getInstance()->createNotificationChannel()->getHandle();
}

//--------------------------------------------------------------------------
LLIMHandler::~LLIMHandler()
{
}

//--------------------------------------------------------------------------
void LLIMHandler::initChannel()
{
	S32 channel_right_bound = gViewerWindow->getWorldViewRectScaled().mRight - gSavedSettings.getS32("NotificationChannelRightMargin"); 
	S32 channel_width = gSavedSettings.getS32("NotifyBoxWidth");
	mChannel.get()->init(channel_right_bound - channel_width, channel_right_bound);
}

//--------------------------------------------------------------------------
bool LLIMHandler::processNotification(const LLSD& notify)
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
		LLSD substitutions = notification->getSubstitutions();

		// According to comments in LLIMMgr::addMessage(), if we get message
		// from ourselves, the sender id is set to null. This fixes EXT-875.
		LLUUID avatar_id = substitutions["FROM_ID"].asUUID();
		if (avatar_id.isNull())
			avatar_id = gAgentID;

		LLToastIMPanel::Params im_p;
		im_p.notification = notification;
		im_p.avatar_id = avatar_id;
		im_p.from = substitutions["FROM"].asString();
		im_p.time = substitutions["TIME"].asString();
		im_p.message = substitutions["MESSAGE"].asString();
		im_p.session_id = substitutions["SESSION_ID"].asUUID();

		LLToastIMPanel* im_box = new LLToastIMPanel(im_p);

		LLToast::Params p;
		p.notif_id = notification->getID();
		p.session_id = im_p.session_id;
		p.notification = notification;
		p.panel = im_box;
		p.can_be_stored = false;
		p.on_delete_toast = boost::bind(&LLIMHandler::onDeleteToast, this, _1);
		LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel.get());
		if(channel)
			channel->addToast(p);

		// send a signal to the counter manager;
		mNewNotificationSignal();
	}
	else if (notify["sigtype"].asString() == "delete")
	{
		mChannel.get()->killToastByNotificationID(notification->getID());
	}
	return false;
}

//--------------------------------------------------------------------------
void LLIMHandler::onDeleteToast(LLToast* toast)
{
	// send a signal to the counter manager
	mDelNotificationSignal();
}

//--------------------------------------------------------------------------


