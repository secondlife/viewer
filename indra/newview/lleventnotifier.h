/** 
 * @file lleventnotifier.h
 * @brief Viewer code for managing event notifications
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLEVENTNOTIFIER_H
#define LL_LLEVENTNOTIFIER_H

#include "llframetimer.h"
#include "lluserauth.h"
#include "v3dmath.h"

class LLEventInfo;
class LLEventNotification;


class LLEventNotifier
{
public:
	LLEventNotifier();
	virtual ~LLEventNotifier();

	void update();	// Notify the user of the event if it's coming up

	void load(const LLUserAuth::options_t& event_options);	// In the format that it comes in from LLUserAuth
	void add(LLEventInfo &event_info);	// Add a new notification for an event
	void remove(U32 event_id);

	BOOL hasNotification(const U32 event_id);

	typedef std::map<U32, LLEventNotification *> en_map;

	static void notifyCallback(S32 option, void *user_data);
protected:
	en_map	mEventNotifications;
	LLFrameTimer	mNotificationTimer;
};


class LLEventNotification
{
public:
	LLEventNotification();
	virtual ~LLEventNotification();

	BOOL load(const LLUserAuth::response_t &en);		// In the format it comes in from LLUserAuth
	BOOL load(const LLEventInfo &event_info);		// From existing event_info on the viewer.
	//void setEventID(const U32 event_id);
	//void setEventName(std::string &event_name);
	U32					getEventID() const				{ return mEventID; }
	const std::string	&getEventName() const			{ return mEventName; }
	U32					getEventDate() const			{ return mEventDate; }
	const std::string	&getEventDateStr() const		{ return mEventDateStr; }
	LLVector3d			getEventPosGlobal() const		{ return mEventPosGlobal; }
protected:
	U32			mEventID;			// EventID for this event
	std::string	mEventName;
	std::string mEventDateStr;
	U32			mEventDate;
	LLVector3d	mEventPosGlobal;
};

extern LLEventNotifier gEventNotifier;

#endif //LL_LLEVENTNOTIFIER_H
