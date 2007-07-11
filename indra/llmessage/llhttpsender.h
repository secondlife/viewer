/** 
 * @file llhttpsender.h
 * @brief Abstracts details of sending messages via HTTP.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_HTTP_SENDER_H
#define LL_HTTP_SENDER_H

#include "llhttpclient.h"

class LLHost;
class LLSD;

class LLHTTPSender
{
 public:

	virtual ~LLHTTPSender();

	/** @brief Send message to host with body, call response when done */ 
	virtual void send(const LLHost& host, 
					  const char* message, const LLSD& body, 
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
