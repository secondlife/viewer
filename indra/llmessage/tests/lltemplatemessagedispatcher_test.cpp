/**
 * @file lltrustedmessageservice_test.cpp
 * @brief LLTrustedMessageService unit tests
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "lltemplatemessagedispatcher.h"
#include "lltut.h"

#include "llhttpnode.h"
#include "llhost.h"
#include "message.h"
#include "llsd.h"
#include "llpounceable.h"

#include "llhost.cpp" // Needed for copy operator
#include "net.cpp" // Needed by LLHost.

LLPounceable<LLMessageSystem*, LLPounceableStatic> gMessageSystem;

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
    tut::factory tf("LLTemplateMessageDispatcher");
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

