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
#include "llmediactrl.h"
#include "llviewermedia.h"
#include "llviewermediafocus.h"

using namespace LLNotificationsUI;

bool LLBrowserNotification::processNotification(const LLSD& notify)
{
	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());
	if (!notification) return false;

	LLUUID media_id = notification->getPayload()["media_id"].asUUID();
	LLMediaCtrl* media_instance = LLMediaCtrl::getInstance(media_id);
	if (media_instance)
	{
		media_instance->showNotification(notification);
	}
	else if (LLViewerMediaFocus::instance().getControlsMediaID() == media_id)
	{
		LLViewerMediaImpl* impl = LLViewerMedia::getMediaImplFromTextureID(media_id);
		if (impl)
		{
			impl->showNotification(notification);
		}
	}
	return false;
}
