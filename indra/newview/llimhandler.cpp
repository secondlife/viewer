/** 
 * @file llimhandler.cpp
 * @brief Notification Handler Class for IM notifications
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

#include "llagentdata.h"
#include "llbottomtray.h"
#include "llviewercontrol.h"
#include "lltoastimpanel.h"

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLIMHandler::LLIMHandler()
{
	
	// getting a Chiclet and creating params for a channel
	LLBottomTray* tray = LLBottomTray::getInstance();
	mChiclet = tray->getSysWell();

	LLChannelManager::Params p;
	// *TODO: createNotificationChannel method
	p.id = LLUUID(gSavedSettings.getString("NotificationChannelUUID"));
	p.channel_right_bound = tray->getRect().mRight - gSavedSettings.getS32("NotificationChannelRightMargin"); 
	p.channel_width = gSavedSettings.getS32("NotifyBoxWidth");

	// Getting a Channel for our notifications
	mChannel = LLChannelManager::getInstance()->createChannel(p);

}

//--------------------------------------------------------------------------
LLIMHandler::~LLIMHandler()
{
}

//--------------------------------------------------------------------------
void LLIMHandler::processNotification(const LLSD& notify)
{
	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());

	if(!notification)
		return;

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
		p.id = notification->getID();
		p.notification = notification;
		p.panel = im_box;
		p.can_be_stored = false;
		p.on_toast_destroy = boost::bind(&LLIMHandler::onToastDestroy, this, _1);
		mChannel->addToast(p);


		static_cast<LLNotificationChiclet*>(mChiclet)->updateUreadIMNotifications();
	}
	else if (notify["sigtype"].asString() == "delete")
	{
		mChannel->killToastByNotificationID(notification->getID());
	}
}

//--------------------------------------------------------------------------
void LLIMHandler::onToastDestroy(LLToast* toast)
{
	toast->closeFloater();
	static_cast<LLNotificationChiclet*>(mChiclet)->updateUreadIMNotifications();
}

//--------------------------------------------------------------------------
void LLIMHandler::onChicletClick(void)
{
}

//--------------------------------------------------------------------------
void LLIMHandler::onChicletClose(void)
{
}

//--------------------------------------------------------------------------



