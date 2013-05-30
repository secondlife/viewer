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

#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llfloaterevent.h"
#include "llagent.h"
#include "llcommandhandler.h"	// secondlife:///app/... support

class LLEventHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLEventHandler() : LLCommandHandler("event", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& params, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		if (params.size() < 2)
		{
			return false;
		}
		std::string event_command = params[1].asString();
		S32 event_id = params[0].asInteger();
		if(event_command == "details")
		{
			LLFloaterEvent* floater = LLFloaterReg::getTypedInstance<LLFloaterEvent>("event");
			if (floater)
			{
				floater->setEventID(event_id);
				LLFloaterReg::showTypedInstance<LLFloaterEvent>("event");
				return true;
			}
		}
		else if(event_command == "notify")
		{
			// we're adding or removing a notification, so grab the date, name and notification bool
			if (params.size() < 3)
			{
				return false;
			}			
			if(params[2].asString() == "enable")
			{
				gEventNotifier.add(event_id);
				// tell the server to modify the database as this was a slurl event notification command
				gEventNotifier.serverPushRequest(event_id, true);
			
			}
			else
			{
				gEventNotifier.remove(event_id);
			}
			return true;
		}

		
		return false;
	}
};
LLEventHandler gEventHandler;


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

		F64 alert_time = LLDate::now().secondsSinceEpoch() + 5 * 60;
		en_map::iterator iter;
		for (iter = mEventNotifications.begin();
			 iter != mEventNotifications.end();)
		{
			LLEventNotification *np = iter->second;

			iter++;
			if (np->getEventDateEpoch() < alert_time)
			{
				LLSD args;
				args["NAME"] = np->getEventName();
				
				args["DATE"] = np->getEventDateStr();
				LLNotificationsUtil::add("EventNotification", args, LLSD(),
					boost::bind(&LLEventNotifier::handleResponse, this, np->getEventID(), _1, _2));
				remove(np->getEventID());
				
			}
		}
		mNotificationTimer.reset();
	}
}



bool LLEventNotifier::handleResponse(U32 eventId, const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch (option)
	{
		case 0:
		{
			LLFloaterEvent* floater = LLFloaterReg::getTypedInstance<LLFloaterEvent>("event");
			if (floater)
			{
				floater->setEventID(eventId);
				LLFloaterReg::showTypedInstance<LLFloaterEvent>("event");
			}
			break;
		}
		case 1:
			break;
	}
	return true;
}

bool LLEventNotifier::add(U32 eventId, F64 eventEpoch, const std::string& eventDateStr, const std::string &eventName)
{
	LLEventNotification *new_enp = new LLEventNotification(eventId, eventEpoch, eventDateStr, eventName);
	
	llinfos << "Add event " << eventName << " id " << eventId << " date " << eventDateStr << llendl;
	if(!new_enp->isValid())
	{
		delete new_enp;
		return false;
	}
	
	mEventNotifications[new_enp->getEventID()] = new_enp;
	return true;
	
}

void LLEventNotifier::add(U32 eventId)
{
	
	gMessageSystem->newMessageFast(_PREHASH_EventInfoRequest);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	gMessageSystem->nextBlockFast(_PREHASH_EventData);
	gMessageSystem->addU32Fast(_PREHASH_EventID, eventId);
	gAgent.sendReliableMessage();

}

//static 
void LLEventNotifier::processEventInfoReply(LLMessageSystem *msg, void **)
{
	// extract the agent id
	LLUUID agent_id;
	U32 event_id;
	std::string event_name;
	std::string eventd_date;
	U32 event_time_utc;
	
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	msg->getU32("EventData", "EventID", event_id);
	msg->getString("EventData", "Name", event_name);
	msg->getString("EventData", "Date", eventd_date);
	msg->getU32("EventData", "DateUTC", event_time_utc);
	
	gEventNotifier.add(event_id, (F64)event_time_utc, eventd_date, event_name);
}	
	
	
void LLEventNotifier::load(const LLSD& event_options)
{
	for(LLSD::array_const_iterator resp_it = event_options.beginArray(),
		end = event_options.endArray(); resp_it != end; ++resp_it)
	{
		LLSD response = *resp_it;

		add(response["event_id"].asInteger(), response["event_date_ut"], response["event_date"].asString(), response["event_name"].asString());
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

void LLEventNotifier::remove(const U32 event_id)
{
	en_map::iterator iter;
	iter = mEventNotifications.find(event_id);
	if (iter == mEventNotifications.end())
	{
		// We don't have a notification for this event, don't bother.
		return;
	}

	serverPushRequest(event_id, false);
	delete iter->second;
	mEventNotifications.erase(iter);
}


void LLEventNotifier::serverPushRequest(U32 event_id, bool add)
{
	// Push up a message to tell the server we have this notification.
	gMessageSystem->newMessage(add?"EventNotificationAddRequest":"EventNotificationRemoveRequest");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlock("EventData");
	gMessageSystem->addU32("EventID", event_id);
	gAgent.sendReliableMessage();
}


LLEventNotification::LLEventNotification(U32 eventId, F64 eventEpoch, const std::string& eventDateStr, const std::string &eventName) :
	mEventID(eventId),
	mEventName(eventName),
	mEventDateEpoch(eventEpoch),
    mEventDateStr(eventDateStr)
{
	
}






