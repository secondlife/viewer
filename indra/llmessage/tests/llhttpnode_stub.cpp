/** 
 * @file llhttpnode_stub.cpp
 * @brief STUB Implementation of classes for generic HTTP/LSL/REST handling.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llhttpnode.h"

const std::string CONTEXT_VERB("verb");
const std::string CONTEXT_REQUEST("request");
const std::string CONTEXT_WILDCARD("wildcard");
const std::string CONTEXT_PATH("path");
const std::string CONTEXT_QUERY_STRING("query-string");
const std::string CONTEXT_REMOTE_HOST("remote-host");
const std::string CONTEXT_REMOTE_PORT("remote-port");
const std::string CONTEXT_HEADERS("headers");
const std::string CONTEXT_RESPONSE("response");

/**
 * LLHTTPNode
 */
class LLHTTPNode::Impl
{
    // dummy
};

LLHTTPNode::LLHTTPNode(): impl(*new Impl) {}
LLHTTPNode::~LLHTTPNode() {}
LLSD LLHTTPNode::simpleGet() const { return LLSD(); }
LLSD LLHTTPNode::simplePut(const LLSD& input) const { return LLSD(); }
LLSD LLHTTPNode::simplePost(const LLSD& input) const { return LLSD(); }
LLSD LLHTTPNode::simpleDel(const LLSD&) const { return LLSD(); }
void LLHTTPNode::get(LLHTTPNode::ResponsePtr response, const LLSD& context) const {}
void LLHTTPNode::put(LLHTTPNode::ResponsePtr response, const LLSD& context, const LLSD& input) const {}
void LLHTTPNode::post(LLHTTPNode::ResponsePtr response, const LLSD& context, const LLSD& input) const {}
void LLHTTPNode::del(LLHTTPNode::ResponsePtr response, const LLSD& context) const {}
void  LLHTTPNode::options(ResponsePtr response, const LLSD& context) const {}
LLHTTPNode* LLHTTPNode::getChild(const std::string& name, LLSD& context) const { return NULL; }
bool LLHTTPNode::handles(const LLSD& remainder, LLSD& context) const { return false; }
bool LLHTTPNode::validate(const std::string& name, LLSD& context) const { return false; }
const LLHTTPNode* LLHTTPNode::traverse(const std::string& path, LLSD& context) const { return NULL; }
void LLHTTPNode::addNode(const std::string& path, LLHTTPNode* nodeToAdd) { }
LLSD LLHTTPNode::allNodePaths() const { return LLSD(); }
const LLHTTPNode* LLHTTPNode::rootNode() const { return NULL; }
const LLHTTPNode* LLHTTPNode::findNode(const std::string& name) const { return NULL; }

LLHTTPNode::Response::~Response(){}
void LLHTTPNode::Response::notFound(const std::string& message)
{
	status(404, message);
}
void LLHTTPNode::Response::notFound()
{
	status(404, "Not Found");
}
void LLHTTPNode::Response::methodNotAllowed()
{
	status(405, "Method Not Allowed");
}
void LLHTTPNode::Response::statusUnknownError(S32 code)
{
	status(code, "Unknown Error");
}

void LLHTTPNode::Response::status(S32 code, const std::string& message)
{
}

void LLHTTPNode::Response::addHeader(const std::string& name,const std::string& value) 
{
	mHeaders[name] = value;
}
void LLHTTPNode::describe(Description& desc) const { }


const LLChainIOFactory* LLHTTPNode::getProtocolHandler() const { return NULL; }


LLHTTPRegistrar::NodeFactory::~NodeFactory() { }

void LLHTTPRegistrar::registerFactory(
    const std::string& path, NodeFactory& factory) {}
void LLHTTPRegistrar::buildAllServices(LLHTTPNode& root) {}


