/**
* @file llnotificationstorage.cpp
* @brief LLPersistentNotificationStorage class implementation
*
* $LicenseInfo:firstyear=2010&license=viewergpl$
*
* Copyright (c) 2010, Linden Research, Inc.
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
		// responded, expired - in this case we are should not save it
		if(notification->isRespondedTo()
			|| notification->isExpired())
		{
			continue;
		}

		data.append(notification->asLLSD());
	}

	LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
	formatter->format(output, notify_file, LLSDFormatter::OPTIONS_PRETTY);
}

void LLPersistentNotificationStorage::loadNotifications()
{
	LLResponderRegistry::registerResponders();

	LLNotifications::instance().getChannel("Persistent")->
		connectChanged(boost::bind(&LLPersistentNotificationStorage::onPersistentChannelChanged, this, _1));

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

	for (LLSD::array_const_iterator notification_it = data.beginArray();
		notification_it != data.endArray();
		++notification_it)
	{
		LLSD notification_params = *notification_it;
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
		llwarns << "Responder for notification \'" << notification_name << "\' is not registered" << llendl;
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
