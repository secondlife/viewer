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

#include "../lltrustedmessageservice.h"

#include "../llhost.cpp"
#include "../net.cpp"

#include "message.h"
#include "llmessageconfig.h"
#include "../tests/llhttpnode_stub.cpp"
#include "llpounceable.h"

LLPounceable<LLMessageSystem*, LLPounceableStatic> gMessageSystem;

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
    using tut_compat::ensure;
    using tut_compat::ensure_equals;

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
}

TUT_SUITE("LLTrustedMessageServiceData")
{
    TUT_CASE("LLTrustedMessageServiceData::object_test_1")
    {
        using namespace tut;
        LLTrustedMessageServiceData data;
        LLHTTPNode::ResponsePtr response;
        LLSD input;
        LLSD context;
        LLTrustedMessageService adapter;
        adapter.post(response, context, input);
        ensure_equals(messageDispatched, true);
        ensure(lastLLSD.has("body"));
    }

    TUT_CASE("LLTrustedMessageServiceData::object_test_2")
    {
        using namespace tut;
        LLTrustedMessageServiceData data;
        LLHTTPNode::ResponsePtr response;
        LLSD input;
        input["binary-template-data"] = "10001010110";
        LLSD context;
        LLTrustedMessageService adapter;

        adapter.post(response, context, input);
        ensure("check template-binary-data message was dispatched as binary", messageDispatchedAsBinary);
        ensure_equals(lastLLSD["body"]["binary-template-data"].asString(), "10001010110");
    }
}
