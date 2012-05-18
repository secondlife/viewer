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
	LLScreenChannel* channel = LLChannelManager::getInstance()->createNotificationChannel();
	if(channel)
	{
		channel->setControlHovering(true);
		mChannel = channel->getHandle();
	}
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
	mChannel.get()->init(channel_right_bound - channel_width, channel_right_bound);
}

//--------------------------------------------------------------------------
bool LLOfferHandler::processNotification(const LLNotificationPtr& notification)
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


	if( notification->getPayload().has("give_inventory_notification")
		&& notification->getPayload()["give_inventory_notification"].asBoolean() == false)
	{
		// This is an original inventory offer, so add a script floater
		LLScriptFloaterManager::instance().onAddNotification(notification->getID());
	}
	else
	{
		bool add_notif_to_im = notification->canLogToIM() && notification->hasFormElements();

		if (add_notif_to_im)
		{
			const std::string name = LLHandlerUtil::getSubstitutionName(notification);

			LLUUID from_id = notification->getPayload()["from_id"];

			LLHandlerUtil::spawnIMSession(name, from_id);
			LLHandlerUtil::addNotifPanelToIM(notification);
		}

		if (!notification->canShowToast())
		{
			LLNotificationsUtil::cancel(notification);
		}
		else if(!notification->canLogToIM() || !LLHandlerUtil::isIMFloaterOpened(notification))
		{
			LLToastNotifyPanel* notify_box = new LLToastNotifyPanel(notification);
			LLToast::Params p;
			p.notif_id = notification->getID();
			p.notification = notification;
			p.panel = notify_box;
			// we not save offer notifications to the syswell floater that should be added to the IM floater
			p.can_be_stored = !add_notif_to_im;

			LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(mChannel.get());
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

/*virtual*/ void LLOfferHandler::onChange(LLNotificationPtr p)
{
	LLToastNotifyPanel* panelp = LLToastNotifyPanel::getInstance(p->getID());
	if (panelp)
	{
		//
		// HACK: if we're dealing with a notification embedded in IM, update it
		// otherwise remove its toast
		//
		if (dynamic_cast<LLIMToastNotifyPanel*>(panelp))
		{
			panelp->updateNotification();
		}
		else
		{
			// if notification has changed, hide it
			mChannel.get()->removeToastByNotificationID(p->getID());
		}
	}
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
		mChannel.get()->removeToastByNotificationID(notification->getID());
	}
}

