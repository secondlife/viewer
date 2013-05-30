/** 
 * @file lliohttpserver.h
 * @brief Declaration of function for creating an HTTP wire server
 * @see LLIOServerSocket, LLPumpIO
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_LLIOHTTPSERVER_H
#define LL_LLIOHTTPSERVER_H

#include "llchainio.h"
#include "llhttpnode.h"

class LLPumpIO;

// common strings use for populating the context. bascally 'request',
// 'wildcard', and 'headers'.
extern const std::string CONTEXT_REQUEST;
extern const std::string CONTEXT_RESPONSE;
extern const std::string CONTEXT_VERB;
extern const std::string CONTEXT_HEADERS;
extern const std::string HTTP_VERB_GET;
extern const std::string HTTP_VERB_PUT;
extern const std::string HTTP_VERB_POST;
extern const std::string HTTP_VERB_DELETE;
extern const std::string HTTP_VERB_OPTIONS;

class LLIOHTTPServer
{
public:
	typedef void (*timing_callback_t)(const char* hashed_name, F32 time, void* data);

	static LLHTTPNode& create(apr_pool_t* pool, LLPumpIO& pump, U16 port);
	/**< Creates an HTTP wire server on the pump for the given TCP port.
	 *
	 *   Returns the root node of the new server.  Add LLHTTPNode instances
	 *   to this root.
	 *
	 *   Nodes that return NULL for getProtocolHandler(), will use the
	 *   default handler that interprets HTTP on the wire and converts
	 *   it into calls to get(), put(), post(), del() with appropriate
	 *   LLSD arguments and results.
	 *
	 *   To have nodes that implement some other wire protocol (XML-RPC
	 *   for example), use the helper templates below.
	 */
 
	static void createPipe(LLPumpIO::chain_t& chain, 
            const LLHTTPNode& root, const LLSD& ctx);
	/**< Create a pipe on the chain that handles HTTP requests.
	 *   The requests are served by the node tree given at root.
	 *
	 *   This is primarily useful for unit testing.
	 */

	static void setTimingCallback(timing_callback_t callback, void* data);
	/**< Register a callback function that will be called every time
	*    a GET, PUT, POST, or DELETE is handled.
	*
	* This is used to time the LLHTTPNode handler code, which often hits
	* the database or does other, slow operations. JC
	*/
};

/* @name Helper Templates
 *
 * These templates make it easy to create nodes that use thier own protocol
 * handlers rather than the default.  Typically, you subclass LLIOPipe to
 * implement the protocol, and then add a node using the templates:
 *
 * rootNode->addNode("thing", new LLHTTPNodeForPipe<LLThingPipe>);
 *
 * The templates are:
 *
 * 	LLChainIOFactoryForPipe
 * 		- a simple factory that builds instances of a pipe
 *
 * 	LLHTTPNodeForFacotry
 * 		- a HTTP node that uses a factory as the protocol handler
 *
 * 	LLHTTPNodeForPipe
 * 		- a HTTP node that uses a simple factory based on a pipe
 */
//@{

template<class Pipe>
class LLChainIOFactoryForPipe : public LLChainIOFactory
{
public:
	virtual bool build(LLPumpIO::chain_t& chain, LLSD context) const
	{   
		chain.push_back(LLIOPipe::ptr_t(new Pipe));
		return true;
	}
};

template<class Factory>
class LLHTTPNodeForFactory : public LLHTTPNode
{
public:
	const LLChainIOFactory* getProtocolHandler() const
		{ return &mProtocolHandler; }

private:
	Factory mProtocolHandler;
};

//@}


template<class Pipe>
class LLHTTPNodeForPipe : public LLHTTPNodeForFactory<
						  			LLChainIOFactoryForPipe<Pipe> >
{
};


#endif // LL_LLIOHTTPSERVER_H

