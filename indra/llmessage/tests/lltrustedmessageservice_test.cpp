/**
 * @file lltrustedmessageservice_test.cpp
 * @brief LLTrustedMessageService unit tests
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "lltrustedmessageservice.h"
#include "../test/lltut.h"

#include "llhost.cpp" // LLHost is a value type for test purposes.
#include "net.cpp" // Needed by LLHost.

#include "message.h"
#include "llmessageconfig.h"

LLMessageSystem* gMessageSystem = NULL;

LLMessageConfig::SenderTrust
LLMessageConfig::getSenderTrustedness(const std::string& msg_name)
{
	return LLMessageConfig::NOT_SET;
}

void LLMessageSystem::receivedMessageFromTrustedSender()
{
}

bool LLMessageSystem::isTrustedSender(const LLHost& host) const
{
	return false;
}

bool LLMessageSystem::isTrustedMessage(const std::string& name) const
{
	return false;
}

bool messageDispatched = false;
bool messageDispatchedAsBinary = false;
LLSD lastLLSD;
std::string lastMessageName;

void LLMessageSystem::dispatch(const std::string& msg_name,
							   const LLSD& message,
							   LLHTTPNode::ResponsePtr responsep)
{
	messageDispatched = true;
	lastLLSD = message;
	lastMessageName = msg_name;
}

void LLMessageSystem::dispatchTemplate(const std::string& msg_name,
						 const LLSD& message,
						 LLHTTPNode::ResponsePtr responsep)
{
	lastLLSD = message;
	lastMessageName = msg_name;
	messageDispatchedAsBinary = true;
}

namespace tut
{
	    struct LLTrustedMessageServiceData
		{
			LLTrustedMessageServiceData()
			{
				LLSD emptyLLSD;
				lastLLSD = emptyLLSD;
				lastMessageName = "uninitialised message name";
				messageDispatched = false;
				messageDispatchedAsBinary = false;
			}
		};

	typedef test_group<LLTrustedMessageServiceData> factory;
	typedef factory::object object;
}

namespace
{
	tut::factory tf("LLTrustedMessageServiceData test");
}

namespace tut
{
	// characterisation tests

	// 1) test that messages get forwarded with names etc. as current behaviour (something like LLMessageSystem::dispatch(name, data...)

	// test llsd messages are sent as normal using LLMessageSystem::dispatch() (eventually)
	template<> template<>
	void object::test<1>()
	{
		LLHTTPNode::ResponsePtr response;
		LLSD input;
		LLSD context;
		LLTrustedMessageService adapter;
		adapter.post(response, context, input);
		// test original ting got called wit nowt, ya get me blood?
		ensure_equals(messageDispatched, true);
		ensure(lastLLSD.has("body"));
	}

	// test that llsd wrapped binary-template-data messages are 
	// sent via LLMessageSystem::binaryDispatch() or similar
	template<> template<>
	void object::test<2>()
	{
		LLHTTPNode::ResponsePtr response;
		LLSD input;
		input["binary-template-data"] = "10001010110"; //make me a message here.
		LLSD context;
		LLTrustedMessageService adapter;

		adapter.post(response, context, input);
		ensure("check template-binary-data message was dispatched as binary", messageDispatchedAsBinary);
		ensure_equals(lastLLSD["body"]["binary-template-data"].asString(),  "10001010110");
		// test somit got called with "10001010110" (something like LLMessageSystem::dispatchTemplate(blah))
	}
}
