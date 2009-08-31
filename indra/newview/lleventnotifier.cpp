/** 
 * @file lleventnotifier.cpp
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

#include "llviewerprecompiledheaders.h"

#include "lleventnotifier.h"

#include "message.h"

#include "llnotify.h"
#include "lleventinfo.h"
#include "llfloaterreg.h"
#include "llfloaterdirectory.h"
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
				LLNotifications::instance().add("EventNotification", args, LLSD(),
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

void LLEventNotifier::load(const LLUserAuth::options_t& event_options)
{
	LLUserAuth::options_t::const_iterator resp_it;
	for (resp_it = event_options.begin(); 
		 resp_it != event_options.end(); 
		 ++resp_it)
	{
		const LLUserAuth::response_t& response = *resp_it;

		LLEventNotification *new_enp = new LLEventNotification();

		if (!new_enp->load(response))
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
	mEventName("")
{
}


LLEventNotification::~LLEventNotification()
{
}

bool LLEventNotification::handleResponse(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
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
		gDisplayEventHack = TRUE;
		LLFloaterReg::showInstance("search", LLSD().insert("panel", "event").insert("id", S32(getEventID())));
		break;
	case 2:
		break;
	}

	// We could clean up the notification on the server now if we really wanted to.
	return false;
}

BOOL LLEventNotification::load(const LLUserAuth::response_t &response)
{

	LLUserAuth::response_t::const_iterator option_it;
	BOOL event_ok = TRUE;
	option_it = response.find("event_id");
	if (option_it != response.end())
	{
		mEventID = atoi(option_it->second.c_str());
	}
	else
	{
		event_ok = FALSE;
	}

	option_it = response.find("event_name");
	if (option_it != response.end())
	{
		llinfos << "Event: " << option_it->second << llendl;
		mEventName = option_it->second;
	}
	else
	{
		event_ok = FALSE;
	}


	option_it = response.find("event_date");
	if (option_it != response.end())
	{
		llinfos << "EventDate: " << option_it->second << llendl;
		mEventDateStr = option_it->second;
	}
	else
	{
		event_ok = FALSE;
	}

	option_it = response.find("event_date_ut");
	if (option_it != response.end())
	{
		llinfos << "EventDate: " << option_it->second << llendl;
		mEventDate = strtoul(option_it->second.c_str(), NULL, 10);
	}
	else
	{
		event_ok = FALSE;
	}

	S32 grid_x = 0;
	S32 grid_y = 0;
	S32 x_region = 0;
	S32 y_region = 0;

	option_it = response.find("grid_x");
	if (option_it != response.end())
	{
		llinfos << "GridX: " << option_it->second << llendl;
		grid_x= atoi(option_it->second.c_str());
	}
	else
	{
		event_ok = FALSE;
	}

	option_it = response.find("grid_y");
	if (option_it != response.end())
	{
		llinfos << "GridY: " << option_it->second << llendl;
		grid_y = atoi(option_it->second.c_str());
	}
	else
	{
		event_ok = FALSE;
	}

	option_it = response.find("x_region");
	if (option_it != response.end())
	{
		llinfos << "RegionX: " << option_it->second << llendl;
		x_region = atoi(option_it->second.c_str());
	}
	else
	{
		event_ok = FALSE;
	}

	option_it = response.find("y_region");
	if (option_it != response.end())
	{
		llinfos << "RegionY: " << option_it->second << llendl;
		y_region = atoi(option_it->second.c_str());
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

