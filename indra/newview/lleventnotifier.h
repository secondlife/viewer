/** 
 * @file lleventnotifier.h
 * @brief Viewer code for managing event notifications
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

#ifndef LL_LLEVENTNOTIFIER_H
#define LL_LLEVENTNOTIFIER_H

#include "llframetimer.h"
#include "v3dmath.h"

class LLEventNotification;
class LLMessageSystem;


class LLEventNotifier
{
public:
	LLEventNotifier();
	virtual ~LLEventNotifier();

	void update();	// Notify the user of the event if it's coming up
	bool add(U32 eventId, F64 eventEpoch, const std::string& eventDateStr, const std::string &eventName);
	void add(U32 eventId);

	
	void load(const LLSD& event_options);	// In the format that it comes in from login
	void remove(U32 event_id);

	BOOL hasNotification(const U32 event_id);
	void serverPushRequest(U32 event_id, bool add);

	typedef std::map<U32, LLEventNotification *> en_map;
	bool  handleResponse(U32 eventId, const LLSD& notification, const LLSD& response);		

	static void processEventInfoReply(LLMessageSystem *msg, void **);	
	
protected:
	en_map	mEventNotifications;
	LLFrameTimer	mNotificationTimer;
};


class LLEventNotification
{
public:
	LLEventNotification(U32 eventId, F64 eventEpoch, const std::string& eventDateStr, const std::string &eventName);


	U32					getEventID() const				{ return mEventID; }
	const std::string	&getEventName() const			{ return mEventName; }
	bool                isValid() const                 { return mEventID > 0 && mEventDateEpoch != 0 && mEventName.size() > 0; }
	const F64		    &getEventDateEpoch() const		{ return mEventDateEpoch; }
	const std::string   &getEventDateStr() const        { return mEventDateStr; }
	
	
protected:
	U32			mEventID;			// EventID for this event
	std::string	mEventName;
	F64		    mEventDateEpoch;
	std::string mEventDateStr;
};

extern LLEventNotifier gEventNotifier;

#endif //LL_LLEVENTNOTIFIER_H
