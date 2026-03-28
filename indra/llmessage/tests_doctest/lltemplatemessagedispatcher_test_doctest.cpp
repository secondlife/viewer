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

#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"

#include "../lltemplatemessagedispatcher.h"

#include "../llhttpnode.h"
#include "../llhost.h"
#include "../message.h"
#include "llsd.h"
#include "llpounceable.h"

#include "../llhost.cpp" // Needed for copy operator
#include "../net.cpp" // Needed by LLHost.

LLPounceable<LLMessageSystem*, LLPounceableStatic> gMessageSystem;

bool gClearRecvWasCalled = false;
void LLMessageSystem::clearReceiveState(void)
{
    gClearRecvWasCalled = true;
}

char gUdpDispatchedData[MAX_BUFFER_SIZE];
bool gUdpDispatchWasCalled = false;
bool LLTemplateMessageReader::readMessage(const U8* data, class LLHost const&)
{
    gUdpDispatchWasCalled = true;
    strcpy(gUdpDispatchedData, reinterpret_cast<const char*>(data));
    return true;
}

bool gValidateMessage = false;
bool LLTemplateMessageReader::validateMessage(const U8*, S32, LLHost const&, bool)
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
    using tut_compat::ensure;

    static LLTemplateMessageReader::message_template_number_map_t numberMap;

    struct LLTemplateMessageDispatcherData
    {
        LLTemplateMessageDispatcherData()
        {
            mMessageName = "MessageName";
            gUdpDispatchWasCalled = false;
            gClearRecvWasCalled = false;
            gValidateMessage = false;
            mMessage["body"]["binary-template-data"] = std::vector<U8>();
        }

        LLSD mMessage;
        LLHTTPNode::ResponsePtr mResponsePtr;
        std::string mMessageName;
    };
}

TUT_SUITE("LLTemplateMessageDispatcher")
{
    TUT_CASE("LLTemplateMessageDispatcher::dispatch_test_1")
    {
        using namespace tut;
        LLTemplateMessageDispatcherData data;
        LLTemplateMessageReader* pReader = NULL;
        LLTemplateMessageDispatcher t(*pReader);
        t.dispatch(data.mMessageName, data.mMessage, data.mResponsePtr);
        ensure(!gUdpDispatchWasCalled);
        ensure(!gClearRecvWasCalled);
    }

    TUT_CASE("LLTemplateMessageDispatcher::dispatch_test_2")
    {
        using namespace tut;
        LLTemplateMessageDispatcherData data;
        LLTemplateMessageReader* pReader = NULL;
        LLTemplateMessageDispatcher t(*pReader);
        gValidateMessage = true;
        std::vector<U8> vector_data;
        fillVector(vector_data, gBinaryTemplateData);
        data.mMessage["body"]["binary-template-data"] = vector_data;
        t.dispatch(data.mMessageName, data.mMessage, data.mResponsePtr);
        ensure("udp dispatch was called", gUdpDispatchWasCalled);
    }

    TUT_CASE("LLTemplateMessageDispatcher::dispatch_test_3")
    {
        using namespace tut;
        LLTemplateMessageDispatcherData data;
        LLTemplateMessageReader* pReader = NULL;
        LLTemplateMessageDispatcher t(*pReader);
        std::vector<U8> vector_data;
        fillVector(vector_data, gBinaryTemplateData);
        data.mMessage["body"]["binary-template-data"] = vector_data;
        gValidateMessage = false;
        t.dispatch(data.mMessageName, data.mMessage, data.mResponsePtr);
        ensure("clear received message was called", gClearRecvWasCalled);
    }

    TUT_CASE("LLTemplateMessageDispatcher::dispatch_test_4")
    {
        using namespace tut;
        LLTemplateMessageDispatcherData data;
        LLTemplateMessageReader* pReader = NULL;
        LLTemplateMessageDispatcher t(*pReader);
        gValidateMessage = true;
        std::vector<U8> vector_data;
        fillVector(vector_data, gBinaryTemplateData);
        data.mMessage["body"]["binary-template-data"] = vector_data;
        t.dispatch(data.mMessageName, data.mMessage, data.mResponsePtr);
        ensure("data couriered correctly", strcmp(gBinaryTemplateData, gUdpDispatchedData) == 0);
    }
}
