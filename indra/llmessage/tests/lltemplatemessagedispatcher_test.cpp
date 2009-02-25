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

#include "lltemplatemessagedispatcher.h"
#include "lltut.h"

#include "llhttpnode.h"
#include "llhost.h"
#include "message.h"
#include "llsd.h"

#include "llhost.cpp" // Needed for copy operator
#include "net.cpp" // Needed by LLHost.

LLMessageSystem * gMessageSystem = NULL;

// sensor test doubles
bool gClearRecvWasCalled = false;
void LLMessageSystem::clearReceiveState(void) 
{ 
	gClearRecvWasCalled = true; 
}

char gUdpDispatchedData[MAX_BUFFER_SIZE];
bool gUdpDispatchWasCalled = false;
BOOL LLTemplateMessageReader::readMessage(const U8* data,class LLHost const &) 
{ 
	gUdpDispatchWasCalled = true;
	strcpy(gUdpDispatchedData, reinterpret_cast<const char*>(data));
	return  true;
}

BOOL gValidateMessage = FALSE;
BOOL LLTemplateMessageReader::validateMessage(const U8*, S32 buffer_size, LLHost const &sender, bool trusted) 
{ 
	return gValidateMessage;
}

LLHost host;
const LLHost& LLMessageSystem::getSender() const 
{ 
	return host; 
}

const char* gBinaryTemplateData = "BINARYTEMPLATEDATA";
void fillVector(std::vector<U8>& vector_data, const char* data)
{
	vector_data.resize(strlen(data) + 1);
	strcpy(reinterpret_cast<char*>(&vector_data[0]), data);
}

namespace tut
{
		static LLTemplateMessageReader::message_template_number_map_t numberMap;

		struct LLTemplateMessageDispatcherData
		{
			LLTemplateMessageDispatcherData()
			{
				mMessageName = "MessageName";
				gUdpDispatchWasCalled = false;
				gClearRecvWasCalled = false;
				gValidateMessage = FALSE;
				mMessage["body"]["binary-template-data"] = std::vector<U8>();
			}

			LLSD mMessage;
			LLHTTPNode::ResponsePtr mResponsePtr;
			std::string mMessageName;
		};

	typedef test_group<LLTemplateMessageDispatcherData> factory;
	typedef factory::object object;
}

namespace
{
	tut::factory tf("LLTemplateMessageDispatcher test");
}

namespace tut
{
	// does an empty message stop processing?
	template<> template<>
	void object::test<1>()
	{
		LLTemplateMessageReader* pReader = NULL;
		LLTemplateMessageDispatcher t(*pReader);
		t.dispatch(mMessageName, mMessage, mResponsePtr);
		ensure(! gUdpDispatchWasCalled);
		ensure(! gClearRecvWasCalled);
	}

	// does the disaptch invoke the udp send method?
	template<> template<>
	void object::test<2>()
	{
		LLTemplateMessageReader* pReader = NULL;
		LLTemplateMessageDispatcher t(*pReader);
		gValidateMessage = TRUE;
		std::vector<U8> vector_data;
		fillVector(vector_data, gBinaryTemplateData);		
		mMessage["body"]["binary-template-data"] = vector_data;
		t.dispatch(mMessageName, mMessage, mResponsePtr);
		ensure("udp dispatch was called", gUdpDispatchWasCalled);
	}

	// what if the message wasn't valid? We would hope the message gets cleared!
	template<> template<>
	void object::test<3>()
	{
		LLTemplateMessageReader* pReader = NULL;
		LLTemplateMessageDispatcher t(*pReader);
		std::vector<U8> vector_data;
		fillVector(vector_data, gBinaryTemplateData);
		mMessage["body"]["binary-template-data"] = vector_data;
		gValidateMessage = FALSE;
		t.dispatch(mMessageName, mMessage, mResponsePtr);
		ensure("clear received message was called", gClearRecvWasCalled);
	}

	// is the binary data passed through correctly?
	template<> template<>
	void object::test<4>()
	{
		LLTemplateMessageReader* pReader = NULL;
		LLTemplateMessageDispatcher t(*pReader);
		gValidateMessage = TRUE;
		std::vector<U8> vector_data;
		fillVector(vector_data, gBinaryTemplateData);
		mMessage["body"]["binary-template-data"] = vector_data;
		t.dispatch(mMessageName, mMessage, mResponsePtr);
		ensure("data couriered correctly", strcmp(gBinaryTemplateData, gUdpDispatchedData) == 0);
	}
}

