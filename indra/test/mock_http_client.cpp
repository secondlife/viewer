/** 
 * @file mock_http_client.cpp
 * @brief Framework for testing HTTP requests
 * Copyright (c) 2007, Linden Research, Inc.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llsdhttpserver.h"
#include "lliohttpserver.h"

namespace tut
{
	class SuccessNode : public LLHTTPNode
	{
	public:
		void get(ResponsePtr r, const LLSD& context) const
	   	{ 
	   		LLSD result;
			result["state"] = "complete";
	   		result["test"] = "test";
	   		r->result(result);
	   	}
		void post(ResponsePtr r, const LLSD& context, const LLSD& input) const
		{
			LLSD result;
			result["state"] = "complete";
			result["test"] = "test";
			r->result(result);
		}
	};

	class ErrorNode : public LLHTTPNode
	{
	public:
		void get(ResponsePtr r, const LLSD& context) const
			{ r->status(599, "Intentional error"); }
		void post(ResponsePtr r, const LLSD& context, const LLSD& input) const
			{ r->status(input["status"], input["reason"]); }
	};

	class TimeOutNode : public LLHTTPNode
	{
	public:
		void get(ResponsePtr r, const LLSD& context) const
		{
            /* do nothing, the request will eventually time out */ 
		}
	};

	LLSD storage;
	
	class LLSDStorageNode : public LLHTTPNode
	{
	public:
		LLSD get() const{ return storage; }
		LLSD put(const LLSD& value) const{ storage = value; return LLSD(); }
	};

	LLHTTPRegistration<LLSDStorageNode> gStorageNode("/test/storage");
	LLHTTPRegistration<SuccessNode>		gSuccessNode("/test/success");
	LLHTTPRegistration<ErrorNode>		gErrorNode("/test/error");
	LLHTTPRegistration<TimeOutNode>		gTimeOutNode("/test/timeout");
}
