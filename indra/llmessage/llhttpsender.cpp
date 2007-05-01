/** 
 * @file llhttpsender.cpp
 * @brief Abstracts details of sending messages via HTTP.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llhttpsender.h"

#include <map>
#include <sstream>

#include "llhost.h"
#include "llsd.h"

namespace
{
	typedef std::map<LLHost, LLHTTPSender*> SenderMap;
	static SenderMap senderMap;
}

//virtual 
LLHTTPSender::~LLHTTPSender()
{
}

//virtual 
void LLHTTPSender::send(const LLHost& host, const char* name, 
						const LLSD& body, 
						LLHTTPClient::ResponderPtr response) const
{
	// Default implementation inserts sender, message and sends HTTP POST
	std::ostringstream stream;
	stream << "http://" << host << "/trusted-message/" << name;
	llinfos << "LLHTTPSender::send: POST to " << stream.str() << llendl;
	LLHTTPClient::post(stream.str(), body, response);
}

//static 
void LLHTTPSender::setSender(const LLHost& host, LLHTTPSender* sender)
{
	llinfos << "LLHTTPSender::setSender " << host << llendl;
	senderMap[host] = sender;
}

//static
const LLHTTPSender& LLHTTPSender::getSender(const LLHost& host)
{
	static LLHTTPSender defaultSender;
	SenderMap::const_iterator iter = senderMap.find(host);
	if(iter == senderMap.end())
	{
		return defaultSender;
	}
	return *(iter->second);
}

//static 
void LLHTTPSender::clearSender(const LLHost& host)
{
	SenderMap::iterator iter = senderMap.find(host);
	if(iter != senderMap.end())
	{
		delete iter->second;
		senderMap.erase(iter);
	}
}
