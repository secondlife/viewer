/** 
 * @file llviewerdisplayname.cpp
 * @brief Wrapper for display name functionality
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

#include "llviewerprecompiledheaders.h"

#include "llviewerdisplayname.h"

// viewer includes
#include "llagent.h"
#include "llviewerregion.h"
#include "llvoavatar.h"

// library includes
#include "llavatarnamecache.h"
#include "llhttpclient.h"
#include "llhttpnode.h"
#include "llnotificationsutil.h"
#include "llui.h"					// getLanguage()

namespace LLViewerDisplayName
{
	// Fired when viewer receives server response to display name change
	set_name_signal_t sSetDisplayNameSignal;

	// Fired when there is a change in the agent's name
	name_changed_signal_t sNameChangedSignal;

	void addNameChangedCallback(const name_changed_signal_t::slot_type& cb) 
	{ 
		sNameChangedSignal.connect(cb); 
	}

}

class LLSetDisplayNameResponder : public LLHTTPClient::Responder
{
public:
	// only care about errors
	/*virtual*/ void error(U32 status, const std::string& reason)
	{
		LLViewerDisplayName::sSetDisplayNameSignal(false, "", LLSD());
		LLViewerDisplayName::sSetDisplayNameSignal.disconnect_all_slots();
	}
};

void LLViewerDisplayName::set(const std::string& display_name, const set_name_slot_t& slot)
{
	// TODO: simple validation here

	LLViewerRegion* region = gAgent.getRegion();
	llassert(region);
	std::string cap_url = region->getCapability("SetDisplayName");
	if (cap_url.empty())
	{
		// this server does not support display names, report error
		slot(false, "unsupported", LLSD());
		return;
	}

	// People API can return localized error messages.  Indicate our
	// language preference via header.
	LLSD headers;
	headers["Accept-Language"] = LLUI::getLanguage();

	// People API requires both the old and new value to change a variable.
	// Our display name will be in cache before the viewer's UI is available
	// to request a change, so we can use direct lookup without callback.
	LLAvatarName av_name;
	if (!LLAvatarNameCache::get( gAgent.getID(), &av_name))
	{
		slot(false, "name unavailable", LLSD());
		return;
	}

	// People API expects array of [ "old value", "new value" ]
	LLSD change_array = LLSD::emptyArray();
	change_array.append(av_name.mDisplayName);
	change_array.append(display_name);
	
	llinfos << "Set name POST to " << cap_url << llendl;

	// Record our caller for when the server sends back a reply
	sSetDisplayNameSignal.connect(slot);
	
	// POST the requested change.  The sim will not send a response back to
	// this request directly, rather it will send a separate message after it
	// communicates with the back-end.
	LLSD body;
	body["display_name"] = change_array;
	LLHTTPClient::post(cap_url, body, new LLSetDisplayNameResponder, headers);
}

class LLSetDisplayNameReply : public LLHTTPNode
{
	LOG_CLASS(LLSetDisplayNameReply);
public:
	/*virtual*/ void post(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{
		LLSD body = input["body"];

		S32 status = body["status"].asInteger();
		bool success = (status == 200);
		std::string reason = body["reason"].asString();
		LLSD content = body["content"];

		llinfos << "status " << status << " reason " << reason << llendl;

		// If viewer's concept of display name is out-of-date, the set request
		// will fail with 409 Conflict.  If that happens, fetch up-to-date
		// name information.
		if (status == 409)
		{
			LLUUID agent_id = gAgent.getID();
			// Flush stale data
			LLAvatarNameCache::erase( agent_id );
			// Queue request for new data
			LLAvatarName ignored;
			LLAvatarNameCache::get( agent_id, &ignored );
			// Kill name tag, as it is wrong
			LLVOAvatar::invalidateNameTag( agent_id );
		}

		// inform caller of result
		LLViewerDisplayName::sSetDisplayNameSignal(success, reason, content);
		LLViewerDisplayName::sSetDisplayNameSignal.disconnect_all_slots();
	}
};


class LLDisplayNameUpdate : public LLHTTPNode
{
	/*virtual*/ void post(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{
		LLSD body = input["body"];
		LLUUID agent_id = body["agent_id"];
		std::string old_display_name = body["old_display_name"];
		// By convention this record is called "agent" in the People API
		LLSD name_data = body["agent"];

		// Inject the new name data into cache
		LLAvatarName av_name;
		av_name.fromLLSD( name_data );

		llinfos << "name-update now " << LLDate::now()
			<< " next_update " << LLDate(av_name.mNextUpdate)
			<< llendl;

		// Name expiration time may be provided in headers, or we may use a
		// default value
		// *TODO: get actual headers out of ResponsePtr
		//LLSD headers = response->mHeaders;
		LLSD headers;
		av_name.mExpires = 
			LLAvatarNameCache::nameExpirationFromHeaders(headers);

		LLAvatarNameCache::insert(agent_id, av_name);

		// force name tag to update
		LLVOAvatar::invalidateNameTag(agent_id);

		LLSD args;
		args["OLD_NAME"] = old_display_name;
		args["SLID"] = av_name.mUsername;
		args["NEW_NAME"] = av_name.mDisplayName;
		LLNotificationsUtil::add("DisplayNameUpdate", args);
		if (agent_id == gAgent.getID())
		{
			LLViewerDisplayName::sNameChangedSignal();
		}
	}
};

LLHTTPRegistration<LLSetDisplayNameReply>
    gHTTPRegistrationMessageSetDisplayNameReply(
		"/message/SetDisplayNameReply");

LLHTTPRegistration<LLDisplayNameUpdate>
    gHTTPRegistrationMessageDisplayNameUpdate(
		"/message/DisplayNameUpdate");
