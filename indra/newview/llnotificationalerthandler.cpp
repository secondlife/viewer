/** 
 * @file llnotificationalerthandler.cpp
 * @brief Notification Handler Class for Alert Notifications
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

	LLChannelManager::Params p;
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
		mChannel->updatePositionAndSize(rc, rc);

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
	return true;
}

//--------------------------------------------------------------------------

void LLAlertHandler::onDeleteToast(LLToast* toast)
{
}

//--------------------------------------------------------------------------

