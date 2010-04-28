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
#include "llnotificationsutil.h"
#include "llscreenchannel.h"
#include "llsyswellwindow.h"
#include "lltoastnotifypanel.h"
#include "lltrans.h"
#include "llviewerwindow.h"
#include "llimfloater.h"

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
: LLDockableFloater(NULL, true, key)
, mScriptForm(NULL)
, mSaveFloaterPosition(false)
{
	setMouseDownCallback(boost::bind(&LLScriptFloater::onMouseDown, this));
	setOverlapsScreenChannel(true);
}

bool LLScriptFloater::toggle(const LLUUID& notification_id)
{
	LLScriptFloater* floater = LLFloaterReg::findTypedInstance<LLScriptFloater>("script_floater", notification_id);

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
			floater->setFocus(FALSE);
		}
	}
	// create and show new floater
	else
	{
		show(notification_id);
	}

	LLBottomTray::getInstance()->getChicletPanel()->setChicletToggleState(notification_id, true);
	return true;
}

LLScriptFloater* LLScriptFloater::show(const LLUUID& notification_id)
{
	LLScriptFloater* floater = LLFloaterReg::getTypedInstance<LLScriptFloater>("script_floater", notification_id);
	floater->setNotificationId(notification_id);
	floater->createForm(notification_id);

	//LLDialog(LLGiveInventory and LLLoadURL) should no longer steal focus (see EXT-5445)
	floater->setAutoFocus(FALSE);

	if(LLScriptFloaterManager::OBJ_SCRIPT == LLScriptFloaterManager::getObjectType(notification_id))
	{
		floater->setSavePosition(true);
		floater->restorePosition();
	}
	else
	{
		floater->dockToChiclet(true);
	}

	//LLDialog(LLGiveInventory and LLLoadURL) should no longer steal focus (see EXT-5445)
	LLFloaterReg::showTypedInstance<LLScriptFloater>("script_floater", notification_id, FALSE);

	return floater;
}

void LLScriptFloater::setNotificationId(const LLUUID& id)
{
	mNotificationId = id;
	// Lets save object id now while notification exists
	mObjectId = notification_id_to_object_id(id);
}

void LLScriptFloater::getAllowedRect(LLRect& rect)
{
	rect = gViewerWindow->getWorldViewRectScaled();
}

void LLScriptFloater::createForm(const LLUUID& notification_id)
{
	// delete old form
	if(mScriptForm)
	{
		removeChild(mScriptForm);
		mScriptForm->die();
	}

	LLNotificationPtr notification = LLNotifications::getInstance()->find(notification_id);
	if(NULL == notification)
	{
		return;
	}

	// create new form
	LLRect toast_rect = getRect();
	// LLToastNotifyPanel will fit own content in vertical direction,
	// but it needs an initial rect to properly calculate  its width
 	// Use an initial rect of the script floater to make the floater window more configurable.
	mScriptForm = new LLToastNotifyPanel(notification, toast_rect); 
	addChild(mScriptForm);

	// position form on floater
	mScriptForm->setOrigin(0, 0);

	// make floater size fit form size
	LLRect panel_rect = mScriptForm->getRect();
	toast_rect.setLeftTopAndSize(toast_rect.mLeft, toast_rect.mTop, panel_rect.getWidth(), panel_rect.getHeight() + getHeaderHeight());
	setShape(toast_rect);
}

void LLScriptFloater::onClose(bool app_quitting)
{
	savePosition();

	if(getNotificationId().notNull())
	{
		LLScriptFloaterManager::getInstance()->onRemoveNotification(getNotificationId());
	}
}

void LLScriptFloater::setDocked(bool docked, bool pop_on_undock /* = true */)
{
	LLDockableFloater::setDocked(docked, pop_on_undock);

	savePosition();

	hideToastsIfNeeded();
}

void LLScriptFloater::setVisible(BOOL visible)
{
	LLDockableFloater::setVisible(visible);

	hideToastsIfNeeded();

	if(!visible)
	{
		LLIMChiclet* chiclet = LLBottomTray::getInstance()->getChicletPanel()->findChiclet<LLIMChiclet>(getNotificationId());
		if(chiclet)
		{
			chiclet->setToggleState(false);
		}
	}
}

void LLScriptFloater::onMouseDown()
{
	if(getNotificationId().notNull())
	{
		// Remove new message icon
		LLIMChiclet* chiclet = LLBottomTray::getInstance()->getChicletPanel()->findChiclet<LLIMChiclet>(getNotificationId());
		if (chiclet == NULL)
		{
			llerror("Dock chiclet for LLScriptFloater doesn't exist", 0);
		}
		else
		{
			chiclet->setShowNewMessagesIcon(false);
		}
	}
}

void LLScriptFloater::savePosition()
{
	if(getSavePosition() && mObjectId.notNull())
	{
		LLScriptFloaterManager::FloaterPositionInfo fpi = {getRect(), isDocked()};
		LLScriptFloaterManager::getInstance()->saveFloaterPosition(mObjectId, fpi);
	}
}

void LLScriptFloater::restorePosition()
{
	LLScriptFloaterManager::FloaterPositionInfo fpi;
	if(LLScriptFloaterManager::getInstance()->getFloaterPosition(mObjectId, fpi))
	{
		dockToChiclet(fpi.mDockState);
		if(!fpi.mDockState)
		{
			// Un-docked floater is opened in 0,0, now move it to saved position
			translate(fpi.mRect.mLeft - getRect().mLeft, fpi.mRect.mTop - getRect().mTop);
		}
	}
	else
	{
		dockToChiclet(true);
	}
}

void LLScriptFloater::onFocusLost()
{
	if(getNotificationId().notNull())
	{
		LLBottomTray::getInstance()->getChicletPanel()->setChicletToggleState(getNotificationId(), false);
	}
}

void LLScriptFloater::onFocusReceived()
{
	// first focus will be received before setObjectId() call - don't toggle chiclet
	if(getNotificationId().notNull())
	{
		LLBottomTray::getInstance()->getChicletPanel()->setChicletToggleState(getNotificationId(), true);
	}
}

void LLScriptFloater::dockToChiclet(bool dock)
{
	if (getDockControl() == NULL)
	{
		LLChiclet* chiclet = LLBottomTray::getInstance()->getChicletPanel()->findChiclet<LLChiclet>(getNotificationId());
		if (chiclet == NULL)
		{
			llwarns << "Dock chiclet for LLScriptFloater doesn't exist" << llendl;
			return;
		}
		else
		{
			LLBottomTray::getInstance()->getChicletPanel()->scrollToChiclet(chiclet);
		}

		// Stop saving position while we dock floater
		bool save = getSavePosition();
		setSavePosition(false);

		setDockControl(new LLDockControl(chiclet, this, getDockTongue(),
			LLDockControl::TOP,  boost::bind(&LLScriptFloater::getAllowedRect, this, _1)));

		setDocked(dock);

		// Restore saving
		setSavePosition(save);
	}
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
		channel->redrawToasts();
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void LLScriptFloaterManager::onAddNotification(const LLUUID& notification_id)
{
	if(notification_id.isNull())
	{
		llwarns << "Invalid notification ID" << llendl;
		return;
	}

	// get scripted Object's ID
	LLUUID object_id = notification_id_to_object_id(notification_id);
	
	// Need to indicate of "new message" for object chiclets according to requirements
	// specified in the Message Bar design specification. See EXT-3142.
	bool set_new_message = false;
	EObjectType obj_type = getObjectType(notification_id);

	// LLDialog can spawn only one instance, LLLoadURL and LLGiveInventory can spawn unlimited number of instances
	if(OBJ_SCRIPT == obj_type)
	{
		// If an Object spawns more-than-one floater, only the newest one is shown. 
		// The previous is automatically closed.
		script_notification_map_t::const_iterator it = findUsingObjectId(object_id);
		if(it != mNotifications.end())
		{
			LLIMChiclet* chiclet = LLBottomTray::getInstance()->getChicletPanel()->findChiclet<LLIMChiclet>(it->first);
			if(chiclet)
			{
				// Pass the new_message icon state further.
				set_new_message = chiclet->getShowNewMessagesIcon();
			}

			LLScriptFloater* floater = LLFloaterReg::findTypedInstance<LLScriptFloater>("script_floater", it->first);
			if(floater)
			{
				// Generate chiclet with a "new message" indicator if a docked window was opened but not in focus. See EXT-3142.
				set_new_message |= !floater->hasFocus();
			}

			onRemoveNotification(it->first);
		}
	}

	mNotifications.insert(std::make_pair(notification_id, object_id));

	// Create inventory offer chiclet for offer type notifications
	if( OBJ_GIVE_INVENTORY == obj_type )
	{
		LLBottomTray::instance().getChicletPanel()->createChiclet<LLInvOfferChiclet>(notification_id);
	}
	else
	{
		LLBottomTray::getInstance()->getChicletPanel()->createChiclet<LLScriptChiclet>(notification_id);
	}

	LLIMWellWindow::getInstance()->addObjectRow(notification_id, set_new_message);

	LLSD data;
	data["notification_id"] = notification_id;
	data["new_message"] = set_new_message;
	data["unread"] = 1; // each object has got only one floater
	mNewObjectSignal(data);

	toggleScriptFloater(notification_id, set_new_message);
}

void LLScriptFloaterManager::onRemoveNotification(const LLUUID& notification_id)
{
	if(notification_id.isNull())
	{
		llwarns << "Invalid notification ID" << llendl;
		return;
	}

	// remove related chiclet
	LLBottomTray::getInstance()->getChicletPanel()->removeChiclet(notification_id);

	LLIMWellWindow::getInstance()->removeObjectRow(notification_id);

	// close floater
	LLScriptFloater* floater = LLFloaterReg::findTypedInstance<LLScriptFloater>("script_floater", notification_id);
	if(floater)
	{
		floater->savePosition();
		floater->setNotificationId(LLUUID::null);
		floater->closeFloater();
	}

	mNotifications.erase(notification_id);
}

void LLScriptFloaterManager::toggleScriptFloater(const LLUUID& notification_id, bool set_new_message)
{
	LLSD data;
	data["notification_id"] = notification_id;
	data["new_message"] = set_new_message;
	mToggleFloaterSignal(data);

	// toggle floater
	LLScriptFloater::toggle(notification_id);
}

LLUUID LLScriptFloaterManager::findObjectId(const LLUUID& notification_id)
{
	script_notification_map_t::const_iterator it = mNotifications.find(notification_id);
	if(mNotifications.end() != it)
	{
		return it->second;
	}
	return LLUUID::null;
}

LLUUID LLScriptFloaterManager::findNotificationId(const LLUUID& object_id)
{
	if(object_id.notNull())
	{
		script_notification_map_t::const_iterator it = findUsingObjectId(object_id);
		if(mNotifications.end() != it)
		{
			return it->first;
		}
	}
	return LLUUID::null;
}

// static
LLScriptFloaterManager::EObjectType LLScriptFloaterManager::getObjectType(const LLUUID& notification_id)
{
	if(notification_id.isNull())
	{
		llwarns << "Invalid notification ID" << llendl;
		return OBJ_UNKNOWN;
	}

	static const object_type_map TYPE_MAP = initObjectTypeMap();

	LLNotificationPtr notification = LLNotificationsUtil::find(notification_id);
	object_type_map::const_iterator it = TYPE_MAP.find(notification->getName());
	if(it != TYPE_MAP.end())
	{
		return it->second;
	}

	llwarns << "Unknown object type" << llendl;
	return OBJ_UNKNOWN;
}

// static
std::string LLScriptFloaterManager::getObjectName(const LLUUID& notification_id)
{
	using namespace LLNotificationsUI;
	LLNotificationPtr notification = LLNotifications::getInstance()->find(notification_id);
	if(!notification)
	{
		llwarns << "Invalid notification" << llendl;
		return LLStringUtil::null;
	}

	std::string text;

	switch(LLScriptFloaterManager::getObjectType(notification_id))
	{
	case LLScriptFloaterManager::OBJ_SCRIPT:
		text = notification->getSubstitutions()["TITLE"].asString();
		break;
	case LLScriptFloaterManager::OBJ_LOAD_URL:
		text = notification->getSubstitutions()["OBJECTNAME"].asString();
		break;
	case LLScriptFloaterManager::OBJ_GIVE_INVENTORY:
		text = notification->getSubstitutions()["NAME"].asString();
		break;
	default:
		text = LLTrans::getString("object");
		break;
	}

	return text;
}

//static
LLScriptFloaterManager::object_type_map LLScriptFloaterManager::initObjectTypeMap()
{
	object_type_map type_map;
	type_map["ScriptDialog"] = OBJ_SCRIPT;
	type_map["ScriptDialogGroup"] = OBJ_SCRIPT;
	type_map["LoadWebPage"] = OBJ_LOAD_URL;
	type_map["ObjectGiveItem"] = OBJ_GIVE_INVENTORY;
	return type_map;
}

LLScriptFloaterManager::script_notification_map_t::const_iterator LLScriptFloaterManager::findUsingObjectId(const LLUUID& object_id)
{
	script_notification_map_t::const_iterator it = mNotifications.begin();
	for(; mNotifications.end() != it; ++it)
	{
		if(object_id == it->second)
		{
			return it;
		}
	}
	return mNotifications.end();
}

void LLScriptFloaterManager::saveFloaterPosition(const LLUUID& object_id, const FloaterPositionInfo& fpi)
{
	if(object_id.notNull())
	{
		LLScriptFloaterManager::getInstance()->mFloaterPositions[object_id] = fpi;
	}
	else
	{
		llwarns << "Invalid object id" << llendl;
	}
}

bool LLScriptFloaterManager::getFloaterPosition(const LLUUID& object_id, FloaterPositionInfo& fpi)
{
	floater_position_map_t::const_iterator it = mFloaterPositions.find(object_id);
	if(LLScriptFloaterManager::getInstance()->mFloaterPositions.end() != it)
	{
		fpi = it->second;
		return true;
	}
	return false;
}

void LLScriptFloaterManager::setFloaterVisible(const LLUUID& notification_id, bool visible)
{
	LLScriptFloater* floater = LLFloaterReg::findTypedInstance<LLScriptFloater>(
		"script_floater", notification_id);
	if(floater)
	{
		floater->setVisible(visible);
	}
}

// EOF
