/** 
 * @file lleventnotifier.cpp
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

#include "llviewerprecompiledheaders.h"

#include "lleventnotifier.h"

#include "llnotificationsutil.h"
#include "message.h"

#include "lleventinfo.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llagent.h"

LLEventNotifier gEventNotifier;

LLEventNotifier::LLEventNotifier()
{
}


LLEventNotifier::~LLEventNotifier()
{
	en_map::iterator iter;

	for (iter = mEventNotifications.begin();
		 iter != mEventNotifications.end();
		 iter++)
	{
		delete iter->second;
	}
}


void LLEventNotifier::update()
{
	if (mNotificationTimer.getElapsedTimeF32() > 30.f)
	{
		// Check our notifications again and send out updates
		// if they happen.

		time_t alert_time = time_corrected() + 5 * 60;
		en_map::iterator iter;
		for (iter = mEventNotifications.begin();
			 iter != mEventNotifications.end();)
		{
			LLEventNotification *np = iter->second;

			if (np->getEventDate() < (alert_time))
			{
				LLSD args;
				args["NAME"] = np->getEventName();
				args["DATE"] = np->getEventDateStr();
				LLNotificationsUtil::add("EventNotification", args, LLSD(),
					boost::bind(&LLEventNotification::handleResponse, np, _1, _2));
				mEventNotifications.erase(iter++);
			}
			else
			{
				iter++;
			}
		}
		mNotificationTimer.reset();
	}
}

void LLEventNotifier::load(const LLSD& event_options)
{
	for(LLSD::array_const_iterator resp_it = event_options.beginArray(),
		end = event_options.endArray(); resp_it != end; ++resp_it)
	{
		LLSD response = *resp_it;

		LLEventNotification *new_enp = new LLEventNotification();

		if(!new_enp->load(response))
		{
			delete new_enp;
			continue;
		}
		
		mEventNotifications[new_enp->getEventID()] = new_enp;
	}
}


BOOL LLEventNotifier::hasNotification(const U32 event_id)
{
	if (mEventNotifications.find(event_id) != mEventNotifications.end())
	{
		return TRUE;
	}
	return FALSE;
}


void LLEventNotifier::add(LLEventInfo &event_info)
{
	// We need to tell the simulator that we want to pay attention to
	// this event, as well as add it to our list.

	if (mEventNotifications.find(event_info.mID) != mEventNotifications.end())
	{
		// We already have a notification for this event, don't bother.
		return;
	}

	// Push up a message to tell the server we have this notification.
	gMessageSystem->newMessage("EventNotificationAddRequest");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlock("EventData");
	gMessageSystem->addU32("EventID", event_info.mID);
	gAgent.sendReliableMessage();

	LLEventNotification *enp = new LLEventNotification;
	enp->load(event_info);
	mEventNotifications[event_info.mID] = enp;
}

void LLEventNotifier::remove(const U32 event_id)
{
	en_map::iterator iter;
	iter = mEventNotifications.find(event_id);
	if (iter == mEventNotifications.end())
	{
		// We don't have a notification for this event, don't bother.
		return;
	}

	// Push up a message to tell the server to remove this notification.
	gMessageSystem->newMessage("EventNotificationRemoveRequest");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlock("EventData");
	gMessageSystem->addU32("EventID", event_id);
	gAgent.sendReliableMessage();
	
	delete iter->second;
	mEventNotifications.erase(iter);
}

LLEventNotification::LLEventNotification() :
	mEventID(0),
	mEventDate(0),
	mEventName("")
{
}


LLEventNotification::~LLEventNotification()
{
}

bool LLEventNotification::handleResponse(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch (option)
	{
	case 0:
		{
			gAgent.teleportViaLocation(getEventPosGlobal());
			LLFloaterWorldMap* floater_world_map = LLFloaterWorldMap::getInstance();
			if(floater_world_map) floater_world_map->trackLocation(getEventPosGlobal());
			break;
		}
	case 1:
		LLFloaterReg::showInstance("search", LLSD().with("category", "events").with("id", S32(getEventID())));
		break;
	case 2:
		break;
	}

	// We could clean up the notification on the server now if we really wanted to.
	return false;
}

BOOL LLEventNotification::load(const LLSD& response)
{
	BOOL event_ok = TRUE;
	LLSD option = response.get("event_id");
	if (option.isDefined())
	{
		mEventID = option.asInteger();
	}
	else
	{
		event_ok = FALSE;
	}

	option = response.get("event_name");
	if (option.isDefined())
	{
		llinfos << "Event: " << option.asString() << llendl;
		mEventName = option.asString();
	}
	else
	{
		event_ok = FALSE;
	}

	option = response.get("event_date");
	if (option.isDefined())
	{
		llinfos << "EventDate: " << option.asString() << llendl;
		mEventDateStr = option.asString();
	}
	else
	{
		event_ok = FALSE;
	}

	option = response.get("event_date_ut");
	if (option.isDefined())
	{
		llinfos << "EventDate: " << option.asString() << llendl;
		mEventDate = strtoul(option.asString().c_str(), NULL, 10);
	}
	else
	{
		event_ok = FALSE;
	}

	S32 grid_x = 0;
	S32 grid_y = 0;
	S32 x_region = 0;
	S32 y_region = 0;

	option = response.get("grid_x");
	if (option.isDefined())
	{
		llinfos << "GridX: " << option.asInteger() << llendl;
		grid_x= option.asInteger();
	}
	else
	{
		event_ok = FALSE;
	}

	option = response.get("grid_y");
	if (option.isDefined())
	{
		llinfos << "GridY: " << option.asInteger() << llendl;
		grid_y = option.asInteger();
	}
	else
	{
		event_ok = FALSE;
	}

	option = response.get("x_region");
	if (option.isDefined())
	{
		llinfos << "RegionX: " << option.asInteger() << llendl;
		x_region = option.asInteger();
	}
	else
	{
		event_ok = FALSE;
	}

	option = response.get("y_region");
	if (option.isDefined())
	{
		llinfos << "RegionY: " << option.asInteger() << llendl;
		y_region = option.asInteger();
	}
	else
	{
		event_ok = FALSE;
	}

	mEventPosGlobal.mdV[VX] = grid_x * 256 + x_region;
	mEventPosGlobal.mdV[VY] = grid_y * 256 + y_region;
	mEventPosGlobal.mdV[VZ] = 0.f;

	return event_ok;
}

BOOL LLEventNotification::load(const LLEventInfo &event_info)
{

	mEventID = event_info.mID;
	mEventName = event_info.mName;
	mEventDateStr = event_info.mTimeStr;
	mEventDate = event_info.mUnixTime;
	mEventPosGlobal = event_info.mPosGlobal;
	return TRUE;
}

