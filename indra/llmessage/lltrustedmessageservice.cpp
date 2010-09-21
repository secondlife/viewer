/**
 * @file lltrustedmessageservice.cpp
 * @brief LLTrustedMessageService implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "lltrustedmessageservice.h"
#include "llhost.h"
#include "llmessageconfig.h"
#include "message.h"


bool LLTrustedMessageService::validate(const std::string& name, LLSD& context)
const
{
	return true;
}

void LLTrustedMessageService::post(LLHTTPNode::ResponsePtr response,
								   const LLSD& context,
								   const LLSD& input) const
{
	std::string name = context["request"]["wildcard"]["message-name"];
	std::string senderIP = context["request"]["remote-host"];
	std::string senderPort = context["request"]["headers"]
		["x-secondlife-udp-listen-port"];

	LLSD message_data;
	std::string sender = senderIP + ":" + senderPort;
	message_data["sender"] = sender;
	message_data["body"] = input;
	
	// untrusted senders should not have access to the trusted message
	// service, but this can happen in development, so check and warn
	LLMessageConfig::SenderTrust trust =
		LLMessageConfig::getSenderTrustedness(name);
	if ((trust == LLMessageConfig::TRUSTED ||
		 (trust == LLMessageConfig::NOT_SET &&
		  gMessageSystem->isTrustedMessage(name)))
		 && !gMessageSystem->isTrustedSender(LLHost(sender)))
	{
		LL_WARNS("Messaging") << "trusted message POST to /trusted-message/" 
				<< name << " from unknown or untrusted sender "
				<< sender << llendl;
		response->status(403, "Unknown or untrusted sender");
	}
	else
	{
		gMessageSystem->receivedMessageFromTrustedSender();
		if (input.has("binary-template-data"))
		{
			llinfos << "Dispatching template: " << input << llendl;
			// try and send this message using udp dispatch
			LLMessageSystem::dispatchTemplate(name, message_data, response);
		}
		else
		{
			llinfos << "Dispatching without template: " << input << llendl;
			LLMessageSystem::dispatch(name, message_data, response);
		}
	}
}
