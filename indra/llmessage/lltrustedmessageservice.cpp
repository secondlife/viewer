/**
 * @file lltrustedmessageservice.cpp
 * @brief LLTrustedMessageService implementation
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

#include "linden_common.h"

#include "lltrustedmessageservice.h"
#include "llhost.h"
#include "llmessageconfig.h"
#include "message.h"
#include "llhttpconstants.h"


bool LLTrustedMessageService::validate(const std::string& name, LLSD& context)
const
{
    return true;
}

void LLTrustedMessageService::post(LLHTTPNode::ResponsePtr response,
                                   const LLSD& context,
                                   const LLSD& input) const
{
    std::string name = context[CONTEXT_REQUEST][CONTEXT_WILDCARD]["message-name"];
    std::string senderIP = context[CONTEXT_REQUEST][CONTEXT_REMOTE_HOST];
    std::string senderPort = context[CONTEXT_REQUEST][CONTEXT_HEADERS]
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
                << sender << LL_ENDL;
        response->status(HTTP_FORBIDDEN, "Unknown or untrusted sender");
    }
    else
    {
        gMessageSystem->receivedMessageFromTrustedSender();
        if (input.has("binary-template-data"))
        {
            LL_INFOS() << "Dispatching template: " << input << LL_ENDL;
            // try and send this message using udp dispatch
            LLMessageSystem::dispatchTemplate(name, message_data, response);
        }
        else
        {
            LL_INFOS() << "Dispatching without template: " << input << LL_ENDL;
            LLMessageSystem::dispatch(name, message_data, response);
        }
    }
}
