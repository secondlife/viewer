/** 
 * @file llmapresponders.h
 * @brief Processes responses received for asset upload requests.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLNEWAGENTINVENTORYRESPONDER_H
#define LL_LLNEWAGENTINVENTORYRESPONDER_H

#include "llhttpclient.h"

class LLNewAgentInventoryResponder : public LLHTTPClient::Responder
{
public:
	LLNewAgentInventoryResponder(const LLUUID& uuid, const LLSD& post_data);
    void error(U32 statusNum, const std::string& reason);
	virtual void result(const LLSD& content);

private:
	LLUUID mUUID;
	LLSD mPostData;
};

#endif // LL_LLNEWAGENTINVENTORYRESPONDER_H
