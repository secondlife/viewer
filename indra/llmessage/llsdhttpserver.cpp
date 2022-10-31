/** 
 * @file llsdhttpserver.cpp
 * @brief Standard LLSD services
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
    
    virtual LLSD simpleGet() const
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
    
    virtual LLSD simplePost(const LLSD& params) const
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
        const LLSD& remainder = context[CONTEXT_REQUEST]["remainder"];
        
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

