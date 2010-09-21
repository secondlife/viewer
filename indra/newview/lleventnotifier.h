/** 
 * @file lleventnotifier.h
 * @brief Viewer code for managing event notifications
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLEVENTNOTIFIER_H
#define LL_LLEVENTNOTIFIER_H

#include "llframetimer.h"
#include "v3dmath.h"

class LLEventInfo;
class LLEventNotification;


class LLEventNotifier
{
public:
	LLEventNotifier();
	virtual ~LLEventNotifier();

	void update();	// Notify the user of the event if it's coming up

	void load(const LLSD& event_options);	// In the format that it comes in from login
	void add(LLEventInfo &event_info);	// Add a new notification for an event
	void remove(U32 event_id);

	BOOL hasNotification(const U32 event_id);

	typedef std::map<U32, LLEventNotification *> en_map;

protected:
	en_map	mEventNotifications;
	LLFrameTimer	mNotificationTimer;
};


class LLEventNotification
{
public:
	LLEventNotification();
	virtual ~LLEventNotification();

	BOOL load(const LLSD& en);		// In the format it comes in from login
	BOOL load(const LLEventInfo &event_info);		// From existing event_info on the viewer.
	//void setEventID(const U32 event_id);
	//void setEventName(std::string &event_name);
	U32					getEventID() const				{ return mEventID; }
	const std::string	&getEventName() const			{ return mEventName; }
	time_t				getEventDate() const			{ return mEventDate; }
	const std::string	&getEventDateStr() const		{ return mEventDateStr; }
	LLVector3d			getEventPosGlobal() const		{ return mEventPosGlobal; }
	bool				handleResponse(const LLSD& notification, const LLSD& payload);
protected:
	U32			mEventID;			// EventID for this event
	std::string	mEventName;
	std::string mEventDateStr;
	time_t		mEventDate;
	LLVector3d	mEventPosGlobal;
};

extern LLEventNotifier gEventNotifier;

#endif //LL_LLEVENTNOTIFIER_H
