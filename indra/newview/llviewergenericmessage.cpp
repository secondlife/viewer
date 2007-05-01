/** 
 * @file llviewergenericmessage.cpp
 * @brief Handle processing of "generic messages" which contain short lists of strings.
 * @author James Cook
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewergenericmessage.h"

#include "lldispatcher.h"
#include "lluuid.h"
#include "message.h"

#include "llagent.h"


LLDispatcher gGenericDispatcher;


void send_generic_message(const char* method,
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
			msg->addString("Parameter", (*it).c_str());
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
		llwarns << "GenericMessage for wrong agent" << llendl;
		return;
	}

	std::string request;
	LLUUID invoice;
	LLDispatcher::sparam_t strings;
	LLDispatcher::unpackMessage(msg, request, invoice, strings);

	if(!gGenericDispatcher.dispatch(request, invoice, strings))
	{
		llwarns << "GenericMessage " << request << " failed to dispatch" 
			<< llendl;
	}
}
