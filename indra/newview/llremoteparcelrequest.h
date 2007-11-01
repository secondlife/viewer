/** 
 * @file lleventpoll.h
 * @brief LLEvDescription of the LLEventPoll class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLREMOTEPARCELREQUEST_H
#define LL_LLREMOTEPARCELREQUEST_H

#include "llhttpclient.h"
#include "llview.h"

class LLRemoteParcelRequestResponder : public LLHTTPClient::Responder
{
public:
	LLRemoteParcelRequestResponder(LLViewHandle place_panel_handle);
	//If we get back a normal response, handle it here
	virtual void result(const LLSD& content);
	//If we get back an error (not found, etc...), handle it here
	virtual void error(U32 status, const std::string& reason);

protected:
	LLViewHandle mPlacePanelHandle;
};

#endif // LL_LLREMOTEPARCELREQUEST_H
