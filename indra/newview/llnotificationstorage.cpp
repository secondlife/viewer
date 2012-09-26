/**
* @file llnotificationstorage.cpp
* @brief LLPersistentNotificationStorage class implementation
*
* $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#include "llnotificationstorage.h"

#include "llxmlnode.h" // for linux compilers

#include "llchannelmanager.h"
#include "llscreenchannel.h"
#include "llscriptfloater.h"
#include "llsdserialize.h"
#include "llviewermessage.h"

//////////////////////////////////////////////////////////////////////////

class LLResponderRegistry
{
public:

	static void registerResponders();

	static LLNotificationResponderInterface* createResponder(const std::string& notification_name, const LLSD& params);

private:

	template<typename RESPONDER_TYPE>
	static LLNotificationResponderInterface* create(const LLSD& params)
	{
		RESPONDER_TYPE* responder = new RESPONDER_TYPE();
		responder->fromLLSD(params);
		return responder;
	}

	typedef boost::function<LLNotificationResponderInterface* (const LLSD& params)> responder_constructor_t;

	static void add(const std::string& notification_name, const responder_constructor_t& ctr);

private:

	typedef std::map<std::string, responder_constructor_t> build_map_t;

	static build_map_t sBuildMap;
};

//////////////////////////////////////////////////////////////////////////

LLPersistentNotificationStorage::LLPersistentNotificationStorage()
{
	mFileName = gDirUtilp->getExpandedFilename ( LL_PATH_PER_SL_ACCOUNT, "open_notifications.xml" );
}

bool LLPersistentNotificationStorage::onPersistentChannelChanged(const LLSD& payload)
{
	// we ignore "load" messages, but rewrite the persistence file on any other
	const std::string sigtype = payload["sigtype"].asString();
	if ("load" != sigtype)
	{
		saveNotifications();
	}
	return false;
}

// Storing or loading too many persistent notifications will severely hurt 
// viewer load times, possibly to the point of failing to log in. Example case
// from MAINT-994 is 821 notifications. 
static const S32 MAX_PERSISTENT_NOTIFICATIONS = 250;

void LLPersistentNotificationStorage::saveNotifications()
{
	// TODO - think about save optimization.

	llofstream notify_file(mFileName.c_str());
	if (!notify_file.is_open())
	{
		llwarns << "Failed to open " << mFileName << llendl;
		return;
	}

	LLSD output;
	LLSD& data = output["data"];

	LLNotificationChannelPtr history_channel = LLNotifications::instance().getChannel("Persistent");
	LLNotificationSet::iterator it = history_channel->begin();

	for ( ; history_channel->end() != it; ++it)
	{
		LLNotificationPtr notification = *it;

		// After a notification was placed in Persist channel, it can become
		// responded, expired or canceled - in this case we are should not save it
		if(notification->isRespondedTo() || notification->isCancelled()
			|| notification->isExpired())
		{
			continue;
		}

		data.append(notification->asLLSD());

		if (data.size() >= MAX_PERSISTENT_NOTIFICATIONS)
		{
			llwarns << "Too many persistent notifications."
					<< " Saved " << MAX_PERSISTENT_NOTIFICATIONS << " of " << history_channel->size() << " persistent notifications." << llendl;
			break;
		}
	}

	LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
	formatter->format(output, notify_file, LLSDFormatter::OPTIONS_PRETTY);
}

void LLPersistentNotificationStorage::loadNotifications()
{
	LLResponderRegistry::registerResponders();

	llifstream notify_file(mFileName.c_str());
	if (!notify_file.is_open())
	{
		llwarns << "Failed to open " << mFileName << llendl;
		return;
	}

	LLSD input;
	LLPointer<LLSDParser> parser = new LLSDXMLParser();
	if (parser->parse(notify_file, input, LLSDSerialize::SIZE_UNLIMITED) < 0)
	{
		llwarns << "Failed to parse open notifications" << llendl;
		return;
	}

	if (input.isUndefined())
	{
		return;
	}

	LLSD& data = input["data"];
	if (data.isUndefined())
	{
		return;
	}

	using namespace LLNotificationsUI;
	LLScreenChannel* notification_channel = dynamic_cast<LLScreenChannel*>(LLChannelManager::getInstance()->
		findChannelByID(LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));

	LLNotifications& instance = LLNotifications::instance();
	S32 processed_notifications = 0;
	for (LLSD::array_const_iterator notification_it = data.beginArray();
		notification_it != data.endArray();
		++notification_it)
	{
		LLSD notification_params = *notification_it;

		if (instance.templateExists(notification_params["name"].asString()))
		{
			LLNotificationPtr notification(new LLNotification(notification_params));

			LLNotificationResponderPtr responder(LLResponderRegistry::
				createResponder(notification_params["name"], notification_params["responder"]));
			notification->setResponseFunctor(responder);

			instance.add(notification);

			// hide script floaters so they don't confuse the user and don't overlap startup toast
			LLScriptFloaterManager::getInstance()->setFloaterVisible(notification->getID(), false);

			if(notification_channel)
			{
				// hide saved toasts so they don't confuse the user
				notification_channel->hideToast(notification->getID());
			}
		}
		else
		{
			llwarns << "Failed to find template for persistent notification " << notification_params["name"].asString() << llendl;
		}

		++processed_notifications;
		if (processed_notifications >= MAX_PERSISTENT_NOTIFICATIONS)
		{
			llwarns << "Too many persistent notifications."
					<< " Processed " << MAX_PERSISTENT_NOTIFICATIONS << " of " << data.size() << " persistent notifications." << llendl;
			break;
		}
	}

	LLNotifications::instance().getChannel("Persistent")->
		connectChanged(boost::bind(&LLPersistentNotificationStorage::onPersistentChannelChanged, this, _1));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLResponderRegistry::build_map_t LLResponderRegistry::sBuildMap;

void LLResponderRegistry::registerResponders()
{
	sBuildMap.clear();

	add("ObjectGiveItem", &create<LLOfferInfo>);
	add("UserGiveItem", &create<LLOfferInfo>);
}

LLNotificationResponderInterface* LLResponderRegistry::createResponder(const std::string& notification_name, const LLSD& params)
{
	build_map_t::const_iterator it = sBuildMap.find(notification_name);
	if(sBuildMap.end() == it)
	{
		return NULL;
	}
	responder_constructor_t ctr = it->second;
	return ctr(params);
}

void LLResponderRegistry::add(const std::string& notification_name, const responder_constructor_t& ctr)
{
	if(sBuildMap.find(notification_name) != sBuildMap.end())
	{
		llwarns << "Responder is already registered : " << notification_name << llendl;
		llassert(!"Responder already registered");
	}
	sBuildMap[notification_name] = ctr;
}

// EOF
