// Notification Manager Class
/** 
 * @file llnotificationmanager.h
 * @brief Class implements a brige between the old and a new notification sistems
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
	std::map<std::string, LLChatHandler*> mChatHandlers;
};

}
#endif

