/** 
 * @file llpanelexperiencelog.cpp
 * @brief llpanelexperiencelog
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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
#include "llpanelexperiencelog.h"
#include "lldispatcher.h"
#include "llsdserialize.h"
#include "llviewergenericmessage.h"
#include "llnotificationsutil.h"
#include "lltrans.h"


class LLExperienceLogDispatchHandler : public LLDispatchHandler
{
public:
	virtual bool operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const LLUUID& invoice,
		const sparam_t& strings)
	{
		LLSD message;

		sparam_t::const_iterator it = strings.begin();
		if(it != strings.end()){
			const std::string& llsdRaw = *it++;
			std::istringstream llsdData(llsdRaw);
			if (!LLSDSerialize::deserialize(message, llsdData, llsdRaw.length()))
			{
				llwarns << "LLExperienceLogDispatchHandler: Attempted to read parameter data into LLSD but failed:" << llsdRaw << llendl;
			}
		}
		message["public_id"] = invoice;

		// Object Name
		if(it != strings.end())
		{
			message["ObjectName"] = *it++;
		}

		// parcel Name
		if(it != strings.end())
		{
			message["ParcelName"] = *it++;
		}

		LLExperienceLog::instance().handleExperienceMessage(message);
		return true;
	}
};

static LLExperienceLogDispatchHandler experience_log_dispatch_handler;

void LLExperienceLog::handleExperienceMessage(LLSD& message)
{
	std::ostringstream str;
	if(message.has("Permission"))
	{
		str << "ExperiencePermission" << message["Permission"].asInteger();
		std::string entry;
		if(LLTrans::findString(entry, str.str()))
		{
			str.str(entry);
		}
		else
		{
			str.str();
		}
	}

	if(str.str().empty())
	{
		str.str(LLTrans::getString("ExperiencePermissionUnknown", message));
	}

	message["EventType"] = str.str();
	if(message.has("IsAttachment") && message["IsAttachment"].asBoolean())
	{
		LLNotificationsUtil::add("ExperienceEventAttachment", message);
	}
	else
	{
		LLNotificationsUtil::add("ExperienceEvent", message);
	}
}

LLExperienceLog::LLExperienceLog()
{
}

void LLExperienceLog::initialize()
{
	gGenericDispatcher.addHandler("ExperienceEvent", &experience_log_dispatch_handler);
}
