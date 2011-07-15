/** 
 * @file llnearbychathandler.h
 * @brief nearby chat notify
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLNEARBYCHATHANDLER_H
#define LL_LLNEARBYCHATHANDLER_H

#include "llnotificationhandler.h"

class LLEventPump;

//add LLNearbyChatHandler to LLNotificationsUI namespace
namespace LLNotificationsUI{

class LLNearbyChatHandler : public LLChatHandler
{
public:
	LLNearbyChatHandler(e_notification_type type,const LLSD& id);
	virtual ~LLNearbyChatHandler();


	virtual void processChat(const LLChat& chat_msg, const LLSD &args);

protected:
	virtual void onDeleteToast(LLToast* toast);
	virtual void initChannel();

	static boost::scoped_ptr<LLEventPump> sChatWatcher;
};

}

#endif /* LL_LLNEARBYCHATHANDLER_H */
