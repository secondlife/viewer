/** 
 * @file llnotificationalerthandler.cpp
 * @brief Notification Handler Class for Alert Notifications
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

#include "llnotifications.h"
#include "llprogressview.h"
#include "lltoastnotifypanel.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

#include "lltoastalertpanel.h"

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLAlertHandler::LLAlertHandler(e_notification_type type, const LLSD& id) : mIsModal(false)
{
	mType = type;

	LLScreenChannelBase::Params p;
	p.id = LLUUID(gSavedSettings.getString("AlertChannelUUID"));
	p.display_toasts_always = true;
	p.toast_align = NA_CENTRE;
	p.channel_align = CA_CENTRE;

	// Getting a Channel for our notifications
	mChannel = LLChannelManager::getInstance()->getChannel(p);
	mChannel->setCanStoreToasts(false);
}

//--------------------------------------------------------------------------
LLAlertHandler::~LLAlertHandler()
{
}

//--------------------------------------------------------------------------
void LLAlertHandler::initChannel()
{
	S32 channel_right_bound = gViewerWindow->getWorldViewRectScaled().getWidth() / 2;
	mChannel->init(channel_right_bound, channel_right_bound);
}

//--------------------------------------------------------------------------
bool LLAlertHandler::processNotification(const LLSD& notify)
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

	if (notify["sigtype"].asString() == "add" || notify["sigtype"].asString() == "load")
	{
		if (LLHandlerUtil::canSpawnSessionAndLogToIM(notification))
		{
			const std::string name = LLHandlerUtil::getSubstitutionName(notification);

			LLUUID from_id = notification->getPayload()["from_id"];

			// firstly create session...
			LLHandlerUtil::spawnIMSession(name, from_id);

			// ...then log message to have IM Well notified about new message
			LLHandlerUtil::logToIMP2P(notification);
		}

		LLToastAlertPanel* alert_dialog = new LLToastAlertPanel(notification, mIsModal);
		LLToast::Params p;
		p.notif_id = notification->getID();
		p.notification = notification;
		p.panel = dynamic_cast<LLToastPanel*>(alert_dialog);
		p.enable_hide_btn = false;
		p.can_fade = false;
		p.is_modal = mIsModal;
		p.on_delete_toast = boost::bind(&LLAlertHandler::onDeleteToast, this, _1);

		// Show alert in middle of progress view (during teleport) (EXT-1093)
		LLProgressView* progress = gViewerWindow->getProgressView();
		LLRect rc = progress && progress->getVisible() ? progress->getRect() : gViewerWindow->getWorldViewRectScaled();
		mChannel->updatePositionAndSize(rc);

		LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel);
		if(channel)
			channel->addToast(p);
	}
	else if (notify["sigtype"].asString() == "change")
	{
		LLToastAlertPanel* alert_dialog = new LLToastAlertPanel(notification, mIsModal);
		LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel);
		if(channel)
			channel->modifyToastByNotificationID(notification->getID(), (LLToastPanel*)alert_dialog);
	}
	else
	{
		LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel);
		if(channel)
			channel->killToastByNotificationID(notification->getID());
	}
	return false;
}

//--------------------------------------------------------------------------

void LLAlertHandler::onDeleteToast(LLToast* toast)
{
}

//--------------------------------------------------------------------------

