/** 
 * @file llhttpsender.cpp
 * @brief Abstracts details of sending messages via HTTP.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
