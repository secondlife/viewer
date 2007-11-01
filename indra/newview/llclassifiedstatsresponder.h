/** 
 * @file llclassifiedstatsrequest.h
 * @brief Responder class for classified stats request.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCLASSIFIEDSTATSRESPONDER_H
#define LL_LLCLASSIFIEDSTATSRESPONDER_H

#include "llhttpclient.h"
#include "llview.h"

class LLClassifiedStatsResponder : public LLHTTPClient::Responder
{
public:
	LLClassifiedStatsResponder(LLViewHandle classified_panel_handle);
	//If we get back a normal response, handle it here
	virtual void result(const LLSD& content);
	//If we get back an error (not found, etc...), handle it here
	virtual void error(U32 status, const std::string& reason);

protected:
	LLViewHandle mClassifiedPanelHandle;
};

#endif // LL_LLCLASSIFIEDSTATSRESPONDER_H
