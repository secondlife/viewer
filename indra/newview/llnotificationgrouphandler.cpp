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
#include "llagent.h"
#include "llbottomtray.h"
#include "llviewercontrol.h"

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLGroupHandler::LLGroupHandler(e_notification_type type, const LLSD& id)
{
	mType = type;

	// getting a Chiclet and creating params for a channel
	LLBottomTray* tray = LLBottomTray::getInstance();
	mChiclet = tray->getSysWell();
	LLChannelManager::Params p;
	p.chiclet = mChiclet;
	p.channel_right_bound = tray->getRect().mRight - 10; //HACK: need to correctly resolve SysWell's position
	p.channel_width = gSavedSettings.getS32("NotifyBoxWidth");

	// Getting a Channel for our notifications
	mChannel = LLChannelManager::getInstance()->createChannel(p);
}

//--------------------------------------------------------------------------
LLGroupHandler::~LLGroupHandler()
{
}

//--------------------------------------------------------------------------
void LLGroupHandler::processNotification(const LLSD& notify)
{
	LLToast* toast = NULL;

	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());
	if(notify["sigtype"].asString() == "add" || notify["sigtype"].asString() == "change")
	{
			LLPanel* notify_box = new LLToastGroupNotifyPanel(notification);
			toast = mChannel->addToast(notification->getID(), notify_box);
			if(!toast)
				return;			
			toast->setAndStartTimer(5); 
			toast->setOnToastDestroyCallback((boost::bind(&LLGroupHandler::onToastDestroy, this, toast)));
			mChiclet->setCounter(mChiclet->getCounter() + 1);
	}
	else if (notify["sigtype"].asString() == "delete")
	{
		mChannel->killToastByNotificationID(notification->getID());
	}
}

//--------------------------------------------------------------------------
void LLGroupHandler::onToastDestroy(LLToast* toast)
{
	mChiclet->setCounter(mChiclet->getCounter() - 1);
	toast->close();
}

//--------------------------------------------------------------------------
void LLGroupHandler::onChicletClick(void)
{
}

//--------------------------------------------------------------------------
void LLGroupHandler::onChicletClose(void)
{
}

//--------------------------------------------------------------------------


//--------------------------------------------------------------------------


//--------------------------------------------------------------------------


//--------------------------------------------------------------------------


//--------------------------------------------------------------------------

