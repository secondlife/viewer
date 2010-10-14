/**
* @file llnotificationstorage.h
* @brief LLNotificationStorage class declaration
*
* $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_NOTIFICATIONSTORAGE_H
#define LL_NOTIFICATIONSTORAGE_H

#include "llnotifications.h"

// Class that saves not responded(unread) notifications.
// Unread notifications are saved in open_notifications.xml in SL account folder
//
// Notifications that should be saved(if unread) are marked with persist="true" in notifications.xml
// Notifications using functor responders are saved automatically (see llviewermessage.cpp
// lure_callback_reg for example).
// Notifications using object responders(LLOfferInfo) need additional tuning. Responder object should
// be a) serializable(implement LLNotificationResponderInterface),
// b) registered with LLResponderRegistry (found in llnotificationstorage.cpp).
class LLPersistentNotificationStorage : public LLSingleton<LLPersistentNotificationStorage>
{
	LOG_CLASS(LLPersistentNotificationStorage);
public:

	LLPersistentNotificationStorage();

	void saveNotifications();

	void loadNotifications();

private:

	bool onPersistentChannelChanged(const LLSD& payload);

	std::string mFileName;
};

#endif // LL_NOTIFICATIONSTORAGE_H
