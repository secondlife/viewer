/**
 * @file llnotificationofferhandler.cpp
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
#include "llimview.h"
#include "llnotificationsutil.h"

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLOfferHandler::LLOfferHandler()
:	LLSysHandler("Offer", "offer")
{
	// Getting a Channel for our notifications
	mChannel = LLChannelManager::getInstance()->createNotificationChannel();
	mChannel->setControlHovering(true);

	LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel);
	if(channel)
		channel->addOnRejectToastCallback(boost::bind(&LLOfferHandler::onRejectToast, this, _1));
}

//--------------------------------------------------------------------------
LLOfferHandler::~LLOfferHandler()
{
}

//--------------------------------------------------------------------------
void LLOfferHandler::initChannel()
{
	S32 channel_right_bound = gViewerWindow->getWorldViewRectScaled().mRight - gSavedSettings.getS32("NotificationChannelRightMargin");
	S32 channel_width = gSavedSettings.getS32("NotifyBoxWidth");
	mChannel->init(channel_right_bound - channel_width, channel_right_bound);
}

//--------------------------------------------------------------------------
bool LLOfferHandler::processNotification(const LLNotificationPtr& notification)
{
	if(!mChannel)
	{
		return false;
	}

	// arrange a channel on a screen
	if(!mChannel->getVisible())
	{
		initChannel();
	}


	if( notification->getPayload().has("give_inventory_notification")
		&& notification->getPayload()["give_inventory_notification"].asBoolean() == false)
	{
		// This is an original inventory offer, so add a script floater
		LLScriptFloaterManager::instance().onAddNotification(notification->getID());
	}
	else
	{
		bool add_notif_to_im = notification->canLogToIM() && notification->hasFormElements();

		notification->setReusable(add_notif_to_im);

		LLUUID session_id;
		if (add_notif_to_im)
		{
			const std::string name = LLHandlerUtil::getSubstitutionName(notification);

			LLUUID from_id = notification->getPayload()["from_id"];

			session_id = LLHandlerUtil::spawnIMSession(name, from_id);
			LLHandlerUtil::addNotifPanelToIM(notification);
		}

		if (notification->getPayload().has("SUPPRESS_TOAST")
					&& notification->getPayload()["SUPPRESS_TOAST"])
		{
			LLNotificationsUtil::cancel(notification);
		}
		else if(!notification->canLogToIM() || !LLHandlerUtil::isIMFloaterOpened(notification))
		{
			LLToastNotifyPanel* notify_box = new LLToastNotifyPanel(notification);
			// don't close notification on panel destroy since it will be used by IM floater
			notify_box->setCloseNotificationOnDestroy(!add_notif_to_im);
			LLToast::Params p;
			p.notif_id = notification->getID();
			p.notification = notification;
			p.panel = notify_box;
			// we not save offer notifications to the syswell floater that should be added to the IM floater
			p.can_be_stored = !add_notif_to_im;

			LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel);
			if(channel)
				channel->addToast(p);
		}

		if (notification->canLogToIM())
		{
			// log only to file if notif panel can be embedded to IM and IM is opened
			bool file_only = add_notif_to_im && LLHandlerUtil::isIMFloaterOpened(notification);
			LLHandlerUtil::logToIMP2P(notification, file_only);
		}
	}

	return false;
}

/*virtual*/ void LLOfferHandler::onDelete(LLNotificationPtr notification)
{
	if( notification->getPayload().has("give_inventory_notification")
		&& !notification->getPayload()["give_inventory_notification"] )
	{
		// Remove original inventory offer script floater
		LLScriptFloaterManager::instance().onRemoveNotification(notification->getID());
	}
	else
	{
		if (notification->canLogToIM() 
			&& notification->hasFormElements()
			&& !LLHandlerUtil::isIMFloaterOpened(notification))
		{
			LLHandlerUtil::decIMMesageCounter(notification);
		}
		mChannel->killToastByNotificationID(notification->getID());
	}
}

//--------------------------------------------------------------------------
void LLOfferHandler::onRejectToast(LLUUID& id)
{
	LLNotificationPtr notification = LLNotifications::instance().find(id);

	if (notification 
		&& mItems.find(notification) != mItems.end()
		// don't delete notification since it may be used by IM floater
		&& (!notification->canLogToIM() || !notification->hasFormElements()))
	{
		LLNotifications::instance().cancel(notification);
	}
}
