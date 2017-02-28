/** 
* @file   llpersistentnotificationstorage.h
* @brief  Header file for llpersistentnotificationstorage
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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
#ifndef LL_LLPERSISTENTNOTIFICATIONSTORAGE_H
#define LL_LLPERSISTENTNOTIFICATIONSTORAGE_H

#include "llerror.h"
#include "llnotificationstorage.h"
#include "llsingleton.h"

class LLSD;

// Class that saves not responded(unread) notifications.
// Unread notifications are saved in open_notifications.xml in SL account folder
//
// Notifications that should be saved(if unread) are marked with persist="true" in notifications.xml
// Notifications using functor responders are saved automatically (see llviewermessage.cpp
// lure_callback_reg for example).
// Notifications using object responders(LLOfferInfo) need additional tuning. Responder object should
// be a) serializable(implement LLNotificationResponderInterface),
// b) registered with LLResponderRegistry (found in llpersistentnotificationstorage.cpp).

class LLPersistentNotificationStorage : public LLSingleton<LLPersistentNotificationStorage>, public LLNotificationStorage
{
	LLSINGLETON(LLPersistentNotificationStorage);
	~LLPersistentNotificationStorage();
	LOG_CLASS(LLPersistentNotificationStorage);
public:

	void saveNotifications();
	void loadNotifications();

	void initialize();

protected:

private:
	bool onPersistentChannelChanged(const LLSD& payload);
	bool mLoaded;
};

#endif // LL_LLPERSISTENTNOTIFICATIONSTORAGE_H

