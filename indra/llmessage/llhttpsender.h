/** 
 * @file llhttpsender.h
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

#ifndef LL_HTTP_SENDER_H
#define LL_HTTP_SENDER_H

#include "llhttpclient.h"

class LLHost;
class LLSD;

class LLHTTPSender : public LLThreadSafeRefCount
{
 public:

	virtual ~LLHTTPSender();

	/** @brief Send message to host with body, call response when done */ 
	virtual void send(const LLHost& host, 
					  const std::string& message, const LLSD& body, 
					  LLHTTPClient::ResponderPtr response) const;

	/** @brief Set sender for host, takes ownership of sender. */
	static void setSender(const LLHost& host, LLHTTPSender* sender);

	/** @brief Get sender for host, retains ownership of returned sender. */
	static const LLHTTPSender& getSender(const LLHost& host);
	
	/** @brief Clear sender for host. */
	static void clearSender(const LLHost& host);

	/** @brief Set default sender, takes ownership of sender. */
	static void setDefaultSender(LLHTTPSender* sender);
};

#endif // LL_HTTP_SENDER_H
