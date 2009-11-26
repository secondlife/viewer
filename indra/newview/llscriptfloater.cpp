/** 
 * @file llscriptfloater.cpp
 * @brief LLScriptFloater class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llscriptfloater.h"

#include "llbottomtray.h"
#include "llchannelmanager.h"
#include "llchiclet.h"
#include "llfloaterreg.h"
#include "llnotifications.h"
#include "llscreenchannel.h"
#include "lltoastnotifypanel.h"
#include "llviewerwindow.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLUUID notification_id_to_object_id(const LLUUID& notification_id)
{
	LLNotificationPtr notification = LLNotifications::getInstance()->find(notification_id);
	if(notification)
	{
		return notification->getPayload()["object_id"].asUUID();
	}
	return LLUUID::null;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLScriptFloater::LLScriptFloater(const LLSD& key)
: LLTransientDockableFloater(NULL, true, key)
, mScriptForm(NULL)
, mObjectId(key.asUUID())
{
}

bool LLScriptFloater::toggle(const LLUUID& object_id)
{
	LLScriptFloater* floater = LLFloaterReg::findTypedInstance<LLScriptFloater>("script_floater", object_id);

	// show existing floater
	if(floater)
	{
		if(floater->getVisible())
		{
			floater->setVisible(false);
			return false;
		}
		else
		{
			floater->setVisible(TRUE);
			floater->setFocus(TRUE);
			return true;
		}
	}
	// create and show new floater
	else
	{
		show(object_id);
		return true;
	}
}

LLScriptFloater* LLScriptFloater::show(const LLUUID& object_id)
{
	LLScriptFloater* floater = LLFloaterReg::showTypedInstance<LLScriptFloater>("script_floater", object_id);
	floater->createForm(object_id);

	if (floater->getDockControl() == NULL)
	{
		LLChiclet* chiclet = LLBottomTray::getInstance()->getChicletPanel()->findChiclet<LLChiclet>(object_id);
		if (chiclet == NULL)
		{
			llerror("Dock chiclet for LLScriptFloater doesn't exist", 0);
		}
		else
		{
			LLBottomTray::getInstance()->getChicletPanel()->scrollToChiclet(chiclet);
		}

		floater->setDockControl(new LLDockControl(chiclet, floater, floater->getDockTongue(),
			LLDockControl::TOP,  boost::bind(&LLScriptFloater::getAllowedRect, floater, _1)));
	}

	return floater;
}

void LLScriptFloater::getAllowedRect(LLRect& rect)
{
	rect = gViewerWindow->getWorldViewRectRaw();
}

void LLScriptFloater::createForm(const LLUUID& object_id)
{
	// delete old form
	if(mScriptForm)
	{
		removeChild(mScriptForm);
		mScriptForm->die();
	}

	LLNotificationPtr notification = LLNotifications::getInstance()->find(
		LLScriptFloaterManager::getInstance()->findNotificationId(object_id));
	if(NULL == notification)
	{
		return;
	}

	// create new form
	mScriptForm = new LLToastNotifyPanel(notification);
	addChild(mScriptForm);

	// position form on floater
	mScriptForm->setOrigin(0, 0);

	// make floater size fit form size
	LLRect toast_rect = getRect();
	LLRect panel_rect = mScriptForm->getRect();
	toast_rect.setLeftTopAndSize(toast_rect.mLeft, toast_rect.mTop, panel_rect.getWidth(), panel_rect.getHeight() + getHeaderHeight());
	setShape(toast_rect);
}

void LLScriptFloater::onClose(bool app_quitting)
{
	LLScriptFloaterManager::getInstance()->removeNotificationByObjectId(getObjectId());
}

void LLScriptFloater::setDocked(bool docked, bool pop_on_undock /* = true */)
{
	LLTransientDockableFloater::setDocked(docked, pop_on_undock);

	hideToastsIfNeeded();
}

void LLScriptFloater::setVisible(BOOL visible)
{
	LLTransientDockableFloater::setVisible(visible);

	hideToastsIfNeeded();
}

void LLScriptFloater::hideToastsIfNeeded()
{
	using namespace LLNotificationsUI;

	// find channel
	LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(LLChannelManager::getInstance()->findChannelByID(
		LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));
	// update notification channel state
	if(channel)
	{
		channel->updateShowToastsState();
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void LLScriptFloaterManager::onAddNotification(const LLUUID& notification_id)
{
	// get scripted Object's ID
	LLUUID object_id = notification_id_to_object_id(notification_id);
	if(object_id.isNull())
	{
		llwarns << "Invalid notification, no object id" << llendl;
		return;
	}

	// If an Object spawns more-than-one floater, only the newest one is shown. 
	// The previous is automatically closed.
	script_notification_map_t::iterator it = mNotifications.find(object_id);
	if(it != mNotifications.end())
	{
		onRemoveNotification(notification_id);
	}

	LLNotificationData nd = {notification_id};
	mNotifications.insert(std::make_pair(object_id, nd));

	LLBottomTray::getInstance()->getChicletPanel()->createChiclet<LLScriptChiclet>(object_id);
}

void LLScriptFloaterManager::onRemoveNotification(const LLUUID& notification_id)
{
	LLUUID object_id = notification_id_to_object_id(notification_id);
	if(object_id.isNull())
	{
		llwarns << "Invalid notification, no object id" << llendl;
		return;
	}

	using namespace LLNotificationsUI;

	// remove related toast
	LLUUID channel_id(gSavedSettings.getString("NotificationChannelUUID"));
	LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>
		(LLChannelManager::getInstance()->findChannelByID(channel_id));
	if(channel)
	{
		channel->killToastByNotificationID(findNotificationToastId(object_id));
	}

	mNotifications.erase(object_id);

	// remove related chiclet
	LLBottomTray::getInstance()->getChicletPanel()->removeChiclet(object_id);

	// close floater
	LLScriptFloater* floater = LLFloaterReg::findTypedInstance<LLScriptFloater>("script_floater", object_id);
	if(floater)
	{
		floater->closeFloater();
	}
}

void LLScriptFloaterManager::removeNotificationByObjectId(const LLUUID& object_id)
{
	// Check we have not removed notification yet
	LLNotificationPtr notification = LLNotifications::getInstance()->find(
		findNotificationId(object_id));
	if(notification)
	{
		onRemoveNotification(notification->getID());
	}
}

void LLScriptFloaterManager::toggleScriptFloater(const LLUUID& object_id)
{
	// hide "new message" icon from chiclet
	LLIMChiclet* chiclet = LLBottomTray::getInstance()->getChicletPanel()->findChiclet<LLIMChiclet>(object_id);
	if(chiclet)
	{
		chiclet->setShowNewMessagesIcon(false);
	}

	// kill toast
	using namespace LLNotificationsUI;
	LLScreenChannel* channel = dynamic_cast<LLScreenChannel*>(LLChannelManager::getInstance()->findChannelByID(
		LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));
	if(channel)
	{
		channel->killToastByNotificationID(findNotificationToastId(object_id));
	}

	// toggle floater
	LLScriptFloater::toggle(object_id);
}

void LLScriptFloaterManager::setNotificationToastId(const LLUUID& object_id, const LLUUID& notification_id)
{
	script_notification_map_t::iterator it = mNotifications.find(object_id);
	if(mNotifications.end() != it)
	{
		it->second.toast_notification_id = notification_id;
	}
}

LLUUID LLScriptFloaterManager::findNotificationId(const LLUUID& object_id)
{
	script_notification_map_t::const_iterator it = mNotifications.find(object_id);
	if(mNotifications.end() != it)
	{
		return it->second.notification_id;
	}
	return LLUUID::null;
}

LLUUID LLScriptFloaterManager::findNotificationToastId(const LLUUID& object_id)
{
	script_notification_map_t::const_iterator it = mNotifications.find(object_id);
	if(mNotifications.end() != it)
	{
		return it->second.toast_notification_id;
	}
	return LLUUID::null;
}

//static
void LLScriptFloaterManager::onToastButtonClick(const LLSD&notification, const LLSD&response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	LLUUID object_id = notification["payload"]["object_id"].asUUID();

	switch(option)
	{
	case 0: // "Open"
		LLScriptFloaterManager::getInstance()->toggleScriptFloater(object_id);
		break;
	case 1: // "Ignore"
		LLScriptFloaterManager::getInstance()->removeNotificationByObjectId(object_id);
		break;
	case 2: // "Block"
		LLMuteList::getInstance()->add(LLMute(object_id, notification["substitutions"]["TITLE"], LLMute::OBJECT));
		LLScriptFloaterManager::getInstance()->removeNotificationByObjectId(object_id);
		break;
	default:
		llwarns << "Unexpected value" << llendl;
		break;
	}
}

// EOF
