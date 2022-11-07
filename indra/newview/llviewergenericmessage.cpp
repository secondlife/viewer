/** 
 * @file llviewergenericmessage.cpp
 * @brief Handle processing of "generic messages" which contain short lists of strings.
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewergenericmessage.h"

#include "lldispatcher.h"
#include "lluuid.h"
#include "message.h"

#include "llagent.h"


LLDispatcher gGenericDispatcher;


void send_generic_message(const std::string& method,
                          const std::vector<std::string>& strings,
                          const LLUUID& invoice)
{
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessage("GenericMessage");
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
    msg->nextBlock("MethodData");
    msg->addString("Method", method);
    msg->addUUID("Invoice", invoice);
    if(strings.empty())
    {
        msg->nextBlock("ParamList");
        msg->addString("Parameter", NULL);
    }
    else
    {
        std::vector<std::string>::const_iterator it = strings.begin();
        std::vector<std::string>::const_iterator end = strings.end();
        for(; it != end; ++it)
        {
            msg->nextBlock("ParamList");
            msg->addString("Parameter", *it);
        }
    }
    gAgent.sendReliableMessage();
}

void process_generic_message(LLMessageSystem* msg, void**)
{
    LLUUID agent_id;
    msg->getUUID("AgentData", "AgentID", agent_id);
    if (agent_id != gAgent.getID())
    {
        LL_WARNS() << "GenericMessage for wrong agent" << LL_ENDL;
        return;
    }

    std::string request;
    LLUUID invoice;
    LLDispatcher::sparam_t strings;
    LLDispatcher::unpackMessage(msg, request, invoice, strings);

    if(!gGenericDispatcher.dispatch(request, invoice, strings))
    {
        LL_WARNS() << "GenericMessage " << request << " failed to dispatch" 
            << LL_ENDL;
    }
}

void process_large_generic_message(LLMessageSystem* msg, void**)
{
    LLUUID agent_id;
    msg->getUUID("AgentData", "AgentID", agent_id);
    if (agent_id != gAgent.getID())
    {
        LL_WARNS() << "GenericMessage for wrong agent" << LL_ENDL;
        return;
    }

    std::string request;
    LLUUID invoice;
    LLDispatcher::sparam_t strings;
    LLDispatcher::unpackLargeMessage(msg, request, invoice, strings);

    if (!gGenericDispatcher.dispatch(request, invoice, strings))
    {
        LL_WARNS() << "GenericMessage " << request << " failed to dispatch"
            << LL_ENDL;
    }
}
