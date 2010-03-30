/** 
 * @file llviewerdisplayname.cpp
 * @brief Wrapper for display name functionality
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

namespace LLViewerDisplayName
{
	// Fired when viewer receives server response to display name change
	set_name_signal_t sSetDisplayNameSignal;
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
		// JAMESDEBUG HACK for demos, fall back to prototype name service
		LLAvatarNameCache::setDisplayName(gAgent.getID(), display_name, slot);
		return;

		// this server does not support display names, report error
		//slot(false, "unsupported", LLSD());
		//return;
	}

	llinfos << "Set name POST to " << cap_url << llendl;

	// Record our caller for when the server sends back a reply
	sSetDisplayNameSignal.connect(slot);

	// POST the requested change.  The sim will not send a response back to
	// this request directly, rather it will send a separate message after it
	// communicates with the back-end.
	LLSD body;
	body["display_name"] = display_name;
	LLHTTPClient::post(cap_url, body, new LLSetDisplayNameResponder);
}

class LLSetDisplayNameReply : public LLHTTPNode
{
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

		llinfos << "JAMESDEBUG LLSetDisplayNameReply status " << status
			<< " reason " << reason << llendl;

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

		llinfos << "JAMESDEBUG LLDisplayNameUpdate agent_id "
			<< agent_id << llendl;

		// force re-request of this agent's name data
		LLAvatarNameCache::erase(agent_id);

		// force name tag to update
		LLVOAvatar::invalidateNameTag(agent_id);
	}
};

LLHTTPRegistration<LLSetDisplayNameReply>
    gHTTPRegistrationMessageSetDisplayNameReply(
		"/message/SetDisplayNameReply");

LLHTTPRegistration<LLDisplayNameUpdate>
    gHTTPRegistrationMessageDisplayNameUpdate(
		"/message/DisplayNameUpdate");
