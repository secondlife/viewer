/**
 * @file llbrowsernotification.cpp
 * @brief Notification Handler Class for browser popups
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
#include "llfloaterreg.h"
#include "llmediactrl.h"

using namespace LLNotificationsUI;

bool LLBrowserNotification::processNotification(const LLSD& notify)
{
	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());
	if (!notification) return false;

	LLMediaCtrl* media_instance = LLMediaCtrl::getInstance(notification->getPayload()["media_id"].asUUID());
	if (media_instance)
	{
		media_instance->showNotification(notification);
	}
	return false;
}
