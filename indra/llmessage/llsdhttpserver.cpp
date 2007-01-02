/** 
 * @file llsdhttpserver.cpp
 * @brief Standard LLSD services
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llsdhttpserver.h"

#include "llhttpnode.h"

/** 
 * This module implements common services that should be included
 * in all server URL trees.  These services facilitate debugging and
 * introsepction.
 */

void LLHTTPStandardServices::useServices()
{
	/*
		Having this function body here, causes the classes and globals in this
		file to be linked into any program that uses the llmessage library.
	*/
}



class LLHTTPHelloService : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("says hello");
		desc.getAPI();
		desc.output("\"hello\"");
		desc.source(__FILE__, __LINE__);
	}
    
	virtual LLSD get() const
	{
		LLSD result = "hello";
		return result;
	}
};

LLHTTPRegistration<LLHTTPHelloService>
	gHTTPRegistrationWebHello("/web/hello");



class LLHTTPEchoService : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("echo input");
		desc.postAPI();
		desc.input("<any>");
		desc.output("<the input>");
		desc.source(__FILE__, __LINE__);
	}
	
    virtual LLSD post(const LLSD& params) const
	{
		return params;
	}
};

LLHTTPRegistration<LLHTTPEchoService>
	gHTTPRegistrationWebEcho("/web/echo");



class LLAPIService : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("information about the URLs this server supports");
		desc.getAPI();
		desc.output("a list of URLs supported");
		desc.source(__FILE__, __LINE__);
	}

	virtual bool handles(const LLSD& remainder, LLSD& context) const
	{
		return followRemainder(remainder) != NULL;
	}

    virtual void get(ResponsePtr response, const LLSD& context) const
	{
		const LLSD& remainder = context["request"]["remainder"];
		
		if (remainder.size() > 0)
		{
			const LLHTTPNode* node = followRemainder(remainder);
			if (!node)
			{
				response->notFound();
				return;
			}
			
			Description desc;
			node->describe(desc);
			response->result(desc.getInfo());
			return;
		}

		response->result(rootNode()->allNodePaths());
	}

private:
	const LLHTTPNode* followRemainder(const LLSD& remainder) const
	{
		const LLHTTPNode* node = rootNode();
		
		LLSD::array_const_iterator i = remainder.beginArray();
		LLSD::array_const_iterator end = remainder.endArray();
		for (; node  &&  i != end; ++i)
		{
			node = node->findNode(*i);
		}
		
		return node;
	}
};

LLHTTPRegistration<LLAPIService>
	gHTTPRegistrationWebServerApi("/web/server/api");

