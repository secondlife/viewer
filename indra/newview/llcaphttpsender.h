/** 
 * @file llcaphttpsender.h
 * @brief Abstracts details of sending messages via the
 *        UntrustedMessage capability.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
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
					  const char* message, const LLSD& body, 
					  LLHTTPClient::ResponderPtr response) const;

private:
	std::string mCap;
};

#endif // LL_CAP_HTTP_SENDER_H
