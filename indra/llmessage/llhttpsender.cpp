/** 
 * @file llhttpsender.cpp
 * @brief Abstracts details of sending messages via HTTP.
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
	static LLHTTPSender* defaultSender = new LLHTTPSender();
}

//virtual 
LLHTTPSender::~LLHTTPSender()
{
}

//virtual 
void LLHTTPSender::send(const LLHost& host, const std::string& name, 
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
	SenderMap::const_iterator iter = senderMap.find(host);
	if(iter == senderMap.end())
	{
		return *defaultSender;
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

//static 
void LLHTTPSender::setDefaultSender(LLHTTPSender* sender)
{
	delete defaultSender;
	defaultSender = sender;
}
