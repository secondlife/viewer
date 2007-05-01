/** 
 * @file llcaphttpsender.cpp
 * @brief Abstracts details of sending messages via UntrustedMessage cap.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "linden_common.h"
#include "llcaphttpsender.h"

LLCapHTTPSender::LLCapHTTPSender(const std::string& cap) :
	mCap(cap)
{
}

//virtual 
void LLCapHTTPSender::send(const LLHost& host, const char* message, 
								  const LLSD& body, 
								  LLHTTPClient::ResponderPtr response) const
{
	llinfos << "LLCapHTTPSender::send: message " << message
			<< " to host " << host << llendl;
	LLSD llsd;
	llsd["message"] = message;
	llsd["body"] = body;
	LLHTTPClient::post(mCap, llsd, response);
}
