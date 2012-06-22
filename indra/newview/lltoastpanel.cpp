/** 
 * @file lltoastpanel.cpp
 * @brief Creates a panel of a specific kind for a toast
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

#include "llviewerprecompiledheaders.h"

#include "llpanelgenerictip.h"
#include "llpanelonlinestatus.h"
#include "llnotifications.h"
#include "lltoastnotifypanel.h"
#include "lltoastpanel.h"
#include "lltoastscriptquestion.h"

//static
const S32 LLToastPanel::MIN_PANEL_HEIGHT = 40; // VPAD(4)*2 + ICON_HEIGHT(32)

LLToastPanel::LLToastPanel(const LLNotificationPtr& notification)
{
	mNotification = notification;
}

LLToastPanel::~LLToastPanel() 
{
}

//virtual
std::string LLToastPanel::getTitle()
{
	// *TODO: create Title and localize it. If it will be required.
	return mNotification->getMessage();
}

//virtual
const LLUUID& LLToastPanel::getID()
{
	return mNotification->id();
}

//snap to the message height if it is visible
void LLToastPanel::snapToMessageHeight(LLTextBase* message, S32 maxLineCount)
{
	if(!message)
	{
		return;
	}

	//Add message height if it is visible
	if (message->getVisible())
	{
		S32 heightDelta = 0;
		S32 maxTextHeight = message->getDefaultFont()->getLineHeight() * maxLineCount;

		LLRect messageRect = message->getRect();
		S32 oldTextHeight = messageRect.getHeight();

		//Knowing the height is set to max allowed, getTextPixelHeight returns needed text height
		//Perhaps we need to pass maxLineCount as parameter to getTextPixelHeight to avoid previous reshape.
		S32 requiredTextHeight = message->getTextBoundingRect().getHeight();
		S32 newTextHeight = llmin(requiredTextHeight, maxTextHeight);

		//Calculate last delta height deducting previous heightDelta 
		heightDelta = newTextHeight - oldTextHeight - heightDelta;

		//reshape the panel with new height
		reshape( getRect().getWidth(), llmax(getRect().getHeight() + heightDelta, MIN_PANEL_HEIGHT));
	}
}

// static
LLToastPanel* LLToastPanel::buidPanelFromNotification(
		const LLNotificationPtr& notification)
{
	LLToastPanel* res = NULL;

	//process tip toast panels
	if ("notifytip" == notification->getType())
	{
		// if it is online/offline notification
		if ("FriendOffline" == notification->getName() || "FriendOnline" == notification->getName())
		{
			res = new LLPanelOnlineStatus(notification);
		}
		// in all other case we use generic tip panel
		else
		{
			res = new LLPanelGenericTip(notification);
		}
	}
	else if("notify" == notification->getType())
	{
		if (notification->getPriority() == NOTIFICATION_PRIORITY_CRITICAL)
		{
			res = new LLToastScriptQuestion(notification);
		}
		else
		{
			res = new LLToastNotifyPanel(notification);
		}
	}
	/*
	 else if(...)
	 create all other specific non-public toast panel
	 */

	return res;
}
