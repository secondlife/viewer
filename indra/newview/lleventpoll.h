/** 
 * @file lleventpoll.h
 * @brief LLEvDescription of the LLEventPoll class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLEVENTPOLL_H
#define LL_LLEVENTPOLL_H

#include "llhttpclient.h"

class LLEventPoll
	///< implements the viewer side of server-to-viewer pushed events.
{
public:
	LLEventPoll(const std::string& pollURL, const LLHost& sender);
		///< Start polling the URL.

	virtual ~LLEventPoll();
		///< will stop polling, cancelling any poll in progress.


private:
	LLHTTPClient::ResponderPtr mImpl;
};


#endif // LL_LLEVENTPOLL_H
