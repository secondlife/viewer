/** 
 * @file llviewergenericmessage.h
 * @brief Handle processing of "generic messages" which contain short lists of strings.
 * @author James Cook
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLVIEWERGENERICMESSAGE_H
#define LLVIEWERGENERICMESSAGE_H

class LLUUID;
class LLDispatcher;


void send_generic_message(const char* method,
						  const std::vector<std::string>& strings,
						  const LLUUID& invoice = LLUUID::null);

void process_generic_message(LLMessageSystem* msg, void**);


extern LLDispatcher gGenericDispatcher;

#endif
