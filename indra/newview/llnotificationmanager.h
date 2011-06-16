// Notification Manager Class
/** 
 * @file llnotificationmanager.h
 * @brief Class implements a brige between the old and a new notification sistems
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLNOTIFICATIONMANAGER_H
#define LL_LLNOTIFICATIONMANAGER_H

#include "lluictrl.h"
#include "llnotificationhandler.h"


#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

namespace LLNotificationsUI {

class LLToast;

/**
 * Responsible for registering notification handlers.
 */
class LLNotificationManager : public LLSingleton<LLNotificationManager>
{
	typedef std::pair<std::string, LLEventHandler*> eventhandlers;
public:	
	LLNotificationManager();	
	virtual ~LLNotificationManager();

	//TODO: make private
	// this method initialize handlers' map for different types of notifications
	void init(void);
	//TODO: combine processing and storage (*)
	
	// this method reacts on system notifications and calls an appropriate handler
	bool onNotification(const LLSD& notification);

	// this method reacts on chat notifications and calls an appropriate handler
	void onChat(const LLChat& msg, const LLSD &args);

	// get a handler for a certain type of notification
	LLEventHandler* getHandlerForNotification(std::string notification_type);


private:
	//TODO (*)
	std::map<std::string, boost::shared_ptr<LLEventHandler> > mNotifyHandlers;
	// cruft std::map<std::string, LLChatHandler*> mChatHandlers;
};

}
#endif

