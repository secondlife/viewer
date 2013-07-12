/** 
 * @file llcaphttpsender.h
 * @brief Abstracts details of sending messages via the
 *        UntrustedMessage capability.
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

#ifndef LL_CAP_HTTP_SENDER_H
#define LL_CAP_HTTP_SENDER_H

#include "llhttpsender.h"

class LLCapHTTPSender : public LLHTTPSender
{
public:
	LLCapHTTPSender(const std::string& cap);

	/** @brief Send message via UntrustedMessage capability with body,
		call response when done */ 
	virtual void send(const LLHost& host, 
					  const std::string& message, const LLSD& body, 
					  LLHTTPClient::ResponderPtr response) const;

private:
	std::string mCap;
};

#endif // LL_CAP_HTTP_SENDER_H
