/** 
 * @file llnotificationmanager.cpp
 * @brief Class implements a brige between the old and a new notification sistems
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


#include "llnotificationmanager.h"

#include "llnearbychathandler.h"
#include "llnotifications.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLNotificationManager::LLNotificationManager()
{
	init();
}

//--------------------------------------------------------------------------
LLNotificationManager::~LLNotificationManager()
{
}

//--------------------------------------------------------------------------
void LLNotificationManager::init()
{
	mChannels.push_back(new LLScriptHandler());
	mChannels.push_back(new LLTipHandler());
	mChannels.push_back(new LLGroupHandler());
	mChannels.push_back(new LLAlertHandler("Alerts", "alert", false));
	mChannels.push_back(new LLAlertHandler("AlertModal", "alertmodal", true));
	mChannels.push_back(new LLOfferHandler());
	mChannels.push_back(new LLHintHandler());
	mChannels.push_back(new LLBrowserNotification());
	mChannels.push_back(new LLOutboxNotification());
	mChannels.push_back(new LLIMHandler());
  
	mChatHandler = boost::shared_ptr<LLNearbyChatHandler>(new LLNearbyChatHandler());
}

//--------------------------------------------------------------------------
void LLNotificationManager::onChat(const LLChat& msg, const LLSD &args)
{
	if(mChatHandler)
		mChatHandler->processChat(msg, args);
}

