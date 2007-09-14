/** 
 * @file llsrv.cpp
 * @brief Wrapper for DNS SRV record lookups
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llsrv.h"
#include "llares.h"

struct Responder : public LLAres::UriRewriteResponder
{
	std::vector<std::string> mUris;
	void rewriteResult(const std::vector<std::string> &uris) {
		for (size_t i = 0; i < uris.size(); i++)
		{
			llinfos << "[" << i << "] " << uris[i] << llendl;
		}
		mUris = uris;
	}
};

std::vector<std::string> LLSRV::rewriteURI(const std::string& uri)
{
	LLPointer<Responder> resp = new Responder;

	gAres->rewriteURI(uri, resp);
	gAres->processAll();
	return resp->mUris;
}
