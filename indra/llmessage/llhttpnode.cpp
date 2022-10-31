/** 
 * @file llhttpnode.cpp
 * @brief Implementation of classes for generic HTTP/LSL/REST handling.
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
#include "llhttpnode.h"

#include <boost/tokenizer.hpp>

#include "llstl.h"
#include "llhttpconstants.h"
#include "llexception.h"

const std::string CONTEXT_HEADERS("headers");
const std::string CONTEXT_PATH("path");
const std::string CONTEXT_QUERY_STRING("query-string");
const std::string CONTEXT_REQUEST("request");
const std::string CONTEXT_RESPONSE("response");
const std::string CONTEXT_VERB("verb");
const std::string CONTEXT_WILDCARD("wildcard");

/**
 * LLHTTPNode
 */
 
class LLHTTPNode::Impl
{
public:
    typedef std::map<std::string, LLHTTPNode*> ChildMap;
    
    ChildMap mNamedChildren;
    LLHTTPNode* mWildcardChild;
    std::string mWildcardName;
    std::string mWildcardKey;
    LLHTTPNode* mParentNode;
    
    Impl() : mWildcardChild(NULL), mParentNode(NULL) { }
    
    LLHTTPNode* findNamedChild(const std::string& name) const;
};


LLHTTPNode* LLHTTPNode::Impl::findNamedChild(const std::string& name) const
{
    LLHTTPNode* child = get_ptr_in_map(mNamedChildren, name);

    if (!child  &&  ((name[0] == '*') || (name == mWildcardName)))
    {
        child = mWildcardChild;
    }
    
    return child;
}


LLHTTPNode::LLHTTPNode()
    : impl(* new Impl)
{
}

// virtual
LLHTTPNode::~LLHTTPNode()
{
    std::for_each(impl.mNamedChildren.begin(), impl.mNamedChildren.end(), DeletePairedPointer());
    impl.mNamedChildren.clear();

    delete impl.mWildcardChild;
    
    delete &impl;
}


namespace {
    struct NotImplemented: public LLException
    {
        NotImplemented(): LLException("LLHTTPNode::NotImplemented") {}
    };
}

// virtual
LLSD LLHTTPNode::simpleGet() const
{
    LLTHROW(NotImplemented());
}

// virtual
LLSD LLHTTPNode::simplePut(const LLSD& input) const
{
    LLTHROW(NotImplemented());
}

// virtual
LLSD LLHTTPNode::simplePost(const LLSD& input) const
{
    LLTHROW(NotImplemented());
}


// virtual
void LLHTTPNode::get(LLHTTPNode::ResponsePtr response, const LLSD& context) const
{
    LL_PROFILE_ZONE_SCOPED;
    try
    {
        response->result(simpleGet());
    }
    catch (NotImplemented&)
    {
        response->methodNotAllowed();
    }
}

// virtual
void LLHTTPNode::put(LLHTTPNode::ResponsePtr response, const LLSD& context, const LLSD& input) const
{
    LL_PROFILE_ZONE_SCOPED;
    try
    {
        response->result(simplePut(input));
    }
    catch (NotImplemented&)
    {
        response->methodNotAllowed();
    }
}

// virtual
void LLHTTPNode::post(LLHTTPNode::ResponsePtr response, const LLSD& context, const LLSD& input) const
{
    LL_PROFILE_ZONE_SCOPED;
    try
    {
        response->result(simplePost(input));
    }
    catch (NotImplemented&)
    {
        response->methodNotAllowed();
    }
}

// virtual
void LLHTTPNode::del(LLHTTPNode::ResponsePtr response, const LLSD& context) const
{
    LL_PROFILE_ZONE_SCOPED;
    try
    {
    response->result(simpleDel(context));
    }
    catch (NotImplemented&)
    {
    response->methodNotAllowed();
    }

}

// virtual
LLSD LLHTTPNode::simpleDel(const LLSD&) const
{
    LLTHROW(NotImplemented());
}

// virtual
void  LLHTTPNode::options(ResponsePtr response, const LLSD& context) const
{
    //LL_INFOS() << "options context: " << context << LL_ENDL;
    LL_DEBUGS("LLHTTPNode") << "context: " << context << LL_ENDL;

    // default implementation constructs an url to the documentation.
    // *TODO: Check for 'Host' header instead of 'host' header?
    std::string host(
        context[CONTEXT_REQUEST][CONTEXT_HEADERS][HTTP_IN_HEADER_HOST].asString());
    if(host.empty())
    {
        response->status(HTTP_BAD_REQUEST, "Bad Request -- need Host header");
        return;
    }
    std::ostringstream ostr;
    ostr << "http://" << host << "/web/server/api";
    ostr << context[CONTEXT_REQUEST][CONTEXT_PATH].asString();
    static const std::string DOC_HEADER("X-Documentation-URL");
    response->addHeader(DOC_HEADER, ostr.str());
    response->status(HTTP_OK, "OK");
}


// virtual
LLHTTPNode* LLHTTPNode::getChild(const std::string& name, LLSD& context) const
{
    LLHTTPNode* namedChild = get_ptr_in_map(impl.mNamedChildren, name);
    if (namedChild)
    {
        return namedChild;
    }
    
    if (impl.mWildcardChild
    &&  impl.mWildcardChild->validate(name, context))
    {
        context[CONTEXT_REQUEST][CONTEXT_WILDCARD][impl.mWildcardKey] = name;
        return impl.mWildcardChild;
    }
    
    return NULL;
}


// virtual
bool LLHTTPNode::handles(const LLSD& remainder, LLSD& context) const
{
    return remainder.size() == 0;
}

// virtual
bool LLHTTPNode::validate(const std::string& name, LLSD& context) const
{
    return false;
}

const LLHTTPNode* LLHTTPNode::traverse(
    const std::string& path, LLSD& context) const
{
    typedef boost::tokenizer< boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep("/", "", boost::drop_empty_tokens);
    tokenizer tokens(path, sep);
    tokenizer::iterator iter = tokens.begin();
    tokenizer::iterator end = tokens.end();

    const LLHTTPNode* node = this;
    for(; iter != end; ++iter)
    {
        LLHTTPNode* child = node->getChild(*iter, context);
        if(!child) 
        {
            LL_DEBUGS() << "LLHTTPNode::traverse: Couldn't find '" << *iter << "'" << LL_ENDL;
            break; 
        }
        LL_DEBUGS() << "LLHTTPNode::traverse: Found '" << *iter << "'" << LL_ENDL;

        node = child;
    }

    LLSD& remainder = context[CONTEXT_REQUEST]["remainder"];
    for(; iter != end; ++iter)
    {
        remainder.append(*iter);
    }

    return node->handles(remainder, context) ? node : NULL;
}



void LLHTTPNode::addNode(const std::string& path, LLHTTPNode* nodeToAdd)
{
    typedef boost::tokenizer< boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep("/", "", boost::drop_empty_tokens);
    tokenizer tokens(path, sep);
    tokenizer::iterator iter = tokens.begin();
    tokenizer::iterator end = tokens.end();

    LLHTTPNode* node = this;
    for(; iter != end; ++iter)
    {
        LLHTTPNode* child = node->impl.findNamedChild(*iter);
        if (!child) { break; }
        node = child;
    }
    
    if (iter == end)
    {
        LL_WARNS() << "LLHTTPNode::addNode: already a node that handles "
            << path << LL_ENDL;
        return;
    }
    
    while (true)
    {
        std::string pathPart = *iter;
        
        ++iter;
        bool lastOne = iter == end;
        
        LLHTTPNode* nextNode = lastOne ? nodeToAdd : new LLHTTPNode();
        
        switch (pathPart[0])
        {
            case '<':
                // *NOTE: This should really validate that it is of
                // the proper form: <wildcardkey> so that the substr()
                // generates the correct key name.
                node->impl.mWildcardChild = nextNode;
                node->impl.mWildcardName = pathPart;
                if(node->impl.mWildcardKey.empty())
                {
                    node->impl.mWildcardKey = pathPart.substr(
                        1,
                        pathPart.size() - 2);
                }
                break;
            case '*':
                node->impl.mWildcardChild = nextNode;
                if(node->impl.mWildcardName.empty())
                {
                    node->impl.mWildcardName = pathPart;
                }
                break;
            
            default:
                node->impl.mNamedChildren[pathPart] = nextNode;
        }
        nextNode->impl.mParentNode = node;

        if (lastOne) break;
        node = nextNode;
    }
}

static void append_node_paths(LLSD& result,
    const std::string& name, const LLHTTPNode* node)
{
    result.append(name);
    
    LLSD paths = node->allNodePaths();
    LLSD::array_const_iterator i = paths.beginArray();
    LLSD::array_const_iterator end = paths.endArray();
    
    for (; i != end; ++i)
    {
        result.append(name + "/" + (*i).asString());
    }
}

LLSD LLHTTPNode::allNodePaths() const
{
    LLSD result;
    
    Impl::ChildMap::const_iterator i = impl.mNamedChildren.begin();
    Impl::ChildMap::const_iterator end = impl.mNamedChildren.end();
    for (; i != end; ++i)
    {
        append_node_paths(result, i->first, i->second);
    }
    
    if (impl.mWildcardChild)
    {
        append_node_paths(result, impl.mWildcardName, impl.mWildcardChild);
    }
    
    return result;
}


const LLHTTPNode* LLHTTPNode::rootNode() const
{
    const LLHTTPNode* node = this;
    
    while (true)
    {
        const LLHTTPNode* next = node->impl.mParentNode;
        if (!next)
        {
            return node;
        }
        node = next;
    }
}


const LLHTTPNode* LLHTTPNode::findNode(const std::string& name) const
{
    return impl.findNamedChild(name);
}

LLHTTPNode::Response::~Response()
{
}

void LLHTTPNode::Response::statusUnknownError(S32 code)
{
    status(code, "Unknown Error");
}

void LLHTTPNode::Response::notFound(const std::string& message)
{
    status(HTTP_NOT_FOUND, message);
}

void LLHTTPNode::Response::notFound()
{
    status(HTTP_NOT_FOUND, "Not Found");
}

void LLHTTPNode::Response::methodNotAllowed()
{
    status(HTTP_METHOD_NOT_ALLOWED, "Method Not Allowed");
}

void LLHTTPNode::Response::addHeader(
    const std::string& name,
    const std::string& value)
{
    mHeaders[name] = value;
}

void LLHTTPNode::describe(Description& desc) const
{
    desc.shortInfo("unknown service (missing describe() method)");
}


const LLChainIOFactory* LLHTTPNode::getProtocolHandler() const
{
    return NULL;
}



namespace
{
    typedef std::map<std::string, LLHTTPRegistrar::NodeFactory*>  FactoryMap;
    
    FactoryMap& factoryMap()
    {
        static FactoryMap theMap;
        return theMap;
    }
}

LLHTTPRegistrar::NodeFactory::~NodeFactory() { }

void LLHTTPRegistrar::registerFactory(
    const std::string& path, NodeFactory& factory)
{
    factoryMap()[path] = &factory;
}

void LLHTTPRegistrar::buildAllServices(LLHTTPNode& root)
{
    const FactoryMap& map = factoryMap();
    
    FactoryMap::const_iterator i = map.begin();
    FactoryMap::const_iterator end = map.end();
    for (; i != end; ++i)
    {
        LL_DEBUGS("AppInit") << "LLHTTPRegistrar::buildAllServices adding node for path "
            << i->first << LL_ENDL;
        
        root.addNode(i->first, i->second->build());
    }
}

LLPointer<LLSimpleResponse> LLSimpleResponse::create()
{
    return new LLSimpleResponse();
}

LLSimpleResponse::~LLSimpleResponse()
{
}

void LLSimpleResponse::result(const LLSD& result)
{
    status(HTTP_OK, "OK");
}

void LLSimpleResponse::extendedResult(S32 code, const std::string& body, const LLSD& headers)
{
    status(code,body);
}

void LLSimpleResponse::extendedResult(S32 code, const LLSD& r, const LLSD& headers)
{
    status(code,"(LLSD)");
}

void LLSimpleResponse::status(S32 code, const std::string& message)
{
    mCode = code;
    mMessage = message;
}

void LLSimpleResponse::print(std::ostream& out) const
{
    out << mCode << " " << mMessage;
}


std::ostream& operator<<(std::ostream& out, const LLSimpleResponse& resp)
{
    resp.print(out);
    return out;
}
